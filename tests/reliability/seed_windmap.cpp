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

    // Stage 6.1.4 — what this seeded session will ACTUALLY produce when the
    // reviewer completes it. Read from the same analysisModel() the screen
    // consumes, so the review sheet cannot claim a verdict the app will not
    // show. Nothing is fabricated: this is the model, not a prediction.
    QString expectation() const
    {
        const QVariantMap m = wm.analysisModel();
        if (m.isEmpty()) return QStringLiteral("(no analysis available yet)");
        const QString ver = m.value(QStringLiteral("session")).toMap()
                             .value(QStringLiteral("analyticsVersion")).toString();
        const QVariantList vs = m.value(QStringLiteral("verdicts")).toList();
        if (vs.isEmpty())
            return QStringLiteral("%1 | NO VERDICT").arg(ver);
        const QVariantMap v0 = vs.first().toMap();
        return QStringLiteral("%1 | %2 | %3 | %4 verdict%5")
            .arg(ver)
            .arg(v0.value(QStringLiteral("category")).toString())
            .arg(v0.value(QStringLiteral("scopeLabel")).toString())
            .arg(vs.size()).arg(vs.size() == 1 ? QString() : QStringLiteral("s"));
    }
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
bool seedProneLong(QString* expect)
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

    const bool ok = s.wm.endCapture();
    if (expect) *expect = s.expectation();
    return ok;      // left in SessionReview, deliberately unclosed
}

// B — 50m 3P, all three positions with their own conditions and sighters.
// Covers: Kneeling / Prone / Standing analyses and the Overview tab.
bool seed3P(QString* expect)
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
    const bool ok = s.wm.endCapture();
    if (expect) *expect = s.expectation();
    return ok;
}

// C — the INSUFFICIENT-SAMPLE state: every condition below a threshold, so
// the analysis must withhold rather than report.
bool seedInsufficient(QString* expect)
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("PRONE50"), 20, false)) return false;
    if (!s.wm.startWindMap(QStringLiteral("Sam Short-Session"))) return false;
    // EXACTLY TWO conditions, so this is the INSUFFICIENT-SAMPLE case and not
    // the fragmented one: a well-sampled calm reference, and a second condition
    // three shots short of the comparison minimum. Three or more conditions
    // would trip "conditions changed too often" instead, which is case H.
    s.wm.beginCountedShots();
    s.wm.setCalmCondition(QStringLiteral("flags down"));
    for (int i = 0; i < 8; ++i)
        s.shoot(0.2 + (i % 4) * 0.5, 0.3 - (i % 3) * 0.4, 10.4);
    s.wm.setMeasuredCondition(0, 1.5, QStringLiteral("light head wind"));
    s.shoot(1.0, 1.0, 10.3);
    s.shoot(1.4, 0.6, 10.0);                    // n=2 -> below the comparison bar
    const bool ok = s.wm.endCapture();
    if (expect) *expect = s.expectation();
    return ok;
}

// ---- Stage 6.1.3 verdict review cases -------------------------------------
// One session per verdict category, so Arnold can read each result and check
// what happened / evidence / what it may mean / next test / coach decision.
// Every one is a REAL controller run - no fabricated analysis. Each is left in
// SESSION REVIEW so the recovery dialog offers it exactly like cases A-C.

// A group of n shots on a horizontal line of the given width, centred at cx.
// Extreme spread == widthMm, so the group diameter is exactly predictable.
void line(Seeder& s, int n, double cx, double cy, double widthMm)
{
    for (int i = 0; i < n; ++i) {
        const double t = (n == 1) ? 0.0 : (double(i) / double(n - 1)) - 0.5;
        s.shoot(cx + t * widthMm, cy, 10.0);
    }
}

// D - COMPACT BUT OFFSET, with a firing direction so the relative wind
// description appears. Firing 0 (north); wind from 270 (west) reads as a
// left-to-right crosswind.
bool seedCompactOffset(QString* expect)
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("PRONE50"), 30, false)) return false;
    if (!s.wm.setFiringDirection(0)) return false;
    if (!s.wm.startWindMap(QStringLiteral("Case D - Compact but offset"))) return false;
    s.wm.setCalmCondition(QStringLiteral("still, flags down"));
    s.wm.beginCountedShots();
    line(s, 10, 0.0, 0.0, 20.0);
    s.wm.setMeasuredCondition(270, 2.0, QStringLiteral("steady from the left"));
    line(s, 10, 8.0, -1.5, 20.0);
    const bool ok = s.wm.endCapture();
    if (expect) *expect = s.expectation();
    return ok;
}

// E - WIDER UNDER A CONDITION.
bool seedWider(QString* expect)
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("PRONE50"), 30, false)) return false;
    if (!s.wm.setFiringDirection(0)) return false;
    if (!s.wm.startWindMap(QStringLiteral("Case E - Wider under condition"))) return false;
    s.wm.setCalmCondition();
    s.wm.beginCountedShots();
    line(s, 10, 0.0, 0.0, 19.0);
    s.wm.setMeasuredCondition(45, 5.0, QStringLiteral("gusting, picking up"));
    line(s, 10, 0.0, 0.0, 34.0);
    const bool ok = s.wm.endCapture();
    if (expect) *expect = s.expectation();
    return ok;
}

// F - SIMILAR ACROSS CONDITIONS.
bool seedSimilar(QString* expect)
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("PRONE50"), 30, false)) return false;
    if (!s.wm.startWindMap(QStringLiteral("Case F - Similar across conditions"))) return false;
    s.wm.setCalmCondition();
    s.wm.beginCountedShots();
    line(s, 10, 0.0, 0.0, 20.0);
    s.wm.setMeasuredCondition(270, 2.0);
    line(s, 10, 2.0, 0.0, 21.0);
    const bool ok = s.wm.endCapture();
    if (expect) *expect = s.expectation();
    return ok;
}

// G - RELATIVELY WIDE ACROSS ALL CONDITIONS.
bool seedWideAll(QString* expect)
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("PRONE50"), 30, false)) return false;
    if (!s.wm.startWindMap(QStringLiteral("Case G - Wide across conditions"))) return false;
    s.wm.setCalmCondition();
    s.wm.beginCountedShots();
    line(s, 10, 0.0, 0.0, 45.0);
    s.wm.setMeasuredCondition(270, 2.0);
    line(s, 10, 0.0, 0.0, 48.0);
    const bool ok = s.wm.endCapture();
    if (expect) *expect = s.expectation();
    return ok;
}

// H - FRAGMENTED DATA: many conditions, none deep enough to compare.
bool seedFragmented(QString* expect)
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("PRONE50"), 30, false)) return false;
    if (!s.wm.startWindMap(QStringLiteral("Case H - Fragmented conditions"))) return false;
    s.wm.beginCountedShots();
    s.wm.setCalmCondition();             s.shoot(1.0, 1.0, 10.2); s.shoot(1.4, 0.6, 10.0);
    s.wm.setMeasuredCondition(270, 2.0); s.shoot(3.0, 1.0, 9.8);  s.shoot(3.4, 1.4, 9.9);
    s.wm.setMeasuredCondition(45, 5.0);  s.shoot(-2.0, 2.0, 9.5); s.shoot(-2.4, 1.6, 9.4);
    s.wm.setMeasuredCondition(180, 1.0); s.shoot(0.5, -1.0, 9.9);
    const bool ok = s.wm.endCapture();
    if (expect) *expect = s.expectation();
    return ok;
}

// I - ONE CONDITION ONLY: the INDICATIVE case. No firing direction recorded,
// so the relative-wind fallback message is visible.
bool seedIndicative(QString* expect)
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("PRONE50"), 30, false)) return false;
    if (!s.wm.startWindMap(QStringLiteral("Case I - One condition, no firing direction")))
        return false;
    s.wm.setMeasuredCondition(270, 2.0, QStringLiteral("steady all session"));
    s.wm.beginCountedShots();
    line(s, 9, 1.0, -1.0, 22.0);
    const bool ok = s.wm.endCapture();
    if (expect) *expect = s.expectation();
    return ok;
}

// J - 3P POSITION-SPECIFIC DIFFERENCE, with a firing direction.
bool seedPositionDifference(QString* expect)
{
    Seeder s;
    if (!s.wm.configureSession(QStringLiteral("3P50"), 30, false)) return false;
    if (!s.wm.setFiringDirection(90)) return false;          // firing east
    if (!s.wm.startWindMap(QStringLiteral("Case J - 3P position difference"))) return false;
    struct Pos { int to; double width; };
    const Pos plan[] = { { 1, 42.0 }, { 2, 16.0 }, { 3, 28.0 } };
    for (int i = 0; i < 3; ++i) {
        if (i > 0) {
            if (!s.wm.endPosition()) return false;
            if (!s.wm.changePosition(plan[i].to)) return false;
        }
        s.wm.setCalmCondition();
        s.wm.beginCountedShots();
        line(s, 8, 0.0, 0.0, plan[i].width);
    }
    const bool ok = s.wm.endCapture();
    if (expect) *expect = s.expectation();
    return ok;
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
    struct Case { const char* name; bool (*fn)(QString*); };
    const Case cases[] = {
        { "A  50m Prone - 44 counted, 4 conditions, sighters", &seedProneLong },
        { "B  50m 3P    - Kneeling / Prone / Standing",        &seed3P },
        { "C  50m Prone - INSUFFICIENT SAMPLE",                &seedInsufficient },
        { "D  50m Prone - COMPACT BUT OFFSET (+ firing dir)",  &seedCompactOffset },
        { "E  50m Prone - WIDER UNDER CONDITION",              &seedWider },
        { "F  50m Prone - SIMILAR ACROSS CONDITIONS",          &seedSimilar },
        { "G  50m Prone - DISPERSION ELEVATED ACROSS ALL",     &seedWideAll },
        { "H  50m Prone - FRAGMENTED DATA",                    &seedFragmented },
        { "I  50m Prone - INDICATIVE / no firing direction",   &seedIndicative },
        { "J  50m 3P    - POSITION DIFFERENCE (+ firing dir)", &seedPositionDifference },
    };
    for (const Case& c : cases) {
        QString expect;
        const bool ok = c.fn(&expect);
        std::printf("  %-46s %s\n", c.name, ok ? "OK" : "FAILED");
        // The review sheet states what the app WILL show, read from the same
        // analysisModel() the screen consumes — never a guess.
        if (ok) std::printf("  %-46s -> %s\n", "", qPrintable(expect));
        std::fflush(stdout);
        if (!ok) ++failures;
    }
    std::printf("\nEach session is left in SESSION REVIEW and deliberately NOT closed,\n"
                "so the application offers it through the normal recovery dialog.\n"
                "Resume one, then press \"Complete session\" to open the analysis.\n");
    return failures == 0 ? 0 : 1;
}
