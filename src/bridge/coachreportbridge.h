#ifndef COACHREPORTBRIDGE_H
#define COACHREPORTBRIDGE_H

// ============================================================================
//  CoachReportBridge — the QML boundary object (context property COACHREPORT)
// ----------------------------------------------------------------------------
//  A THIN wrapper around the pure analytics engine and the QtCore marshalling
//  functions in coachreportvariant.*. It holds the last CoachReportData, runs
//  the engine on inbound shots, and exposes the report to QML as QVariant
//  trees. It contains NO analytics logic and never recomputes a section — QML
//  may format, filter, collapse and convert units, but not analyse.
//
//  The analytics engine (src/analytics/*) remains completely Qt-free; this is
//  the single place Qt and the engine meet.
// ============================================================================

#include <vector>
#include <QObject>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QPointF>
#include <QDateTime>

#include "CoachAnalyticsEngine.h"
#include "CoachReportData.h"
#include "ShotAnalyticsTypes.h"

class CoachReportBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool        valid   READ valid   NOTIFY reportChanged)
    Q_PROPERTY(QString     message READ message NOTIFY reportChanged)
    Q_PROPERTY(QVariantMap report  READ report  NOTIFY reportChanged)
    // Display transform for mm -> screen pixels (set by the frontend view).
    Q_PROPERTY(qreal   displayScale  READ displayScale  WRITE setDisplayScale  NOTIFY displayTransformChanged)
    Q_PROPERTY(QPointF displayCenter READ displayCenter WRITE setDisplayCenter NOTIFY displayTransformChanged)
    Q_PROPERTY(bool    flipY         READ flipY         WRITE setFlipY         NOTIFY displayTransformChanged)

public:
    explicit CoachReportBridge(QObject* parent = nullptr);

    // ---- run the engine (thin: convert -> analyze -> store -> notify) ----
    // Inbound shots from QML/JS as a list of maps (see shotFromVariant keys).
    Q_INVOKABLE bool analyzeShots(const QVariantList& shots);
    // C++-side entry for the real data source (Step 2): already-built shots.
    void analyze(const std::vector<techaim::analytics::ShotAnalyticsData>& shots);
    // C++-side entry for a report produced elsewhere (e.g. loaded from file).
    void setReport(const techaim::analytics::CoachReportData& report);
    Q_INVOKABLE void clear();

    // ---- section accessors (convenience; identical data to report()) ----
    Q_INVOKABLE QVariantMap  executiveSummary() const;
    Q_INVOKABLE QVariantMap  shotDistribution() const;
    Q_INVOKABLE QVariantMap  trendAnalysis() const;
    Q_INVOKABLE QVariantMap  heatMapAnalysis() const;
    Q_INVOKABLE QVariantMap  timingAnalysis() const;
    Q_INVOKABLE QVariantMap  positionAnalysis() const;
    Q_INVOKABLE QVariantMap  recoveryAnalysis() const;
    Q_INVOKABLE QVariantMap  fatigueAnalysis() const;
    Q_INVOKABLE QVariantList trainingPriorities() const;
    Q_INVOKABLE QVariantMap  coachConclusion() const;

    // Raw analysed shots for plotting the target/shot-map (mm coordinates,
    // score, position). Display data — QML converts units via mmToDisplay.
    Q_INVOKABLE QVariantList shots() const;

    // ---- coach diary (human context: read + write-back) ----
    Q_INVOKABLE QVariantMap coachDiary() const;
    Q_INVOKABLE void setCoachDiary(const QVariantMap& diary);

    // ---- conversions exposed to QML ----
    Q_INVOKABLE QPointF   mmToDisplay(qreal xMm, qreal yMm) const;   // uses display transform
    Q_INVOKABLE QDateTime toDateTime(const QString& iso) const;
    Q_INVOKABLE QString   toIso(const QDateTime& dt) const;

    // property getters/setters
    bool        valid() const   { return m_report.valid; }
    QString     message() const;
    QVariantMap report() const;
    qreal   displayScale() const  { return m_scale; }
    QPointF displayCenter() const { return m_center; }
    bool    flipY() const         { return m_flipY; }
    void setDisplayScale(qreal s);
    void setDisplayCenter(const QPointF& c);
    void setFlipY(bool f);

signals:
    void reportChanged();
    void displayTransformChanged();

private:
    techaim::analytics::CoachReportData m_report;
    std::vector<techaim::analytics::ShotAnalyticsData> m_shots;  // for the shot-map
    qreal   m_scale  = 4.0;              // pixels per millimetre
    QPointF m_center = QPointF(0.0, 0.0);
    bool    m_flipY  = true;             // screen y grows down; target y grows up
};

#endif // COACHREPORTBRIDGE_H
