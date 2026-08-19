#ifndef TA_RMS_LANELISTMODEL_H
#define TA_RMS_LANELISTMODEL_H

// ─────────────────────────────────────────────────────────────────────────────
// THE LIVE RANGE — one row per PHYSICAL LANE, not per observed node.
//
// This is the difference milestone 3 makes. The rows come from the range
// CONFIGURATION, so a ten-lane range has ten rows whether two stations are on
// or all ten. Observation is then joined onto each row by the lane's assigned
// nodeId. A lane whose station is off does not vanish from the range; it reads
// OFFLINE, which is what the range officer actually needs to see.
//
// Three distinct states, deliberately not collapsed into one:
//   NO DEVICE   the lane exists and has no station assigned  (a setup task)
//   OFFLINE     a station is assigned and is not being heard  (an ops problem)
//   <live>      the station's own reported connection state
//
// The join key is the nodeId and nothing else, which is why a station that
// comes back on a new IP with a new bootId lands on its own lane again with
// no operator action.
// ─────────────────────────────────────────────────────────────────────────────

#include "RangeConfigurationService.h"
#include "RangeMonitor.h"

#include <QAbstractListModel>
#include <QVariantList>
#include <QVariantMap>

namespace ta {
namespace rms {

class LaneListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int laneCount READ rowCountProperty NOTIFY summaryChanged)
    Q_PROPERTY(int onlineCount READ onlineCount NOTIFY summaryChanged)
    Q_PROPERTY(int offlineCount READ offlineCount NOTIFY summaryChanged)
    Q_PROPERTY(int unassignedLaneCount READ unassignedLaneCount NOTIFY summaryChanged)
    Q_PROPERTY(int activeSessionCount READ activeSessionCount NOTIFY summaryChanged)

public:
    enum Roles {
        LaneNumberRole = Qt::UserRole + 1,
        LaneLabelRole,
        AssignedNodeIdRole,
        HasDeviceRole,
        OnlineRole,
        OfflineRole,
        LaneEnabledRole,
        ConnectionRole,
        StatusTextRole,
        AthleteRole,
        ProgrammeIdRole,
        ProgrammeLabelRole,
        OfficialRole,
        PhaseRole,
        ShotsLabelRole,
        ScoreLabelRole,
        UnobservedRole,
        GapCountRole
    };

    LaneListModel(RangeConfigurationService* config, RangeMonitor* monitor,
                  QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rowCountProperty() const;
    int onlineCount() const;
    int offlineCount() const;
    int unassignedLaneCount() const;
    int activeSessionCount() const;

    // Everything the lane-detail surface shows, including the milestone-1
    // engineering diagnostics — moved here rather than deleted.
    Q_INVOKABLE QVariantMap laneDetail(int row) const;
    Q_INVOKABLE QVariantList recentShots(int row, int maxCount = 12) const;

    // Plain-text render, used by --dump and by the harness as evidence.
    QString renderTextRange() const;

signals:
    void summaryChanged();

private slots:
    void onRangeChanged();
    void onNodeChanged(const QString& nodeId);

private:
    const LaneDefinition* laneAt(int row) const;
    const TargetNodeRecord* recordForRow(int row) const;

    RangeConfigurationService* m_config = nullptr;
    RangeMonitor* m_monitor = nullptr;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_LANELISTMODEL_H
