// ============================================================================
//  TechAim Coach Analytics Engine — implementation (Phase 1 + Phase 2)
//  Standard, well-tested statistical and shot-geometry formulas only.
// ============================================================================

#include "CoachAnalyticsEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

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

std::string toString(ImpactLevel i)
{
    switch (i) {
    case ImpactLevel::Low:    return "Low";
    case ImpactLevel::Medium: return "Medium";
    case ImpactLevel::High:   default: return "High";
    }
}

std::string toString(PaceTrend p)
{
    switch (p) {
    case PaceTrend::SpeedingUp:  return "SpeedingUp";
    case PaceTrend::SlowingDown: return "SlowingDown";
    case PaceTrend::Steady:      default: return "Steady";
    }
}

std::string toString(Severity s)
{
    switch (s) {
    case Severity::None:     return "None";
    case Severity::Low:      return "Low";
    case Severity::Moderate: return "Moderate";
    case Severity::High:     return "High";
    case Severity::Critical: default: return "Critical";
    }
}

std::string toString(RecoveryPattern p)
{
    switch (p) {
    case RecoveryPattern::GoodRecovery:          return "GoodRecovery";
    case RecoveryPattern::SlowRecovery:          return "SlowRecovery";
    case RecoveryPattern::RepeatedErrorPattern:  return "RepeatedErrorPattern";
    case RecoveryPattern::OverCorrectionPattern: return "OverCorrectionPattern";
    case RecoveryPattern::InsufficientData:      default: return "InsufficientData";
    }
}

std::string toString(FatiguePattern p)
{
    switch (p) {
    case FatiguePattern::NoFatigueDetected:       return "NoFatigueDetected";
    case FatiguePattern::GradualDecline:          return "GradualDecline";
    case FatiguePattern::LateMatchDrop:           return "LateMatchDrop";
    case FatiguePattern::IncreasingDispersion:    return "IncreasingDispersion";
    case FatiguePattern::TimingSlowdown:          return "TimingSlowdown";
    case FatiguePattern::FatigueCompensation:     return "FatigueCompensation";
    case FatiguePattern::PositionSpecificFatigue: return "PositionSpecificFatigue";
    case FatiguePattern::InsufficientData:        default: return "InsufficientData";
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

CoordinateStats CoachAnalyticsEngine::computeCoordinateStats(const std::vector<ShotAnalyticsData>& shots)
{
    CoordinateStats s;
    s.shotCount = static_cast<int>(shots.size());
    if (shots.empty()) return s;
    s.hasData = true;
    s.mpi = groupCentre(shots);
    s.meanRadius   = groupRadius(shots);
    s.groupRadius  = s.meanRadius;           // same measure (project definition)
    s.groupDiameter= groupDiameter(shots);
    s.horizontalSpread = horizontalStandardDeviation(shots);
    s.verticalSpread   = verticalStandardDeviation(shots);
    return s;
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
//  Phase 6 — Heat Map source data
// ----------------------------------------------------------------------------
CoachAnalyticsEngine::HeatMapFrame CoachAnalyticsEngine::computeHeatMapFrame(
    const std::vector<ShotAnalyticsData>& shots, const Options& o)
{
    HeatMapFrame f;
    f.binSize = (o.heatMapBinSize > 0.0) ? o.heatMapBinSize : 5.0;
    const double bs  = f.binSize;
    const double eps = 1e-9;

    // Fixed, symmetric extent requested: cover [-E, +E] on both axes.
    if (o.heatMapExtent > 0.0) {
        const double E = o.heatMapExtent;
        f.originX = std::floor((-E) / bs + eps) * bs;
        f.originY = std::floor((-E) / bs + eps) * bs;
        f.gridWidth  = static_cast<int>(std::floor((E - f.originX) / bs + eps)) + 1;
        f.gridHeight = static_cast<int>(std::floor((E - f.originY) / bs + eps)) + 1;
        return f;
    }

    if (shots.empty()) return f;   // gridWidth/Height stay 0

    double minX = shots[0].x, maxX = shots[0].x;
    double minY = shots[0].y, maxY = shots[0].y;
    for (const auto& s : shots) {
        minX = std::min(minX, s.x); maxX = std::max(maxX, s.x);
        minY = std::min(minY, s.y); maxY = std::max(maxY, s.y);
    }
    const double m = std::max(0.0, o.heatMapMargin);
    minX -= m; maxX += m; minY -= m; maxY += m;

    f.originX = std::floor(minX / bs + eps) * bs;
    f.originY = std::floor(minY / bs + eps) * bs;
    f.gridWidth  = static_cast<int>(std::floor((maxX - f.originX) / bs + eps)) + 1;
    f.gridHeight = static_cast<int>(std::floor((maxY - f.originY) / bs + eps)) + 1;
    if (f.gridWidth  < 1) f.gridWidth  = 1;
    if (f.gridHeight < 1) f.gridHeight = 1;
    return f;
}

HeatMapGrid CoachAnalyticsEngine::buildGrid(const std::vector<ShotAnalyticsData>& shots,
                                            const HeatMapFrame& frame)
{
    HeatMapGrid g;
    g.binSize    = frame.binSize;
    g.originX    = frame.originX;
    g.originY    = frame.originY;
    g.gridWidth  = frame.gridWidth;
    g.gridHeight = frame.gridHeight;
    g.shotCount  = static_cast<int>(shots.size());
    if (shots.empty() || frame.gridWidth <= 0 || frame.gridHeight <= 0) return g;
    g.hasData = true;

    const double bs  = frame.binSize;
    const double eps = 1e-9;

    // Ground-truth geometry from raw coordinates (not binned).
    g.stats = computeCoordinateStats(shots);

    struct Acc { int count = 0; double scoreSum = 0.0; std::vector<int> shotNums; };
    std::vector<Acc> acc(static_cast<size_t>(frame.gridWidth) * frame.gridHeight);

    for (const auto& s : shots) {
        int bx = static_cast<int>(std::floor((s.x - frame.originX) / bs + eps));
        int by = static_cast<int>(std::floor((s.y - frame.originY) / bs + eps));
        if (bx < 0) bx = 0;
        if (bx >= frame.gridWidth)  bx = frame.gridWidth  - 1;
        if (by < 0) by = 0;
        if (by >= frame.gridHeight) by = frame.gridHeight - 1;
        const size_t idx = static_cast<size_t>(by) * frame.gridWidth + bx;
        acc[idx].count++;
        acc[idx].scoreSum += s.decimalScore;
        acc[idx].shotNums.push_back(s.shotNumber);
    }

    // Emit non-empty cells in (binY, binX) order; track dominant (first max = tie-break).
    int    bestCount = 0;
    size_t bestIdx   = 0;
    bool   anyDom    = false;
    for (int by = 0; by < frame.gridHeight; ++by) {
        for (int bx = 0; bx < frame.gridWidth; ++bx) {
            const size_t idx = static_cast<size_t>(by) * frame.gridWidth + bx;
            if (acc[idx].count == 0) continue;
            HeatMapCell cell;
            cell.binX = bx; cell.binY = by;
            cell.centreX = frame.originX + (bx + 0.5) * bs;
            cell.centreY = frame.originY + (by + 0.5) * bs;
            cell.count = acc[idx].count;
            cell.averageScore = acc[idx].scoreSum / acc[idx].count;
            cell.shotNumbers = acc[idx].shotNums;
            g.cells.push_back(cell);
            if (acc[idx].count > g.maxCellDensity) g.maxCellDensity = acc[idx].count;
            if (acc[idx].count > bestCount) { bestCount = acc[idx].count; bestIdx = idx; anyDom = true; }
        }
    }

    if (anyDom) {
        g.hasDominantZone = true;
        const int by = static_cast<int>(bestIdx / frame.gridWidth);
        const int bx = static_cast<int>(bestIdx % frame.gridWidth);
        g.dominantBinX = bx; g.dominantBinY = by;
        g.dominantCentreX = frame.originX + (bx + 0.5) * bs;
        g.dominantCentreY = frame.originY + (by + 0.5) * bs;
        g.dominantCount = bestCount;
        g.dominantSharePct = 100.0 * bestCount / static_cast<double>(g.shotCount);
    }
    return g;
}

HeatMapComparison CoachAnalyticsEngine::compareHeatMaps(const HeatMapGrid& a, const HeatMapGrid& b)
{
    HeatMapComparison c;
    if (!a.hasData || !b.hasData) return c;
    c.available      = true;
    c.shiftX         = b.stats.mpi.x - a.stats.mpi.x;
    c.shiftY         = b.stats.mpi.y - a.stats.mpi.y;
    c.shiftDistance  = std::sqrt(c.shiftX * c.shiftX + c.shiftY * c.shiftY);
    c.radiusChange   = b.stats.meanRadius - a.stats.meanRadius;
    c.radiusChangePct= (std::fabs(a.stats.meanRadius) > 1e-9)
                         ? (c.radiusChange / a.stats.meanRadius * 100.0) : 0.0;
    if (a.hasDominantZone && b.hasDominantZone) {
        c.dominantShiftX = b.dominantCentreX - a.dominantCentreX;
        c.dominantShiftY = b.dominantCentreY - a.dominantCentreY;
    }
    return c;
}

HeatMapAnalysis CoachAnalyticsEngine::buildHeatMapAnalysis(
    const std::vector<ShotAnalyticsData>& shots, const Options& o)
{
    HeatMapAnalysis h;
    const int n = static_cast<int>(shots.size());
    h.binSize = (o.heatMapBinSize > 0.0) ? o.heatMapBinSize : 5.0;
    if (n == 0) return h;
    h.available = true;

    const HeatMapFrame frame = computeHeatMapFrame(shots, o);
    h.binSize = frame.binSize;

    auto slice = [&](int begin, int end) {
        if (begin < 0) begin = 0;
        if (end > n)   end = n;
        std::vector<ShotAnalyticsData> v;
        for (int i = begin; i < end; ++i) v.push_back(shots[i]);
        return v;
    };
    auto byPos = [&](PositionType p) {
        std::vector<ShotAnalyticsData> v;
        for (const auto& s : shots) if (s.positionType == p) v.push_back(s);
        return v;
    };

    h.allShots = buildGrid(shots, frame);

    const int half = n / 2;
    h.firstHalf  = buildGrid(slice(0, half), frame);
    h.secondHalf = buildGrid(slice(half, n), frame);

    const int third = n / 3;
    if (third >= 1) {
        h.firstThird  = buildGrid(slice(0, third), frame);
        h.middleThird = buildGrid(slice(third, n - third), frame);
        h.lastThird   = buildGrid(slice(n - third, n), frame);
    }

    std::vector<ShotAnalyticsData> good, poor;
    for (const auto& s : shots) {
        if (s.decimalScore >= o.heatMapGoodMin) good.push_back(s);
        if (s.decimalScore <  o.heatMapPoorMax) poor.push_back(s);
    }
    h.goodShots = buildGrid(good, frame);
    h.poorShots = buildGrid(poor, frame);

    h.prone           = buildGrid(byPos(PositionType::Prone),     frame);
    h.kneeling        = buildGrid(byPos(PositionType::Kneeling),  frame);
    h.standing        = buildGrid(byPos(PositionType::Standing),  frame);
    h.airRifle        = buildGrid(byPos(PositionType::AirRifle),  frame);
    h.airPistol       = buildGrid(byPos(PositionType::AirPistol), frame);
    h.unknownPosition = buildGrid(byPos(PositionType::Unknown),   frame);

    h.firstToSecondHalf = compareHeatMaps(h.firstHalf, h.secondHalf);
    h.firstToLastThird  = compareHeatMaps(h.firstThird, h.lastThird);
    h.horizontalDriftHalves = h.firstToSecondHalf.shiftX;
    h.verticalDriftHalves   = h.firstToSecondHalf.shiftY;
    h.horizontalDriftThirds = h.firstToLastThird.shiftX;
    h.verticalDriftThirds   = h.firstToLastThird.shiftY;

    return h;
}

// ----------------------------------------------------------------------------
//  Phase 7 — Timing Analysis
// ----------------------------------------------------------------------------
IntervalSeries CoachAnalyticsEngine::extractIntervals(const std::vector<ShotAnalyticsData>& shots)
{
    IntervalSeries is;
    for (const auto& s : shots) if (s.hasTimestamp) ++is.timedShotCount;

    const int n = static_cast<int>(shots.size());
    for (int i = 1; i < n; ++i) {
        if (!shots[i].hasTimestamp || !shots[i - 1].hasTimestamp) continue;
        const double dt = shots[i].timestamp - shots[i - 1].timestamp;
        if (dt < 0.0) continue;   // non-monotonic timestamps: skip rather than fabricate
        ShotInterval iv;
        iv.fromIndex = i - 1;
        iv.toIndex   = i;
        iv.fromShotNumber = shots[i - 1].shotNumber;
        iv.toShotNumber   = shots[i].shotNumber;
        iv.seconds = dt;
        is.intervals.push_back(iv);
        is.seconds.push_back(dt);
    }
    is.available = !is.intervals.empty();
    return is;
}

TimingAnalysis CoachAnalyticsEngine::buildTimingAnalysis(
    const std::vector<ShotAnalyticsData>& shots, const Options& o)
{
    TimingAnalysis t;
    const IntervalSeries is = extractIntervals(shots);
    t.timedShotCount = is.timedShotCount;
    t.intervalCount  = static_cast<int>(is.intervals.size());
    if (!is.available) return t;   // no timing data -> everything stays unavailable

    t.available = true;
    t.intervals = is.seconds;
    const std::vector<double>& secs = is.seconds;

    // Basic stats (>= 1 interval).
    t.statsAvailable   = true;
    t.averageInterval  = mean(secs);
    t.medianInterval   = median(secs);
    t.intervalSD       = standardDeviation(secs, o.useSampleStatistics);
    t.fastestInterval  = minimum(secs);
    t.slowestInterval  = maximum(secs);

    // Rolling interval series (complete windows only).
    t.rolling3Average = rollingMeanComplete(secs, 3);
    t.rolling3SD      = rollingSdComplete(secs, 3);
    t.rolling3Available = !t.rolling3Average.empty();
    t.rolling5Average = rollingMeanComplete(secs, 5);
    t.rolling5SD      = rollingSdComplete(secs, 5);
    t.rolling5Available = !t.rolling5Average.empty();

    // Rushed / delayed relative to the median.
    const double med = t.medianInterval;
    if (med > 0.0) {
        for (const auto& iv : is.intervals) {
            if (iv.seconds < o.timingRushedFactor * med) {
                ++t.rushedShotCount;  t.rushedShotNumbers.push_back(iv.toShotNumber);
            }
            if (iv.seconds > o.timingDelayedFactor * med) {
                ++t.delayedShotCount; t.delayedShotNumbers.push_back(iv.toShotNumber);
            }
        }
    }

    // Rhythm consistency (needs a spread => >= 2 intervals, positive median).
    if (t.intervalCount >= 2 && med > 0.0) {
        t.rhythmAvailable = true;
        t.rhythmConsistency = clampd(100.0 - (t.intervalSD / med) * 100.0, 0.0, 100.0);
    }

    // Interval trend + score-vs-interval correlation (>= 2 gaps).
    if (t.intervalCount >= 2) {
        std::vector<double> idx(secs.size());
        for (size_t i = 0; i < idx.size(); ++i) idx[i] = static_cast<double>(i);
        t.trendAvailable = true;
        t.intervalRegressionSlope       = linearRegressionSlope(idx, secs);
        t.intervalRegressionCorrelation = correlation(idx, secs);
        if (t.intervalRegressionSlope > 1e-9)       t.intervalTrend = PaceTrend::SlowingDown;
        else if (t.intervalRegressionSlope < -1e-9) t.intervalTrend = PaceTrend::SpeedingUp;
        else                                        t.intervalTrend = PaceTrend::Steady;

        std::vector<double> laterScores;
        laterScores.reserve(is.intervals.size());
        for (const auto& iv : is.intervals) laterScores.push_back(shots[iv.toIndex].decimalScore);
        t.scoreIntervalCorrelationAvailable = true;
        t.scoreVsIntervalCorrelation = correlation(secs, laterScores);
    }

    // Reset time: intervals whose EARLIER shot was poor.
    {
        std::vector<double> recov;
        for (const auto& iv : is.intervals)
            if (shots[iv.fromIndex].decimalScore < o.timingPoorThreshold) recov.push_back(iv.seconds);
        if (!recov.empty()) {
            t.recoveryIntervalAvailable   = true;
            t.recoveryIntervalSampleCount = static_cast<int>(recov.size());
            t.averageRecoveryInterval     = mean(recov);
        }
    }
    // Decision time before high-value shots: intervals whose LATER shot >= high.
    {
        std::vector<double> dh;
        for (const auto& iv : is.intervals)
            if (shots[iv.toIndex].decimalScore >= o.timingHighThreshold) dh.push_back(iv.seconds);
        if (!dh.empty()) {
            t.decisionBeforeHighAvailable   = true;
            t.decisionBeforeHighSampleCount = static_cast<int>(dh.size());
            t.averageDecisionTimeBeforeHigh = mean(dh);
        }
    }
    // Decision time before poor shots: intervals whose LATER shot < poor.
    {
        std::vector<double> dp;
        for (const auto& iv : is.intervals)
            if (shots[iv.toIndex].decimalScore < o.timingPoorThreshold) dp.push_back(iv.seconds);
        if (!dp.empty()) {
            t.decisionBeforePoorAvailable   = true;
            t.decisionBeforePoorSampleCount = static_cast<int>(dp.size());
            t.averageDecisionTimeBeforePoor = mean(dp);
        }
    }

    return t;
}

// ----------------------------------------------------------------------------
//  Reusable score-recovery primitive (Phase 8 + future Phase 9)
// ----------------------------------------------------------------------------
RecoverySummary CoachAnalyticsEngine::computeScoreRecovery(
    const std::vector<ShotAnalyticsData>& shots, double poorThreshold, bool useSample)
{
    RecoverySummary r;
    const int n = static_cast<int>(shots.size());
    if (n < 2) return r;

    std::vector<double> scores; scores.reserve(n);
    for (const auto& s : shots) scores.push_back(s.decimalScore);
    const double avg = mean(scores);
    const double sd  = standardDeviation(scores, useSample);

    double nextSum = 0.0;
    for (int i = 0; i + 1 < n; ++i) {           // last shot cannot be "recovered from"
        const bool poor = (scores[i] < poorThreshold) || (scores[i] < avg - sd);
        if (!poor) continue;
        ++r.poorShotCount;
        const double next = scores[i + 1];
        nextSum += next;
        if (next >= avg) ++r.successfulRecoveryCount; else ++r.failedRecoveryCount;
    }
    if (r.poorShotCount > 0) {
        r.available = true;
        r.recoveryPercentage       = 100.0 * r.successfulRecoveryCount / r.poorShotCount;
        r.averageNextShotAfterPoor = nextSum / r.poorShotCount;
    }
    return r;
}

// ----------------------------------------------------------------------------
//  Phase 8 — Position Analysis (file-local helpers)
// ----------------------------------------------------------------------------
namespace {

std::string fnum(double v, int prec)
{
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", prec, v);
    return std::string(buf);
}

// Component is 0..100 where higher = better; map its LOW-ness to a severity.
Severity severityFromComponent(double component)
{
    if (component >= 70.0) return Severity::None;
    if (component >= 55.0) return Severity::Low;
    if (component >= 40.0) return Severity::Moderate;
    if (component >= 25.0) return Severity::High;
    return Severity::Critical;
}

// 8-way compass label for an average error vector (mm); +x=right, +y=high.
std::string outlierDirection(double dx, double dy)
{
    if (std::fabs(dx) < 1e-9 && std::fabs(dy) < 1e-9) return "centre";
    double ang = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846; // (-180,180]
    if (ang < 0) ang += 360.0;                                        // [0,360)
    if (ang < 22.5  || ang >= 337.5) return "right";
    if (ang < 67.5)  return "high-right";
    if (ang < 112.5) return "high";
    if (ang < 157.5) return "high-left";
    if (ang < 202.5) return "left";
    if (ang < 247.5) return "low-left";
    if (ang < 292.5) return "low";
    return "low-right";
}

template <typename ValueFn, typename AvailFn>
std::vector<PositionRankingEntry> rankPositions(const std::vector<PositionReport>& reports,
                                                ValueFn valueOf, AvailFn availOf, bool descending)
{
    std::vector<PositionRankingEntry> v;
    for (const auto& r : reports) {
        if (!availOf(r)) continue;
        PositionRankingEntry e;
        e.position = r.position; e.positionName = r.positionName; e.value = valueOf(r);
        v.push_back(e);
    }
    std::stable_sort(v.begin(), v.end(),
        [descending](const PositionRankingEntry& a, const PositionRankingEntry& b) {
            return descending ? (a.value > b.value) : (a.value < b.value);
        });
    return v;
}

} // anonymous namespace

PositionReport CoachAnalyticsEngine::buildPositionReport(
    const std::vector<ShotAnalyticsData>& shots, PositionType position, const Options& o)
{
    PositionReport pr;
    pr.position     = position;
    pr.positionName = toString(position);
    pr.shotCount    = static_cast<int>(shots.size());
    if (shots.empty()) return pr;
    pr.available = true;

    const double refN = (o.positionConfidentSampleSize > 0.0) ? o.positionConfidentSampleSize : 1.0;
    pr.confidence = clampd(100.0 * pr.shotCount / refN, 0.0, 100.0);

    PositionMeasurements& m = pr.measurements;
    m.available = true;
    m.shotCount = pr.shotCount;
    const int n = pr.shotCount;

    // ---- Scoring measurements ----
    const std::vector<double> scores = scoresOf(shots);
    m.totalScore   = sum(scores);
    m.averageScore = mean(scores);
    m.medianScore  = median(scores);
    m.minScore     = minimum(scores);
    m.maxScore     = maximum(scores);
    m.scoreSD      = standardDeviation(scores, o.useSampleStatistics);
    m.scoreRange   = range(scores);
    m.scoreVariance= variance(scores, o.useSampleStatistics);
    m.coefficientOfVariation = (std::fabs(m.averageScore) > 1e-9) ? (m.scoreSD / m.averageScore) : 0.0;
    int intCount = 0; double intSum = 0.0;
    for (const auto& s : shots) if (s.hasIntegerScore) { ++intCount; intSum += s.integerScore; }
    if (intCount > 0) { m.hasIntegerScores = true; m.averageIntegerScore = intSum / intCount; }

    // ---- Geometry measurements (raw coords) ----
    m.geometry        = computeCoordinateStats(shots);
    m.horizontalBias  = m.geometry.mpi.x;
    m.verticalBias    = m.geometry.mpi.y;
    m.dispersionRatio = (std::fabs(m.geometry.verticalSpread) > 1e-9)
                          ? (m.geometry.horizontalSpread / m.geometry.verticalSpread) : 0.0;

    // ---- Trend measurements ----
    if (n >= 2) {
        std::vector<double> xs; xs.reserve(n);
        for (const auto& s : shots) xs.push_back(static_cast<double>(s.shotNumber));
        bool usable = false; for (int i = 1; i < n; ++i) if (xs[i] != xs[0]) { usable = true; break; }
        if (!usable) { xs.clear(); for (int i = 0; i < n; ++i) xs.push_back(i + 1); }
        m.trendAvailable   = true;
        m.trendSlope       = linearRegressionSlope(xs, scores);
        m.trendCorrelation = correlation(xs, scores);
        m.halvesAvailable  = true;
        const int half = n / 2;
        m.firstHalfAverage  = averageScoreOfRange(shots, 0, half);
        m.secondHalfAverage = averageScoreOfRange(shots, half, n);
    }
    if (n >= 3) {
        m.thirdsAvailable = true;
        const int third = n / 3;
        m.firstThirdAverage  = averageScoreOfRange(shots, 0, third);
        m.middleThirdAverage = averageScoreOfRange(shots, third, n - third);
        m.lastThirdAverage   = averageScoreOfRange(shots, n - third, n);
    }

    // ---- Timing measurements (reuse the timing engine on this position) ----
    const TimingAnalysis ta = buildTimingAnalysis(shots, o);
    if (ta.available) {
        m.timingAvailable   = true;
        m.averageInterval   = ta.averageInterval;
        m.rhythmConsistency = ta.rhythmConsistency;   // 0 unless rhythmAvailable
        m.rushedShotCount   = ta.rushedShotCount;
        m.delayedShotCount  = ta.delayedShotCount;
    }

    // ---- Recovery (reuse the recovery primitive) ----
    pr.recovery = computeScoreRecovery(shots, o.positionRecoveryPoorThreshold, o.useSampleStatistics);

    // ---- Outliers (spatial, radial distance from MPI) ----
    if (n >= 3) {
        std::vector<double> dists; dists.reserve(n);
        for (const auto& s : shots)
            dists.push_back(distanceFromCentre(s.x - m.geometry.mpi.x, s.y - m.geometry.mpi.y));
        const double dMean = mean(dists);
        const double dSD   = standardDeviation(dists, o.useSampleStatistics);
        const double thr   = dMean + o.positionOutlierSigma * dSD;
        double sumOutDist = 0.0, sumDx = 0.0, sumDy = 0.0;
        for (int i = 0; i < n; ++i) {
            if (dists[i] > thr) {
                ++m.outlierCount; sumOutDist += dists[i];
                sumDx += shots[i].x - m.geometry.mpi.x;
                sumDy += shots[i].y - m.geometry.mpi.y;
            }
        }
        if (m.outlierCount > 0) {
            m.outlierPercentage      = 100.0 * m.outlierCount / n;
            m.averageOutlierDistance = sumOutDist / m.outlierCount;
            m.dominantOutlierDirection = outlierDirection(sumDx / m.outlierCount, sumDy / m.outlierCount);
        }
    }

    // ---- Quality components (0..100, higher = better) ----
    if (n >= 1) {
        pr.scoreComponentAvailable = true;
        const double denom = o.positionScoreCeil - o.positionScoreFloor;
        pr.scoreComponent = clampd((denom > 1e-9)
            ? (m.averageScore - o.positionScoreFloor) / denom * 100.0 : 0.0, 0.0, 100.0);
    }
    if (n >= 2 && o.expectedEliteScoreSD > 0.0) {
        pr.consistencyComponentAvailable = true;
        pr.consistencyComponent = clampd(100.0 * (1.0 - m.scoreSD / o.expectedEliteScoreSD), 0.0, 100.0);
    }
    if (n >= 2 && o.positionExpectedGroupRadius > 0.0) {
        pr.geometryComponentAvailable = true;
        pr.geometryComponent = clampd(100.0 * (1.0 - m.geometry.groupRadius / o.positionExpectedGroupRadius), 0.0, 100.0);
    }
    if (m.trendAvailable) {
        pr.trendComponentAvailable = true;
        pr.trendComponent = clampd(50.0 + m.trendSlope * o.positionTrendScale, 0.0, 100.0);
    }
    if (pr.recovery.available) {
        pr.recoveryComponentAvailable = true;
        pr.recoveryComponent = clampd(pr.recovery.recoveryPercentage, 0.0, 100.0);
    }
    if (ta.rhythmAvailable) {
        pr.timingComponentAvailable = true;
        pr.timingComponent = clampd(ta.rhythmConsistency, 0.0, 100.0);
    }

    // Weighted quality over the AVAILABLE components (weights renormalised).
    double wsum = 0.0, vsum = 0.0;
    auto acc = [&](bool avail, double comp, double w) { if (avail) { wsum += w; vsum += w * comp; } };
    acc(pr.scoreComponentAvailable,       pr.scoreComponent,       o.positionWeightScore);
    acc(pr.consistencyComponentAvailable, pr.consistencyComponent, o.positionWeightConsistency);
    acc(pr.geometryComponentAvailable,    pr.geometryComponent,    o.positionWeightGeometry);
    acc(pr.trendComponentAvailable,       pr.trendComponent,       o.positionWeightTrend);
    acc(pr.recoveryComponentAvailable,    pr.recoveryComponent,    o.positionWeightRecovery);
    acc(pr.timingComponentAvailable,      pr.timingComponent,      o.positionWeightTiming);
    if (wsum > 0.0) { pr.qualityAvailable = true; pr.qualityScore = vsum / wsum; }

    pr.pointsLost = pr.shotCount * o.positionMaxShotScore - m.totalScore;

    // ---- Derived performance metrics + strengths/weaknesses (raw info only) ----
    auto addMetric = [&](const std::string& name, const std::string& key,
                         bool avail, double component, const std::string& evidenceText) {
        PerformanceMetric pm;
        pm.name = name; pm.value = component; pm.availability = avail; pm.confidence = pr.confidence;
        if (avail) {
            pm.severity = severityFromComponent(component);
            pm.addEvidence(evidenceText, key, component);
            if (component >= o.positionStrengthThreshold) {
                pr.strengths.emplace_back(pr.positionName + " " + name + ": " + evidenceText, key, component);
                pr.linkedMetrics.push_back(key);
            } else if (component <= o.positionWeaknessThreshold) {
                pr.weaknesses.emplace_back(pr.positionName + " " + name + ": " + evidenceText, key, component);
                pr.linkedMetrics.push_back(key);
            }
        }
        pr.metrics.push_back(pm);
    };

    addMetric("Score level", "position.score", pr.scoreComponentAvailable, pr.scoreComponent,
              "average score " + fnum(m.averageScore, 2));
    addMetric("Consistency", "position.consistency", pr.consistencyComponentAvailable, pr.consistencyComponent,
              "score SD " + fnum(m.scoreSD, 3));
    addMetric("Group tightness", "position.groupRadius", pr.geometryComponentAvailable, pr.geometryComponent,
              "group radius " + fnum(m.geometry.groupRadius, 2) + " mm");
    addMetric("Trend", "position.trend", pr.trendComponentAvailable, pr.trendComponent,
              "score slope " + fnum(m.trendSlope, 3) + "/shot");
    addMetric("Recovery", "position.recovery", pr.recoveryComponentAvailable, pr.recoveryComponent,
              "recovered " + fnum(pr.recovery.recoveryPercentage, 0) + "% of "
              + std::to_string(pr.recovery.poorShotCount) + " poor shots");
    addMetric("Cadence stability", "position.rhythm", pr.timingComponentAvailable, pr.timingComponent,
              "rhythm " + fnum(m.rhythmConsistency, 1));

    return pr;
}

PositionAnalysis CoachAnalyticsEngine::buildPositionAnalysis(
    const std::vector<ShotAnalyticsData>& shots, const Options& o)
{
    PositionAnalysis pa;

    // Independent datasets, deterministic enum order.
    static const PositionType order[] = {
        PositionType::Prone, PositionType::Kneeling, PositionType::Standing,
        PositionType::AirRifle, PositionType::AirPistol, PositionType::Unknown
    };
    for (PositionType p : order) {
        std::vector<ShotAnalyticsData> sub;
        for (const auto& s : shots) if (s.positionType == p) sub.push_back(s);
        if (sub.empty()) continue;
        pa.positions.push_back(buildPositionReport(sub, p, o));
    }
    pa.positionsPresent = static_cast<int>(pa.positions.size());
    pa.available = pa.positionsPresent >= 1;
    if (!pa.available) return pa;

    pa.pointsLostRanking = rankPositions(pa.positions,
        [](const PositionReport& r){ return r.pointsLost; },
        [](const PositionReport&){ return true; }, true);
    pa.consistencyRanking = rankPositions(pa.positions,
        [](const PositionReport& r){ return r.consistencyComponent; },
        [](const PositionReport& r){ return r.consistencyComponentAvailable; }, true);
    pa.geometryRanking = rankPositions(pa.positions,
        [](const PositionReport& r){ return r.measurements.geometry.groupRadius; },
        [](const PositionReport& r){ return r.geometryComponentAvailable; }, false);
    pa.trendRanking = rankPositions(pa.positions,
        [](const PositionReport& r){ return r.measurements.trendSlope; },
        [](const PositionReport& r){ return r.measurements.trendAvailable; }, true);
    pa.recoveryRanking = rankPositions(pa.positions,
        [](const PositionReport& r){ return r.recovery.recoveryPercentage; },
        [](const PositionReport& r){ return r.recovery.available; }, true);
    pa.timingRanking = rankPositions(pa.positions,
        [](const PositionReport& r){ return r.timingComponent; },
        [](const PositionReport& r){ return r.timingComponentAvailable; }, true);
    pa.overallRanking = rankPositions(pa.positions,
        [](const PositionReport& r){ return r.qualityScore; },
        [](const PositionReport& r){ return r.qualityAvailable; }, true);

    // Comparative extremes require >= 2 positions.
    if (pa.positionsPresent >= 2) {
        pa.comparativeAvailable = true;
        if (pa.overallRanking.size() >= 2) {
            pa.hasStrongest = true; pa.strongestPosition = pa.overallRanking.front().position;
            pa.hasWeakest   = true; pa.weakestPosition   = pa.overallRanking.back().position;
        }
        if (pa.consistencyRanking.size() >= 2) {
            pa.hasMostConsistent  = true; pa.mostConsistentPosition  = pa.consistencyRanking.front().position;
            pa.hasLeastConsistent = true; pa.leastConsistentPosition = pa.consistencyRanking.back().position;
        }
        if (pa.geometryRanking.size() >= 2) {
            pa.hasSmallestGroup = true; pa.smallestGroupPosition = pa.geometryRanking.front().position;
            pa.hasLargestGroup  = true; pa.largestGroupPosition  = pa.geometryRanking.back().position;
        }
        if (pa.trendRanking.size() >= 2) {
            pa.hasGreatestImprovement   = true; pa.greatestImprovementPosition   = pa.trendRanking.front().position;
            pa.hasGreatestDeterioration = true; pa.greatestDeteriorationPosition = pa.trendRanking.back().position;
        }
    }
    return pa;
}

// ----------------------------------------------------------------------------
//  Phase 9 — Recovery Analysis
// ----------------------------------------------------------------------------
//  Built on the computeScoreRecovery primitive but expanded into a full report
//  layer. The poor-shot definition is single-sourced with the primitive
//  (score < poorThreshold OR score < set-average - set-SD) so the headline rate
//  matches summary.recoveryPercentage exactly.
ShotRecoveryReport CoachAnalyticsEngine::buildShotRecovery(
    const std::vector<ShotAnalyticsData>& shots, const Options& o)
{
    ShotRecoveryReport r;
    const int n = static_cast<int>(shots.size());
    r.shotCount   = n;
    r.neutralBand = o.recoveryNeutralBand;
    if (n < 2) return r;

    std::vector<double> scores; scores.reserve(n);
    for (const auto& s : shots) scores.push_back(s.decimalScore);
    const double avg = mean(scores);
    const double sd  = standardDeviation(scores, o.useSampleStatistics);
    r.baseline   = avg;
    r.baselineSD = sd;

    // Single-sourced core rate from the reusable primitive.
    r.summary = computeScoreRecovery(shots, o.recoveryPoorThreshold, o.useSampleStatistics);

    auto isPoor = [&](double sc){ return (sc < o.recoveryPoorThreshold) || (sc < avg - sd); };

    double recShotsSum = 0.0;
    for (int i = 0; i + 1 < n; ++i) {         // last shot cannot be recovered from
        if (!isPoor(scores[i])) continue;
        ++r.badShotCount;
        const double next = scores[i + 1];

        // Next-shot recovery (baseline = set average).
        if (next >= avg) ++r.recoveredNextCount; else ++r.notRecoveredNextCount;

        // Follow-up quality relative to the poor shot itself.
        if (next > scores[i] + o.recoveryNeutralBand)      ++r.followUpBetterCount;
        else if (next < scores[i] - o.recoveryNeutralBand) ++r.followUpWorseCount;
        else                                               ++r.followUpNeutralCount;

        // Repeated error: the very next shot is also poor.
        if (isPoor(next)) ++r.repeatedErrorCount;

        // Depth: number of shots until the FIRST return to baseline.
        int k = 0;
        for (int j = i + 1; j < n; ++j) { if (scores[j] >= avg) { k = j - i; break; } }
        if (k > 0) { recShotsSum += k; ++r.recoveryShotsSampleCount; }
        else       { ++r.unrecoveredCount; }

        // Over-correction (spatial): needs coordinates and an off-centre poor shot.
        const ShotAnalyticsData& a = shots[i];
        const ShotAnalyticsData& b = shots[i + 1];
        const double magA = std::sqrt(a.x * a.x + a.y * a.y);
        const double magB = std::sqrt(b.x * b.x + b.y * b.y);
        if (magA > 1e-9) {
            ++r.overCorrectionSampleCount;
            const double dot = a.x * b.x + a.y * b.y;         // < 0 => opposite side
            if (dot < 0.0 && magB > o.recoveryOverCorrectFactor * magA) ++r.overCorrectionCount;
        }
    }

    if (r.badShotCount == 0) return r;        // available stays false: nothing to recover from

    r.available           = true;
    r.badShotRecoveryRate = r.summary.recoveryPercentage;        // == 100 * recoveredNext / bad
    r.postErrorAverage    = r.summary.averageNextShotAfterPoor;
    r.postErrorDelta      = r.postErrorAverage - avg;
    r.repeatedErrorRate   = 100.0 * r.repeatedErrorCount / r.badShotCount;

    if (r.recoveryShotsSampleCount > 0) {
        r.recoveryShotsAvailable = true;
        r.averageRecoveryShots   = recShotsSum / r.recoveryShotsSampleCount;
    }
    if (r.overCorrectionSampleCount > 0) {
        r.overCorrectionAvailable = true;
        r.overCorrectionRate = 100.0 * r.overCorrectionCount / r.overCorrectionSampleCount;
    }

    // Confidence from poor-shot sample size (honest for small samples).
    const double confN = o.recoveryConfidentSampleSize > 0.0 ? o.recoveryConfidentSampleSize : 1.0;
    r.recoveryConfidence = clampd(100.0 * r.badShotCount / confN, 0.0, 100.0);

    // Severity from recovery goodness (reuse the shared component mapping).
    r.recoverySeverity = severityFromComponent(r.badShotRecoveryRate);

    // Practical classification (priority-ordered, data-driven; NOT prose).
    if (r.badShotCount < static_cast<int>(o.recoveryMinPatternSample)) {
        r.pattern = RecoveryPattern::InsufficientData;
    } else if (r.repeatedErrorRate >= o.recoveryRepeatedPatternRate) {
        r.pattern = RecoveryPattern::RepeatedErrorPattern;
    } else if (r.overCorrectionAvailable && r.overCorrectionRate >= o.recoveryOverCorrectPatternRate) {
        r.pattern = RecoveryPattern::OverCorrectionPattern;
    } else if (r.badShotRecoveryRate >= o.recoveryGoodRateThreshold
               && (!r.recoveryShotsAvailable || r.averageRecoveryShots <= o.recoverySlowMaxShots)) {
        r.pattern = RecoveryPattern::GoodRecovery;
    } else {
        r.pattern = RecoveryPattern::SlowRecovery;
    }
    return r;
}

SeriesRecoveryReport CoachAnalyticsEngine::buildSeriesRecovery(
    const std::vector<ShotAnalyticsData>& shots, const Options& o)
{
    SeriesRecoveryReport r;
    const int n = static_cast<int>(shots.size());
    if (n < 1) return r;

    // Group by seriesNumber in first-appearance order (deterministic).
    std::vector<int> order;
    std::vector<std::vector<double>> buckets;
    for (const auto& s : shots) {
        int idx = -1;
        for (int j = 0; j < static_cast<int>(order.size()); ++j) {
            if (order[j] == s.seriesNumber) { idx = j; break; }
        }
        if (idx < 0) { order.push_back(s.seriesNumber); buckets.emplace_back(); idx = static_cast<int>(order.size()) - 1; }
        buckets[idx].push_back(s.decimalScore);
    }

    std::vector<double> allScores; allScores.reserve(n);
    for (const auto& s : shots) allScores.push_back(s.decimalScore);
    r.sessionAverage = mean(allScores);
    r.seriesCount    = static_cast<int>(order.size());

    for (int j = 0; j < static_cast<int>(order.size()); ++j) {
        SeriesRecoveryPoint p;
        p.seriesNumber = order[j];
        p.shotCount    = static_cast<int>(buckets[j].size());
        p.average      = mean(buckets[j]);
        p.isDip        = p.average < r.sessionAverage - o.seriesDipThreshold;
        r.series.push_back(p);
    }

    if (r.seriesCount < 2) return r;          // need >= 2 series to talk about rebound
    r.available = true;

    double reboundDeltaSum = 0.0;
    for (int j = 0; j < static_cast<int>(r.series.size()); ++j) {
        if (!r.series[j].isDip) continue;
        ++r.dipCount;
        if (j + 1 < static_cast<int>(r.series.size())) {
            ++r.evaluableDipCount;
            const double delta = r.series[j + 1].average - r.series[j].average;
            reboundDeltaSum += delta;
            if (r.series[j + 1].average > r.series[j].average) ++r.dipReboundCount;
            else                                               ++r.dipSustainedCount;
        }
    }
    if (r.evaluableDipCount > 0) {
        r.dipReboundRate      = 100.0 * r.dipReboundCount / r.evaluableDipCount;
        r.averageReboundDelta = reboundDeltaSum / r.evaluableDipCount;
    }
    return r;
}

RecoveryAnalysis CoachAnalyticsEngine::buildRecoveryAnalysis(
    const std::vector<ShotAnalyticsData>& shots, const Options& o)
{
    RecoveryAnalysis ra;
    const int n = static_cast<int>(shots.size());
    if (n < 2) return ra;
    ra.available = true;

    ra.overall = buildShotRecovery(shots, o);
    ra.series  = buildSeriesRecovery(shots, o);

    // Independent per-position recovery — never merged across positions.
    static const PositionType porder[] = {
        PositionType::Prone, PositionType::Kneeling, PositionType::Standing,
        PositionType::AirRifle, PositionType::AirPistol, PositionType::Unknown
    };
    for (PositionType p : porder) {
        std::vector<ShotAnalyticsData> sub;
        for (const auto& s : shots) if (s.positionType == p) sub.push_back(s);
        if (sub.empty()) continue;
        PositionRecoveryEntry e;
        e.position     = p;
        e.positionName = toString(p);
        e.recovery     = buildShotRecovery(sub, o);
        ra.byPosition.push_back(e);
    }

    // Cross-position comparison over positions with an available recovery report.
    std::vector<const PositionRecoveryEntry*> avail;
    for (const auto& e : ra.byPosition) if (e.recovery.available) avail.push_back(&e);
    if (avail.size() >= 2) {
        ra.comparativeAvailable = true;
        const PositionRecoveryEntry* best  = avail.front();
        const PositionRecoveryEntry* worst = avail.front();
        for (const auto* e : avail) {
            if (e->recovery.badShotRecoveryRate > best->recovery.badShotRecoveryRate)  best  = e;
            if (e->recovery.badShotRecoveryRate < worst->recovery.badShotRecoveryRate) worst = e;
        }
        ra.hasBestRecovery  = true; ra.bestRecoveryPosition  = best->position;
        ra.hasWorstRecovery = true; ra.worstRecoveryPosition = worst->position;
    }

    // Whole-match interpreted metrics in the shared PerformanceMetric language.
    // Higher-is-worse severity mapper (ascending cut points).
    auto worseWhenHigh = [](double v, double good, double ok, double poor) -> Severity {
        if (v <= good) return Severity::None;
        if (v <= ok)   return Severity::Low;
        if (v <= poor) return Severity::Moderate;
        return Severity::High;
    };
    const ShotRecoveryReport& sr = ra.overall;
    if (sr.available) {
        {
            PerformanceMetric m;
            m.name = "Bad-shot recovery rate";
            m.value = sr.badShotRecoveryRate;
            m.availability = true;
            m.confidence = sr.recoveryConfidence;
            m.severity = severityFromComponent(sr.badShotRecoveryRate);
            m.addEvidence(fnum(sr.recoveredNextCount, 0) + " of " + fnum(sr.badShotCount, 0) +
                          " poor shots recovered on the next shot", "recovery.rate",
                          sr.badShotRecoveryRate);
            ra.metrics.push_back(m);
        }
        {
            PerformanceMetric m;
            m.name = "Average recovery shots";
            m.value = sr.averageRecoveryShots;
            m.availability = sr.recoveryShotsAvailable;
            m.confidence = sr.recoveryConfidence;
            m.severity = sr.recoveryShotsAvailable
                ? worseWhenHigh(sr.averageRecoveryShots, 1.2, 2.0, 3.0) : Severity::None;
            m.addEvidence("Shots to regain baseline, averaged over " +
                          fnum(sr.recoveryShotsSampleCount, 0) + " recovered poor shots",
                          "recovery.shots", sr.averageRecoveryShots);
            ra.metrics.push_back(m);
        }
        {
            PerformanceMetric m;
            m.name = "Post-error delta";
            m.value = sr.postErrorDelta;
            m.availability = true;
            m.confidence = sr.recoveryConfidence;
            m.severity = sr.postErrorDelta >= 0.0 ? Severity::None
                         : worseWhenHigh(-sr.postErrorDelta, 0.1, 0.3, 0.6);
            m.addEvidence("Post-error average " + fnum(sr.postErrorAverage, 2) +
                          " vs baseline " + fnum(sr.baseline, 2), "recovery.postErrorDelta",
                          sr.postErrorDelta);
            ra.metrics.push_back(m);
        }
        {
            PerformanceMetric m;
            m.name = "Repeated-error rate";
            m.value = sr.repeatedErrorRate;
            m.availability = true;
            m.confidence = sr.recoveryConfidence;
            m.severity = worseWhenHigh(sr.repeatedErrorRate, 15.0, 30.0, 50.0);
            m.addEvidence(fnum(sr.repeatedErrorCount, 0) + " of " + fnum(sr.badShotCount, 0) +
                          " poor shots followed by another poor shot", "recovery.repeatedError",
                          sr.repeatedErrorRate);
            ra.metrics.push_back(m);
        }
        {
            PerformanceMetric m;
            m.name = "Over-correction rate";
            m.value = sr.overCorrectionRate;
            m.availability = sr.overCorrectionAvailable;
            m.confidence = sr.recoveryConfidence;
            m.severity = sr.overCorrectionAvailable
                ? worseWhenHigh(sr.overCorrectionRate, 15.0, 30.0, 45.0) : Severity::None;
            m.addEvidence(sr.overCorrectionAvailable
                          ? (fnum(sr.overCorrectionCount, 0) + " of " +
                             fnum(sr.overCorrectionSampleCount, 0) +
                             " poor shots overshot to the opposite side")
                          : std::string("No coordinates available for over-correction"),
                          "recovery.overCorrection", sr.overCorrectionRate);
            ra.metrics.push_back(m);
        }
    }
    return ra;
}

// ----------------------------------------------------------------------------
//  Phase 10 — Fatigue Analysis (file-local helpers)
// ----------------------------------------------------------------------------
namespace {

double clamp01d(double v, double lo, double hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

// Disjoint early/late block size for n items at the given fraction. 0 if n < 2.
int earlyLateBlock(int n, double frac)
{
    if (n < 2) return 0;
    int k = static_cast<int>(std::floor(n * frac));
    if (k < 1) k = 1;
    if (2 * k > n) k = n / 2;
    return k < 1 ? 0 : k;
}

bool anyCoords(const std::vector<ShotAnalyticsData>& v)
{
    for (const auto& s : v) if (std::fabs(s.x) > 1e-9 || std::fabs(s.y) > 1e-9) return true;
    return false;
}

double meanRadiusOf(const std::vector<ShotAnalyticsData>& v)
{
    return CoachAnalyticsEngine::computeCoordinateStats(v).meanRadius;
}

// Higher fatigue index => higher severity (data-driven concern level).
Severity fatigueSeverity(double index)
{
    if (index < 0.20) return Severity::None;
    if (index < 0.35) return Severity::Low;
    if (index < 0.50) return Severity::Moderate;
    if (index < 0.70) return Severity::High;
    return Severity::Critical;
}

// Compute a full fatigue view for one ordered dataset (whole match OR one
// position). Sets .pattern to a non-position-specific value; the caller layers
// the position-specific override on top for the whole-match result.
PositionFatigueAnalysis computeFatigueCore(const std::vector<ShotAnalyticsData>& shots,
                                           const CoachAnalyticsEngine::Options& o,
                                           int minTrendSample, PositionType pos)
{
    using Eng = CoachAnalyticsEngine;
    PositionFatigueAnalysis f;
    f.position     = pos;
    f.positionName = toString(pos);
    const int n = static_cast<int>(shots.size());
    f.shotCount = n;
    if (n < 2) { f.pattern = FatiguePattern::InsufficientData; return f; }
    f.available = true;

    std::vector<double> scores(n), idx(n);
    for (int i = 0; i < n; ++i) { scores[i] = shots[i].decimalScore; idx[i] = i; }
    const double sessionAvg = Eng::mean(scores);

    // --- early / middle / late score segments ---
    const int k = earlyLateBlock(n, o.fatigueEarlyLateFraction);
    std::vector<ShotAnalyticsData> early(shots.begin(), shots.begin() + k);
    std::vector<ShotAnalyticsData> late(shots.end() - k, shots.end());
    std::vector<ShotAnalyticsData> middle;
    if (n - 2 * k > 0) middle.assign(shots.begin() + k, shots.end() - k);

    std::vector<double> es, ls, ms;
    for (const auto& s : early)  es.push_back(s.decimalScore);
    for (const auto& s : late)   ls.push_back(s.decimalScore);
    for (const auto& s : middle) ms.push_back(s.decimalScore);
    f.earlyAverage  = Eng::mean(es);
    f.lateAverage   = Eng::mean(ls);
    f.middleAverage = middle.empty() ? (f.earlyAverage + f.lateAverage) / 2.0 : Eng::mean(ms);
    f.earlyLateDelta = f.lateAverage - f.earlyAverage;

    // --- score trend ---
    f.scoreTrendAvailable = (n >= minTrendSample);
    f.scoreSlope = Eng::linearRegressionSlope(idx, scores);

    // late low-shot clustering (below the athlete's own session average)
    int lateLow = 0;
    for (double s : ls) if (s < sessionAvg) ++lateLow;
    const double lateLowFraction = ls.empty() ? 0.0 : static_cast<double>(lateLow) / ls.size();

    // --- dispersion (each segment from its OWN MPI so shift != expansion) ---
    f.coordinateTrendAvailable = anyCoords(shots) && !early.empty() && !late.empty();
    if (f.coordinateTrendAvailable) {
        f.earlyGroupRadius = meanRadiusOf(early);
        f.lateGroupRadius  = meanRadiusOf(late);
        f.groupExpansionRate = f.earlyGroupRadius > 1e-9
            ? (f.lateGroupRadius - f.earlyGroupRadius) / f.earlyGroupRadius : 0.0;
        const CoordinateStats all = Eng::computeCoordinateStats(shots);
        std::vector<double> rad(n);
        for (int i = 0; i < n; ++i) {
            const double dx = shots[i].x - all.mpi.x, dy = shots[i].y - all.mpi.y;
            rad[i] = std::sqrt(dx * dx + dy * dy);
        }
        f.dispersionSlope = Eng::linearRegressionSlope(idx, rad);
    }

    // --- timing (reuse the interval primitive) ---
    const IntervalSeries is = Eng::extractIntervals(shots);
    if (is.available && is.seconds.size() >= 2) {
        const int m = static_cast<int>(is.seconds.size());
        const int kk = earlyLateBlock(m, o.fatigueEarlyLateFraction);
        if (kk >= 1 && 2 * kk <= m) {
            std::vector<double> eiv(is.seconds.begin(), is.seconds.begin() + kk);
            std::vector<double> liv(is.seconds.end() - kk, is.seconds.end());
            f.earlyShotInterval = Eng::mean(eiv);
            f.lateShotInterval  = Eng::mean(liv);
            f.timingChangeRate  = f.earlyShotInterval > 1e-9
                ? (f.lateShotInterval - f.earlyShotInterval) / f.earlyShotInterval : 0.0;
            std::vector<double> gi(m);
            for (int i = 0; i < m; ++i) gi[i] = i;
            f.timingSlope = Eng::linearRegressionSlope(gi, is.seconds);
            f.timingTrendAvailable = true;
        }
    }

    // --- bounded fatigue index (only available components, renormalised) ---
    const double scoreComp = clamp01d(-f.earlyLateDelta / o.fatigueScoreDropRef, 0.0, 1.0);
    const double lateClusterComp = clamp01d(lateLowFraction / o.fatigueLateClusterRef, 0.0, 1.0);
    double num = o.fatigueWeightScore * scoreComp + o.fatigueWeightLateCluster * lateClusterComp;
    double den = o.fatigueWeightScore + o.fatigueWeightLateCluster;
    if (f.coordinateTrendAvailable) {
        const double dispComp = clamp01d(f.groupExpansionRate / o.fatigueDispersionRef, 0.0, 1.0);
        num += o.fatigueWeightDispersion * dispComp; den += o.fatigueWeightDispersion;
    }
    if (f.timingTrendAvailable) {
        const double timeComp = clamp01d(std::fabs(f.timingChangeRate) / o.fatigueTimingRef, 0.0, 1.0);
        num += o.fatigueWeightTiming * timeComp; den += o.fatigueWeightTiming;
    }
    f.fatigueIndex = den > 0.0 ? clamp01d(num / den, 0.0, 1.0) : 0.0;

    // --- confidence (sample size; capped hard when the trend is unreliable) ---
    const double confN = o.fatigueConfidentSampleSize > 0.0 ? o.fatigueConfidentSampleSize : 1.0;
    f.confidence = clamp01d(100.0 * n / confN, 0.0, 100.0);
    if (!f.scoreTrendAvailable && f.confidence > o.fatigueLowSampleConfCap)
        f.confidence = o.fatigueLowSampleConfCap;

    // --- pattern classification (deterministic, priority-ordered) ---
    // NOTE ON PRECEDENCE: FatigueCompensation is checked before the generic
    // IncreasingDispersion / TimingSlowdown because a MAINTAINED score while
    // effort indicators rise ("working harder to hold the result") is the
    // higher-information story. A falling score with dispersion up is plain
    // IncreasingDispersion; a falling score with timing up is TimingSlowdown.
    const bool  middleAvail   = !middle.empty();
    const double earlyToMiddle = f.middleAverage - f.earlyAverage;
    const double middleToLate  = f.lateAverage - f.middleAverage;
    const bool scoreStable  = std::fabs(f.earlyLateDelta) < o.fatigueStableBand;
    const bool lateDrop     = middleAvail
                              && (middleToLate <= -o.fatigueLateDropThreshold)
                              && (std::fabs(earlyToMiddle) < o.fatigueStableBand)
                              && (f.lateAverage < f.earlyAverage);
    const bool dispersionUp = f.coordinateTrendAvailable
                              && (f.groupExpansionRate >= o.fatigueExpansionThreshold);
    const bool timingUp     = f.timingTrendAvailable
                              && (f.timingChangeRate >= o.fatigueTimingSlowThreshold);
    const bool scoreLowerLate = f.earlyLateDelta <= -0.05;
    const bool gradual = (f.scoreSlope <= o.fatigueScoreSlopeThreshold)
                         && ((f.earlyAverage - f.lateAverage) >= o.fatigueGradualTotalDrop);

    if (!f.scoreTrendAvailable)                 f.pattern = FatiguePattern::InsufficientData;
    else if (lateDrop)                          f.pattern = FatiguePattern::LateMatchDrop;
    else if (scoreStable && timingUp)           f.pattern = FatiguePattern::FatigueCompensation;
    else if (dispersionUp)                      f.pattern = FatiguePattern::IncreasingDispersion;
    else if (timingUp && scoreLowerLate)        f.pattern = FatiguePattern::TimingSlowdown;
    else if (gradual)                           f.pattern = FatiguePattern::GradualDecline;
    else                                        f.pattern = FatiguePattern::NoFatigueDetected;
    return f;
}

} // namespace

FatigueAnalysis CoachAnalyticsEngine::buildFatigueAnalysis(
    const std::vector<ShotAnalyticsData>& shots, const Options& o)
{
    FatigueAnalysis fa;
    const int n = static_cast<int>(shots.size());
    if (n < 2) { fa.overallPattern = FatiguePattern::InsufficientData; return fa; }

    const PositionFatigueAnalysis whole =
        computeFatigueCore(shots, o, o.fatigueMinWholeMatch, PositionType::Unknown);
    fa.available          = true;
    fa.earlyAverage       = whole.earlyAverage;
    fa.middleAverage      = whole.middleAverage;
    fa.lateAverage        = whole.lateAverage;
    fa.earlyLateDelta     = whole.earlyLateDelta;
    fa.scoreSlope         = whole.scoreSlope;
    fa.dispersionSlope    = whole.dispersionSlope;
    fa.timingSlope        = whole.timingSlope;
    fa.earlyGroupRadius   = whole.earlyGroupRadius;
    fa.lateGroupRadius    = whole.lateGroupRadius;
    fa.groupExpansionRate = whole.groupExpansionRate;
    fa.earlyShotInterval  = whole.earlyShotInterval;
    fa.lateShotInterval   = whole.lateShotInterval;
    fa.timingChangeRate   = whole.timingChangeRate;
    fa.fatigueIndex       = whole.fatigueIndex;
    fa.confidence         = whole.confidence;
    fa.scoreTrendAvailable      = whole.scoreTrendAvailable;
    fa.coordinateTrendAvailable = whole.coordinateTrendAvailable;
    fa.timingTrendAvailable     = whole.timingTrendAvailable;
    fa.overallPattern     = whole.pattern;

    // Independent per-position analyses (never merged).
    static const PositionType order[] = {
        PositionType::Prone, PositionType::Kneeling, PositionType::Standing,
        PositionType::AirRifle, PositionType::AirPistol, PositionType::Unknown
    };
    int positionsWithData = 0, fatiguedCount = 0;
    for (PositionType p : order) {
        std::vector<ShotAnalyticsData> sub;
        for (const auto& s : shots) if (s.positionType == p) sub.push_back(s);
        if (sub.empty()) continue;
        PositionFatigueAnalysis pf = computeFatigueCore(sub, o, o.fatigueMinPerPosition, p);
        if (pf.pattern != FatiguePattern::InsufficientData) ++positionsWithData;
        if (pf.pattern != FatiguePattern::InsufficientData
            && pf.pattern != FatiguePattern::NoFatigueDetected) ++fatiguedCount;
        fa.byPosition.push_back(pf);
    }

    // Most-fatigued position (by index) among the fatigued ones.
    double bestIdx = -1.0;
    for (const auto& pf : fa.byPosition) {
        const bool fat = pf.pattern != FatiguePattern::InsufficientData
                         && pf.pattern != FatiguePattern::NoFatigueDetected;
        if (fat && pf.fatigueIndex > bestIdx) {
            bestIdx = pf.fatigueIndex;
            fa.hasFatiguedPosition = true;
            fa.mostFatiguedPosition = pf.position;
        }
    }

    // Position-specific override: a STRICT nonzero subset of positions fatigued.
    if (positionsWithData >= 2 && fatiguedCount >= 1 && fatiguedCount < positionsWithData)
        fa.overallPattern = FatiguePattern::PositionSpecificFatigue;

    // Interpreted whole-match metrics in the shared language.
    auto negWorse = [](double v, double good, double ok, double poor) -> Severity {
        const double d = -v;                       // more-negative = worse
        if (d <= good) return Severity::None;
        if (d <= ok)   return Severity::Low;
        if (d <= poor) return Severity::Moderate;
        return Severity::High;
    };
    {
        PerformanceMetric m;
        m.name = "Fatigue index";
        m.value = fa.fatigueIndex;
        m.availability = fa.scoreTrendAvailable;
        m.confidence = fa.confidence;
        m.severity = fatigueSeverity(fa.fatigueIndex);
        m.addEvidence("Bounded fatigue signal (0..1) over " + fnum(n, 0) + " shots",
                      "fatigue.index", fa.fatigueIndex);
        fa.metrics.push_back(m);
    }
    {
        PerformanceMetric m;
        m.name = "Early-to-late score delta";
        m.value = fa.earlyLateDelta;
        m.availability = true;
        m.confidence = fa.confidence;
        m.severity = fa.earlyLateDelta >= 0.0 ? Severity::None : negWorse(fa.earlyLateDelta, 0.1, 0.3, 0.6);
        m.addEvidence("Late average " + fnum(fa.lateAverage, 2) + " vs early " +
                      fnum(fa.earlyAverage, 2), "fatigue.earlyLateDelta", fa.earlyLateDelta);
        fa.metrics.push_back(m);
    }
    {
        PerformanceMetric m;
        m.name = "Group expansion rate";
        m.value = fa.groupExpansionRate;
        m.availability = fa.coordinateTrendAvailable;
        m.confidence = fa.confidence;
        m.severity = fa.coordinateTrendAvailable
            ? fatigueSeverity(clamp01d(fa.groupExpansionRate / o.fatigueDispersionRef, 0.0, 1.0))
            : Severity::None;
        m.addEvidence(fa.coordinateTrendAvailable
                      ? ("Late group radius " + fnum(fa.lateGroupRadius, 2) + "mm vs early " +
                         fnum(fa.earlyGroupRadius, 2) + "mm")
                      : std::string("No coordinates available for dispersion trend"),
                      "fatigue.groupExpansion", fa.groupExpansionRate);
        fa.metrics.push_back(m);
    }
    {
        PerformanceMetric m;
        m.name = "Timing change rate";
        m.value = fa.timingChangeRate;
        m.availability = fa.timingTrendAvailable;
        m.confidence = fa.confidence;
        m.severity = fa.timingTrendAvailable
            ? fatigueSeverity(clamp01d(std::fabs(fa.timingChangeRate) / o.fatigueTimingRef, 0.0, 1.0))
            : Severity::None;
        m.addEvidence(fa.timingTrendAvailable
                      ? ("Late interval " + fnum(fa.lateShotInterval, 2) + "s vs early " +
                         fnum(fa.earlyShotInterval, 2) + "s")
                      : std::string("No timestamps available for timing trend"),
                      "fatigue.timingChange", fa.timingChangeRate);
        fa.metrics.push_back(m);
    }
    return fa;
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
    report.heatMapAnalysis  = buildHeatMapAnalysis(prepared, options);
    report.timingAnalysis   = buildTimingAnalysis(prepared, options);
    report.positionAnalysis = buildPositionAnalysis(prepared, options);
    report.recoveryAnalysis = buildRecoveryAnalysis(prepared, options);
    report.fatigueAnalysis  = buildFatigueAnalysis(prepared, options);
    report.valid = true;
    report.message = report.lowSampleWarning
        ? "Analysed with fewer than 10 shots; results are indicative only."
        : "OK.";
    return report;
}

} // namespace analytics
} // namespace techaim
