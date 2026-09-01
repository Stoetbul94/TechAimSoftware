// RMS control plane (RMS-CTRL-001).
//
// Both ends are transport-free, so every case below is exact: no socket, no
// timing, no flake. That matters most for the refusals - a security test that
// only usually fails is not a security test.

#include "test_support.h"

#include "rms/control/ControlAuth.h"
#include "rms/control/ControlProtocol.h"
#include "rms/control/NodeControlEndpoint.h"
#include "rms/control/RmsControlClient.h"

#include <QJsonObject>
#include <cstdio>

using namespace ta::rms::control;

namespace {

// A deterministic key that exists ONLY here. It is not a default and no
// shipped configuration contains it.
QByteArray testKey()   { return QByteArray(32, '\x5a'); }
QByteArray wrongKey()  { return QByteArray(32, '\x11'); }

NodeControlEndpoint::Identity nodeIdentity(const QString& id = QStringLiteral("TA-NODE-001"))
{
    NodeControlEndpoint::Identity n;
    n.nodeId = id;
    n.bootId = QStringLiteral("boot-a");
    n.product = QStringLiteral("Tech Aim");
    n.appVersion = QStringLiteral("1.0.0");
    n.commit = QStringLiteral("abc1234");
    n.capabilities = QStringList{
        QLatin1String(cap::kStatus), QLatin1String(cap::kEventReplay),
        QLatin1String(cap::kAthleteAssignment), QLatin1String(cap::kSessionPrepare),
        QLatin1String(cap::kStartAt), QLatin1String(cap::kStop)
        // paperFeed deliberately NOT advertised
    };
    return n;
}

// Records what was applied, so "applied once" is observed and not inferred.
class RecordingHandler : public IControlCommandHandler
{
public:
    QStringList applied;
    QList<QJsonObject> events;

    Result apply(const Command& c) override
    {
        applied << c.commandType;
        Result r;
        r.accepted = true;
        r.reasonCode = QLatin1String(reason::kOk);
        r.resultingState = QJsonObject{{"phase", "READY"}};
        return r;
    }

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

// Drives a full handshake and leaves both ends authenticated.
struct Pair {
    RecordingHandler handler;
    NodeControlEndpoint node;
    RmsControlClient    rms;

    Pair(const QByteArray& rmsKey, const QByteArray& nodeKey)
        : node(nodeIdentity(), nodeKey, &handler)
        , rms(QStringLiteral("RMS-1"), rmsKey) {}

    bool handshake(qint64 now = 1000)
    {
        rms.setExpectedNode(QStringLiteral("TA-NODE-001"));
        QByteArray toNode = rms.start();
        for (int i = 0; i < 4; ++i) {
            auto nr = node.onBytes(toNode, now);
            if (nr.reply.isEmpty()) break;
            auto rr = rms.onBytes(nr.reply);
            if (rr.closeConnection || nr.closeConnection) break;
            toNode = rr.reply;
            if (toNode.isEmpty()) break;
        }
        return rms.state() == RmsControlClient::State::Authenticated;
    }

    Ack send(const Command& c, qint64 now)
    {
        auto nr = node.onBytes(rms.sendCommand(c), now);
        auto rr = rms.onBytes(nr.reply);
        return rr.ack;
    }
};

Command makeCmd(const char* type, const QString& id, qint64 issuedAt = 1000)
{
    Command c;
    c.controlProtocolVersion = kControlProtocolVersion;
    c.commandId   = id;
    c.nodeId      = QStringLiteral("TA-NODE-001");
    c.commandType = QLatin1String(type);
    c.issuedAtUtcMs = issuedAt;
    return c;
}

} // namespace

void run_control_tests()
{
    printf("\n-- RMS control plane (RMS-CTRL-001) --\n");
    fflush(stdout);

    // ── framing: TCP is a byte stream (§30) ───────────────────────────────
    {
        FrameReader r;
        QList<QByteArray> out;
        const QByteArray a = frame(QByteArray("{\"a\":1}"));
        const QByteArray b = frame(QByteArray("{\"b\":2}"));

        // one frame split across three reads
        r.append(a.left(2), &out);
        check(out.isEmpty(), "RMS-CTRL-001: a partial length yields no frame");
        r.append(a.mid(2, 3), &out);
        check(out.isEmpty(), "RMS-CTRL-001: a partial body yields no frame");
        r.append(a.mid(5), &out);
        check(out.size() == 1, "RMS-CTRL-001: the frame completes across three reads",
              QString::number(out.size()));

        // two frames in ONE read
        out.clear();
        FrameReader r2;
        r2.append(a + b, &out);
        check(out.size() == 2,
              "RMS-CTRL-001: two frames arriving in one read are both delivered",
              QString::number(out.size()));

        // disconnect mid-frame leaves a truncated remainder, not a message
        FrameReader r3; out.clear();
        r3.append(a.left(a.size() - 1), &out);
        check(out.isEmpty() && r3.hasPartialFrame(),
              "RMS-CTRL-001: a disconnect mid-frame leaves a partial, never a "
              "half-message");

        // an oversize DECLARED length is refused without allocating it
        FrameReader r4; out.clear();
        QByteArray huge;
        const quint32 n = quint32(kMaxFrameBytes) + 1;
        huge.append(char((n >> 24) & 0xFF)); huge.append(char((n >> 16) & 0xFF));
        huge.append(char((n >> 8) & 0xFF));  huge.append(char(n & 0xFF));
        check(r4.append(huge, &out) == FrameReader::Status::Oversize,
              "RMS-CTRL-001: an oversize length is refused from the four header "
              "bytes alone - nothing of that size is ever allocated");

        FrameReader r5; out.clear();
        check(r5.append(QByteArray(4, '\0'), &out) == FrameReader::Status::Malformed,
              "RMS-CTRL-001: a zero-length frame is malformed, not an empty message");
    }

    // ── the happy path ────────────────────────────────────────────────────
    {
        Pair p(testKey(), testKey());
        check(p.handshake(), "RMS-CTRL-001: a correct key authenticates");
        check(p.node.authenticated(), "RMS-CTRL-001: and the node agrees");
        check(p.rms.nodeId() == QStringLiteral("TA-NODE-001"),
              "RMS-CTRL-001: RMS learned the node identity from the handshake");
        check(p.rms.supports(cap::kStartAt) && !p.rms.supports(cap::kPaperFeed),
              "RMS-CTRL-001: capabilities are advertised, and paperFeed is NOT");
    }

    // ── §29 the security negatives ────────────────────────────────────────
    {
        Pair p(wrongKey(), testKey());
        check(!p.handshake(),
              "RMS-CTRL-001: a WRONG range key does not authenticate");
        Ack a = p.send(makeCmd(cmd::kStop, QStringLiteral("c1")), 1000);
        check(!a.accepted && a.reasonCode == QLatin1String(reason::kNotAuthenticated),
              "RMS-CTRL-001: and a command on that channel is refused as "
              "NOT_AUTHENTICATED", a.reasonCode);
        check(p.handler.applied.isEmpty(),
              "RMS-CTRL-001: the handler was never reached - nothing was applied");
    }
    {
        // A command with no handshake at all.
        RecordingHandler h;
        NodeControlEndpoint node(nodeIdentity(), testKey(), &h);
        auto r = node.onBytes(frame(encode(makeCmd(cmd::kStartAt, QStringLiteral("c2")))), 1000);
        const DecodedControl d = decodeControl(r.reply.mid(4));
        check(d.type == MessageType::Ack && !d.ack.accepted,
              "RMS-CTRL-001: a command with NO authentication is refused");
        check(h.applied.isEmpty(), "RMS-CTRL-001: and applies nothing");
    }
    {
        // A tampered MAC.
        RecordingHandler h;
        NodeControlEndpoint node(nodeIdentity(), testKey(), &h);
        RmsControlClient rms(QStringLiteral("RMS-1"), testKey());
        auto nr = node.onBytes(rms.start(), 1000);
        rms.onBytes(nr.reply);
        Auth bad;
        bad.controlProtocolVersion = kControlProtocolVersion;
        bad.mac = QString(64, QLatin1Char('0'));
        auto nr2 = node.onBytes(frame(encode(bad)), 1000);
        const DecodedControl d = decodeControl(nr2.reply.mid(4));
        check(d.type == MessageType::AuthResult && !d.authResult.accepted,
              "RMS-CTRL-001: a forged MAC is rejected");
        check(nr2.closeConnection,
              "RMS-CTRL-001: and the connection is closed rather than left open "
              "for another guess");
    }
    {
        // A handshake captured from one node replayed at another. The nodeId is
        // inside the MAC, so it cannot transfer.
        RecordingHandler h;
        NodeControlEndpoint other(nodeIdentity(QStringLiteral("TA-NODE-002")), testKey(), &h);
        RmsControlClient rms(QStringLiteral("RMS-1"), testKey());
        rms.setExpectedNode(QStringLiteral("TA-NODE-001"));
        auto nr = other.onBytes(rms.start(), 1000);
        auto rr = rms.onBytes(nr.reply);
        check(rr.closeConnection && rms.state() == RmsControlClient::State::Error,
              "RMS-CTRL-001: a node answering with a DIFFERENT identity is "
              "dropped - the address is not the identity", rms.lastError());
    }
    {
        // Wrong control protocol version.
        RecordingHandler h;
        NodeControlEndpoint node(nodeIdentity(), testKey(), &h);
        QByteArray bad = "{\"controlProtocolVersion\":99,\"messageType\":\"HELLO\","
                         "\"rmsNonce\":\"aa\"}";
        auto r = node.onBytes(frame(bad), 1000);
        check(r.closeConnection,
              "RMS-CTRL-001: an unimplemented control version is rejected, never "
              "silently downgraded");
    }
    {
        // Malformed JSON, and a frame with no version at all.
        RecordingHandler h;
        NodeControlEndpoint node(nodeIdentity(), testKey(), &h);
        check(node.onBytes(frame(QByteArray("{not json")), 1000).closeConnection,
              "RMS-CTRL-001: malformed JSON is rejected");
        NodeControlEndpoint node2(nodeIdentity(), testKey(), &h);
        check(node2.onBytes(frame(QByteArray("{\"messageType\":\"HELLO\"}")), 1000).closeConnection,
              "RMS-CTRL-001: a frame with no controlProtocolVersion is not this "
              "protocol and is rejected");
    }
    {
        // An unusable key must not become "no authentication required".
        RecordingHandler h;
        NodeControlEndpoint node(nodeIdentity(), QByteArray("short"), &h);
        RmsControlClient rms(QStringLiteral("RMS-1"), testKey());
        auto nr = node.onBytes(rms.start(), 1000);
        auto rr = rms.onBytes(nr.reply);
        auto nr2 = node.onBytes(rr.reply, 1000);
        const DecodedControl d = decodeControl(nr2.reply.mid(4));
        check(!d.authResult.accepted,
              "RMS-CTRL-001: an unusable range key FAILS authentication - it "
              "never degrades to an open channel");
    }

    // ── §14 idempotency ───────────────────────────────────────────────────
    {
        Pair p(testKey(), testKey());
        check(p.handshake(), "RMS-CTRL-001: authenticated for the duplicate test");

        Ack a1 = p.send(makeCmd(cmd::kAssignAthlete, QStringLiteral("cmd-42")), 1000);
        check(a1.accepted && !a1.duplicate,
              "RMS-CTRL-001: the first ASSIGN_ATHLETE is applied");
        check(p.handler.applied.size() == 1,
              "RMS-CTRL-001: the handler ran exactly once",
              QString::number(p.handler.applied.size()));

        Ack a2 = p.send(makeCmd(cmd::kAssignAthlete, QStringLiteral("cmd-42")), 1200);
        check(a2.duplicate,
              "RMS-CTRL-001: the SAME commandId is reported as a duplicate");
        check(a2.accepted == a1.accepted && a2.reasonCode == a1.reasonCode,
              "RMS-CTRL-001: and returns the ORIGINAL outcome");
        check(p.handler.applied.size() == 1,
              "RMS-CTRL-001: THE ACTION WAS NOT PERFORMED TWICE - which is what "
              "stops a retried feed feeding twice",
              QString::number(p.handler.applied.size()));
        check(p.node.duplicatesRefused() == 1,
              "RMS-CTRL-001: and the node counted the refusal");
    }

    // ── §11 capability gating ─────────────────────────────────────────────
    {
        Pair p(testKey(), testKey());
        p.handshake();
        Ack a = p.send(makeCmd(cmd::kFeedPaper, QStringLiteral("feed-1")), 1000);
        check(!a.accepted && a.reasonCode == QLatin1String(reason::kUnsupportedCapability),
              "RMS-CTRL-001: FEED_PAPER is refused because the node does not "
              "advertise it - capability-driven, not name-driven", a.reasonCode);
        check(p.handler.applied.isEmpty(),
              "RMS-CTRL-001: and it never reached the handler");
    }

    // ── unknown command, wrong node, stale command ────────────────────────
    {
        Pair p(testKey(), testKey());
        p.handshake();
        Ack a = p.send(makeCmd("DESTROY_EVERYTHING", QStringLiteral("x1")), 1000);
        check(!a.accepted && a.reasonCode == QLatin1String(reason::kUnknownCommand),
              "RMS-CTRL-001: an unknown command type is refused", a.reasonCode);

        Command mis = makeCmd(cmd::kStop, QStringLiteral("x2"));
        mis.nodeId = QStringLiteral("TA-NODE-999");
        Ack b = p.send(mis, 1000);
        check(!b.accepted && b.reasonCode == QLatin1String(reason::kBadNode),
              "RMS-CTRL-001: a command addressed to another node is refused even "
              "though it arrived here", b.reasonCode);

        Ack c = p.send(makeCmd(cmd::kStop, QStringLiteral("x3"), 1000),
                       1000 + NodeControlEndpoint::kCommandWindowMs + 1);
        check(!c.accepted && c.reasonCode == QLatin1String(reason::kStaleCommand),
              "RMS-CTRL-001: a command older than the window is refused - a "
              "captured START cannot be replayed an hour later", c.reasonCode);
    }

    // ── auth primitives ───────────────────────────────────────────────────
    {
        check(makeNonce() != makeNonce(),
              "RMS-CTRL-001: nonces are fresh, not fixed");
        const QString m1 = computeMac(testKey(), "a", "b", "node1", "rms1");
        check(m1 == computeMac(testKey(), "a", "b", "node1", "rms1"),
              "RMS-CTRL-001: the MAC is deterministic for identical input");
        check(m1 != computeMac(testKey(), "a", "b", "node2", "rms1"),
              "RMS-CTRL-001: changing ONLY the nodeId changes the MAC");
        check(m1 != computeMac(wrongKey(), "a", "b", "node1", "rms1"),
              "RMS-CTRL-001: changing only the key changes the MAC");
        // The separator is what stops field boundaries being shifted.
        check(computeMac(testKey(), "ab", "c", "n", "r")
              != computeMac(testKey(), "a", "bc", "n", "r"),
              "RMS-CTRL-001: fields are separated, so bytes cannot be moved "
              "between them to forge the same MAC");
        check(!macEquals(QString(), QString()),
              "RMS-CTRL-001: two empty MACs are not a match");
    }
}
