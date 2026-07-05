#ifndef COACHREPORTVARIANT_H
#define COACHREPORTVARIANT_H

// ============================================================================
//  Coach-report <-> QVariant marshalling (QtCore only, no QObject / no moc)
// ----------------------------------------------------------------------------
//  This is the ONLY place the pure analytics structs meet Qt types. It performs
//  marshalling exclusively — NO analytics logic, NO recomputation. The engine
//  stays Qt-free; the QObject bridge (coachreportbridge.*) is a thin wrapper
//  around these functions.
//
//  Conversions handled here:
//    std::string        <-> QString
//    std::vector<...>   <-> QVariantList
//    ISO-8601 string    <-> QDateTime
//    raw millimetres    ->  display coordinates (mmToDisplay)
//    enum values        ->  readable labels (via the engine's toString)
//
//  Coordinates are kept in raw MILLIMETRES inside the maps (single source of
//  truth). Pixel conversion is an explicit, separate step (mmToDisplay) so the
//  frontend converts only at draw time.
// ============================================================================

#include <vector>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QPointF>
#include <QDateTime>
#include <QString>

#include "CoachReportData.h"
#include "ShotAnalyticsTypes.h"

namespace techaim {
namespace bridge {

// ---- Inbound: QML/C++ shot descriptions -> engine input ----
analytics::ShotAnalyticsData shotFromVariant(const QVariantMap& m);
std::vector<analytics::ShotAnalyticsData> shotsFromVariant(const QVariantList& list);

// ---- Outbound: full report -> nested QVariantMap tree ----
QVariantMap toVariantMap(const analytics::CoachReportData& r);

// ---- Per-section accessors (used by the bridge's convenience getters) ----
QVariantMap  executiveSummaryMap(const analytics::ExecutiveSummary& s);
QVariantMap  shotDistributionMap(const analytics::ShotDistribution& s);
QVariantMap  trendAnalysisMap(const analytics::TrendAnalysis& s);
QVariantMap  heatMapAnalysisMap(const analytics::HeatMapAnalysis& s);
QVariantMap  timingAnalysisMap(const analytics::TimingAnalysis& s);
QVariantMap  positionAnalysisMap(const analytics::PositionAnalysis& s);
QVariantMap  recoveryAnalysisMap(const analytics::RecoveryAnalysis& s);
QVariantMap  fatigueAnalysisMap(const analytics::FatigueAnalysis& s);
QVariantList trainingPrioritiesList(const analytics::TrainingPriorities& s);
QVariantMap  coachConclusionMap(const analytics::CoachConclusion& s);

// ---- Coach diary (read + write-back of human context) ----
QVariantMap        diaryToVariant(const analytics::CoachDiary& d);
analytics::CoachDiary diaryFromVariant(const QVariantMap& m);

// ---- Unit / type conversions ----
// display = center + scale * (xMm, flipY ? -yMm : yMm)   (screen y grows down)
QPointF   mmToDisplay(double xMm, double yMm, double scale, const QPointF& center, bool flipY);
QDateTime isoToDateTime(const QString& iso);
QString   dateTimeToIso(const QDateTime& dt);

} // namespace bridge
} // namespace techaim

#endif // COACHREPORTVARIANT_H
