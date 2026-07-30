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
// A displacement must clear BOTH bars, so it survives neither a very large
// group (where the absolute bar alone would be trivial) nor a very tight one
// (where the relative bar alone would be).
inline constexpr double kOffsetRelativeToMeanRadius = 1.00;
inline constexpr double kOffsetMinimumMm            = 3.00;
// Compact and wider are RELATIVE to what this athlete shot in the reference
// condition — never an absolute standard, which would vary by athlete and
// position. The gap between them deliberately yields no dispersion claim.
inline constexpr double kCompactRelativeToReference = 1.25;
inline constexpr double kWiderRelativeToReference   = 1.50;
// The least well-founded constant here: absolute, because when EVERY group is
// wide there is no within-session reference to scale against. Flagged for
// coach review in the rules document.
inline constexpr double kWideAbsoluteMm             = 40.00;
inline constexpr int    kFragmentedMinConditions    = 3;

class WindMapVerdictEngine
{
public:
    // The one entry point. Returns verdicts ordered by priority, so the view
    // shows [0] as primary and the rest as secondary observations.
    static QVector<Verdict> evaluate(const SessionAnalysis& analysis);

    // Exposed for tests and for the rules document's boundary cases.
    static bool isMeaningfulOffset(double magnitudeMm, double referenceMeanRadiusMm);
    static bool isCompact(double comparedDiameterMm, double referenceDiameterMm);
    static bool isWider(double comparedDiameterMm, double referenceDiameterMm);
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
