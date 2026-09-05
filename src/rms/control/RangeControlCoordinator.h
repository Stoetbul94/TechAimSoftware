#ifndef TA_RMS_RANGECONTROLCOORDINATOR_H
#define TA_RMS_RANGECONTROLCOORDINATOR_H

// The RMS side of range control: gap detection, automatic catch-up, the
// command audit, the reconciliation watermark, time-offset measurement and
// scheduled starts.
//
// TRANSPORT-FREE, like the rest of the control plane. A caller supplies a
// `Link` that moves bytes to one node and back; production supplies a socket,
// tests supply a direct call. Every decision in this file is therefore
// provable without a network.
//
// WHAT IT DOES NOT DO. It does not score, it does not re-derive anything from
// coordinates, and it never decides that a shot happened. It compares
// SEQUENCES - what the node says it has against what RMS holds - and asks for
// the difference. The node remains authoritative throughout.

#include "rms/RangeMonitor.h"
#include "rms/RmsJsonStore.h"
#include "rms/control/CommandJournal.h"
#include "rms/control/ControlProtocol.h"
#include "rms/control/RmsControlClient.h"

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QSet>
#include <QString>
#include <functional>

namespace ta {
namespace rms {
namespace control {

// WHERE A NODE'S CONTROL CHANNEL IS. Deliberately separate from telemetry: a
// healthy UDP heartbeat says nothing about whether commands can be delivered,
// and R2B proved the two can disagree for a whole restart.
enum class ControlLinkState {
    Disconnected,       // no channel, and none attempted
    RestartDetected,    // the node's bootId changed; the old channel is void
    Reauthenticating,   // handshaking again against the SAME nodeId
    Authenticated,      // a channel exists and is trusted
    Replaying,          // authenticated, and fetching what was missed
    Current,            // authenticated, and RMS holds everything the node has
    Failed              // the handshake was refused
};
QString controlLinkStateName(ControlLinkState s);

// WHAT AN AUDIT LINE MEANS. Without this, a retry that the node correctly
// suppressed and a command that genuinely ran a second time look identical in
// the record - which would make the audit worse than useless for the one
// question it exists to answer.
enum class AuditKind {
    Initial,             // first issue of this commandId
    Retry,               // the SAME commandId, sent again on purpose
    DuplicateSuppressed, // the node recognised it; NOTHING ran a second time
    AckRecovered,        // a retry recovered the outcome of a lost ack
    NodeRestart,         // observed, not commanded
    Reauthentication     // control authority re-established
};
QString auditKindName(AuditKind k);

// One state-changing command, as persisted. Deliberately carries NO key, NO
// MAC and NO nonce: an audit trail exists to answer "who asked what, and what
// happened", and none of those help.
struct CommandAuditEntry {
    AuditKind kind = AuditKind::Initial;
    // True when the node answered that it had already handled this id. The
    // action did NOT happen twice, and this line must never be counted as a
    // second execution.
    bool    duplicate = false;
    QString bootId;
    QString commandId;
    QString commandType;
    QString nodeId;
    QString laneId;
    QString sessionId;
    qint64  issuedAtUtcMs = 0;
    bool    accepted = false;
    QString reasonCode;
    qint64  ackUtcMs = 0;
    QJsonObject resultingState;

    QJsonObject toJson() const;
    static CommandAuditEntry fromJson(const QJsonObject& o);
};

// How far RMS has reconciled a given node/session. Persisted, because an
// in-memory-only watermark is exactly what is lost in the crash it exists to
// survive.
struct ReconciliationWatermark {
    QString nodeId;
    QString sessionId;
    QString lastBootId;
    int     highestSequence = 0;

    QJsonObject toJson() const;
    static ReconciliationWatermark fromJson(const QJsonObject& o);
};

// Time-offset measurement quality. Named states rather than a fabricated
// tolerance: this measures software behaviour, and claiming a competition-grade
// bound from it would be inventing evidence.
enum class SyncQuality { Unusable, Degraded, Good };
QString syncQualityName(SyncQuality q);

struct TimeSync {
    qint64      offsetMs = 0;      // add to an RMS instant to get node time
    qint64      rttMs = 0;
    qint64      uncertaintyMs = 0; // half the round trip: the honest bound
    SyncQuality quality = SyncQuality::Unusable;
};

// The result of a command sent to one node.
struct CommandOutcome {
    QString nodeId;
    QString laneId;
    QString commandId;
    bool    accepted = false;
    QString reasonCode;
    QString message;
    qint64  latencyMs = 0;
};

// A fan-out result. NEVER collapses to a single boolean: an operator who is
// told "all started" when one lane did not has been misinformed at the worst
// possible moment.
struct FanOutResult {
    QList<CommandOutcome> outcomes;
    int accepted() const;
    int failed() const;
    QList<CommandOutcome> failures() const;
    bool allAccepted() const { return failed() == 0; }
};

class RangeControlCoordinator
{
public:
    // Moves a frame to one node and returns whatever came back. Returning an
    // empty QByteArray means the node is unreachable - which is a FAILED
    // command, not a silent success.
    using Link = std::function<QByteArray(const QString& nodeId, const QByteArray& frame)>;

    RangeControlCoordinator(QString rmsInstanceId, QByteArray rangeKey);
    ~RangeControlCoordinator();

    // The client channels are owned outright and are not QObjects, so copying
    // this would double-free them.
    RangeControlCoordinator(const RangeControlCoordinator&) = delete;
    RangeControlCoordinator& operator=(const RangeControlCoordinator&) = delete;

    void setLink(Link link) { m_link = std::move(link); }

    // Called immediately BEFORE each handshake, with the node about to be
    // dialled. A real transport uses it to drop any socket it still holds: the
    // node builds one endpoint per connection, so a new handshake arriving on
    // an old socket would be answered by an endpoint that already believes it
    // is authenticated. Purely a notification - it makes no decision and the
    // handshake logic below is unchanged.
    void setPreConnectHook(std::function<void(const QString&)> hook)
    { m_preConnect = std::move(hook); }

    // Authenticates a control channel to one node. Returns false and records
    // the reason on any failure - wrong key, wrong identity, wrong version.
    bool connectNode(const QString& nodeId);
    bool isAuthenticated(const QString& nodeId) const;
    QString lastError(const QString& nodeId) const;

    // ── boot transitions and control authority (§7-§10) ───────────────────
    //
    // R2B left RMS believing a channel was authenticated after the node behind
    // it had restarted. The node refused the next command, so nothing was
    // wrongly applied - but RMS discovered the restart by being told NO, which
    // is a bad way to learn it and no way to recover automatically.
    //
    // Now the boot identity in the node's own telemetry drives it. A bootId
    // change means the process was replaced, so whatever authenticated to the
    // previous one is VOID by definition.
    ControlLinkState linkState(const QString& nodeId) const;
    QString observedBootId(const QString& nodeId) const
    { return m_knownBoot.value(nodeId); }
    int restartsObserved(const QString& nodeId) const
    { return m_restartsSeen.value(nodeId, 0); }
    int reauthentications() const { return m_reauths; }

    // Called with what telemetry reports. A CHANGED bootId retires the channel
    // immediately; an unchanged one does nothing at all.
    //
    // BOOT IS NOT IDENTITY (§8). This never creates a lane, a node, an athlete
    // assignment or a session - a new process incarnation is the same node
    // doing the same job, and treating it as a new one would split a live
    // athlete's match in half.
    void noteBootIdentity(const QString& nodeId, const QString& bootId,
                          qint64 nowUtcMs);

    // THE AUTOMATIC LOOP. Boot transitions, reauthentication, retry of pending
    // commands and catch-up, for the whole range, with no operator action.
    // Returns the number of nodes whose control authority it re-established.
    int serviceNodes(RangeMonitor* monitor, qint64 nowUtcMs);

    // ── pending commands (§9) ─────────────────────────────────────────────
    // A command whose answer never arrived. RMS does not know whether the node
    // applied it, so it is neither forgotten nor re-invented: it is retried
    // with the SAME commandId after reauthentication. Minting a new id for the
    // same operator intent would defeat exactly-once protection.
    int pendingCommandCount(const QString& nodeId) const;
    int pendingCommandCount() const;

    // ── time sync (§16) ───────────────────────────────────────────────────
    // t0 send, t1/t2 at the node, t3 receive. Offset and round trip are
    // estimated the classic way; uncertainty is half the round trip, which is
    // the strongest statement this exchange can honestly support.
    TimeSync measureTimeSync(const QString& nodeId, qint64 t0, qint64 t3,
                             qint64 nodeT1, qint64 nodeT2);
    TimeSync timeSync(const QString& nodeId) const { return m_sync.value(nodeId); }

    // ── commands ──────────────────────────────────────────────────────────
    CommandOutcome send(const QString& nodeId, const QString& laneId,
                        const QString& commandType, const QJsonObject& payload,
                        qint64 nowUtcMs, const QString& commandId = QString());

    // One, selected, or all. Each node gets its OWN commandId and its own
    // outcome - "all lanes" is N commands, not one broadcast that cannot say
    // which lane failed.
    FanOutResult sendToMany(const QStringList& nodeIds, const QString& commandType,
                            const QJsonObject& payload, qint64 nowUtcMs);

    // START_AT. Refuses when sync quality is Unusable rather than scheduling a
    // start it cannot place on the node's clock.
    FanOutResult startAt(const QStringList& nodeIds, qint64 startAtUtcMs,
                         qint64 nowUtcMs);

    // ── catch-up (§6, §7) ─────────────────────────────────────────────────
    // Compares what the node says it holds against what RMS holds, and fetches
    // the difference. Returns the number of events ingested. Runs
    // automatically as part of reconnect; no operator click required.
    int catchUp(const QString& nodeId, RangeMonitor* monitor, qint64 nowUtcMs);

    // Every node the monitor knows that is behind. This is the automatic path.
    int reconcileAll(RangeMonitor* monitor, qint64 nowUtcMs);

    // ── persistence (§12, §13) ────────────────────────────────────────────
    // The schema this build writes and is willing to read.
    static constexpr int kControlStateSchemaVersion = 1;

    QJsonObject saveState() const;
    void        loadState(const QJsonObject& o);

    // Through the SAME versioned atomic store every other RMS document uses:
    // temp-file-then-rename, schemaVersion stamped by the store, and a document
    // from a newer RMS refused rather than overwritten. Not a second private
    // file format, and deliberately not a database - this is a few hundred
    // records that must survive a crash, which is what that store is for.
    StoreResult saveTo(RmsJsonStore& store) const;
    StoreResult loadFrom(RmsJsonStore& store);

    QList<CommandAuditEntry> audit() const { return m_audit; }
    ReconciliationWatermark watermark(const QString& nodeId) const
    { return m_watermarks.value(nodeId); }

    int replayBatchesReceived() const { return m_replayBatches; }
    int replayEventsIngested() const  { return m_replayEvents; }

    // Audit lines that represent a SECOND physical execution of an action.
    // Must stay at zero: a duplicate the node suppressed is not one of these.
    int semanticDoubleExecutions() const;

private:
    void markReconciled(const QString& nodeId, RangeMonitor* monitor);
    void recordAudit(const CommandOutcome& o, const QString& type,
                     const QString& laneId, const QString& sessionId,
                     qint64 issuedAt, const QJsonObject& state,
                     AuditKind kind, bool duplicate);
    void recordEvent(AuditKind kind, const QString& nodeId, const QString& bootId,
                     qint64 nowUtcMs, const QString& reasonCode);
    void invalidateControl(const QString& nodeId, const QString& newBootId,
                           qint64 nowUtcMs);
    void retryPending(const QString& nodeId, qint64 nowUtcMs);
    void rememberPending(const QString& nodeId, const QString& commandId,
                         const QString& commandType, const QString& laneId,
                         const QJsonObject& payload, qint64 nowUtcMs);
    void forgetPending(const QString& nodeId, const QString& commandId);

    // A command sent but not answered. Held so it can be retried with the SAME
    // id, which is what the node's journal recognises.
    struct PendingCommand {
        QString commandId, commandType, laneId;
        QJsonObject payload;
        qint64 issuedAtUtcMs = 0;
        int    attempts = 1;

        QJsonObject toJson() const;
        static PendingCommand fromJson(const QJsonObject& o);
    };

    std::function<void(const QString&)> m_preConnect;
    QString    m_instanceId;
    QByteArray m_key;
    Link       m_link;

    QHash<QString, RmsControlClient*> m_clients;
    QHash<QString, ControlLinkState>  m_state;
    QHash<QString, QString>           m_knownBoot;
    QHash<QString, int>               m_restartsSeen;
    QHash<QString, QList<PendingCommand>> m_pending;
    // Every commandId RMS has issued, so a re-issue is recognisable as a RETRY
    // rather than mistaken for a new command in the audit. Rebuilt from the
    // audit on load rather than persisted separately.
    QSet<QString> m_issued;
    int m_reauths = 0;
    QHash<QString, TimeSync>          m_sync;
    QHash<QString, ReconciliationWatermark> m_watermarks;
    QList<CommandAuditEntry> m_audit;
    int m_commandSeq = 0;
    int m_replayBatches = 0;
    int m_replayEvents = 0;
};

} // namespace control
} // namespace rms
} // namespace ta

#endif // TA_RMS_RANGECONTROLCOORDINATOR_H
