#ifndef TA_RMS_UNASSIGNEDNODEMODEL_H
#define TA_RMS_UNASSIGNEDNODEMODEL_H

// ─────────────────────────────────────────────────────────────────────────────
// Stations RMS can hear that no lane claims.
//
// A DISCOVERED DEVICE IS NOT A LANE. RMS refuses to invent a physical firing
// point from a datagram: a tablet on the bench, a spare in the office and a
// station that has been moved to another range would all become phantom lanes
// on the range officer's screen. Discovery puts a device HERE, and an operator
// decides which physical lane — if any — it is standing on.
// ─────────────────────────────────────────────────────────────────────────────

#include "RangeConfigurationService.h"
#include "RangeMonitor.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QVector>

namespace ta {
namespace rms {

class UnassignedNodeModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCountProperty NOTIFY countChanged)

public:
    enum Roles {
        NodeIdRole = Qt::UserRole + 1,
        ShortIdRole,
        ConnectionRole,
        OfflineRole,
        DeviceIdentityRole,
        AppVersionRole,
        ProductIdentityRole,
        LaneHintRole,       // the lane name the station was told locally, if any
        PhaseRole
    };

    UnassignedNodeModel(RangeConfigurationService* config, RangeMonitor* monitor,
                        QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rowCountProperty() const { return int(m_rows.size()); }
    // Discovered order — the order a temporary range would number them in.
    Q_INVOKABLE QStringList nodeIds() const;

signals:
    void countChanged();

private slots:
    void rebuild();

private:
    RangeConfigurationService* m_config = nullptr;
    RangeMonitor* m_monitor = nullptr;
    QVector<QString> m_rows;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_UNASSIGNEDNODEMODEL_H
