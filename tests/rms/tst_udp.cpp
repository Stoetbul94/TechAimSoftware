// The real socket path. Proves that the receive-only endpoint actually binds
// and delivers a node's datagram into the observer — on a scratch port, so
// the test never touches the live range port.

#include "test_support.h"

#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "rms/RmsUdpObserver.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QUdpSocket>

#include <cstdio>
#include <functional>

using namespace ta::rms;

namespace {

// Spins the event loop until `predicate` holds or the budget expires. No
// sleeps: the loop returns as soon as the datagram lands.
bool waitFor(const std::function<bool()>& predicate, int budgetMs = 2000)
{
    QElapsedTimer t;
    t.start();
    while (!predicate() && t.elapsed() < budgetMs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
    return predicate();
}

} // namespace

void run_udp_tests()
{
    std::printf("\n-- udp observation --\n");

    // A scratch port. The live observation port (7755) is deliberately not
    // bound by the harness: a test must never contend with a running range.
    const quint16 scratchPort = 47755;

    RangeMonitor monitor;
    RmsUdpObserver observer;
    QObject::connect(&observer, &RmsUdpObserver::datagramReceived,
                     [&](const QByteArray& d) { monitor.ingestDatagram(d, 1000); });

    const bool bound = observer.listen(scratchPort);
    check(bound, "the receive-only endpoint binds", observer.lastError());
    if (!bound) {
        check(false, "udp observation tests skipped - no socket");
        return;
    }
    check(observer.isListening() && observer.boundPort() == scratchPort,
          "the endpoint reports the port it is observing");

    // Stand in for a target node broadcasting its telemetry.
    QUdpSocket node;
    NodeAnnounce a;
    a.nodeId = QStringLiteral("TA-NODE-UDP");
    a.bootId = QStringLiteral("boot-udp");
    a.laneId = QStringLiteral("Lane 9");
    const QByteArray announcePayload = encode(a);
    node.writeDatagram(announcePayload, QHostAddress::LocalHost, scratchPort);

    check(waitFor([&] { return monitor.nodeCount() == 1; }),
          "a datagram sent by a node is discovered over a real socket");

    AcceptedShot sh;
    sh.eventId = QStringLiteral("udp-evt-1");
    sh.nodeId = a.nodeId;
    sh.bootId = a.bootId;
    sh.sessionId = QStringLiteral("udp-sess");
    sh.shotSequence = 1;
    sh.authoritativeScore = 10.9;
    sh.innerTen = true;
    sh.acquisitionStatus = QStringLiteral("ACCEPTED");
    const QByteArray shotPayload = encode(sh);
    node.writeDatagram(shotPayload, QHostAddress::LocalHost, scratchPort);
    node.writeDatagram(shotPayload, QHostAddress::LocalHost, scratchPort);  // duplicate

    check(waitFor([&] {
              const TargetNodeRecord* r = monitor.nodeById(a.nodeId);
              return r && r->ledger.duplicatesSuppressed() == 1;
          }),
          "a duplicate delivered over the real socket is suppressed once");

    const TargetNodeRecord* r = monitor.nodeById(a.nodeId);
    check(r && r->ledger.observedCount() == 1, "the duplicated shot is held exactly once");
    check(r && qAbs(r->ledger.latestReceived().authoritativeScore - 10.9) < 1e-9,
          "the node's score survives the network path unchanged");

    // Junk on the wire must not disturb a live range.
    monitor.ingestDatagram(QByteArray("<<< not our protocol >>>"), 1000);
    check(monitor.rejectedDatagrams() == 1, "foreign traffic on the port is rejected");
    check(monitor.nodeCount() == 1, "...and creates no lane");
}
