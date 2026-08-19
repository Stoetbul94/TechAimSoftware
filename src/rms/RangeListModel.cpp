#include "RangeListModel.h"
#include "ProgrammeDisplay.h"

#include <QStringList>

namespace ta {
namespace rms {

RangeListModel::RangeListModel(RangeMonitor* monitor, QObject* parent)
    : QAbstractListModel(parent)
    , m_monitor(monitor)
{
    connect(m_monitor, &RangeMonitor::nodeAdded,   this, &RangeListModel::onNodeAdded);
    connect(m_monitor, &RangeMonitor::nodeChanged, this, &RangeListModel::onNodeChanged);
    connect(m_monitor, &RangeMonitor::nodeRemoved, this, &RangeListModel::onMonitorReset);
}

int RangeListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

int RangeListModel::rowOf(const QString& nodeId) const
{
    return int(m_rows.indexOf(nodeId));
}

const TargetNodeRecord* RangeListModel::recordAt(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return nullptr;
    return m_monitor->nodeById(m_rows.at(row));
}

void RangeListModel::onNodeAdded(const QString& nodeId)
{
    if (rowOf(nodeId) >= 0) {
        onNodeChanged(nodeId);
        return;
    }
    const int row = m_rows.size();
    beginInsertRows(QModelIndex(), row, row);
    m_rows.append(nodeId);
    endInsertRows();
    emit summaryChanged();
}

void RangeListModel::onNodeChanged(const QString& nodeId)
{
    const int row = rowOf(nodeId);
    if (row < 0) {
        onNodeAdded(nodeId);
        return;
    }
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx);
    emit summaryChanged();
}

void RangeListModel::onMonitorReset()
{
    // The monitor emits nodeRemoved once per node on reset(); rebuilding the
    // whole row list is simpler and correct for both cases.
    if (m_rows.isEmpty())
        return;
    beginResetModel();
    m_rows.clear();
    endResetModel();
    emit summaryChanged();
}

int RangeListModel::onlineCount() const
{
    int n = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        const TargetNodeRecord* r = recordAt(i);
        if (r && !r->isOffline())
            ++n;
    }
    return n;
}

int RangeListModel::offlineCount() const
{
    return m_rows.size() - onlineCount();
}

QHash<int, QByteArray> RangeListModel::roleNames() const
{
    return {
        { NodeIdRole,           "nodeId" },
        { LaneLabelRole,        "laneLabel" },
        { AthleteRole,          "athlete" },
        { ProgrammeIdRole,      "programmeId" },
        { ProgrammeLabelRole,   "programmeLabel" },
        { RulesetIdRole,        "rulesetId" },
        { TargetStandardIdRole, "targetStandardId" },
        { OfficialRole,         "officialProgramme" },
        { PositionRole,         "position" },
        { PhaseRole,            "phase" },
        { ConnectionRole,       "connection" },
        { OfflineRole,          "offline" },
        { ShotsAcceptedRole,    "shotsAccepted" },
        { ShotsExpectedRole,    "shotsExpected" },
        { ShotsLabelRole,       "shotsLabel" },
        { TotalScoreRole,       "totalScore" },
        { ScoreLabelRole,       "scoreLabel" },
        { UnobservedRole,       "unobserved" },
        { DuplicatesRole,       "duplicatesSuppressed" },
        { OutOfOrderRole,       "outOfOrder" },
        { GapCountRole,         "gapCount" },
        { RestartsRole,         "nodeRestarts" },
        { HealthRole,           "health" },
        { DeviceIdentityRole,   "deviceIdentity" }
    };
}

QVariant RangeListModel::data(const QModelIndex& index, int role) const
{
    const TargetNodeRecord* r = recordAt(index.row());
    if (!r)
        return QVariant();

    switch (role) {
    case NodeIdRole:           return r->nodeId;
    case LaneLabelRole:        return r->laneId.isEmpty()
                                      ? QStringLiteral("unassigned") : r->laneId;
    case AthleteRole:          return r->athleteName;
    case ProgrammeIdRole:      return r->programmeId;
    case ProgrammeLabelRole:   return ProgrammeDisplay::describe(r->programmeId);
    case RulesetIdRole:        return r->rulesetId;
    case TargetStandardIdRole: return r->targetStandardId;
    case OfficialRole:         return ProgrammeDisplay::isOfficialProgramme(r->rulesetId);
    case PositionRole:         return r->position;
    case PhaseRole:            return toString(r->phase);
    case ConnectionRole:       return toString(r->connection);
    case OfflineRole:          return r->isOffline();
    case ShotsAcceptedRole:    return r->shotsAcceptedByNode;
    case ShotsExpectedRole:    return r->shotsExpected;
    case ShotsLabelRole:
        if (r->shotsExpected > 0)
            return QStringLiteral("%1/%2").arg(r->shotsAcceptedByNode).arg(r->shotsExpected);
        return QString::number(r->shotsAcceptedByNode);
    case TotalScoreRole:       return r->totalScoreByNode;
    case ScoreLabelRole:
        // Shown to one decimal because the node reports a decimal total.
        // RMS formats; it does not compute. See the read-only invariant.
        return r->shotsAcceptedByNode > 0
                   ? QString::number(r->totalScoreByNode, 'f', 1) : QStringLiteral("—");
    case UnobservedRole:       return r->unobservedShotCount();
    case DuplicatesRole:       return r->ledger.duplicatesSuppressed();
    case OutOfOrderRole:       return r->ledger.outOfOrderAccepted();
    case GapCountRole:         return int(r->ledger.missingSequences().size());
    case RestartsRole:         return r->nodeRestarts;
    case HealthRole:           return r->health;
    case DeviceIdentityRole:   return r->deviceIdentity;
    default:                   return QVariant();
    }
}

QVariantMap RangeListModel::nodeDetail(int row) const
{
    QVariantMap m;
    const TargetNodeRecord* r = recordAt(row);
    if (!r)
        return m;

    m[QStringLiteral("nodeId")]           = r->nodeId;
    m[QStringLiteral("bootId")]           = r->bootId;
    m[QStringLiteral("laneLabel")]        = r->laneId.isEmpty()
                                            ? QStringLiteral("unassigned") : r->laneId;
    m[QStringLiteral("athlete")]          = r->athleteName;
    m[QStringLiteral("programmeId")]      = r->programmeId;
    m[QStringLiteral("programmeLabel")]   = ProgrammeDisplay::describe(r->programmeId);
    m[QStringLiteral("rulesetId")]        = r->rulesetId;
    m[QStringLiteral("targetStandardId")] = r->targetStandardId;
    m[QStringLiteral("officialProgramme")]= ProgrammeDisplay::isOfficialProgramme(r->rulesetId);
    m[QStringLiteral("sessionId")]        = r->sessionId;
    m[QStringLiteral("position")]         = r->position;
    m[QStringLiteral("phase")]            = toString(r->phase);
    m[QStringLiteral("connection")]       = toString(r->connection);
    m[QStringLiteral("offline")]          = r->isOffline();
    m[QStringLiteral("shotsAccepted")]    = r->shotsAcceptedByNode;
    m[QStringLiteral("shotsExpected")]    = r->shotsExpected;
    m[QStringLiteral("totalScore")]       = r->totalScoreByNode;
    m[QStringLiteral("scoreLabel")]       = r->shotsAcceptedByNode > 0
                                            ? QString::number(r->totalScoreByNode, 'f', 1)
                                            : QStringLiteral("—");
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

    QStringList gaps;
    const QList<int> missing = r->ledger.missingSequences();
    for (int s : missing)
        gaps << QString::number(s);
    m[QStringLiteral("gapList")] = gaps.join(QStringLiteral(", "));
    m[QStringLiteral("gapCount")] = int(missing.size());

    if (r->ledger.hasShots()) {
        const AcceptedShot last = r->ledger.latestReceived();
        m[QStringLiteral("latestSequence")] = last.shotSequence;
        m[QStringLiteral("latestScore")]    = QString::number(last.authoritativeScore, 'f', 1);
        m[QStringLiteral("latestX")]        = QString::number(last.rawXMm, 'f', 2);
        m[QStringLiteral("latestY")]        = QString::number(last.rawYMm, 'f', 2);
        m[QStringLiteral("latestInnerTen")] = last.innerTen;
        m[QStringLiteral("latestStatus")]   = last.acquisitionStatus;
    } else {
        m[QStringLiteral("latestSequence")] = 0;
        m[QStringLiteral("latestScore")]    = QStringLiteral("—");
    }
    return m;
}

QVariantList RangeListModel::recentShots(int row, int maxCount) const
{
    QVariantList out;
    const TargetNodeRecord* r = recordAt(row);
    if (!r)
        return out;

    const QList<AcceptedShot> ordered = r->ledger.shotsInOrder();
    const int start = qMax(0, int(ordered.size()) - maxCount);
    // Newest first — a range officer reads the top of the list.
    for (int i = int(ordered.size()) - 1; i >= start; --i) {
        const AcceptedShot& s = ordered.at(i);
        QVariantMap m;
        m[QStringLiteral("sequence")]  = s.shotSequence;
        m[QStringLiteral("score")]     = QString::number(s.authoritativeScore, 'f', 1);
        m[QStringLiteral("innerTen")]  = s.innerTen;
        m[QStringLiteral("position")]  = s.position;
        m[QStringLiteral("x")]         = QString::number(s.rawXMm, 'f', 2);
        m[QStringLiteral("y")]         = QString::number(s.rawYMm, 'f', 2);
        m[QStringLiteral("status")]    = s.acquisitionStatus;
        out.append(m);
    }
    return out;
}

QString RangeListModel::renderTextDashboard() const
{
    QString out;
    out += QStringLiteral("LANE      NODE          TARGET               PROGRAMME"
                          "                          PHASE            SHOTS   SCORE\n");
    out += QString(112, QLatin1Char('-')) + QLatin1Char('\n');
    for (int i = 0; i < m_rows.size(); ++i) {
        const TargetNodeRecord* r = recordAt(i);
        if (!r)
            continue;
        const QString lane = r->laneId.isEmpty() ? QStringLiteral("—") : r->laneId;
        const QString shots = r->shotsExpected > 0
            ? QStringLiteral("%1/%2").arg(r->shotsAcceptedByNode).arg(r->shotsExpected)
            : QString::number(r->shotsAcceptedByNode);
        const QString score = r->shotsAcceptedByNode > 0
            ? QString::number(r->totalScoreByNode, 'f', 1) : QStringLiteral("—");
        out += QStringLiteral("%1%2%3%4%5%6%7\n")
                   .arg(lane, -10)
                   .arg(r->nodeId.left(12), -14)
                   .arg(toString(r->connection), -21)
                   .arg(ProgrammeDisplay::describe(r->programmeId), -35)
                   .arg(toString(r->phase), -17)
                   .arg(shots, -8)
                   .arg(score);
    }
    return out;
}

} // namespace rms
} // namespace ta
