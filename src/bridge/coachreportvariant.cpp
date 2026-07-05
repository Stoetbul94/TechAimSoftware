#include "coachreportvariant.h"

// Marshalling only. Every enum is rendered through the engine's toString so the
// frontend never hard-codes label strings. Coordinates stay in millimetres.

namespace techaim {
namespace bridge {

using namespace techaim::analytics;

namespace {

inline QString q(const std::string& s) { return QString::fromStdString(s); }

// Tolerant position parsing at the boundary: accept any case and separators
// (e.g. "Standing", "air rifle", "AIR_PISTOL") and normalise to the engine's
// canonical lowercase token. The engine's positionFromString stays strict.
PositionType parsePosition(const QString& raw)
{
    QString s = raw.toLower();
    s.remove(' ').remove('_').remove('-');
    if (s == "prone")     return PositionType::Prone;
    if (s == "kneeling")  return PositionType::Kneeling;
    if (s == "standing")  return PositionType::Standing;
    if (s == "airrifle")  return PositionType::AirRifle;
    if (s == "airpistol") return PositionType::AirPistol;
    return PositionType::Unknown;
}

QVariantList evidenceList(const std::vector<Evidence>& ev)
{
    QVariantList l;
    for (const auto& e : ev) {
        QVariantMap m;
        m["statement"] = q(e.statement);
        m["metricKey"] = q(e.metricKey);
        m["value"]     = e.value;
        m["hasValue"]  = e.hasValue;
        l << m;
    }
    return l;
}

QVariantList stringList(const std::vector<std::string>& v)
{
    QVariantList l;
    for (const auto& s : v) l << q(s);
    return l;
}

QVariantList doubleList(const std::vector<double>& v)
{
    QVariantList l;
    for (double d : v) l << d;
    return l;
}

QVariantList intList(const std::vector<int>& v)
{
    QVariantList l;
    for (int i : v) l << i;
    return l;
}

QVariantMap metricToMap(const PerformanceMetric& m)
{
    QVariantMap o;
    o["name"]         = q(m.name);
    o["value"]        = m.value;
    o["availability"] = m.availability;
    o["confidence"]   = m.confidence;
    o["severity"]     = q(toString(m.severity));
    o["evidence"]     = evidenceList(m.evidence);
    return o;
}

QVariantList metricList(const std::vector<PerformanceMetric>& v)
{
    QVariantList l;
    for (const auto& m : v) l << metricToMap(m);
    return l;
}

QVariantMap coordStatsToMap(const CoordinateStats& c)
{
    QVariantMap o;
    o["hasData"]          = c.hasData;
    o["shotCount"]        = c.shotCount;
    o["mpiX"]             = c.mpi.x;      // millimetres
    o["mpiY"]             = c.mpi.y;
    o["meanRadius"]       = c.meanRadius;
    o["groupRadius"]      = c.groupRadius;
    o["groupDiameter"]    = c.groupDiameter;
    o["horizontalSpread"] = c.horizontalSpread;
    o["verticalSpread"]   = c.verticalSpread;
    return o;
}

QVariantMap gridToMap(const HeatMapGrid& g)
{
    QVariantMap o;
    o["binSize"]        = g.binSize;
    o["originX"]        = g.originX;
    o["originY"]        = g.originY;
    o["gridWidth"]      = g.gridWidth;
    o["gridHeight"]     = g.gridHeight;
    o["shotCount"]      = g.shotCount;
    o["hasData"]        = g.hasData;
    o["stats"]          = coordStatsToMap(g.stats);
    o["maxCellDensity"] = g.maxCellDensity;
    o["hasDominantZone"]  = g.hasDominantZone;
    o["dominantBinX"]     = g.dominantBinX;
    o["dominantBinY"]     = g.dominantBinY;
    o["dominantCentreX"]  = g.dominantCentreX;
    o["dominantCentreY"]  = g.dominantCentreY;
    o["dominantCount"]    = g.dominantCount;
    o["dominantSharePct"] = g.dominantSharePct;
    QVariantList cells;
    for (const auto& c : g.cells) {
        QVariantMap cm;
        cm["binX"]         = c.binX;
        cm["binY"]         = c.binY;
        cm["centreX"]      = c.centreX;   // millimetres
        cm["centreY"]      = c.centreY;
        cm["count"]        = c.count;
        cm["averageScore"] = c.averageScore;
        cells << cm;
    }
    o["cells"] = cells;
    return o;
}

QVariantMap comparisonToMap(const HeatMapComparison& c)
{
    QVariantMap o;
    o["available"]       = c.available;
    o["shiftX"]          = c.shiftX;
    o["shiftY"]          = c.shiftY;
    o["shiftDistance"]   = c.shiftDistance;
    o["radiusChange"]    = c.radiusChange;
    o["radiusChangePct"] = c.radiusChangePct;
    o["dominantShiftX"]  = c.dominantShiftX;
    o["dominantShiftY"]  = c.dominantShiftY;
    return o;
}

QVariantMap recoverySummaryToMap(const RecoverySummary& r)
{
    QVariantMap o;
    o["available"]               = r.available;
    o["poorShotCount"]           = r.poorShotCount;
    o["successfulRecoveryCount"] = r.successfulRecoveryCount;
    o["failedRecoveryCount"]     = r.failedRecoveryCount;
    o["recoveryPercentage"]      = r.recoveryPercentage;
    o["averageNextShotAfterPoor"]= r.averageNextShotAfterPoor;
    return o;
}

QVariantMap shotRecoveryToMap(const ShotRecoveryReport& r)
{
    QVariantMap o;
    o["available"]              = r.available;
    o["shotCount"]              = r.shotCount;
    o["baseline"]               = r.baseline;
    o["baselineSD"]             = r.baselineSD;
    o["badShotCount"]           = r.badShotCount;
    o["recoveredNextCount"]     = r.recoveredNextCount;
    o["notRecoveredNextCount"]  = r.notRecoveredNextCount;
    o["badShotRecoveryRate"]    = r.badShotRecoveryRate;
    o["recoveryShotsAvailable"] = r.recoveryShotsAvailable;
    o["averageRecoveryShots"]   = r.averageRecoveryShots;
    o["unrecoveredCount"]       = r.unrecoveredCount;
    o["postErrorAverage"]       = r.postErrorAverage;
    o["postErrorDelta"]         = r.postErrorDelta;
    o["followUpBetterCount"]    = r.followUpBetterCount;
    o["followUpWorseCount"]     = r.followUpWorseCount;
    o["followUpNeutralCount"]   = r.followUpNeutralCount;
    o["repeatedErrorRate"]      = r.repeatedErrorRate;
    o["overCorrectionAvailable"]= r.overCorrectionAvailable;
    o["overCorrectionRate"]     = r.overCorrectionRate;
    o["recoveryConfidence"]     = r.recoveryConfidence;
    o["recoverySeverity"]       = q(toString(r.recoverySeverity));
    o["pattern"]                = q(toString(r.pattern));
    o["summary"]                = recoverySummaryToMap(r.summary);
    return o;
}

QVariantMap positionFatigueToMap(const PositionFatigueAnalysis& f)
{
    QVariantMap o;
    o["position"]     = q(toString(f.position));
    o["available"]    = f.available;
    o["shotCount"]    = f.shotCount;
    o["pattern"]      = q(toString(f.pattern));
    o["earlyAverage"] = f.earlyAverage;
    o["middleAverage"]= f.middleAverage;
    o["lateAverage"]  = f.lateAverage;
    o["earlyLateDelta"]     = f.earlyLateDelta;
    o["scoreSlope"]         = f.scoreSlope;
    o["dispersionSlope"]    = f.dispersionSlope;
    o["timingSlope"]        = f.timingSlope;
    o["earlyGroupRadius"]   = f.earlyGroupRadius;
    o["lateGroupRadius"]    = f.lateGroupRadius;
    o["groupExpansionRate"] = f.groupExpansionRate;
    o["earlyShotInterval"]  = f.earlyShotInterval;
    o["lateShotInterval"]   = f.lateShotInterval;
    o["timingChangeRate"]   = f.timingChangeRate;
    o["fatigueIndex"]       = f.fatigueIndex;
    o["confidence"]         = f.confidence;
    o["scoreTrendAvailable"]      = f.scoreTrendAvailable;
    o["coordinateTrendAvailable"] = f.coordinateTrendAvailable;
    o["timingTrendAvailable"]     = f.timingTrendAvailable;
    return o;
}

QVariantList rankingList(const std::vector<PositionRankingEntry>& v)
{
    QVariantList l;
    for (const auto& e : v) {
        QVariantMap m;
        m["position"] = q(toString(e.position));
        m["value"]    = e.value;
        l << m;
    }
    return l;
}

QVariantMap positionReportToMap(const PositionReport& p)
{
    QVariantMap o;
    o["position"]   = q(toString(p.position));
    o["available"]  = p.available;
    o["shotCount"]  = p.shotCount;
    o["confidence"] = p.confidence;

    const PositionMeasurements& m = p.measurements;
    QVariantMap mm;
    mm["available"]    = m.available;
    mm["shotCount"]    = m.shotCount;
    mm["totalScore"]   = m.totalScore;
    mm["averageScore"] = m.averageScore;
    mm["medianScore"]  = m.medianScore;
    mm["minScore"]     = m.minScore;
    mm["maxScore"]     = m.maxScore;
    mm["scoreSD"]      = m.scoreSD;
    mm["scoreRange"]   = m.scoreRange;
    mm["scoreVariance"]= m.scoreVariance;
    mm["coefficientOfVariation"] = m.coefficientOfVariation;
    mm["averageIntegerScore"]    = m.averageIntegerScore;
    mm["geometry"]        = coordStatsToMap(m.geometry);
    mm["horizontalBias"]  = m.horizontalBias;
    mm["verticalBias"]    = m.verticalBias;
    mm["dispersionRatio"] = m.dispersionRatio;
    mm["trendAvailable"]  = m.trendAvailable;
    mm["trendSlope"]      = m.trendSlope;
    mm["trendCorrelation"]= m.trendCorrelation;
    mm["firstHalfAverage"]  = m.firstHalfAverage;
    mm["secondHalfAverage"] = m.secondHalfAverage;
    mm["firstThirdAverage"] = m.firstThirdAverage;
    mm["middleThirdAverage"]= m.middleThirdAverage;
    mm["lastThirdAverage"]  = m.lastThirdAverage;
    mm["timingAvailable"]   = m.timingAvailable;
    mm["averageInterval"]   = m.averageInterval;
    mm["rhythmConsistency"] = m.rhythmConsistency;
    mm["rushedShotCount"]   = m.rushedShotCount;
    mm["delayedShotCount"]  = m.delayedShotCount;
    mm["outlierCount"]      = m.outlierCount;
    mm["outlierPercentage"] = m.outlierPercentage;
    mm["averageOutlierDistance"]  = m.averageOutlierDistance;
    mm["dominantOutlierDirection"]= q(m.dominantOutlierDirection);
    o["measurements"] = mm;

    o["recovery"] = recoverySummaryToMap(p.recovery);
    o["metrics"]  = metricList(p.metrics);

    o["qualityAvailable"] = p.qualityAvailable;
    o["qualityScore"]     = p.qualityScore;
    o["scoreComponent"]       = p.scoreComponent;
    o["consistencyComponent"] = p.consistencyComponent;
    o["geometryComponent"]    = p.geometryComponent;
    o["trendComponent"]       = p.trendComponent;
    o["recoveryComponent"]    = p.recoveryComponent;
    o["timingComponent"]      = p.timingComponent;
    o["pointsLost"]           = p.pointsLost;
    o["strengths"]     = evidenceList(p.strengths);
    o["weaknesses"]    = evidenceList(p.weaknesses);
    o["linkedMetrics"] = stringList(p.linkedMetrics);
    return o;
}

} // namespace

// ============================ inbound =======================================

ShotAnalyticsData shotFromVariant(const QVariantMap& m)
{
    ShotAnalyticsData s;
    s.shotNumber       = m.value("shotNumber", 0).toInt();
    s.seriesNumber     = m.value("seriesNumber", 0).toInt();
    s.decimalScore     = m.value("decimalScore", 0.0).toDouble();
    s.integerScore     = m.value("integerScore", 0).toInt();
    s.hasIntegerScore  = m.value("hasIntegerScore", false).toBool();
    s.x                = m.value("x", 0.0).toDouble();
    s.y                = m.value("y", 0.0).toDouble();
    s.timestamp        = m.value("timestamp", 0.0).toDouble();
    s.hasTimestamp     = m.value("hasTimestamp", false).toBool();
    s.positionType     = parsePosition(m.value("position", "Unknown").toString());
    s.isValid          = m.value("isValid", true).toBool();
    s.isSighter        = m.value("isSighter", false).toBool();
    s.isCompetitionShot= m.value("isCompetitionShot", true).toBool();
    s.penalty          = m.value("penalty", 0.0).toDouble();
    s.hasPenalty       = m.value("hasPenalty", false).toBool();
    return s;
}

std::vector<ShotAnalyticsData> shotsFromVariant(const QVariantList& list)
{
    std::vector<ShotAnalyticsData> out;
    out.reserve(static_cast<size_t>(list.size()));
    for (const QVariant& v : list) out.push_back(shotFromVariant(v.toMap()));
    return out;
}

QVariantList shotsToVariant(const std::vector<ShotAnalyticsData>& shots)
{
    QVariantList l;
    for (const auto& s : shots) {
        QVariantMap m;
        m["shotNumber"]        = s.shotNumber;
        m["seriesNumber"]      = s.seriesNumber;
        m["x"]                 = s.x;             // millimetres
        m["y"]                 = s.y;
        m["decimalScore"]      = s.decimalScore;
        m["position"]          = q(toString(s.positionType));
        m["isCompetitionShot"] = s.isCompetitionShot;
        m["isSighter"]         = s.isSighter;
        m["hasTimestamp"]      = s.hasTimestamp;
        m["timestamp"]         = s.timestamp;
        l << m;
    }
    return l;
}

// ============================ sections ======================================

QVariantMap executiveSummaryMap(const ExecutiveSummary& s)
{
    QVariantMap o;
    o["competitionShotCount"]   = s.competitionShotCount;
    o["totalScore"]             = s.totalScore;
    o["averageScore"]           = s.averageScore;
    o["bestShot"]               = s.bestShot;
    o["worstShot"]              = s.worstShot;
    o["scoreStandardDeviation"] = s.scoreStandardDeviation;
    o["consistencyPercentage"]  = s.consistencyPercentage;
    o["groupCentreX"]           = s.groupCentreX;
    o["groupCentreY"]           = s.groupCentreY;
    o["groupRadius"]            = s.groupRadius;
    o["groupDiameter"]          = s.groupDiameter;
    o["horizontalBias"]         = s.horizontalBias;
    o["verticalBias"]           = s.verticalBias;
    o["horizontalSD"]           = s.horizontalSD;
    o["verticalSD"]             = s.verticalSD;
    o["firstHalfAverage"]       = s.firstHalfAverage;
    o["secondHalfAverage"]      = s.secondHalfAverage;
    o["firstThirdAverage"]      = s.firstThirdAverage;
    o["lastThirdAverage"]       = s.lastThirdAverage;
    o["trendSlope"]             = s.trendSlope;
    o["trendDirection"]         = q(toString(s.trendDirection));
    o["fatigueLevel"]           = q(s.fatigueLevel);
    o["recoveryRating"]         = q(s.recoveryRating);
    o["mainTrainingPriority"]   = q(s.mainTrainingPriority);
    return o;
}

QVariantMap shotDistributionMap(const ShotDistribution& s)
{
    QVariantMap o;
    o["totalAnalysedShots"] = s.totalAnalysedShots;
    o["perfectCount"]    = s.perfectCount;
    o["excellentCount"]  = s.excellentCount;
    o["goodCount"]       = s.goodCount;
    o["acceptableCount"] = s.acceptableCount;
    o["recoveryCount"]   = s.recoveryCount;
    o["poorCount"]       = s.poorCount;
    o["perfectPct"]    = s.perfectPct;
    o["excellentPct"]  = s.excellentPct;
    o["goodPct"]       = s.goodPct;
    o["acceptablePct"] = s.acceptablePct;
    o["recoveryPct"]   = s.recoveryPct;
    o["poorPct"]       = s.poorPct;
    o["bestShot"]  = s.bestShot;
    o["worstShot"] = s.worstShot;
    o["countAtLeast10_5"] = s.countAtLeast10_5;
    o["countAtLeast10_7"] = s.countAtLeast10_7;
    o["countAtLeast10_8"] = s.countAtLeast10_8;
    o["countBelow10_0"]   = s.countBelow10_0;
    o["countBelowAverage"]        = s.countBelowAverage;
    o["countBelowAverageMinusSD"] = s.countBelowAverageMinusSD;
    o["bestStreak10_5"]    = s.bestStreak10_5;
    o["bestStreak10_7"]    = s.bestStreak10_7;
    o["longestPoorStreak"] = s.longestPoorStreak;
    o["binWidth"] = s.binWidth;
    QVariantList hist;
    for (const auto& b : s.histogram) {
        QVariantMap m;
        m["lowerEdge"] = b.lowerEdge;
        m["upperEdge"] = b.upperEdge;
        m["count"]     = b.count;
        hist << m;
    }
    o["histogram"] = hist;
    return o;
}

QVariantMap trendAnalysisMap(const TrendAnalysis& s)
{
    QVariantMap o;
    o["scores"] = doubleList(s.scores);
    o["rolling5Available"]  = s.rolling5Available;
    o["rolling5Average"]    = doubleList(s.rolling5Average);
    o["rolling5SD"]         = doubleList(s.rolling5SD);
    o["rolling10Available"] = s.rolling10Available;
    o["rolling10Average"]   = doubleList(s.rolling10Average);
    o["rolling10SD"]        = doubleList(s.rolling10SD);
    o["first10Available"] = s.first10Available;
    o["first10Average"]   = s.first10Average;
    o["last10Available"]  = s.last10Available;
    o["last10Average"]    = s.last10Average;
    o["halvesAvailable"]   = s.halvesAvailable;
    o["firstHalfAverage"]  = s.firstHalfAverage;
    o["secondHalfAverage"] = s.secondHalfAverage;
    o["thirdsAvailable"]    = s.thirdsAvailable;
    o["firstThirdAverage"]  = s.firstThirdAverage;
    o["middleThirdAverage"] = s.middleThirdAverage;
    o["lastThirdAverage"]   = s.lastThirdAverage;
    o["regressionAvailable"]   = s.regressionAvailable;
    o["regressionSlope"]       = s.regressionSlope;
    o["regressionCorrelation"] = s.regressionCorrelation;
    o["trendDirection"] = q(toString(s.trendDirection));
    o["deteriorationDeterminable"] = s.deteriorationDeterminable;
    o["deteriorationFlag"]         = s.deteriorationFlag;
    o["lateSessionDrop"]     = s.lateSessionDrop;
    o["lateSessionDropFlag"] = s.lateSessionDropFlag;
    return o;
}

QVariantMap heatMapAnalysisMap(const HeatMapAnalysis& s)
{
    QVariantMap o;
    o["available"] = s.available;
    o["binSize"]   = s.binSize;
    o["allShots"]    = gridToMap(s.allShots);
    o["firstHalf"]   = gridToMap(s.firstHalf);
    o["secondHalf"]  = gridToMap(s.secondHalf);
    o["firstThird"]  = gridToMap(s.firstThird);
    o["middleThird"] = gridToMap(s.middleThird);
    o["lastThird"]   = gridToMap(s.lastThird);
    o["goodShots"]   = gridToMap(s.goodShots);
    o["poorShots"]   = gridToMap(s.poorShots);
    o["prone"]     = gridToMap(s.prone);
    o["kneeling"]  = gridToMap(s.kneeling);
    o["standing"]  = gridToMap(s.standing);
    o["airRifle"]  = gridToMap(s.airRifle);
    o["airPistol"] = gridToMap(s.airPistol);
    o["unknownPosition"] = gridToMap(s.unknownPosition);
    o["firstToSecondHalf"] = comparisonToMap(s.firstToSecondHalf);
    o["firstToLastThird"]  = comparisonToMap(s.firstToLastThird);
    o["horizontalDriftHalves"] = s.horizontalDriftHalves;
    o["verticalDriftHalves"]   = s.verticalDriftHalves;
    o["horizontalDriftThirds"] = s.horizontalDriftThirds;
    o["verticalDriftThirds"]   = s.verticalDriftThirds;
    return o;
}

QVariantMap timingAnalysisMap(const TimingAnalysis& s)
{
    QVariantMap o;
    o["available"]      = s.available;
    o["timedShotCount"] = s.timedShotCount;
    o["intervalCount"]  = s.intervalCount;
    o["intervals"]      = doubleList(s.intervals);
    o["statsAvailable"]  = s.statsAvailable;
    o["averageInterval"] = s.averageInterval;
    o["medianInterval"]  = s.medianInterval;
    o["intervalSD"]      = s.intervalSD;
    o["fastestInterval"] = s.fastestInterval;
    o["slowestInterval"] = s.slowestInterval;
    o["rolling3Available"] = s.rolling3Available;
    o["rolling3Average"]   = doubleList(s.rolling3Average);
    o["rolling3SD"]        = doubleList(s.rolling3SD);
    o["rolling5Available"] = s.rolling5Available;
    o["rolling5Average"]   = doubleList(s.rolling5Average);
    o["rolling5SD"]        = doubleList(s.rolling5SD);
    o["rushedShotCount"]   = s.rushedShotCount;
    o["delayedShotCount"]  = s.delayedShotCount;
    o["rushedShotNumbers"] = intList(s.rushedShotNumbers);
    o["delayedShotNumbers"]= intList(s.delayedShotNumbers);
    o["rhythmAvailable"]   = s.rhythmAvailable;
    o["rhythmConsistency"] = s.rhythmConsistency;
    o["trendAvailable"] = s.trendAvailable;
    o["intervalRegressionSlope"]       = s.intervalRegressionSlope;
    o["intervalRegressionCorrelation"] = s.intervalRegressionCorrelation;
    o["intervalTrend"] = q(toString(s.intervalTrend));
    o["scoreIntervalCorrelationAvailable"] = s.scoreIntervalCorrelationAvailable;
    o["scoreVsIntervalCorrelation"]        = s.scoreVsIntervalCorrelation;
    o["recoveryIntervalAvailable"]   = s.recoveryIntervalAvailable;
    o["recoveryIntervalSampleCount"] = s.recoveryIntervalSampleCount;
    o["averageRecoveryInterval"]     = s.averageRecoveryInterval;
    o["decisionBeforeHighAvailable"]   = s.decisionBeforeHighAvailable;
    o["decisionBeforeHighSampleCount"] = s.decisionBeforeHighSampleCount;
    o["averageDecisionTimeBeforeHigh"] = s.averageDecisionTimeBeforeHigh;
    o["decisionBeforePoorAvailable"]   = s.decisionBeforePoorAvailable;
    o["decisionBeforePoorSampleCount"] = s.decisionBeforePoorSampleCount;
    o["averageDecisionTimeBeforePoor"] = s.averageDecisionTimeBeforePoor;
    return o;
}

QVariantMap positionAnalysisMap(const PositionAnalysis& s)
{
    QVariantMap o;
    o["available"]        = s.available;
    o["positionsPresent"] = s.positionsPresent;
    QVariantList positions;
    for (const auto& p : s.positions) positions << positionReportToMap(p);
    o["positions"] = positions;
    o["pointsLostRanking"]  = rankingList(s.pointsLostRanking);
    o["consistencyRanking"] = rankingList(s.consistencyRanking);
    o["geometryRanking"]    = rankingList(s.geometryRanking);
    o["trendRanking"]       = rankingList(s.trendRanking);
    o["recoveryRanking"]    = rankingList(s.recoveryRanking);
    o["timingRanking"]      = rankingList(s.timingRanking);
    o["overallRanking"]     = rankingList(s.overallRanking);
    o["comparativeAvailable"] = s.comparativeAvailable;
    o["hasStrongest"] = s.hasStrongest; o["strongestPosition"] = q(toString(s.strongestPosition));
    o["hasWeakest"]   = s.hasWeakest;   o["weakestPosition"]   = q(toString(s.weakestPosition));
    o["hasMostConsistent"]  = s.hasMostConsistent;  o["mostConsistentPosition"]  = q(toString(s.mostConsistentPosition));
    o["hasLeastConsistent"] = s.hasLeastConsistent; o["leastConsistentPosition"] = q(toString(s.leastConsistentPosition));
    o["hasLargestGroup"]  = s.hasLargestGroup;  o["largestGroupPosition"]  = q(toString(s.largestGroupPosition));
    o["hasSmallestGroup"] = s.hasSmallestGroup; o["smallestGroupPosition"] = q(toString(s.smallestGroupPosition));
    o["hasGreatestImprovement"]   = s.hasGreatestImprovement;   o["greatestImprovementPosition"]   = q(toString(s.greatestImprovementPosition));
    o["hasGreatestDeterioration"] = s.hasGreatestDeterioration; o["greatestDeteriorationPosition"] = q(toString(s.greatestDeteriorationPosition));
    return o;
}

QVariantMap recoveryAnalysisMap(const RecoveryAnalysis& s)
{
    QVariantMap o;
    o["available"] = s.available;
    o["overall"]   = shotRecoveryToMap(s.overall);

    QVariantMap ser;
    ser["available"]      = s.series.available;
    ser["seriesCount"]    = s.series.seriesCount;
    ser["sessionAverage"] = s.series.sessionAverage;
    QVariantList pts;
    for (const auto& p : s.series.series) {
        QVariantMap m;
        m["seriesNumber"] = p.seriesNumber;
        m["shotCount"]    = p.shotCount;
        m["average"]      = p.average;
        m["isDip"]        = p.isDip;
        pts << m;
    }
    ser["series"]            = pts;
    ser["dipCount"]          = s.series.dipCount;
    ser["evaluableDipCount"] = s.series.evaluableDipCount;
    ser["dipReboundCount"]   = s.series.dipReboundCount;
    ser["dipSustainedCount"] = s.series.dipSustainedCount;
    ser["dipReboundRate"]    = s.series.dipReboundRate;
    ser["averageReboundDelta"] = s.series.averageReboundDelta;
    o["series"] = ser;

    QVariantList byPos;
    for (const auto& e : s.byPosition) {
        QVariantMap m;
        m["position"] = q(toString(e.position));
        m["recovery"] = shotRecoveryToMap(e.recovery);
        byPos << m;
    }
    o["byPosition"] = byPos;
    o["metrics"]    = metricList(s.metrics);
    o["comparativeAvailable"] = s.comparativeAvailable;
    o["hasBestRecovery"]  = s.hasBestRecovery;  o["bestRecoveryPosition"]  = q(toString(s.bestRecoveryPosition));
    o["hasWorstRecovery"] = s.hasWorstRecovery; o["worstRecoveryPosition"] = q(toString(s.worstRecoveryPosition));
    return o;
}

QVariantMap fatigueAnalysisMap(const FatigueAnalysis& s)
{
    QVariantMap o;
    o["available"]      = s.available;
    o["overallPattern"] = q(toString(s.overallPattern));
    o["earlyAverage"]   = s.earlyAverage;
    o["middleAverage"]  = s.middleAverage;
    o["lateAverage"]    = s.lateAverage;
    o["earlyLateDelta"] = s.earlyLateDelta;
    o["scoreSlope"]      = s.scoreSlope;
    o["dispersionSlope"] = s.dispersionSlope;
    o["timingSlope"]     = s.timingSlope;
    o["earlyGroupRadius"]   = s.earlyGroupRadius;
    o["lateGroupRadius"]    = s.lateGroupRadius;
    o["groupExpansionRate"] = s.groupExpansionRate;
    o["earlyShotInterval"]  = s.earlyShotInterval;
    o["lateShotInterval"]   = s.lateShotInterval;
    o["timingChangeRate"]   = s.timingChangeRate;
    o["fatigueIndex"] = s.fatigueIndex;
    o["confidence"]   = s.confidence;
    o["scoreTrendAvailable"]      = s.scoreTrendAvailable;
    o["coordinateTrendAvailable"] = s.coordinateTrendAvailable;
    o["timingTrendAvailable"]     = s.timingTrendAvailable;
    QVariantList byPos;
    for (const auto& f : s.byPosition) byPos << positionFatigueToMap(f);
    o["byPosition"] = byPos;
    o["metrics"]    = metricList(s.metrics);
    o["hasFatiguedPosition"]  = s.hasFatiguedPosition;
    o["mostFatiguedPosition"] = q(toString(s.mostFatiguedPosition));
    return o;
}

QVariantList trainingPrioritiesList(const TrainingPriorities& s)
{
    QVariantList l;
    for (const auto& p : s.priorities) {
        QVariantMap m;
        m["area"]          = q(toString(p.area));
        m["priority"]      = q(p.priority);
        m["impact"]        = q(toString(p.impact));
        m["severity"]      = q(toString(p.severity));
        m["priorityScore"] = p.priorityScore;
        m["confidence"]    = p.confidence;
        m["evidence"]      = evidenceList(p.evidence);
        m["linkedMetrics"] = stringList(p.linkedMetrics);
        l << m;
    }
    return l;
}

QVariantMap coachConclusionMap(const CoachConclusion& s)
{
    QVariantMap o;
    o["available"]           = s.available;
    o["rating"]              = q(toString(s.rating));
    o["overallAssessment"]   = q(s.overallAssessment);
    o["keyStrengths"]        = stringList(s.keyStrengths);
    o["mainLimitingFactors"] = stringList(s.mainLimitingFactors);
    o["technicalRiskFlags"]  = stringList(s.technicalRiskFlags);
    o["trainingFocusSummary"]= q(s.trainingFocusSummary);
    o["confidence"]          = s.confidence;
    o["evidence"]            = evidenceList(s.evidence);
    return o;
}

// ============================ diary =========================================

QVariantMap diaryToVariant(const CoachDiary& d)
{
    QVariantMap o;
    o["sessionType"]      = q(toString(d.sessionType));
    o["sessionIntensity"] = q(toString(d.sessionIntensity));
    o["sessionGoal"]   = q(d.sessionGoal);
    o["coachNotes"]    = q(d.coachNotes);
    o["athleteNotes"]  = q(d.athleteNotes);
    o["equipmentNotes"]       = q(d.equipmentNotes);
    o["ammunitionNotes"]      = q(d.ammunitionNotes);
    o["sightAdjustmentNotes"] = q(d.sightAdjustmentNotes);
    o["positionSetupNotes"]   = q(d.positionSetupNotes);
    o["windNotes"]           = q(d.windNotes);
    o["lightingNotes"]       = q(d.lightingNotes);
    o["rangeConditionNotes"] = q(d.rangeConditionNotes);
    o["mentalStateNotes"]        = q(d.mentalStateNotes);
    o["physicalConditionNotes"]  = q(d.physicalConditionNotes);
    o["fatigueNotes"]            = q(d.fatigueNotes);
    o["technicalFocus"]   = q(d.technicalFocus);
    o["nextSessionFocus"] = q(d.nextSessionFocus);
    o["tags"]        = stringList(d.tags);
    o["attachments"] = stringList(d.attachments);
    o["createdAt"] = q(d.createdAt);
    o["updatedAt"] = q(d.updatedAt);
    o["isEmpty"]   = d.isEmpty();
    return o;
}

CoachDiary diaryFromVariant(const QVariantMap& m)
{
    CoachDiary d;
    d.sessionType      = sessionTypeFromString(m.value("sessionType", "Unknown").toString().toStdString());
    d.sessionIntensity = sessionIntensityFromString(m.value("sessionIntensity", "Unknown").toString().toStdString());
    d.sessionGoal   = m.value("sessionGoal").toString().toStdString();
    d.coachNotes    = m.value("coachNotes").toString().toStdString();
    d.athleteNotes  = m.value("athleteNotes").toString().toStdString();
    d.equipmentNotes       = m.value("equipmentNotes").toString().toStdString();
    d.ammunitionNotes      = m.value("ammunitionNotes").toString().toStdString();
    d.sightAdjustmentNotes = m.value("sightAdjustmentNotes").toString().toStdString();
    d.positionSetupNotes   = m.value("positionSetupNotes").toString().toStdString();
    d.windNotes           = m.value("windNotes").toString().toStdString();
    d.lightingNotes       = m.value("lightingNotes").toString().toStdString();
    d.rangeConditionNotes = m.value("rangeConditionNotes").toString().toStdString();
    d.mentalStateNotes       = m.value("mentalStateNotes").toString().toStdString();
    d.physicalConditionNotes = m.value("physicalConditionNotes").toString().toStdString();
    d.fatigueNotes           = m.value("fatigueNotes").toString().toStdString();
    d.technicalFocus   = m.value("technicalFocus").toString().toStdString();
    d.nextSessionFocus = m.value("nextSessionFocus").toString().toStdString();
    for (const QVariant& t : m.value("tags").toList())        d.tags.push_back(t.toString().toStdString());
    for (const QVariant& a : m.value("attachments").toList()) d.attachments.push_back(a.toString().toStdString());
    d.createdAt = m.value("createdAt").toString().toStdString();
    d.updatedAt = m.value("updatedAt").toString().toStdString();
    return d;
}

// ============================ top level =====================================

QVariantMap toVariantMap(const CoachReportData& r)
{
    QVariantMap o;
    o["valid"]   = r.valid;
    o["message"] = q(r.message);
    o["inputShotCount"]       = r.inputShotCount;
    o["invalidRemovedCount"]  = r.invalidRemovedCount;
    o["sighterExcludedCount"] = r.sighterExcludedCount;
    o["analysedShotCount"]    = r.analysedShotCount;
    o["competitionShotCount"] = r.competitionShotCount;
    o["lowSampleWarning"]     = r.lowSampleWarning;
    o["sightersIncluded"]     = r.sightersIncluded;
    if (!r.valid) return o;   // sections are defaults on an invalid report

    o["executiveSummary"]   = executiveSummaryMap(r.executiveSummary);
    o["shotDistribution"]   = shotDistributionMap(r.shotDistribution);
    o["trendAnalysis"]      = trendAnalysisMap(r.trendAnalysis);
    o["heatMapAnalysis"]    = heatMapAnalysisMap(r.heatMapAnalysis);
    o["timingAnalysis"]     = timingAnalysisMap(r.timingAnalysis);
    o["positionAnalysis"]   = positionAnalysisMap(r.positionAnalysis);
    o["recoveryAnalysis"]   = recoveryAnalysisMap(r.recoveryAnalysis);
    o["fatigueAnalysis"]    = fatigueAnalysisMap(r.fatigueAnalysis);
    o["trainingPriorities"] = trainingPrioritiesList(r.trainingPriorities);
    o["coachConclusion"]    = coachConclusionMap(r.coachConclusion);
    o["coachDiary"]         = diaryToVariant(r.manualDiary);
    return o;
}

// ============================ conversions ===================================

QPointF mmToDisplay(double xMm, double yMm, double scale, const QPointF& center, bool flipY)
{
    return QPointF(center.x() + scale * xMm,
                   center.y() + (flipY ? -1.0 : 1.0) * scale * yMm);
}

QDateTime isoToDateTime(const QString& iso)
{
    if (iso.isEmpty()) return QDateTime();
    return QDateTime::fromString(iso, Qt::ISODate);
}

QString dateTimeToIso(const QDateTime& dt)
{
    return dt.isValid() ? dt.toString(Qt::ISODate) : QString();
}

} // namespace bridge
} // namespace techaim
