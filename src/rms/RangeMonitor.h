#ifndef TA_RMS_RANGEMONITOR_H
#define TA_RMS_RANGEMONITOR_H

// ─────────────────────────────────────────────────────────────────────────────
// The read-only range observer. ONE ingress (`ingestDatagram`) and NO egress.
//
// This class has no socket, no writer and no command API by construction —
// that is how the read-only invariant is enforced rather than merely promised.
// The UDP observer and the development simulator both hand their payloads to
// the same `ingestDatagram`, so there is exactly one parse-and-apply path to
// reason about and to test.
//
// TIME IS INJECTED. Every entry point takes `nowUtcMs`. Liveness, offline
// detection and reconnection are therefore fully deterministic in the harness,
// with no sleeps and no wall-clock flakiness.
// ─────────────────────────────────────────────────────────────────────────────

#include "TargetNodeRecord.h"

#include <QHash>
#include <QObject>
#include <QVector>

namespace ta {
namespace rms {

// What a node's bootId told us about the datagram carrying it.
enum class BootOutcome {
    Same,        // the boot we are already tracking
    Restarted,   // a new boot: the node's application restarted
    Stale        // a boot we have already superseded — a straggler, drop it
};

struct IngestOutcome {
    bool        accepted = false;
    MessageType type = MessageType::Unknown;
    QString     nodeId;
    QString     rejectReason;
    ShotIngest  shotResult = ShotIngest::Accepted;
    bool        nodeIsNew = false;
    bool        nodeRestarted = false;
    bool        staleBoot = false;
};

class RangeMonitor : public QObject
{
    Q_OBJECT
public:
    explicit RangeMonitor(QObject* parent = nullptr);

    // The one and only way data enters RMS.
    IngestOutcome ingestDatagram(const QByteArray& payload, qint64 nowUtcMs);

    // Mark nodes that have gone quiet as OFFLINE. Called on a timer by the
    // application and directly by the tests.
    void evaluateLiveness(qint64 nowUtcMs);

    // Silence after which a node is presumed offline. Three heartbeats.
    int  offlineTimeoutMs() const { return m_offlineTimeoutMs; }
    void setOfflineTimeoutMs(int ms) { m_offlineTimeoutMs = ms; }

    int nodeCount() const { return m_order.size(); }
    const TargetNodeRecord* nodeAt(int index) const;
    const TargetNodeRecord* nodeById(const QString& nodeId) const;
    QVector<QString> nodeIds() const { return m_order; }

    // Observation quality counters, for the status bar and the tests.
    int rejectedDatagrams() const { return m_rejected; }
    int acceptedDatagrams() const { return m_accepted; }

    // Simulates an RMS restart: everything RMS knew is dropped. The nodes are
    // untouched — that is the point of the test that uses this.
    void reset();

    // ── development only ─────────────────────────────────────────────────
    // Sets a station's COMPETITION state directly, tagged as a development
    // injection so nothing downstream can mistake it for a real elimination.
    //
    // It exists because protocol v1 carries no competition status at all, and
    // a display cannot be shown to handle a terminal state without one. It is
    // deliberately NOT part of ingestDatagram: no datagram can produce this,
    // which is what keeps "RMS never infers elimination" true — the only two
    // ways this field can move are a future v2 telemetry field and this
    // explicitly-labelled tool.
    void injectDevelopmentCompetitionState(const QString& nodeId,
                                           const CompetitionState& state);

signals:
    void nodeAdded(const QString& nodeId);
    void nodeChanged(const QString& nodeId);
    void nodeRemoved(const QString& nodeId);   // reserved; not emitted in M1
    void shotObserved(const QString& nodeId, int shotSequence);
    void datagramRejected(const QString& reason);

private:
    TargetNodeRecord& recordFor(const QString& nodeId, bool* created);
    // Classifies and applies a bootId. A node restart re-bases observation but
    // never discards the node from the range; a superseded boot is refused.
    BootOutcome applyBoot(TargetNodeRecord& r, const QString& bootId);

    QHash<QString, TargetNodeRecord> m_nodes;
    QVector<QString> m_order;           // stable insertion order for the UI
    int m_offlineTimeoutMs = 6000;
    int m_rejected = 0;
    int m_accepted = 0;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_RANGEMONITOR_H
