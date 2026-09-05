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

bool RangeMonitor::isStaleBoot(const TargetNodeRecord& r, const QString& bootId) const
{
    // Asks the question without CHANGING anything - applyBoot has side effects
    // (the drop counter, the restart counter, lastStatusSeq) that are right for
    // state messages and wrong for a historical shot.
    return !bootId.isEmpty() && r.bootId != bootId && r.priorBootIds.contains(bootId);
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

    m_lastDatagramUtcMs = nowUtcMs;

    const DecodedMessage msg = decode(payload);
    if (msg.type == MessageType::Unknown) {
        ++m_rejected;
        // Split by cause. A range chasing "why is nothing showing" needs to
        // know whether the packets are unreadable, a protocol RMS does not
        // speak, or simply a type it has no use for - three very different
        // problems that a single reject count hides.
        if (msg.rejectReason.contains(QLatin1String("protocolVersion")))
            ++m_unknownVersion;
        else if (msg.rejectReason.contains(QLatin1String("unknown type"))
                 || msg.rejectReason.contains(QLatin1String("unknown message")))
            ++m_unknownType;
        else
            ++m_malformed;
        out.rejectReason = msg.rejectReason;
        emit datagramRejected(msg.rejectReason);
        return out;
    }

    ++m_accepted;
    switch (msg.type) {
    case MessageType::NodeAnnounce: ++m_announces; break;
    case MessageType::NodeStatus:   ++m_statuses; break;
    case MessageType::AcceptedShot: ++m_shotMessages; break;
    default: break;
    }
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

        // A SHOT FROM A SUPERSEDED BOOT IS STILL A SHOT THE ATHLETE FIRED.
        //
        // The stale-boot guard exists to stop an old run's STATE overwriting
        // the current run's - an old heartbeat dragging the shot count or the
        // phase backwards. A shot is not state. It is an immutable historical
        // fact carrying its own unique eventId, and the ledger deduplicates on
        // that, so accepting a late one cannot corrupt anything.
        //
        // Dropping it, on the other hand, loses it permanently - and the case
        // is not hypothetical: after a node restart, catch-up replays exactly
        // these events, because the shots taken before the restart were taken
        // under the previous boot. Discarding them would leave a lane
        // permanently short of the shots its athlete actually fired.
        //
        // So it is ingested, and NOTHING about the node's identity, liveness or
        // boot bookkeeping is touched by it.
        if (isStaleBoot(r, sh.bootId)) {
            out.staleBoot = true;
            out.shotResult = r.ledger.ingest(sh);
            emit nodeChanged(sh.nodeId);
            if (out.shotResult != ShotIngest::DuplicateSuppressed
                && out.shotResult != ShotIngest::SequenceConflict)
                emit shotObserved(sh.nodeId, sh.shotSequence);
            return out;
        }

        const BootOutcome boot = applyBoot(r, sh.bootId);
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

void RangeMonitor::injectDevelopmentCompetitionState(const QString& nodeId,
                                                     const CompetitionState& state)
{
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end())
        return;
    it.value().competition = state;
    // Forced, whatever the caller passed: a value that arrived this way is a
    // demonstration, and an audit must always be able to tell it from a real
    // elimination the node decided.
    it.value().competition.source = CompetitionState::Source::DevelopmentInjection;
    emit nodeChanged(nodeId);
}

void RangeMonitor::reset()
{
    const QVector<QString> gone = m_order;
    m_nodes.clear();
    m_order.clear();
    m_rejected = 0;
    m_accepted = 0;
    m_announces = 0;
    m_statuses = 0;
    m_shotMessages = 0;
    m_malformed = 0;
    m_unknownVersion = 0;
    m_unknownType = 0;
    m_lastDatagramUtcMs = 0;
    for (const QString& id : gone)
        emit nodeRemoved(id);
}

} // namespace rms
} // namespace ta
