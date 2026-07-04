#pragma once

// ============================================================================
//  TechAim Coach Analytics — result object (Phase 1 / milestone 1)
// ----------------------------------------------------------------------------
//  CoachReportData is the single value returned by CoachAnalyticsEngine::analyze.
//  It is a plain data object (no Qt, no logic) so the QML layer only ever
//  *displays* it.
//
//  This milestone populates:
//    - validation summary
//    - executiveSummary
//  The remaining sections listed in the specification (shot distribution,
//  trend, heat map, timing, position, recovery, fatigue, training priorities,
//  coach conclusion, coach diary) are added in later phases. They are named
//  here as a contract but intentionally NOT computed yet — the engine must not
//  invent conclusions it cannot support with data.
// ============================================================================

#include <string>
#include <vector>
#include <utility>
#include "ShotAnalyticsTypes.h"

namespace techaim {
namespace analytics {

// ===========================================================================
//  Confidence & evidence infrastructure
// ---------------------------------------------------------------------------
//  Every higher-level conclusion the engine makes (fatigue, training
//  priorities, the coach conclusion) must be able to explain *why* it was
//  reached. These primitives make that traceability first-class: a conclusion
//  carries a 0..100 confidence and the list of measured facts behind it.
//
//  Raw-metric sections (Executive Summary, Shot Distribution, Trend, Heat Map)
//  do NOT carry confidence — they are direct measurements, not judgements.
// ===========================================================================

// One measurable, human-readable justification for a conclusion.
struct Evidence {
    std::string statement;      // e.g. "Last third average declined by 0.23"
    std::string metricKey;      // stable machine id, e.g. "trend.lateSessionDrop"
    double      value    = 0.0; // the number behind the statement
    bool        hasValue = false;

    Evidence() = default;
    explicit Evidence(std::string s) : statement(std::move(s)) {}
    Evidence(std::string s, std::string key, double v)
        : statement(std::move(s)), metricKey(std::move(key)), value(v), hasValue(true) {}
};

// Mixed into any confidence-scored conclusion.
struct ConfidenceScored {
    double confidence = 0.0;    // 0..100
    std::vector<Evidence> evidence;

    void addEvidence(Evidence e)        { evidence.push_back(std::move(e)); }
    void addEvidence(std::string s)     { evidence.emplace_back(std::move(s)); }
    void addEvidence(std::string s, std::string key, double v)
                                        { evidence.emplace_back(std::move(s), std::move(key), v); }
};

// Estimated coaching impact of acting on a finding.
enum class ImpactLevel { Low, Medium, High };
std::string toString(ImpactLevel i);

// Exemplar of a confidence-scored recommendation (populated in Phase 11).
// Declared now so the shape is fixed and every priority is traceable.
struct TrainingPriority : ConfidenceScored {
    std::string priority;                 // e.g. "Standing Stability"
    ImpactLevel impact = ImpactLevel::Low;
    std::vector<std::string> linkedMetrics;
};

// ---- Section 1: Executive Summary -----------------------------------------
struct ExecutiveSummary {
    int    competitionShotCount   = 0;

    // Scoring
    double totalScore             = 0.0;
    double averageScore           = 0.0;
    double bestShot               = 0.0;
    double worstShot              = 0.0;
    double scoreStandardDeviation = 0.0;   // sample SD of decimal scores
    double consistencyPercentage  = 0.0;   // normalized vs elite SD, clamped 0..100

    // Group geometry (mean point of impact = group centre)
    double groupCentreX           = 0.0;   // = mean x  (also horizontalBias)
    double groupCentreY           = 0.0;   // = mean y  (also verticalBias)
    double groupRadius            = 0.0;   // mean distance from group centre
    double groupDiameter          = 0.0;   // extreme spread (max pairwise distance)
    double horizontalBias         = 0.0;   // mean x
    double verticalBias           = 0.0;   // mean y
    double horizontalSD           = 0.0;   // sample SD of x
    double verticalSD             = 0.0;   // sample SD of y

    // Segment comparisons (order = shotNumber/timestamp)
    double firstHalfAverage       = 0.0;
    double secondHalfAverage      = 0.0;
    double firstThirdAverage      = 0.0;
    double lastThirdAverage       = 0.0;

    // Trend
    double         trendSlope     = 0.0;   // least-squares slope, score vs shotNumber
    TrendDirection trendDirection = TrendDirection::Stable;

    // Filled by later phases; kept honest ("Not analysed") until then.
    std::string fatigueLevel        = "Not analysed";
    std::string recoveryRating      = "Not analysed";
    std::string mainTrainingPriority= "Not analysed";
};

// ---- Section 2: Shot Distribution -----------------------------------------
struct HistogramBin {
    double lowerEdge = 0.0;   // inclusive
    double upperEdge = 0.0;   // exclusive (last bin inclusive of the max score)
    int    count     = 0;
};

struct ShotDistribution {
    int totalAnalysedShots = 0;

    // Quality-band counts (thresholds configurable; defaults per spec)
    int perfectCount    = 0;  // score >= 10.8
    int excellentCount  = 0;  // 10.5 <= score < 10.8
    int goodCount       = 0;  // 10.2 <= score < 10.5
    int acceptableCount = 0;  // 10.0 <= score < 10.2
    int recoveryCount   = 0;  // 9.5  <= score < 10.0
    int poorCount       = 0;  // score < 9.5

    // Percentages of total analysed shots (0..100)
    double perfectPct    = 0.0;
    double excellentPct  = 0.0;
    double goodPct       = 0.0;
    double acceptablePct = 0.0;
    double recoveryPct   = 0.0;
    double poorPct       = 0.0;

    double bestShot  = 0.0;
    double worstShot = 0.0;

    // Reporting counts (thresholds configurable; defaults 10.5 / 10.7 / 10.8 / 10.0)
    int countAtLeast10_5 = 0;
    int countAtLeast10_7 = 0;
    int countAtLeast10_8 = 0;
    int countBelow10_0   = 0;

    // Relative to the athlete's own performance this session
    int countBelowAverage         = 0;  // score < averageScore
    int countBelowAverageMinusSD  = 0;  // score < averageScore - scoreSD

    // Streaks (longest consecutive runs)
    int bestStreak10_5     = 0;  // score >= 10.5
    int bestStreak10_7     = 0;  // score >= 10.7
    int longestPoorStreak  = 0;  // score < 9.5

    // Histogram (configurable bin width, default 0.1)
    double binWidth = 0.1;
    std::vector<HistogramBin> histogram;
};

// ---- Section 3: Trend Analysis --------------------------------------------
struct TrendAnalysis {
    std::vector<double> scores;          // per-shot decimal scores (session order)

    // Rolling series use COMPLETE windows only (length n-w+1); empty + flag=false
    // when the window is not possible (n < w). No faked partial windows.
    bool rolling5Available  = false;
    std::vector<double> rolling5Average;
    std::vector<double> rolling5SD;
    bool rolling10Available = false;
    std::vector<double> rolling10Average;
    std::vector<double> rolling10SD;

    bool   first10Available = false;
    double first10Average   = 0.0;
    bool   last10Available  = false;
    double last10Average    = 0.0;

    bool   halvesAvailable   = false;    // n >= 2
    double firstHalfAverage  = 0.0;
    double secondHalfAverage = 0.0;

    bool   thirdsAvailable    = false;   // n >= 3
    double firstThirdAverage  = 0.0;
    double middleThirdAverage = 0.0;
    double lastThirdAverage   = 0.0;

    bool   regressionAvailable   = false; // n >= 2
    double regressionSlope       = 0.0;   // score vs shot order
    double regressionCorrelation = 0.0;

    bool   rolling5ExtremesAvailable  = false;
    double highestRolling5Average     = 0.0;
    double lowestRolling5Average      = 0.0;
    bool   rolling10ExtremesAvailable = false;
    double highestRolling10Average    = 0.0;
    double lowestRolling10Average     = 0.0;

    TrendDirection trendDirection = TrendDirection::Stable;

    // Rolling average declines while rolling SD rises (computed on rolling-5
    // series; requires n >= 6 to have a slope). false when undeterminable.
    bool deteriorationDeterminable = false;
    bool deteriorationFlag         = false;

    // Late-session drop = firstThirdAverage - lastThirdAverage (thirds required)
    double lateSessionDrop     = 0.0;
    bool   lateSessionDropFlag = false;   // drop > threshold (default 0.15)
};

// ---- Reusable raw-coordinate statistics (millimetres) ---------------------
//  The GROUND TRUTH for a set of shots, computed from raw coordinates. Heat-map
//  grids are a visualisation layer only; future algorithms (KDE, clustering,
//  Gaussian density, CEP, ...) must use these raw statistics / the raw
//  coordinates, never reconstructed grid data. All values are in millimetres.
struct CoordinateStats {
    bool    hasData        = false;
    int     shotCount      = 0;
    Point2D mpi;                       // mean point of impact
    double  meanRadius      = 0.0;     // mean distance of shots from MPI
    double  groupRadius     = 0.0;     // == meanRadius (project definition); kept for naming
    double  groupDiameter   = 0.0;     // extreme spread: max pairwise distance
    double  horizontalSpread= 0.0;     // sample SD of x
    double  verticalSpread  = 0.0;     // sample SD of y
};

// ---- Section 4: Heat Map source data (Phase 6) ----------------------------
//  Pure numeric source data for later rendering — no drawing here. Grids are
//  square, aligned to a common frame (origin + bin size) so that sub-maps
//  share bins and can be compared cell-for-cell.
struct HeatMapCell {
    int    binX     = 0;      // column index (0 = leftmost)
    int    binY     = 0;      // row index    (0 = lowest)
    double centreX  = 0.0;    // cell centre, target-face mm
    double centreY  = 0.0;
    int    count    = 0;
    double averageScore = 0.0;
    std::vector<int> shotNumbers;
};

// A density grid over one subset of shots, on a shared coordinate frame.
struct HeatMapGrid {
    // Frame (shared across all sub-maps in a HeatMapAnalysis so bins align).
    double binSize    = 5.0;  // mm per cell (square)
    double originX    = 0.0;  // lower-left corner of cell (0,0)
    double originY    = 0.0;
    int    gridWidth  = 0;    // columns
    int    gridHeight = 0;    // rows

    int    shotCount  = 0;
    bool   hasData    = false;
    std::vector<HeatMapCell> cells;   // VISUALISATION LAYER ONLY, ordered (binY, binX)

    // Ground-truth raw-coordinate statistics (mm). Use these — not the binned
    // cells — for any further mathematics.
    CoordinateStats stats;

    int maxCellDensity = 0;            // peak cell count

    // Dominant impact zone = most populated cell (ties -> lowest binY, then binX)
    bool   hasDominantZone   = false;
    int    dominantBinX      = 0;
    int    dominantBinY      = 0;
    double dominantCentreX   = 0.0;
    double dominantCentreY   = 0.0;
    int    dominantCount     = 0;
    double dominantSharePct  = 0.0;    // dominantCount / shotCount * 100
};

// Comparison metrics between two grids on the same frame (B relative to A).
struct HeatMapComparison {
    bool   available     = false;      // both grids have data
    double shiftX        = 0.0;        // MPI shift, + = right   (B - A, mm)
    double shiftY        = 0.0;        // MPI shift, + = high
    double shiftDistance = 0.0;        // sqrt(shiftX^2 + shiftY^2)
    double radiusChange  = 0.0;        // B.meanRadius - A.meanRadius (mm)
    double radiusChangePct = 0.0;      // relative to A.meanRadius (0 if A radius ~0)
    double dominantShiftX = 0.0;       // dominant-cell centre shift, mm
    double dominantShiftY = 0.0;
};

struct HeatMapAnalysis {
    bool   available = false;
    double binSize   = 5.0;

    // Temporal / quality / all
    HeatMapGrid allShots;
    HeatMapGrid firstHalf;
    HeatMapGrid secondHalf;
    HeatMapGrid firstThird;
    HeatMapGrid middleThird;
    HeatMapGrid lastThird;
    HeatMapGrid goodShots;   // score >= good threshold (default 10.5)
    HeatMapGrid poorShots;   // score <  poor threshold (default 10.0)

    // Position-specific (empty grids for positions not present)
    HeatMapGrid prone;
    HeatMapGrid kneeling;
    HeatMapGrid standing;
    HeatMapGrid airRifle;
    HeatMapGrid airPistol;
    HeatMapGrid unknownPosition;

    // Drift comparisons over the session
    HeatMapComparison firstToSecondHalf;
    HeatMapComparison firstToLastThird;

    // Convenience X/Y drift read-outs (mm)
    double horizontalDriftHalves = 0.0;  // firstToSecondHalf.shiftX
    double verticalDriftHalves   = 0.0;  // firstToSecondHalf.shiftY
    double horizontalDriftThirds = 0.0;  // firstToLastThird.shiftX
    double verticalDriftThirds   = 0.0;
};

// ---- Reusable timing primitives -------------------------------------------
//  Intervals are derived ONLY from real timestamps. Later phases (fatigue,
//  pressure, recovery) reuse IntervalSeries rather than recomputing gaps.
enum class PaceTrend { SpeedingUp, Steady, SlowingDown };
std::string toString(PaceTrend p);

// One gap between two consecutive, both-timestamped shots (seconds).
struct ShotInterval {
    int    fromIndex      = 0;   // index of the earlier shot in the analysed list
    int    toIndex        = 0;   // index of the later shot
    int    fromShotNumber = 0;
    int    toShotNumber   = 0;
    double seconds        = 0.0; // timestamp(to) - timestamp(from), >= 0
};

struct IntervalSeries {
    bool available       = false;         // at least one valid interval exists
    int  timedShotCount  = 0;             // shots carrying a timestamp
    std::vector<ShotInterval> intervals;
    std::vector<double>       seconds;    // parallel: just the interval seconds
};

// ---- Section 5: Timing Analysis (Phase 7) ---------------------------------
//  Every metric carries its own availability flag. When timestamps are missing
//  nothing is fabricated: available stays false and values remain at defaults.
struct TimingAnalysis {
    bool available      = false;          // timestamps present and >= 1 interval
    int  timedShotCount = 0;
    int  intervalCount  = 0;
    std::vector<double> intervals;        // seconds per gap (to the later shot)

    bool   statsAvailable   = false;      // intervalCount >= 1
    double averageInterval  = 0.0;
    double medianInterval   = 0.0;
    double intervalSD       = 0.0;        // sample SD (0 for a single interval)
    double fastestInterval  = 0.0;
    double slowestInterval  = 0.0;

    bool rolling3Available = false;
    std::vector<double> rolling3Average;  // complete-window over the interval series
    std::vector<double> rolling3SD;
    bool rolling5Available = false;
    std::vector<double> rolling5Average;
    std::vector<double> rolling5SD;

    // Rushed / delayed relative to the median interval (factors configurable).
    int rushedShotCount  = 0;             // interval < rushedFactor  * median
    int delayedShotCount = 0;             // interval > delayedFactor * median
    std::vector<int> rushedShotNumbers;   // the later shot of each rushed gap
    std::vector<int> delayedShotNumbers;

    bool   rhythmAvailable   = false;     // intervalCount >= 2 and median > 0
    double rhythmConsistency = 0.0;       // clamp(100 - SD/median*100, 0, 100)

    bool      trendAvailable = false;     // intervalCount >= 2
    double    intervalRegressionSlope       = 0.0;  // interval vs gap index
    double    intervalRegressionCorrelation = 0.0;
    PaceTrend intervalTrend  = PaceTrend::Steady;

    bool   scoreIntervalCorrelationAvailable = false; // intervalCount >= 2
    double scoreVsIntervalCorrelation = 0.0;  // corr(interval, score of the later shot)

    // Time taken AFTER a poor shot before the next shot (reset time).
    bool   recoveryIntervalAvailable   = false;
    int    recoveryIntervalSampleCount = 0;
    double averageRecoveryInterval     = 0.0;

    // Decision time = interval leading INTO a shot.
    bool   decisionBeforeHighAvailable   = false;  // later shot >= high threshold (10.7)
    int    decisionBeforeHighSampleCount = 0;
    double averageDecisionTimeBeforeHigh = 0.0;

    bool   decisionBeforePoorAvailable   = false;  // later shot < poor threshold (10.0)
    int    decisionBeforePoorSampleCount = 0;
    double averageDecisionTimeBeforePoor = 0.0;
};

// ---- Top-level result -----------------------------------------------------
struct CoachReportData {
    // Validation / provenance
    bool        valid                = false; // false => see message, other fields default
    std::string message;                      // human-readable validation outcome

    int  inputShotCount              = 0;     // raw shots received
    int  invalidRemovedCount         = 0;     // isValid == false
    int  sighterExcludedCount        = 0;     // sighters dropped (default policy)
    int  analysedShotCount           = 0;     // shots actually analysed
    int  competitionShotCount        = 0;     // of analysed, isCompetitionShot == true
    bool lowSampleWarning            = false; // analysedShotCount < 10 (results less reliable)
    bool sightersIncluded            = false; // policy actually applied

    // Sections implemented
    ExecutiveSummary executiveSummary;   // Section 1
    ShotDistribution shotDistribution;   // Section 2 (Phase 4)
    TrendAnalysis    trendAnalysis;      // Section 3 (Phase 5)
    HeatMapAnalysis  heatMapAnalysis;    // Section 4 (Phase 6)
    TimingAnalysis   timingAnalysis;     // Section 5 (Phase 7)

    // Sections 6..11 — declared contract, implemented in later phases:
    //   TimingAnalysis    timingAnalysis;
    //   PositionAnalysis  positionAnalysis;
    //   RecoveryAnalysis  recoveryAnalysis;
    //   FatigueAnalysis   fatigueAnalysis;
    //   TrainingPriorities trainingPriorities;
    //   CoachConclusion   coachConclusion;
    //   CoachDiaryFields  manualDiaryFields;
};

} // namespace analytics
} // namespace techaim
