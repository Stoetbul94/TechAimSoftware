#include "WindMapVerdict.h"

#include <QtGlobal>
#include <algorithm>
#include <cmath>

namespace ta {
namespace training {

// ── labels ─────────────────────────────────────────────────────────────────

QString evidenceLevelLabel(EvidenceLevel e)
{
    switch (e) {
    case EvidenceLevel::Insufficient: return QStringLiteral("Insufficient");
    case EvidenceLevel::Indicative:   return QStringLiteral("Indicative");
    case EvidenceLevel::Comparative:  return QStringLiteral("Comparative");
    case EvidenceLevel::Repeated:     return QStringLiteral("Repeated");
    }
    return QString();
}

QString evidenceLevelExplanation(EvidenceLevel e)
{
    switch (e) {
    case EvidenceLevel::Insufficient:
        return QStringLiteral("Not enough shots were recorded to describe this group. "
                              "No comparison is offered.");
    case EvidenceLevel::Indicative:
        return QStringLiteral("One condition has enough shots to describe its group, but no "
                              "second condition has enough to compare it against.");
    case EvidenceLevel::Comparative:
        return QStringLiteral("Both conditions have enough shots for a descriptive comparison. "
                              "This describes what was observed together — it does not prove a cause.");
    case EvidenceLevel::Repeated:
        return QStringLiteral("The same pattern was observed across separate sessions. "
                              "Not available yet — Wind Map compares one session at a time.");
    }
    return QString();
}

QString verdictCategoryLabel(VerdictCategory c)
{
    switch (c) {
    case VerdictCategory::InsufficientSample:         return QStringLiteral("Insufficient sample");
    case VerdictCategory::FragmentedData:             return QStringLiteral("Conditions changed too often");
    case VerdictCategory::CompactButOffset:           return QStringLiteral("Compact but offset");
    case VerdictCategory::WiderUnderCondition:        return QStringLiteral("Wider under a condition");
    case VerdictCategory::SimilarAcrossConditions:    return QStringLiteral("Similar across conditions");
    case VerdictCategory::WideAcrossConditions:       return QStringLiteral("Wide across all conditions");
    case VerdictCategory::PositionSpecificDifference: return QStringLiteral("Position difference");
    case VerdictCategory::NoValidComparison:          return QStringLiteral("No valid comparison");
    case VerdictCategory::RepeatedPattern:            return QStringLiteral("Repeated pattern");
    }
    return QString();
}

QString verdictScopeLabel(VerdictScope s)
{
    switch (s) {
    case VerdictScope::Session:      return QStringLiteral("Session");
    case VerdictScope::Position:     return QStringLiteral("Position");
    case VerdictScope::Condition:    return QStringLiteral("Condition");
    case VerdictScope::CrossSession: return QStringLiteral("Across sessions");
    }
    return QString();
}

// ── threshold predicates ───────────────────────────────────────────────────

bool WindMapVerdictEngine::isMeaningfulOffset(double magnitudeMm, double referenceMeanRadiusMm)
{
    // BOTH bars. See the rules document §3.2.
    const double bar = qMax(kOffsetMinimumMm,
                            kOffsetRelativeToMeanRadius * referenceMeanRadiusMm);
    return magnitudeMm >= bar;
}

bool WindMapVerdictEngine::isCompact(double comparedDiameterMm, double referenceDiameterMm)
{
    if (referenceDiameterMm <= 0.0) return false;
    return comparedDiameterMm <= kCompactRelativeToReference * referenceDiameterMm;
}

bool WindMapVerdictEngine::isWider(double comparedDiameterMm, double referenceDiameterMm)
{
    if (referenceDiameterMm <= 0.0) return false;
    return comparedDiameterMm >= kWiderRelativeToReference * referenceDiameterMm;
}

// ── relative wind direction ────────────────────────────────────────────────

QString relativeWindLabel(RelativeWind r)
{
    switch (r) {
    case RelativeWind::Unavailable:
        return QStringLiteral("Relative wind direction unavailable — "
                              "firing direction was not recorded");
    case RelativeWind::Headwind:             return QStringLiteral("Headwind");
    case RelativeWind::HeadwindFromLeft:     return QStringLiteral("Headwind from the left");
    case RelativeWind::LeftToRightCrosswind: return QStringLiteral("Left-to-right crosswind");
    case RelativeWind::TailwindFromLeft:     return QStringLiteral("Tailwind from the left");
    case RelativeWind::Tailwind:             return QStringLiteral("Tailwind");
    case RelativeWind::TailwindFromRight:    return QStringLiteral("Tailwind from the right");
    case RelativeWind::RightToLeftCrosswind: return QStringLiteral("Right-to-left crosswind");
    case RelativeWind::HeadwindFromRight:    return QStringLiteral("Headwind from the right");
    }
    return QString();
}

RelativeWind relativeWindFor(qint16 windFromDegrees, int firingDegrees)
{
    // Optional metadata: absent means absent. The recorded compass value is
    // never mutated, and an old session stays fully readable without this.
    if (firingDegrees < 0) return RelativeWind::Unavailable;

    // Angle of the wind's ORIGIN relative to where the athlete is pointing.
    // 0 = straight into the face (headwind); 180 = from behind.
    int rel = (static_cast<int>(windFromDegrees) - firingDegrees) % 360;
    if (rel < 0) rel += 360;

    // Eight 45-degree sectors centred on the cardinal relative directions, the
    // same +22 half-sector shift the compass grouping uses.
    const int idx = ((rel + 22) % 360) / 45;
    switch (idx) {
    case 0: return RelativeWind::Headwind;               // from ahead
    case 1: return RelativeWind::HeadwindFromRight;      // ahead-right
    case 2: return RelativeWind::RightToLeftCrosswind;   // from the right
    case 3: return RelativeWind::TailwindFromRight;      // behind-right
    case 4: return RelativeWind::Tailwind;               // from behind
    case 5: return RelativeWind::TailwindFromLeft;       // behind-left
    case 6: return RelativeWind::LeftToRightCrosswind;   // from the left
    case 7: return RelativeWind::HeadwindFromLeft;       // ahead-left
    }
    return RelativeWind::Unavailable;
}

// ── the engine ─────────────────────────────────────────────────────────────

namespace {

const QStringList kAlwaysLimitations = {
    QStringLiteral("The recorded condition is an athlete observation and may not represent "
                   "wind across the complete bullet path."),
    QStringLiteral("This describes what was observed together. It does not establish a cause."),
    QStringLiteral("Training material only — never an official competition result."),
};

// Finds a group by its condition label.
const GroupStats* groupFor(const PositionAnalysis& p, const QString& label)
{
    for (const GroupStats& g : p.byExactCondition)
        if (g.label == label) return &g;
    return nullptr;
}

QString mm(double v) { return QString::number(v, 'f', 1); }

// "6.2 mm right and 1.4 mm low" — the athlete never reads a signed number.
QString offsetWords(double dxMm, double dyMm)
{
    const QString h = dxMm >= 0 ? QStringLiteral("right") : QStringLiteral("left");
    const QString v = dyMm >= 0 ? QStringLiteral("high")  : QStringLiteral("low");
    return QStringLiteral("%1 mm %2 and %3 mm %4")
        .arg(mm(std::fabs(dxMm)), h, mm(std::fabs(dyMm)), v);
}

} // namespace

QVector<Verdict> WindMapVerdictEngine::evaluate(const SessionAnalysis& a)
{
    QVector<Verdict> out;
    if (!a.valid) return out;

    for (const PositionAnalysis& p : a.positions) {
        const QString where = a.threePositions
            ? QStringLiteral("%1: ").arg(p.positionName) : QString();
        const bool isRef = p.reference.valid;
        const GroupStats* refGroup = isRef ? groupFor(p, p.reference.label) : nullptr;

        // How many condition groups can be described / compared at all?
        QVector<const GroupStats*> describable, comparable;
        for (const GroupStats& g : p.byExactCondition) {
            if (g.hasDispersion) describable.append(&g);
            if (g.n >= kMinSamplesComparison && g.hasMpi) comparable.append(&g);
        }

        auto base = [&](Verdict& v, VerdictCategory c, VerdictScope s, int prio) {
            v.category = c;
            v.scope = s;
            v.position = p.position;
            v.positionName = p.positionName;
            v.priority = prio;
            v.limitations = kAlwaysLimitations;
            if (isRef) {
                v.referenceCondition = p.reference.label;
                v.sampleCountReference = p.reference.n;
            }
        };

        // ── VERDICT 1 — INSUFFICIENT SAMPLE ─────────────────────────────
        if (p.countedShots < kMinSamplesDispersion) {
            Verdict v;
            base(v, VerdictCategory::InsufficientSample, VerdictScope::Position, 1);
            v.verdictId = QStringLiteral("pos%1.insufficient").arg(p.position);
            v.evidence = EvidenceLevel::Insufficient;
            const int need = kMinSamplesDispersion - p.countedShots;
            v.headline = QStringLiteral("%1Only %2 counted %3 recorded. No reliable condition "
                                        "comparison can be made.")
                             .arg(where).arg(p.countedShots)
                             .arg(p.countedShots == 1 ? QStringLiteral("shot")
                                                      : QStringLiteral("shots"));
            v.observedPattern = QStringLiteral("%1 of the %2 shots needed to describe a group.")
                                    .arg(p.countedShots).arg(kMinSamplesDispersion);
            v.interpretation = QStringLiteral("There is not enough recorded here to describe a "
                                              "group, let alone compare two.");
            v.nextTrainingStep =
                QStringLiteral("Record at least %1 more counted %2 under one repeatable "
                               "condition, and at least %3 under a second.")
                    .arg(need).arg(need == 1 ? QStringLiteral("shot") : QStringLiteral("shots"))
                    .arg(kMinSamplesComparison);
            v.supportingMetricIds << QStringLiteral("countedShots");
            out.append(v);
            continue;                       // nothing else can be said
        }

        // ── VERDICT 2 — FRAGMENTED DATA ─────────────────────────────────
        if (p.byExactCondition.size() >= kFragmentedMinConditions && comparable.isEmpty()) {
            Verdict v;
            base(v, VerdictCategory::FragmentedData, VerdictScope::Position, 2);
            v.verdictId = QStringLiteral("pos%1.fragmented").arg(p.position);
            v.evidence = EvidenceLevel::Insufficient;
            v.headline = QStringLiteral("%1%2 shots were spread across %3 different conditions. "
                                        "No condition contains enough shots for a reliable "
                                        "comparison.")
                             .arg(where).arg(p.countedShots).arg(p.byExactCondition.size());
            v.observedPattern = QStringLiteral("No condition reached the %1 counted shots a "
                                               "comparison needs.").arg(kMinSamplesComparison);
            v.interpretation = QStringLiteral("The session changed conditions too frequently to "
                                              "distinguish a repeatable pattern.");
            v.nextTrainingStep = QStringLiteral("Choose two clearly repeatable conditions and "
                                                "record at least %1 to 10 shots under each.")
                                     .arg(kMinSamplesComparison);
            v.supportingMetricIds << QStringLiteral("uniqueConditions") << QStringLiteral("n");
            out.append(v);
            continue;
        }

        // Everything below needs a valid reference to compare against.
        if (!isRef || !refGroup || !refGroup->hasDispersion) {
            Verdict v;
            base(v, VerdictCategory::NoValidComparison, VerdictScope::Position, 8);
            v.verdictId = QStringLiteral("pos%1.novalid").arg(p.position);
            v.evidence = describable.isEmpty() ? EvidenceLevel::Insufficient
                                               : EvidenceLevel::Indicative;
            v.headline = QStringLiteral("%1No reference group with enough shots was available, "
                                        "so no comparison was made.").arg(where);
            v.observedPattern = QStringLiteral("%1 condition %2 could be described.")
                                    .arg(describable.size())
                                    .arg(describable.size() == 1 ? QStringLiteral("group")
                                                                 : QStringLiteral("groups"));
            v.interpretation = QStringLiteral("The measurements below stand on their own; "
                                              "nothing is being compared.");
            v.nextTrainingStep = QStringLiteral("Record at least %1 counted shots under one "
                                                "condition and %1 under a second.")
                                     .arg(kMinSamplesComparison);
            out.append(v);
            continue;
        }

        // ── VERDICT 6 — WIDE ACROSS ALL CONDITIONS ──────────────────────
        // Checked before the per-condition verdicts: when every group is wide,
        // no single condition explains it, and saying so first is more useful.
        if (describable.size() >= 2) {
            bool allWide = true;
            for (const GroupStats* g : describable)
                if (g->groupDiameterMm < kWideProvisionalMm) allWide = false;
            if (allWide) {
                Verdict v;
                base(v, VerdictCategory::WideAcrossConditions, VerdictScope::Position, 5);
                v.verdictId = QStringLiteral("pos%1.wideall").arg(p.position);
                v.evidence = EvidenceLevel::Comparative;
                v.sampleCountCompared = p.countedShots;
                // Deliberately RELATIVE, descriptive wording. The threshold
                // behind it is provisional and unreviewed, so the verdict
                // reports the measurement and never calls the shooting poor.
                v.headline = QStringLiteral("%1Groups remained relatively wide across the "
                                            "recorded conditions.").arg(where);
                v.observedPattern = QStringLiteral("Every described group measured at least "
                                                   "%1 mm across.").arg(mm(kWideProvisionalMm));
                v.interpretation = QStringLiteral("No single recorded condition separates these "
                                                  "groups from one another.");
                v.nextTrainingStep = QStringLiteral("Review position stability, aiming and "
                                                    "triggering with Group Pattern Coach or an "
                                                    "aim-trace tool.");
                v.supportingMetricIds << QStringLiteral("groupDiameterMm");
                out.append(v);
            }
        }

        // ── INDICATIVE: one describable condition, nothing to compare ───
        // Found by the boundary tests: a session with exactly one well-sampled
        // condition previously produced NO verdict at all, because every
        // comparison rule needs a second group. That is the Indicative case
        // the evidence vocabulary defines — describe the group, claim no
        // condition-associated difference.
        if (comparable.size() == 1 && p.shifts.isEmpty()) {
            Verdict v;
            base(v, VerdictCategory::NoValidComparison, VerdictScope::Condition, 8);
            v.verdictId = QStringLiteral("pos%1.single").arg(p.position);
            v.comparedCondition = refGroup->label;
            v.sampleCountCompared = refGroup->n;
            v.evidence = EvidenceLevel::Indicative;
            v.headline = QStringLiteral("%1%2 counted shots were recorded, all under %3, forming "
                                        "a group %4 mm across.")
                             .arg(where).arg(refGroup->n).arg(refGroup->label)
                             .arg(mm(refGroup->groupDiameterMm));
            v.observedPattern = QStringLiteral("Group centre %1; average distance from that "
                                               "centre %2 mm.")
                                    .arg(offsetWords(refGroup->mpiXMm, refGroup->mpiYMm))
                                    .arg(mm(refGroup->meanRadiusMm));
            v.interpretation = QStringLiteral("Only one condition was recorded, so there is "
                                              "nothing to compare it against. This describes the "
                                              "group, not an effect of the condition.");
            v.nextTrainingStep = QStringLiteral("Record at least %1 counted shots under a second, "
                                                "clearly different condition so the two can be "
                                                "compared.").arg(kMinSamplesComparison);
            v.supportingMetricIds << QStringLiteral("groupDiameterMm")
                                  << QStringLiteral("meanRadiusMm")
                                  << QStringLiteral("mpiXMm");
            out.append(v);
        }

        // ── per-condition verdicts, against the reference ───────────────
        bool anyMeaningful = false;
        bool anySizeDifference = false;
        for (const ShiftVector& sv : p.shifts) {
            const GroupStats* g = groupFor(p, sv.label);
            if (!g) continue;

            // Below the comparison threshold: say what is missing, not zero.
            if (!sv.valid) {
                if (sv.shotsNeeded <= 0) continue;
                Verdict v;
                base(v, VerdictCategory::InsufficientSample, VerdictScope::Condition, 1);
                v.verdictId = QStringLiteral("pos%1.%2.insufficient").arg(p.position).arg(sv.label);
                v.comparedCondition = sv.label;
                v.sampleCountCompared = sv.n;
                v.evidence = EvidenceLevel::Insufficient;
                v.headline = QStringLiteral("%1Only %2 counted %3 recorded under %4. No reliable "
                                            "comparison can be made.")
                                 .arg(where).arg(sv.n)
                                 .arg(sv.n == 1 ? QStringLiteral("shot") : QStringLiteral("shots"))
                                 .arg(sv.label);
                v.observedPattern = QStringLiteral("%1 more %2 required for a group comparison.")
                                        .arg(sv.shotsNeeded)
                                        .arg(sv.shotsNeeded == 1 ? QStringLiteral("shot is")
                                                                 : QStringLiteral("shots are"));
                v.interpretation = QStringLiteral("Too few shots to describe how this condition "
                                                  "compares.");
                v.nextTrainingStep = QStringLiteral("Repeat %1 and record at least %2 more "
                                                    "counted shots.").arg(sv.label)
                                         .arg(sv.shotsNeeded);
                v.supportingMetricIds << QStringLiteral("n");
                out.append(v);
                continue;
            }

            const bool meaningful = isMeaningfulOffset(sv.magnitudeMm, refGroup->meanRadiusMm);
            const bool compact = g->hasDispersion
                              && isCompact(g->groupDiameterMm, refGroup->groupDiameterMm);
            const bool wider = g->hasDispersion
                            && isWider(g->groupDiameterMm, refGroup->groupDiameterMm);
            if (meaningful) anyMeaningful = true;
            if (wider || !compact) anySizeDifference = true;

            // ── VERDICT 3 — COMPACT BUT OFFSET ──────────────────────────
            if (meaningful && compact) {
                Verdict v;
                base(v, VerdictCategory::CompactButOffset, VerdictScope::Condition, 3);
                v.verdictId = QStringLiteral("pos%1.%2.offset").arg(p.position).arg(sv.label);
                v.comparedCondition = sv.label;
                v.sampleCountCompared = sv.n;
                v.evidence = EvidenceLevel::Comparative;
                v.headline = QStringLiteral("%1%2 shots under %3 formed a compact group whose "
                                            "centre was %4 of the reference group.")
                                 .arg(where).arg(sv.n).arg(sv.label)
                                 .arg(offsetWords(sv.dxMm, sv.dyMm));
                v.observedPattern = QStringLiteral("Group %1 mm across against %2 mm for %3; "
                                                   "centre displaced %4 mm.")
                                        .arg(mm(g->groupDiameterMm))
                                        .arg(mm(refGroup->groupDiameterMm))
                                        .arg(p.reference.label)
                                        .arg(mm(sv.magnitudeMm));
                v.interpretation = QStringLiteral("A group-centre difference was observed "
                                                  "alongside this recorded condition. This does "
                                                  "not prove wind was the only reason.");
                v.nextTrainingStep = QStringLiteral("Repeat the same two conditions in another "
                                                    "session before changing sight or hold "
                                                    "strategy.");
                v.coachDecision = QStringLiteral("Discuss sight or wind strategy with a coach "
                                                 "only after the pattern is repeated.");
                v.supportingMetricIds << QStringLiteral("magnitudeMm")
                                      << QStringLiteral("groupDiameterMm")
                                      << QStringLiteral("meanRadiusMm");
                out.append(v);
                continue;
            }

            // ── VERDICT 4 — WIDER UNDER CONDITION ───────────────────────
            if (wider) {
                Verdict v;
                base(v, VerdictCategory::WiderUnderCondition, VerdictScope::Condition, 4);
                v.verdictId = QStringLiteral("pos%1.%2.wider").arg(p.position).arg(sv.label);
                v.comparedCondition = sv.label;
                v.sampleCountCompared = sv.n;
                v.evidence = EvidenceLevel::Comparative;
                v.headline = QStringLiteral("%1The %2 group measured %3 mm compared with %4 mm "
                                            "for %5.")
                                 .arg(where).arg(sv.label).arg(mm(g->groupDiameterMm))
                                 .arg(mm(refGroup->groupDiameterMm)).arg(p.reference.label);
                v.observedPattern = QStringLiteral("%1 counted shots; horizontal spread %2 mm "
                                                   "against %3 mm.")
                                        .arg(sv.n).arg(mm(g->horizontalSpreadMm))
                                        .arg(mm(refGroup->horizontalSpreadMm));
                v.interpretation = QStringLiteral("Shot placement was less consistent while this "
                                                  "condition was recorded. The software cannot "
                                                  "determine whether the reason was changing "
                                                  "wind, timing, hold or another execution "
                                                  "factor.");
                v.nextTrainingStep = QStringLiteral("Wait for a repeatable flag picture and "
                                                    "record ten shots without changing the "
                                                    "position setup.");
                v.supportingMetricIds << QStringLiteral("groupDiameterMm")
                                      << QStringLiteral("horizontalSpreadMm");
                out.append(v);
                continue;
            }
        }

        // ── VERDICT 5 — SIMILAR ACROSS CONDITIONS ───────────────────────
        if (comparable.size() >= 2 && !anyMeaningful && !anySizeDifference) {
            Verdict v;
            base(v, VerdictCategory::SimilarAcrossConditions, VerdictScope::Position, 6);
            v.verdictId = QStringLiteral("pos%1.similar").arg(p.position);
            v.evidence = EvidenceLevel::Comparative;
            v.sampleCountCompared = p.countedShots;
            v.headline = QStringLiteral("%1Group centres and group sizes were similar across the "
                                        "recorded conditions.").arg(where);
            v.observedPattern = QStringLiteral("No compared centre moved more than the reference "
                                               "group's own average scatter.");
            v.interpretation = QStringLiteral("This session did not establish a clear "
                                              "condition-associated pattern. That is not the "
                                              "same as showing there is none.");
            v.nextTrainingStep = QStringLiteral("Continue recording wind conditions, and use "
                                                "Group Pattern Coach to examine position and "
                                                "execution consistency.");
            v.supportingMetricIds << QStringLiteral("magnitudeMm")
                                  << QStringLiteral("groupDiameterMm");
            out.append(v);
        }
    }

    // ── VERDICT 7 — POSITION-SPECIFIC DIFFERENCE (3P, SESSION scope) ────
    // Deliberately Session-scoped: it compares positions AGAINST EACH OTHER,
    // so it must never be shown as one position's own result (UI-WIND-006).
    if (a.threePositions && a.positions.size() >= 2) {
        const PositionAnalysis* widest = nullptr;
        double widestVal = -1.0;
        for (const PositionAnalysis& p : a.positions)
            for (const GroupStats& g : p.byExactCondition) {
                if (!g.hasDispersion) continue;
                if (g.groupDiameterMm > widestVal) { widestVal = g.groupDiameterMm; widest = &p; }
            }
        if (widest && widestVal > 0.0) {
            Verdict v;
            v.category = VerdictCategory::PositionSpecificDifference;
            v.scope = VerdictScope::Session;     // never a position result
            v.priority = 7;
            v.verdictId = QStringLiteral("session.position");
            v.evidence = EvidenceLevel::Indicative;
            v.sampleCountCompared = widest->countedShots;
            v.positionName = widest->positionName;
            v.headline = QStringLiteral("%1 produced the widest recorded group in this session "
                                        "(%2 mm).").arg(widest->positionName).arg(mm(widestVal));
            v.observedPattern = QStringLiteral("Compared across %1 positions recorded in this "
                                               "session.").arg(a.positions.size());
            v.interpretation = QStringLiteral("Each position has different stability demands and "
                                              "was recorded under different conditions. The "
                                              "difference cannot be attributed to wind alone.");
            v.nextTrainingStep = QStringLiteral("Repeat a %1 Wind Map session under stable "
                                                "conditions and compare it with another %1 "
                                                "session.").arg(widest->positionName);
            v.limitations = kAlwaysLimitations;
            v.limitations << QStringLiteral("Positions are compared against each other here. "
                                            "Compare a position with itself across sessions for "
                                            "a stronger reading.");
            v.supportingMetricIds << QStringLiteral("groupDiameterMm");
            out.append(v);
        }
    }

    // Stable priority order: the view shows [0] as primary.
    std::stable_sort(out.begin(), out.end(), [](const Verdict& x, const Verdict& y) {
        return x.priority < y.priority;
    });
    return out;
}

} // namespace training
} // namespace ta
