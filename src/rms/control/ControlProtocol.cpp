#include "rms/control/ControlProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

namespace ta {
namespace rms {
namespace control {

namespace cmd {
const char* kPing           = "PING";
const char* kRequestStatus  = "REQUEST_STATUS";
const char* kRequestReplay  = "REQUEST_REPLAY";
const char* kAssignAthlete  = "ASSIGN_ATHLETE";
const char* kPrepareSession = "PREPARE_SESSION";
const char* kStartAt        = "START_AT";
const char* kStop           = "STOP";
const char* kFeedPaper      = "FEED_PAPER";
}

namespace cap {
const char* kStatus            = "status";
const char* kEventReplay       = "eventReplay";
const char* kAthleteAssignment = "athleteAssignment";
const char* kSessionPrepare    = "sessionPrepare";
const char* kStartAt           = "startAt";
const char* kStop              = "stop";
const char* kPaperFeed         = "paperFeed";
}

namespace reason {
const char* kOk                     = "OK";
const char* kUnsupportedCapability  = "UNSUPPORTED_CAPABILITY";
const char* kUnknownCommand         = "UNKNOWN_COMMAND";
const char* kNotAuthenticated       = "NOT_AUTHENTICATED";
const char* kBadVersion             = "BAD_PROTOCOL_VERSION";
const char* kBadNode                = "WRONG_NODE";
const char* kAuthFailed             = "AUTH_FAILED";
const char* kStaleCommand           = "STALE_COMMAND";
const char* kMalformed              = "MALFORMED";
const char* kPreconditionFailed     = "PRECONDITION_FAILED";
}

namespace {
const char* kTypeKey = "messageType";
const char* kVerKey  = "controlProtocolVersion";

QJsonObject base(MessageType t)
{
    QJsonObject o;
    o[QLatin1String(kVerKey)]  = kControlProtocolVersion;
    o[QLatin1String(kTypeKey)] = messageTypeName(t);
    return o;
}

QByteArray pack(const QJsonObject& o)
{
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}
}

QString messageTypeName(MessageType t)
{
    switch (t) {
    case MessageType::Hello:       return QStringLiteral("HELLO");
    case MessageType::Challenge:   return QStringLiteral("CHALLENGE");
    case MessageType::Auth:        return QStringLiteral("AUTH");
    case MessageType::AuthResult:  return QStringLiteral("AUTH_RESULT");
    case MessageType::Command:     return QStringLiteral("COMMAND");
    case MessageType::Ack:         return QStringLiteral("ACK");
    case MessageType::ReplayBatch: return QStringLiteral("REPLAY_BATCH");
    case MessageType::Unknown:     break;
    }
    return QStringLiteral("UNKNOWN");
}

MessageType messageTypeFromName(const QString& s)
{
    if (s == QLatin1String("HELLO"))        return MessageType::Hello;
    if (s == QLatin1String("CHALLENGE"))    return MessageType::Challenge;
    if (s == QLatin1String("AUTH"))         return MessageType::Auth;
    if (s == QLatin1String("AUTH_RESULT"))  return MessageType::AuthResult;
    if (s == QLatin1String("COMMAND"))      return MessageType::Command;
    if (s == QLatin1String("ACK"))          return MessageType::Ack;
    if (s == QLatin1String("REPLAY_BATCH")) return MessageType::ReplayBatch;
    return MessageType::Unknown;
}

// ── framing ──────────────────────────────────────────────────────────────

QByteArray frame(const QByteArray& payload)
{
    const quint32 n = quint32(payload.size());
    QByteArray out;
    out.reserve(4 + payload.size());
    out.append(char((n >> 24) & 0xFF));
    out.append(char((n >> 16) & 0xFF));
    out.append(char((n >> 8)  & 0xFF));
    out.append(char( n        & 0xFF));
    out.append(payload);
    return out;
}

FrameReader::Status FrameReader::append(const QByteArray& bytes,
                                        QList<QByteArray>* framesOut)
{
    m_buffer.append(bytes);

    for (;;) {
        if (m_buffer.size() < 4)
            return Status::Ok;                 // not even a length yet

        const quint32 len =
              (quint32(quint8(m_buffer[0])) << 24)
            | (quint32(quint8(m_buffer[1])) << 16)
            | (quint32(quint8(m_buffer[2])) << 8)
            |  quint32(quint8(m_buffer[3]));

        // THE CHECK THAT MATTERS. The declared length is validated BEFORE it is
        // used for anything - no reserve, no resize, no read of that size. A
        // peer claiming 4 GiB costs us four bytes and a closed connection.
        //
        // The stream cannot be resynchronised afterwards, because the thing we
        // could not trust IS the length: there is no way to know where the next
        // frame would start. So this is terminal for the connection, by design.
        if (len > quint32(kMaxFrameBytes))
            return Status::Oversize;

        // A zero-length frame is not a frame. Accepting it would let a peer
        // drive an infinite loop of empty messages.
        if (len == 0)
            return Status::Malformed;

        if (quint32(m_buffer.size() - 4) < len)
            return Status::Ok;                 // body still arriving

        if (framesOut)
            framesOut->append(m_buffer.mid(4, int(len)));
        m_buffer.remove(0, 4 + int(len));
    }
}

// ── encoders ─────────────────────────────────────────────────────────────

QByteArray encode(const Hello& m)
{
    QJsonObject o = base(MessageType::Hello);
    o["rmsInstanceId"] = m.rmsInstanceId;
    o["rmsNonce"]      = m.rmsNonce;
    return pack(o);
}

QByteArray encode(const Challenge& m)
{
    QJsonObject o = base(MessageType::Challenge);
    o["nodeId"]     = m.nodeId;
    o["bootId"]     = m.bootId;
    o["product"]    = m.product;
    o["appVersion"] = m.appVersion;
    o["commit"]     = m.commit;
    o["nodeNonce"]  = m.nodeNonce;
    QJsonArray caps;
    for (const QString& c : m.capabilities) caps.append(c);
    o["capabilities"] = caps;
    return pack(o);
}

QByteArray encode(const Auth& m)
{
    QJsonObject o = base(MessageType::Auth);
    o["mac"] = m.mac;
    return pack(o);
}

QByteArray encode(const AuthResult& m)
{
    QJsonObject o = base(MessageType::AuthResult);
    o["accepted"]   = m.accepted;
    o["reasonCode"] = m.reasonCode;
    o["nodeId"]     = m.nodeId;
    return pack(o);
}

QByteArray encode(const Command& m)
{
    QJsonObject o = base(MessageType::Command);
    o["commandId"]     = m.commandId;
    o["nodeId"]        = m.nodeId;
    o["laneId"]        = m.laneId;
    o["sessionId"]     = m.sessionId;
    o["commandType"]   = m.commandType;
    o["issuedAtUtcMs"] = m.issuedAtUtcMs;
    o["payload"]       = m.payload;
    return pack(o);
}

QByteArray encode(const Ack& m)
{
    QJsonObject o = base(MessageType::Ack);
    o["commandId"]          = m.commandId;
    o["nodeId"]             = m.nodeId;
    o["accepted"]           = m.accepted;
    o["reasonCode"]         = m.reasonCode;
    o["message"]            = m.message;
    o["resultingState"]     = m.resultingState;
    o["nodeTimestampUtcMs"] = m.nodeTimestampUtcMs;
    o["duplicate"]          = m.duplicate;
    return pack(o);
}

QByteArray encode(const ReplayBatch& m)
{
    QJsonObject o = base(MessageType::ReplayBatch);
    o["requestId"]    = m.requestId;
    o["sessionId"]    = m.sessionId;
    o["fromSequence"] = m.fromSequence;
    o["hasMore"]      = m.hasMore;
    o["nextSequence"] = m.nextSequence;
    QJsonArray evts;
    for (const QJsonObject& e : m.events) evts.append(e);
    o["events"] = evts;
    return pack(o);
}

// ── decoder ──────────────────────────────────────────────────────────────

DecodedControl decodeControl(const QByteArray& payload)
{
    DecodedControl d;

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        d.rejectReason = QStringLiteral("malformed JSON: %1").arg(err.errorString());
        return d;
    }
    const QJsonObject o = doc.object();

    // Version first, and it is MANDATORY. A frame without one is not an older
    // dialect to be guessed at - it is not this protocol.
    if (!o.contains(QLatin1String(kVerKey))) {
        d.rejectReason = QStringLiteral("missing controlProtocolVersion");
        return d;
    }
    const int ver = o.value(QLatin1String(kVerKey)).toInt(-1);
    if (ver != kControlProtocolVersion) {
        d.rejectReason =
            QStringLiteral("unsupported controlProtocolVersion %1").arg(ver);
        return d;
    }

    const MessageType t = messageTypeFromName(o.value(QLatin1String(kTypeKey)).toString());
    if (t == MessageType::Unknown) {
        d.rejectReason = QStringLiteral("unknown messageType");
        return d;
    }

    switch (t) {
    case MessageType::Hello:
        d.hello.controlProtocolVersion = ver;
        d.hello.rmsInstanceId = o.value("rmsInstanceId").toString();
        d.hello.rmsNonce      = o.value("rmsNonce").toString();
        if (d.hello.rmsNonce.isEmpty()) {
            d.rejectReason = QStringLiteral("hello without a nonce");
            return d;
        }
        break;
    case MessageType::Challenge: {
        d.challenge.controlProtocolVersion = ver;
        d.challenge.nodeId     = o.value("nodeId").toString();
        d.challenge.bootId     = o.value("bootId").toString();
        d.challenge.product    = o.value("product").toString();
        d.challenge.appVersion = o.value("appVersion").toString();
        d.challenge.commit     = o.value("commit").toString();
        d.challenge.nodeNonce  = o.value("nodeNonce").toString();
        const QJsonArray caps = o.value("capabilities").toArray();
        for (const QJsonValue& c : caps) d.challenge.capabilities << c.toString();
        if (d.challenge.nodeId.isEmpty() || d.challenge.nodeNonce.isEmpty()) {
            d.rejectReason = QStringLiteral("challenge without identity or nonce");
            return d;
        }
        break;
    }
    case MessageType::Auth:
        d.auth.controlProtocolVersion = ver;
        d.auth.mac = o.value("mac").toString();
        if (d.auth.mac.isEmpty()) {
            d.rejectReason = QStringLiteral("auth without a mac");
            return d;
        }
        break;
    case MessageType::AuthResult:
        d.authResult.controlProtocolVersion = ver;
        d.authResult.accepted   = o.value("accepted").toBool(false);
        d.authResult.reasonCode = o.value("reasonCode").toString();
        d.authResult.nodeId     = o.value("nodeId").toString();
        break;
    case MessageType::Command:
        d.command.controlProtocolVersion = ver;
        d.command.commandId     = o.value("commandId").toString();
        d.command.nodeId        = o.value("nodeId").toString();
        d.command.laneId        = o.value("laneId").toString();
        d.command.sessionId     = o.value("sessionId").toString();
        d.command.commandType   = o.value("commandType").toString();
        d.command.issuedAtUtcMs = qint64(o.value("issuedAtUtcMs").toDouble());
        d.command.payload       = o.value("payload").toObject();
        // Without a commandId there is no idempotency, so there is no command.
        if (d.command.commandId.isEmpty() || d.command.commandType.isEmpty()) {
            d.rejectReason = QStringLiteral("command without an id or a type");
            return d;
        }
        break;
    case MessageType::Ack:
        d.ack.controlProtocolVersion = ver;
        d.ack.commandId          = o.value("commandId").toString();
        d.ack.nodeId             = o.value("nodeId").toString();
        d.ack.accepted           = o.value("accepted").toBool(false);
        d.ack.reasonCode         = o.value("reasonCode").toString();
        d.ack.message            = o.value("message").toString();
        d.ack.resultingState     = o.value("resultingState").toObject();
        d.ack.nodeTimestampUtcMs = qint64(o.value("nodeTimestampUtcMs").toDouble());
        d.ack.duplicate          = o.value("duplicate").toBool(false);
        break;
    case MessageType::ReplayBatch: {
        d.replay.controlProtocolVersion = ver;
        d.replay.requestId    = o.value("requestId").toString();
        d.replay.sessionId    = o.value("sessionId").toString();
        d.replay.fromSequence = o.value("fromSequence").toInt();
        d.replay.hasMore      = o.value("hasMore").toBool(false);
        d.replay.nextSequence = o.value("nextSequence").toInt();
        const QJsonArray evts = o.value("events").toArray();
        if (evts.size() > kMaxReplayEvents) {
            d.rejectReason = QStringLiteral("replay batch of %1 exceeds the %2 cap")
                                 .arg(evts.size()).arg(kMaxReplayEvents);
            return d;
        }
        for (const QJsonValue& e : evts) d.replay.events.append(e.toObject());
        break;
    }
    case MessageType::Unknown:
        break;
    }

    d.type = t;
    return d;
}

} // namespace control
} // namespace rms
} // namespace ta
