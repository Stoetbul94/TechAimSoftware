// 3P FINAL — Phase A acceptance test suite for Finals3PController.
// Drives the controller end-to-end in accelerated time (TECHAIM_FINALS_TIMESCALE)
// and asserts the full command/stage ordering, warning timing, LOAD->START
// delays, shot numbering, window gating, rejections, pause/abort/reset and the
// structural no-scoring guarantee. Standalone console binary — the controller
// has NO access to any QML model or qualification timer by construction; these
// tests additionally assert its only outputs are signals/events.
//
// Timing note: the controller ticks every 50 ms of real time, so at timeScale S
// one tick is 50*S scaled ms. All timing assertions therefore use a tolerance
// of a few ticks; ordering assertions are exact.

#include <QApplication>
#include <QElapsedTimer>
#include <QVariantList>
#include <QDebug>
#include <cstdio>

#include <QFile>

#include "Finals3PController.h"
#include "Finals3PTypes.h"

using techaim::finals::Stage;

static int g_failures = 0;
static int g_checks = 0;

static void check(bool ok, const char* name, const QString& detail = QString())
{
    ++g_checks;
    if (ok) {
        std::printf("PASS  %s\n", name);
    } else {
        ++g_failures;
        std::printf("FAIL  %s  %s\n", name, qUtf8Printable(detail));
    }
}

// Spin the event loop until pred() or timeout (real ms). Returns pred() result.
template <typename F>
static bool waitUntil(F pred, int timeoutMs)
{
    QElapsedTimer t; t.start();
    while (!pred()) {
        if (t.elapsed() > timeoutMs)
            return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    return true;
}

struct Recorder {
    QStringList commands;          // typeName sequence
    QVariantList commandEvents;    // full event maps
    QList<int> stageOrder;         // distinct stage entries
    QVariantList accepted;         // shotAccepted maps
    QVariantList rejected;         // shotRejected maps
    int finalCompletedCount = 0;

    void attach(Finals3PController* c)
    {
        QObject::connect(c, &Finals3PController::commandIssued, [this](const QVariantMap& e) {
            commands << e.value("typeName").toString();
            commandEvents << e;
        });
        QObject::connect(c, &Finals3PController::phaseChanged, [this, c]() {
            if (stageOrder.isEmpty() || stageOrder.last() != c->stageId())
                stageOrder << c->stageId();
        });
        QObject::connect(c, &Finals3PController::shotAccepted, [this](const QVariantMap& s) {
            accepted << s;
        });
        QObject::connect(c, &Finals3PController::shotRejected, [this](const QVariantMap& r) {
            rejected << r;
        });
        QObject::connect(c, &Finals3PController::finalCompleted, [this]() { ++finalCompletedCount; });
    }

    qint64 issuedAt(const QString& type, int occurrence = 1) const
    {
        int n = 0;
        for (const QVariant& v : commandEvents) {
            const QVariantMap m = v.toMap();
            if (m.value("typeName").toString() == type && ++n == occurrence)
                return m.value("issuedAt").toLongLong();
        }
        return -1;
    }
    int countOf(const QString& type) const
    {
        int n = 0;
        for (const QString& c : commands) if (c == type) ++n;
        return n;
    }
};

static bool inStage(Finals3PController& c, Stage s)
{
    return c.stageId() == static_cast<int>(s);
}

// ── The main end-to-end run: Full ceremony, athlete completes all 35 ────────
static void runFullFinal()
{
    Finals3PController c;
    check(qFuzzyCompare(c.timeScale(), 60.0), "env TECHAIM_FINALS_TIMESCALE applied");
    Recorder r; r.attach(&c);
    const qint64 tolMs = 12000;   // a few 50ms ticks at scale 60 (3s scaled each)

    // Idle: shots rejected, no state corruption from invalid calls.
    c.simulateShot();
    check(r.rejected.size() == 1 && c.stageId() == 0, "Idle: shot rejected, state intact");
    c.advanceStage1();
    check(c.targetMode() == 0, "Idle: advance ignored (transition rejected)");

    c.startFinal();
    check(inStage(c, Stage::Ceremony), "startFinal -> Ceremony (Full)");
    c.startFinal();   // repeated command must not corrupt
    check(inStage(c, Stage::Ceremony) && r.countOf("AthletesToLine") == 1,
          "repeated startFinal ignored");
    c.simulateShot();
    check(r.rejected.size() == 2
              && r.rejected.last().toMap().value("reason") == "WindowClosed",
          "Ceremony: shot rejected WindowClosed");

    // Ceremony -> prep: intro then TAKE YOUR POSITIONS then 30 s hold (separate
    // from the 5-minute prep timer).
    check(waitUntil([&]{ return inStage(c, Stage::KneelingPrepSight); }, 8000),
          "reached KneelingPrepSight");
    const qint64 tAth  = r.issuedAt("AthletesToLine");
    const qint64 tTake = r.issuedAt("TakeYourPositions");
    const qint64 tPrep = r.issuedAt("PreparationSightingStart");
    check(tAth >= 0 && tTake > tAth && tPrep > tTake, "ceremony command order");
    check(tTake - tAth >= 20000 && tTake - tAth < 20000 + tolMs,
          "intro duration before TAKE YOUR POSITIONS", QString::number(tTake - tAth));
    check(tPrep - tTake >= 30000 && tPrep - tTake < 30000 + tolMs,
          "30 s hold separate from 5:00 prep", QString::number(tPrep - tTake));

    check(c.windowState() == 1 && c.targetMode() == 0,
          "prep: SightingOpen, target SIGHTER");
    c.simulateShot();
    check(r.accepted.size() == 1 && r.accepted.last().toMap().value("sighter").toBool(),
          "prep sighter accepted without official number");
    c.advanceStage1();
    check(c.targetMode() == 0 && inStage(c, Stage::KneelingPrepSight),
          "prep: advance illegal (transition rejected, stage intact)");

    // Prep warning + end (auto-MATCH = the only automatic switch).
    check(waitUntil([&]{ return inStage(c, Stage::KneelingMatch); }, 10000),
          "prep -> KneelingMatch");
    const qint64 tWarn = r.issuedAt("ThirtySeconds", 1);
    check(tWarn - tPrep >= 270000 && tWarn - tPrep < 270000 + tolMs,
          "prep 30-second warning at 4:30", QString::number(tWarn - tPrep));
    check(c.targetMode() == 1, "auto-switch to MATCH at prep end (EST officer)");

    // Stage 1: MATCH FIRING START 5 s after the announcement; 22:00 clock.
    check(waitUntil([&]{ return c.windowState() == 2; }, 5000),
          "kneeling match window opened");
    const qint64 tAnn   = r.issuedAt("StageOneAnnouncement");
    const qint64 tStart = r.issuedAt("MatchFiringStart");
    check(tStart - tAnn >= 5000 && tStart - tAnn < 5000 + tolMs,
          "5 s announcement -> MATCH FIRING START", QString::number(tStart - tAnn));

    // Kneeling 1-10, then the 11th is hard-blocked.
    for (int i = 0; i < 10; ++i) c.simulateShot();
    c.simulateShot();   // 11th
    check(r.rejected.last().toMap().value("reason") == "StageShotLimitReached",
          "11th kneeling shot rejected StageShotLimitReached");
    check(c.shotsInStage() == 10, "kneeling limited to 10");

    // Athlete-controlled transitions via the single stage-aware action.
    check(c.advanceLabel().startsWith("CHANGE TO PRONE") && c.advanceLabel().endsWith("SIGHTERS"),
          "advance label: kneeling", c.advanceLabel());
    c.advanceStage1();
    check(inStage(c, Stage::ProneSighting) && c.windowState() == 1,
          "advance -> ProneSighting (SightingOpen)");
    c.simulateShot();
    check(r.accepted.last().toMap().value("sighter").toBool(), "prone sighter accepted");
    check(c.advanceLabel() == QStringLiteral("START PRONE MATCH"),
          "advance label: prone sighting", c.advanceLabel());
    c.advanceStage1();
    check(inStage(c, Stage::ProneMatch) && c.windowState() == 2,
          "advance -> ProneMatch (MatchOpen)");
    for (int i = 0; i < 10; ++i) c.simulateShot();
    check(c.shotsInStage() == 10, "prone limited to 10");
    check(c.advanceLabel().startsWith("CHANGE TO STANDING") && c.advanceLabel().endsWith("SIGHTERS"),
          "advance label: prone match", c.advanceLabel());
    c.advanceStage1();
    check(inStage(c, Stage::StandingSighting), "advance -> StandingSighting");
    c.simulateShot();
    check(r.accepted.last().toMap().value("sighter").toBool(), "standing sighter accepted");
    check(c.advanceLabel().startsWith("TARGET TO MATCH") && c.advanceLabel().endsWith("READY"),
          "advance label: standing sighting", c.advanceLabel());
    c.advanceStage1();   // athlete readies MATCH for the commanded series
    check(inStage(c, Stage::StandingSighting) && c.windowState() == 0,
          "standing MATCH ready: window closed, stage unchanged");
    c.simulateShot();
    check(r.rejected.last().toMap().value("reason") == "WindowClosed",
          "shot outside firing window rejected");

    // Stage-1 warnings and STOP on the continuous 22:00 clock.
    check(waitUntil([&]{ return inStage(c, Stage::StandingSeries1); }, 40000),
          "stage-1 STOP -> StandingSeries1");
    const qint64 tW5  = r.issuedAt("FiveMinutes");
    const qint64 tW30 = r.issuedAt("ThirtySeconds", 2);
    check(tW5 - tStart >= 1020000 && tW5 - tStart < 1020000 + tolMs,
          "stage-1 FIVE MINUTES at 17:00", QString::number(tW5 - tStart));
    check(tW30 - tStart >= 1290000 && tW30 - tStart < 1290000 + tolMs,
          "stage-1 THIRTY SECONDS at 21:30", QString::number(tW30 - tStart));

    // Series 1: LOAD -> 5 s -> START -> 5 shots -> STOP.
    check(waitUntil([&]{ return c.windowState() == 2; }, 10000), "series 1 window opened");
    const qint64 tL1 = r.issuedAt("LoadSeries", 1);
    const qint64 tS1 = r.issuedAt("StartSeries", 1);
    check(tS1 - tL1 >= 5000 && tS1 - tL1 < 5000 + tolMs,
          "series 1 LOAD->START 5 s", QString::number(tS1 - tL1));
    for (int i = 0; i < 5; ++i) c.simulateShot();
    check(inStage(c, Stage::StandingSeries2), "series 1 complete -> series 2");

    // Series 2: same cycle; elimination notice follows.
    check(waitUntil([&]{ return c.windowState() == 2; }, 10000), "series 2 window opened");
    const qint64 tL2 = r.issuedAt("LoadSeries", 2);
    const qint64 tS2 = r.issuedAt("StartSeries", 2);
    check(tS2 - tL2 >= 5000 && tS2 - tL2 < 5000 + tolMs,
          "series 2 LOAD->START 5 s", QString::number(tS2 - tL2));
    for (int i = 0; i < 5; ++i) c.simulateShot();

    // Singles 31-35: five LOAD->START cycles, one-shot 50 s windows.
    for (int k = 0; k < 5; ++k) {
        check(waitUntil([&]{ return c.windowState() == 2; }, 10000),
              qUtf8Printable(QStringLiteral("single %1 window opened").arg(31 + k)));
        const qint64 tL = r.issuedAt("LoadSingle", k + 1);
        const qint64 tS = r.issuedAt("StartSingle", k + 1);
        check(tS - tL >= 5000 && tS - tL < 5000 + tolMs,
              qUtf8Printable(QStringLiteral("single %1 LOAD->START 5 s").arg(31 + k)));
        c.simulateShot();
        check(c.windowState() == 0,
              qUtf8Printable(QStringLiteral("single %1 window closed after 1 shot").arg(31 + k)));
    }

    check(waitUntil([&]{ return inStage(c, Stage::Complete); }, 5000), "reached Complete");
    check(r.finalCompletedCount == 1, "finalCompleted emitted once");

    // Exact command ordering (structured events, not just final state).
    const QStringList expected = {
        "AthletesToLine", "TakeYourPositions", "PreparationSightingStart",
        "ThirtySeconds", "Stop", "StageOneAnnouncement", "MatchFiringStart",
        "FiveMinutes", "ThirtySeconds", "Stop",
        "LoadSeries", "StartSeries", "Stop",
        "LoadSeries", "StartSeries", "Stop", "InfoNotice",
        "LoadSingle", "StartSingle", "Stop", "InfoNotice",
        "LoadSingle", "StartSingle", "Stop", "InfoNotice",
        "LoadSingle", "StartSingle", "Stop", "InfoNotice",
        "LoadSingle", "StartSingle", "Stop", "InfoNotice",
        "LoadSingle", "StartSingle", "Stop", "InfoNotice",
        "Unload", "ResultsFinal"
    };
    check(r.commands == expected, "full command-event ordering",
          r.commands.join(","));

    // Exact stage-entry ordering.
    const QList<int> expectedStages = {
        static_cast<int>(Stage::Ceremony), static_cast<int>(Stage::KneelingPrepSight),
        static_cast<int>(Stage::KneelingMatch), static_cast<int>(Stage::ProneSighting),
        static_cast<int>(Stage::ProneMatch), static_cast<int>(Stage::StandingSighting),
        static_cast<int>(Stage::StandingSeries1), static_cast<int>(Stage::StandingSeries2),
        static_cast<int>(Stage::StandingSingle1), static_cast<int>(Stage::StandingSingle2),
        static_cast<int>(Stage::StandingSingle3), static_cast<int>(Stage::StandingSingle4),
        static_cast<int>(Stage::StandingSingle5), static_cast<int>(Stage::Complete)
    };
    check(r.stageOrder == expectedStages, "stage-entry ordering");

    // Continuous official numbering 1-35; sighters carry no numbers; every
    // event is flagged simulated; no synthetic shots exist.
    QList<int> numbers;
    int sighters = 0;
    bool allSimulated = true;
    for (const QVariant& v : r.accepted) {
        const QVariantMap m = v.toMap();
        if (!m.value("simulated").toBool()) allSimulated = false;
        if (m.value("sighter").toBool()) { ++sighters; continue; }
        numbers << m.value("finalShotNumber").toInt();
    }
    QList<int> expectNums;
    for (int i = 1; i <= 35; ++i) expectNums << i;
    check(numbers == expectNums, "official shot numbers 1..35 continuous");
    check(sighters == 3, "3 sighting shots accepted without numbers");
    check(allSimulated, "no scoring: every Phase-A event flagged simulated");

    // B1: schema completeness — every plan-§2 role on the FIRST accepted record.
    {
        const QVariantMap first = r.accepted.first().toMap();
        const QStringList roles = {
            "calculatedscore","score","xmm","ymm","direction","timestamp",
            "timeComsumed","position","finalsStageId","finalsStageLabel",
            "finalsPosition","finalsShotNumber","finalsWindowId",
            "finalsSeriesIndex","shotWithinStage","targetModeAtReceipt",
            "isSighter","isFinalsShot" };
        bool allPresent = true;
        QString missing;
        for (const QString& role : roles)
            if (!first.contains(role)) { allPresent = false; missing += role + " "; }
        check(allPresent, "shot record schema complete on first record", missing);
    }
    check(c.missingShots().isEmpty(), "complete final: no MissingShot records");

    // RMS hooks exist and do not disturb local state.
    const int stageBefore = c.stageId();
    c.startPhaseFromServer(QVariantMap());
    c.stopPhaseFromServer(QVariantMap());
    c.synchroniseClock(0, 0);
    c.applyAuthoritativeState(QVariantMap());
    check(c.stageId() == stageBefore, "RMS hook stubs do not break local state");
}

// ── Ceremony modes, pause/resume, abort/reset, unfired shots ────────────────
static void runSecondaryChecks()
{
    // Short ceremony: no introductions, straight to the 30 s hold.
    {
        Finals3PController c;
        c.setTimeScale(240.0);
        c.setCeremonyMode(1);
        Recorder r; r.attach(&c);
        c.startFinal();
        check(c.stageId() == static_cast<int>(Stage::Ceremony)
                  && r.commands.first() == "TakeYourPositions",
              "Short ceremony: hold only, no introductions");
        c.abortFinal();
        check(c.stageId() == static_cast<int>(Stage::Aborted) && c.windowState() == 0,
              "abort -> Aborted, window closed");
        c.startFinal();
        check(c.stageId() == static_cast<int>(Stage::Aborted),
              "startFinal ignored while Aborted");
        c.resetFinal();
        check(c.stageId() == static_cast<int>(Stage::Idle), "reset -> Idle");
    }

    // Skip ceremony: straight to preparation/sighting. Pause preserves time.
    {
        Finals3PController c;
        c.setTimeScale(240.0);
        c.setCeremonyMode(2);
        Recorder r; r.attach(&c);
        c.startFinal();
        check(c.stageId() == static_cast<int>(Stage::KneelingPrepSight)
                  && r.commands.first() == "PreparationSightingStart",
              "Skip ceremony: straight to prep/sighting");

        c.pauseTrainingSimulation();
        const qint64 remA = c.remainingMs();
        QElapsedTimer t; t.start();
        while (t.elapsed() < 400) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        const qint64 remB = c.remainingMs();
        check(remA == remB, "pause freezes remaining time",
              QString("%1 vs %2").arg(remA).arg(remB));
        c.resumeTrainingSimulation();
        t.restart();
        while (t.elapsed() < 300) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        check(c.remainingMs() < remB, "resume: countdown continues");
        check(!waitUntil([&]{ return false; }, 50) && c.paused() == false, "resume clears paused");

        // Unfired shots: fire only 3 kneeling shots; at 22:00 the window simply
        // closes — no synthetic 0.0 shots are fabricated ([P1] disabled).
        check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::KneelingMatch)
                                    && c.windowState() == 2; }, 10000),
              "unfired-run reached kneeling match");
        c.simulateShot(); c.simulateShot(); c.simulateShot();
        const int acceptedBefore = r.accepted.size();
        check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::StandingSeries1); }, 30000),
              "stage-1 expiry with unfired shots -> series 1");
        check(r.accepted.size() == acceptedBefore,
              "no synthetic shots created at expiry (P1=B: no model entries)");
        check(c.missingShots().size() == 17,
              "MissingShot records for 7 kneeling + 10 prone unfired",
              QString::number(c.missingShots().size()));
        c.abortFinal();
        c.registerShot(1, 1, 9.0, 50);
        check(r.rejected.last().toMap().value("reason") == "FinalsNotActive",
              "shot after abort rejected FinalsNotActive");
    }

    // Under-limit advance confirmation + registerShot validation chain.
    {
        Finals3PController c;
        c.setTimeScale(240.0);
        c.setCeremonyMode(2);
        Recorder r; r.attach(&c);
        int confirmAsks = 0;
        QObject::connect(&c, &Finals3PController::advanceConfirmationRequired,
                         [&confirmAsks](int, int) { ++confirmAsks; });
        c.startFinal();
        check(waitUntil([&]{ return c.stageId() == static_cast<int>(Stage::KneelingMatch)
                                    && c.windowState() == 2; }, 10000),
              "B1 run reached kneeling match window");

        c.registerShot(1.5, -2.0, 10.4, 100);
        check(!r.accepted.isEmpty()
                  && r.accepted.last().toMap().value("finalsShotNumber").toInt() == 1
                  && r.accepted.last().toMap().value("calculatedscore").toString() == "10.4",
              "registerShot accepted with schema values");
        c.registerShot(2.0, 2.0, 9.6, 100);
        check(r.rejected.last().toMap().value("reason") == "DuplicateShot",
              "repeated externalShotId rejected DuplicateShot");
        c.registerShot(0, 0, -1.0, 101);
        check(r.rejected.last().toMap().value("reason") == "InvalidShotData",
              "invalid score rejected InvalidShotData");

        c.advanceStage1();
        check(confirmAsks == 1 && c.stageId() == static_cast<int>(Stage::KneelingMatch),
              "under-limit advance asks for confirmation, stage unchanged");
        c.advanceStage1();
        check(confirmAsks == 2 && c.stageId() == static_cast<int>(Stage::KneelingMatch),
              "repeated advance re-asks, never self-confirms");
        c.cancelStage1Advance();
        c.confirmStage1Advance();
        check(c.stageId() == static_cast<int>(Stage::KneelingMatch),
              "confirm after cancel is inert");
        c.advanceStage1();
        c.confirmStage1Advance();
        check(c.stageId() == static_cast<int>(Stage::ProneSighting),
              "confirmed under-limit advance -> ProneSighting");
        c.registerShot(0.5, 0.5, 9.9, 102);
        check(r.accepted.last().toMap().value("isSighter").toBool()
                  && r.accepted.last().toMap().value("finalsShotNumber").toInt() == 0,
              "prone sighter accepted unnumbered via registerShot");

        // B4: decimal totals over ACCEPTED OFFICIAL shots only.
        check(qFuzzyCompare(c.cumulativeTotal(), 10.4),
              "cumulative total exact (sighters excluded)",
              QString::number(c.cumulativeTotal()));
        check(qFuzzyCompare(c.stageSubtotal() + 1.0, 1.0),
              "stage subtotal reset on stage entry",
              QString::number(c.stageSubtotal()));

        // B4: template record carries the complete role union.
        {
            const QVariantMap tpl = c.templateShotRecord();
            const QStringList roles = {
                "calculatedscore","score","xmm","ymm","direction","timestamp",
                "timeComsumed","position","finalsStageId","finalsStageLabel",
                "finalsPosition","finalsShotNumber","finalsWindowId",
                "finalsSeriesIndex","shotWithinStage","targetModeAtReceipt",
                "isSighter","isFinalsShot" };
            bool ok = true; QString missing;
            for (const QString& role : roles)
                if (!tpl.contains(role)) { ok = false; missing += role + " "; }
            check(ok, "templateShotRecord covers the full role union", missing);
        }

        // B4: session journal exists and recorded the accepted shots.
        {
            QFile jf(QStringLiteral("finals_session.jsonl"));
            bool okOpen = jf.open(QIODevice::ReadOnly | QIODevice::Text);
            const QString all = okOpen ? QString::fromUtf8(jf.readAll()) : QString();
            check(okOpen && all.count(QStringLiteral("\"journalType\":\"shotAccepted\"")) == 2
                       && all.contains(QStringLiteral("\"journalType\":\"stageEntered\""))
                       && all.contains(QStringLiteral("\"journalType\":\"finalStarted\"")),
              "journal: one line per accepted shot + stage entries");
        }
        c.abortFinal();
    }
}

int main(int argc, char** argv)
{
    qputenv("TECHAIM_FINALS_TIMESCALE", "60");
    qputenv("QT_QPA_PLATFORM", "minimal");
    QApplication app(argc, argv);

    std::printf("=== Finals3PController Phase A acceptance tests ===\n");
    runFullFinal();
    runSecondaryChecks();

    std::printf("=== %d checks, %d failures ===\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
