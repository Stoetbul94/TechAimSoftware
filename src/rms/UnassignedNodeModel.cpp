#include "UnassignedNodeModel.h"

namespace ta {
namespace rms {

UnassignedNodeModel::UnassignedNodeModel(RangeConfigurationService* config,
                                         RangeMonitor* monitor, QObject* parent)
    : QAbstractListModel(parent)
    , m_config(config)
    , m_monitor(monitor)
{
    connect(m_config, &RangeConfigurationService::rangeChanged,
            this, &UnassignedNodeModel::rebuild);
    connect(m_monitor, &RangeMonitor::nodeAdded,   this, &UnassignedNodeModel::rebuild);
    connect(m_monitor, &RangeMonitor::nodeChanged, this, &UnassignedNodeModel::rebuild);
    connect(m_monitor, &RangeMonitor::nodeRemoved, this, &UnassignedNodeModel::rebuild);
    rebuild();
}

void UnassignedNodeModel::rebuild()
{
    QVector<QString> next;
    for (const QString& nodeId : m_monitor->nodeIds())
        if (!m_config->isNodeAssigned(nodeId))
            next.append(nodeId);

    if (next == m_rows) {
        // The membership is unchanged; only a row's contents can have moved.
        if (!m_rows.isEmpty())
            emit dataChanged(index(0, 0), index(m_rows.size() - 1, 0));
        return;
    }
    beginResetModel();
    m_rows = next;
    endResetModel();
    emit countChanged();
}

int UnassignedNodeModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QStringList UnassignedNodeModel::nodeIds() const
{
    QStringList out;
    for (const QString& id : m_rows)
        out << id;
    return out;
}

QHash<int, QByteArray> UnassignedNodeModel::roleNames() const
{
    return {
        { NodeIdRole,          "nodeId" },
        { ShortIdRole,         "shortId" },
        { ConnectionRole,      "connection" },
        { OfflineRole,         "offline" },
        { DeviceIdentityRole,  "deviceIdentity" },
        { AppVersionRole,      "appVersion" },
        { ProductIdentityRole, "productIdentity" },
        { LaneHintRole,        "laneHint" },
        { PhaseRole,           "phase" }
    };
}

QVariant UnassignedNodeModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();
    const QString nodeId = m_rows.at(index.row());
    const TargetNodeRecord* r = m_monitor->nodeById(nodeId);

    switch (role) {
    case NodeIdRole:  return nodeId;
    case ShortIdRole: return nodeId.right(8);
    case ConnectionRole:      return r ? toString(r->connection) : QStringLiteral("OFFLINE");
    case OfflineRole:         return !r || r->isOffline();
    case DeviceIdentityRole:  return r ? r->deviceIdentity : QString();
    case AppVersionRole:      return r ? r->appVersion : QString();
    case ProductIdentityRole: return r ? r->productIdentity : QString();
    // What the station was told its lane is, locally. PROVISIONAL: it is a
    // hint for the operator doing the assignment, never a mapping. The
    // authoritative lane is the one configured here.
    case LaneHintRole:        return r ? r->laneId : QString();
    case PhaseRole:           return r ? toString(r->phase) : QString();
    default:                  return QVariant();
    }
}

} // namespace rms
} // namespace ta
