#include "TrainingBlockMetrics.h"

#include "analytics/CoachAnalyticsEngine.h"

#include <cmath>
#include <vector>

using techaim::analytics::CoachAnalyticsEngine;
using techaim::analytics::ShotAnalyticsData;

namespace ta {
namespace training {

namespace {

double sampleStdDev(const QVector<double>& v)
{
    const int n = v.size();
    if (n < 2)
        return 0.0;
    double mean = 0.0;
    for (double x : v) mean += x;
    mean /= n;
    double ss = 0.0;
    for (double x : v) ss += (x - mean) * (x - mean);
    return std::sqrt(ss / (n - 1));
}

} // namespace

BlockMetrics computeBlockMetrics(const QVector<ta::rel::ShotCore>& shots)
{
    BlockMetrics m;
    m.shotCount = shots.size();
    if (shots.isEmpty())
        return m;
    m.hasData = true;
    m.hasGroup = shots.size() >= 2;

    // score stats (decimal units from fixed-point tenths)
    QVector<double> scores; scores.reserve(shots.size());
    QVector<double> times;  times.reserve(shots.size());
    bool allTimed = true;
    for (const ta::rel::ShotCore& s : shots) {
        const double sc = s.scoreTenths / 10.0;
        scores.append(sc);
        m.totalScore += sc;
        if (s.splitMs > 0)
            times.append(s.splitMs / 1000.0);
        else
            allTimed = false;
    }
    m.averageScore = m.totalScore / shots.size();
    m.scoreStdDev = sampleStdDev(scores);

    // geometry — delegate to the ONE accepted implementation.
    std::vector<ShotAnalyticsData> a;
    a.reserve(static_cast<size_t>(shots.size()));
    int n = 0;
    for (const ta::rel::ShotCore& s : shots) {
        ShotAnalyticsData d;
        d.shotNumber = ++n;
        d.decimalScore = s.scoreTenths / 10.0;
        d.x = s.xHundredthMm / 100.0;    // hundredth-mm -> mm, +right
        d.y = s.yHundredthMm / 100.0;    // +high
        d.isValid = true;
        d.isSighter = false;
        d.isCompetitionShot = false;     // Training shot — never competition
        a.push_back(d);
    }
    const auto stats = CoachAnalyticsEngine::computeCoordinateStats(a);
    if (stats.hasData) {
        m.mpiX = stats.mpi.x;
        m.mpiY = stats.mpi.y;
        m.groupRadius = stats.groupRadius;
        m.groupDiameter = stats.groupDiameter;
        m.horizontalSpread = stats.horizontalSpread;
        m.verticalSpread = stats.verticalSpread;
    }

    // timing stats only when every shot carried a split (no fabricated timing)
    if (allTimed && !times.isEmpty()) {
        m.hasTiming = true;
        double sum = 0.0;
        for (double t : times) sum += t;
        m.averageShotTime = sum / times.size();
        m.shotTimeStdDev = sampleStdDev(times);
    }
    return m;
}

BlockComparison compareBlocks(const QVector<BlockMetrics>& blocks)
{
    BlockComparison c;
    double bestScore = -1.0, tightest = -1.0, lowestSd = -1.0;
    int firstWithGroup = -1, lastWithGroup = -1;
    int firstWithData = -1, lastWithData = -1;

    for (int i = 0; i < blocks.size(); ++i) {
        const BlockMetrics& b = blocks[i];
        if (!b.hasData)
            continue;                       // empty block: no observation
        if (firstWithData < 0) firstWithData = i;
        lastWithData = i;
        if (b.averageScore > bestScore) { bestScore = b.averageScore; c.bestScoreBlock = i + 1; }
        if (b.hasGroup) {                   // one shot is never a "group"
            if (firstWithGroup < 0) firstWithGroup = i;
            lastWithGroup = i;
            if (tightest < 0 || b.groupDiameter < tightest) {
                tightest = b.groupDiameter; c.tightestGroupBlock = i + 1;
            }
            if (b.shotCount >= 2 && (lowestSd < 0 || b.scoreStdDev < lowestSd)) {
                lowestSd = b.scoreStdDev; c.mostRepeatableBlock = i + 1;
            }
        }
    }

    // centre drift first -> last block with data (needs two distinct blocks)
    if (firstWithData >= 0 && lastWithData > firstWithData) {
        const BlockMetrics& f = blocks[firstWithData];
        const BlockMetrics& l = blocks[lastWithData];
        c.centreDriftX = l.mpiX - f.mpiX;
        c.centreDriftY = l.mpiY - f.mpiY;
        c.centreDriftMm = std::sqrt(c.centreDriftX * c.centreDriftX
                                    + c.centreDriftY * c.centreDriftY);
        c.hasDrift = true;
    }
    // group-size change first -> last block WITH a group; guard zero diameter
    if (firstWithGroup >= 0 && lastWithGroup > firstWithGroup) {
        const double d0 = blocks[firstWithGroup].groupDiameter;
        const double d1 = blocks[lastWithGroup].groupDiameter;
        if (d0 > 1e-9) {
            c.groupSizeChangePct = (d1 - d0) / d0 * 100.0;
            c.hasSizeChange = true;
        }
    }
    return c;
}

}} // namespace ta::training
