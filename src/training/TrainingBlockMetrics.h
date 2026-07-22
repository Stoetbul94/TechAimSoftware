#ifndef TA_TRAINING_BLOCKMETRICS_H
#define TA_TRAINING_BLOCKMETRICS_H

// Training Lab (T1) — measured block metrics.
//
// Geometry (MPI, group radius, group diameter, H/V spread) is delegated to the
// accepted CoachAnalyticsEngine (`computeCoordinateStats`) — ONE authoritative
// implementation, never recomputed in QML. Score/timing statistics are simple
// scalar stats computed here. Everything is MEASURED/OBSERVED language only:
// no causes, no diagnoses, no confidence claims.
//
// Future Group Pattern contract: per-shot coordinates + these metrics are the
// stored inputs a future pattern classifier (tight-centred / offset / wide /
// H-string / V-string / diagonal / two-cluster / drift / outlier / expansion)
// will operate on — only after a completed block or >= 5 valid shots. T1
// stores the data and ships NO diagnostic engine.

#include <QString>
#include <QVector>

#include "reliability/events/EventTypes.h"     // ShotCore
#include "reliability/core/FixedPoint.h"

namespace ta {
namespace training {

struct BlockMetrics {
    int  shotCount = 0;
    bool hasData = false;          // >= 1 shot
    bool hasGroup = false;         // >= 2 shots (a single shot is NOT a group)
    // score (decimal units, from scoreTenths)
    double totalScore = 0.0;
    double averageScore = 0.0;
    double scoreStdDev = 0.0;      // sample SD, 0 when < 2 shots
    // geometry (mm) — from the shared analytics engine
    double mpiX = 0.0, mpiY = 0.0;
    double groupRadius = 0.0;      // mean distance from MPI
    double groupDiameter = 0.0;    // extreme spread
    double horizontalSpread = 0.0; // sample SD of x
    double verticalSpread = 0.0;   // sample SD of y
    // timing (seconds, from splitMs) — valid only when every shot carried one
    bool   hasTiming = false;
    double averageShotTime = 0.0;
    double shotTimeStdDev = 0.0;
};

// Compute metrics for one block's measured shots (order preserved).
BlockMetrics computeBlockMetrics(const QVector<ta::rel::ShotCore>& shots);

// Cross-block observation helpers (guarded: empty/one-shot blocks and zero
// denominators produce no observation rather than nonsense).
struct BlockComparison {
    int bestScoreBlock = 0;        // 1-based; 0 = none
    int tightestGroupBlock = 0;    // smallest diameter among blocks with a group
    int mostRepeatableBlock = 0;   // lowest score SD among blocks with >= 2 shots
    double centreDriftMm = 0.0;    // |MPI(last) - MPI(first)| for blocks with data
    double centreDriftX = 0.0;     // signed (+right)
    double centreDriftY = 0.0;     // signed (+high)
    double groupSizeChangePct = 0.0;   // diameter change first -> last (+grew)
    bool   hasDrift = false;
    bool   hasSizeChange = false;
};

BlockComparison compareBlocks(const QVector<BlockMetrics>& blocks);

}} // namespace ta::training

#endif // TA_TRAINING_BLOCKMETRICS_H
