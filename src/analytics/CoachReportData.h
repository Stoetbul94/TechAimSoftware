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

// How much concern an interpreted metric represents (data-driven, not advice).
enum class Severity { None, Low, Moderate, High, Critical };
std::string toString(Severity s);

// The common interpreted-metric object used across all judgement sections
// (position, fatigue, recovery, pressure, priorities, coach conclusion).
// A measurement is a raw number; a PerformanceMetric is a measurement that has
// been interpreted, with confidence, evidence, availability and severity.
struct PerformanceMetric : ConfidenceScored {
    std::string name;
    double      value        = 0.0;
    bool        availability  = false;
    Severity    severity      = Severity::None;
};

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

// ---- Reusable score-recovery primitive ------------------------------------
//  Poor shot = score < poorThreshold OR score < (setAverage - setSD).
//  A poor shot is only counted if it is followed by another shot. Successful
//  recovery = the next shot reaches the set average. Reused by Phase 8 and the
//  future Phase 9 Recovery Analysis so the definition stays single-sourced.
struct RecoverySummary {
    bool   available               = false;
    int    poorShotCount           = 0;   // poor shots that have a following shot
    int    successfulRecoveryCount = 0;
    int    failedRecoveryCount     = 0;
    double recoveryPercentage      = 0.0; // 0..100
    double averageNextShotAfterPoor= 0.0;
};

// ===========================================================================
//  Section 6: Position Analysis (Phase 8)
// ---------------------------------------------------------------------------
//  Each shooting position is analysed as an INDEPENDENT dataset; positions are
//  never merged. Measurements (raw numbers, for graphs) are kept separate from
//  the derived performance metrics (interpreted, for conclusions).
// ===========================================================================

struct PositionMeasurements {
    bool available = false;
    int  shotCount = 0;

    // Scoring
    double totalScore   = 0.0;
    double averageScore = 0.0;
    double medianScore  = 0.0;
    double minScore     = 0.0;
    double maxScore     = 0.0;
    double scoreSD      = 0.0;
    double scoreRange   = 0.0;
    double scoreVariance= 0.0;
    double coefficientOfVariation = 0.0;   // scoreSD / averageScore (0 if avg ~0)
    bool   hasIntegerScores    = false;
    double averageIntegerScore = 0.0;

    // Geometry (raw coordinates, mm)
    CoordinateStats geometry;              // MPI, mean/group radius, diameter, H/V spread
    double horizontalBias  = 0.0;          // = geometry.mpi.x
    double verticalBias    = 0.0;          // = geometry.mpi.y
    double dispersionRatio = 0.0;          // horizontalSpread / verticalSpread (0 if vSpread ~0)

    // Trend
    bool   trendAvailable   = false;       // shotCount >= 2
    double trendSlope       = 0.0;
    double trendCorrelation = 0.0;
    bool   halvesAvailable  = false;
    double firstHalfAverage = 0.0;
    double secondHalfAverage= 0.0;
    bool   thirdsAvailable  = false;
    double firstThirdAverage  = 0.0;
    double middleThirdAverage = 0.0;
    double lastThirdAverage   = 0.0;

    // Timing (reuses the timing primitives)
    bool   timingAvailable   = false;
    double averageInterval   = 0.0;
    double rhythmConsistency = 0.0;
    int    rushedShotCount   = 0;
    int    delayedShotCount  = 0;

    // Outliers (spatial: radial distance from MPI beyond mean + sigma*SD)
    int    outlierCount            = 0;
    double outlierPercentage       = 0.0;
    double averageOutlierDistance  = 0.0;  // mm from MPI
    std::string dominantOutlierDirection = "none"; // right/high-right/high/.../low-right
};

struct PositionReport {
    PositionType position = PositionType::Unknown;
    std::string  positionName;
    bool   available  = false;             // shotCount >= 1
    int    shotCount  = 0;
    double confidence = 0.0;               // 0..100, grows with sample size

    PositionMeasurements measurements;
    RecoverySummary      recovery;
    std::vector<PerformanceMetric> metrics; // derived, confidence-scored

    // Weighted position quality (components normalized 0..100; weights configurable)
    bool   qualityAvailable = false;
    double qualityScore     = 0.0;         // 0..100
    double scoreComponent        = 0.0; bool scoreComponentAvailable       = false;
    double consistencyComponent  = 0.0; bool consistencyComponentAvailable = false;
    double geometryComponent     = 0.0; bool geometryComponentAvailable    = false;
    double trendComponent        = 0.0; bool trendComponentAvailable       = false;
    double recoveryComponent     = 0.0; bool recoveryComponentAvailable    = false;
    double timingComponent       = 0.0; bool timingComponentAvailable      = false;

    // Points lost = shotCount * maxShotScore - totalScore (maxShotScore configurable)
    double pointsLost = 0.0;

    // Training-priority support — raw measurable info only, no recommendations.
    std::vector<Evidence>    strengths;
    std::vector<Evidence>    weaknesses;
    std::vector<std::string> linkedMetrics;
};

struct PositionRankingEntry {
    PositionType position = PositionType::Unknown;
    std::string  positionName;
    double       value = 0.0;              // the ranked quantity
};

struct PositionAnalysis {
    bool available        = false;         // >= 1 position present
    int  positionsPresent = 0;

    std::vector<PositionReport> positions;  // deterministic enum order, present only

    // Rankings (each ordered best-first for its dimension; only positions where
    // the ranked quantity is available are included).
    std::vector<PositionRankingEntry> pointsLostRanking;   // most points lost first (worst)
    std::vector<PositionRankingEntry> consistencyRanking;  // most consistent first
    std::vector<PositionRankingEntry> geometryRanking;     // smallest group first
    std::vector<PositionRankingEntry> trendRanking;        // best (most positive) trend first
    std::vector<PositionRankingEntry> recoveryRanking;     // best recovery first
    std::vector<PositionRankingEntry> timingRanking;       // best rhythm first
    std::vector<PositionRankingEntry> overallRanking;      // highest quality first

    // Identified extremes (require >= 2 positions for a comparison to be meaningful).
    bool comparativeAvailable = false;
    bool hasStrongest = false;             PositionType strongestPosition = PositionType::Unknown;
    bool hasWeakest   = false;             PositionType weakestPosition   = PositionType::Unknown;
    bool hasMostConsistent  = false;       PositionType mostConsistentPosition  = PositionType::Unknown;
    bool hasLeastConsistent = false;       PositionType leastConsistentPosition = PositionType::Unknown;
    bool hasLargestGroup  = false;         PositionType largestGroupPosition  = PositionType::Unknown;
    bool hasSmallestGroup = false;         PositionType smallestGroupPosition = PositionType::Unknown;
    bool hasGreatestImprovement   = false; PositionType greatestImprovementPosition   = PositionType::Unknown;
    bool hasGreatestDeterioration = false; PositionType greatestDeteriorationPosition = PositionType::Unknown;
};

// ===========================================================================
//  Section 7: Recovery Analysis (Phase 9)
// ---------------------------------------------------------------------------
//  What happens AFTER a poor shot, an unstable sequence, or a local dip — not
//  just a generic average. Built on the reusable computeScoreRecovery
//  primitive but expanded into a proper report layer: shot-to-shot recovery,
//  series-level rebound, and independent per-position recovery. Every metric
//  carries an availability / confidence flag; nothing is fabricated for a low
//  sample. Coordinate-dependent metrics (over-correction) additionally require
//  raw coordinates and flag themselves unavailable when those are missing.
// ===========================================================================

// A practical, data-driven classification of the recovery picture (NOT prose).
enum class RecoveryPattern {
    InsufficientData,      // too few poor shots to conclude anything
    GoodRecovery,          // bounces back to baseline quickly and reliably
    SlowRecovery,          // eventually recovers but takes several shots / often misses baseline
    RepeatedErrorPattern,  // a poor shot tends to be followed by another poor shot
    OverCorrectionPattern  // corrects past centre and overshoots to the opposite side
};
std::string toString(RecoveryPattern p);

// Shot-to-shot recovery for ONE dataset (the whole match, or one position).
struct ShotRecoveryReport {
    bool   available   = false;    // >= 1 poor shot that has a following shot
    int    shotCount   = 0;
    double baseline    = 0.0;      // return-to-baseline reference = set average
    double baselineSD  = 0.0;

    // Core rate (single-sourced from the computeScoreRecovery primitive).
    int    badShotCount          = 0;   // poor shots that have a following shot
    int    recoveredNextCount    = 0;   // next shot reached baseline
    int    notRecoveredNextCount = 0;
    double badShotRecoveryRate   = 0.0; // 100 * recoveredNext / bad (== summary.recoveryPercentage)

    // Depth: how many shots to first return to baseline after a poor shot.
    bool   recoveryShotsAvailable   = false;
    double averageRecoveryShots     = 0.0; // averaged over poor shots that DID recover
    int    recoveryShotsSampleCount = 0;
    int    unrecoveredCount         = 0;   // poor shots that never regained baseline before the end

    // Post-error behaviour.
    double postErrorAverage = 0.0;  // avg score of the shot AFTER a poor shot (== summary.averageNextShotAfterPoor)
    double postErrorDelta   = 0.0;  // postErrorAverage - baseline (negative = compounding)
    int    followUpBetterCount  = 0; // next > bad + neutralBand
    int    followUpWorseCount   = 0; // next < bad - neutralBand
    int    followUpNeutralCount = 0; // within +/- neutralBand of the bad shot
    double neutralBand          = 0.0;

    // Dysfunction patterns.
    double repeatedErrorRate  = 0.0; // 100 * (poor followed by poor) / bad
    int    repeatedErrorCount = 0;

    bool   overCorrectionAvailable   = false; // needs coordinates on poor shot + successor
    double overCorrectionRate        = 0.0;   // 100 * opposite-side-overshoot / evaluable poor shots
    int    overCorrectionCount       = 0;
    int    overCorrectionSampleCount = 0;

    // Confidence + interpretation.
    double          recoveryConfidence = 0.0;  // 0..100 from poor-shot sample size
    Severity        recoverySeverity   = Severity::None;
    RecoveryPattern pattern            = RecoveryPattern::InsufficientData;

    // The reusable primitive result kept verbatim (single source of truth).
    RecoverySummary summary;
};

// One series' scoring summary for series-level rebound analysis.
struct SeriesRecoveryPoint {
    int    seriesNumber = 0;
    int    shotCount    = 0;
    double average      = 0.0;
    bool   isDip        = false;   // average below session average by >= dip threshold
};

// Series-level recovery: does a poor series get followed by a better one?
struct SeriesRecoveryReport {
    bool   available      = false; // >= 2 series present
    int    seriesCount    = 0;
    double sessionAverage = 0.0;
    std::vector<SeriesRecoveryPoint> series; // first-appearance order

    int    dipCount          = 0;  // series flagged as a dip
    int    evaluableDipCount = 0;  // dips that have a following series
    int    dipReboundCount   = 0;  // dip followed by a HIGHER series (temporary dip)
    int    dipSustainedCount = 0;  // dip followed by an equal/lower series (sustained degradation)
    double dipReboundRate    = 0.0;// 100 * rebound / evaluable dips
    double averageReboundDelta = 0.0; // avg (nextSeriesAvg - dipSeriesAvg) over evaluable dips
};

// Per-position recovery — each computed independently (never merged).
struct PositionRecoveryEntry {
    PositionType       position = PositionType::Unknown;
    std::string        positionName;
    ShotRecoveryReport recovery;
};

struct RecoveryAnalysis {
    bool available = false;        // there was something to analyse (>= 2 shots)

    ShotRecoveryReport overall;    // whole match, shot-to-shot
    SeriesRecoveryReport series;   // series-level rebound
    std::vector<PositionRecoveryEntry> byPosition; // independent per present position

    // Interpreted metrics in the shared PerformanceMetric language (whole match).
    std::vector<PerformanceMetric> metrics;

    // Cross-position comparison (requires >= 2 positions with available recovery).
    bool comparativeAvailable = false;
    bool hasBestRecovery  = false; PositionType bestRecoveryPosition  = PositionType::Unknown;
    bool hasWorstRecovery = false; PositionType worstRecoveryPosition = PositionType::Unknown;
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
    PositionAnalysis positionAnalysis;   // Section 6 (Phase 8)
    RecoveryAnalysis recoveryAnalysis;   // Section 7 (Phase 9)

    // Sections 8..11 — declared contract, implemented in later phases:
    //   FatigueAnalysis   fatigueAnalysis;
    //   TrainingPriorities trainingPriorities;
    //   CoachConclusion   coachConclusion;
    //   CoachDiaryFields  manualDiaryFields;
};

} // namespace analytics
} // namespace techaim
