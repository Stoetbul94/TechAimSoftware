#include "rms/control/RmsControlClient.h"

#include "rms/control/ControlAuth.h"

namespace ta {
namespace rms {
namespace control {

RmsControlClient::RmsControlClient(QString rmsInstanceId, QByteArray rangeKey)
    : m_instanceId(std::move(rmsInstanceId))
    , m_key(std::move(rangeKey))
{
}

QByteArray RmsControlClient::start()
{
    m_rmsNonce = makeNonce();          // fresh per connection
    m_state = State::Connecting;
    Hello h;
    h.controlProtocolVersion = kControlProtocolVersion;
    h.rmsInstanceId = m_instanceId;
    h.rmsNonce      = m_rmsNonce;
    return frame(encode(h));
}

RmsControlClient::Reaction RmsControlClient::onBytes(const QByteArray& bytes)
{
    Reaction out;

    QList<QByteArray> frames;
    const FrameReader::Status st = m_reader.append(bytes, &frames);
    if (st != FrameReader::Status::Ok) {
        m_lastError = (st == FrameReader::Status::Oversize)
                          ? QStringLiteral("node declared an oversize frame")
                          : QStringLiteral("node sent a malformed frame");
        m_state = State::Error;
        out.closeConnection = true;
        return out;
    }

    for (const QByteArray& payload : frames) {
        const DecodedControl d = decodeControl(payload);
        if (d.type == MessageType::Unknown) {
            m_lastError = d.rejectReason;      // includes a version mismatch
            m_state = State::Error;
            out.closeConnection = true;
            return out;
        }

        switch (d.type) {
        case MessageType::Challenge: {
            // IDENTITY IS VERIFIED HERE, not assumed from the address we
            // dialled. A node answering with a different nodeId is not the node
            // this connection was for.
            if (!m_expectedNodeId.isEmpty() && d.challenge.nodeId != m_expectedNodeId) {
                m_lastError = QStringLiteral("expected node %1 but %2 answered")
                                  .arg(m_expectedNodeId, d.challenge.nodeId);
                m_state = State::Error;
                out.closeConnection = true;
                return out;
            }
            m_nodeId       = d.challenge.nodeId;
            m_bootId       = d.challenge.bootId;
            m_capabilities = d.challenge.capabilities;

            Auth a;
            a.controlProtocolVersion = kControlProtocolVersion;
            a.mac = computeMac(m_key, m_rmsNonce, d.challenge.nodeNonce,
                               d.challenge.nodeId, m_instanceId);
            out.reply += frame(encode(a));
            m_state = State::Authenticating;
            break;
        }

        case MessageType::AuthResult:
            if (d.authResult.accepted) {
                m_state = State::Authenticated;
                out.becameAuthenticated = true;
            } else {
                m_lastError = QStringLiteral("node rejected authentication: %1")
                                  .arg(d.authResult.reasonCode);
                m_state = State::Error;
                out.closeConnection = true;
            }
            break;

        case MessageType::Ack:
            out.ack = d.ack;
            out.gotAck = true;
            break;

        case MessageType::ReplayBatch:
            out.replay = d.replay;
            out.gotReplay = true;
            break;

        // RMS never receives these.
        case MessageType::Hello:
        case MessageType::Auth:
        case MessageType::Command:
        case MessageType::Unknown:
            m_lastError = QStringLiteral("unexpected message type at RMS");
            m_state = State::Error;
            out.closeConnection = true;
            return out;
        }
    }
    return out;
}

} // namespace control
} // namespace rms
} // namespace ta
