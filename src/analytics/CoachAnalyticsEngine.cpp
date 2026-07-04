// ============================================================================
//  TechAim Coach Analytics Engine — implementation (Phase 1 + Phase 2)
//  Standard, well-tested statistical and shot-geometry formulas only.
// ============================================================================

#include "CoachAnalyticsEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace techaim {
namespace analytics {

// ----------------------------------------------------------------------------
//  Enum <-> string
// ----------------------------------------------------------------------------
std::string toString(PositionType p)
{
    switch (p) {
    case PositionType::Prone:     return "prone";
    case PositionType::Kneeling:  return "kneeling";
    case PositionType::Standing:  return "standing";
    case PositionType::AirRifle:  return "airRifle";
    case PositionType::AirPistol: return "airPistol";
    case PositionType::Unknown:   default: return "unknown";
    }
}

PositionType positionFromString(const std::string& s)
{
    if (s == "prone")     return PositionType::Prone;
    if (s == "kneeling")  return PositionType::Kneeling;
    if (s == "standing")  return PositionType::Standing;
    if (s == "airRifle")  return PositionType::AirRifle;
    if (s == "airPistol") return PositionType::AirPistol;
    return PositionType::Unknown;
}

std::string toString(TrendDirection t)
{
    switch (t) {
    case TrendDirection::Improving: return "Improving";
    case TrendDirection::Declining: return "Declining";
    case TrendDirection::Stable:    default: return "Stable";
    }
}

// ----------------------------------------------------------------------------
//  Layer 1 — basic statistics
// ----------------------------------------------------------------------------
double CoachAnalyticsEngine::sum(const std::vector<double>& v)
{
    double s = 0.0;
    for (double x : v) s += x;
    return s;
}

double CoachAnalyticsEngine::mean(const std::vector<double>& v)
{
    if (v.empty()) return 0.0;
    return sum(v) / static_cast<double>(v.size());
}

double CoachAnalyticsEngine::median(std::vector<double> v)
{
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    if (n % 2 == 1) return v[n / 2];
    return 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

double CoachAnalyticsEngine::variance(const std::vector<double>& v, bool sample)
{
    const size_t n = v.size();
    if (n < 2) return 0.0;                 // undefined for < 2; report 0
    const double m = mean(v);
    double acc = 0.0;
    for (double x : v) { const double d = x - m; acc += d * d; }
    const double denom = sample ? static_cast<double>(n - 1)
                                : static_cast<double>(n);
    return acc / denom;
}

double CoachAnalyticsEngine::standardDeviation(const std::vector<double>& v, bool sample)
{
    return std::sqrt(variance(v, sample));
}

double CoachAnalyticsEngine::minimum(const std::vector<double>& v)
{
    if (v.empty()) return 0.0;
    return *std::min_element(v.begin(), v.end());
}

double CoachAnalyticsEngine::maximum(const std::vector<double>& v)
{
    if (v.empty()) return 0.0;
    return *std::max_element(v.begin(), v.end());
}

double CoachAnalyticsEngine::range(const std::vector<double>& v)
{
    if (v.empty()) return 0.0;
    return maximum(v) - minimum(v);
}

// Linear-interpolation percentile (a.k.a. "type 7"): index = p/100 * (n-1).
double CoachAnalyticsEngine::percentile(std::vector<double> v, double p)
{
    if (v.empty()) return 0.0;
    p = clampd(p, 0.0, 100.0);
    std::sort(v.begin(), v.end());
    if (v.size() == 1) return v[0];
    const double rank = (p / 100.0) * static_cast<double>(v.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(rank));
    const size_t hi = static_cast<size_t>(std::ceil(rank));
    const double frac = rank - static_cast<double>(lo);
    return v[lo] + (v[hi] - v[lo]) * frac;
}

// Trailing window: result[i] = mean of v[max(0, i-window+1) .. i].
std::vector<double> CoachAnalyticsEngine::rollingAverage(const std::vector<double>& v, int window)
{
    std::vector<double> out;
    if (v.empty() || window <= 0) return out;
    out.reserve(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        const size_t begin = (i + 1 >= static_cast<size_t>(window))
                                 ? (i + 1 - static_cast<size_t>(window)) : 0;
        double acc = 0.0; size_t cnt = 0;
        for (size_t j = begin; j <= i; ++j) { acc += v[j]; ++cnt; }
        out.push_back(cnt ? acc / static_cast<double>(cnt) : 0.0);
    }
    return out;
}

std::vector<double> CoachAnalyticsEngine::rollingStandardDeviation(const std::vector<double>& v, int window)
{
    std::vector<double> out;
    if (v.empty() || window <= 0) return out;
    out.reserve(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        const size_t begin = (i + 1 >= static_cast<size_t>(window))
                                 ? (i + 1 - static_cast<size_t>(window)) : 0;
        std::vector<double> w(v.begin() + begin, v.begin() + i + 1);
        out.push_back(standardDeviation(w, true));
    }
    return out;
}

double CoachAnalyticsEngine::linearRegressionSlope(const std::vector<double>& xs,
                                                   const std::vector<double>& ys)
{
    const size_t n = xs.size();
    if (n < 2 || ys.size() != n) return 0.0;
    const double mx = mean(xs);
    const double my = mean(ys);
    double num = 0.0, den = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double dx = xs[i] - mx;
        num += dx * (ys[i] - my);
        den += dx * dx;
    }
    if (den == 0.0) return 0.0;            // all x identical => no slope
    return num / den;
}

double CoachAnalyticsEngine::correlation(const std::vector<double>& xs,
                                         const std::vector<double>& ys)
{
    const size_t n = xs.size();
    if (n < 2 || ys.size() != n) return 0.0;
    const double mx = mean(xs);
    const double my = mean(ys);
    double sxy = 0.0, sxx = 0.0, syy = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double dx = xs[i] - mx;
        const double dy = ys[i] - my;
        sxy += dx * dy; sxx += dx * dx; syy += dy * dy;
    }
    if (sxx == 0.0 || syy == 0.0) return 0.0;
    return sxy / std::sqrt(sxx * syy);
}

std::vector<double> CoachAnalyticsEngine::rollingMeanComplete(const std::vector<double>& v, int window)
{
    std::vector<double> out;
    if (window <= 0 || static_cast<int>(v.size()) < window) return out;
    const int n = static_cast<int>(v.size());
    out.reserve(n - window + 1);
    // Sliding sum for O(n).
    double acc = 0.0;
    for (int i = 0; i < window; ++i) acc += v[i];
    out.push_back(acc / window);
    for (int i = window; i < n; ++i) {
        acc += v[i] - v[i - window];
        out.push_back(acc / window);
    }
    return out;
}

std::vector<double> CoachAnalyticsEngine::rollingSdComplete(const std::vector<double>& v, int window)
{
    std::vector<double> out;
    if (window <= 0 || static_cast<int>(v.size()) < window) return out;
    const int n = static_cast<int>(v.size());
    out.reserve(n - window + 1);
    for (int start = 0; start + window <= n; ++start) {
        std::vector<double> w(v.begin() + start, v.begin() + start + window);
        out.push_back(standardDeviation(w, true));   // sample SD within the window
    }
    return out;
}

int CoachAnalyticsEngine::longestStreakAtLeast(const std::vector<double>& scores, double threshold)
{
    int best = 0, cur = 0;
    for (double s : scores) {
        if (s >= threshold) { ++cur; if (cur > best) best = cur; }
        else cur = 0;
    }
    return best;
}

int CoachAnalyticsEngine::longestStreakBelow(const std::vector<double>& scores, double threshold)
{
    int best = 0, cur = 0;
    for (double s : scores) {
        if (s < threshold) { ++cur; if (cur > best) best = cur; }
        else cur = 0;
    }
    return best;
}

// ----------------------------------------------------------------------------
//  Layer 2 — shot geometry
// ----------------------------------------------------------------------------
double CoachAnalyticsEngine::distanceFromCentre(double x, double y)
{
    return std::sqrt(x * x + y * y);
}

Point2D CoachAnalyticsEngine::groupCentre(const std::vector<ShotAnalyticsData>& shots)
{
    Point2D c;
    if (shots.empty()) return c;
    double sx = 0.0, sy = 0.0;
    for (const auto& s : shots) { sx += s.x; sy += s.y; }
    const double n = static_cast<double>(shots.size());
    c.x = sx / n; c.y = sy / n;
    return c;
}

double CoachAnalyticsEngine::groupRadius(const std::vector<ShotAnalyticsData>& shots)
{
    if (shots.empty()) return 0.0;
    const Point2D c = groupCentre(shots);
    double acc = 0.0;
    for (const auto& s : shots)
        acc += distanceFromCentre(s.x - c.x, s.y - c.y);
    return acc / static_cast<double>(shots.size());
}

double CoachAnalyticsEngine::groupDiameter(const std::vector<ShotAnalyticsData>& shots)
{
    // Extreme spread: the largest distance between any two shots.
    const size_t n = shots.size();
    if (n < 2) return 0.0;
    double best = 0.0;
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            const double d = distanceFromCentre(shots[i].x - shots[j].x,
                                                shots[i].y - shots[j].y);
            if (d > best) best = d;
        }
    }
    return best;
}

double CoachAnalyticsEngine::horizontalStandardDeviation(const std::vector<ShotAnalyticsData>& shots)
{
    std::vector<double> xs; xs.reserve(shots.size());
    for (const auto& s : shots) xs.push_back(s.x);
    return standardDeviation(xs, true);
}

double CoachAnalyticsEngine::verticalStandardDeviation(const std::vector<ShotAnalyticsData>& shots)
{
    std::vector<double> ys; ys.reserve(shots.size());
    for (const auto& s : shots) ys.push_back(s.y);
    return standardDeviation(ys, true);
}

double CoachAnalyticsEngine::horizontalBias(const std::vector<ShotAnalyticsData>& shots)
{
    return groupCentre(shots).x;
}

double CoachAnalyticsEngine::verticalBias(const std::vector<ShotAnalyticsData>& shots)
{
    return groupCentre(shots).y;
}

// ----------------------------------------------------------------------------
//  Small utilities
// ----------------------------------------------------------------------------
double CoachAnalyticsEngine::clampd(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

std::vector<double> CoachAnalyticsEngine::scoresOf(const std::vector<ShotAnalyticsData>& shots)
{
    std::vector<double> s; s.reserve(shots.size());
    for (const auto& sh : shots) s.push_back(sh.decimalScore);
    return s;
}

double CoachAnalyticsEngine::averageScoreOfRange(const std::vector<ShotAnalyticsData>& shots,
                                                 int beginIndex, int endIndex)
{
    if (beginIndex < 0) beginIndex = 0;
    if (endIndex > static_cast<int>(shots.size())) endIndex = static_cast<int>(shots.size());
    if (endIndex <= beginIndex) return 0.0;
    double acc = 0.0;
    for (int i = beginIndex; i < endIndex; ++i) acc += shots[i].decimalScore;
    return acc / static_cast<double>(endIndex - beginIndex);
}

// ----------------------------------------------------------------------------
//  Phase 1 — validation + ordering
// ----------------------------------------------------------------------------
std::vector<ShotAnalyticsData> CoachAnalyticsEngine::prepareShots(
    const std::vector<ShotAnalyticsData>& raw,
    const Options& options,
    CoachReportData& meta)
{
    std::vector<ShotAnalyticsData> out;
    meta.inputShotCount   = static_cast<int>(raw.size());
    meta.sightersIncluded = options.includeSighters;

    for (const auto& s : raw) {
        if (!s.isValid) { ++meta.invalidRemovedCount; continue; }
        if (s.isSighter && !options.includeSighters) { ++meta.sighterExcludedCount; continue; }
        out.push_back(s);
    }

    // Ordering: prefer shotNumber (all positive), else timestamp (all present),
    // else keep the caller's order. Stable so ties keep input order.
    bool allShotNumbers = !out.empty();
    for (const auto& s : out) if (s.shotNumber <= 0) { allShotNumbers = false; break; }

    if (allShotNumbers) {
        std::stable_sort(out.begin(), out.end(),
            [](const ShotAnalyticsData& a, const ShotAnalyticsData& b) {
                return a.shotNumber < b.shotNumber;
            });
    } else {
        bool allTimestamps = !out.empty();
        for (const auto& s : out) if (!s.hasTimestamp) { allTimestamps = false; break; }
        if (allTimestamps) {
            std::stable_sort(out.begin(), out.end(),
                [](const ShotAnalyticsData& a, const ShotAnalyticsData& b) {
                    return a.timestamp < b.timestamp;
                });
        }
        // else: missing both -> keep input order (handled gracefully).
    }

    meta.analysedShotCount = static_cast<int>(out.size());
    int comp = 0;
    for (const auto& s : out) if (s.isCompetitionShot) ++comp;
    meta.competitionShotCount = comp;
    meta.lowSampleWarning = meta.analysedShotCount < 10;

    return out;
}

// ----------------------------------------------------------------------------
//  Milestone — Executive Summary
// ----------------------------------------------------------------------------
ExecutiveSummary CoachAnalyticsEngine::buildExecutiveSummary(
    const std::vector<ShotAnalyticsData>& shots,
    const Options& options)
{
    ExecutiveSummary es;
    const int n = static_cast<int>(shots.size());
    es.competitionShotCount = n;
    if (n == 0) return es;

    const std::vector<double> scores = scoresOf(shots);

    // Scoring
    es.totalScore             = sum(scores);
    es.averageScore           = mean(scores);
    es.bestShot               = maximum(scores);
    es.worstShot              = minimum(scores);
    es.scoreStandardDeviation = standardDeviation(scores, options.useSampleStatistics);

    // Consistency: normalized against elite reference SD (tunable), clamped.
    if (options.expectedEliteScoreSD > 0.0) {
        const double c = 100.0 * (1.0 - es.scoreStandardDeviation / options.expectedEliteScoreSD);
        es.consistencyPercentage = clampd(c, 0.0, 100.0);
    } else {
        es.consistencyPercentage = 0.0;
    }

    // Group geometry
    const Point2D centre = groupCentre(shots);
    es.groupCentreX  = centre.x;
    es.groupCentreY  = centre.y;
    es.groupRadius   = groupRadius(shots);
    es.groupDiameter = groupDiameter(shots);
    es.horizontalBias= centre.x;
    es.verticalBias  = centre.y;
    es.horizontalSD  = horizontalStandardDeviation(shots);
    es.verticalSD    = verticalStandardDeviation(shots);

    // Halves ( [0, n/2) vs [n/2, n) )
    const int half = n / 2;
    es.firstHalfAverage  = averageScoreOfRange(shots, 0, half > 0 ? half : n);
    es.secondHalfAverage = averageScoreOfRange(shots, half, n);

    // Thirds ( first floor(n/3) vs last floor(n/3) )
    const int third = n / 3;
    if (third >= 1) {
        es.firstThirdAverage = averageScoreOfRange(shots, 0, third);
        es.lastThirdAverage  = averageScoreOfRange(shots, n - third, n);
    } else {
        // Too few shots for thirds — fall back to whole-session average so the
        // value is defined rather than misleadingly zero.
        es.firstThirdAverage = es.averageScore;
        es.lastThirdAverage  = es.averageScore;
    }

    // Trend: least-squares slope of decimal score vs shot number.
    std::vector<double> xs; xs.reserve(n);
    for (const auto& s : shots) xs.push_back(static_cast<double>(s.shotNumber));
    // If shot numbers are unusable (all zero/equal), fall back to 1..n index.
    bool xsUsable = false;
    for (size_t i = 1; i < xs.size(); ++i) if (xs[i] != xs[0]) { xsUsable = true; break; }
    if (!xsUsable) { xs.clear(); for (int i = 0; i < n; ++i) xs.push_back(i + 1); }

    es.trendSlope = linearRegressionSlope(xs, scores);
    if (es.trendSlope > 0.01)       es.trendDirection = TrendDirection::Improving;
    else if (es.trendSlope < -0.01) es.trendDirection = TrendDirection::Declining;
    else                            es.trendDirection = TrendDirection::Stable;

    return es;
}

// ----------------------------------------------------------------------------
//  Phase 4 — Shot Distribution
// ----------------------------------------------------------------------------
ShotDistribution CoachAnalyticsEngine::buildShotDistribution(
    const std::vector<ShotAnalyticsData>& shots, const Options& o)
{
    ShotDistribution d;
    const int n = static_cast<int>(shots.size());
    d.totalAnalysedShots = n;
    const double bw = (o.histogramBinWidth > 0.0) ? o.histogramBinWidth : 0.1;
    d.binWidth = bw;
    if (n == 0) return d;

    const std::vector<double> scores = scoresOf(shots);
    const double avg = mean(scores);
    const double sd  = standardDeviation(scores, o.useSampleStatistics);

    d.bestShot  = maximum(scores);
    d.worstShot = minimum(scores);

    for (double s : scores) {
        // Quality band — highest band the shot reaches.
        if      (s >= o.bandPerfectMin)    ++d.perfectCount;
        else if (s >= o.bandExcellentMin)  ++d.excellentCount;
        else if (s >= o.bandGoodMin)       ++d.goodCount;
        else if (s >= o.bandAcceptableMin) ++d.acceptableCount;
        else if (s >= o.bandRecoveryMin)   ++d.recoveryCount;
        else                               ++d.poorCount;

        if (s >= o.count10_5Threshold) ++d.countAtLeast10_5;
        if (s >= o.count10_7Threshold) ++d.countAtLeast10_7;
        if (s >= o.count10_8Threshold) ++d.countAtLeast10_8;
        if (s <  o.below10_0Threshold) ++d.countBelow10_0;

        if (s <  avg)      ++d.countBelowAverage;
        if (s <  avg - sd) ++d.countBelowAverageMinusSD;
    }

    const double inv = 100.0 / static_cast<double>(n);
    d.perfectPct    = d.perfectCount    * inv;
    d.excellentPct  = d.excellentCount  * inv;
    d.goodPct       = d.goodCount       * inv;
    d.acceptablePct = d.acceptableCount * inv;
    d.recoveryPct   = d.recoveryCount   * inv;
    d.poorPct       = d.poorCount       * inv;

    d.bestStreak10_5    = longestStreakAtLeast(scores, o.streak10_5Threshold);
    d.bestStreak10_7    = longestStreakAtLeast(scores, o.streak10_7Threshold);
    d.longestPoorStreak = longestStreakBelow(scores, o.poorStreakThreshold);

    // Histogram, aligned to a binWidth grid spanning [worst, best].
    const double eps = 1e-9;
    const double gridStart = std::floor(d.worstShot / bw + eps) * bw;
    int nbins = static_cast<int>(std::floor((d.bestShot - gridStart) / bw + eps)) + 1;
    if (nbins < 1) nbins = 1;
    d.histogram.resize(static_cast<size_t>(nbins));
    for (int b = 0; b < nbins; ++b) {
        d.histogram[b].lowerEdge = gridStart + b * bw;
        d.histogram[b].upperEdge = gridStart + (b + 1) * bw;
        d.histogram[b].count = 0;
    }
    for (double s : scores) {
        int idx = static_cast<int>(std::floor((s - gridStart) / bw + eps));
        if (idx < 0) idx = 0;
        if (idx >= nbins) idx = nbins - 1;
        ++d.histogram[static_cast<size_t>(idx)].count;
    }
    return d;
}

// ----------------------------------------------------------------------------
//  Phase 5 — Trend Analysis
// ----------------------------------------------------------------------------
TrendAnalysis CoachAnalyticsEngine::buildTrendAnalysis(
    const std::vector<ShotAnalyticsData>& shots, const Options& o)
{
    TrendAnalysis t;
    const int n = static_cast<int>(shots.size());
    const std::vector<double> scores = scoresOf(shots);
    t.scores = scores;
    if (n == 0) return t;

    // Rolling complete-window series (no partial/faked windows).
    t.rolling5Average    = rollingMeanComplete(scores, 5);
    t.rolling5SD         = rollingSdComplete(scores, 5);
    t.rolling5Available  = !t.rolling5Average.empty();
    t.rolling10Average   = rollingMeanComplete(scores, 10);
    t.rolling10SD        = rollingSdComplete(scores, 10);
    t.rolling10Available = !t.rolling10Average.empty();

    if (n >= 10) {
        t.first10Available = true; t.first10Average = averageScoreOfRange(shots, 0, 10);
        t.last10Available  = true; t.last10Average  = averageScoreOfRange(shots, n - 10, n);
    }

    if (n >= 2) {
        t.halvesAvailable = true;
        const int half = n / 2;
        t.firstHalfAverage  = averageScoreOfRange(shots, 0, half);
        t.secondHalfAverage = averageScoreOfRange(shots, half, n);
    }

    if (n >= 3) {
        t.thirdsAvailable = true;
        const int third = n / 3;
        t.firstThirdAverage  = averageScoreOfRange(shots, 0, third);
        t.middleThirdAverage = averageScoreOfRange(shots, third, n - third);
        t.lastThirdAverage   = averageScoreOfRange(shots, n - third, n);
    }

    if (n >= 2) {
        std::vector<double> xs; xs.reserve(n);
        for (const auto& s : shots) xs.push_back(static_cast<double>(s.shotNumber));
        bool usable = false;
        for (int i = 1; i < n; ++i) if (xs[i] != xs[0]) { usable = true; break; }
        if (!usable) { xs.clear(); for (int i = 0; i < n; ++i) xs.push_back(i + 1); }
        t.regressionAvailable   = true;
        t.regressionSlope       = linearRegressionSlope(xs, scores);
        t.regressionCorrelation = correlation(xs, scores);
    }

    if (t.regressionSlope > 0.01)       t.trendDirection = TrendDirection::Improving;
    else if (t.regressionSlope < -0.01) t.trendDirection = TrendDirection::Declining;
    else                                t.trendDirection = TrendDirection::Stable;

    if (t.rolling5Available) {
        t.rolling5ExtremesAvailable = true;
        t.highestRolling5Average = maximum(t.rolling5Average);
        t.lowestRolling5Average  = minimum(t.rolling5Average);
    }
    if (t.rolling10Available) {
        t.rolling10ExtremesAvailable = true;
        t.highestRolling10Average = maximum(t.rolling10Average);
        t.lowestRolling10Average  = minimum(t.rolling10Average);
    }

    // Deterioration: rolling-5 average declines while rolling-5 SD rises.
    // Needs at least 2 rolling points (=> n >= 6) to have a slope.
    if (t.rolling5Average.size() >= 2 && t.rolling5SD.size() >= 2) {
        std::vector<double> idxA(t.rolling5Average.size());
        for (size_t i = 0; i < idxA.size(); ++i) idxA[i] = static_cast<double>(i);
        std::vector<double> idxB(t.rolling5SD.size());
        for (size_t i = 0; i < idxB.size(); ++i) idxB[i] = static_cast<double>(i);
        const double avgSlope = linearRegressionSlope(idxA, t.rolling5Average);
        const double sdSlope  = linearRegressionSlope(idxB, t.rolling5SD);
        t.deteriorationDeterminable = true;
        t.deteriorationFlag = (avgSlope < o.deteriorationAvgSlopeMax) &&
                              (sdSlope  > o.deteriorationSdSlopeMin);
    }

    if (t.thirdsAvailable) {
        t.lateSessionDrop     = t.firstThirdAverage - t.lastThirdAverage;
        t.lateSessionDropFlag = t.lateSessionDrop > o.lateSessionDropThreshold;
    }

    return t;
}

// ----------------------------------------------------------------------------
//  Public entry point
// ----------------------------------------------------------------------------
CoachReportData CoachAnalyticsEngine::analyze(const std::vector<ShotAnalyticsData>& shots)
{
    return analyze(shots, Options());
}

CoachReportData CoachAnalyticsEngine::analyze(const std::vector<ShotAnalyticsData>& shots,
                                              const Options& options)
{
    CoachReportData report;

    if (shots.empty()) {
        report.valid = false;
        report.message = "No shots provided.";
        return report;
    }

    const std::vector<ShotAnalyticsData> prepared = prepareShots(shots, options, report);

    if (prepared.empty()) {
        report.valid = false;
        report.message = "No valid competition shots to analyse (all invalid or sighters).";
        return report;
    }

    report.executiveSummary = buildExecutiveSummary(prepared, options);
    report.shotDistribution = buildShotDistribution(prepared, options);
    report.trendAnalysis    = buildTrendAnalysis(prepared, options);
    report.valid = true;
    report.message = report.lowSampleWarning
        ? "Analysed with fewer than 10 shots; results are indicative only."
        : "OK.";
    return report;
}

} // namespace analytics
} // namespace techaim
