// Wind Map — EVID-WM-001: sample-size-safe dispersion comparison.
//
// The defect: verdict classification used groupDiameterMm (extreme spread), an
// order statistic fixed by the two most distant shots. Adding shots gives a
// group more opportunities to contain an extreme pair, so comparing groups of
// unequal size could report "wider" partly from sample count.
//
// The fix: classify on RADIAL RMS DISPERSION
//     radialRmsMm = sqrt( SUM((x-mx)^2 + (y-my)^2) / (n-1) )
// which uses every shot. Extreme spread stays on screen as a descriptive size.
//
// These are DETERMINISTIC, geometrically controlled datasets — evenly spaced
// points on a line, on a circle, and cluster constructions — so every expected
// value is computable by hand. No random data anywhere in this file.
#include "training/WindMapVerdict.h"
#include "test_support.h"

#include <cmath>

using namespace ta::rel;
using namespace ta::training;

namespace {

WindMapShotRecord rec(qint32 id, bool sighter, qint8 pos, double xMm, double yMm,
                      double score, const WindConditionSnapshot& w)
{
    WindMapShotRecord r;
    r.shotId = id; r.sighter = sighter; r.position = pos;
    r.shot.xHundredthMm = static_cast<qint32>(qRound(xMm * 100.0));
    r.shot.yHundredthMm = static_cast<qint32>(qRound(yMm * 100.0));
    r.shot.scoreTenths = static_cast<qint16>(qRound(score * 10.0));
    r.windValid = w.valid; r.windCalm = w.calm;
    r.windDirectionDegrees = w.directionDegrees;
    r.windSpeedHundredthMs = w.speedHundredthMs;
    r.windSource = static_cast<qint8>(w.source);
    r.windRecordedMs = w.recordedMsSinceEpoch; r.windNote = w.note;
    return r;
}
WindConditionSnapshot measured(int deg, double ms)
{
    WindConditionSnapshot w; WindConditionSnapshot::measured(deg, ms, 1000, &w); return w;
}
WindConditionSnapshot calm() { return WindConditionSnapshot::calmAt(1000); }

SessionState baseState(bool threeP)
{
    SessionState s;
    s.wmProgramId = QStringLiteral("wind_map");
    s.wmActive = true;
    s.wmDisciplineId = threeP ? QStringLiteral("3P50") : QStringLiteral("PRONE50");
    s.wmThreePositions = threeP;
    s.sessionKind = QStringLiteral("Training");
    return s;
}

// n points evenly spaced along x, centred on cx, with the given STEP between
// neighbours. Sample SD of such a set is exactly step*sqrt(n(n+1)/12), and its
// extreme spread is exactly step*(n-1) — both known before the engine runs.
void addEvenLine(SessionState& s, qint32 firstId, qint8 pos, int n,
                 double cx, double cy, double stepMm, const WindConditionSnapshot& w)
{
    for (int i = 0; i < n; ++i) {
        const double t = (double(i) - double(n - 1) / 2.0) * stepMm;
        s.wmShots << rec(firstId + i, false, pos, cx + t, cy, 10.0, w);
    }
}

// n points evenly spaced on a circle of radius R about (cx, cy). For n >= 3 the
// centroid is exactly the centre, every point is exactly R from it, so
//   radialRms = R * sqrt(n / (n - 1))     and     meanRadius = R.
void addCircle(SessionState& s, qint32 firstId, qint8 pos, int n,
               double cx, double cy, double rMm, const WindConditionSnapshot& w)
{
    for (int i = 0; i < n; ++i) {
        const double a = 2.0 * M_PI * double(i) / double(n);
        s.wmShots << rec(firstId + i, false, pos,
                         cx + rMm * std::cos(a), cy + rMm * std::sin(a), 10.0, w);
    }
}

double circleRms(int n, double rMm) { return rMm * std::sqrt(double(n) / double(n - 1)); }

// The single reference group used by most cases: calm, well sampled.
const GroupStats* groupNamed(const SessionAnalysis& a, int posIndex, const QString& label)
{
    if (posIndex >= a.positions.size()) return nullptr;
    for (const GroupStats& g : a.positions[posIndex].byExactCondition)
        if (g.label == label) return &g;
    return nullptr;
}

QVector<Verdict> verdictsFor(const SessionState& s)
{
    return WindMapVerdictEngine::evaluate(WindMapAnalyticsEngine::analyse(s));
}

bool hasCategory(const QVector<Verdict>& v, VerdictCategory c)
{
    for (const Verdict& x : v) if (x.category == c) return true;
    return false;
}

// Builds calm(reference) + measured(comparison) in one Prone position.
SessionState twoGroups(int refN, double refStep, int cmpN, double cmpStep)
{
    SessionState s = baseState(false);
    addEvenLine(s, 1, 0, refN, 0.0, 0.0, refStep, calm());
    addEvenLine(s, 100, 0, cmpN, 0.0, 0.0, cmpStep, measured(270, 3.0));
    return s;
}

const double kTol = 0.02;   // mm, comfortably above hundredth-mm storage rounding

} // namespace

void run_windmap_dispersion_tests()
{
    fputs("\n--- wind map dispersion: EVID-WM-001 sample-size safety ---\n", stdout);

    // ── 0. the estimator itself ─────────────────────────────────────────
    {
        SessionState s = baseState(false);
        // A circle: every point exactly 10 mm from a centroid at the origin.
        addCircle(s, 1, 0, 12, 0.0, 0.0, 10.0, calm());
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const GroupStats* g = groupNamed(a, 0, QStringLiteral("Calm"));
        check(g && g->hasDispersion, "0. a 12-shot circle yields dispersion");
        if (g) {
            check(std::fabs(g->meanRadiusMm - 10.0) < kTol,
                  "0. mean radius of a circle is its radius",
                  QString::number(g->meanRadiusMm));
            check(std::fabs(g->radialRmsMm - circleRms(12, 10.0)) < kTol,
                  "0. radial RMS is R*sqrt(n/(n-1)) exactly",
                  QString::number(g->radialRmsMm));
            // The trace identity: rms^2 == hSd^2 + vSd^2. Not assumed — asserted.
            const double lhs = g->radialRmsMm * g->radialRmsMm;
            const double rhs = g->horizontalSdMm * g->horizontalSdMm
                             + g->verticalSdMm * g->verticalSdMm;
            check(std::fabs(lhs - rhs) < 1e-6,
                  "0. radialRms^2 == hSd^2 + vSd^2 (trace of the sample covariance)",
                  QString::number(lhs - rhs));
        }
        check(a.analyticsVersion == QLatin1String("windmap-analytics-v2"),
              "0. the analysis is stamped windmap-analytics-v2", a.analyticsVersion);
        check(std::fabs(a.ringSpacingMm - 8.0) < 1e-9,
              "0. 50 m rifle ring spacing is resolved centrally");
    }

    // ── 1-4. equivalent dispersion at unequal sample counts ─────────────
    // Steps chosen so every pair has the SAME sample SD. If sample size could
    // still drive the verdict, one of these four would fire.
    {
        struct Case { int refN, cmpN; const char* name; };
        const Case cases[] = {
            { 5,  5,  "1. reference n=5 vs comparison n=5" },
            { 5,  10, "2. reference n=5 vs comparison n=10" },
            { 5,  20, "3. reference n=5 vs comparison n=20" },
            { 20, 5,  "4. reference n=20 vs comparison n=5" },
        };
        const double targetSd = 6.0;
        for (const Case& c : cases) {
            const double refStep = targetSd / std::sqrt(double(c.refN) * (c.refN + 1) / 12.0);
            const double cmpStep = targetSd / std::sqrt(double(c.cmpN) * (c.cmpN + 1) / 12.0);
            const SessionState s = twoGroups(c.refN, refStep, c.cmpN, cmpStep);
            const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
            const GroupStats* ref = groupNamed(a, 0, QStringLiteral("Calm"));
            const GroupStats* cmp = groupNamed(a, 0, QStringLiteral("W · 3.0 m/s"));
            check(ref && cmp && ref->hasDispersion && cmp->hasDispersion,
                  QString("%1 — both groups measured").arg(c.name));
            if (ref && cmp) {
                check(std::fabs(ref->radialRmsMm - cmp->radialRmsMm) < 0.05,
                      QString("%1 — radial RMS matches by construction").arg(c.name),
                      QString("%1 vs %2").arg(ref->radialRmsMm).arg(cmp->radialRmsMm));
            }
            check(!hasCategory(verdictsFor(s), VerdictCategory::WiderUnderCondition),
                  QString("%1 — equivalent dispersion produces NO wider verdict").arg(c.name));
        }
    }

    // ── 5. equal radial RMS, different extreme spread ───────────────────
    // The regression that proves the fix. n=5 and n=20 with identical sample
    // SD: the 20-shot group's extreme spread exceeds 1.25x the 5-shot group's,
    // so the OLD extreme-spread rule would have refused to call it compact
    // while the corrected rule correctly does.
    {
        const double targetSd = 6.0;
        const double refStep = targetSd / std::sqrt(5.0 * 6.0 / 12.0);
        const double cmpStep = targetSd / std::sqrt(20.0 * 21.0 / 12.0);
        const SessionState s = twoGroups(5, refStep, 20, cmpStep);
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const GroupStats* ref = groupNamed(a, 0, QStringLiteral("Calm"));
        const GroupStats* cmp = groupNamed(a, 0, QStringLiteral("W · 3.0 m/s"));
        check(ref && cmp, "5. both groups present");
        if (ref && cmp) {
            check(std::fabs(ref->radialRmsMm - cmp->radialRmsMm) < 0.05,
                  "5. radial RMS is equal");
            const double spreadRatio = cmp->groupDiameterMm / ref->groupDiameterMm;
            check(spreadRatio > kCompactRelativeToReference,
                  "5. extreme-spread ratio exceeds 1.25 from SAMPLE SIZE ALONE",
                  QString::number(spreadRatio));
            // The old rule would have failed; the corrected one passes.
            check(!WindMapVerdictEngine::isCompact(cmp->groupDiameterMm, ref->groupDiameterMm),
                  "5. the SUPERSEDED extreme-spread test would have refused 'compact'");
            check(WindMapVerdictEngine::isCompact(cmp->radialRmsMm, ref->radialRmsMm),
                  "5. the corrected radial-RMS test correctly calls it compact");
        }
        check(!hasCategory(verdictsFor(s), VerdictCategory::WiderUnderCondition),
              "5. no wider verdict is produced");
    }

    // ── 6. equal extreme spread, different radial RMS ───────────────────
    // Reference: 10 shots on a circle of radius 10 (extreme spread 20).
    // Comparison: 2 shots at (+/-10, 0) and 8 at the centre — extreme spread is
    // also 20, but the shots are far less dispersed. Extreme spread cannot tell
    // these apart; radial RMS can.
    {
        SessionState s = baseState(false);
        addCircle(s, 1, 0, 10, 0.0, 0.0, 10.0, calm());
        s.wmShots << rec(100, false, 0, -10.0, 0.0, 10.0, measured(270, 3.0));
        s.wmShots << rec(101, false, 0,  10.0, 0.0, 10.0, measured(270, 3.0));
        for (int i = 0; i < 8; ++i)
            s.wmShots << rec(102 + i, false, 0, 0.0, 0.0, 10.0, measured(270, 3.0));
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const GroupStats* ref = groupNamed(a, 0, QStringLiteral("Calm"));
        const GroupStats* cmp = groupNamed(a, 0, QStringLiteral("W · 3.0 m/s"));
        check(ref && cmp, "6. both groups present");
        if (ref && cmp) {
            check(std::fabs(ref->groupDiameterMm - cmp->groupDiameterMm) < kTol,
                  "6. extreme spread is identical (20.0 mm both)",
                  QString("%1 vs %2").arg(ref->groupDiameterMm).arg(cmp->groupDiameterMm));
            check(cmp->radialRmsMm < ref->radialRmsMm * 0.6,
                  "6. radial RMS separates them where extreme spread cannot",
                  QString("%1 vs %2").arg(cmp->radialRmsMm).arg(ref->radialRmsMm));
            check(WindMapVerdictEngine::isCompact(cmp->radialRmsMm, ref->radialRmsMm),
                  "6. the tighter distribution is classified compact");
        }
    }

    // ── 7. additional INTERIOR shots never create a wider verdict ───────
    {
        SessionState s = baseState(false);
        addCircle(s, 1, 0, 8, 0.0, 0.0, 10.0, calm());
        // Same perimeter, plus ten interior shots. A real distribution that is
        // no more dispersed — it is more concentrated.
        addCircle(s, 100, 0, 8, 0.0, 0.0, 10.0, measured(270, 3.0));
        for (int i = 0; i < 10; ++i)
            s.wmShots << rec(200 + i, false, 0, (i % 2 ? 1.0 : -1.0), 0.0, 10.0,
                             measured(270, 3.0));
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const GroupStats* ref = groupNamed(a, 0, QStringLiteral("Calm"));
        const GroupStats* cmp = groupNamed(a, 0, QStringLiteral("W · 3.0 m/s"));
        check(ref && cmp && cmp->n == 18 && ref->n == 8, "7. 8 vs 18 counted shots");
        if (ref && cmp) {
            check(std::fabs(ref->groupDiameterMm - cmp->groupDiameterMm) < kTol,
                  "7. extreme spread is unchanged by interior shots");
            check(cmp->radialRmsMm < ref->radialRmsMm,
                  "7. radial RMS correctly falls when interior shots are added");
        }
        check(!hasCategory(verdictsFor(s), VerdictCategory::WiderUnderCondition),
              "7. adding interior shots produces NO wider verdict");
    }

    // ── 8. a genuinely wider radial distribution DOES fire ──────────────
    {
        SessionState s = baseState(false);
        addCircle(s, 1, 0, 10, 0.0, 0.0, 6.0, calm());
        addCircle(s, 100, 0, 10, 0.0, 0.0, 6.0 * 1.60, measured(270, 3.0));
        const QVector<Verdict> v = verdictsFor(s);
        check(hasCategory(v, VerdictCategory::WiderUnderCondition),
              "8. a 1.6x wider radial distribution IS reported as wider");
        // And it reports the metric it classified on.
        for (const Verdict& x : v)
            if (x.category == VerdictCategory::WiderUnderCondition) {
                check(x.supportingMetricIds.contains(QStringLiteral("radialRmsMm")),
                      "8. the wider verdict cites radialRmsMm");
                check(!x.supportingMetricIds.contains(QStringLiteral("groupDiameterMm")),
                      "8. the wider verdict does NOT cite extreme spread");
                check(!x.headline.contains(QStringLiteral("radial RMS")),
                      "8. the headline does not lead with the estimator's name",
                      x.headline);
                check(x.observedPattern.contains(QStringLiteral("radial RMS")),
                      "8. the measurements line names the measure that was compared",
                      x.observedPattern);
            }
    }

    // ── 9-12. ratio boundaries, exact ───────────────────────────────────
    {
        check(WindMapVerdictEngine::isCompact(12.50, 10.0),
              "9. exactly 1.25x is compact");
        check(!WindMapVerdictEngine::isWider(12.50, 10.0),
              "9. exactly 1.25x is not wider");
        check(!WindMapVerdictEngine::isCompact(12.51, 10.0),
              "10. immediately above 1.25x is not compact");
        check(!WindMapVerdictEngine::isWider(12.51, 10.0),
              "10. immediately above 1.25x is not wider either — indeterminate");
        check(!WindMapVerdictEngine::isWider(14.99, 10.0),
              "11. immediately below 1.50x is not wider");
        check(!WindMapVerdictEngine::isCompact(14.99, 10.0),
              "11. immediately below 1.50x is not compact — still indeterminate");
        check(WindMapVerdictEngine::isWider(15.00, 10.0),
              "12. exactly 1.50x is wider");
    }

    // ── 13. both sides must meet the sample minimum ─────────────────────
    {
        SessionState s = baseState(false);
        addCircle(s, 1, 0, 10, 0.0, 0.0, 6.0, calm());
        addCircle(s, 100, 0, 4, 0.0, 0.0, 6.0 * 3.0, measured(270, 3.0));   // n=4
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const GroupStats* cmp = groupNamed(a, 0, QStringLiteral("W · 3.0 m/s"));
        check(cmp && !cmp->hasDispersion,
              "13. a 4-shot group yields no dispersion at all");
        const QVector<Verdict> v = verdictsFor(s);
        check(!hasCategory(v, VerdictCategory::WiderUnderCondition),
              "13. no wider verdict below the sample minimum, however wide the shots");
        check(hasCategory(v, VerdictCategory::InsufficientSample),
              "13. the shortfall is reported instead of being ignored");
    }

    // ── 14. 3P positions stay separate ──────────────────────────────────
    {
        SessionState s = baseState(true);
        // Kneeling: tight in both conditions. Standing: wide in both.
        addCircle(s, 1,   1, 10, 0.0, 0.0, 4.0, calm());
        addCircle(s, 20,  1, 10, 0.0, 0.0, 4.0, measured(270, 3.0));
        addCircle(s, 40,  3, 10, 0.0, 0.0, 16.0, calm());
        addCircle(s, 60,  3, 10, 0.0, 0.0, 16.0, measured(270, 3.0));
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        check(a.positions.size() == 2, "14. two positions are analysed separately",
              QString::number(a.positions.size()));
        // Kneeling's tight groups must not be pooled with Standing's wide ones.
        for (const PositionAnalysis& p : a.positions)
            for (const GroupStats& g : p.byExactCondition) {
                if (!g.hasDispersion) continue;
                check(g.n == 10, "14. no group pools shots across positions",
                      QString("%1 n=%2").arg(p.positionName).arg(g.n));
            }
        const QVector<Verdict> v = verdictsFor(s);
        // The elevated-dispersion verdict belongs to Standing alone.
        for (const Verdict& x : v)
            if (x.category == VerdictCategory::WideAcrossConditions)
                check(x.scope == VerdictScope::Position && x.position == 3,
                      "14. elevated dispersion is scoped to the position that showed it",
                      QString("pos %1").arg(x.position));
    }

    // ── 15. sighters are excluded from every dispersion figure ──────────
    {
        SessionState s = baseState(false);
        addCircle(s, 1, 0, 10, 0.0, 0.0, 6.0, calm());
        // Four wild sighters that would wreck any dispersion measure.
        for (int i = 0; i < 4; ++i)
            s.wmShots << rec(500 + i, true, 0, 60.0 * (i % 2 ? 1 : -1), 60.0, 5.0, calm());
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const GroupStats* g = groupNamed(a, 0, QStringLiteral("Calm"));
        check(g && g->n == 10, "15. sighters are not counted into the group",
              QString::number(g ? g->n : -1));
        if (g) {
            check(std::fabs(g->radialRmsMm - circleRms(10, 6.0)) < kTol,
                  "15. radial RMS is unaffected by sighters",
                  QString::number(g->radialRmsMm));
            check(std::fabs(g->groupDiameterMm - 12.0) < 0.2,
                  "15. extreme spread is unaffected by sighters",
                  QString::number(g->groupDiameterMm));
        }
    }

    // ── 16. the analysis never mutates stored shot data ─────────────────
    {
        SessionState s = baseState(false);
        addCircle(s, 1, 0, 10, 0.0, 0.0, 6.0, calm());
        addCircle(s, 100, 0, 12, 3.0, -2.0, 9.0, measured(270, 3.0));
        const QVector<WindMapShotRecord> before = s.wmShots;
        WindMapAnalyticsEngine::analyse(s);
        WindMapVerdictEngine::evaluate(WindMapAnalyticsEngine::analyse(s));
        check(s.wmShots.size() == before.size(), "16. no shot is added or removed");
        bool identical = true;
        for (int i = 0; i < s.wmShots.size() && i < before.size(); ++i) {
            const WindMapShotRecord& x = s.wmShots[i];
            const WindMapShotRecord& y = before[i];
            if (x.shotId != y.shotId || x.sighter != y.sighter || x.position != y.position
                || x.shot.xHundredthMm != y.shot.xHundredthMm
                || x.shot.yHundredthMm != y.shot.yHundredthMm
                || x.shot.scoreTenths != y.shot.scoreTenths
                || x.windValid != y.windValid || x.windCalm != y.windCalm
                || x.windDirectionDegrees != y.windDirectionDegrees
                || x.windSpeedHundredthMs != y.windSpeedHundredthMs
                || x.windRecordedMs != y.windRecordedMs || x.windNote != y.windNote) {
                identical = false;
                break;
            }
        }
        check(identical, "16. every stored coordinate and wind snapshot is byte-identical after analysis");
    }

    // ── 17. elevated dispersion is ring-scaled, and never a judgement ───
    {
        SessionState s = baseState(false);
        // Radial RMS comfortably above 1.5 ring spacings (8.0 mm) in both.
        addCircle(s, 1,   0, 10, 0.0, 0.0, 16.0, calm());
        addCircle(s, 100, 0, 10, 0.0, 0.0, 17.0, measured(270, 3.0));
        const QVector<Verdict> v = verdictsFor(s);
        check(hasCategory(v, VerdictCategory::WideAcrossConditions),
              "17. sustained high dispersion is reported");
        for (const Verdict& x : v) {
            if (x.category != VerdictCategory::WideAcrossConditions) continue;
            check(x.headline.contains(QStringLiteral("Dispersion remained elevated")),
                  "17. the approved wording is used", x.headline);
            const QString all = x.headline + x.observedPattern + x.interpretation
                              + x.nextTrainingStep + x.coachDecision
                              + x.limitations.join(QStringLiteral(" "));
            for (const char* bad : { "poor", "bad", "weak", "unacceptable", "inadequate" })
                check(!all.contains(QLatin1String(bad), Qt::CaseInsensitive),
                      QString("17. the verdict never says '%1'").arg(QLatin1String(bad)));
            check(x.limitations.join(QStringLiteral(" ")).contains(QStringLiteral("provisional"),
                                                                   Qt::CaseInsensitive),
                  "17. the provisional status of the rule is stated to the athlete");
            check(x.supportingMetricIds.contains(QStringLiteral("radialRmsMm")),
                  "17. it cites radial RMS, not extreme spread");
        }
        // Below the bar, the verdict must not appear at all.
        SessionState tight = baseState(false);
        addCircle(tight, 1,   0, 10, 0.0, 0.0, 3.0, calm());
        addCircle(tight, 100, 0, 10, 0.0, 0.0, 3.2, measured(270, 3.0));
        check(!hasCategory(verdictsFor(tight), VerdictCategory::WideAcrossConditions),
              "17. tight groups never produce an elevated-dispersion verdict");
    }
}
