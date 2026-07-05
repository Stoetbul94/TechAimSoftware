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

        // ---- Phase 6: heat-map source data ----
        double heatMapBinSize = 5.0;   // mm per (square) cell
        double heatMapExtent  = 0.0;   // >0 => grid covers [-extent,+extent] both axes; 0 => auto-fit to data
        double heatMapMargin  = 5.0;   // mm padding added around data when auto-fitting
        double heatMapGoodMin = 10.5;  // good-shot heat map: score >=
        double heatMapPoorMax = 10.0;  // poor-shot heat map: score <

        // ---- Phase 7: timing thresholds ----
        double timingRushedFactor  = 0.7;   // interval < factor * median  => rushed
        double timingDelayedFactor = 1.5;   // interval > factor * median  => delayed
        double timingHighThreshold = 10.7;  // decision-time-before "high" shot
        double timingPoorThreshold = 10.0;  // decision-before / recovery-after "poor" shot

        // ---- Phase 8: position analysis ----
        double positionConfidentSampleSize   = 20.0;  // shots for full-confidence assessment
        double positionMaxShotScore          = 10.9;  // for points-lost
        double positionScoreFloor            = 9.0;   // score-component normalisation range
        double positionScoreCeil             = 10.9;
        double positionExpectedGroupRadius   = 15.0;  // mm; geometry-component reference (discipline-tunable)
        double positionTrendScale            = 500.0; // score slope -> component points
        double positionRecoveryPoorThreshold = 10.0;
        double positionOutlierSigma          = 2.0;   // outlier if radial dist > mean + sigma*SD
        // Quality-score weights (renormalised over the components that are available).
        double positionWeightScore       = 0.30;
        double positionWeightConsistency = 0.20;
        double positionWeightGeometry    = 0.20;
        double positionWeightTrend       = 0.10;
        double positionWeightRecovery    = 0.10;
        double positionWeightTiming      = 0.10;
        // Strength / weakness thresholds on 0..100 components.
        double positionStrengthThreshold = 70.0;
        double positionWeaknessThreshold = 45.0;
    };

    // Shared coordinate frame for a set of heat-map grids (so bins align).
    struct HeatMapFrame {
        double originX   = 0.0;   // lower-left corner of cell (0,0)
        double originY   = 0.0;
        double binSize   = 5.0;
        int    gridWidth = 0;
        int    gridHeight= 0;
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

    // Full raw-coordinate statistics (mm) for a shot set — the ground truth
    // reused by heat maps and future geometry algorithms.
    static CoordinateStats computeCoordinateStats(const std::vector<ShotAnalyticsData>& shots);

    // ----------------------------------------------------------------------
    //  Phase 6 — heat-map source data (public for unit testing)
    // ----------------------------------------------------------------------
    // Grid frame spanning all shots (or the configured fixed extent).
    static HeatMapFrame computeHeatMapFrame(const std::vector<ShotAnalyticsData>& allShots,
                                            const Options& options);
    // Density grid for a subset of shots on a shared frame (MPI/radius are
    // computed from the raw coordinates, not the binned cells).
    static HeatMapGrid buildGrid(const std::vector<ShotAnalyticsData>& shots,
                                 const HeatMapFrame& frame);
    // B relative to A (both must share a frame for the dominant-shift to be meaningful).
    static HeatMapComparison compareHeatMaps(const HeatMapGrid& a, const HeatMapGrid& b);

    // ----------------------------------------------------------------------
    //  Phase 7 — timing primitives (public, reusable by later phases)
    // ----------------------------------------------------------------------
    // Intervals between consecutive both-timestamped shots. Empty +
    // available=false when timing data is absent — never fabricated.
    static IntervalSeries extractIntervals(const std::vector<ShotAnalyticsData>& shots);

    // ----------------------------------------------------------------------
    //  Reusable score-recovery primitive (shared by Phase 8 and Phase 9)
    // ----------------------------------------------------------------------
    static RecoverySummary computeScoreRecovery(const std::vector<ShotAnalyticsData>& shots,
                                                double poorThreshold, bool useSampleStatistics);

    // ----------------------------------------------------------------------
    //  Phase 8 — position analysis (public for unit testing)
    // ----------------------------------------------------------------------
    static PositionReport buildPositionReport(const std::vector<ShotAnalyticsData>& positionShots,
                                              PositionType position, const Options& options);

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

    static HeatMapAnalysis buildHeatMapAnalysis(
        const std::vector<ShotAnalyticsData>& shots,
        const Options& options);

    static TimingAnalysis buildTimingAnalysis(
        const std::vector<ShotAnalyticsData>& shots,
        const Options& options);

    static PositionAnalysis buildPositionAnalysis(
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
