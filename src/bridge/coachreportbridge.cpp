#include "coachreportbridge.h"
#include "coachreportvariant.h"

using namespace techaim;

CoachReportBridge::CoachReportBridge(QObject* parent) : QObject(parent) {}

// ---- run the engine ---------------------------------------------------------

bool CoachReportBridge::analyzeShots(const QVariantList& shots)
{
    analyze(bridge::shotsFromVariant(shots));
    return m_report.valid;
}

void CoachReportBridge::analyze(const std::vector<analytics::ShotAnalyticsData>& shots)
{
    // Preserve any human-entered diary across a re-analysis (analytics never
    // touches it, so carry it over rather than dropping the coach's notes).
    analytics::CoachDiary keepDiary = m_report.manualDiary;
    m_report = analytics::CoachAnalyticsEngine::analyze(shots);
    m_report.manualDiary = keepDiary;
    m_shots = shots;                    // kept for the shot-map plot
    emit reportChanged();
}

void CoachReportBridge::setReport(const analytics::CoachReportData& report)
{
    m_report = report;
    emit reportChanged();
}

void CoachReportBridge::clear()
{
    m_report = analytics::CoachReportData();
    m_shots.clear();
    emit reportChanged();
}

// ---- properties -------------------------------------------------------------

QString CoachReportBridge::message() const
{
    return QString::fromStdString(m_report.message);
}

QVariantMap CoachReportBridge::report() const
{
    return bridge::toVariantMap(m_report);
}

// ---- section accessors ------------------------------------------------------

QVariantMap CoachReportBridge::executiveSummary() const
{
    return bridge::executiveSummaryMap(m_report.executiveSummary);
}
QVariantMap CoachReportBridge::shotDistribution() const
{
    return bridge::shotDistributionMap(m_report.shotDistribution);
}
QVariantMap CoachReportBridge::trendAnalysis() const
{
    return bridge::trendAnalysisMap(m_report.trendAnalysis);
}
QVariantMap CoachReportBridge::heatMapAnalysis() const
{
    return bridge::heatMapAnalysisMap(m_report.heatMapAnalysis);
}
QVariantMap CoachReportBridge::timingAnalysis() const
{
    return bridge::timingAnalysisMap(m_report.timingAnalysis);
}
QVariantMap CoachReportBridge::positionAnalysis() const
{
    return bridge::positionAnalysisMap(m_report.positionAnalysis);
}
QVariantMap CoachReportBridge::recoveryAnalysis() const
{
    return bridge::recoveryAnalysisMap(m_report.recoveryAnalysis);
}
QVariantMap CoachReportBridge::fatigueAnalysis() const
{
    return bridge::fatigueAnalysisMap(m_report.fatigueAnalysis);
}
QVariantList CoachReportBridge::trainingPriorities() const
{
    return bridge::trainingPrioritiesList(m_report.trainingPriorities);
}
QVariantMap CoachReportBridge::coachConclusion() const
{
    return bridge::coachConclusionMap(m_report.coachConclusion);
}
QVariantList CoachReportBridge::shots() const
{
    return bridge::shotsToVariant(m_shots);
}

// ---- coach diary ------------------------------------------------------------

QVariantMap CoachReportBridge::coachDiary() const
{
    return bridge::diaryToVariant(m_report.manualDiary);
}

void CoachReportBridge::setCoachDiary(const QVariantMap& diary)
{
    m_report.manualDiary = bridge::diaryFromVariant(diary);
    emit reportChanged();
}

// ---- conversions ------------------------------------------------------------

QPointF CoachReportBridge::mmToDisplay(qreal xMm, qreal yMm) const
{
    return bridge::mmToDisplay(xMm, yMm, m_scale, m_center, m_flipY);
}
QDateTime CoachReportBridge::toDateTime(const QString& iso) const
{
    return bridge::isoToDateTime(iso);
}
QString CoachReportBridge::toIso(const QDateTime& dt) const
{
    return bridge::dateTimeToIso(dt);
}

void CoachReportBridge::setDisplayScale(qreal s)
{
    if (qFuzzyCompare(m_scale, s)) return;
    m_scale = s;
    emit displayTransformChanged();
}
void CoachReportBridge::setDisplayCenter(const QPointF& c)
{
    if (m_center == c) return;
    m_center = c;
    emit displayTransformChanged();
}
void CoachReportBridge::setFlipY(bool f)
{
    if (m_flipY == f) return;
    m_flipY = f;
    emit displayTransformChanged();
}
