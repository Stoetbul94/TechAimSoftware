// Wind Map — analytics engine (Stage 6).
//
// The engine is pure QtCore and consumes the REDUCER state, so these tests
// build a SessionState directly and assert the arithmetic. Where a formula has
// a hand-checkable answer, the expected value is worked out in the comment so a
// future change cannot quietly redefine the metric.
//
// The rules under test:
//   · MPI needs n >= 3, dispersion n >= 5, a comparison n >= 5 on BOTH sides.
//   · Below threshold the value is WITHHELD and the shortfall reported.
//   · Calm and No Reading are never merged, with each other or with a reading.
//   · Kneeling / Prone / Standing are never pooled, and a reference centre is
//     position-specific.
//   · Sighters are excluded from every counted statistic.
//   · No generated text is prescriptive or causal.
#include "training/WindMapAnalytics.h"
#include "test_support.h"

#include <cmath>

using namespace ta::rel;
using namespace ta::training;

namespace {

// Build one recorded shot. Coordinates in mm, score in points.
WindMapShotRecord rec(qint32 id, bool sighter, qint8 pos, double xMm, double yMm,
                      double score, const WindConditionSnapshot& w, qint32 splitMs = 0)
{
    WindMapShotRecord r;
    r.shotId = id;
    r.sighter = sighter;
    r.position = pos;
    r.shot.xHundredthMm = static_cast<qint32>(qRound(xMm * 100.0));
    r.shot.yHundredthMm = static_cast<qint32>(qRound(yMm * 100.0));
    r.shot.scoreTenths = static_cast<qint16>(qRound(score * 10.0));
    r.shot.splitMs = splitMs;
    r.windValid = w.valid;
    r.windCalm = w.calm;
    r.windDirectionDegrees = w.directionDegrees;
    r.windSpeedHundredthMs = w.speedHundredthMs;
    r.windSource = static_cast<qint8>(w.source);
    r.windRecordedMs = w.recordedMsSinceEpoch;
    r.windNote = w.note;
    return r;
}

WindConditionSnapshot measured(int deg, double ms, qint64 t = 1000)
{
    WindConditionSnapshot w;
    WindConditionSnapshot::measured(deg, ms, t, &w);
    return w;
}
WindConditionSnapshot calm(qint64 t = 1000)     { return WindConditionSnapshot::calmAt(t); }
WindConditionSnapshot noRead()                  { return WindConditionSnapshot::noReading(); }

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

const GroupStats* find(const QVector<GroupStats>& v, const QString& label)
{
    for (const GroupStats& g : v) if (g.label == label) return &g;
    return nullptr;
}

} // namespace

void run_windmap_analytics_tests()
{
    fputs("\n--- wind map analytics engine (stage 6) ---\n", stdout);

    // ── 1. it fails closed on a non-Wind-Map session ────────────────────
    {
        SessionState s;                        // no wmProgramId
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        check(!a.valid, "1. a non-Wind-Map session yields no analysis");
    }

    // ── 2. MPI arithmetic and the n >= 3 threshold ──────────────────────
    {
        SessionState s = baseState(false);
        const WindConditionSnapshot w = measured(270, 2.5);
        // Two shots: MPI must be WITHHELD (threshold is 3).
        s.wmShots << rec(1, false, 0, 10.0, 0.0, 10.0, w)
                  << rec(2, false, 0, 0.0, 10.0, 10.0, w);
        SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const GroupStats* g = find(a.positions[0].byExactCondition, QStringLiteral("W · 2.5 m/s"));
        check(g && g->n == 2, "2. the group holds both shots");
        check(g && !g->hasMpi, "2. MPI is WITHHELD below n=3");
        check(g && g->shotsNeededForMpi() == 1, "2. it reports 1 more shot needed");
        check(g && g->evidence == Evidence::Insufficient, "2. evidence is Insufficient");

        // Third shot completes it. x = (10+0+2)/3 = 4.0, y = (0+10+2)/3 = 4.0
        s.wmShots << rec(3, false, 0, 2.0, 2.0, 10.0, w);
        a = WindMapAnalyticsEngine::analyse(s);
        g = find(a.positions[0].byExactCondition, QStringLiteral("W · 2.5 m/s"));
        check(g && g->hasMpi, "2. MPI is produced at n=3");
        check(g && std::fabs(g->mpiXMm - 4.0) < 1e-9 && std::fabs(g->mpiYMm - 4.0) < 1e-9,
              "2. MPI = the arithmetic mean of x and y",
              g ? QStringLiteral("%1 / %2").arg(g->mpiXMm).arg(g->mpiYMm) : QString());
        check(g && !g->hasDispersion, "2. dispersion is still WITHHELD at n=3");
        check(g && g->evidence == Evidence::Indicative, "2. evidence is Indicative at n=3");
        check(g && g->shotsNeededForDispersion() == 2, "2. it reports 2 more for dispersion");
    }

    // ── 3. dispersion formulas at n = 5 ─────────────────────────────────
    {
        SessionState s = baseState(false);
        const WindConditionSnapshot w = calm();
        // A cross centred on the origin: (0,0) (3,0) (-3,0) (0,4) (0,-4).
        //   MPI      = (0, 0)
        //   radii    = 0, 3, 3, 4, 4  -> mean radius = 14/5 = 2.8
        //   H spread = 3 - (-3) = 6
        //   V spread = 4 - (-4) = 8
        //   max spread = the (0,4)-(0,-4) pair = 8
        s.wmShots << rec(1, false, 0, 0.0,  0.0, 10.0, w)
                  << rec(2, false, 0, 3.0,  0.0, 10.0, w)
                  << rec(3, false, 0, -3.0, 0.0, 10.0, w)
                  << rec(4, false, 0, 0.0,  4.0, 10.0, w)
                  << rec(5, false, 0, 0.0, -4.0, 10.0, w);
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const GroupStats* g = find(a.positions[0].byExactCondition, QStringLiteral("Calm"));
        check(g && g->hasDispersion, "3. dispersion is produced at n=5");
        check(g && std::fabs(g->mpiXMm) < 1e-9 && std::fabs(g->mpiYMm) < 1e-9,
              "3. MPI is the origin");
        check(g && std::fabs(g->meanRadiusMm - 2.8) < 1e-9,
              "3. mean radius = mean distance from the group's OWN MPI (2.8)",
              g ? QString::number(g->meanRadiusMm) : QString());
        check(g && std::fabs(g->horizontalSpreadMm - 6.0) < 1e-9, "3. horizontal spread = 6.0");
        check(g && std::fabs(g->verticalSpreadMm - 8.0) < 1e-9, "3. vertical spread = 8.0");
        check(g && std::fabs(g->groupDiameterMm - 8.0) < 1e-9,
              "3. group diameter = MAXIMUM shot-to-shot spread (8.0)",
              g ? QString::number(g->groupDiameterMm) : QString());
        check(g && g->evidence == Evidence::Sufficient, "3. evidence is Sufficient at n=5");
    }

    // ── 4. Calm, No Reading and a measured reading never merge ──────────
    {
        SessionState s = baseState(false);
        s.wmShots << rec(1, false, 0, 1.0, 1.0, 10.0, calm())
                  << rec(2, false, 0, 1.0, 1.0, 10.0, calm(7777))       // different stamp
                  << rec(3, false, 0, 2.0, 2.0, 10.0, noRead())
                  << rec(4, false, 0, 2.0, 2.0, 10.0, noRead())
                  << rec(5, false, 0, 3.0, 3.0, 10.0, measured(0, 0.0)); // 0 m/s, NOT calm
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const QVector<GroupStats>& ex = a.positions[0].byExactCondition;
        check(ex.size() == 3, "4. three distinct groups", QString::number(ex.size()));
        const GroupStats* c = find(ex, QStringLiteral("Calm"));
        const GroupStats* n = find(ex, QStringLiteral("No reading"));
        check(c && c->n == 2, "4. both calm entries form ONE calm group");
        check(n && n->n == 2, "4. both no-reading shots form ONE no-reading group");
        check(c && n && c->key.hasReading && !n->key.hasReading,
              "4. Calm is a reading; No reading is not — never equivalent");
        check(find(ex, QStringLiteral("N · 0.0 m/s")) != nullptr,
              "4. a MEASURED 0.0 m/s is its own group, not Calm");
    }

    // ── 5. direction grouping and speed-band boundaries ─────────────────
    {
        SessionState s = baseState(false);
        // Boundary values: each belongs to the LOWER band.
        s.wmShots << rec(1, false, 0, 0, 0, 10.0, measured(270, 2.00))   // Light
                  << rec(2, false, 0, 0, 0, 10.0, measured(271, 2.01))   // Moderate
                  << rec(3, false, 0, 0, 0, 10.0, measured(272, 4.00))   // Moderate
                  << rec(4, false, 0, 0, 0, 10.0, measured(273, 4.01))   // Strong
                  << rec(5, false, 0, 0, 0, 10.0, measured(274, 7.00))   // Strong
                  << rec(6, false, 0, 0, 0, 10.0, measured(275, 7.01));  // Very strong
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const QVector<GroupStats>& bands = a.positions[0].bySpeedBand;
        const GroupStats* light = find(bands, QStringLiteral("0-2.0 m/s"));
        const GroupStats* mod   = find(bands, QStringLiteral("2.0-4.0 m/s"));
        const GroupStats* str   = find(bands, QStringLiteral("4.0-7.0 m/s"));
        const GroupStats* vs    = find(bands, QStringLiteral("over 7.0 m/s"));
        check(light && light->n == 1, "5. 2.00 m/s is the TOP of the light band");
        check(mod && mod->n == 2, "5. 2.01 and 4.00 are moderate");
        check(str && str->n == 2, "5. 4.01 and 7.00 are strong");
        check(vs && vs->n == 1, "5. 7.01 is very strong");
        // All six are within 270-275 deg, i.e. the W sector -> ONE direction group.
        const QVector<GroupStats>& dirs = a.positions[0].byDirection;
        const GroupStats* west = find(dirs, QStringLiteral("W"));
        check(west && west->n == 6,
              "5. direction grouping ignores speed — all six are W",
              west ? QString::number(west->n) : QString());
    }

    // ── 6. sighters are excluded from every counted statistic ───────────
    {
        SessionState s = baseState(false);
        const WindConditionSnapshot w = measured(90, 3.0);
        for (int i = 0; i < 5; ++i)
            s.wmShots << rec(i + 1, true, 0, 50.0, 50.0, 5.0, w);   // wild sighters
        for (int i = 0; i < 5; ++i)
            s.wmShots << rec(i + 6, false, 0, 0.0, 0.0, 10.0, w);   // tight counted
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        check(a.countedShots == 5 && a.sighterShots == 5, "6. the two totals are separate");
        const GroupStats* g = find(a.positions[0].byExactCondition, QStringLiteral("E · 3.0 m/s"));
        check(g && g->n == 5, "6. the group counts only the counted shots");
        check(g && std::fabs(g->mpiXMm) < 1e-9 && std::fabs(g->mpiYMm) < 1e-9,
              "6. the sighters did not drag the MPI");
        check(g && std::fabs(g->meanScore - 10.0) < 1e-9,
              "6. the sighters did not enter the mean score");
        check(a.timeline.size() == 10, "6. the timeline still SHOWS the sighters");
    }

    // ── 7. reference centre and observed shift ──────────────────────────
    {
        SessionState s = baseState(false);
        // Calm reference: 5 shots whose x offsets (-2,-1,0,1,2) sum to zero,
        // so the MPI is exactly (0,0). An alternating +/-1 set over five shots
        // would average to -0.2, not 0.
        for (int i = 0; i < 5; ++i)
            s.wmShots << rec(i + 1, false, 0, double(i) - 2.0, 0.0, 10.0, calm());
        // W 2.5: the same symmetric spread, centred on (6.8, 1.2).
        const WindConditionSnapshot w = measured(270, 2.5);
        for (int i = 0; i < 5; ++i)
            s.wmShots << rec(i + 6, false, 0, 6.8 + double(i) - 2.0, 1.2, 10.0, w);
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const PositionAnalysis& p = a.positions[0];
        check(p.reference.kind == ReferenceKind::CalmGroup,
              "7. the calm group is chosen as the reference when large enough");
        check(p.reference.valid && std::fabs(p.reference.xMm) < 1e-9,
              "7. the reference centre is the calm group's MPI");
        const ShiftVector* sv = nullptr;
        for (const ShiftVector& v : p.shifts)
            if (v.label == QStringLiteral("W · 2.5 m/s")) sv = &v;
        check(sv && sv->valid, "7. the shift is produced with n=5 on both sides");
        check(sv && std::fabs(sv->dxMm - 6.8) < 1e-9,
              "7. dx = condition MPI - reference centre (6.8)",
              sv ? QString::number(sv->dxMm) : QString());
        check(sv && std::fabs(sv->dyMm - 1.2) < 1e-9, "7. dy = 1.2");
        check(sv && std::fabs(sv->magnitudeMm - std::sqrt(6.8 * 6.8 + 1.2 * 1.2)) < 1e-9,
              "7. magnitude is the vector length");
        check(sv && sv->directionWords.contains(QLatin1String("right"))
                 && sv->directionWords.contains(QLatin1String("high")),
              "7. the words describe WHERE the group sat, not what to change",
              sv ? sv->directionWords : QString());
    }

    // ── 8. a comparison is withheld below n=5 on either side ────────────
    {
        SessionState s = baseState(false);
        for (int i = 0; i < 5; ++i)
            s.wmShots << rec(i + 1, false, 0, 0.0, 0.0, 10.0, calm());
        // Only 3 shots in the compared condition: MPI exists, comparison does not.
        const WindConditionSnapshot w = measured(45, 5.0);
        for (int i = 0; i < 3; ++i)
            s.wmShots << rec(i + 6, false, 0, 9.0, 0.0, 10.0, w);
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const ShiftVector* sv = nullptr;
        for (const ShiftVector& v : a.positions[0].shifts)
            if (v.label == QStringLiteral("NE · 5.0 m/s")) sv = &v;
        check(sv && !sv->valid, "8. the comparison is WITHHELD at n=3");
        check(sv && sv->shotsNeeded == 2, "8. it states 2 more shots are needed");
        bool saidSo = false;
        for (const Finding& f : a.findings)
            if (f.category == PatternCategory::InsufficientSample
                && f.text.contains(QLatin1String("No reliable comparison"))) saidSo = true;
        check(saidSo, "8. a finding says plainly that no comparison can be made");
    }

    // ── 9. 3P positions are analysed independently and never pooled ─────
    {
        SessionState s = baseState(true);
        const WindConditionSnapshot w = measured(270, 2.5);
        // Kneeling centred at (20,20); Prone at (0,0); Standing at (-20,-20).
        for (int i = 0; i < 5; ++i) s.wmShots << rec(i + 1,  false, 1,  20.0,  20.0, 9.0, w);
        for (int i = 0; i < 5; ++i) s.wmShots << rec(i + 6,  false, 2,   0.0,   0.0, 10.5, w);
        for (int i = 0; i < 5; ++i) s.wmShots << rec(i + 11, false, 3, -20.0, -20.0, 8.0, w);
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        check(a.positions.size() == 3, "9. three position analyses");
        check(a.positions[0].positionName == QLatin1String("Kneeling")
              && a.positions[1].positionName == QLatin1String("Prone")
              && a.positions[2].positionName == QLatin1String("Standing"),
              "9. in K -> P -> S order");
        for (const PositionAnalysis& p : a.positions)
            check(p.countedShots == 5,
                  QString(QStringLiteral("9. %1 counts only its own shots")).arg(p.positionName));
        const GroupStats* k = find(a.positions[0].byExactCondition, QStringLiteral("W · 2.5 m/s"));
        const GroupStats* pr = find(a.positions[1].byExactCondition, QStringLiteral("W · 2.5 m/s"));
        check(k && std::fabs(k->mpiXMm - 20.0) < 1e-9, "9. Kneeling's MPI is Kneeling's alone");
        check(pr && std::fabs(pr->mpiXMm) < 1e-9, "9. Prone's MPI is unaffected by Kneeling");
        // The reference centre must be position-specific.
        check(std::fabs(a.positions[0].reference.xMm - 20.0) < 1e-9
              && std::fabs(a.positions[2].reference.xMm + 20.0) < 1e-9,
              "9. each position has its OWN reference centre — Standing is never "
              "measured against a Prone centre");
    }

    // ── 10. the timeline preserves order and marks the transitions ──────
    {
        SessionState s = baseState(false);
        s.wmShots << rec(1, true,  0, 0, 0, 9.0, calm(), 1000)
                  << rec(2, true,  0, 0, 0, 9.0, calm(), 2000)
                  << rec(3, false, 0, 0, 0, 10.0, calm(), 3000)
                  << rec(4, false, 0, 0, 0, 10.0, measured(270, 2.5), 4000)
                  << rec(5, false, 0, 0, 0, 10.0, measured(270, 2.5), 5000);
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        check(a.timeline.size() == 5, "10. every shot appears once");
        bool ordered = true;
        for (int i = 1; i < a.timeline.size(); ++i)
            if (a.timeline[i].shotId <= a.timeline[i - 1].shotId) ordered = false;
        check(ordered, "10. the timeline is in journal order");
        check(a.timeline[2].phaseChangedBefore,
              "10. the sighter -> counted transition is marked");
        check(!a.timeline[1].phaseChangedBefore, "10. and only once");
        check(a.timeline[3].conditionChangedBefore,
              "10. the first shot under a new condition is marked");
        check(!a.timeline[4].conditionChangedBefore,
              "10. an unchanged condition is not marked again");
    }

    // ── 11. no generated text is prescriptive or causal ─────────────────
    {
        // Drive several shapes of session through the generator and scan
        // everything it produced.
        QStringList text;
        auto harvest = [&text](const SessionAnalysis& a) {
            for (const Finding& f : a.findings) { text << f.text << f.suggestion; }
            text << a.limitations;
        };
        {   // a clear offset
            SessionState s = baseState(false);
            for (int i = 0; i < 6; ++i) s.wmShots << rec(i + 1, false, 0, 0.0, 0.0, 10.0, calm());
            for (int i = 0; i < 6; ++i) s.wmShots << rec(i + 7, false, 0, 8.0, 0.0, 10.0, measured(270, 2.5));
            harvest(WindMapAnalyticsEngine::analyse(s));
        }
        {   // wide everywhere
            SessionState s = baseState(false);
            for (int i = 0; i < 6; ++i)
                s.wmShots << rec(i + 1, false, 0, (i % 2 ? 40.0 : -40.0), 0.0, 6.0, calm());
            harvest(WindMapAnalyticsEngine::analyse(s));
        }
        {   // nothing at all
            SessionState s = baseState(false);
            s.wmShots << rec(1, false, 0, 0, 0, 10.0, calm());
            harvest(WindMapAnalyticsEngine::analyse(s));
        }
        {   // 3P
            SessionState s = baseState(true);
            for (int i = 0; i < 6; ++i) s.wmShots << rec(i + 1,  false, 1, 5.0, 0.0, 9.0, calm());
            for (int i = 0; i < 6; ++i) s.wmShots << rec(i + 7,  false, 2, 0.0, 0.0, 10.0, calm());
            harvest(WindMapAnalyticsEngine::analyse(s));
        }
        const QString all = text.join(QLatin1Char('\n'));
        const char* prohibited[] = {
            "sight click", "clicks left", "clicks right", "clicks up", "clicks down",
            "add ", "remove ", "hold left", "hold right", "aim at", "aim off",
            "the wind caused", "the wind pushed", "wind caused", "wind pushed",
            "you must", "you should", "guaranteed", "will correct", "corrects for",
        };
        bool clean = true;
        QString found;
        for (const char* p : prohibited)
            if (all.contains(QLatin1String(p), Qt::CaseInsensitive)) {
                clean = false; found += QLatin1String(p) + QStringLiteral("|");
            }
        check(clean, "11. no generated text is prescriptive or causal", found);
        check(!all.isEmpty(), "11. the generator did produce text");
        check(all.contains(QLatin1String("may not represent wind across the complete bullet path")),
              "11. the observation limitation is always stated");
        check(all.contains(QLatin1String("never an official competition result")),
              "11. the training-only limitation is always stated");
    }

    // ── 12. old sessions stay readable ──────────────────────────────────
    {
        // A pre-Stage-6 session: shots recorded with no note, no phase, source 0.
        SessionState s = baseState(false);
        s.wmPhase = 0;                          // as a v4/v5 snapshot would restore
        for (int i = 0; i < 5; ++i)
            s.wmShots << rec(i + 1, false, 0, 1.0, 1.0, 10.0, measured(180, 1.0));
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        check(a.valid && a.countedShots == 5,
              "12. a session recorded before Stage 6 still analyses");
        check(!a.positions.isEmpty() && a.positions[0].byExactCondition.size() == 1,
              "12. its conditions still group");
    }

    // ── 13. UI-WIND-006: every finding declares its SCOPE ───────────────
    {
        // The defect: a session-level position comparison was shown unchanged
        // under Kneeling, Prone and Standing, so an athlete could read a
        // cross-position statement as a result about one position.
        SessionState s = baseState(true);
        const WindConditionSnapshot w = calm();
        for (int i = 0; i < 8; ++i) s.wmShots << rec(i + 1,  false, 1, 20.0 + (i % 4) * 3.0, 20.0, 9.0, w);
        for (int i = 0; i < 8; ++i) s.wmShots << rec(i + 9,  false, 2, double(i % 4) - 1.5, 0.0, 10.5, w);
        for (int i = 0; i < 8; ++i) s.wmShots << rec(i + 17, false, 3, -20.0 - (i % 4) * 2.0, -20.0, 8.0, w);
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);

        check(!a.findings.isEmpty(), "13. findings were produced");
        int sessionScoped = 0, positionScoped = 0, conditionScoped = 0;
        bool everyPositionOneIsNamed = true;
        bool crossPositionIsSession = true;
        for (const Finding& f : a.findings) {
            if (f.scope == FindingScope::Session)   ++sessionScoped;
            if (f.scope == FindingScope::Position) {
                ++positionScoped;
                if (f.positionName.isEmpty() || f.position == 0) everyPositionOneIsNamed = false;
            }
            if (f.scope == FindingScope::Condition) ++conditionScoped;
            // The cross-position comparison must NEVER be position-scoped.
            if (f.category == PatternCategory::PositionSpecificDifference
                && f.scope != FindingScope::Session)
                crossPositionIsSession = false;
        }
        check(sessionScoped + positionScoped + conditionScoped == a.findings.size(),
              "13. every finding carries one of the three scopes");
        check(everyPositionOneIsNamed,
              "13. a position-scoped finding names its position");
        check(crossPositionIsSession,
              "13. the cross-position comparison is SESSION-scoped, never position-scoped");
        check(sessionScoped >= 1,
              "13. at least one session-level finding exists for the Overview");

        // Filtering the way the view does must not leak a session finding into
        // a position, and must not drop a position's own findings.
        for (int pos = 1; pos <= 3; ++pos) {
            int leaked = 0;
            for (const Finding& f : a.findings) {
                if (f.scope == FindingScope::Session) continue;   // view excludes these
                if (f.position != pos) continue;
                ++leaked;
                (void)leaked;
            }
            // Nothing to assert on count; the real assertion is that no
            // session finding claims a position.
        }
        bool noSessionClaimsAPosition = true;
        for (const Finding& f : a.findings)
            if (f.scope == FindingScope::Session && f.position != 0)
                noSessionClaimsAPosition = false;
        check(noSessionClaimsAPosition,
              "13. a session-level finding never claims a position id");
    }

    // ── 14. analytics values are UNCHANGED by the UX phase ──────────────
    {
        // Stage 6.1.1 was presentation only. Re-assert the hand-checked
        // dispersion answers from case 3 so a UX change cannot have moved a
        // formula without this failing.
        SessionState s = baseState(false);
        const WindConditionSnapshot w = calm();
        s.wmShots << rec(1, false, 0, 0.0,  0.0, 10.0, w)
                  << rec(2, false, 0, 3.0,  0.0, 10.0, w)
                  << rec(3, false, 0, -3.0, 0.0, 10.0, w)
                  << rec(4, false, 0, 0.0,  4.0, 10.0, w)
                  << rec(5, false, 0, 0.0, -4.0, 10.0, w);
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const GroupStats* g = find(a.positions[0].byExactCondition, QStringLiteral("Calm"));
        check(g && std::fabs(g->meanRadiusMm - 2.8) < 1e-9
              && std::fabs(g->groupDiameterMm - 8.0) < 1e-9
              && std::fabs(g->horizontalSpreadMm - 6.0) < 1e-9
              && std::fabs(g->verticalSpreadMm - 8.0) < 1e-9,
              "14. every dispersion formula is unchanged by the UX redesign");
        check(kMinSamplesMpi == 3 && kMinSamplesDispersion == 5 && kMinSamplesComparison == 5,
              "14. the approved sample thresholds are unchanged");
    }
}
