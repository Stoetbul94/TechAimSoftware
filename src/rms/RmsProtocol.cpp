#include "RmsProtocol.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace ta {
namespace rms {

namespace {

// Wire tokens. These are protocol constants, not display strings: they are
// never translated and never derived from a UI label (QML-LANG-001).
const char* kTypeAnnounce = "node.announce";
const char* kTypeStatus   = "node.status";
const char* kTypeShot     = "shot.accepted";

} // namespace

QString toString(ConnectionState s)
{
    switch (s) {
    case ConnectionState::Online:              return QStringLiteral("ONLINE");
    case ConnectionState::TargetConnected:     return QStringLiteral("TARGET_CONNECTED");
    case ConnectionState::TargetDisconnected:  return QStringLiteral("TARGET_DISCONNECTED");
    case ConnectionState::Reconnecting:        return QStringLiteral("RECONNECTING");
    case ConnectionState::Offline:             return QStringLiteral("OFFLINE");
    case ConnectionState::Unknown:             break;
    }
    return QStringLiteral("UNKNOWN");
}

QString toString(MatchPhase p)
{
    switch (p) {
    case MatchPhase::Idle:             return QStringLiteral("IDLE");
    case MatchPhase::Preparation:      return QStringLiteral("PREPARATION");
    case MatchPhase::Sighting:         return QStringLiteral("SIGHTING");
    case MatchPhase::Match:            return QStringLiteral("MATCH");
    case MatchPhase::PositionChange:   return QStringLiteral("POSITION_CHANGE");
    case MatchPhase::Complete:         return QStringLiteral("COMPLETE");
    case MatchPhase::RecoveryRequired: return QStringLiteral("RECOVERY_REQUIRED");
    case MatchPhase::Unknown:          break;
    }
    return QStringLiteral("UNKNOWN");
}

ConnectionState connectionStateFromString(const QString& s)
{
    if (s == QLatin1String("ONLINE"))              return ConnectionState::Online;
    if (s == QLatin1String("TARGET_CONNECTED"))    return ConnectionState::TargetConnected;
    if (s == QLatin1String("TARGET_DISCONNECTED")) return ConnectionState::TargetDisconnected;
    if (s == QLatin1String("RECONNECTING"))        return ConnectionState::Reconnecting;
    // OFFLINE is deliberately NOT decodable. It is an RMS conclusion drawn
    // from heartbeat silence; a node claiming to be offline would be a
    // contradiction, and accepting it would let one stale datagram park a
    // live lane in the wrong state.
    return ConnectionState::Unknown;
}

MatchPhase matchPhaseFromString(const QString& s)
{
    if (s == QLatin1String("IDLE"))              return MatchPhase::Idle;
    if (s == QLatin1String("PREPARATION"))       return MatchPhase::Preparation;
    if (s == QLatin1String("SIGHTING"))          return MatchPhase::Sighting;
    if (s == QLatin1String("MATCH"))             return MatchPhase::Match;
    if (s == QLatin1String("POSITION_CHANGE"))   return MatchPhase::PositionChange;
    if (s == QLatin1String("COMPLETE"))          return MatchPhase::Complete;
    if (s == QLatin1String("RECOVERY_REQUIRED")) return MatchPhase::RecoveryRequired;
    return MatchPhase::Unknown;
}

// ── decode ───────────────────────────────────────────────────────────────
//
// Rejection is explicit and total. A datagram is either fully understood or
// discarded with a reason; there is no partial ingest, because a half-read
// status message would put a wrong phase on a real lane.

DecodedMessage decode(const QByteArray& datagram)
{
    DecodedMessage out;

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(datagram, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        out.rejectReason = QStringLiteral("malformed JSON");
        return out;
    }
    const QJsonObject o = doc.object();

    if (!o.contains(QStringLiteral("protocolVersion"))) {
        out.rejectReason = QStringLiteral("missing protocolVersion");
        return out;
    }
    const int version = o.value(QStringLiteral("protocolVersion")).toInt(-1);
    if (version != kProtocolVersion) {
        out.rejectReason = QStringLiteral("unsupported protocolVersion %1").arg(version);
        return out;
    }

    const QString type = o.value(QStringLiteral("type")).toString();
    const QString nodeId = o.value(QStringLiteral("nodeId")).toString();
    if (nodeId.isEmpty()) {
        out.rejectReason = QStringLiteral("missing nodeId");
        return out;
    }

    if (type == QLatin1String(kTypeAnnounce)) {
        NodeAnnounce m;
        m.protocolVersion = version;
        m.nodeId          = nodeId;
        m.bootId          = o.value(QStringLiteral("bootId")).toString();
        m.laneId          = o.value(QStringLiteral("laneId")).toString();
        m.deviceIdentity  = o.value(QStringLiteral("deviceIdentity")).toString();
        m.appVersion      = o.value(QStringLiteral("appVersion")).toString();
        m.productIdentity = o.value(QStringLiteral("productIdentity")).toString();
        m.timestampUtcMs  = qint64(o.value(QStringLiteral("timestampUtcMs")).toDouble());
        if (m.bootId.isEmpty()) {
            out.rejectReason = QStringLiteral("announce without bootId");
            return out;
        }
        out.type = MessageType::NodeAnnounce;
        out.announce = m;
        return out;
    }

    if (type == QLatin1String(kTypeStatus)) {
        NodeStatus m;
        m.protocolVersion  = version;
        m.nodeId           = nodeId;
        m.bootId           = o.value(QStringLiteral("bootId")).toString();
        m.laneId           = o.value(QStringLiteral("laneId")).toString();
        m.sessionId        = o.value(QStringLiteral("sessionId")).toString();
        m.programmeId      = o.value(QStringLiteral("programmeId")).toString();
        m.rulesetId        = o.value(QStringLiteral("rulesetId")).toString();
        m.targetStandardId = o.value(QStringLiteral("targetStandardId")).toString();
        m.athleteName      = o.value(QStringLiteral("athleteName")).toString();
        m.position         = o.value(QStringLiteral("position")).toString();
        m.connection       = connectionStateFromString(
                                 o.value(QStringLiteral("connection")).toString());
        m.phase            = matchPhaseFromString(
                                 o.value(QStringLiteral("phase")).toString());
        m.shotsAccepted    = o.value(QStringLiteral("shotsAccepted")).toInt(0);
        m.shotsExpected    = o.value(QStringLiteral("shotsExpected")).toInt(-1);
        m.totalScore       = o.value(QStringLiteral("totalScore")).toDouble(0.0);
        m.health           = o.value(QStringLiteral("health")).toString();
        m.statusSeq        = quint64(o.value(QStringLiteral("statusSeq")).toDouble());
        m.timestampUtcMs   = qint64(o.value(QStringLiteral("timestampUtcMs")).toDouble());
        if (m.bootId.isEmpty()) {
            out.rejectReason = QStringLiteral("status without bootId");
            return out;
        }
        out.type = MessageType::NodeStatus;
        out.status = m;
        return out;
    }

    if (type == QLatin1String(kTypeShot)) {
        AcceptedShot m;
        m.protocolVersion     = version;
        m.eventId             = o.value(QStringLiteral("eventId")).toString();
        m.nodeId              = nodeId;
        m.bootId              = o.value(QStringLiteral("bootId")).toString();
        m.laneId              = o.value(QStringLiteral("laneId")).toString();
        m.sessionId           = o.value(QStringLiteral("sessionId")).toString();
        m.programmeId         = o.value(QStringLiteral("programmeId")).toString();
        m.position            = o.value(QStringLiteral("position")).toString();
        m.shotSequence        = o.value(QStringLiteral("shotSequence")).toInt(0);
        m.rawXMm              = o.value(QStringLiteral("rawXMm")).toDouble(0.0);
        m.rawYMm              = o.value(QStringLiteral("rawYMm")).toDouble(0.0);
        m.authoritativeScore  = o.value(QStringLiteral("authoritativeScore")).toDouble(0.0);
        m.integerScore        = o.value(QStringLiteral("integerScore")).toInt(-1);
        m.innerTen            = o.value(QStringLiteral("innerTen")).toBool(false);
        m.timestampUtcMs      = qint64(o.value(QStringLiteral("timestampUtcMs")).toDouble());
        m.acquisitionStatus   = o.value(QStringLiteral("acquisitionStatus")).toString();
        if (m.eventId.isEmpty()) {
            out.rejectReason = QStringLiteral("accepted shot without eventId");
            return out;
        }
        if (m.sessionId.isEmpty()) {
            out.rejectReason = QStringLiteral("accepted shot without sessionId");
            return out;
        }
        if (m.shotSequence <= 0) {
            out.rejectReason = QStringLiteral("accepted shot with non-positive shotSequence");
            return out;
        }
        out.type = MessageType::AcceptedShot;
        out.shot = m;
        return out;
    }

    out.rejectReason = type.isEmpty() ? QStringLiteral("missing type")
                                      : QStringLiteral("unknown type '%1'").arg(type);
    return out;
}

// ── encode ───────────────────────────────────────────────────────────────

QByteArray encode(const NodeAnnounce& m)
{
    QJsonObject o;
    o[QStringLiteral("protocolVersion")] = kProtocolVersion;
    o[QStringLiteral("type")]            = QLatin1String(kTypeAnnounce);
    o[QStringLiteral("nodeId")]          = m.nodeId;
    o[QStringLiteral("bootId")]          = m.bootId;
    o[QStringLiteral("laneId")]          = m.laneId;
    o[QStringLiteral("deviceIdentity")]  = m.deviceIdentity;
    o[QStringLiteral("appVersion")]      = m.appVersion;
    o[QStringLiteral("productIdentity")] = m.productIdentity;
    o[QStringLiteral("timestampUtcMs")]  = double(m.timestampUtcMs);
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

QByteArray encode(const NodeStatus& m)
{
    QJsonObject o;
    o[QStringLiteral("protocolVersion")]  = kProtocolVersion;
    o[QStringLiteral("type")]             = QLatin1String(kTypeStatus);
    o[QStringLiteral("nodeId")]           = m.nodeId;
    o[QStringLiteral("bootId")]           = m.bootId;
    o[QStringLiteral("laneId")]           = m.laneId;
    o[QStringLiteral("sessionId")]        = m.sessionId;
    o[QStringLiteral("programmeId")]      = m.programmeId;
    o[QStringLiteral("rulesetId")]        = m.rulesetId;
    o[QStringLiteral("targetStandardId")] = m.targetStandardId;
    o[QStringLiteral("athleteName")]      = m.athleteName;
    o[QStringLiteral("position")]         = m.position;
    o[QStringLiteral("connection")]       = toString(m.connection);
    o[QStringLiteral("phase")]            = toString(m.phase);
    o[QStringLiteral("shotsAccepted")]    = m.shotsAccepted;
    o[QStringLiteral("shotsExpected")]    = m.shotsExpected;
    o[QStringLiteral("totalScore")]       = m.totalScore;
    o[QStringLiteral("health")]           = m.health;
    o[QStringLiteral("statusSeq")]        = double(m.statusSeq);
    o[QStringLiteral("timestampUtcMs")]   = double(m.timestampUtcMs);
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

QByteArray encode(const AcceptedShot& m)
{
    QJsonObject o;
    o[QStringLiteral("protocolVersion")]    = kProtocolVersion;
    o[QStringLiteral("type")]               = QLatin1String(kTypeShot);
    o[QStringLiteral("eventId")]            = m.eventId;
    o[QStringLiteral("nodeId")]             = m.nodeId;
    o[QStringLiteral("bootId")]             = m.bootId;
    o[QStringLiteral("laneId")]             = m.laneId;
    o[QStringLiteral("sessionId")]          = m.sessionId;
    o[QStringLiteral("programmeId")]        = m.programmeId;
    o[QStringLiteral("position")]           = m.position;
    o[QStringLiteral("shotSequence")]       = m.shotSequence;
    o[QStringLiteral("rawXMm")]             = m.rawXMm;
    o[QStringLiteral("rawYMm")]             = m.rawYMm;
    o[QStringLiteral("authoritativeScore")] = m.authoritativeScore;
    o[QStringLiteral("integerScore")]       = m.integerScore;
    o[QStringLiteral("innerTen")]           = m.innerTen;
    o[QStringLiteral("timestampUtcMs")]     = double(m.timestampUtcMs);
    o[QStringLiteral("acquisitionStatus")]  = m.acquisitionStatus;
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

} // namespace rms
} // namespace ta
