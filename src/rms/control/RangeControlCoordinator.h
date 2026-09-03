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
#include "rms/control/ControlProtocol.h"
#include "rms/control/RmsControlClient.h"

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <functional>

namespace ta {
namespace rms {
namespace control {

// One state-changing command, as persisted. Deliberately carries NO key, NO
// MAC and NO nonce: an audit trail exists to answer "who asked what, and what
// happened", and none of those help.
struct CommandAuditEntry {
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

    // Authenticates a control channel to one node. Returns false and records
    // the reason on any failure - wrong key, wrong identity, wrong version.
    bool connectNode(const QString& nodeId);
    bool isAuthenticated(const QString& nodeId) const;
    QString lastError(const QString& nodeId) const;

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

private:
    void markReconciled(const QString& nodeId, RangeMonitor* monitor);
    void recordAudit(const CommandOutcome& o, const QString& type,
                     const QString& laneId, const QString& sessionId,
                     qint64 issuedAt, const QJsonObject& state);

    QString    m_instanceId;
    QByteArray m_key;
    Link       m_link;

    QHash<QString, RmsControlClient*> m_clients;
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
