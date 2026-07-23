#include "CallDiagnoseAnalytics.h"

#include <algorithm>
#include <cmath>

namespace ta {
namespace training {

using ta::rel::CallDiagnoseShotRecord;

QVector<CallShotStat> computeCallShotStats(const QVector<CallDiagnoseShotRecord>& shots)
{
    QVector<CallShotStat> out;
    out.reserve(shots.size());
    for (const CallDiagnoseShotRecord& r : shots) {
        if (!r.hasCall) continue;                 // only completed calls
        CallShotStat s;
        s.shotNumber = r.shotNumber;
        s.position = r.position;
        s.actualXMm = r.actual.xHundredthMm / 100.0;
        s.actualYMm = r.actual.yHundredthMm / 100.0;
        s.calledXMm = r.calledXHundredthMm / 100.0;
        s.calledYMm = r.calledYHundredthMm / 100.0;
        s.errorXMm = s.calledXMm - s.actualXMm;   // + = call was right of impact
        s.errorYMm = s.calledYMm - s.actualYMm;   // + = call was high of impact
        s.errorMm = std::sqrt(s.errorXMm * s.errorXMm + s.errorYMm * s.errorYMm);
        s.actualScore = r.actual.scoreTenths / 10.0;
        out.append(s);
    }
    return out;
}

CallSessionStats computeCallSessionStats(const QVector<CallShotStat>& stats)
{
    CallSessionStats o;
    o.count = stats.size();
    if (stats.isEmpty())
        return o;

    QVector<double> errors;
    errors.reserve(stats.size());
    double sumErr = 0.0, sumAbsX = 0.0, sumAbsY = 0.0, sumX = 0.0, sumY = 0.0;
    o.smallestError = stats[0].errorMm;
    o.largestError = stats[0].errorMm;
    o.bestShotNumber = stats[0].shotNumber;
    o.worstShotNumber = stats[0].shotNumber;
    for (const CallShotStat& s : stats) {
        errors.append(s.errorMm);
        sumErr += s.errorMm;
        sumAbsX += std::fabs(s.errorXMm);
        sumAbsY += std::fabs(s.errorYMm);
        sumX += s.errorXMm;
        sumY += s.errorYMm;
        if (s.errorMm < o.smallestError) { o.smallestError = s.errorMm; o.bestShotNumber = s.shotNumber; }
        if (s.errorMm > o.largestError)  { o.largestError = s.errorMm; o.worstShotNumber = s.shotNumber; }
    }
    const int n = stats.size();
    o.averageError = sumErr / n;
    o.avgAbsX = sumAbsX / n;
    o.avgAbsY = sumAbsY / n;
    o.biasX = sumX / n;
    o.biasY = sumY / n;
    o.hasBias = (n >= 3);

    // median
    std::sort(errors.begin(), errors.end());
    o.medianError = (n % 2 == 1) ? errors[n / 2]
                                 : 0.5 * (errors[n / 2 - 1] + errors[n / 2]);

    // sample standard deviation of the radial error
    if (n >= 2) {
        double ss = 0.0;
        for (double e : errors) { const double d = e - o.averageError; ss += d * d; }
        o.errorStdDev = std::sqrt(ss / (n - 1));
    }

    // trend: least-squares slope of error vs 0-based order (needs >= 5 calls)
    if (n >= 5) {
        const double meanX = (n - 1) / 2.0;
        double num = 0.0, den = 0.0;
        for (int i = 0; i < n; ++i) {
            const double dx = i - meanX;
            num += dx * (stats[i].errorMm - o.averageError);
            den += dx * dx;
        }
        if (den > 0.0) { o.trendSlope = num / den; o.hasTrend = true; }
    }

    // robust outlier detection — Tukey IQR fences on the (sorted) radial error.
    // Linear-interpolated quartiles; an error above q3 + 1.5*IQR is an outlier.
    auto quantile = [&errors, n](double p) -> double {
        if (n == 1) return errors[0];
        const double pos = p * (n - 1);
        const int lo = static_cast<int>(std::floor(pos));
        const int hi = static_cast<int>(std::ceil(pos));
        if (lo == hi) return errors[lo];
        return errors[lo] + (pos - lo) * (errors[hi] - errors[lo]);
    };
    o.q1 = quantile(0.25);
    o.q3 = quantile(0.75);
    o.iqr = o.q3 - o.q1;
    o.outlierThreshold = o.q3 + 1.5 * o.iqr;
    if (n >= 4) {
        for (const CallShotStat& s : stats)
            if (s.errorMm > o.outlierThreshold) ++o.outlierCount;
    }

    // early-vs-late average error (>= 6 calls) — for the trend narrative.
    if (n >= 6) {
        const int half = n / 2;
        double e1 = 0.0, e2 = 0.0;
        for (int i = 0; i < half; ++i) e1 += stats[i].errorMm;
        for (int i = n - half; i < n; ++i) e2 += stats[i].errorMm;
        o.firstHalfAvg = e1 / half;
        o.secondHalfAvg = e2 / half;
        o.hasHalves = true;
    }
    return o;
}

CompareBounds comparisonBounds(double calledXMm, double calledYMm,
                               double actualXMm, double actualYMm,
                               double minHalfRangeMm, double markerMm)
{
    // include call, actual AND the target centre (0,0); square viewport.
    double maxAbs = 0.0;
    maxAbs = std::max(maxAbs, std::fabs(calledXMm));
    maxAbs = std::max(maxAbs, std::fabs(calledYMm));
    maxAbs = std::max(maxAbs, std::fabs(actualXMm));
    maxAbs = std::max(maxAbs, std::fabs(actualYMm));
    CompareBounds b;
    // 20% padding for the extreme point + the marker radius so nothing clips.
    b.halfRangeMm = std::max(minHalfRangeMm, maxAbs * 1.20 + markerMm);
    b.outsideFace = false;
    return b;
}

CompareBounds targetBounds(double calledXMm, double calledYMm,
                           double actualXMm, double actualYMm,
                           double faceRadiusMm, double markerMm)
{
    double maxAbs = 0.0;
    maxAbs = std::max(maxAbs, std::fabs(calledXMm));
    maxAbs = std::max(maxAbs, std::fabs(calledYMm));
    maxAbs = std::max(maxAbs, std::fabs(actualXMm));
    maxAbs = std::max(maxAbs, std::fabs(actualYMm));
    CompareBounds b;
    b.outsideFace = (maxAbs > faceRadiusMm);
    // normally the whole face; expand (never clamp) if a marker lies outside.
    b.halfRangeMm = b.outsideFace ? (maxAbs * 1.10 + markerMm) : faceRadiusMm;
    return b;
}

}} // namespace ta::training
