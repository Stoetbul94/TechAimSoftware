#ifndef TA_RMS_RANGELISTMODEL_H
#define TA_RMS_RANGELISTMODEL_H

// ─────────────────────────────────────────────────────────────────────────────
// The dashboard's model. A read-only projection of RangeMonitor: every role
// is a getter, there is no setData, and no Q_INVOKABLE takes an action — only
// queries. QML can therefore not reach a control path even by accident.
//
// QtCore only (QAbstractListModel lives in QtCore), so the model is exercised
// in the GUI-free test harness exactly as the UI sees it. The "dashboard
// evidence" in the harness is this model's own rows.
// ─────────────────────────────────────────────────────────────────────────────

#include "RangeMonitor.h"

#include <QAbstractListModel>
#include <QVariantList>
#include <QVariantMap>

namespace ta {
namespace rms {

class RangeListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int nodeCount READ rowCountProperty NOTIFY summaryChanged)
    Q_PROPERTY(int onlineCount READ onlineCount NOTIFY summaryChanged)
    Q_PROPERTY(int offlineCount READ offlineCount NOTIFY summaryChanged)
    Q_PROPERTY(int rejectedDatagrams READ rejectedDatagrams NOTIFY summaryChanged)
    // Always true in this build. Bound by the UI so the read-only boundary is
    // stated on screen, not just in a document.
    Q_PROPERTY(bool readOnly READ readOnly CONSTANT)

public:
    enum Roles {
        NodeIdRole = Qt::UserRole + 1,
        LaneLabelRole,
        AthleteRole,
        ProgrammeIdRole,
        ProgrammeLabelRole,
        RulesetIdRole,
        TargetStandardIdRole,
        OfficialRole,
        PositionRole,
        PhaseRole,
        ConnectionRole,
        OfflineRole,
        ShotsAcceptedRole,
        ShotsExpectedRole,
        ShotsLabelRole,
        TotalScoreRole,
        ScoreLabelRole,
        UnobservedRole,
        DuplicatesRole,
        OutOfOrderRole,
        GapCountRole,
        RestartsRole,
        HealthRole,
        DeviceIdentityRole
    };

    explicit RangeListModel(RangeMonitor* monitor, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rowCountProperty() const { return m_rows.size(); }
    int onlineCount() const;
    int offlineCount() const;
    int rejectedDatagrams() const { return m_monitor->rejectedDatagrams(); }
    bool readOnly() const { return true; }

    // Queries only — the detail pane's data. Nothing here mutates a node.
    Q_INVOKABLE QVariantMap nodeDetail(int row) const;
    Q_INVOKABLE QVariantList recentShots(int row, int maxCount = 12) const;

    // Plain-text render of the whole dashboard. Used by the harness as
    // dashboard evidence and by --dump in the application.
    QString renderTextDashboard() const;

signals:
    void summaryChanged();

private slots:
    void onNodeAdded(const QString& nodeId);
    void onNodeChanged(const QString& nodeId);
    void onMonitorReset();

private:
    int rowOf(const QString& nodeId) const;
    const TargetNodeRecord* recordAt(int row) const;

    RangeMonitor* m_monitor = nullptr;
    QVector<QString> m_rows;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_RANGELISTMODEL_H
