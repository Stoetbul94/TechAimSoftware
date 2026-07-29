// Wind Map — review-fixture seeder (TEST TOOL, not production code).
//
// Writes real Wind Map journals into an ISOLATED data root so a manual visual
// review does not require hundreds of clicks to reach a 40-shot session, a
// long condition note, or the insufficient-sample state.
//
// It drives the REAL WindMapController, so every session it writes is a
// genuine journal: same events, same hash chain, same reducer state. Nothing
// is fabricated and no screen is driven — this creates DATA, not input.
//
// Each session is left in SessionReview (capture ended, not completed) and
// NOT closed, so the application offers it through the normal recovery
// dialog. Resuming one and pressing "Complete session" opens the analysis
// view under review.
//
//   reliability_tests --seed-windmap "<absolute data root>"
//
// The root must be a documentation-capture root; the caller passes the same
// path the application is launched with.
#include "training/WindMapController.h"
#include "reliability/storage/StoragePaths.h"

#include <QDir>
#include <QString>
#include <cstdio>

using namespace ta::training;

namespace {

// One shot, from the physical-target source, with a monotonically increasing
// external id so the duplicate guard behaves exactly as it does live.
struct Seeder {
    WindMapController wm;
    qint64 ext = 1000;
    void shoot(double x, double y, double score) { wm.registerShot(x, y, score, ++ext, 0.0, 0); }
};

void report(const char* name, bool ok, const WindMapController& wm)
{
    std::printf("  %-46s %s   %s\n", name, ok ? "OK " : "FAIL",
                ok ? qPrintable(wm.sessionId().left(8))
                   : qPrintable(wm.lastError()));
    std::fflush(stdout);
}

// A — 50m Prone, 44 counted shots across four conditions, with sighters and a
// long condition note. Covers: the 40+ shot session, long condition labels,
// condition filtering, the sighter toggle and a long timeline.
bool seedProneLong()
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("PRONE50"), 40, true)) return false;
    if (!s.wm.startWindMap(QStringLiteral("Alexandra Fitzwilliam-Habersham"))) return false;

    s.wm.setCalmCondition(QStringLiteral("flags hanging limp, first light, no movement"));
    s.wm.beginSighters();
    for (int i = 0; i < 4; ++i) s.shoot(-1.0 + i * 0.7, 3.0 + i * 0.4, 9.4);
    s.wm.finishSighters();

    // Calm reference group — 12 shots.
    for (int i = 0; i < 12; ++i)
        s.shoot(-1.2 + (i % 5) * 0.6, 0.4 - (i % 4) * 0.5, 10.1 + (i % 3) * 0.2);

    // A deliberately LONG note, to exercise wrapping in the plot legend,
    // the appendix and the PDF.
    s.wm.setMeasuredCondition(270, 2.5,
        QStringLiteral("steady from the left across both flags, near flag showing more "
                       "than far flag, gusting slightly between shots"));
    for (int i = 0; i < 14; ++i)
        s.shoot(6.2 + (i % 6) * 0.7, 1.0 - (i % 5) * 0.6, 9.8 + (i % 4) * 0.3);

    s.wm.setMeasuredCondition(45, 5.0, QStringLiteral("picking up, far flag lifting"));
    for (int i = 0; i < 11; ++i)
        s.shoot(-5.5 - (i % 5) * 0.9, 4.0 + (i % 4) * 1.1, 9.2 + (i % 5) * 0.3);

    // No reading — the absence state, still recorded.
    s.wm.setNoReadingCondition();
    for (int i = 0; i < 7; ++i)
        s.shoot(1.0 + (i % 3) * 1.4, -2.0 - (i % 3) * 0.8, 9.9);

    return s.wm.endCapture();      // left in SessionReview, deliberately unclosed
}

// B — 50m 3P, all three positions with their own conditions and sighters.
// Covers: Kneeling / Prone / Standing analyses and the Overview tab.
bool seed3P()
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("3P50"), 30, true)) return false;
    if (!s.wm.startWindMap(QStringLiteral("Arnold Bailie"))) return false;

    struct Pos { int to; double cx, cy; double spread; };
    const Pos plan[] = { { 1, 8.0, 6.0, 2.4 }, { 2, 0.5, -0.5, 1.0 }, { 3, -9.0, -7.0, 3.2 } };

    for (int p = 0; p < 3; ++p) {
        if (p > 0) {
            if (!s.wm.endPosition()) return false;
            if (!s.wm.changePosition(plan[p].to)) return false;
        }
        s.wm.setCalmCondition(QStringLiteral("calm between positions"));
        s.wm.beginSighters();
        for (int i = 0; i < 2; ++i)
            s.shoot(plan[p].cx + i, plan[p].cy + i, 9.0);
        s.wm.finishSighters();
        // calm group
        for (int i = 0; i < 6; ++i)
            s.shoot(plan[p].cx + (i % 4) * plan[p].spread * 0.5 - plan[p].spread,
                    plan[p].cy - (i % 3) * plan[p].spread * 0.4, 10.0);
        // a measured condition in the same position
        s.wm.setMeasuredCondition(90, 3.2, QStringLiteral("from the right"));
        for (int i = 0; i < 6; ++i)
            s.shoot(plan[p].cx + 5.0 + (i % 4) * plan[p].spread * 0.5,
                    plan[p].cy + (i % 3) * plan[p].spread * 0.4, 9.7);
    }
    return s.wm.endCapture();
}

// C — the INSUFFICIENT-SAMPLE state: every condition below a threshold, so
// the analysis must withhold rather than report.
bool seedInsufficient()
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("PRONE50"), 20, false)) return false;
    if (!s.wm.startWindMap(QStringLiteral("Sam Short-Session"))) return false;
    s.wm.beginCountedShots();
    s.wm.setMeasuredCondition(0, 1.5, QStringLiteral("light head wind"));
    s.shoot(1.0, 1.0, 10.3);
    s.shoot(1.4, 0.6, 10.0);                    // n=2 -> MPI withheld
    s.wm.setMeasuredCondition(180, 6.5, QStringLiteral("strong tail"));
    s.shoot(-3.0, -1.0, 9.1);
    s.wm.setCalmCondition();
    s.shoot(0.2, 0.2, 10.6);
    s.shoot(0.4, -0.1, 10.4);
    s.shoot(-0.3, 0.5, 10.5);
    s.shoot(0.1, 0.3, 10.7);                    // n=4 -> MPI only, no dispersion
    return s.wm.endCapture();
}

} // namespace

int seedWindMapSessions(const QString& root)
{
    if (root.isEmpty() || QDir(root).isRelative()) {
        std::printf("SEED REFUSED: --seed-windmap needs an ABSOLUTE data root\n");
        return 2;
    }
    // Point the storage layer at the isolated root the reviewer will launch
    // the application with. Nothing is written anywhere else.
    ta::rel::StoragePaths::setRootOverrideForTesting(root);
    const ta::rel::StorageResult sr = ta::rel::StoragePaths::initialize();
    if (!sr.ok) {
        std::printf("SEED REFUSED: storage init failed at %s\n", qPrintable(root));
        return 2;
    }
    std::printf("Seeding Wind Map review fixtures into:\n  %s\n\n", qPrintable(root));

    int failures = 0;
    {
        Seeder probe;                       // only to satisfy report()'s signature
        (void)probe;
    }
    struct Case { const char* name; bool (*fn)(); };
    const Case cases[] = {
        { "A  50m Prone - 44 counted, 4 conditions, sighters", &seedProneLong },
        { "B  50m 3P    - Kneeling / Prone / Standing",        &seed3P },
        { "C  50m Prone - insufficient samples",               &seedInsufficient },
    };
    for (const Case& c : cases) {
        const bool ok = c.fn();
        std::printf("  %-46s %s\n", c.name, ok ? "OK" : "FAILED");
        std::fflush(stdout);
        if (!ok) ++failures;
    }
    std::printf("\nEach session is left in SESSION REVIEW and deliberately NOT closed,\n"
                "so the application offers it through the normal recovery dialog.\n"
                "Resume one, then press \"Complete session\" to open the analysis.\n");
    return failures == 0 ? 0 : 1;
}
