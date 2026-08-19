#include "DisplayLaneModel.h"
#include "ProgrammeDisplay.h"
#include "TargetGeometry.h"

namespace ta {
namespace rms {

DisplayLaneModel::DisplayLaneModel(RangeConfigurationService* range, RangeMonitor* monitor,
                                   MatchPlanService* plans, AthleteRegistry* athletes,
                                   DisplayController* display, QObject* parent)
    : QAbstractListModel(parent)
    , m_range(range)
    , m_monitor(monitor)
    , m_plans(plans)
    , m_athletes(athletes)
    , m_display(display)
{
    connect(m_display, &DisplayController::changed, this, &DisplayLaneModel::rebuild);
    connect(m_range, &RangeConfigurationService::rangeChanged, this, &DisplayLaneModel::rebuild);
    connect(m_plans, &MatchPlanService::planChanged, this, &DisplayLaneModel::rebuild);
    connect(m_athletes, &AthleteRegistry::athletesChanged, this, &DisplayLaneModel::rebuild);
    connect(m_monitor, &RangeMonitor::nodeAdded,   this, &DisplayLaneModel::rebuild);
    connect(m_monitor, &RangeMonitor::nodeChanged, this, &DisplayLaneModel::rebuild);
    connect(m_monitor, &RangeMonitor::nodeRemoved, this, &DisplayLaneModel::rebuild);
    m_rows = m_display->laneOrderNumbers();
}

void DisplayLaneModel::rebuild()
{
    const QVector<int> next = m_display->laneOrderNumbers();
    if (next != m_rows) {
        beginResetModel();
        m_rows = next;
        endResetModel();
    } else if (!m_rows.isEmpty()) {
        // Same lanes, moved contents. Repainting rows rather than resetting
        // keeps a 20-lane grid from rebuilding every delegate on every
        // heartbeat.
        emit dataChanged(index(0, 0), index(m_rows.size() - 1, 0));
    }
    emit changed();
}

int DisplayLaneModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

int DisplayLaneModel::rowCountProperty() const
{
    return int(m_rows.size());
}

QHash<int, QByteArray> DisplayLaneModel::roleNames() const
{
    return {
        { LaneNumberRole,          "laneNumber" },
        { LaneLabelRole,           "laneLabel" },
        { HasDeviceRole,           "hasDevice" },
        { OnlineRole,              "online" },
        { ConnectionRole,          "connection" },
        { StatusTextRole,          "statusText" },
        { AthleteRole,             "athlete" },
        { PlannedAthleteRole,      "plannedAthlete" },
        { ObservedAthleteRole,     "observedAthlete" },
        { AthleteMismatchRole,     "athleteMismatch" },
        { ProgrammeLabelRole,      "programmeLabel" },
        { PlannedProgrammeRole,    "plannedProgramme" },
        { ProgrammeMismatchRole,   "programmeMismatch" },
        { PhaseRole,               "phase" },
        { PositionRole,            "position" },
        { TargetStandardIdRole,    "targetStandardId" },
        { TargetStandardNameRole,  "targetStandardName" },
        { TargetSupportedRole,     "targetSupported" },
        { ShotsRole,               "shots" },
        { ShotsAcceptedRole,       "shotsAccepted" },
        { ShotsExpectedRole,       "shotsExpected" },
        { ObservedShotCountRole,   "observedShotCount" },
        { UnseenShotCountRole,     "unseenShotCount" },
        { ShotsLabelRole,          "shotsLabel" },
        { NodeTotalLabelRole,      "nodeTotalLabel" },
        { ObservedTotalLabelRole,  "observedTotalLabel" },
        { LastShotScoreRole,       "lastShotScore" },
        { LastShotSequenceRole,    "lastShotSequence" },
        { CompetitionStatusRole,   "competitionStatus" },
        { CompetitionTerminalRole, "competitionTerminal" },
        { EliminatedRole,          "eliminated" },
        { FinishedRole,            "finished" },
        { FinalRankLabelRole,      "finalRankLabel" },
        { FinalScoreLabelRole,     "finalScoreLabel" },
        { CompetitionSimulatedRole,"competitionSimulated" }
    };
}

QVariantMap DisplayLaneModel::buildLane(int laneNumber) const
{
    QVariantMap m;
    const LaneDefinition* lane = m_range->range().laneByNumber(laneNumber);
    if (!lane)
        return m;

    const TargetNodeRecord* r = lane->isAssigned()
                                    ? m_monitor->nodeById(lane->assignedNodeId)
                                    : nullptr;
    const bool online = r && !r->isOffline();

    m[QStringLiteral("laneNumber")] = lane->laneNumber;
    m[QStringLiteral("laneLabel")]  = lane->label();
    m[QStringLiteral("hasDevice")]  = lane->isAssigned();
    m[QStringLiteral("online")]     = online;
    m[QStringLiteral("connection")] = !lane->isAssigned()
                                          ? QStringLiteral("NO DEVICE")
                                          : (r ? toString(r->connection)
                                               : QStringLiteral("OFFLINE"));
    m[QStringLiteral("statusText")] =
        !lane->enabled            ? QStringLiteral("Out of service")
      : !lane->isAssigned()       ? QStringLiteral("No device assigned")
      : !r                        ? QStringLiteral("Assigned, never seen")
      : r->isOffline()            ? QStringLiteral("Offline — last known data")
                                  : QString();

    // ── athlete: plan and station, both kept ────────────────────────────
    const QString plannedAthlete =
        (m_plans && m_athletes && m_plans->hasPlan())
            ? m_athletes->displayNameFor(m_plans->athleteOnLane(laneNumber))
            : QString();
    const QString observedAthlete = r ? r->athleteName : QString();
    // The display TITLE prefers the plan when a plan is open: the operator
    // chose who is on that lane, and the station's own text is whatever it was
    // told locally. Both are kept, and a disagreement is flagged rather than
    // resolved.
    m[QStringLiteral("plannedAthlete")]  = plannedAthlete;
    m[QStringLiteral("observedAthlete")] = observedAthlete;
    m[QStringLiteral("athlete")] = !plannedAthlete.isEmpty() ? plannedAthlete
                                                             : observedAthlete;
    m[QStringLiteral("athleteMismatch")] =
        !plannedAthlete.isEmpty() && !observedAthlete.isEmpty()
        && plannedAthlete != observedAthlete;

    // ── programme: same discipline ──────────────────────────────────────
    m[QStringLiteral("programmeLabel")] =
        r ? ProgrammeDisplay::describe(r->programmeId) : QString();
    m[QStringLiteral("plannedProgramme")] =
        (m_plans && m_plans->isLaneSelected(laneNumber)) ? m_plans->programmeLabel()
                                                         : QString();
    m[QStringLiteral("programmeMismatch")] =
        m_plans && m_plans->programmeMatchForLane(laneNumber) == ProgrammeMatch::Mismatch;
    m[QStringLiteral("phase")]    = r ? toString(r->phase) : QString();
    m[QStringLiteral("position")] = r ? r->position : QString();

    // ── the face ────────────────────────────────────────────────────────
    // Observed standard first; the plan's is a reasonable stand-in before a
    // station has reported one, and an unknown standard draws a placeholder
    // rather than the wrong target.
    QString standardId = r ? r->targetStandardId : QString();
    if (standardId.isEmpty() && m_plans && m_plans->isLaneSelected(laneNumber))
        standardId = m_plans->current().programme.targetStandardId;
    const TargetSpec spec = TargetGeometry::specFor(standardId);
    m[QStringLiteral("targetStandardId")]   = standardId;
    m[QStringLiteral("targetStandardName")] = spec.displayName;
    m[QStringLiteral("targetSupported")]    = spec.supported;

    // ── shots ───────────────────────────────────────────────────────────
    QVariantList shots;
    int observedCount = 0;
    double observedSum = 0.0;
    QString lastScore = QStringLiteral("—");
    int lastSequence = 0;

    if (r) {
        const QList<AcceptedShot> ordered = r->ledger.shotsInOrder();
        observedCount = int(ordered.size());
        for (const AcceptedShot& s : ordered)
            observedSum += s.authoritativeScore;

        const int start = qMax(0, observedCount - kVisibleShots);
        for (int i = start; i < observedCount; ++i) {
            const AcceptedShot& s = ordered.at(i);
            QVariantMap sm;
            // Normalised once, here, so every view draws the same point and no
            // QML has to remember the y flip.
            const QPointF n = spec.supported
                                  ? TargetGeometry::normaliseClamped(spec, s.rawXMm, s.rawYMm)
                                  : QPointF(0.0, 0.0);
            sm[QStringLiteral("x")] = n.x();
            sm[QStringLiteral("y")] = n.y();
            sm[QStringLiteral("xMm")] = s.rawXMm;
            sm[QStringLiteral("yMm")] = s.rawYMm;
            // THE NODE'S SCORE. Never derived from the coordinates above.
            sm[QStringLiteral("score")] = QString::number(s.authoritativeScore, 'f', 1);
            sm[QStringLiteral("scoreValue")] = s.authoritativeScore;
            sm[QStringLiteral("sequence")] = s.shotSequence;
            sm[QStringLiteral("innerTen")] = s.innerTen;
            sm[QStringLiteral("offFace")] =
                spec.supported && !TargetGeometry::isWithinFace(spec, s.rawXMm, s.rawYMm);
            sm[QStringLiteral("last")] = (i == observedCount - 1);
            shots.append(sm);
        }
        if (r->ledger.hasShots()) {
            const AcceptedShot last = r->ledger.latestReceived();
            lastScore = QString::number(last.authoritativeScore, 'f', 1);
            lastSequence = last.shotSequence;
        }
    }

    m[QStringLiteral("shots")] = shots;
    m[QStringLiteral("observedShotCount")] = observedCount;
    m[QStringLiteral("shotsAccepted")] = r ? r->shotsAcceptedByNode : 0;
    m[QStringLiteral("shotsExpected")] = r ? r->shotsExpected : -1;
    m[QStringLiteral("unseenShotCount")] = r ? r->unobservedShotCount() : 0;
    m[QStringLiteral("shotsLabel")] =
        !r ? QStringLiteral("—")
           : (r->shotsExpected > 0
                  ? QStringLiteral("%1/%2").arg(r->shotsAcceptedByNode).arg(r->shotsExpected)
                  : QString::number(r->shotsAcceptedByNode));
    // The node's own running total — authoritative, and what a score display
    // should read.
    m[QStringLiteral("nodeTotalLabel")] = (r && r->shotsAcceptedByNode > 0)
                                              ? QString::number(r->totalScoreByNode, 'f', 1)
                                              : QStringLiteral("—");
    // The sum of what RMS actually received. Shown ONLY beside the unseen
    // count, and never as a result: if RMS missed shots this is not the match.
    m[QStringLiteral("observedTotalLabel")] = observedCount > 0
                                                  ? QString::number(observedSum, 'f', 1)
                                                  : QStringLiteral("—");
    m[QStringLiteral("lastShotScore")] = lastScore;
    m[QStringLiteral("lastShotSequence")] = lastSequence;

    // ── competition axis ────────────────────────────────────────────────
    const CompetitionState comp = r ? r->competition : CompetitionState();
    m[QStringLiteral("competitionStatus")]    = toString(comp.status);
    m[QStringLiteral("competitionTerminal")]  = comp.isTerminal();
    m[QStringLiteral("eliminated")]           = comp.isEliminated();
    m[QStringLiteral("finished")]             = comp.status == CompetitionStatus::Finished;
    m[QStringLiteral("finalRankLabel")]       = comp.rankLabel();
    m[QStringLiteral("finalScoreLabel")]      = comp.scoreLabel();
    m[QStringLiteral("competitionSimulated")] = comp.isSimulated();
    return m;
}

QVariantMap DisplayLaneModel::laneByNumber(int laneNumber) const
{
    return buildLane(laneNumber);
}

QVariantMap DisplayLaneModel::laneAtRow(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return QVariantMap();
    return buildLane(m_rows.at(row));
}

QVariant DisplayLaneModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();
    const QVariantMap lane = buildLane(m_rows.at(index.row()));
    if (lane.isEmpty())
        return QVariant();

    const QHash<int, QByteArray> names = roleNames();
    const auto it = names.constFind(role);
    if (it == names.constEnd())
        return QVariant();
    return lane.value(QString::fromLatin1(it.value()));
}

} // namespace rms
} // namespace ta
