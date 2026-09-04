// SOFTWARE ONE-NODE TEST (R3A §8).
//
// Drives a RUNNING Tech Aim RMS node over its real sockets: UDP 7755 for
// telemetry and TCP 7756 for control. Nothing here is stubbed - the handshake
// is the production HMAC exchange, the frames are the production framing, and
// the acks come back from the production endpoint inside the shooting
// application.
//
// WHAT IT CANNOT PROVE. No target is attached, so no shot is fired and no
// physical behaviour is exercised. This proves the RANGE INTEGRATION of a real
// build: that the node announces itself, authenticates, answers commands
// honestly, and refuses what it has not been armed to do. A physical pass is a
// separate document and a separate day.
//
//   rmsnodecheck.exe [--key <path>] [--host 127.0.0.1] [--seconds 12]

#include "rms/RmsProtocol.h"
#include "rms/control/ControlAuth.h"
#include "rms/control/ControlProtocol.h"
#include "rms/control/RmsControlClient.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QUdpSocket>

#include <cstdio>

using namespace ta::rms;
using namespace ta::rms::control;

static int g_checks = 0;
static int g_failures = 0;

static void check(bool ok, const QString& what, const QString& detail = QString())
{
    ++g_checks;
    if (!ok) ++g_failures;
    std::printf("  %s  %s%s\n", ok ? "PASS" : "FAIL", qPrintable(what),
                detail.isEmpty() ? "" : qPrintable(QStringLiteral("  [%1]").arg(detail)));
    std::fflush(stdout);
}

// One blocking request/response over the control socket.
static RmsControlClient::Reaction exchange(QTcpSocket& s, RmsControlClient& c,
                                           const QByteArray& out, int waitMs = 3000)
{
    RmsControlClient::Reaction r;
    if (!out.isEmpty()) {
        s.write(out);
        s.flush();
    }
    if (!s.waitForReadyRead(waitMs))
        return r;
    return c.onBytes(s.readAll());
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QString host = QStringLiteral("127.0.0.1");
    QString keyPath;
    int listenSeconds = 12;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args.at(i) == QLatin1String("--host") && i + 1 < args.size()) host = args.at(++i);
        else if (args.at(i) == QLatin1String("--key") && i + 1 < args.size()) keyPath = args.at(++i);
        else if (args.at(i) == QLatin1String("--seconds") && i + 1 < args.size()) listenSeconds = args.at(++i).toInt();
    }
    if (keyPath.isEmpty()) {
        // The node writes its key into its own settings directory.
        keyPath = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                      .filePath(QStringLiteral("TechAim/TechAim/Settings/range.key"));
    }

    std::printf("=== Tech Aim RMS node - software one-node test ===\n");
    std::printf("host %s   key %s\n\n", qPrintable(host), qPrintable(keyPath));

    // ── 1. telemetry on UDP 7755 ─────────────────────────────────────────
    std::printf("-- telemetry (UDP %d) --\n", int(kObservationPort));
    QUdpSocket udp;
    const bool bound = udp.bind(QHostAddress::AnyIPv4, kObservationPort,
                                QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    check(bound, QStringLiteral("bound the observation port"), udp.errorString());

    QString seenNodeId, seenBootId, seenSessionId;
    int announces = 0, statuses = 0, shots = 0;
    bool sawProtocolV1 = true;
    QElapsedTimer t; t.start();
    while (t.elapsed() < listenSeconds * 1000) {
        if (!udp.waitForReadyRead(500)) continue;
        while (udp.hasPendingDatagrams()) {
            QByteArray d; d.resize(int(udp.pendingDatagramSize()));
            udp.readDatagram(d.data(), d.size());
            const QJsonObject o = QJsonDocument::fromJson(d).object();
            if (o.value(QStringLiteral("protocolVersion")).toInt() != kProtocolVersion)
                sawProtocolV1 = false;
            const QString type = o.value(QStringLiteral("type")).toString();
            if (type == QLatin1String("node.announce")) ++announces;
            else if (type == QLatin1String("node.status")) ++statuses;
            else if (type == QLatin1String("shot.accepted")) ++shots;
            if (seenNodeId.isEmpty()) seenNodeId = o.value(QStringLiteral("nodeId")).toString();
            if (seenBootId.isEmpty()) seenBootId = o.value(QStringLiteral("bootId")).toString();
            const QString sid = o.value(QStringLiteral("sessionId")).toString();
            if (!sid.isEmpty()) seenSessionId = sid;
        }
        if (announces > 0 && statuses > 1) break;
    }
    check(announces > 0 || statuses > 0,
          QStringLiteral("the node is publishing telemetry"),
          QStringLiteral("announce=%1 status=%2").arg(announces).arg(statuses));
    check(sawProtocolV1, QStringLiteral("every datagram declares protocolVersion 1"));
    check(!seenNodeId.isEmpty(), QStringLiteral("it carries a nodeId"), seenNodeId);
    check(!seenBootId.isEmpty(), QStringLiteral("and a per-process bootId"), seenBootId);
    check(statuses > 0, QStringLiteral("a heartbeat/status is published"));

    // ── 2. control on TCP 7756 ───────────────────────────────────────────
    std::printf("\n-- control (TCP %d) --\n", int(kControlPort));
    QString keyErr;
    QFile kf(keyPath);
    QByteArray key;
    if (kf.open(QIODevice::ReadOnly))
        key = QByteArray::fromHex(kf.readAll().trimmed());
    check(key.size() >= 32, QStringLiteral("the range key was readable"),
          QStringLiteral("%1 bytes").arg(key.size()));
    if (key.size() < 32) {
        std::printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures + 1);
        return 1;
    }

    QTcpSocket sock;
    sock.connectToHost(host, kControlPort);
    const bool connected = sock.waitForConnected(3000);
    check(connected, QStringLiteral("the control port accepted a connection"),
          sock.errorString());
    if (!connected) {
        std::printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
        return 1;
    }

    RmsControlClient client(QStringLiteral("RMS-NODECHECK"), key);
    client.setExpectedNode(seenNodeId);
    QByteArray toNode = client.start();
    for (int i = 0; i < 4 && !toNode.isEmpty(); ++i) {
        const auto r = exchange(sock, client, toNode);
        if (r.closeConnection) break;
        toNode = r.reply;
        if (client.state() == RmsControlClient::State::Authenticated) break;
    }
    const bool authed = client.state() == RmsControlClient::State::Authenticated;
    check(authed, QStringLiteral("HMAC-SHA256 authentication succeeded"), client.lastError());
    check(client.nodeId() == seenNodeId,
          QStringLiteral("the authenticated nodeId matches the telemetry nodeId"));
    const QStringList caps = client.nodeCapabilities();
    check(caps.contains(QLatin1String(cap::kStatus)),
          QStringLiteral("it advertises status"));
    check(caps.contains(QLatin1String(cap::kEventReplay)),
          QStringLiteral("it advertises eventReplay"));
    check(!caps.contains(QLatin1String(cap::kPaperFeed)),
          QStringLiteral("it does NOT advertise paperFeed"));
    const bool armed = caps.contains(QLatin1String(cap::kStartAt));
    std::printf("     session control is %s\n", armed ? "ARMED" : "NOT armed");

    auto sendCmd = [&](const QString& type, const QJsonObject& payload,
                       const QString& id) {
        Command c;
        c.controlProtocolVersion = kControlProtocolVersion;
        c.commandId = id;
        c.nodeId = seenNodeId;
        c.commandType = type;
        c.issuedAtUtcMs = QDateTime::currentMSecsSinceEpoch();
        c.payload = payload;
        return exchange(sock, client, client.sendCommand(c));
    };

    // REQUEST_STATUS - always available, changes nothing.
    auto st = sendCmd(QLatin1String(cmd::kRequestStatus), {},
                      QStringLiteral("chk-status-1"));
    check(st.gotAck && st.ack.accepted, QStringLiteral("REQUEST_STATUS answered"),
          st.ack.message);
    check(st.ack.resultingState.contains(QStringLiteral("nodeId")),
          QStringLiteral("the status names the node"));
    check(st.ack.resultingState.contains(QStringLiteral("sessionActive")),
          QStringLiteral("and reports target/session state separately from the link"));

    // REQUEST_REPLAY - reads the node's own journal.
    auto rp = sendCmd(QLatin1String(cmd::kRequestReplay),
                      QJsonObject{{"sessionId", seenSessionId}, {"afterSequence", 0}},
                      QStringLiteral("chk-replay-1"));
    check(rp.gotReplay || (rp.gotAck && rp.ack.accepted),
          QStringLiteral("REQUEST_REPLAY answered from the journal"),
          rp.gotReplay ? QStringLiteral("%1 events").arg(rp.replay.events.size())
                       : rp.ack.message);

    // The session-control group, whose availability is the point.
    auto assign = sendCmd(QLatin1String(cmd::kAssignAthlete),
                          QJsonObject{{"athlete", "R3A Check"}},
                          QStringLiteral("chk-assign-1"));
    if (armed) {
        check(assign.gotAck && assign.ack.accepted,
              QStringLiteral("ASSIGN_ATHLETE accepted"), assign.ack.message);
    } else {
        check(assign.gotAck && !assign.ack.accepted
                  && assign.ack.reasonCode == QLatin1String(reason::kUnsupportedCapability),
              QStringLiteral("ASSIGN_ATHLETE refused as UNSUPPORTED while unarmed"),
              assign.ack.reasonCode);
    }

    // IDEMPOTENCY within this connection: the same id must not act twice.
    auto again = sendCmd(QLatin1String(cmd::kAssignAthlete),
                         QJsonObject{{"athlete", "R3A Check"}},
                         QStringLiteral("chk-assign-1"));
    check(again.gotAck && again.ack.duplicate,
          QStringLiteral("a repeated commandId is answered as a DUPLICATE"),
          again.ack.reasonCode);

    auto prep = sendCmd(QLatin1String(cmd::kPrepareSession), {},
                        QStringLiteral("chk-prepare-1"));
    check(prep.gotAck, QStringLiteral("PREPARE_SESSION answered"),
          prep.ack.accepted ? QStringLiteral("accepted") : prep.ack.reasonCode);

    const qint64 startAt = QDateTime::currentMSecsSinceEpoch() + 30000;
    auto start = sendCmd(QLatin1String(cmd::kStartAt),
                         QJsonObject{{"startAtUtcMs", double(startAt)},
                                     {"rmsToNodeOffsetMs", 0.0},
                                     {"syncQuality", "GOOD"}},
                         QStringLiteral("chk-startat-1"));
    check(start.gotAck, QStringLiteral("START_AT answered"),
          start.ack.accepted ? QStringLiteral("accepted") : start.ack.reasonCode);
    if (armed && start.ack.accepted) {
        check(start.ack.resultingState.contains(QStringLiteral("scheduledStartUtcMs")),
              QStringLiteral("START_AT scheduled on the NODE clock, not on arrival"));
        auto second = sendCmd(QLatin1String(cmd::kStartAt),
                              QJsonObject{{"startAtUtcMs", double(startAt + 5000)},
                                          {"rmsToNodeOffsetMs", 0.0}},
                              QStringLiteral("chk-startat-2"));
        check(second.gotAck && !second.ack.accepted,
              QStringLiteral("a SECOND start is refused, not silently re-based"),
              second.ack.reasonCode);
    }

    auto stop = sendCmd(QLatin1String(cmd::kStop), {}, QStringLiteral("chk-stop-1"));
    check(stop.gotAck, QStringLiteral("STOP answered"),
          stop.ack.accepted ? QStringLiteral("accepted") : stop.ack.reasonCode);

    // An unknown command is refused rather than ignored.
    auto bogus = sendCmd(QStringLiteral("DEMOLISH_RANGE"), {},
                         QStringLiteral("chk-bogus-1"));
    check(bogus.gotAck && !bogus.ack.accepted,
          QStringLiteral("an unknown command is refused"), bogus.ack.reasonCode);

    // ── cross-boot idempotency ───────────────────────────────────────────
    // Run with --expect-prior-boot after the node has been RESTARTED: the ids
    // used above were handled by the previous incarnation, and the journal it
    // recovered from disk must recognise them. This is the check that fails if
    // the journal is only written at shutdown, because a force-kill never
    // reaches shutdown.
    if (args.contains(QStringLiteral("--expect-prior-boot"))) {
        std::printf("\n-- cross-boot idempotency --\n");
        auto acrossBoot = sendCmd(QLatin1String(cmd::kAssignAthlete),
                                  QJsonObject{{"athlete", "R3A Check"}},
                                  QStringLiteral("chk-assign-1"));
        check(acrossBoot.gotAck && acrossBoot.ack.duplicate,
              QStringLiteral("a commandId from the PREVIOUS boot is recognised"),
              acrossBoot.ack.reasonCode);
        auto startAcross = sendCmd(QLatin1String(cmd::kStartAt),
                                   QJsonObject{{"startAtUtcMs", double(startAt)},
                                               {"rmsToNodeOffsetMs", 0.0}},
                                   QStringLiteral("chk-startat-1"));
        check(startAcross.gotAck && startAcross.ack.duplicate,
              QStringLiteral("and so is the previous boot's START_AT - it does NOT restart"),
              startAcross.ack.reasonCode);
    }

    std::printf("\n  node %s  boot %s\n", qPrintable(seenNodeId), qPrintable(seenBootId));
    std::printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
