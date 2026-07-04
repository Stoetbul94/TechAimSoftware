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

    // Section 1 (implemented)
    ExecutiveSummary executiveSummary;

    // Sections 2..11 — declared contract, implemented in later phases:
    //   ShotDistribution  shotDistribution;
    //   TrendAnalysis     trendAnalysis;
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
