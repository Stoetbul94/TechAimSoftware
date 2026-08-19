#include "AthleteListModel.h"
#include "MatchPlanService.h"

namespace ta {
namespace rms {

AthleteListModel::AthleteListModel(AthleteRegistry* registry, MatchPlanService* plans,
                                   QObject* parent)
    : QAbstractListModel(parent)
    , m_registry(registry)
    , m_plans(plans)
{
    m_rows = m_registry->count();
    connect(m_registry, &AthleteRegistry::athletesChanged, this, &AthleteListModel::refresh);
    if (m_plans)
        connect(m_plans, &MatchPlanService::planChanged, this, &AthleteListModel::refresh);
}

int AthleteListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_registry->count();
}

int AthleteListModel::rowCountProperty() const
{
    return m_registry->count();
}

void AthleteListModel::refresh()
{
    const int now = m_registry->count();
    if (now != m_rows) {
        beginResetModel();
        m_rows = now;
        endResetModel();
    } else if (m_rows > 0) {
        emit dataChanged(index(0, 0), index(m_rows - 1, 0));
    }
    emit changed();
}

QHash<int, QByteArray> AthleteListModel::roleNames() const
{
    return {
        { AthleteIdRole,    "athleteId" },
        { DisplayNameRole,  "displayName" },
        { ClubRole,         "club" },
        { CountryRole,      "country" },
        { NotesRole,        "notes" },
        { TemporaryRole,    "temporary" },
        { AssignedLaneRole, "assignedLane" },
        { AssignedRole,     "assigned" }
    };
}

QVariant AthleteListModel::data(const QModelIndex& index, int role) const
{
    const QVariantMap a = m_registry->athleteAt(index.row());
    if (a.isEmpty())
        return QVariant();
    const QString id = a.value(QStringLiteral("athleteId")).toString();
    const int lane = m_plans ? m_plans->laneNumberForAthlete(id) : -1;

    switch (role) {
    case AthleteIdRole:    return id;
    case DisplayNameRole:  return a.value(QStringLiteral("displayName"));
    case ClubRole:         return a.value(QStringLiteral("club"));
    case CountryRole:      return a.value(QStringLiteral("country"));
    case NotesRole:        return a.value(QStringLiteral("notes"));
    case TemporaryRole:    return a.value(QStringLiteral("temporary"));
    case AssignedLaneRole: return lane;
    case AssignedRole:     return lane >= 0;
    default:               return QVariant();
    }
}

} // namespace rms
} // namespace ta
