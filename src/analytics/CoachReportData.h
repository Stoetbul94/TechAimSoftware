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
#include "ShotAnalyticsTypes.h"

namespace techaim {
namespace analytics {

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

    // Sections 4..11 — declared contract, implemented in later phases:
    //   HeatMapAnalysis   heatMapAnalysis;
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
