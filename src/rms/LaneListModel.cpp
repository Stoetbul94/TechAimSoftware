#include "LaneListModel.h"
#include "AthleteRegistry.h"
#include "MatchPlanService.h"
#include "ProgrammeDisplay.h"

#include <QStringList>

namespace ta {
namespace rms {

LaneListModel::LaneListModel(RangeConfigurationService* config,
                             RangeMonitor* monitor, QObject* parent)
    : QAbstractListModel(parent)
    , m_config(config)
    , m_monitor(monitor)
{
    connect(m_config, &RangeConfigurationService::rangeChanged,
            this, &LaneListModel::onRangeChanged);
    connect(m_monitor, &RangeMonitor::nodeAdded,   this, &LaneListModel::onNodeChanged);
    connect(m_monitor, &RangeMonitor::nodeChanged, this, &LaneListModel::onNodeChanged);
    connect(m_monitor, &RangeMonitor::nodeRemoved, this, &LaneListModel::onNodeChanged);
}

void LaneListModel::setPlanContext(MatchPlanService* plans, AthleteRegistry* athletes)
{
    m_plans = plans;
    m_athletes = athletes;
    if (m_plans)
        connect(m_plans, &MatchPlanService::planChanged, this, [this] {
            if (rowCountProperty() > 0)
                emit dataChanged(index(0, 0), index(rowCountProperty() - 1, 0));
            emit summaryChanged();
        });
}

const LaneDefinition* LaneListModel::laneAt(int row) const
{
    const RangeDefinition& r = m_config->range();
    if (row < 0 || row >= r.lanes.size())
        return nullptr;
    return &r.lanes.at(row);
}

const TargetNodeRecord* LaneListModel::recordForRow(int row) const
{
    const LaneDefinition* lane = laneAt(row);
    if (!lane || !lane->isAssigned())
        return nullptr;
    return m_monitor->nodeById(lane->assignedNodeId);
}

int LaneListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_config->range().laneCount();
}

int LaneListModel::rowCountProperty() const
{
    return m_config->range().laneCount();
}

void LaneListModel::onRangeChanged()
{
    // The lane set itself changed. A reset is correct and cheap at range
    // sizes, and it keeps the "rows are the configuration" rule obvious.
    beginResetModel();
    endResetModel();
    emit summaryChanged();
}

void LaneListModel::onNodeChanged(const QString& nodeId)
{
    // Only the lane holding this station can have changed. Repainting the one
    // row keeps a busy range from redrawing itself on every heartbeat.
    const int lane = m_config->laneNumberForNode(nodeId);
    if (lane < 0) {
        // An unassigned station moving does not change any lane row, but it
        // does change the unassigned count the summary reports.
        emit summaryChanged();
        return;
    }
    const int row = m_config->range().indexOfLaneNumber(lane);
    if (row >= 0) {
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx);
    }
    emit summaryChanged();
}

int LaneListModel::onlineCount() const
{
    int n = 0;
    for (int i = 0; i < rowCountProperty(); ++i) {
        const TargetNodeRecord* r = recordForRow(i);
        if (r && !r->isOffline())
            ++n;
    }
    return n;
}

int LaneListModel::offlineCount() const
{
    // Every lane that is NOT currently live: unassigned lanes and assigned
    // lanes whose station is silent both count, because from the range
    // officer's point of view neither is shooting.
    return rowCountProperty() - onlineCount();
}

int LaneListModel::unassignedLaneCount() const
{
    int n = 0;
    for (int i = 0; i < rowCountProperty(); ++i) {
        const LaneDefinition* l = laneAt(i);
        if (l && !l->isAssigned())
            ++n;
    }
    return n;
}

int LaneListModel::activeSessionCount() const
{
    int n = 0;
    for (int i = 0; i < rowCountProperty(); ++i) {
        const TargetNodeRecord* r = recordForRow(i);
        if (r && !r->isOffline() && !r->sessionId.isEmpty()
            && r->phase != MatchPhase::Idle && r->phase != MatchPhase::Unknown)
            ++n;
    }
    return n;
}

QHash<int, QByteArray> LaneListModel::roleNames() const
{
    return {
        { LaneNumberRole,     "laneNumber" },
        { LaneLabelRole,      "laneLabel" },
        { AssignedNodeIdRole, "assignedNodeId" },
        { HasDeviceRole,      "hasDevice" },
        { OnlineRole,         "online" },
        { OfflineRole,        "offline" },
        { LaneEnabledRole,    "laneEnabled" },
        { ConnectionRole,     "connection" },
        { StatusTextRole,     "statusText" },
        { AthleteRole,        "athlete" },
        { ProgrammeIdRole,    "programmeId" },
        { ProgrammeLabelRole, "programmeLabel" },
        { OfficialRole,       "officialProgramme" },
        { PhaseRole,          "phase" },
        { ShotsLabelRole,     "shotsLabel" },
        { ScoreLabelRole,     "scoreLabel" },
        { UnobservedRole,     "unobserved" },
        { GapCountRole,       "gapCount" },
        { PlannedAthleteRole,   "plannedAthlete" },
        { PlannedProgrammeRole, "plannedProgramme" },
        { InPlanRole,           "inPlan" },
        { ProgrammeMatchRole,   "programmeMatch" },
        { ProgrammeMismatchRole,"programmeMismatch" }
    };
}

QVariant LaneListModel::data(const QModelIndex& index, int role) const
{
    const LaneDefinition* lane = laneAt(index.row());
    if (!lane)
        return QVariant();
    const TargetNodeRecord* r = recordForRow(index.row());
    const bool online = r && !r->isOffline();

    switch (role) {
    case LaneNumberRole:     return lane->laneNumber;
    case LaneLabelRole:      return lane->label();
    case AssignedNodeIdRole: return lane->assignedNodeId;
    case HasDeviceRole:      return lane->isAssigned();
    case OnlineRole:         return online;
    case OfflineRole:        return !online;
    case LaneEnabledRole:    return lane->enabled;

    case ConnectionRole:
        if (!lane->isAssigned())
            return QStringLiteral("NO DEVICE");
        // Assigned but never heard from in this RMS run is OFFLINE, not
        // unknown: from the range's point of view the lane is not answering.
        return r ? toString(r->connection) : QStringLiteral("OFFLINE");

    case StatusTextRole:
        if (!lane->enabled)          return QStringLiteral("Out of service");
        if (!lane->isAssigned())     return QStringLiteral("No device assigned");
        if (!r)                      return QStringLiteral("Assigned, never seen");
        if (r->isOffline())          return QStringLiteral("Device offline");
        return QString();

    case AthleteRole:        return r ? r->athleteName : QString();
    case ProgrammeIdRole:    return r ? r->programmeId : QString();
    case ProgrammeLabelRole: return r ? ProgrammeDisplay::describe(r->programmeId)
                                      : QString();
    case OfficialRole:       return r && ProgrammeDisplay::isOfficialProgramme(r->rulesetId);
    case PhaseRole:          return r ? toString(r->phase) : QString();

    case ShotsLabelRole:
        if (!r)
            return QStringLiteral("—");
        if (r->shotsExpected > 0)
            return QStringLiteral("%1/%2").arg(r->shotsAcceptedByNode).arg(r->shotsExpected);
        return QString::number(r->shotsAcceptedByNode);

    case ScoreLabelRole:
        // Formats the node's total. RMS does not compute a score anywhere.
        return (r && r->shotsAcceptedByNode > 0)
                   ? QString::number(r->totalScoreByNode, 'f', 1)
                   : QStringLiteral("—");

    case UnobservedRole:     return r ? r->unobservedShotCount() : 0;
    case GapCountRole:       return r ? int(r->ledger.missingSequences().size()) : 0;

    // ── PLANNED, never merged into the observed values above ────────────
    // The plan is what the operator intends. It has not been sent anywhere,
    // so it may legitimately differ from what the station reports, and the
    // two are shown side by side rather than one overwriting the other.
    case InPlanRole:
        return m_plans && m_plans->isLaneSelected(lane->laneNumber);
    case PlannedAthleteRole: {
        if (!m_plans || !m_athletes)
            return QString();
        return m_athletes->displayNameFor(m_plans->athleteOnLane(lane->laneNumber));
    }
    case PlannedProgrammeRole:
        return (m_plans && m_plans->isLaneSelected(lane->laneNumber))
                   ? m_plans->programmeLabel() : QString();
    case ProgrammeMatchRole:
        return m_plans ? toString(m_plans->programmeMatchForLane(lane->laneNumber))
                       : QString();
    case ProgrammeMismatchRole:
        return m_plans && m_plans->programmeMatchForLane(lane->laneNumber)
                              == ProgrammeMatch::Mismatch;

    default:                 return QVariant();
    }
}

QVariantMap LaneListModel::laneDetail(int row) const
{
    QVariantMap m;
    const LaneDefinition* lane = laneAt(row);
    if (!lane)
        return m;

    m[QStringLiteral("laneNumber")]     = lane->laneNumber;
    m[QStringLiteral("laneLabel")]      = lane->label();
    m[QStringLiteral("laneId")]         = lane->laneId;
    m[QStringLiteral("assignedNodeId")] = lane->assignedNodeId;
    m[QStringLiteral("hasDevice")]      = lane->isAssigned();
    m[QStringLiteral("laneEnabled")]    = lane->enabled;
    m[QStringLiteral("notes")]          = lane->notes;

    const TargetNodeRecord* r = recordForRow(row);
    // The plan's intention for this lane, beside the observation.
    if (m_plans) {
        m[QStringLiteral("inPlan")] = m_plans->isLaneSelected(lane->laneNumber);
        m[QStringLiteral("plannedProgramme")] =
            m_plans->isLaneSelected(lane->laneNumber) ? m_plans->programmeLabel() : QString();
        m[QStringLiteral("programmeMatch")] =
            toString(m_plans->programmeMatchForLane(lane->laneNumber));
        m[QStringLiteral("programmeMismatch")] =
            m_plans->programmeMatchForLane(lane->laneNumber) == ProgrammeMatch::Mismatch;
        if (m_athletes)
            m[QStringLiteral("plannedAthlete")] =
                m_athletes->displayNameFor(m_plans->athleteOnLane(lane->laneNumber));
    }

    m[QStringLiteral("observed")] = (r != nullptr);
    if (!r) {
        m[QStringLiteral("connection")] = lane->isAssigned()
                                              ? QStringLiteral("OFFLINE")
                                              : QStringLiteral("NO DEVICE");
        m[QStringLiteral("offline")]    = true;
        m[QStringLiteral("scoreLabel")] = QStringLiteral("—");
        m[QStringLiteral("latestSequence")] = 0;
        return m;
    }

    m[QStringLiteral("connection")]       = toString(r->connection);
    m[QStringLiteral("offline")]          = r->isOffline();
    m[QStringLiteral("athlete")]          = r->athleteName;
    m[QStringLiteral("programmeId")]      = r->programmeId;
    m[QStringLiteral("programmeLabel")]   = ProgrammeDisplay::describe(r->programmeId);
    m[QStringLiteral("rulesetId")]        = r->rulesetId;
    m[QStringLiteral("targetStandardId")] = r->targetStandardId;
    m[QStringLiteral("officialProgramme")]= ProgrammeDisplay::isOfficialProgramme(r->rulesetId);
    m[QStringLiteral("phase")]            = toString(r->phase);
    m[QStringLiteral("shotsAccepted")]    = r->shotsAcceptedByNode;
    m[QStringLiteral("shotsExpected")]    = r->shotsExpected;
    m[QStringLiteral("scoreLabel")]       = r->shotsAcceptedByNode > 0
                                            ? QString::number(r->totalScoreByNode, 'f', 1)
                                            : QStringLiteral("—");
    // ── diagnostics: milestone 1's engineering detail, preserved ─────────
    m[QStringLiteral("nodeId")]           = r->nodeId;
    m[QStringLiteral("bootId")]           = r->bootId;
    m[QStringLiteral("sessionId")]        = r->sessionId;
    m[QStringLiteral("deviceIdentity")]   = r->deviceIdentity;
    m[QStringLiteral("appVersion")]       = r->appVersion;
    m[QStringLiteral("productIdentity")]  = r->productIdentity;
    m[QStringLiteral("health")]           = r->health;
    m[QStringLiteral("observedShots")]    = r->ledger.observedCount();
    m[QStringLiteral("unobserved")]       = r->unobservedShotCount();
    m[QStringLiteral("duplicatesSuppressed")] = r->ledger.duplicatesSuppressed();
    m[QStringLiteral("outOfOrder")]       = r->ledger.outOfOrderAccepted();
    m[QStringLiteral("sequenceConflicts")]= r->ledger.sequenceConflicts();
    m[QStringLiteral("nodeRestarts")]     = r->nodeRestarts;
    m[QStringLiteral("offlineEpisodes")]  = r->offlineEpisodes;
    m[QStringLiteral("staleStatusDropped")] = r->staleStatusDropped;
    m[QStringLiteral("staleBootDropped")] = r->staleBootDropped;

    QStringList gaps;
    const QList<int> missing = r->ledger.missingSequences();
    for (int s : missing)
        gaps << QString::number(s);
    m[QStringLiteral("gapList")]  = gaps.join(QStringLiteral(", "));
    m[QStringLiteral("gapCount")] = int(missing.size());

    if (r->ledger.hasShots()) {
        const AcceptedShot last = r->ledger.latestReceived();
        m[QStringLiteral("latestSequence")] = last.shotSequence;
        m[QStringLiteral("latestScore")]    = QString::number(last.authoritativeScore, 'f', 1);
        m[QStringLiteral("latestStatus")]   = last.acquisitionStatus;
    } else {
        m[QStringLiteral("latestSequence")] = 0;
        m[QStringLiteral("latestScore")]    = QStringLiteral("—");
    }
    return m;
}

QVariantList LaneListModel::recentShots(int row, int maxCount) const
{
    QVariantList out;
    const TargetNodeRecord* r = recordForRow(row);
    if (!r)
        return out;
    const QList<AcceptedShot> ordered = r->ledger.shotsInOrder();
    const int start = qMax(0, int(ordered.size()) - maxCount);
    for (int i = int(ordered.size()) - 1; i >= start; --i) {
        const AcceptedShot& s = ordered.at(i);
        QVariantMap m;
        m[QStringLiteral("sequence")] = s.shotSequence;
        m[QStringLiteral("score")]    = QString::number(s.authoritativeScore, 'f', 1);
        m[QStringLiteral("innerTen")] = s.innerTen;
        m[QStringLiteral("x")]        = QString::number(s.rawXMm, 'f', 2);
        m[QStringLiteral("y")]        = QString::number(s.rawYMm, 'f', 2);
        m[QStringLiteral("status")]   = s.acquisitionStatus;
        out.append(m);
    }
    return out;
}

QString LaneListModel::renderTextRange() const
{
    QString out;
    out += QStringLiteral("LANE      DEVICE          STATUS               PROGRAMME"
                          "                          PHASE            SHOTS   SCORE\n");
    out += QString(112, QLatin1Char('-')) + QLatin1Char('\n');
    for (int i = 0; i < rowCountProperty(); ++i) {
        const LaneDefinition* lane = laneAt(i);
        const TargetNodeRecord* r = recordForRow(i);
        const QString device = lane->isAssigned() ? lane->assignedNodeId.right(8)
                                                  : QStringLiteral("—");
        const QString status = !lane->isAssigned() ? QStringLiteral("NO DEVICE")
                             : (r ? toString(r->connection) : QStringLiteral("OFFLINE"));
        const QString shots = !r ? QStringLiteral("—")
                            : (r->shotsExpected > 0
                                   ? QStringLiteral("%1/%2").arg(r->shotsAcceptedByNode)
                                                            .arg(r->shotsExpected)
                                   : QString::number(r->shotsAcceptedByNode));
        const QString score = (r && r->shotsAcceptedByNode > 0)
                                  ? QString::number(r->totalScoreByNode, 'f', 1)
                                  : QStringLiteral("—");
        out += QStringLiteral("%1%2%3%4%5%6%7\n")
                   .arg(lane->label(), -10)
                   .arg(device, -16)
                   .arg(status, -21)
                   .arg(r ? ProgrammeDisplay::describe(r->programmeId) : QString(), -35)
                   .arg(r ? toString(r->phase) : QString(), -17)
                   .arg(shots, -8)
                   .arg(score);
    }
    return out;
}

} // namespace rms
} // namespace ta
