#include "rms/control/NodeControlEndpoint.h"

#include "rms/control/ControlAuth.h"

#include <QJsonArray>

namespace ta {
namespace rms {
namespace control {

NodeControlEndpoint::NodeControlEndpoint(Identity id,
                                         QByteArray rangeKey,
                                         IControlCommandHandler* handler)
    : m_id(std::move(id))
    , m_key(std::move(rangeKey))
    , m_handler(handler)
{
}

bool NodeControlEndpoint::hasCapability(const char* c) const
{
    return m_id.capabilities.contains(QLatin1String(c));
}

Ack NodeControlEndpoint::makeAck(const Command& c, bool accepted,
                                 const char* reasonCode, const QString& message,
                                 qint64 nowUtcMs) const
{
    Ack a;
    a.controlProtocolVersion = kControlProtocolVersion;
    a.commandId  = c.commandId;
    a.nodeId     = m_id.nodeId;
    a.accepted   = accepted;
    a.reasonCode = QLatin1String(reasonCode);
    a.message    = message;
    a.nodeTimestampUtcMs = nowUtcMs;
    return a;
}

NodeControlEndpoint::Reaction
NodeControlEndpoint::onBytes(const QByteArray& bytes, qint64 nowUtcMs)
{
    Reaction out;

    QList<QByteArray> frames;
    const FrameReader::Status st = m_reader.append(bytes, &frames);
    if (st == FrameReader::Status::Oversize) {
        // Terminal: the length was the thing we could not trust, so there is no
        // way to find where the next frame starts.
        m_lastError = QStringLiteral("oversize frame declared; connection closed");
        out.closeConnection = true;
        return out;
    }
    if (st == FrameReader::Status::Malformed) {
        m_lastError = QStringLiteral("malformed frame length; connection closed");
        out.closeConnection = true;
        return out;
    }

    for (const QByteArray& payload : frames) {
        const DecodedControl d = decodeControl(payload);

        if (d.type == MessageType::Unknown) {
            // Includes a bad version and malformed JSON. Both are terminal:
            // there is no downgrade path, and a peer that cannot frame valid
            // JSON is not going to start.
            m_lastError = d.rejectReason;
            out.closeConnection = true;
            return out;
        }

        switch (d.type) {
        case MessageType::Hello: {
            m_rmsNonce       = d.hello.rmsNonce;
            m_rmsInstanceId  = d.hello.rmsInstanceId;
            m_nodeNonce      = makeNonce();   // FRESH every connection
            Challenge ch;
            ch.controlProtocolVersion = kControlProtocolVersion;
            ch.nodeId       = m_id.nodeId;
            ch.bootId       = m_id.bootId;
            ch.product      = m_id.product;
            ch.appVersion   = m_id.appVersion;
            ch.commit       = m_id.commit;
            ch.capabilities = m_id.capabilities;
            ch.nodeNonce    = m_nodeNonce;
            out.reply += frame(encode(ch));
            break;
        }

        case MessageType::Auth: {
            AuthResult r;
            r.controlProtocolVersion = kControlProtocolVersion;
            r.nodeId = m_id.nodeId;

            // An unusable key must never mean "no authentication required".
            if (m_key.size() < 32) {
                r.accepted = false;
                r.reasonCode = QLatin1String(reason::kAuthFailed);
                m_lastError = QStringLiteral("no usable range key");
            } else if (m_rmsNonce.isEmpty() || m_nodeNonce.isEmpty()) {
                // AUTH before HELLO: there is no challenge to answer.
                r.accepted = false;
                r.reasonCode = QLatin1String(reason::kAuthFailed);
                m_lastError = QStringLiteral("auth before hello");
            } else {
                const QString expect = computeMac(m_key, m_rmsNonce, m_nodeNonce,
                                                  m_id.nodeId, m_rmsInstanceId);
                const bool ok = macEquals(expect, d.auth.mac);
                r.accepted = ok;
                r.reasonCode = QLatin1String(ok ? reason::kOk : reason::kAuthFailed);
                m_authenticated = ok;
                if (!ok)
                    m_lastError = QStringLiteral("authentication failed");
            }
            out.reply += frame(encode(r));
            if (!r.accepted)
                out.closeConnection = true;   // no retry loop on one connection
            break;
        }

        case MessageType::Command: {
            if (!m_authenticated) {
                // The whole point: no state-changing command before auth.
                Ack a = makeAck(d.command, false, reason::kNotAuthenticated,
                                QStringLiteral("control channel is not authenticated"),
                                nowUtcMs);
                out.reply += frame(encode(a));
                out.closeConnection = true;
                break;
            }
            out.reply += frame(encode(handleCommand(d.command, nowUtcMs)));
            break;
        }

        // A node never receives these.
        case MessageType::Challenge:
        case MessageType::AuthResult:
        case MessageType::Ack:
        case MessageType::ReplayBatch:
        case MessageType::Unknown:
            m_lastError = QStringLiteral("unexpected message type at the node");
            out.closeConnection = true;
            return out;
        }
    }
    return out;
}

Ack NodeControlEndpoint::handleCommand(const Command& c, qint64 nowUtcMs)
{
    // Addressed to someone else. Identity is checked even though RMS dialled
    // us: the address is not the identity, and a misrouted command must not be
    // executed just because it arrived.
    if (!c.nodeId.isEmpty() && c.nodeId != m_id.nodeId)
        return makeAck(c, false, reason::kBadNode,
                       QStringLiteral("command addressed to another node"), nowUtcMs);

    // IDEMPOTENCY, before anything is applied. A repeated commandId returns the
    // ORIGINAL outcome and does not act again - which is what stops a retried
    // FEED_PAPER feeding twice.
    auto it = m_handled.constFind(c.commandId);
    if (it != m_handled.constEnd()) {
        ++m_duplicates;
        Ack a = it.value();
        a.duplicate = true;
        a.nodeTimestampUtcMs = nowUtcMs;
        return a;
    }

    // Stale commands are refused rather than applied late. A START_AT captured
    // and replayed an hour later must not start a match.
    if (c.issuedAtUtcMs > 0 && nowUtcMs > 0
        && (nowUtcMs - c.issuedAtUtcMs) > kCommandWindowMs) {
        return makeAck(c, false, reason::kStaleCommand,
                       QStringLiteral("command is older than the acceptance window"),
                       nowUtcMs);
    }

    const QString type = c.commandType;
    Ack ack;

    // Capability gate. A command the node did not advertise is REFUSED, not
    // attempted - availability is capability-driven, never inferred from the
    // product name.
    struct Gate { const char* cmd; const char* cap; };
    static const Gate kGates[] = {
        { cmd::kRequestReplay,  cap::kEventReplay },
        { cmd::kAssignAthlete,  cap::kAthleteAssignment },
        { cmd::kPrepareSession, cap::kSessionPrepare },
        { cmd::kStartAt,        cap::kStartAt },
        { cmd::kStop,           cap::kStop },
        { cmd::kFeedPaper,      cap::kPaperFeed },
    };
    for (const Gate& g : kGates) {
        if (type == QLatin1String(g.cmd) && !hasCapability(g.cap)) {
            ack = makeAck(c, false, reason::kUnsupportedCapability,
                          QStringLiteral("node does not advertise %1")
                              .arg(QLatin1String(g.cap)), nowUtcMs);
            // Still recorded, so a retry of a refused command is also a no-op
            // and gets the same answer.
            m_handled.insert(c.commandId, ack);
            m_handledOrder.enqueue(c.commandId);
            return ack;
        }
    }

    if (type == QLatin1String(cmd::kPing)) {
        ack = makeAck(c, true, reason::kOk, QString(), nowUtcMs);
    } else if (!m_handler) {
        ack = makeAck(c, false, reason::kPreconditionFailed,
                      QStringLiteral("no command handler is attached"), nowUtcMs);
    } else if (type == QLatin1String(cmd::kRequestStatus)
               || type == QLatin1String(cmd::kRequestReplay)
               || type == QLatin1String(cmd::kAssignAthlete)
               || type == QLatin1String(cmd::kPrepareSession)
               || type == QLatin1String(cmd::kStartAt)
               || type == QLatin1String(cmd::kStop)
               || type == QLatin1String(cmd::kFeedPaper)) {
        const IControlCommandHandler::Result r = m_handler->apply(c);
        ack = makeAck(c, r.accepted,
                      r.reasonCode.isEmpty()
                          ? (r.accepted ? reason::kOk : reason::kPreconditionFailed)
                          : reason::kOk,
                      r.message, nowUtcMs);
        if (!r.reasonCode.isEmpty())
            ack.reasonCode = r.reasonCode;
        ack.resultingState = r.resultingState;
        if (r.accepted) ++m_applied;
    } else {
        ack = makeAck(c, false, reason::kUnknownCommand,
                      QStringLiteral("unknown command type"), nowUtcMs);
    }

    m_handled.insert(c.commandId, ack);
    m_handledOrder.enqueue(c.commandId);
    while (m_handledOrder.size() > kCommandHistory)
        m_handled.remove(m_handledOrder.dequeue());

    return ack;
}

} // namespace control
} // namespace rms
} // namespace ta
