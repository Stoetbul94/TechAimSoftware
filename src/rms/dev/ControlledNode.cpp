#include "rms/dev/ControlledNode.h"

#include "rms/RmsProtocol.h"

#include <QJsonDocument>

namespace ta {
namespace rms {
namespace dev {

using namespace ta::rms::control;

namespace {
QStringList fullCapabilities()
{
    // Everything except paperFeed. It moves physical hardware and stays
    // capability-gated off until a node adapter is physically validated - the
    // simulator does not get to pretend otherwise.
    return QStringList{
        QLatin1String(cap::kStatus),            QLatin1String(cap::kEventReplay),
        QLatin1String(cap::kAthleteAssignment), QLatin1String(cap::kSessionPrepare),
        QLatin1String(cap::kStartAt),           QLatin1String(cap::kStop)
    };
}
}

ControlledNode::ControlledNode(const QString& nodeId, const QString& laneId,
                               const QString& sessionId, const QByteArray& rangeKey)
    : m_nodeId(nodeId), m_laneId(laneId), m_sessionId(sessionId)
    , m_bootId(QStringLiteral("boot-%1-a").arg(nodeId))
    , m_key(rangeKey)
{
    NodeControlEndpoint::Identity id;
    id.nodeId = m_nodeId;
    id.bootId = m_bootId;
    id.product = QStringLiteral("Tech Aim");
    id.appVersion = QStringLiteral("1.0.0");
    id.commit = QStringLiteral("simnode");
    id.capabilities = fullCapabilities();
    m_endpoint = std::make_unique<NodeControlEndpoint>(id, m_key, this);
}

QJsonObject ControlledNode::buildShot(int seq, qint64 nowUtcMs) const
{
    AcceptedShot s;
    s.protocolVersion = kProtocolVersion;
    // The identity RMS deduplicates on. Stable for the life of the event, and
    // deliberately NOT regenerated on replay.
    s.eventId   = QStringLiteral("%1-%2-%3").arg(m_nodeId, m_sessionId).arg(seq);
    s.nodeId    = m_nodeId;
    s.bootId    = m_bootId;
    s.laneId    = m_laneId;
    s.sessionId = m_sessionId;
    s.programmeId = QStringLiteral("issf.10m.air-rifle.qualification60");
    s.shotSequence = seq;
    s.rawXMm = 0.4; s.rawYMm = -0.3;
    s.authoritativeScore = 10.2;      // computed by the NODE; never re-scored
    s.integerScore = 10;
    s.timestampUtcMs = nowUtcMs;
    s.acquisitionStatus = QStringLiteral("ACCEPTED");
    return QJsonDocument::fromJson(encode(s)).object();
}

void ControlledNode::fire(int count, qint64 nowUtcMs)
{
    for (int i = 0; i < count; ++i) {
        const QJsonObject e = buildShot(++m_seq, nowUtcMs + i);
        // The event enters the node's own history UNCONDITIONALLY. Whether the
        // network is up has nothing to do with whether the athlete shot.
        m_events.append(e);
        if (m_linkUp)
            m_pending.append(QJsonDocument(e).toJson(QJsonDocument::Compact));
    }
}

QList<QByteArray> ControlledNode::drainTelemetry()
{
    QList<QByteArray> out;
    out.swap(m_pending);
    return out;
}

void ControlledNode::restart()
{
    ++m_restarts;
    // Same node, same session, NEW boot. This is what lets RMS tell "the
    // application restarted" from "the network blinked".
    m_bootId = QStringLiteral("boot-%1-%2").arg(m_nodeId).arg(m_restarts + 1);

    NodeControlEndpoint::Identity id;
    id.nodeId = m_nodeId;
    id.bootId = m_bootId;
    id.product = QStringLiteral("Tech Aim");
    id.appVersion = QStringLiteral("1.0.0");
    id.commit = QStringLiteral("simnode");
    id.capabilities = fullCapabilities();

    // A NEW endpoint, on purpose. A restarted process has no memory of the old
    // connection - and none of the command ids it had handled. That is a real
    // property of the system and the tests assert it rather than papering over
    // it: duplicate protection is per process unless a command is persisted,
    // and the qualification document says so.
    m_endpoint = std::make_unique<NodeControlEndpoint>(id, m_key, this);

    // The session and its events SURVIVE - the node recovered them from its own
    // store. Only the boot identity and the connection are new.
    m_scheduledStartNodeMs = -1;
}

IControlCommandHandler::Result ControlledNode::apply(const Command& c)
{
    Result r;
    const QString t = c.commandType;
    m_applied << t;

    if (t == QLatin1String(cmd::kRequestStatus)) {
        r.accepted = true;
        r.reasonCode = QLatin1String(reason::kOk);
        r.resultingState = QJsonObject{
            {"sessionId", m_sessionId}, {"bootId", m_bootId},
            {"highestSequence", m_seq}, {"started", m_started}};
        return r;
    }
    if (t == QLatin1String(cmd::kAssignAthlete)) {
        m_athlete = c.payload.value(QStringLiteral("athlete")).toString();
        r.accepted = true; r.reasonCode = QLatin1String(reason::kOk);
        r.resultingState = QJsonObject{{"athlete", m_athlete}};
        return r;
    }
    if (t == QLatin1String(cmd::kPrepareSession)) {
        r.accepted = true; r.reasonCode = QLatin1String(reason::kOk);
        r.resultingState = QJsonObject{{"sessionId", m_sessionId}, {"phase", "PREPARED"}};
        return r;
    }
    if (t == QLatin1String(cmd::kStartAt)) {
        // ALREADY STARTED is refused, not silently re-started. A second
        // START_AT with a different commandId must never reset a running
        // competition clock.
        if (m_started || m_scheduledStartNodeMs >= 0) {
            r.accepted = false;
            r.reasonCode = QLatin1String(reason::kPreconditionFailed);
            r.message = QStringLiteral("a start is already scheduled or running");
            return r;
        }
        const qint64 startAtRms = qint64(c.payload.value(QStringLiteral("startAtUtcMs")).toDouble());
        const qint64 offset     = qint64(c.payload.value(QStringLiteral("rmsToNodeOffsetMs")).toDouble());
        // The node converts the RMS instant into ITS OWN clock using the
        // measured offset, then schedules. It does not start on arrival, which
        // is what would make every lane begin at its own delivery jitter.
        m_scheduledStartNodeMs = startAtRms + offset;
        r.accepted = true; r.reasonCode = QLatin1String(reason::kOk);
        r.resultingState = QJsonObject{{"scheduledStartNodeMs", double(m_scheduledStartNodeMs)}};
        return r;
    }
    if (t == QLatin1String(cmd::kStop)) {
        m_started = false;
        m_scheduledStartNodeMs = -1;
        r.accepted = true; r.reasonCode = QLatin1String(reason::kOk);
        r.resultingState = QJsonObject{{"phase", "STOPPED"}};
        return r;
    }

    r.accepted = false;
    r.reasonCode = QLatin1String(reason::kUnknownCommand);
    return r;
}

QList<QJsonObject> ControlledNode::replayEvents(const QString& sessionId,
                                                int afterSequence, int maxEvents,
                                                bool* hasMoreOut)
{
    QList<QJsonObject> out;
    if (hasMoreOut) *hasMoreOut = false;
    // A request for another session returns nothing rather than this one's
    // history: answering the wrong session would put one lane's shots on
    // another lane's ledger.
    if (!sessionId.isEmpty() && sessionId != m_sessionId)
        return out;

    for (const QJsonObject& e : m_events) {
        if (e.value(QStringLiteral("shotSequence")).toInt() <= afterSequence)
            continue;
        if (out.size() >= maxEvents) {
            if (hasMoreOut) *hasMoreOut = true;
            return out;
        }
        out.append(e);      // the ORIGINAL object; no id is minted here
    }
    return out;
}

} // namespace dev
} // namespace rms
} // namespace ta
