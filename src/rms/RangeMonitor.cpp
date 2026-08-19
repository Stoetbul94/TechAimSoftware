#include "RangeMonitor.h"

namespace ta {
namespace rms {

RangeMonitor::RangeMonitor(QObject* parent)
    : QObject(parent)
{
}

const TargetNodeRecord* RangeMonitor::nodeAt(int index) const
{
    if (index < 0 || index >= m_order.size())
        return nullptr;
    auto it = m_nodes.constFind(m_order.at(index));
    return it == m_nodes.constEnd() ? nullptr : &it.value();
}

const TargetNodeRecord* RangeMonitor::nodeById(const QString& nodeId) const
{
    auto it = m_nodes.constFind(nodeId);
    return it == m_nodes.constEnd() ? nullptr : &it.value();
}

TargetNodeRecord& RangeMonitor::recordFor(const QString& nodeId, bool* created)
{
    auto it = m_nodes.find(nodeId);
    if (it != m_nodes.end()) {
        if (created) *created = false;
        return it.value();
    }
    TargetNodeRecord r;
    r.nodeId = nodeId;
    m_order.append(nodeId);
    if (created) *created = true;
    return m_nodes.insert(nodeId, r).value();
}

BootOutcome RangeMonitor::applyBoot(TargetNodeRecord& r, const QString& bootId)
{
    if (bootId.isEmpty() || r.bootId == bootId)
        return BootOutcome::Same;

    // A boot we have already moved past. Datagrams do not stop in flight when
    // an application restarts, so one can arrive after the new boot has been
    // seen. Treating it as a restart would tick the restart counter a second
    // time AND reset lastStatusSeq to 0 — which would then let the old run's
    // heartbeats overwrite the new run's state.
    if (r.priorBootIds.contains(bootId)) {
        ++r.staleBootDropped;
        return BootOutcome::Stale;
    }

    if (r.bootId.isEmpty()) {
        r.bootId = bootId;
        return BootOutcome::Same;    // first sighting, not a restart
    }

    // The node's process restarted. Its own SessionStore and recovery decide
    // what happens to the match — RMS only notes that everything it observed
    // before this point belongs to a previous run, and lets the node's next
    // status re-establish the truth.
    r.priorBootIds.append(r.bootId);
    ++r.nodeRestarts;
    r.lastStatusSeq = 0;             // statusSeq is monotonic per boot only
    r.bootId = bootId;
    return BootOutcome::Restarted;
}

IngestOutcome RangeMonitor::ingestDatagram(const QByteArray& payload, qint64 nowUtcMs)
{
    IngestOutcome out;

    const DecodedMessage msg = decode(payload);
    if (msg.type == MessageType::Unknown) {
        ++m_rejected;
        out.rejectReason = msg.rejectReason;
        emit datagramRejected(msg.rejectReason);
        return out;
    }

    ++m_accepted;
    out.accepted = true;
    out.type = msg.type;

    switch (msg.type) {

    case MessageType::NodeAnnounce: {
        const NodeAnnounce& a = msg.announce;
        out.nodeId = a.nodeId;
        bool created = false;
        TargetNodeRecord& r = recordFor(a.nodeId, &created);
        const BootOutcome boot = applyBoot(r, a.bootId);
        if (boot == BootOutcome::Stale) {
            out.staleBoot = true;    // a straggler from a previous run
            return out;
        }
        const bool restarted = (boot == BootOutcome::Restarted);
        if (created)
            r.firstSeenUtcMs = nowUtcMs;
        r.laneId          = a.laneId;
        r.deviceIdentity  = a.deviceIdentity;
        r.appVersion      = a.appVersion;
        r.productIdentity = a.productIdentity;
        r.lastSeenUtcMs   = nowUtcMs;
        // An announce proves the node is reachable; it says nothing about the
        // target hardware, so the connection state is only lifted OUT of
        // Offline/Unknown, never downgraded from a richer status value.
        if (r.connection == ConnectionState::Offline
            || r.connection == ConnectionState::Unknown)
            r.connection = ConnectionState::Online;
        out.nodeIsNew = created;
        out.nodeRestarted = restarted;
        if (created) emit nodeAdded(a.nodeId); else emit nodeChanged(a.nodeId);
        return out;
    }

    case MessageType::NodeStatus: {
        const NodeStatus& s = msg.status;
        out.nodeId = s.nodeId;
        bool created = false;
        TargetNodeRecord& r = recordFor(s.nodeId, &created);
        const BootOutcome boot = applyBoot(r, s.bootId);
        if (boot == BootOutcome::Stale) {
            out.staleBoot = true;
            return out;
        }
        const bool restarted = (boot == BootOutcome::Restarted);
        if (created)
            r.firstSeenUtcMs = nowUtcMs;

        // Out-of-order status datagrams must not drag a lane backwards.
        // statusSeq is strictly monotonic within a boot, so a value that is
        // not GREATER than the last one is either a re-delivery or a stale
        // datagram overtaken by a newer one — either way it must not be
        // applied on top of fresher state.
        if (s.statusSeq != 0 && s.statusSeq <= r.lastStatusSeq) {
            ++r.staleStatusDropped;
            r.lastSeenUtcMs = nowUtcMs;   // still proof of life
            out.accepted = true;
            emit nodeChanged(s.nodeId);
            return out;
        }
        r.lastStatusSeq = s.statusSeq;

        r.laneId              = s.laneId;
        r.sessionId           = s.sessionId;
        r.programmeId         = s.programmeId;
        r.rulesetId           = s.rulesetId;
        r.targetStandardId    = s.targetStandardId;
        r.athleteName         = s.athleteName;
        r.position            = s.position;
        r.connection          = s.connection;
        r.phase               = s.phase;
        r.shotsAcceptedByNode = s.shotsAccepted;
        r.shotsExpected       = s.shotsExpected;
        r.totalScoreByNode    = s.totalScore;
        r.health              = s.health;
        r.lastSeenUtcMs       = nowUtcMs;

        out.nodeIsNew = created;
        out.nodeRestarted = restarted;
        if (created) emit nodeAdded(s.nodeId); else emit nodeChanged(s.nodeId);
        return out;
    }

    case MessageType::AcceptedShot: {
        const AcceptedShot& sh = msg.shot;
        out.nodeId = sh.nodeId;
        bool created = false;
        TargetNodeRecord& r = recordFor(sh.nodeId, &created);
        const BootOutcome boot = applyBoot(r, sh.bootId);
        if (boot == BootOutcome::Stale) {
            out.staleBoot = true;
            return out;
        }
        const bool restarted = (boot == BootOutcome::Restarted);
        if (created)
            r.firstSeenUtcMs = nowUtcMs;
        if (!sh.laneId.isEmpty())
            r.laneId = sh.laneId;
        r.lastSeenUtcMs = nowUtcMs;
        if (r.connection == ConnectionState::Offline
            || r.connection == ConnectionState::Unknown)
            r.connection = ConnectionState::Online;

        out.shotResult = r.ledger.ingest(sh);
        out.nodeIsNew = created;
        out.nodeRestarted = restarted;

        if (created) emit nodeAdded(sh.nodeId); else emit nodeChanged(sh.nodeId);
        if (out.shotResult != ShotIngest::DuplicateSuppressed
            && out.shotResult != ShotIngest::SequenceConflict)
            emit shotObserved(sh.nodeId, sh.shotSequence);
        return out;
    }

    case MessageType::Unknown:
        break;
    }

    return out;
}

void RangeMonitor::evaluateLiveness(qint64 nowUtcMs)
{
    for (const QString& id : m_order) {
        auto it = m_nodes.find(id);
        if (it == m_nodes.end())
            continue;
        TargetNodeRecord& r = it.value();
        const bool silent = (nowUtcMs - r.lastSeenUtcMs) > m_offlineTimeoutMs;
        if (silent && r.connection != ConnectionState::Offline) {
            // RMS concludes the node is unreachable. Nothing is sent, nothing
            // on the node is changed, and the ledger is preserved intact so
            // the lane's history is still there when it returns.
            ++r.offlineEpisodes;
            r.connection = ConnectionState::Offline;
            emit nodeChanged(id);
        }
    }
}

void RangeMonitor::reset()
{
    const QVector<QString> gone = m_order;
    m_nodes.clear();
    m_order.clear();
    m_rejected = 0;
    m_accepted = 0;
    for (const QString& id : gone)
        emit nodeRemoved(id);
}

} // namespace rms
} // namespace ta
