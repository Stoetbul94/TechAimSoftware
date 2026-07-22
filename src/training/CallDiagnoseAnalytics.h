#ifndef TA_TRAINING_CALLDIAGNOSEANALYTICS_H
#define TA_TRAINING_CALLDIAGNOSEANALYTICS_H

// Call & Diagnose (T2) — pure-C++ call-accuracy analytics. Operates on the
// reducer-authoritative shot records (actual + call). MEASURED/OBSERVED only:
// radial call error, X/Y error, directional bias, consistency and trend. No
// diagnosis, no technical-cause claims. All geometry guarded for zero/one-shot,
// missing calls, identical points and extreme values. Analytics NEVER run in
// QML — this is the single authoritative implementation.

#include <QVector>

#include "reliability/reducer/SessionState.h"   // CallDiagnoseShotRecord

namespace ta {
namespace training {

// One measured shot with a completed call (mm units; +x right, +y high).
struct CallShotStat {
    int    shotNumber = 0;
    int    position = 0;
    double actualXMm = 0.0, actualYMm = 0.0;
    double calledXMm = 0.0, calledYMm = 0.0;
    double errorMm = 0.0;      // radial distance call → actual
    double errorXMm = 0.0;     // signed: called − actual (+ = call was right)
    double errorYMm = 0.0;     // signed: called − actual (+ = call was high)
    double actualScore = 0.0;  // decimal
};

struct CallSessionStats {
    int    count = 0;          // shots with a completed call
    double averageError = 0.0;
    double medianError = 0.0;
    double smallestError = 0.0;
    double largestError = 0.0;
    double errorStdDev = 0.0;
    double avgAbsX = 0.0, avgAbsY = 0.0;
    // signed mean error (call − actual): +x right / −x left, +y high / −y low.
    double biasX = 0.0, biasY = 0.0;
    bool   hasBias = false;    // >= 3 calls
    // trend: least-squares slope of error vs call order (negative = improving).
    double trendSlope = 0.0;
    bool   hasTrend = false;   // >= 5 calls
    int    bestShotNumber = 0; // shotNumber of the smallest-error call
    int    worstShotNumber = 0;
};

// Per-shot stats for the shots that have a completed call (order preserved).
QVector<CallShotStat> computeCallShotStats(const QVector<ta::rel::CallDiagnoseShotRecord>& shots);

// Session aggregates over the given per-shot stats (already filtered/scoped).
CallSessionStats computeCallSessionStats(const QVector<CallShotStat>& stats);

}} // namespace ta::training

#endif // TA_TRAINING_CALLDIAGNOSEANALYTICS_H
