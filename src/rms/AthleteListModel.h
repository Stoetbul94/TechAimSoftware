#ifndef TA_RMS_ATHLETELISTMODEL_H
#define TA_RMS_ATHLETELISTMODEL_H

// The start list, for the athlete page and the assignment picker. It also
// reports which lane of the plan being edited each athlete is on, so the
// picker can grey out somebody who is already shooting elsewhere rather than
// offering them and then refusing.

#include "AthleteRegistry.h"

#include <QAbstractListModel>

namespace ta {
namespace rms {

class MatchPlanService;

class AthleteListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCountProperty NOTIFY changed)

public:
    enum Roles {
        AthleteIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        ClubRole,
        CountryRole,
        NotesRole,
        TemporaryRole,
        AssignedLaneRole,   // -1 when not on a lane in the current plan
        AssignedRole
    };

    AthleteListModel(AthleteRegistry* registry, MatchPlanService* plans,
                     QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rowCountProperty() const;

signals:
    void changed();

private slots:
    void refresh();

private:
    AthleteRegistry* m_registry = nullptr;
    MatchPlanService* m_plans = nullptr;
    int m_rows = 0;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_ATHLETELISTMODEL_H
