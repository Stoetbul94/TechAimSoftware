// Wind Map — analysis preparation cost (Stage 6.1.1, UI-WIND-003).
//
// MEASUREMENT, not speculation. These time the C++ half of the analysis path
// on a realistic 44-shot session and assert the caching contract the UX
// redesign depends on:
//
//   · the engine runs ONCE per completed session, not per page or per
//     position change;
//   · a cached model is returned without re-running anything;
//   · the cost is small enough that the on-screen delay cannot be blamed on
//     the C++ side without evidence.
//
// Timings are PRINTED so a regression is visible even when the assertion
// bound is generous — the bound guards against a pathological change, the
// printed number is the honest measurement.
#include "training/WindMapController.h"
#include "training/WindMapAnalytics.h"
#include "test_support.h"

#include <QElapsedTimer>

using namespace ta::rel;
using namespace ta::training;

namespace {

// A realistic review session: 44 counted shots, 4 sighters, 4 conditions.
void buildLongSession(WindMapController& wm, MemoryJournalFile& file, ManualClock& clock)
{
    wm.storeForTesting()->setClockForTesting(&clock);
    wm.storeForTesting()->setJournalFileForTesting(&file);
    wm.configureSession(QStringLiteral("PRONE50"), 40, true);
    wm.startWindMap(QStringLiteral("Alexandra Fitzwilliam-Habersham"));
    qint64 ext = 1000;
    auto shoot = [&](double x, double y, double sc) {
        wm.registerShot(x, y, sc, ++ext, 0.0, 0);
    };
    wm.setCalmCondition(QStringLiteral("flags hanging limp, first light"));
    wm.beginSighters();
    for (int i = 0; i < 4; ++i) shoot(-1.0 + i * 0.7, 3.0 + i * 0.4, 9.4);
    wm.finishSighters();
    for (int i = 0; i < 12; ++i) shoot(-1.2 + (i % 5) * 0.6, 0.4 - (i % 4) * 0.5, 10.1);
    wm.setMeasuredCondition(270, 2.5, QStringLiteral("steady from the left across both flags"));
    for (int i = 0; i < 14; ++i) shoot(6.2 + (i % 6) * 0.7, 1.0 - (i % 5) * 0.6, 9.8);
    wm.setMeasuredCondition(45, 5.0, QStringLiteral("picking up"));
    for (int i = 0; i < 11; ++i) shoot(-5.5 - (i % 5) * 0.9, 4.0 + (i % 4) * 1.1, 9.2);
    wm.setNoReadingCondition();
    for (int i = 0; i < 7; ++i) shoot(1.0 + (i % 3) * 1.4, -2.0 - (i % 3) * 0.8, 9.9);
    wm.endCapture();
    wm.completeSession();
}

} // namespace

void run_windmap_perf_tests()
{
    fputs("\n--- wind map analysis preparation cost (stage 6.1.1) ---\n", stdout);

    MemoryJournalFile file;
    ManualClock clock;
    WindMapController wm;
    buildLongSession(wm, file, clock);
    check(wm.phase() == 6, "1. the 44-shot session completed");

    // ── the ENGINE itself ───────────────────────────────────────────────
    {
        const SessionState& st = wm.storeForTesting()->state();
        check(st.wmShots.size() == 48, "1. 44 counted + 4 sighters recorded",
              QString::number(st.wmShots.size()));

        QElapsedTimer t;
        t.start();
        const int runs = 50;
        for (int i = 0; i < runs; ++i) {
            const SessionAnalysis a = WindMapAnalyticsEngine::analyse(st);
            (void)a;
        }
        const double perRunMs = double(t.nsecsElapsed()) / 1e6 / runs;
        std::printf("      engine analyse()      : %.3f ms per run (%d runs)\n",
                    perRunMs, runs);
        std::fflush(stdout);
        // Generous bound: this guards a pathological regression, not a budget.
        check(perRunMs < 25.0,
              "1. one engine run over 48 shots costs well under 25 ms",
              QStringLiteral("%1 ms").arg(perRunMs, 0, 'f', 3));
    }

    // ── the VIEW MODEL projection, COLD ─────────────────────────────────
    {
        // Measured on the FIRST call only. Looping here would populate the
        // cache on iteration 0 and then time 49 cache hits — which would
        // report a "cold" cost roughly six times faster than the engine it
        // wraps, an obviously impossible number.
        check(wm.analysisBuildCountForTesting() == 0,
              "2. nothing has been built yet — this really is a cold call");
        QElapsedTimer t;
        t.start();
        const QVariantMap m = wm.analysisModel();
        const double coldMs = double(t.nsecsElapsed()) / 1e6;
        std::printf("      analysisModel() COLD    : %.3f ms (first call, one build)\n", coldMs);
        std::fflush(stdout);
        check(!m.isEmpty(), "2. the cold call produced a model");
        check(wm.analysisBuildCountForTesting() == 1,
              "2. exactly ONE build happened",
              QStringLiteral("%1 builds").arg(wm.analysisBuildCountForTesting()));
        check(coldMs < 40.0,
              "2. the cold build costs well under 40 ms",
              QStringLiteral("%1 ms").arg(coldMs, 0, 'f', 3));
    }

    // ── the CACHING CONTRACT the UX redesign depends on ─────────────────
    {
        // The engine must run ONCE per completed session. The counter is
        // incremented inside the controller whenever it actually analyses.
        const int before = wm.analysisBuildCountForTesting();
        for (int i = 0; i < 20; ++i) {
            const QVariantMap m = wm.analysisModel();
            (void)m;
        }
        const int after = wm.analysisBuildCountForTesting();
        check(after == before,
              "3. twenty analysisModel() calls rebuild NOTHING — the model is cached",
              QStringLiteral("%1 rebuilds").arg(after - before));

        QElapsedTimer t;
        t.start();
        const int runs = 200;
        for (int i = 0; i < runs; ++i) {
            const QVariantMap m = wm.analysisModel();
            (void)m;
        }
        const double perRunMs = double(t.nsecsElapsed()) / 1e6 / runs;
        std::printf("      analysisModel() CACHED  : %.4f ms per run (%d runs)\n",
                    perRunMs, runs);
        std::fflush(stdout);
        check(perRunMs < 2.0,
              "3. a cached fetch is effectively free — page and position "
              "switching cannot cost an analysis",
              QStringLiteral("%1 ms").arg(perRunMs, 0, 'f', 4));
    }

    // ── the cache must INVALIDATE when the session changes ──────────────
    {
        // A cache that never invalidates is a correctness bug, not a speedup.
        MemoryJournalFile f2;
        ManualClock c2;
        WindMapController w2;
        w2.storeForTesting()->setClockForTesting(&c2);
        w2.storeForTesting()->setJournalFileForTesting(&f2);
        w2.configureSession(QStringLiteral("PRONE50"), 20, false);
        w2.startWindMap(QStringLiteral("Cache Check"));
        w2.setCalmCondition();
        w2.beginCountedShots();
        qint64 ext = 5000;
        for (int i = 0; i < 5; ++i) w2.registerShot(double(i) - 2.0, 0.0, 10.0, ++ext, 0.0, 0);
        w2.endCapture();
        w2.completeSession();
        const QVariantMap first = w2.analysisModel();
        const int firstCounted =
            first.value(QStringLiteral("summary")).toMap()
                 .value(QStringLiteral("countedShots")).toInt();
        check(firstCounted == 5, "4. the first analysis sees 5 counted shots");

        // Start a NEW session on the same controller. The cache must not
        // serve the previous session's analysis.
        w2.closeSessionCleanly();
        MemoryJournalFile f3;
        w2.storeForTesting()->setJournalFileForTesting(&f3);
        w2.configureSession(QStringLiteral("PRONE50"), 20, false);
        w2.startWindMap(QStringLiteral("Cache Check Two"));
        w2.setCalmCondition();
        w2.beginCountedShots();
        for (int i = 0; i < 3; ++i) w2.registerShot(double(i), 0.0, 9.0, ++ext, 0.0, 0);
        w2.endCapture();
        w2.completeSession();
        const QVariantMap second = w2.analysisModel();
        check(second.value(QStringLiteral("summary")).toMap()
                    .value(QStringLiteral("countedShots")).toInt() == 3,
              "4. a NEW session invalidates the cache — no stale analysis");
        check(second.value(QStringLiteral("session")).toMap()
                    .value(QStringLiteral("athlete")).toString() == QLatin1String("Cache Check Two"),
              "4. and the identity is the new session's");
    }
}
