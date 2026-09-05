#include "rms/control/RangeControlCoordinator.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace ta {
namespace rms {
namespace control {

QString controlLinkStateName(ControlLinkState s)
{
    switch (s) {
    case ControlLinkState::RestartDetected:  return QStringLiteral("RESTART DETECTED");
    case ControlLinkState::Reauthenticating: return QStringLiteral("REAUTHENTICATING");
    case ControlLinkState::Authenticated:    return QStringLiteral("AUTHENTICATED");
    case ControlLinkState::Replaying:        return QStringLiteral("REPLAYING");
    case ControlLinkState::Current:          return QStringLiteral("CURRENT");
    case ControlLinkState::Failed:           return QStringLiteral("AUTH FAILURE");
    case ControlLinkState::Disconnected:     break;
    }
    return QStringLiteral("DISCONNECTED");
}

QString auditKindName(AuditKind k)
{
    switch (k) {
    case AuditKind::Retry:               return QStringLiteral("RETRY");
    case AuditKind::DuplicateSuppressed: return QStringLiteral("DUPLICATE SUPPRESSED");
    case AuditKind::AckRecovered:        return QStringLiteral("ACK RECOVERED");
    case AuditKind::NodeRestart:         return QStringLiteral("NODE RESTART");
    case AuditKind::Reauthentication:    return QStringLiteral("REAUTHENTICATION");
    case AuditKind::Initial:             break;
    }
    return QStringLiteral("INITIAL");
}

namespace {
AuditKind auditKindFromName(const QString& n)
{
    if (n == QLatin1String("RETRY"))                return AuditKind::Retry;
    if (n == QLatin1String("DUPLICATE SUPPRESSED")) return AuditKind::DuplicateSuppressed;
    if (n == QLatin1String("ACK RECOVERED"))        return AuditKind::AckRecovered;
    if (n == QLatin1String("NODE RESTART"))         return AuditKind::NodeRestart;
    if (n == QLatin1String("REAUTHENTICATION"))     return AuditKind::Reauthentication;
    return AuditKind::Initial;
}
}

QString syncQualityName(SyncQuality q)
{
    switch (q) {
    case SyncQuality::Good:     return QStringLiteral("GOOD");
    case SyncQuality::Degraded: return QStringLiteral("DEGRADED");
    case SyncQuality::Unusable: break;
    }
    return QStringLiteral("UNUSABLE");
}

QJsonObject CommandAuditEntry::toJson() const
{
    return QJsonObject{
        {"kind", auditKindName(kind)}, {"duplicate", duplicate},
        {"bootId", bootId},
        {"commandId", commandId}, {"commandType", commandType},
        {"nodeId", nodeId}, {"laneId", laneId}, {"sessionId", sessionId},
        {"issuedAtUtcMs", double(issuedAtUtcMs)}, {"accepted", accepted},
        {"reasonCode", reasonCode}, {"ackUtcMs", double(ackUtcMs)},
        {"resultingState", resultingState}};
}

CommandAuditEntry CommandAuditEntry::fromJson(const QJsonObject& o)
{
    CommandAuditEntry e;
    e.kind        = auditKindFromName(o.value("kind").toString());
    e.duplicate   = o.value("duplicate").toBool();
    e.bootId      = o.value("bootId").toString();
    e.commandId   = o.value("commandId").toString();
    e.commandType = o.value("commandType").toString();
    e.nodeId      = o.value("nodeId").toString();
    e.laneId      = o.value("laneId").toString();
    e.sessionId   = o.value("sessionId").toString();
    e.issuedAtUtcMs = qint64(o.value("issuedAtUtcMs").toDouble());
    e.accepted    = o.value("accepted").toBool();
    e.reasonCode  = o.value("reasonCode").toString();
    e.ackUtcMs    = qint64(o.value("ackUtcMs").toDouble());
    e.resultingState = o.value("resultingState").toObject();
    return e;
}

QJsonObject ReconciliationWatermark::toJson() const
{
    return QJsonObject{{"nodeId", nodeId}, {"sessionId", sessionId},
                       {"lastBootId", lastBootId},
                       {"highestSequence", highestSequence}};
}

ReconciliationWatermark ReconciliationWatermark::fromJson(const QJsonObject& o)
{
    ReconciliationWatermark w;
    w.nodeId = o.value("nodeId").toString();
    w.sessionId = o.value("sessionId").toString();
    w.lastBootId = o.value("lastBootId").toString();
    w.highestSequence = o.value("highestSequence").toInt();
    return w;
}

int FanOutResult::accepted() const
{
    int n = 0;
    for (const CommandOutcome& o : outcomes) if (o.accepted) ++n;
    return n;
}
int FanOutResult::failed() const { return outcomes.size() - accepted(); }
QList<CommandOutcome> FanOutResult::failures() const
{
    QList<CommandOutcome> f;
    for (const CommandOutcome& o : outcomes) if (!o.accepted) f.append(o);
    return f;
}

RangeControlCoordinator::RangeControlCoordinator(QString rmsInstanceId,
                                                 QByteArray rangeKey)
    : m_instanceId(std::move(rmsInstanceId)), m_key(std::move(rangeKey))
{
}

RangeControlCoordinator::~RangeControlCoordinator()
{
    qDeleteAll(m_clients);
}

bool RangeControlCoordinator::connectNode(const QString& nodeId)
{
    if (!m_link)
        return false;

    if (m_preConnect)
        m_preConnect(nodeId);
    delete m_clients.take(nodeId);
    auto* c = new RmsControlClient(m_instanceId, m_key);
    c->setExpectedNode(nodeId);          // identity is verified, not assumed
    m_clients.insert(nodeId, c);

    QByteArray toNode = c->start();
    for (int i = 0; i < 4 && !toNode.isEmpty(); ++i) {
        const QByteArray back = m_link(nodeId, toNode);
        if (back.isEmpty())
            break;                        // unreachable: NOT authenticated
        const auto r = c->onBytes(back);
        if (r.closeConnection)
            break;
        toNode = r.reply;
    }
    const bool ok = c->state() == RmsControlClient::State::Authenticated;
    // Capabilities arrive on the Challenge of THIS handshake, so a
    // reauthentication refreshes them by construction: a node that came back
    // running a different build is believed about the capabilities it now
    // advertises, not the ones the previous process advertised.
    m_state.insert(nodeId, ok ? ControlLinkState::Authenticated
                              : ControlLinkState::Failed);
    return ok;
}

ControlLinkState RangeControlCoordinator::linkState(const QString& nodeId) const
{
    return m_state.value(nodeId, ControlLinkState::Disconnected);
}

bool RangeControlCoordinator::isAuthenticated(const QString& nodeId) const
{
    const auto* c = m_clients.value(nodeId, nullptr);
    return c && c->state() == RmsControlClient::State::Authenticated;
}

QString RangeControlCoordinator::lastError(const QString& nodeId) const
{
    const auto* c = m_clients.value(nodeId, nullptr);
    return c ? c->lastError() : QStringLiteral("no control channel");
}

TimeSync RangeControlCoordinator::measureTimeSync(const QString& nodeId,
                                                  qint64 t0, qint64 t3,
                                                  qint64 nodeT1, qint64 nodeT2)
{
    TimeSync s;
    // The classic estimate. It assumes the path is roughly symmetric, which on
    // a range LAN is reasonable and on a congested link is not - which is
    // exactly why the uncertainty below is reported rather than hidden.
    s.rttMs    = (t3 - t0) - (nodeT2 - nodeT1);
    s.offsetMs = ((nodeT1 - t0) + (nodeT2 - t3)) / 2;
    // Half the round trip is the most this exchange can honestly bound the
    // offset by. It is not an NTP-grade figure and is not presented as one.
    s.uncertaintyMs = s.rttMs / 2;

    if (s.rttMs < 0)               s.quality = SyncQuality::Unusable;  // impossible
    else if (s.uncertaintyMs <= 25)  s.quality = SyncQuality::Good;
    else if (s.uncertaintyMs <= 250) s.quality = SyncQuality::Degraded;
    else                             s.quality = SyncQuality::Unusable;

    m_sync.insert(nodeId, s);
    return s;
}

CommandOutcome RangeControlCoordinator::send(const QString& nodeId,
                                             const QString& laneId,
                                             const QString& commandType,
                                             const QJsonObject& payload,
                                             qint64 nowUtcMs,
                                             const QString& commandId)
{
    CommandOutcome out;
    out.nodeId = nodeId;
    out.laneId = laneId;
    // Caller-supplied ids let a RETRY reuse one, which is what reaches the
    // node's idempotency. Generated ids are unique per command otherwise.
    out.commandId = commandId.isEmpty()
        ? QStringLiteral("%1-%2-%3").arg(m_instanceId, commandType).arg(++m_commandSeq)
        : commandId;

    // A RETRY is an explicit re-issue of an id RMS has already used. It is the
    // only way exactly-once protection can be reached at all, so it is named in
    // the audit rather than left looking like a second, separate command.
    const bool isRetry = !commandId.isEmpty() && m_issued.contains(commandId);
    const AuditKind kind = isRetry ? AuditKind::Retry : AuditKind::Initial;
    m_issued.insert(out.commandId);

    auto* c = m_clients.value(nodeId, nullptr);
    if (!c || c->state() != RmsControlClient::State::Authenticated) {
        out.accepted = false;
        out.reasonCode = QLatin1String(reason::kNotAuthenticated);
        out.message = QStringLiteral("control channel is not authenticated");
        rememberPending(nodeId, out.commandId, commandType, laneId, payload, nowUtcMs);
        recordAudit(out, commandType, laneId, QString(), nowUtcMs, QJsonObject(),
                    kind, false);
        return out;
    }

    Command cmd;
    cmd.controlProtocolVersion = kControlProtocolVersion;
    cmd.commandId   = out.commandId;
    cmd.nodeId      = nodeId;
    cmd.laneId      = laneId;
    cmd.commandType = commandType;
    cmd.issuedAtUtcMs = nowUtcMs;
    cmd.payload     = payload;

    const QByteArray back = m_link ? m_link(nodeId, c->sendCommand(cmd)) : QByteArray();
    if (back.isEmpty()) {
        // Unreachable. A command that could not be delivered is FAILED - never
        // assumed applied, which is the whole reason acks exist.
        //
        // But FAILED is not the same as DID NOT HAPPEN: the frame may have
        // reached the node and only the answer been lost. It is therefore held
        // as PENDING and retried later with the SAME id, which is the only kind
        // of retry the node journal can recognise as a duplicate.
        out.accepted = false;
        out.reasonCode = QStringLiteral("UNREACHABLE");
        out.message = QStringLiteral("node did not answer");
        rememberPending(nodeId, out.commandId, commandType, laneId, payload, nowUtcMs);
        recordAudit(out, commandType, laneId, QString(), nowUtcMs, QJsonObject(),
                    kind, false);
        return out;
    }

    const auto r = c->onBytes(back);
    bool duplicate = false;
    if (!r.gotAck) {
        out.accepted = false;
        out.reasonCode = QStringLiteral("NO_ACK");
        out.message = QStringLiteral("node answered without an acknowledgement");
        rememberPending(nodeId, out.commandId, commandType, laneId, payload, nowUtcMs);
    } else {
        out.accepted   = r.ack.accepted;
        out.reasonCode = r.ack.reasonCode;
        out.message    = r.ack.message;
        duplicate      = r.ack.duplicate;
        out.latencyMs  = r.ack.nodeTimestampUtcMs > 0
                             ? qAbs(r.ack.nodeTimestampUtcMs - nowUtcMs) : 0;
        // An answer of any kind settles it: the node has now spoken about this
        // id, so there is nothing left to retry.
        forgetPending(nodeId, out.commandId);
    }

    AuditKind resolved = kind;
    if (duplicate) {
        // The node recognised the id and did NOT act again. Recorded as a
        // suppression - or, when RMS was retrying precisely because it never
        // heard the first answer, as the RECOVERY of that lost ack. Neither is
        // a second execution, and neither may ever be counted as one.
        resolved = isRetry ? AuditKind::AckRecovered : AuditKind::DuplicateSuppressed;
    }
    recordAudit(out, commandType, laneId, QString(), nowUtcMs,
                r.gotAck ? r.ack.resultingState : QJsonObject(), resolved, duplicate);
    return out;
}

void RangeControlCoordinator::rememberPending(const QString& nodeId,
                                              const QString& commandId,
                                              const QString& commandType,
                                              const QString& laneId,
                                              const QJsonObject& payload,
                                              qint64 nowUtcMs)
{
    // Only state-changing commands are worth retrying. A lost PING answer costs
    // nothing, and re-sending diagnostics after every restart would put
    // pointless traffic on a range that has just had a problem.
    if (!isDurableCommand(commandType))
        return;
    QList<PendingCommand>& list = m_pending[nodeId];
    for (const PendingCommand& p : list)
        if (p.commandId == commandId)
            return;                    // already held; never duplicated
    PendingCommand p;
    p.commandId = commandId;
    p.commandType = commandType;
    p.laneId = laneId;
    p.payload = payload;
    p.issuedAtUtcMs = nowUtcMs;
    list.append(p);
}

void RangeControlCoordinator::forgetPending(const QString& nodeId,
                                            const QString& commandId)
{
    auto it = m_pending.find(nodeId);
    if (it == m_pending.end())
        return;
    for (int i = 0; i < it->size(); ++i) {
        if (it->at(i).commandId == commandId) { it->removeAt(i); break; }
    }
    if (it->isEmpty())
        m_pending.erase(it);
}

int RangeControlCoordinator::pendingCommandCount(const QString& nodeId) const
{
    return m_pending.value(nodeId).size();
}

int RangeControlCoordinator::pendingCommandCount() const
{
    int n = 0;
    for (const QList<PendingCommand>& l : m_pending) n += l.size();
    return n;
}

void RangeControlCoordinator::recordAudit(const CommandOutcome& o,
                                          const QString& type, const QString& laneId,
                                          const QString& sessionId, qint64 issuedAt,
                                          const QJsonObject& state,
                                          AuditKind kind, bool duplicate)
{
    // PING and REQUEST_STATUS are diagnostics: they change nothing, and
    // persisting a heartbeat's worth of them would bury the entries that
    // matter. Everything state-changing is recorded.
    if (type == QLatin1String(cmd::kPing) || type == QLatin1String(cmd::kRequestStatus))
        return;

    CommandAuditEntry e;
    e.kind = kind;
    e.duplicate = duplicate;
    e.bootId = m_knownBoot.value(o.nodeId);
    e.commandId = o.commandId;
    e.commandType = type;
    e.nodeId = o.nodeId;
    e.laneId = laneId;
    e.sessionId = sessionId;
    e.issuedAtUtcMs = issuedAt;
    e.accepted = o.accepted;
    e.reasonCode = o.reasonCode;
    e.ackUtcMs = issuedAt + o.latencyMs;
    e.resultingState = state;   // no key, no MAC, no nonce - by construction
    m_audit.append(e);
}

void RangeControlCoordinator::recordEvent(AuditKind kind, const QString& nodeId,
                                          const QString& bootId, qint64 nowUtcMs,
                                          const QString& reasonCode)
{
    // A restart and a reauthentication are things that HAPPENED TO the range,
    // not things RMS commanded. They belong in the same trail, because reading
    // a duplicate suppression without the restart that caused it explains
    // nothing.
    CommandAuditEntry e;
    e.kind = kind;
    e.nodeId = nodeId;
    e.bootId = bootId;
    e.issuedAtUtcMs = nowUtcMs;
    e.ackUtcMs = nowUtcMs;
    e.accepted = (kind == AuditKind::Reauthentication);
    e.reasonCode = reasonCode;
    m_audit.append(e);
}

int RangeControlCoordinator::semanticDoubleExecutions() const
{
    // An action counts as executed when the node accepted it AND did not say it
    // was a duplicate. Counting accepted duplicates here would report an
    // execution the node explicitly said it did not perform.
    QHash<QString, int> executions;
    for (const CommandAuditEntry& e : m_audit) {
        if (e.commandId.isEmpty() || !isDurableCommand(e.commandType))
            continue;
        if (e.accepted && !e.duplicate)
            ++executions[e.commandId];
    }
    int doubles = 0;
    for (auto it = executions.constBegin(); it != executions.constEnd(); ++it)
        if (it.value() > 1) ++doubles;
    return doubles;
}

FanOutResult RangeControlCoordinator::sendToMany(const QStringList& nodeIds,
                                                 const QString& commandType,
                                                 const QJsonObject& payload,
                                                 qint64 nowUtcMs)
{
    FanOutResult r;
    for (const QString& id : nodeIds)
        r.outcomes.append(send(id, QString(), commandType, payload, nowUtcMs));
    return r;
}

FanOutResult RangeControlCoordinator::startAt(const QStringList& nodeIds,
                                              qint64 startAtUtcMs, qint64 nowUtcMs)
{
    FanOutResult r;
    for (const QString& id : nodeIds) {
        const TimeSync s = m_sync.value(id);
        if (s.quality == SyncQuality::Unusable) {
            // Refused rather than scheduled. Without a usable offset the node
            // cannot place the instant on its own clock, and a start placed
            // wrongly is worse than a start refused.
            CommandOutcome o;
            o.nodeId = id;
            o.commandId = QStringLiteral("%1-STARTAT-%2").arg(m_instanceId).arg(++m_commandSeq);
            o.accepted = false;
            o.reasonCode = QStringLiteral("SYNC_UNUSABLE");
            o.message = QStringLiteral("time sync quality is unusable");
            r.outcomes.append(o);
            recordAudit(o, QLatin1String(cmd::kStartAt), QString(), QString(),
                        nowUtcMs, QJsonObject(), AuditKind::Initial, false);
            continue;
        }
        const QJsonObject payload{
            {"startAtUtcMs", double(startAtUtcMs)},
            {"rmsToNodeOffsetMs", double(s.offsetMs)},
            {"syncQuality", syncQualityName(s.quality)}};
        r.outcomes.append(send(id, QString(), QLatin1String(cmd::kStartAt),
                               payload, nowUtcMs));
    }
    return r;
}

void RangeControlCoordinator::noteBootIdentity(const QString& nodeId,
                                               const QString& bootId,
                                               qint64 nowUtcMs)
{
    if (nodeId.isEmpty() || bootId.isEmpty())
        return;

    const QString known = m_knownBoot.value(nodeId);
    if (known == bootId)
        return;                         // the boot we are already tracking

    if (known.isEmpty()) {
        // First sight of this node. Not a restart - there is nothing to
        // invalidate, and calling it one would inflate every range start.
        m_knownBoot.insert(nodeId, bootId);
        return;
    }

    invalidateControl(nodeId, bootId, nowUtcMs);
}

void RangeControlCoordinator::invalidateControl(const QString& nodeId,
                                                const QString& newBootId,
                                                qint64 nowUtcMs)
{
    // THE PROCESS BEHIND THE CHANNEL IS GONE. Whatever authenticated to the
    // previous incarnation authenticated to something that no longer exists, so
    // the channel is void by definition - not "probably stale", not "worth a
    // try". RMS retires it here rather than discovering it by being refused.
    delete m_clients.take(nodeId);

    m_knownBoot.insert(nodeId, newBootId);
    m_restartsSeen[nodeId] = m_restartsSeen.value(nodeId, 0) + 1;
    m_state.insert(nodeId, ControlLinkState::RestartDetected);

    // NOT TOUCHED, deliberately: the lane, the athlete, the session, the
    // ledger, the watermark and the time-sync measurement. A bootId is a
    // process incarnation, not an identity - the same node is doing the same
    // job for the same athlete, and resetting any of that would split a live
    // match in half. Only the CHANNEL is invalid.
    //
    // Pending commands are kept for the same reason. They were issued to this
    // NODE, and after reauthentication they are retried with their original
    // ids so the node journal can recognise them.
    recordEvent(AuditKind::NodeRestart, nodeId, newBootId, nowUtcMs,
                QStringLiteral("BOOT_CHANGED"));
}

void RangeControlCoordinator::retryPending(const QString& nodeId, qint64 nowUtcMs)
{
    const QList<PendingCommand> pending = m_pending.value(nodeId);
    for (const PendingCommand& p : pending) {
        // THE SAME commandId. Minting a new one for the same operator intent
        // would make the node treat it as a new command and apply it a second
        // time, which is precisely the failure being closed here.
        send(nodeId, p.laneId, p.commandType, p.payload, nowUtcMs, p.commandId);
    }
}

int RangeControlCoordinator::serviceNodes(RangeMonitor* monitor, qint64 nowUtcMs)
{
    if (!monitor)
        return 0;

    int reestablished = 0;
    for (int i = 0; i < monitor->nodeCount(); ++i) {
        const TargetNodeRecord* rec = monitor->nodeAt(i);
        if (!rec || rec->nodeId.isEmpty())
            continue;
        const QString nodeId = rec->nodeId;

        // 1. The node's own telemetry is what reveals the restart. RMS does
        //    not wait to be refused by a command it should never have sent.
        noteBootIdentity(nodeId, rec->bootId, nowUtcMs);

        // Anything without a live authenticated channel needs one. That covers
        // the restart just detected, an earlier handshake failure, AND an RMS
        // that has itself just restarted - after which no channel exists even
        // though the nodes never went anywhere.
        if (isAuthenticated(nodeId))
            continue;

        // 2. Reauthenticate against the SAME stable nodeId. Capabilities come
        //    back on this handshake, so they are refreshed rather than
        //    remembered from a process that no longer exists.
        m_state.insert(nodeId, ControlLinkState::Reauthenticating);
        if (!connectNode(nodeId)) {
            m_state.insert(nodeId, ControlLinkState::Failed);
            continue;                   // stays visible as a failure
        }
        ++m_reauths;
        ++reestablished;
        recordEvent(AuditKind::Reauthentication, nodeId, m_knownBoot.value(nodeId),
                    nowUtcMs, QLatin1String(reason::kOk));

        // 3. Anything that was in flight when the process died is retried with
        //    its ORIGINAL id, so the node answers with what it already did
        //    instead of doing it again.
        retryPending(nodeId, nowUtcMs);

        // 4. Then catch up on shots taken while RMS was not listening.
        m_state.insert(nodeId, ControlLinkState::Replaying);
        catchUp(nodeId, monitor, nowUtcMs);
        m_state.insert(nodeId, ControlLinkState::Current);
    }

    // Every authenticated node that is behind is reconciled on the same pass,
    // restart or no restart - an ordinary reconnect needs no operator either.
    reconcileAll(monitor, nowUtcMs);

    // Then settle each lane's reported state against what is actually true.
    // CURRENT is a stronger claim than AUTHENTICATED and must be earned: a
    // channel exists AND RMS holds everything the node says it has. A lane
    // still short of shots reads AUTHENTICATED, which is exactly what it is.
    for (int i = 0; i < monitor->nodeCount(); ++i) {
        const TargetNodeRecord* rec = monitor->nodeAt(i);
        if (!rec || !isAuthenticated(rec->nodeId))
            continue;
        const bool complete = rec->unobservedShotCount() == 0
                              && rec->ledger.missingSequences().isEmpty();
        m_state.insert(rec->nodeId, complete ? ControlLinkState::Current
                                             : ControlLinkState::Authenticated);
    }
    return reestablished;
}

void RangeControlCoordinator::markReconciled(const QString& nodeId,
                                             RangeMonitor* monitor)
{
    // Written from what RMS ACTUALLY holds, never from what it asked for.
    // Recording the request would claim a reconciliation that may not have
    // happened, and the shots between the claim and the truth would never be
    // asked for again.
    const TargetNodeRecord* n = monitor ? monitor->nodeById(nodeId) : nullptr;
    if (!n)
        return;
    ReconciliationWatermark w;
    w.nodeId = nodeId;
    w.sessionId = n->sessionId;
    w.lastBootId = n->bootId;
    w.highestSequence = n->ledger.highestSequence();
    m_watermarks.insert(nodeId, w);
}

int RangeControlCoordinator::catchUp(const QString& nodeId, RangeMonitor* monitor,
                                     qint64 nowUtcMs)
{
    if (!monitor || !isAuthenticated(nodeId))
        return 0;

    const TargetNodeRecord* rec = monitor->nodeById(nodeId);
    if (!rec)
        return 0;

    // THE GAP, from sequences only. Never from score totals: a total can match
    // while shots are missing, and a total cannot say WHICH are missing.
    //
    // TWO kinds of gap, and only asking about one of them would leave real
    // shots unrecovered:
    //   * a SHORTFALL - the node reports more accepted shots than RMS holds,
    //     which is what an offline stretch looks like;
    //   * a HOLE - RMS holds #17 and #19 but not #18, which is what ONE lost
    //     datagram looks like. The shortfall test alone misses this whenever a
    //     later shot arrived, because the highest sequence then already matches.
    const QList<int> holes = rec->ledger.missingSequences();
    const int shortfall = rec->shotsAcceptedByNode - rec->ledger.observedCount();
    if (shortfall <= 0 && holes.isEmpty()) {
        // Already current: no request at all. The WATERMARK is still recorded -
        // "reconciled up to 20 with nothing missing" is exactly the fact a
        // crash must not lose, and it is the common case, not the exception.
        markReconciled(nodeId, monitor);
        return 0;
    }

    // Copied out BEFORE any ingest: ingesting mutates the monitor, and a record
    // pointer read afterwards would be a pointer into something that has moved.
    const QString sessionId = rec->sessionId;

    // Start below the FIRST hole when there is one, so the missing middle is
    // fetched rather than skipped. Re-fetching shots RMS already holds is free:
    // the ledger deduplicates on eventId, which is exactly why replay events
    // keep their original ids.
    int after = holes.isEmpty() ? rec->ledger.highestSequence() : (holes.first() - 1);
    if (after < 0) after = 0;
    rec = nullptr;                      // do not read it again; ingest follows

    auto* c = m_clients.value(nodeId, nullptr);
    int ingested = 0;

    // Batched until the node says there is no more. Each batch is bounded, so
    // a long session is several requests rather than one huge response.
    for (int guard = 0; guard < 64; ++guard) {
        Command cmd;
        cmd.controlProtocolVersion = kControlProtocolVersion;
        cmd.commandId   = QStringLiteral("%1-REPLAY-%2").arg(m_instanceId).arg(++m_commandSeq);
        cmd.nodeId      = nodeId;
        cmd.commandType = QLatin1String(cmd::kRequestReplay);
        cmd.issuedAtUtcMs = nowUtcMs;
        cmd.payload = QJsonObject{{"sessionId", sessionId},
                                  {"afterSequence", after}};

        const QByteArray back = m_link ? m_link(nodeId, c->sendCommand(cmd)) : QByteArray();
        if (back.isEmpty())
            break;
        const auto r = c->onBytes(back);
        if (!r.gotReplay || r.replay.events.isEmpty())
            break;

        ++m_replayBatches;
        for (const QJsonObject& e : r.replay.events) {
            // The SAME ingest live telemetry uses, so dedup is inherited
            // rather than reimplemented. A replayed shot RMS already holds is
            // suppressed here exactly as a duplicated datagram would be.
            const IngestOutcome io =
                monitor->ingestDatagram(QJsonDocument(e).toJson(QJsonDocument::Compact),
                                        nowUtcMs);
            ++m_replayEvents;
            // Counted as RECOVERED only when it was genuinely new. Counting
            // every replayed event would report a recovery that did not happen.
            //
            // SessionRestarted belongs here: it is what the FIRST shot of a
            // session looks like when RMS learned of the node from a status
            // message and has never seen one of its shots - the ledger rebases
            // and stores it. Omitting it under-reports every cold catch-up by
            // exactly one shot.
            if (io.accepted && (io.shotResult == ShotIngest::Accepted
                                || io.shotResult == ShotIngest::AcceptedOutOfOrder
                                || io.shotResult == ShotIngest::SessionRestarted))
                ++ingested;
        }

        const int next = r.replay.nextSequence;
        // A batch that does not advance the cursor would loop forever asking
        // the same question. Stop instead of spinning against the guard.
        if (next <= after)
            break;
        after = next;
        if (!r.replay.hasMore)
            break;
    }

    markReconciled(nodeId, monitor);
    return ingested;
}

int RangeControlCoordinator::reconcileAll(RangeMonitor* monitor, qint64 nowUtcMs)
{
    if (!monitor) return 0;
    int total = 0;
    for (int i = 0; i < monitor->nodeCount(); ++i) {
        const auto* n = monitor->nodeAt(i);
        if (!n) continue;
        // Automatic: any authenticated node the monitor says is behind gets
        // caught up. No operator click is required for an ordinary reconnect.
        if (isAuthenticated(n->nodeId))
            total += catchUp(n->nodeId, monitor, nowUtcMs);
    }
    return total;
}

QJsonObject RangeControlCoordinator::saveState() const
{
    QJsonArray marks;
    for (const ReconciliationWatermark& w : m_watermarks)
        marks.append(w.toJson());
    QJsonArray aud;
    for (const CommandAuditEntry& e : m_audit)
        aud.append(e.toJson());
    // PENDING COMMANDS ARE PERSISTED. Without this, an RMS crash between
    // issuing a command and hearing its answer would lose the fact that an
    // answer is still owed - and RMS would either forget the operator intent
    // entirely or reissue it under a NEW id, which the node journal could not
    // recognise. Either way exactly-once is defeated by the RMS side alone.
    QJsonArray pend;
    for (auto it = m_pending.constBegin(); it != m_pending.constEnd(); ++it) {
        for (const PendingCommand& p : it.value()) {
            QJsonObject o = p.toJson();
            o.insert(QStringLiteral("nodeId"), it.key());
            pend.append(o);
        }
    }
    // The boot identities RMS last saw, so a node that restarted WHILE RMS was
    // down is still recognised as restarted when RMS comes back.
    QJsonArray boots;
    for (auto it = m_knownBoot.constBegin(); it != m_knownBoot.constEnd(); ++it) {
        boots.append(QJsonObject{{"nodeId", it.key()}, {"bootId", it.value()},
                                 {"restarts", m_restartsSeen.value(it.key(), 0)}});
    }

    // No schemaVersion stamped here: the store stamps it, so no caller can
    // forget to and no two callers can disagree about it.
    return QJsonObject{{"watermarks", marks},
                       {"commandAudit", aud},
                       {"pendingCommands", pend},
                       {"observedBoots", boots}};
}

QJsonObject RangeControlCoordinator::PendingCommand::toJson() const
{
    return QJsonObject{{"commandId", commandId}, {"commandType", commandType},
                       {"laneId", laneId}, {"payload", payload},
                       {"issuedAtUtcMs", double(issuedAtUtcMs)},
                       {"attempts", attempts}};
}

RangeControlCoordinator::PendingCommand
RangeControlCoordinator::PendingCommand::fromJson(const QJsonObject& o)
{
    PendingCommand p;
    p.commandId   = o.value("commandId").toString();
    p.commandType = o.value("commandType").toString();
    p.laneId      = o.value("laneId").toString();
    p.payload     = o.value("payload").toObject();
    p.issuedAtUtcMs = qint64(o.value("issuedAtUtcMs").toDouble());
    p.attempts    = o.value("attempts").toInt(1);
    return p;
}

void RangeControlCoordinator::loadState(const QJsonObject& o)
{
    m_watermarks.clear();
    m_audit.clear();
    const QJsonArray marks = o.value("watermarks").toArray();
    for (const QJsonValue& v : marks) {
        const ReconciliationWatermark w = ReconciliationWatermark::fromJson(v.toObject());
        if (!w.nodeId.isEmpty()) m_watermarks.insert(w.nodeId, w);
    }
    const QJsonArray aud = o.value("commandAudit").toArray();
    for (const QJsonValue& v : aud)
        m_audit.append(CommandAuditEntry::fromJson(v.toObject()));

    m_pending.clear();
    const QJsonArray pend = o.value("pendingCommands").toArray();
    for (const QJsonValue& v : pend) {
        const QJsonObject po = v.toObject();
        const QString nodeId = po.value("nodeId").toString();
        const PendingCommand p = PendingCommand::fromJson(po);
        if (!nodeId.isEmpty() && !p.commandId.isEmpty())
            m_pending[nodeId].append(p);
    }

    m_knownBoot.clear();
    m_restartsSeen.clear();
    const QJsonArray boots = o.value("observedBoots").toArray();
    for (const QJsonValue& v : boots) {
        const QJsonObject bo = v.toObject();
        const QString nodeId = bo.value("nodeId").toString();
        if (nodeId.isEmpty()) continue;
        m_knownBoot.insert(nodeId, bo.value("bootId").toString());
        m_restartsSeen.insert(nodeId, bo.value("restarts").toInt());
    }

    // Rebuilt rather than stored: every commandId in the audit was issued, so a
    // re-issue after an RMS restart is still recognised as a RETRY instead of
    // being logged as a fresh command.
    m_issued.clear();
    for (const CommandAuditEntry& e : m_audit)
        if (!e.commandId.isEmpty())
            m_issued.insert(e.commandId);

    // Every channel died with the process. Nothing is assumed authenticated
    // across an RMS restart - the handshakes are redone.
    qDeleteAll(m_clients);
    m_clients.clear();
    m_state.clear();
}

StoreResult RangeControlCoordinator::saveTo(RmsJsonStore& store) const
{
    return store.save(kControlStateSchemaVersion, saveState());
}

StoreResult RangeControlCoordinator::loadFrom(RmsJsonStore& store)
{
    QJsonObject doc;
    const StoreResult r = store.load(kControlStateSchemaVersion, &doc);
    // NOTHING is loaded on failure. A partially-applied watermark would tell
    // RMS it had reconciled further than it had, and the shots between the two
    // points would never be asked for again.
    if (r.ok)
        loadState(doc);
    return r;
}

} // namespace control
} // namespace rms
} // namespace ta
