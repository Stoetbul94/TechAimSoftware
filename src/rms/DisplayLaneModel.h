#ifndef TA_RMS_DISPLAYLANEMODEL_H
#define TA_RMS_DISPLAYLANEMODEL_H

// ─────────────────────────────────────────────────────────────────────────────
// WHAT A TARGET DISPLAY DRAWS. One row per lane in the display's ordered set,
// carrying everything the renderer needs and nothing an engineer would want.
//
// ═══ SCORES ARE TRANSPORTED, NEVER COMPUTED ════════════════════════════════
//
// Every score here came from the node with the shot. There is no arithmetic on
// coordinates anywhere in this file, and the only addition performed is the
// OBSERVED sum — which is labelled as observed, sits beside the node's
// authoritative total, and is never presented as a result.
//
// ═══ WHAT RMS DID NOT SEE, IT SAYS IT DID NOT SEE ══════════════════════════
//
// When the node has accepted more shots than RMS received, the difference is
// reported as unseen and the rendered face is explicitly not the whole match.
// No coordinate is ever invented to fill a gap: the node kept shooting safely,
// RMS simply lacks those impact positions.
// ─────────────────────────────────────────────────────────────────────────────

#include "AthleteRegistry.h"
#include "DisplayController.h"
#include "MatchPlanService.h"
#include "RangeConfigurationService.h"
#include "RangeMonitor.h"

#include <QAbstractListModel>
#include <QVariantList>
#include <QVariantMap>

namespace ta {
namespace rms {

class DisplayLaneModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCountProperty NOTIFY changed)

public:
    // How many observed shots a target face shows. Bounded so a long course
    // cannot grow the drawing without limit, and identical in the small card
    // and the full-screen view so a shot means the same thing at both scales.
    static constexpr int kVisibleShots = 30;

    enum Roles {
        LaneNumberRole = Qt::UserRole + 1,
        LaneLabelRole,
        HasDeviceRole,
        OnlineRole,
        ConnectionRole,
        StatusTextRole,
        // Athlete: the plan's intention and the station's report, kept apart.
        AthleteRole,
        PlannedAthleteRole,
        ObservedAthleteRole,
        AthleteMismatchRole,
        // Programme: same discipline.
        ProgrammeLabelRole,
        PlannedProgrammeRole,
        ProgrammeMismatchRole,
        PhaseRole,
        PositionRole,
        // Target face.
        TargetStandardIdRole,
        TargetStandardNameRole,
        TargetSupportedRole,
        ShotsRole,                 // [{x, y, score, sequence, last}] normalised
        // Counts and scores.
        ShotsAcceptedRole,         // the node's own accepted count
        ShotsExpectedRole,
        ObservedShotCountRole,
        UnseenShotCountRole,
        ShotsLabelRole,
        NodeTotalLabelRole,        // authoritative running total
        ObservedTotalLabelRole,    // sum of what RMS actually saw
        LastShotScoreRole,
        LastShotSequenceRole,
        // Competition axis.
        CompetitionStatusRole,
        CompetitionTerminalRole,
        EliminatedRole,
        FinishedRole,
        FinalRankLabelRole,
        FinalScoreLabelRole,
        CompetitionSimulatedRole
    };

    DisplayLaneModel(RangeConfigurationService* range, RangeMonitor* monitor,
                     MatchPlanService* plans, AthleteRegistry* athletes,
                     DisplayController* display, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rowCountProperty() const;
    // Everything for one lane, by LANE NUMBER — what the single-target view
    // binds to, so it never has to know a row index.
    Q_INVOKABLE QVariantMap laneByNumber(int laneNumber) const;
    Q_INVOKABLE QVariantMap laneAtRow(int row) const;

signals:
    void changed();

private slots:
    void rebuild();

private:
    QVariantMap buildLane(int laneNumber) const;

    RangeConfigurationService* m_range = nullptr;
    RangeMonitor* m_monitor = nullptr;
    MatchPlanService* m_plans = nullptr;
    AthleteRegistry* m_athletes = nullptr;
    DisplayController* m_display = nullptr;
    QVector<int> m_rows;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_DISPLAYLANEMODEL_H
