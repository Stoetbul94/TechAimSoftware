#ifndef TA_RMS_RMSCONTROLCLIENT_H
#define TA_RMS_RMSCONTROLCLIENT_H

// The RMS side of the control channel. Transport-free, for the same reason the
// node side is: the interesting cases are refusals, and refusals should be
// provable without a socket.
//
// RMS DIALS, THE NODE LISTENS. RMS learns a node's current address from its
// UDP telemetry and connects there. The address is CONNECTION METADATA - it is
// never identity, and after the handshake RMS verifies the authenticated
// nodeId is the one it meant to reach. A node that answers with a different
// identity is dropped, not adopted.

#include "rms/control/ControlProtocol.h"

#include <QByteArray>
#include <QString>

namespace ta {
namespace rms {
namespace control {

class RmsControlClient
{
public:
    enum class State {
        Disconnected,
        Connecting,      // hello sent, waiting for the challenge
        Authenticating,  // mac sent, waiting for the result
        Authenticated,
        Error
    };

    RmsControlClient(QString rmsInstanceId, QByteArray rangeKey);

    // The node this connection is FOR. Set before starting; the handshake is
    // rejected if the node answers with anything else.
    void setExpectedNode(const QString& nodeId) { m_expectedNodeId = nodeId; }

    // First bytes to send.
    QByteArray start();

    struct Reaction {
        QByteArray reply;
        bool       closeConnection = false;
        bool       becameAuthenticated = false;
        Ack        ack;                 // valid when gotAck
        bool       gotAck = false;
        ReplayBatch replay;             // valid when gotReplay
        bool       gotReplay = false;
    };
    Reaction onBytes(const QByteArray& bytes);

    // Encodes a command. Caller supplies the commandId so a retry can reuse it -
    // which is what makes the node's idempotency reachable.
    QByteArray sendCommand(const Command& c) const { return frame(encode(c)); }

    State   state() const { return m_state; }
    QString lastError() const { return m_lastError; }
    QStringList nodeCapabilities() const { return m_capabilities; }
    QString    nodeId() const { return m_nodeId; }
    QString    bootId() const { return m_bootId; }

    // A capability the node did not advertise must not be sent at all.
    bool supports(const char* capability) const
    { return m_capabilities.contains(QLatin1String(capability)); }

private:
    QString    m_instanceId;
    QByteArray m_key;
    QString    m_expectedNodeId;

    FrameReader m_reader;
    State   m_state = State::Disconnected;
    QString m_rmsNonce;
    QString m_nodeId, m_bootId, m_lastError;
    QStringList m_capabilities;
};

} // namespace control
} // namespace rms
} // namespace ta

#endif // TA_RMS_RMSCONTROLCLIENT_H
