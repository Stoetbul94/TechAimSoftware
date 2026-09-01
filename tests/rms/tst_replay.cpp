// RMS replay / catch-up (RMS-REPLAY-001).
//
// Closes the gap R1 named: RMS could DETECT missed shots through
// unobservedShotCount() but had no way to fetch them.
//
// The decisive design choice under test is that replayed events are the SAME
// shape as live telemetry and go through the SAME ingest. That is what makes
// replay idempotent for free - it inherits the deduplication already proven at
// fifty lanes - and it is why the events must keep their ORIGINAL eventId. A
// re-minted id would make every catch-up insert duplicates, which is the exact
// failure replay exists to prevent.

#include "test_support.h"

#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "rms/control/ControlProtocol.h"
#include "rms/control/NodeControlEndpoint.h"
#include "rms/control/RmsControlClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <cstdio>

using namespace ta::rms::control;
using ta::rms::RangeMonitor;

namespace {

QByteArray key32() { return QByteArray(32, '\x5a'); }

const char* kNode    = "TA-NODE-001";
const char* kSession = "sess-1";

// Builds an authoritative accepted-shot exactly as the node broadcasts it, so
// live and replayed events are indistinguishable to RMS by construction.
QJsonObject shotEvent(int seq)
{
    ta::rms::AcceptedShot s;
    s.protocolVersion = ta::rms::kProtocolVersion;
    s.eventId   = QStringLiteral("evt-%1").arg(seq);   // STABLE identity
    s.nodeId    = QLatin1String(kNode);
    s.bootId    = QStringLiteral("boot-a");
    s.laneId    = QStringLiteral("Lane 1");
    s.sessionId = QLatin1String(kSession);
    s.programmeId = QStringLiteral("issf.10m.air-rifle.qualification60");
    s.shotSequence = seq;
    s.rawXMm = 0.5; s.rawYMm = -0.5;
    s.authoritativeScore = 10.4;
    s.integerScore = 10;
    s.timestampUtcMs = 1000 + seq;
    s.acquisitionStatus = QStringLiteral("ACCEPTED");
    return QJsonDocument::fromJson(ta::rms::encode(s)).object();
}

class SessionHandler : public IControlCommandHandler
{
public:
    QList<QJsonObject> events;      // the node's persisted session

    Result apply(const Command&) override
    { Result r; r.accepted = true; r.reasonCode = QLatin1String(reason::kOk); return r; }

    QList<QJsonObject> replayEvents(const QString&, int afterSequence,
                                    int maxEvents, bool* hasMoreOut) override
    {
        QList<QJsonObject> out;
        for (const QJsonObject& e : events) {
            if (e.value("shotSequence").toInt() <= afterSequence) continue;
            if (out.size() >= maxEvents) { if (hasMoreOut) *hasMoreOut = true; return out; }
            out.append(e);
        }
        if (hasMoreOut) *hasMoreOut = false;
        return out;
    }
};

NodeControlEndpoint::Identity ident()
{
    NodeControlEndpoint::Identity n;
    n.nodeId = QLatin1String(kNode);
    n.bootId = QStringLiteral("boot-a");
    n.product = QStringLiteral("Tech Aim");
    n.appVersion = QStringLiteral("1.0.0");
    n.commit = QStringLiteral("abc1234");
    n.capabilities = QStringList{ QLatin1String(cap::kEventReplay) };
    return n;
}

// Node + RMS + the real monitor, wired the way production will be.
struct Rig {
    SessionHandler      handler;
    NodeControlEndpoint node;
    RmsControlClient    rms;
    RangeMonitor        monitor;
    qint64              clock = 1;

    Rig() : node(ident(), key32(), &handler), rms(QStringLiteral("RMS-1"), key32())
    {
        rms.setExpectedNode(QLatin1String(kNode));
        QByteArray toNode = rms.start();
        for (int i = 0; i < 4 && !toNode.isEmpty(); ++i) {
            auto nr = node.onBytes(toNode, 1000);
            auto rr = rms.onBytes(nr.reply);
            toNode = rr.reply;
        }
    }

    // Live telemetry, the ordinary path.
    void deliverLive(int seq)
    {
        monitor.ingestDatagram(QJsonDocument(shotEvent(seq)).toJson(QJsonDocument::Compact),
                               clock += 10);
    }

    // A replay request, end to end: RMS asks, the node answers with a batch,
    // and RMS feeds every event through the SAME ingest live telemetry uses.
    int requestReplay(int afterSequence, const QString& commandId)
    {
        Command c;
        c.controlProtocolVersion = kControlProtocolVersion;
        c.commandId   = commandId;
        c.nodeId      = QLatin1String(kNode);
        c.commandType = QLatin1String(cmd::kRequestReplay);
        c.issuedAtUtcMs = 1000;
        c.payload = QJsonObject{{"sessionId", QLatin1String(kSession)},
                                {"afterSequence", afterSequence}};
        auto nr = node.onBytes(rms.sendCommand(c), 1000);
        auto rr = rms.onBytes(nr.reply);
        if (!rr.gotReplay) return -1;
        for (const QJsonObject& e : rr.replay.events)
            monitor.ingestDatagram(QJsonDocument(e).toJson(QJsonDocument::Compact),
                                   clock += 10);
        return rr.replay.events.size();
    }

    int observed() const
    {
        const auto* n = monitor.nodeAt(0);
        return n ? n->ledger.observedCount() : -1;
    }
    int suppressed() const
    {
        const auto* n = monitor.nodeAt(0);
        return n ? n->ledger.duplicatesSuppressed() : -1;
    }
};

} // namespace

void run_replay_tests()
{
    printf("\n-- RMS replay / catch-up (RMS-REPLAY-001) --\n");
    fflush(stdout);

    // ── §25, exactly as written ───────────────────────────────────────────
    {
        Rig rig;
        check(rig.rms.state() == RmsControlClient::State::Authenticated,
              "RMS-REPLAY-001: the control channel authenticated");

        for (int s = 1; s <= 25; ++s) rig.handler.events.append(shotEvent(s));

        // RMS sees 1-20 live and MISSES 21-25 - the network drop.
        for (int s = 1; s <= 20; ++s) rig.deliverLive(s);
        check(rig.observed() == 20,
              "RMS-REPLAY-001: RMS holds the twenty it saw live",
              QString::number(rig.observed()));

        const int got = rig.requestReplay(20, QStringLiteral("rep-1"));
        check(got == 5,
              "RMS-REPLAY-001: requesting afterSequence 20 returns exactly the "
              "five that were missed", QString::number(got));
        check(rig.observed() == 25,
              "RMS-REPLAY-001: THE LEDGER IS NOW 25 - the missed shots were "
              "recovered", QString::number(rig.observed()));

        // The same request again.
        const int before = rig.suppressed();
        rig.requestReplay(20, QStringLiteral("rep-2"));
        check(rig.observed() == 25,
              "RMS-REPLAY-001: the identical replay again leaves the ledger at 25",
              QString::number(rig.observed()));
        check(rig.suppressed() > before,
              "RMS-REPLAY-001: and duplicatesSuppressed ROSE - they were seen and "
              "rejected, not simply never sent",
              QString("%1 -> %2").arg(before).arg(rig.suppressed()));

        // The whole session from the beginning.
        const int all = rig.requestReplay(0, QStringLiteral("rep-3"));
        check(all == 25,
              "RMS-REPLAY-001: replaying from sequence 0 returns all 25",
              QString::number(all));
        check(rig.observed() == 25,
              "RMS-REPLAY-001: AND THE LEDGER IS STILL 25 - a full re-replay adds "
              "nothing", QString::number(rig.observed()));
    }

    // ── the event identity that makes all of the above work ───────────────
    {
        Rig rig;
        for (int s = 1; s <= 3; ++s) rig.handler.events.append(shotEvent(s));
        Command c;
        c.controlProtocolVersion = kControlProtocolVersion;
        c.commandId = QStringLiteral("rep-id");
        c.nodeId = QLatin1String(kNode);
        c.commandType = QLatin1String(cmd::kRequestReplay);
        c.issuedAtUtcMs = 1000;
        c.payload = QJsonObject{{"sessionId", QLatin1String(kSession)},
                                {"afterSequence", 0}};
        auto nr = rig.node.onBytes(rig.rms.sendCommand(c), 1000);
        auto rr = rig.rms.onBytes(nr.reply);
        check(rr.gotReplay && rr.replay.events.size() == 3,
              "RMS-REPLAY-001: the batch carries the events");
        check(rr.replay.events.at(0).value("eventId").toString()
                  == QStringLiteral("evt-1"),
              "RMS-REPLAY-001: the ORIGINAL eventId is preserved - re-minting it "
              "would make every catch-up insert duplicates",
              rr.replay.events.at(0).value("eventId").toString());
        check(rr.replay.events.at(2).value("shotSequence").toInt() == 3
              && rr.replay.events.at(0).value("authoritativeScore").toDouble() > 0.0,
              "RMS-REPLAY-001: sequence and the node's authoritative score survive "
              "the round trip");
        check(rr.gotAck,
              "RMS-REPLAY-001: and the request is acknowledged as well as answered");
    }

    // ── §24 batching is bounded, and the peer cannot enlarge it ───────────
    {
        Rig rig;
        for (int s = 1; s <= 500; ++s) rig.handler.events.append(shotEvent(s));
        Command c;
        c.controlProtocolVersion = kControlProtocolVersion;
        c.commandId = QStringLiteral("rep-big");
        c.nodeId = QLatin1String(kNode);
        c.commandType = QLatin1String(cmd::kRequestReplay);
        c.issuedAtUtcMs = 1000;
        // Ask for far more than the cap.
        c.payload = QJsonObject{{"sessionId", QLatin1String(kSession)},
                                {"afterSequence", 0}, {"maxEvents", 100000}};
        auto nr = rig.node.onBytes(rig.rms.sendCommand(c), 1000);
        auto rr = rig.rms.onBytes(nr.reply);
        check(rr.gotReplay && rr.replay.events.size() == kMaxReplayEvents,
              "RMS-REPLAY-001: a request for 100000 events is CLAMPED to the cap - "
              "a peer cannot make the node build an unbounded response",
              QString::number(rr.replay.events.size()));
        check(rr.replay.hasMore && rr.replay.nextSequence == kMaxReplayEvents,
              "RMS-REPLAY-001: and hasMore/nextSequence let the rest be fetched",
              QString::number(rr.replay.nextSequence));
    }

    // ── replay is refused without the capability, and without auth ────────
    {
        SessionHandler h;
        NodeControlEndpoint::Identity id = ident();
        id.capabilities.clear();                 // no eventReplay
        NodeControlEndpoint node(id, key32(), &h);
        RmsControlClient rms(QStringLiteral("RMS-1"), key32());
        rms.setExpectedNode(QLatin1String(kNode));
        QByteArray toNode = rms.start();
        for (int i = 0; i < 4 && !toNode.isEmpty(); ++i) {
            auto nr = node.onBytes(toNode, 1000);
            auto rr = rms.onBytes(nr.reply);
            toNode = rr.reply;
        }
        Command c;
        c.controlProtocolVersion = kControlProtocolVersion;
        c.commandId = QStringLiteral("rep-nocap");
        c.nodeId = QLatin1String(kNode);
        c.commandType = QLatin1String(cmd::kRequestReplay);
        c.issuedAtUtcMs = 1000;
        auto nr = node.onBytes(rms.sendCommand(c), 1000);
        auto rr = rms.onBytes(nr.reply);
        check(!rr.gotReplay && rr.gotAck && !rr.ack.accepted,
              "RMS-REPLAY-001: replay is refused when the node does not advertise "
              "eventReplay", rr.ack.reasonCode);
    }
    {
        SessionHandler h;
        NodeControlEndpoint node(ident(), key32(), &h);
        Command c;
        c.controlProtocolVersion = kControlProtocolVersion;
        c.commandId = QStringLiteral("rep-noauth");
        c.nodeId = QLatin1String(kNode);
        c.commandType = QLatin1String(cmd::kRequestReplay);
        c.issuedAtUtcMs = 1000;
        auto nr = node.onBytes(frame(encode(c)), 1000);
        const DecodedControl d = decodeControl(nr.reply.mid(4));
        check(d.type == MessageType::Ack && !d.ack.accepted,
              "RMS-REPLAY-001: an UNAUTHENTICATED replay request returns no "
              "history - session data is not readable without the key");
    }
}
