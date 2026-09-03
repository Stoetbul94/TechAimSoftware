#ifndef TA_RMS_DEV_CONTROLLEDNODE_H
#define TA_RMS_DEV_CONTROLLEDNODE_H

// A simulated target node that speaks the REAL control protocol.
//
// NO SIMULATOR-ONLY PROTOCOL. This composes the production
// NodeControlEndpoint and the production handler interface, so the framing,
// the HMAC handshake, the command envelope, the ack model, the replay schema
// and the idempotency rules under test are the ones that will ship. Nothing
// here calls RMS internals directly to manufacture a result - if a command
// succeeds in a test, it succeeded through the same bytes a real node would
// have parsed.
//
// WHAT IT MODELS that the telemetry simulator could not:
//   * a telemetry link that goes down while the node KEEPS SHOOTING, so
//     events accumulate at the node and RMS falls behind
//   * a process restart: same nodeId, NEW bootId, same recovered session
//   * a session history deep enough to need several replay batches
//
// It is confined to src/rms/dev/, which the read-only guard excludes by name,
// because a simulator plays the node's role and is allowed to transmit.

#include "rms/control/ControlProtocol.h"
#include "rms/control/NodeControlEndpoint.h"

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>

namespace ta {
namespace rms {
namespace dev {

class ControlledNode : public control::IControlCommandHandler
{
public:
    ControlledNode(const QString& nodeId, const QString& laneId,
                   const QString& sessionId, const QByteArray& rangeKey);

    // ── the node's own life ───────────────────────────────────────────────

    // Fires `count` shots. They ALWAYS enter the node's authoritative history;
    // whether RMS hears about them depends on the telemetry link. That is the
    // whole point of offline shooting: the athlete's record is never at the
    // mercy of the network.
    void fire(int count, qint64 nowUtcMs);

    // The telemetry link. Down means RMS stops hearing; the node does not stop.
    void setTelemetryLink(bool up) { m_linkUp = up; }
    bool telemetryLink() const { return m_linkUp; }

    // Datagrams produced since the last drain, for the caller to feed to the
    // monitor. While the link is down this stays empty and the events are held.
    QList<QByteArray> drainTelemetry();

    // A process restart: same nodeId, same session, NEW bootId. The control
    // endpoint is recreated because a restarted process has no memory of the
    // connection - and, deliberately, no memory of handled command ids either.
    void restart();

    // ── control plane ─────────────────────────────────────────────────────
    control::NodeControlEndpoint& endpoint() { return *m_endpoint; }

    QString nodeId() const    { return m_nodeId; }
    QString laneId() const    { return m_laneId; }
    QString sessionId() const { return m_sessionId; }
    QString bootId() const    { return m_bootId; }
    int     eventCount() const { return m_events.size(); }
    int     restarts() const  { return m_restarts; }

    // Commands this node actually applied, in order - so a test can assert
    // WHAT happened, not merely that an ack came back.
    QStringList applied() const { return m_applied; }

    // Scheduled start, in the NODE's own clock. -1 when not scheduled.
    qint64 scheduledStartNodeMs() const { return m_scheduledStartNodeMs; }
    bool   started() const { return m_started; }

    // The node's clock differs from the RMS clock by this much. The simulator
    // sets it per lane so time sync has something real to measure.
    void   setClockOffsetMs(qint64 off) { m_clockOffsetMs = off; }
    qint64 clockOffsetMs() const { return m_clockOffsetMs; }
    qint64 nodeNowMs(qint64 rmsNowMs) const { return rmsNowMs + m_clockOffsetMs; }

    // ── IControlCommandHandler ────────────────────────────────────────────
    Result apply(const control::Command& c) override;
    QList<QJsonObject> replayEvents(const QString& sessionId, int afterSequence,
                                    int maxEvents, bool* hasMoreOut) override;

private:
    QJsonObject buildShot(int seq, qint64 nowUtcMs) const;

    QString m_nodeId, m_laneId, m_sessionId, m_bootId;
    QByteArray m_key;
    std::unique_ptr<control::NodeControlEndpoint> m_endpoint;

    QList<QJsonObject> m_events;      // authoritative session history
    QList<QByteArray>  m_pending;     // telemetry not yet drained
    bool    m_linkUp = true;
    int     m_seq = 0;
    int     m_restarts = 0;
    qint64  m_clockOffsetMs = 0;
    qint64  m_scheduledStartNodeMs = -1;
    bool    m_started = false;
    QString m_athlete;
    QStringList m_applied;
};

} // namespace dev
} // namespace rms
} // namespace ta

#endif // TA_RMS_DEV_CONTROLLEDNODE_H
