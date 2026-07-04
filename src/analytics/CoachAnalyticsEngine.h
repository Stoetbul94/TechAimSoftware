#pragma once

// ============================================================================
//  TechAim Coach Analytics Engine (Phase 1 + Phase 2)
// ----------------------------------------------------------------------------
//  Deterministic, offline, explainable performance analytics for a completed
//  match / training session. No Qt, no QML, no networking, no AI.
//
//  Usage:
//      CoachReportData report = CoachAnalyticsEngine::analyze(shots);
//
//  Layers implemented here:
//    Layer 1  Basic statistics   (mean, median, SD, regression, ...)
//    Layer 2  Shot geometry      (group centre, radius, extreme spread, ...)
//    Milestone: Executive Summary built from the above.
//
//  All helpers are public and static so they can be unit-tested and reused by
//  later phases (distribution, trend, heat map, timing, position, recovery,
//  fatigue, priorities, conclusion).
// ============================================================================

#include <vector>
#include "ShotAnalyticsTypes.h"
#include "CoachReportData.h"

namespace techaim {
namespace analytics {

class CoachAnalyticsEngine {
public:
    struct Options {
        // Include sighters in the official analysis. Off by default: sighters
        // are excluded unless a caller specifically requests them.
        bool includeSighters = false;

        // Consistency is normalized against an "elite" shot-to-shot score SD
        // rather than an arbitrary 100 - SD*100 scale. This value is a tunable
        // reference (decimal-score SD of a strong performance) and should be
        // calibrated per discipline against real elite data. 0.30 is a sane,
        // discipline-neutral starting point.
        double expectedEliteScoreSD = 0.30;

        // Sample statistics (n-1) by default — the standard estimator for a
        // finite sample of shots.
        bool useSampleStatistics = true;

        // ---- Phase 4: shot-distribution thresholds ----
        // Quality-band lower bounds (a shot is in the highest band it reaches).
        double bandPerfectMin    = 10.8;
        double bandExcellentMin  = 10.5;
        double bandGoodMin       = 10.2;
        double bandAcceptableMin = 10.0;
        double bandRecoveryMin   = 9.5;   // below this = Poor

        // Reporting-count thresholds.
        double count10_5Threshold = 10.5;
        double count10_7Threshold = 10.7;
        double count10_8Threshold = 10.8;
        double below10_0Threshold = 10.0;

        // Streak thresholds.
        double streak10_5Threshold = 10.5;  // longest run at or above
        double streak10_7Threshold = 10.7;
        double poorStreakThreshold = 9.5;   // longest run below

        // Histogram bin width (decimal score units).
        double histogramBinWidth = 0.1;

        // ---- Phase 5: trend thresholds ----
        // Late-session drop flag: firstThird - lastThird greater than this.
        double lateSessionDropThreshold = 0.15;
        // Deterioration: rolling-average slope below this AND rolling-SD slope
        // above this (both on the rolling-5 series).
        double deteriorationAvgSlopeMax = 0.0;
        double deteriorationSdSlopeMin  = 0.0;
    };

    // Main entry point. Validates + prepares the shot list, then builds the
    // Executive Summary. Never throws; on bad input returns a CoachReportData
    // with valid == false and an explanatory message.
    static CoachReportData analyze(const std::vector<ShotAnalyticsData>& shots);
    static CoachReportData analyze(const std::vector<ShotAnalyticsData>& shots,
                                   const Options& options);

    // ----------------------------------------------------------------------
    //  Layer 1 — basic statistics (operate on plain value vectors)
    // ----------------------------------------------------------------------
    static double sum(const std::vector<double>& v);
    static double mean(const std::vector<double>& v);
    static double median(std::vector<double> v);                 // copies + sorts
    static double variance(const std::vector<double>& v, bool sample = true);
    static double standardDeviation(const std::vector<double>& v, bool sample = true);
    static double minimum(const std::vector<double>& v);
    static double maximum(const std::vector<double>& v);
    static double range(const std::vector<double>& v);
    static double percentile(std::vector<double> v, double p);   // p in [0,100], linear interp
    static std::vector<double> rollingAverage(const std::vector<double>& v, int window);
    static std::vector<double> rollingStandardDeviation(const std::vector<double>& v, int window);
    // Least-squares slope of y over x; 0 if degenerate.
    static double linearRegressionSlope(const std::vector<double>& xs,
                                        const std::vector<double>& ys);
    // Pearson correlation coefficient; 0 if degenerate.
    static double correlation(const std::vector<double>& xs,
                              const std::vector<double>& ys);

    // Complete-window rolling series (length max(0, n-window+1)); empty if the
    // window cannot fit. Unlike rollingAverage/rollingStandardDeviation above,
    // these never emit partial-window values.
    static std::vector<double> rollingMeanComplete(const std::vector<double>& v, int window);
    static std::vector<double> rollingSdComplete(const std::vector<double>& v, int window);

    // ----------------------------------------------------------------------
    //  Layer 2 — shot geometry (operate on shot lists)
    // ----------------------------------------------------------------------
    static double  distanceFromCentre(double x, double y);       // sqrt(x^2 + y^2)
    static Point2D groupCentre(const std::vector<ShotAnalyticsData>& shots);
    static double  groupRadius(const std::vector<ShotAnalyticsData>& shots);   // mean dist from centre
    static double  groupDiameter(const std::vector<ShotAnalyticsData>& shots); // extreme spread
    static double  horizontalStandardDeviation(const std::vector<ShotAnalyticsData>& shots);
    static double  verticalStandardDeviation(const std::vector<ShotAnalyticsData>& shots);
    static double  horizontalBias(const std::vector<ShotAnalyticsData>& shots); // mean x
    static double  verticalBias(const std::vector<ShotAnalyticsData>& shots);   // mean y

private:
    // Validation + ordering. Returns the shots to analyse and fills the
    // provenance/validation fields on `meta`.
    static std::vector<ShotAnalyticsData> prepareShots(
        const std::vector<ShotAnalyticsData>& raw,
        const Options& options,
        CoachReportData& meta);

    static ExecutiveSummary buildExecutiveSummary(
        const std::vector<ShotAnalyticsData>& shots,
        const Options& options);

    static ShotDistribution buildShotDistribution(
        const std::vector<ShotAnalyticsData>& shots,
        const Options& options);

    static TrendAnalysis buildTrendAnalysis(
        const std::vector<ShotAnalyticsData>& shots,
        const Options& options);

    // Longest consecutive run satisfying a predicate over scores.
    static int longestStreakAtLeast(const std::vector<double>& scores, double threshold);
    static int longestStreakBelow(const std::vector<double>& scores, double threshold);

    // Small utilities.
    static double clampd(double v, double lo, double hi);
    static std::vector<double> scoresOf(const std::vector<ShotAnalyticsData>& shots);
    static double averageScoreOfRange(const std::vector<ShotAnalyticsData>& shots,
                                      int beginIndex, int endIndex); // [begin, end)
};

} // namespace analytics
} // namespace techaim
