#ifndef TA_RMS_PLANLANEMODEL_H
#define TA_RMS_PLANLANEMODEL_H

// Every PHYSICAL lane of the configured range, with the plan's view of it
// alongside. One model serves both the lane-selection step and the athlete
// assignment step, because they are two views of the same question: which
// lanes are taking part, and who is on them.
//
// OFFLINE LANES ARE NOT HIDDEN. A tablet may be switched on minutes before the
// start, so the operator may deliberately include a lane that is not answering
// yet — with the warning shown, never suppressed.

#include "MatchPlanService.h"

#include <QAbstractListModel>

namespace ta {
namespace rms {

class PlanLaneModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCountProperty NOTIFY changed)

public:
    enum Roles {
        LaneNumberRole = Qt::UserRole + 1,
        LaneLabelRole,
        SelectedRole,
        HasDeviceRole,
        OnlineRole,
        ConnectionRole,
        AthleteIdRole,
        AthleteNameRole,
        ReadinessRole,
        ReadyRole,
        ObservedProgrammeRole,
        ProgrammeMatchRole
    };

    PlanLaneModel(RangeConfigurationService* range, RangeMonitor* monitor,
                  MatchPlanService* plans, AthleteRegistry* athletes,
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
    const LaneDefinition* laneAt(int row) const;

    RangeConfigurationService* m_range = nullptr;
    RangeMonitor* m_monitor = nullptr;
    MatchPlanService* m_plans = nullptr;
    AthleteRegistry* m_athletes = nullptr;
    int m_rows = 0;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_PLANLANEMODEL_H
