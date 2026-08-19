#include "PlanLaneModel.h"
#include "ProgrammeDisplay.h"

namespace ta {
namespace rms {

PlanLaneModel::PlanLaneModel(RangeConfigurationService* range, RangeMonitor* monitor,
                             MatchPlanService* plans, AthleteRegistry* athletes,
                             QObject* parent)
    : QAbstractListModel(parent)
    , m_range(range)
    , m_monitor(monitor)
    , m_plans(plans)
    , m_athletes(athletes)
{
    m_rows = m_range->range().laneCount();
    connect(m_range, &RangeConfigurationService::rangeChanged, this, &PlanLaneModel::refresh);
    connect(m_plans, &MatchPlanService::planChanged, this, &PlanLaneModel::refresh);
    connect(m_athletes, &AthleteRegistry::athletesChanged, this, &PlanLaneModel::refresh);
    connect(m_monitor, &RangeMonitor::nodeAdded,   this, &PlanLaneModel::refresh);
    connect(m_monitor, &RangeMonitor::nodeChanged, this, &PlanLaneModel::refresh);
    connect(m_monitor, &RangeMonitor::nodeRemoved, this, &PlanLaneModel::refresh);
}

const LaneDefinition* PlanLaneModel::laneAt(int row) const
{
    const RangeDefinition& r = m_range->range();
    if (row < 0 || row >= r.lanes.size())
        return nullptr;
    return &r.lanes.at(row);
}

int PlanLaneModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_range->range().laneCount();
}

int PlanLaneModel::rowCountProperty() const
{
    return m_range->range().laneCount();
}

void PlanLaneModel::refresh()
{
    const int now = m_range->range().laneCount();
    if (now != m_rows) {
        beginResetModel();
        m_rows = now;
        endResetModel();
    } else if (m_rows > 0) {
        emit dataChanged(index(0, 0), index(m_rows - 1, 0));
    }
    emit changed();
}

QHash<int, QByteArray> PlanLaneModel::roleNames() const
{
    return {
        { LaneNumberRole,        "laneNumber" },
        { LaneLabelRole,         "laneLabel" },
        { SelectedRole,          "selected" },
        { HasDeviceRole,         "hasDevice" },
        { OnlineRole,            "online" },
        { ConnectionRole,        "connection" },
        { AthleteIdRole,         "athleteId" },
        { AthleteNameRole,       "athleteName" },
        { ReadinessRole,         "readiness" },
        { ReadyRole,             "ready" },
        { ObservedProgrammeRole, "observedProgramme" },
        { ProgrammeMatchRole,    "programmeMatch" }
    };
}

QVariant PlanLaneModel::data(const QModelIndex& index, int role) const
{
    const LaneDefinition* lane = laneAt(index.row());
    if (!lane)
        return QVariant();
    const TargetNodeRecord* r = lane->isAssigned()
                                    ? m_monitor->nodeById(lane->assignedNodeId)
                                    : nullptr;
    const bool online = r && !r->isOffline();
    const QString athleteId = m_plans->athleteOnLane(lane->laneNumber);

    switch (role) {
    case LaneNumberRole: return lane->laneNumber;
    case LaneLabelRole:  return lane->label();
    case SelectedRole:   return m_plans->isLaneSelected(lane->laneNumber);
    case HasDeviceRole:  return lane->isAssigned();
    case OnlineRole:     return online;
    case ConnectionRole:
        if (!lane->isAssigned())
            return QStringLiteral("NO DEVICE");
        return r ? toString(r->connection) : QStringLiteral("OFFLINE");
    case AthleteIdRole:   return athleteId;
    case AthleteNameRole: return m_athletes ? m_athletes->displayNameFor(athleteId)
                                            : QString();
    case ReadinessRole:   return toString(m_plans->laneReadiness(lane->laneNumber));
    case ReadyRole:       return m_plans->laneReadiness(lane->laneNumber)
                                     == LaneReadiness::Ready;
    // What the STATION says it is set to, as distinct from what the plan says.
    case ObservedProgrammeRole:
        return r ? ProgrammeDisplay::describe(r->programmeId) : QString();
    case ProgrammeMatchRole:
        return toString(m_plans->programmeMatchForLane(lane->laneNumber));
    default: return QVariant();
    }
}

} // namespace rms
} // namespace ta
