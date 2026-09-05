#ifndef TA_TELEMETRY_NODETELEMETRYSERVICE_H
#define TA_TELEMETRY_NODETELEMETRYSERVICE_H

// ─────────────────────────────────────────────────────────────────────────────
// THE TARGET NODE'S RMS TELEMETRY PUBLISHER.
//
// It observes this station's own authoritative record and describes it on the
// wire. It decides nothing, scores nothing, and can refuse nothing.
//
// WHERE IT SUBSCRIBES, AND WHY THERE.
//   SessionStore::eventApplied is the single point at which an event has been
//   validated, accepted by the reducer and applied to the authoritative state.
//   Subscribing there means telemetry can only ever describe shots the node
//   has ALREADY accepted. It is structurally impossible for this class to see
//   a raw Modbus coordinate read, a candidate shot, a rejected shot, or a UI
//   click, because none of those reach the store.
//
//   `replayed` events are skipped. A recovery replay is history being rebuilt,
//   not a shot being fired; protocol v1 has no historical-replay message, and
//   re-announcing old shots as live ones would be a lie about when they
//   happened.
//
// IT CANNOT AFFECT A SHOT.
//   The eventApplied slot only formats bytes and appends them to a bounded
//   outbox — no socket call, no allocation-heavy work, no waiting, and above
//   all no return value the acquisition path could act on. Sending happens
//   later, on a timer, off that call stack. If the outbox is full the OLDEST
//   telemetry is dropped and counted; the shot is untouched. If the send
//   fails, it is counted and the datagram is dropped; the shot is untouched.
//   If RMS is not running, nothing anywhere behaves differently.
//
// QtCore only. The UDP socket lives behind ITelemetrySink.
// ─────────────────────────────────────────────────────────────────────────────

#include "ITelemetrySink.h"
#include "NodeIdentity.h"

#include "reliability/store/SessionStore.h"
#include "rms/RmsProtocol.h"

#include <QHash>
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QVector>

#include <functional>

class QTimer;

namespace ta {
namespace telemetry {

class NodeTelemetryService : public QObject
{
    Q_OBJECT
public:
    // The outbox is deliberately small. Telemetry is a live view, not an
    // archive: if RMS cannot keep up, the RIGHT answer is to lose old
    // observations and keep the recent ones, and to let RMS reconcile from
    // the node's authoritative counts. An unbounded queue would trade a
    // display gap for a memory leak during a match.
    static constexpr int kOutboxCapacity = 256;

    NodeTelemetryService(NodeIdentity identity, ITelemetrySink* sink,
                         QObject* parent = nullptr);
    ~NodeTelemetryService() override;

    // ── node description ─────────────────────────────────────────────────
    void setAppVersion(const QString& v) { m_appVersion = v; }
    void setProductIdentity(const QString& v) { m_productIdentity = v; }
    // The target hardware fingerprint, distinct from the node identity: the
    // device can be swapped under a station.
    void setDeviceIdentity(const QString& v) { m_deviceIdentity = v; }
    // Provisional display text only (milestone 2 §20). The authoritative
    // nodeId ↔ laneId mapping is range configuration and belongs to RMS in
    // milestone 3; this is whatever the station has been told locally.
    void setLaneHint(const QString& v) { m_laneHint = v; }
    // Driven from the existing target-status signal. RMS never infers the
    // target link from datagram arrival — a node can be reachable with its
    // target unplugged, and those must not look the same on a range display.
    void setTargetConnected(bool connected);

    // The stable programme identity chosen from CompetitionCatalogue. Set from
    // QML at selection time. Never a display label (QML-LANG-001).
    Q_INVOKABLE void setProgramme(const QString& programmeId,
                                  const QString& rulesetId,
                                  const QString& targetStandardId);

    // ── the accepted-shot seam ───────────────────────────────────────────
    // Attach a COMPETITION session store. Safe to call more than once and
    // with more than one store; each is observed independently.
    void attachStore(ta::rel::SessionStore* store);

    // ── lifecycle ────────────────────────────────────────────────────────
    // Announces, then keeps announcing slowly and heart-beating.
    void start();
    void stop();
    bool isRunning() const { return m_running; }

    // ── deterministic drive points (production timers call these too) ─────
    void publishAnnounce();
    void publishStatus();
    // Sends what is queued. Returns the number of datagrams actually sent.
    int  flushOutbox();

    // Tests inject a clock so nothing here depends on the wall clock.
    void setClockForTesting(std::function<qint64()> clock) { m_clock = std::move(clock); }

    // ── observation counters (diagnostics, logs, tests) ──────────────────
    int queuedCount() const { return int(m_outbox.size()); }
    int droppedCount() const { return m_dropped; }
    int sendFailures() const { return m_sendFailures; }
    int shotsPublished() const { return m_shotsPublished; }
    int announcesPublished() const { return m_announces; }
    int statusPublished() const { return m_statuses; }
    quint64 statusSeq() const { return m_statusSeq; }
    QString nodeId() const { return m_identity.nodeId(); }
    QString bootId() const { return m_identity.bootId(); }

signals:
    // Diagnostics only. Nothing in the shot path listens to these.
    void telemetrySendFailed(QString detail);
    void telemetryDropped(int totalDropped);

    // WHAT HAPPENED TO ONE DATAGRAM, WITH ENOUGH TO IDENTIFY IT.
    //
    // The first physical test lost exactly one accepted shot between this
    // publisher and RMS, and the node could not say which side lost it -
    // because the only signals it had were a bare count and a string, neither
    // carrying an eventId. A telemetry path that cannot name the event it
    // dropped cannot be diagnosed, only guessed at.
    //
    // `stage` is one of: QUEUED, SENT, SEND_FAILED, DROPPED_OVERFLOW.
    void telemetryStage(const QString& stage, const QString& messageType,
                        const QString& eventId, const QString& sessionId,
                        int shotSequence, int bytes, int queueDepth,
                        const QString& detail);

private:
    void onEventApplied(ta::rel::SessionStore* store,
                        const ta::rel::DomainEvent& event, bool replayed);
    void enqueue(const QByteArray& datagram, const QString& messageType,
                 const QString& eventId = QString(),
                 const QString& sessionId = QString(),
                 int shotSequence = 0);
    qint64 nowMs() const;

    ta::rms::NodeStatus buildStatus() const;
    // The store whose session RMS should be shown, or nullptr when this
    // station has no competition session open.
    ta::rel::SessionStore* activeStore() const;

    NodeIdentity   m_identity;
    ITelemetrySink* m_sink = nullptr;
    std::function<qint64()> m_clock;

    QString m_appVersion;
    QString m_productIdentity;
    QString m_deviceIdentity;
    QString m_laneHint;
    QString m_programmeId, m_rulesetId, m_targetStandardId;
    bool    m_targetConnected = false;

    QVector<ta::rel::SessionStore*> m_stores;
    ta::rel::SessionStore* m_activeStore = nullptr;

    // Guards against one accepted shot being described twice, whichever
    // internal path delivered it. Keyed by sessionId + official shot number,
    // which is also exactly how the eventId is derived — so a shot that IS
    // re-sent keeps its identity and RMS suppresses the copy.
    QString       m_publishedSession;
    QSet<qint32>  m_publishedShots;

    // The datagram AND enough context to name it later. Storing bytes alone is
    // what made a send failure anonymous: by the time flushOutbox() fails, the
    // shot it belonged to is otherwise unknowable without re-parsing JSON in
    // the failure path.
    struct Outgoing {
        QByteArray  datagram;
        QString     messageType;
        QString     eventId;
        QString     sessionId;
        int         shotSequence = 0;
    };
    QQueue<Outgoing> m_outbox;
    QTimer* m_flushTimer = nullptr;
    QTimer* m_statusTimer = nullptr;
    QTimer* m_announceTimer = nullptr;

    bool    m_running = false;
    quint64 m_statusSeq = 0;
    int     m_dropped = 0;
    int     m_sendFailures = 0;
    int     m_shotsPublished = 0;
    int     m_announces = 0;
    int     m_statuses = 0;
};

} // namespace telemetry
} // namespace ta

#endif // TA_TELEMETRY_NODETELEMETRYSERVICE_H
