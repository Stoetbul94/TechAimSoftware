#ifndef TA_TRAINING_WINDMAPANALYTICS_H
#define TA_TRAINING_WINDMAPANALYTICS_H

// Wind Map — analytics engine (Training Lab Release 2, Stage 6).
//
// PURE QtCore. No QML, no GUI, no presentation. Every reported value derives
// HERE; QML and the PDF only format what this returns. Compiling it into the
// QT=core reliability harness is the proof of that.
//
// INPUT IS THE REDUCER STATE. The engine consumes SessionState::wmShots —
// the replayed, recovered record with each shot's IMMUTABLE wind snapshot.
// It never reads QML state and never reconstructs data from screen controls,
// so an analysis of a recovered session is identical to a live one.
//
// WHAT IT MAY SAY. It reports observed differences between groups of shots
// and the evidence behind them. It never states that wind CAUSED anything,
// never proposes a sight click, hold-off or aiming point, and withholds a
// comparison rather than making one from too few shots.

#include <QString>
#include <QVector>

#include "WindMapTypes.h"
#include "reliability/reducer/SessionState.h"

namespace ta {
namespace training {

// ── Approved sample thresholds ─────────────────────────────────────────────
// Below these, the corresponding value is NOT produced and the caller is told
// how many more shots are needed. Nothing is reported "provisionally".
inline constexpr int kMinSamplesMpi        = 3;   // mean point of impact
inline constexpr int kMinSamplesDispersion = 5;   // group / spread / sd
inline constexpr int kMinSamplesComparison = 5;   // per side of a comparison

// How much of a statistic the sample supports.
enum class Evidence : qint8 {
    Insufficient = 0,   // below kMinSamplesMpi — nothing is claimed
    Indicative,         // MPI available, dispersion not (3-4 shots)
    Sufficient,         // dispersion available (5+)
    Strong,             // a comfortably sized sample (10+)
};

QString evidenceLabel(Evidence e);

// ── Condition grouping ─────────────────────────────────────────────────────
// Three independent ways to group the same shots. Calm and No Reading are
// ALWAYS their own groups and are never merged with each other or with a
// measured reading.
enum class GroupingMode : qint8 {
    Direction = 0,      // Calm · N · NE · E · SE · S · SW · W · NW · No Reading
    SpeedBand,          // Calm · <=2 · <=4 · <=7 · >7 m/s · No Reading
    ExactCondition,     // "W · 2.5 m/s", "NE · 5.0 m/s", Calm, No Reading
};

// A stable identity for one analysis group.
struct ConditionKey {
    bool    hasReading = false;
    bool    calm = false;
    qint16  directionDegrees = 0;    // meaningful only for ExactCondition
    qint32  speedHundredthMs = 0;    // meaningful only for ExactCondition
    qint8   sector = -1;             // WindSector, -1 = n/a
    qint8   band = -1;               // WindSpeedBand, -1 = n/a
    bool operator==(const ConditionKey& o) const;
    // Human label: "Calm", "No reading", "W", "2.0-4.0 m/s", "W · 2.5 m/s".
    QString label(GroupingMode mode) const;
};

// ── Per-group statistics ───────────────────────────────────────────────────
// EVERY statistic carries its own n. A value that its sample does not support
// is left `false`/0 and its `has*` flag says so — the caller must not print a
// number whose flag is false.
struct GroupStats {
    ConditionKey key;
    QString  label;
    int      n = 0;                  // counted shots in this group
    Evidence evidence = Evidence::Insufficient;

    // score
    bool   hasMeanScore = false;
    double meanScore = 0.0;

    // centre — n >= kMinSamplesMpi
    bool   hasMpi = false;
    double mpiXMm = 0.0, mpiYMm = 0.0;

    // dispersion — n >= kMinSamplesDispersion
    bool   hasDispersion = false;
    double meanRadiusMm = 0.0;       // mean distance from the group's own MPI
    double groupDiameterMm = 0.0;    // maximum spread, extreme shot to shot
    double horizontalSpreadMm = 0.0; // max x - min x
    double verticalSpreadMm = 0.0;   // max y - min y
    double scoreStdDev = 0.0;
    double radiusStdDev = 0.0;

    // How many more shots this group needs to reach the next threshold.
    int shotsNeededForMpi() const;
    int shotsNeededForDispersion() const;
};

// The reference a shift is measured FROM. Position-specific in 3P.
enum class ReferenceKind : qint8 {
    None = 0,
    CalmGroup,          // the recorded-calm group, when it is large enough
    MostPopulated,      // the largest valid condition group
    SessionCentre,      // the centre of every counted shot in this position
    CoachSelected,      // an explicitly chosen condition
};

QString referenceKindLabel(ReferenceKind k);

struct ReferenceCentre {
    ReferenceKind kind = ReferenceKind::None;
    ConditionKey  key;               // meaningful for CalmGroup/MostPopulated/CoachSelected
    QString label;
    bool   valid = false;
    int    n = 0;
    double xMm = 0.0, yMm = 0.0;
};

// An OBSERVED difference between a group's centre and the reference centre.
// It is a description of two measured centres, not an instruction.
struct ShiftVector {
    ConditionKey key;
    QString label;
    bool   valid = false;            // false => below threshold on either side
    int    n = 0;                    // n of the condition group
    int    referenceN = 0;
    double dxMm = 0.0, dyMm = 0.0;   // condition MPI - reference centre
    double magnitudeMm = 0.0;
    double bearingDegrees = 0.0;     // 0 = up/12 o'clock, clockwise
    QString directionWords;          // "6.8 mm right, 1.2 mm high"
    Evidence evidence = Evidence::Insufficient;
    int shotsNeeded = 0;             // > 0 when withheld for sample size
};

// One position's complete analysis. For 50m Prone there is exactly one of
// these, with position 0.
struct PositionAnalysis {
    int     position = 0;            // 0 n/a, 1 K, 2 P, 3 S
    QString positionName;
    int     countedShots = 0;
    int     sighterShots = 0;
    bool    hasOverallMpi = false;
    double  overallMpiXMm = 0.0, overallMpiYMm = 0.0;
    ReferenceCentre reference;
    QVector<GroupStats>  byDirection;
    QVector<GroupStats>  bySpeedBand;
    QVector<GroupStats>  byExactCondition;
    QVector<ShiftVector> shifts;      // vs `reference`, grouped by exact condition
};

// One counted or sighter shot, flattened for the timeline and the appendix.
struct TimelineEntry {
    int     shotId = 0;
    bool    sighter = false;
    int     position = 0;
    QString positionName;
    double  xMm = 0.0, yMm = 0.0;
    double  score = 0.0;
    qint64  splitMs = 0;
    bool    conditionChangedBefore = false;   // first shot under a new condition
    bool    phaseChangedBefore = false;       // sighter -> counted boundary
    WindConditionSnapshot wind;
    QString conditionLabel;
};

// The pattern the evidence supports. Deliberately a small, closed set.
enum class PatternCategory : qint8 {
    InsufficientSample = 0,
    TightButOffset,
    WiderUnderCondition,
    SimilarAcrossConditions,
    WideAcrossAllConditions,
    PositionSpecificDifference,
};

QString patternCategoryLabel(PatternCategory c);

// UI-WIND-006: WHAT a finding is about. Without this the view cannot tell a
// cross-position session comparison from a Kneeling-only result, and showed
// the same session-level text unchanged under every position — an athlete
// could read it as a statement about that position alone.
enum class FindingScope : qint8 {
    Session = 0,        // the whole session, e.g. a position-to-position comparison
    Position,           // one position only (positionName says which)
    Condition,          // one recorded condition only
};

QString findingScopeLabel(FindingScope s);

struct Finding {
    PatternCategory category = PatternCategory::InsufficientSample;
    FindingScope    scope = FindingScope::Session;
    int             position = 0;        // 0 = n/a; 1 K, 2 P, 3 S when scope==Position
    QString         positionName;        // empty unless scope==Position
    QString         conditionLabel;      // empty unless scope==Condition
    QString text;               // the observation, neutrally worded
    QString suggestion;         // a next-session action, never a correction
    int     n = 0;
    int     shotsNeeded = 0;
};

struct SessionAnalysis {
    bool    valid = false;
    QString disciplineId;
    bool    threePositions = false;
    int     countedShots = 0;
    int     sighterShots = 0;
    int     uniqueConditions = 0;
    int     conditionEntries = 0;
    int     countedWithReading = 0, countedCalm = 0, countedNoReading = 0;
    QVector<PositionAnalysis> positions;    // 1 for Prone, up to 3 for 3P
    QVector<TimelineEntry>    timeline;
    QVector<Finding>          findings;
    QStringList               limitations;
};

// ── The engine ─────────────────────────────────────────────────────────────
class WindMapAnalyticsEngine
{
public:
    struct Options {
        bool includeSighters = false;        // sighters are NEVER counted; this
                                             // only adds them to the timeline
        ReferenceKind preferredReference = ReferenceKind::CalmGroup;
        ConditionKey  coachSelected;         // used when preferredReference is
        bool          hasCoachSelected = false;
    };

    // The one entry point. Everything downstream — screen, PDF — reads this.
    // Two overloads rather than a defaulted argument: Options is a nested type
    // and its default member initializers are not usable in a default argument
    // until the enclosing class is complete.
    static SessionAnalysis analyse(const ta::rel::SessionState& state);
    static SessionAnalysis analyse(const ta::rel::SessionState& state,
                                   const Options& opts);

    // Exposed for tests and for the report's grouping tables.
    static ConditionKey keyFor(const WindConditionSnapshot& w, GroupingMode mode);
    static QVector<GroupStats> group(const QVector<TimelineEntry>& shots,
                                     GroupingMode mode);
    static GroupStats statsFor(const QVector<const TimelineEntry*>& shots,
                               const ConditionKey& key, GroupingMode mode);
    static ReferenceCentre chooseReference(const QVector<GroupStats>& exact,
                                           const QVector<TimelineEntry>& shots,
                                           const Options& opts);
    static ShiftVector shiftOf(const GroupStats& g, const ReferenceCentre& ref);
    static QVector<Finding> deriveFindings(const SessionAnalysis& a);
};

} // namespace training
} // namespace ta

#endif // TA_TRAINING_WINDMAPANALYTICS_H
