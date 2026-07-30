// Wind Map — verdict engine boundaries (Stage 6.1.2).
//
// Every threshold in docs/training-lab-wind-map-verdict-rules.md is tested AT
// ITS EDGE, in both directions. A rule that cannot be pinned to an exact
// boundary is a magic number, which the brief prohibits.
//
// Also asserts what the engine may never say: no causal claim, no sight or
// hold prescription, no statistical certainty, and REPEATED never assigned
// from a single session.
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

QVector<Verdict> verdictsFor(const SessionState& s)
{
    return WindMapVerdictEngine::evaluate(WindMapAnalyticsEngine::analyse(s));
}

bool hasCategory(const QVector<Verdict>& v, VerdictCategory c)
{
    for (const Verdict& x : v) if (x.category == c) return true;
    return false;
}
const Verdict* firstOf(const QVector<Verdict>& v, VerdictCategory c)
{
    for (const Verdict& x : v) if (x.category == c) return &x;
    return nullptr;
}

// A group of n shots on a horizontal line of the given width, centred at cx.
// Width is the extreme spread, so groupDiameter == width exactly.
void addLineGroup(SessionState& s, qint32 firstId, qint8 pos, int n,
                  double cx, double cy, double widthMm, const WindConditionSnapshot& w)
{
    for (int i = 0; i < n; ++i) {
        const double t = (n == 1) ? 0.0 : (double(i) / double(n - 1)) - 0.5;   // -0.5..+0.5
        s.wmShots << rec(firstId + i, false, pos, cx + t * widthMm, cy, 10.0, w);
    }
}

} // namespace

void run_windmap_verdict_tests()
{
    fputs("\n--- wind map verdict engine (stage 6.1.2) ---\n", stdout);

    // ── 1. threshold predicates, at the exact boundary ──────────────────
    {
        // Meaningful offset = max(3.0 mm, 1.0 x reference mean radius).
        // Reference mean radius 2.0 -> the absolute floor of 3.0 governs.
        check(!WindMapVerdictEngine::isMeaningfulOffset(2.99, 2.0),
              "1. 2.99 mm is below the 3.0 mm absolute floor");
        check(WindMapVerdictEngine::isMeaningfulOffset(3.00, 2.0),
              "1. exactly 3.00 mm CLEARS the floor (>=)");
        // Reference mean radius 8.0 -> the scale-aware bar governs.
        check(!WindMapVerdictEngine::isMeaningfulOffset(7.99, 8.0),
              "1. 7.99 mm is inside a group whose own mean radius is 8.0");
        check(WindMapVerdictEngine::isMeaningfulOffset(8.00, 8.0),
              "1. exactly 8.00 mm clears the scale-aware bar");
        check(!WindMapVerdictEngine::isMeaningfulOffset(5.00, 8.0),
              "1. 5 mm clears the floor but NOT the scale-aware bar — both must pass");

        // Compact <= 1.25x reference diameter.
        check(WindMapVerdictEngine::isCompact(25.0, 20.0),
              "1. exactly 1.25x is still compact (<=)");
        check(!WindMapVerdictEngine::isCompact(25.01, 20.0),
              "1. just past 1.25x is not compact");
        // Wider >= 1.50x reference diameter.
        check(WindMapVerdictEngine::isWider(30.0, 20.0),
              "1. exactly 1.50x is wider (>=)");
        check(!WindMapVerdictEngine::isWider(29.99, 20.0),
              "1. just under 1.50x is not wider");
        // The deliberate gap produces NEITHER claim.
        check(!WindMapVerdictEngine::isCompact(28.0, 20.0)
              && !WindMapVerdictEngine::isWider(28.0, 20.0),
              "1. 1.4x sits in the gap — neither compact nor wider, so no claim");
        // A zero-width reference cannot anchor a ratio.
        check(!WindMapVerdictEngine::isCompact(10.0, 0.0)
              && !WindMapVerdictEngine::isWider(10.0, 0.0),
              "1. a zero-diameter reference yields no ratio claim");
    }

    // ── 2. VERDICT 1 — insufficient sample ──────────────────────────────
    {
        SessionState s = baseState(false);
        addLineGroup(s, 1, 0, 4, 0.0, 0.0, 6.0, calm());     // 4 < 5
        const QVector<Verdict> v = verdictsFor(s);
        check(!v.isEmpty(), "2. a verdict was produced");
        check(v[0].category == VerdictCategory::InsufficientSample,
              "2. insufficient sample is the PRIMARY verdict");
        check(v[0].evidence == EvidenceLevel::Insufficient, "2. evidence is Insufficient");
        check(v[0].headline.contains(QLatin1String("4")),
              "2. it states how many shots were recorded");
        check(v[0].nextTrainingStep.contains(QLatin1String("1 more")),
              "2. it states how many more are required", v[0].nextTrainingStep);
        // Nothing may be reported as zero.
        check(!v[0].observedPattern.contains(QLatin1String("0.0")),
              "2. no withheld metric is reported as 0.0");
    }

    // ── 3. VERDICT 2 — fragmented data ──────────────────────────────────
    {
        SessionState s = baseState(false);
        // 7 shots across 4 conditions: enough to describe nothing.
        s.wmShots << rec(1, false, 0, 1.0, 1.0, 10.0, calm())
                  << rec(2, false, 0, 1.5, 0.5, 10.0, calm())
                  << rec(3, false, 0, 3.0, 1.0, 9.8, measured(270, 2.5))
                  << rec(4, false, 0, 3.5, 1.2, 9.8, measured(270, 2.5))
                  << rec(5, false, 0, -2.0, 2.0, 9.5, measured(45, 5.0))
                  << rec(6, false, 0, -2.5, 1.5, 9.5, measured(45, 5.0))
                  << rec(7, false, 0, 0.5, -1.0, 9.9, measured(180, 1.0));
        const QVector<Verdict> v = verdictsFor(s);
        check(hasCategory(v, VerdictCategory::FragmentedData),
              "3. fragmented data is detected at 4 conditions with none reaching 5");
        const Verdict* f = firstOf(v, VerdictCategory::FragmentedData);
        check(f && f->headline.contains(QLatin1String("4 different conditions")),
              "3. it names how many conditions were used", f ? f->headline : QString());
        check(f && f->interpretation.contains(QLatin1String("too frequently")),
              "3. it explains the session changed conditions too often");
        check(f && f->nextTrainingStep.contains(QLatin1String("two")),
              "3. it proposes concentrating on two conditions");
    }

    // ── 4. VERDICT 3 — compact but offset ───────────────────────────────
    {
        SessionState s = baseState(false);
        // Reference: 10 calm shots, 20 mm wide, centred on origin.
        addLineGroup(s, 1, 0, 10, 0.0, 0.0, 20.0, calm());
        // Compared: 10 shots, 20 mm wide (1.0x -> compact), centred 8 mm right.
        // Reference mean radius for a uniform line of width 20 is 5.0, so the
        // bar is max(3.0, 5.0) = 5.0 and 8.0 clears it.
        addLineGroup(s, 11, 0, 10, 8.0, 0.0, 20.0, measured(270, 2.0));
        const QVector<Verdict> v = verdictsFor(s);
        check(hasCategory(v, VerdictCategory::CompactButOffset),
              "4. compact-but-offset is detected");
        const Verdict* o = firstOf(v, VerdictCategory::CompactButOffset);
        check(o && o->evidence == EvidenceLevel::Comparative,
              "4. evidence is Comparative — both sides met the threshold");
        check(o && o->sampleCountReference == 10 && o->sampleCountCompared == 10,
              "4. both sample counts travel with the verdict");
        check(o && o->headline.contains(QLatin1String("right")),
              "4. the offset is worded right/left, not as a signed number",
              o ? o->headline : QString());
        check(o && !o->coachDecision.isEmpty(),
              "4. it carries a coach-decision line");
        check(o && o->coachDecision.contains(QLatin1String("after the pattern is repeated")),
              "4. the coach line requires repetition first");
        check(o && o->nextTrainingStep.contains(QLatin1String("another session")),
              "4. the next step is to repeat in another session");
        check(o && o->interpretation.contains(QLatin1String("does not prove")),
              "4. it states plainly that causation is not proven");
    }

    // ── 5. VERDICT 4 — wider under a condition ──────────────────────────
    {
        SessionState s = baseState(false);
        addLineGroup(s, 1, 0, 10, 0.0, 0.0, 20.0, calm());          // reference 20 mm
        addLineGroup(s, 11, 0, 10, 0.0, 0.0, 32.0, measured(45, 5.0)); // 1.6x -> wider
        const QVector<Verdict> v = verdictsFor(s);
        check(hasCategory(v, VerdictCategory::WiderUnderCondition),
              "5. wider-under-condition is detected at 1.6x");
        const Verdict* w = firstOf(v, VerdictCategory::WiderUnderCondition);
        check(w && w->headline.contains(QLatin1String("32.0"))
              && w->headline.contains(QLatin1String("20.0")),
              "5. it states both group sizes", w ? w->headline : QString());
        check(w && w->interpretation.contains(QLatin1String("cannot determine")),
              "5. it says the software cannot determine the reason");
    }

    // ── 6. VERDICT 5 — similar across conditions ────────────────────────
    {
        SessionState s = baseState(false);
        addLineGroup(s, 1, 0, 10, 0.0, 0.0, 20.0, calm());
        // Same size, centre moved only 2 mm — below the 5.0 mm bar.
        addLineGroup(s, 11, 0, 10, 2.0, 0.0, 21.0, measured(270, 2.0));
        const QVector<Verdict> v = verdictsFor(s);
        check(hasCategory(v, VerdictCategory::SimilarAcrossConditions),
              "6. similar-across-conditions is detected");
        const Verdict* sim = firstOf(v, VerdictCategory::SimilarAcrossConditions);
        check(sim && sim->interpretation.contains(QLatin1String("not the same as showing there is none")),
              "6. it does NOT claim wind had no effect", sim ? sim->interpretation : QString());
    }

    // ── 7. VERDICT 6 — wide across all conditions ───────────────────────
    {
        SessionState s = baseState(false);
        addLineGroup(s, 1, 0, 10, 0.0, 0.0, 45.0, calm());              // >= 40
        addLineGroup(s, 11, 0, 10, 0.0, 0.0, 48.0, measured(270, 2.0)); // >= 40
        const QVector<Verdict> v = verdictsFor(s);
        check(hasCategory(v, VerdictCategory::WideAcrossConditions),
              "7. wide-across-conditions is detected when every group exceeds 40 mm");
        const Verdict* w = firstOf(v, VerdictCategory::WideAcrossConditions);
        check(w && w->interpretation.contains(QLatin1String("No single condition explains")),
              "7. it refuses to blame one condition");
        check(w && w->nextTrainingStep.contains(QLatin1String("Group Pattern Coach")),
              "7. it refers to Group Pattern Coach rather than diagnosing a fault");

        // Boundary: one group just under 40 mm means NOT all wide.
        // 39.8 rather than 39.99: coordinates are stored as HUNDREDTHS of a
        // millimetre, and a 39.99-wide line rounds its end shots to +/-20.00,
        // giving a stored spread of exactly 40.00 — the test data could not
        // express the value it was asserting on.
        SessionState s2 = baseState(false);
        addLineGroup(s2, 1, 0, 10, 0.0, 0.0, 39.8, calm());
        addLineGroup(s2, 11, 0, 10, 0.0, 0.0, 48.0, measured(270, 2.0));
        check(!hasCategory(verdictsFor(s2), VerdictCategory::WideAcrossConditions),
              "7. 39.8 mm keeps the session out of wide-across-conditions");
    }

    // ── 8. VERDICT 7 — position difference is SESSION scope ─────────────
    {
        SessionState s = baseState(true);
        addLineGroup(s, 1,  0, 8, 0.0, 0.0, 40.0, calm());   // Kneeling, widest
        for (int i = 0; i < 8; ++i) s.wmShots[i].position = 1;
        addLineGroup(s, 9,  2, 8, 0.0, 0.0, 15.0, calm());   // Prone
        addLineGroup(s, 17, 3, 8, 0.0, 0.0, 25.0, calm());   // Standing
        const QVector<Verdict> v = verdictsFor(s);
        const Verdict* pv = firstOf(v, VerdictCategory::PositionSpecificDifference);
        check(pv != nullptr, "8. the position comparison is produced for 3P");
        check(pv && pv->scope == VerdictScope::Session,
              "8. UI-WIND-006: it is SESSION-scoped, never a position result");
        check(pv && pv->position == 0,
              "8. it claims no position id, so it cannot be filtered into one");
        check(pv && pv->interpretation.contains(QLatin1String("cannot be attributed to wind alone")),
              "8. it refuses to attribute the difference to wind");
        check(pv && pv->nextTrainingStep.contains(QLatin1String("another")),
              "8. it proposes comparing the position with ITSELF across sessions");
        // Every other verdict must name its position.
        bool positionsNamed = true;
        for (const Verdict& x : v)
            if (x.scope == VerdictScope::Position && x.positionName.isEmpty())
                positionsNamed = false;
        check(positionsNamed, "8. every position-scoped verdict names its position");
    }

    // ── 9. no verdict is FORCED when no rule fits ───────────────────────
    {
        SessionState s = baseState(false);
        // Enough shots to describe one group, but only ONE condition, so
        // there is nothing to compare against.
        addLineGroup(s, 1, 0, 8, 0.0, 0.0, 20.0, calm());
        const QVector<Verdict> v = verdictsFor(s);
        check(!hasCategory(v, VerdictCategory::CompactButOffset)
              && !hasCategory(v, VerdictCategory::WiderUnderCondition)
              && !hasCategory(v, VerdictCategory::SimilarAcrossConditions),
              "9. no comparison verdict is invented from a single condition");
        // The engine must still say SOMETHING useful — this is the INDICATIVE
        // case, and the first run of these tests found it produced nothing.
        check(!v.isEmpty(), "9. a verdict is still produced");
        const Verdict* nv = firstOf(v, VerdictCategory::NoValidComparison);
        check(nv != nullptr, "9. it is NoValidComparison, not a forced category");
        check(nv && nv->evidence == EvidenceLevel::Indicative,
              "9. evidence is Indicative — the group is described, not compared");
        check(nv && nv->headline.contains(QLatin1String("20.0")),
              "9. the group is still described with its size",
              nv ? nv->headline : QString());
        check(nv && nv->interpretation.contains(QLatin1String("nothing to compare")),
              "9. it says plainly there is nothing to compare against");
        check(nv && nv->nextTrainingStep.contains(QLatin1String("second")),
              "9. it asks for a second condition");
    }

    // ── 10. REPEATED is reserved and unassignable ───────────────────────
    {
        // Every session shape above, plus these, must never emit Repeated.
        SessionState s = baseState(false);
        addLineGroup(s, 1, 0, 20, 0.0, 0.0, 20.0, calm());
        addLineGroup(s, 21, 0, 20, 9.0, 0.0, 20.0, measured(270, 2.0));
        const QVector<Verdict> v = verdictsFor(s);
        bool anyRepeated = false;
        for (const Verdict& x : v) {
            if (x.evidence == EvidenceLevel::Repeated) anyRepeated = true;
            if (x.category == VerdictCategory::RepeatedPattern) anyRepeated = true;
            if (x.scope == VerdictScope::CrossSession) anyRepeated = true;
        }
        check(!anyRepeated,
              "10. REPEATED is never assigned from a single session, even a large one");
        check(evidenceLevelExplanation(EvidenceLevel::Repeated)
                  .contains(QLatin1String("Not available yet")),
              "10. the Repeated label explains that it is not available yet");
    }

    // ── 11. prohibited wording, across every session shape ──────────────
    {
        QStringList text;
        auto harvest = [&text](const QVector<Verdict>& vs) {
            for (const Verdict& v : vs) {
                text << v.headline << v.observedPattern << v.interpretation
                     << v.nextTrainingStep << v.coachDecision << v.limitations;
            }
        };
        {   SessionState s = baseState(false);
            addLineGroup(s, 1, 0, 4, 0.0, 0.0, 6.0, calm());
            harvest(verdictsFor(s)); }
        {   SessionState s = baseState(false);
            addLineGroup(s, 1, 0, 10, 0.0, 0.0, 20.0, calm());
            addLineGroup(s, 11, 0, 10, 8.0, 0.0, 20.0, measured(270, 2.0));
            harvest(verdictsFor(s)); }
        {   SessionState s = baseState(false);
            addLineGroup(s, 1, 0, 10, 0.0, 0.0, 20.0, calm());
            addLineGroup(s, 11, 0, 10, 0.0, 0.0, 32.0, measured(45, 5.0));
            harvest(verdictsFor(s)); }
        {   SessionState s = baseState(false);
            addLineGroup(s, 1, 0, 10, 0.0, 0.0, 45.0, calm());
            addLineGroup(s, 11, 0, 10, 0.0, 0.0, 48.0, measured(270, 2.0));
            harvest(verdictsFor(s)); }
        {   SessionState s = baseState(true);
            addLineGroup(s, 1, 1, 8, 0.0, 0.0, 40.0, calm());
            addLineGroup(s, 9, 2, 8, 0.0, 0.0, 15.0, calm());
            harvest(verdictsFor(s)); }

        const QString all = text.join(QLatin1Char('\n'));
        check(!all.isEmpty(), "11. the engine produced text to scan");

        // Causal claims.
        const char* causal[] = {
            "caused", "because of", "due to", "the wind pushed", "wind caused",
            "proves", "proven that", "demonstrates that",
        };
        bool causalClean = true; QString causalFound;
        for (const char* c : causal)
            if (all.contains(QLatin1String(c), Qt::CaseInsensitive)) {
                // "does not prove" / "cannot be attributed" are the permitted
                // NEGATIONS; check the bare claim is not made.
                if (QLatin1String(c) == QLatin1String("proves")
                    && all.contains(QLatin1String("does not prove"))) continue;
                causalClean = false; causalFound += QLatin1String(c) + QStringLiteral(" ");
            }
        check(causalClean, "11. no causal claim is made", causalFound);

        // Prescriptions.
        const char* prescriptive[] = {
            "sight click", "clicks left", "clicks right", "clicks up", "clicks down",
            "aim off", "hold left", "hold right", "hold into", "aim at",
            "move your sight", "adjust your sight", "come up", "come down",
        };
        bool presClean = true; QString presFound;
        for (const char* c : prescriptive)
            if (all.contains(QLatin1String(c), Qt::CaseInsensitive)) {
                presClean = false; presFound += QLatin1String(c) + QStringLiteral(" ");
            }
        check(presClean, "11. no sight, aim or hold prescription", presFound);

        // Unearned statistical certainty.
        const char* stats[] = {
            "statistically significant", "confidence interval", "p-value", "p <",
            "95%", "certainty",
        };
        bool statsClean = true; QString statsFound;
        for (const char* c : stats)
            if (all.contains(QLatin1String(c), Qt::CaseInsensitive)) {
                statsClean = false; statsFound += QLatin1String(c) + QStringLiteral(" ");
            }
        check(statsClean, "11. no statistical certainty is claimed that was never computed",
              statsFound);

        check(all.contains(QLatin1String("may not represent wind across the complete bullet path")),
              "11. the observation limitation is always attached");
        check(all.contains(QLatin1String("never an official competition result")),
              "11. the training-only limitation is always attached");
    }

    // ── 12. relative wind direction ─────────────────────────────────────
    {
        // Firing due north (0). Wind FROM north is straight into the face.
        check(relativeWindFor(0, 0) == RelativeWind::Headwind,
              "12. wind from the firing direction is a headwind");
        check(relativeWindFor(180, 0) == RelativeWind::Tailwind,
              "12. wind from directly behind is a tailwind");
        check(relativeWindFor(270, 0) == RelativeWind::LeftToRightCrosswind,
              "12. firing north, wind from the west blows left to right");
        check(relativeWindFor(90, 0) == RelativeWind::RightToLeftCrosswind,
              "12. firing north, wind from the east blows right to left");
        // Rotating the range must rotate the relative answer identically.
        check(relativeWindFor(90, 90) == RelativeWind::Headwind,
              "12. firing east, wind from the east is a headwind");
        check(relativeWindFor(0, 90) == RelativeWind::LeftToRightCrosswind,
              "12. firing east, wind from the north blows left to right");
        // Absent metadata is absent, not guessed.
        check(relativeWindFor(270, -1) == RelativeWind::Unavailable,
              "12. no firing direction yields Unavailable, never a guess");
        check(relativeWindLabel(RelativeWind::Unavailable)
                  .contains(QLatin1String("firing direction was not recorded")),
              "12. the fallback explains WHY it is unavailable");
    }

    // ── 13. old sessions remain fully readable ──────────────────────────
    {
        // A pre-Stage-6.1.2 session: no firing direction, no verdict fields.
        SessionState s = baseState(false);
        s.wmPhase = 0;                       // as a v4/v5 snapshot would restore
        addLineGroup(s, 1, 0, 10, 0.0, 0.0, 20.0, calm());
        addLineGroup(s, 11, 0, 10, 8.0, 0.0, 20.0, measured(270, 2.0));
        const QVector<Verdict> v = verdictsFor(s);
        check(!v.isEmpty(), "13. a session recorded before this phase still yields verdicts");
        // The recorded compass value is untouched by the derivation.
        check(s.wmShots[15].windDirectionDegrees == 270,
              "13. the recorded compass direction is never mutated");
        check(relativeWindFor(s.wmShots[15].windDirectionDegrees, -1) == RelativeWind::Unavailable,
              "13. and no relative direction is invented for it");
    }

    // ── 14. priority order ──────────────────────────────────────────────
    {
        SessionState s = baseState(false);
        // A data-quality problem plus a comparable pair: the warning wins.
        addLineGroup(s, 1, 0, 10, 0.0, 0.0, 20.0, calm());
        addLineGroup(s, 11, 0, 10, 8.0, 0.0, 20.0, measured(270, 2.0));
        addLineGroup(s, 21, 0, 2, 0.0, 0.0, 2.0, measured(45, 5.0));   // too few
        const QVector<Verdict> v = verdictsFor(s);
        check(v.size() >= 2, "14. several verdicts were produced");
        check(v[0].category == VerdictCategory::InsufficientSample,
              "14. the data-quality warning outranks the valid comparison",
              verdictCategoryLabel(v[0].category));
        bool sortedByPriority = true;
        for (int i = 1; i < v.size(); ++i)
            if (v[i].priority < v[i - 1].priority) sortedByPriority = false;
        check(sortedByPriority, "14. verdicts are returned in priority order");
    }

    // ── 15. the analytics formulas are untouched ────────────────────────
    {
        check(kMinSamplesMpi == 3 && kMinSamplesDispersion == 5
              && kMinSamplesComparison == 5,
              "15. the approved sample thresholds are unchanged by this phase");
        SessionState s = baseState(false);
        const WindConditionSnapshot w = calm();
        s.wmShots << rec(1, false, 0, 0.0,  0.0, 10.0, w)
                  << rec(2, false, 0, 3.0,  0.0, 10.0, w)
                  << rec(3, false, 0, -3.0, 0.0, 10.0, w)
                  << rec(4, false, 0, 0.0,  4.0, 10.0, w)
                  << rec(5, false, 0, 0.0, -4.0, 10.0, w);
        const SessionAnalysis a = WindMapAnalyticsEngine::analyse(s);
        const GroupStats& g = a.positions[0].byExactCondition[0];
        check(std::fabs(g.meanRadiusMm - 2.8) < 1e-9
              && std::fabs(g.groupDiameterMm - 8.0) < 1e-9,
              "15. the hand-checked dispersion answers are unchanged");
    }
}
