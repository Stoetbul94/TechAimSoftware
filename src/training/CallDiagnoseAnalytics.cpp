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
    return o;
}

}} // namespace ta::training
