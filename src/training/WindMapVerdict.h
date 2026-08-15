#ifndef TA_TRAINING_WINDMAPVERDICT_H
#define TA_TRAINING_WINDMAPVERDICT_H

// Wind Map — verdict engine (Training Lab Release 2, Stage 6.1.2).
//
// Turns the accepted mathematical analysis into structured athlete feedback.
// PURE QtCore, like the analytics engine it consumes. It introduces NO new
// mathematics: every threshold below is expressed in values WindMapAnalytics
// already produces, so the accepted formulas are untouched.
//
// Rules and their justification: docs/training-lab-wind-map-verdict-rules.md
// Evidence base and its limits:  docs/research/wind-map-feedback-evidence.md
//
// EVERY verdict answers five questions:
//   1. What happened?              -> headline / observedPattern
//   2. How much evidence?          -> evidence + sample counts
//   3. What may it mean?           -> interpretation
//   4. What should I test next?    -> nextTrainingStep
//   5. What needs a coach?         -> coachDecision
//
// WHAT IT MAY NOT DO. It may not diagnose the cause of a shot, state that
// wind definitely caused anything, prescribe a sight click, an aim-off or a
// hold, claim a statistical certainty that was never computed, or stand in
// for a coach. These are enforced by a prohibited-phrase test.

#include <QString>
#include <QStringList>
#include <QVector>

#include "WindMapAnalytics.h"

namespace ta {
namespace training {

// ── Evidence levels ────────────────────────────────────────────────────────
// PRODUCT states describing what the sample supports. They are NOT confidence
// intervals — no inferential statistic is computed anywhere in Wind Map, so
// none may be implied.
enum class EvidenceLevel : qint8 {
    Insufficient = 0,   // the sample threshold is not met
    Indicative,         // one valid group, no second to compare against
    Comparative,        // both sides meet the comparison threshold
    Repeated,           // RESERVED — cross-session only, never from one session
};

QString evidenceLevelLabel(EvidenceLevel e);
// The plain-language explanation the UI shows beside the label.
QString evidenceLevelExplanation(EvidenceLevel e);

// ── Verdict categories ─────────────────────────────────────────────────────
enum class VerdictCategory : qint8 {
    InsufficientSample = 0,
    FragmentedData,
    CompactButOffset,
    WiderUnderCondition,
    SimilarAcrossConditions,
    WideAcrossConditions,
    PositionSpecificDifference,
    NoValidComparison,
    RepeatedPattern,        // RESERVED — cross-session only
};

QString verdictCategoryLabel(VerdictCategory c);

enum class VerdictScope : qint8 {
    Session = 0,
    Position,
    Condition,
    CrossSession,           // RESERVED until the cross-session feature exists
};

QString verdictScopeLabel(VerdictScope s);

// ── The immutable verdict record ───────────────────────────────────────────
// Built once from analytics output. QML and the PDF both consume THIS; neither
// composes verdict text of its own.
struct Verdict {
    QString         verdictId;          // stable within a session, e.g. "pos2.cond1.offset"
    VerdictCategory category = VerdictCategory::NoValidComparison;
    VerdictScope    scope = VerdictScope::Session;

    int     position = 0;               // 0 n/a; 1 K, 2 P, 3 S
    QString positionName;
    QString referenceCondition;
    QString comparedCondition;
    int     sampleCountReference = 0;
    int     sampleCountCompared = 0;

    EvidenceLevel evidence = EvidenceLevel::Insufficient;

    QString headline;                   // 1. what happened
    QString observedPattern;            // the measured detail behind it
    QString interpretation;             // 3. what it MAY mean
    QString nextTrainingStep;           // 4. what to test next
    QString coachDecision;              // 5. what needs a coach (may be empty)
    QStringList limitations;
    QStringList supportingMetricIds;    // e.g. "groupDiameterMm", "magnitudeMm"

    int priority = 99;                  // lower shows first; see the rules doc
};

// ── Classification thresholds ──────────────────────────────────────────────
// Every one is documented and justified in the rules document. These are
// TECH AIM TRAINING-ANALYSIS RULES — not ISSF rules, not a statistical test,
// not a medical or scientific diagnosis.
//
// CLASSIFICATION (Stage 6.1.3): every constant below is a
//   REASONED PRODUCT RULE — COACH REVIEW REQUIRED.
// NONE is research-validated. The verified sources
// (docs/research/wind-map-feedback-evidence.md) support the CAUTIONS this
// product applies — they validate no numeric threshold in it. In particular
// Mononen et al. (2007) found the stability/accuracy relationship held only
// BETWEEN athletes and not WITHIN one, which is the case Wind Map analyses.
//
// A displacement must clear BOTH bars, so it survives neither a very large
// group (where the absolute bar alone would be trivial) nor a very tight one
// (where the relative bar alone would be).
inline constexpr double kOffsetRelativeToMeanRadius = 1.00;
inline constexpr double kOffsetMinimumMm            = 3.00;
// Compact and wider are RELATIVE to what this athlete shot in the reference
// condition — never an absolute standard, which would vary by athlete and
// position. The gap between them deliberately yields no dispersion claim.
//
// EVID-WM-001: these ratios are applied to RADIAL RMS DISPERSION, never to
// extreme spread. Extreme spread is an order statistic set by the two most
// distant shots, so it tends to grow with sample count; comparing it between a
// 5-shot and a 20-shot group could report "wider" from sample size alone.
// Radial RMS uses every shot and does not behave that way. The RATIOS are
// unchanged — only the metric they are applied to.
inline constexpr double kCompactRelativeToReference = 1.25;
inline constexpr double kWiderRelativeToReference   = 1.50;

// PROVISIONAL TECH AIM RULE — COACH REVIEW REQUIRED.
//
// "Elevated dispersion across every recorded condition", expressed as a
// multiple of the discipline's ISSF ring spacing rather than a bare millimetre
// figure — so it scales with the target the athlete is actually shooting at,
// the same principle Group Pattern Coach uses.
//
// It exists because when EVERY group is wide there is no within-session
// reference to scale against, so some absolute-ish bar is unavoidable. It is
// the weakest rule in the set and is treated accordingly:
//   · no verified source supports any dispersion figure for 50 m — this is a
//     Tech Aim product decision and nothing more;
//   · the wording says dispersion "remained elevated", never that the athlete
//     or the group was poor, bad, weak, unacceptable or inadequate;
//   · it triggers a referral to Group Pattern Coach, not a judgement.
//
// The superseded 40 mm EXTREME-SPREAD constant is deliberately gone from the
// code. Its value survives only as history in
// docs/training-lab-wind-map-verdict-rules.md, so it cannot silently keep
// classifying athletes under a new name.
inline constexpr double kElevatedDispersionRingMultiple = 1.50;

inline constexpr int    kFragmentedMinConditions    = 3;

class WindMapVerdictEngine
{
public:
    // The one entry point. Returns verdicts ordered by priority, so the view
    // shows [0] as primary and the rest as secondary observations.
    static QVector<Verdict> evaluate(const SessionAnalysis& analysis);

    // Exposed for tests and for the rules document's boundary cases.
    //
    // The dispersion predicates take RADIAL RMS on both sides. Passing a
    // diameter would compile and be wrong, so the parameter names say what the
    // arguments must be and the boundary tests pin the behaviour.
    static bool isMeaningfulOffset(double magnitudeMm, double referenceMeanRadiusMm);
    static bool isCompact(double comparedRadialRmsMm, double referenceRadialRmsMm);
    static bool isWider(double comparedRadialRmsMm, double referenceRadialRmsMm);
    // Elevated dispersion for a single group, against the discipline geometry.
    static bool isElevatedDispersion(double radialRmsMm, double ringSpacingMm);
};

// ── Relative wind direction (optional session metadata) ────────────────────
// The recorded compass value is AUTHORITATIVE and is never mutated. When a
// session records the optional firing direction, an athlete-relative label is
// DERIVED as an extra field. Sessions without it stay fully readable.
enum class RelativeWind : qint8 {
    Unavailable = 0,        // no firing direction recorded
    Headwind,
    HeadwindFromLeft,
    LeftToRightCrosswind,
    TailwindFromLeft,
    Tailwind,
    TailwindFromRight,
    RightToLeftCrosswind,
    HeadwindFromRight,
};

QString relativeWindLabel(RelativeWind r);

// windFromDegrees: the recorded compass direction the wind comes FROM.
// firingDegrees:   the compass direction the athlete fires TOWARDS.
// Returns Unavailable when the firing direction is absent (< 0).
RelativeWind relativeWindFor(qint16 windFromDegrees, int firingDegrees);

} // namespace training
} // namespace ta

#endif // TA_TRAINING_WINDMAPVERDICT_H
