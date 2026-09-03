#include "rms/control/ControlStatusModel.h"

namespace ta {
namespace rms {
namespace control {

namespace {
const char* kNotConnected = "NOT CONNECTED";
const char* kAuthenticated = "AUTHENTICATED";
}

ControlStatusModel::ControlStatusModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void ControlStatusModel::setSources(RangeMonitor* monitor,
                                    RangeControlCoordinator* coordinator)
{
    m_monitor = monitor;
    m_coordinator = coordinator;
    refresh();
}

void ControlStatusModel::setTransportAttached(bool attached)
{
    if (m_transportAttached == attached)
        return;
    m_transportAttached = attached;
    refresh();
}

void ControlStatusModel::refresh()
{
    beginResetModel();
    m_rows.clear();
    if (m_monitor) {
        for (int i = 0; i < m_monitor->nodeCount(); ++i) {
            const TargetNodeRecord* rec = m_monitor->nodeAt(i);
            if (!rec)
                continue;
            Row r;
            r.nodeId = rec->nodeId;
            r.laneLabel = rec->laneId.isEmpty() ? rec->nodeId : rec->laneId;
            r.unobserved = rec->unobservedShotCount();
            // WITHOUT A TRANSPORT, every lane reads NOT CONNECTED regardless of
            // what any in-process object believes. The channel state shown to an
            // operator must describe the range, not the software's optimism.
            if (m_transportAttached && m_coordinator) {
                r.authenticated = m_coordinator->isAuthenticated(rec->nodeId);
                r.syncQuality = syncQualityName(
                    m_coordinator->timeSync(rec->nodeId).quality);
                const ReconciliationWatermark w = m_coordinator->watermark(rec->nodeId);
                r.reconciledTo = w.nodeId.isEmpty() ? -1 : w.highestSequence;
            } else {
                r.syncQuality = syncQualityName(SyncQuality::Unusable);
            }
            m_rows.append(r);
        }
    }
    endResetModel();
    emit summaryChanged();
}

int ControlStatusModel::authenticatedCount() const
{
    int n = 0;
    for (const Row& r : m_rows) if (r.authenticated) ++n;
    return n;
}

int ControlStatusModel::unobservedShots() const
{
    int n = 0;
    for (const Row& r : m_rows) n += r.unobserved;
    return n;
}

int ControlStatusModel::lanesBehind() const
{
    int n = 0;
    for (const Row& r : m_rows) if (r.unobserved > 0) ++n;
    return n;
}

QString ControlStatusModel::statusLine() const
{
    if (!m_transportAttached) {
        // The whole point of the banner. No hedging, no "standby", no spinner
        // that suggests it is about to connect.
        return QStringLiteral(
            "CONTROL CHANNEL NOT ENABLED - this build observes only. "
            "Lane commands are not available.");
    }
    if (m_rows.isEmpty())
        return QStringLiteral("Control channel enabled. No lanes are reporting yet.");

    const int auth = authenticatedCount();
    QString s = QStringLiteral("Control channel enabled. %1 of %2 lanes authenticated")
                    .arg(auth).arg(m_rows.size());
    const int behind = lanesBehind();
    if (behind > 0)
        s += QStringLiteral("; %1 lane%2 behind by %3 shot%4 - recovering")
                 .arg(behind).arg(behind == 1 ? "" : "s")
                 .arg(unobservedShots()).arg(unobservedShots() == 1 ? "" : "s");
    return s + QLatin1Char('.');
}

QString ControlStatusModel::tone() const
{
    // Not connected is NEUTRAL, not a warning: a build without a control
    // transport is behaving correctly, and colouring it red would train
    // operators to ignore the colour that means something is wrong.
    if (!m_transportAttached)
        return QStringLiteral("neutral");
    if (m_rows.isEmpty())
        return QStringLiteral("neutral");
    if (authenticatedCount() < m_rows.size())
        return QStringLiteral("warn");
    return lanesBehind() > 0 ? QStringLiteral("warn") : QStringLiteral("live");
}

int ControlStatusModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant ControlStatusModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();
    const Row& r = m_rows.at(index.row());
    switch (role) {
    case NodeIdRole:       return r.nodeId;
    case LaneLabelRole:    return r.laneLabel;
    case ChannelRole:      return QLatin1String(r.authenticated ? kAuthenticated
                                                                : kNotConnected);
    case SyncQualityRole:  return r.syncQuality;
    case UnobservedRole:   return r.unobserved;
    case ReconciledToRole: return r.reconciledTo;
    case ToneRole:
        if (!r.authenticated) return QStringLiteral("neutral");
        return r.unobserved > 0 ? QStringLiteral("warn") : QStringLiteral("live");
    default: break;
    }
    return QVariant();
}

QHash<int, QByteArray> ControlStatusModel::roleNames() const
{
    return {{NodeIdRole, "nodeId"},
            {LaneLabelRole, "laneLabel"},
            {ChannelRole, "channel"},
            {SyncQualityRole, "syncQuality"},
            {UnobservedRole, "unobserved"},
            {ReconciledToRole, "reconciledTo"},
            {ToneRole, "tone"}};
}

} // namespace control
} // namespace rms
} // namespace ta
