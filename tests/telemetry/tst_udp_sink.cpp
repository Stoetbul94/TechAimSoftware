// The real socket path: a real accepted shot leaves a real SessionStore,
// crosses a real UDP socket, and decodes on the other side with the shared
// contract.
//
// This is the milestone's end-to-end assertion minus the RMS observer itself,
// which lives on the RMS product branch — the integration against
// RangeMonitor is asserted there, on the same bytes.

#include "reliability/journal/JournalWriter.h"
#include "reliability/store/SessionStore.h"
#include "rms/RmsProtocol.h"
#include "telemetry/NodeIdentity.h"
#include "telemetry/NodeTelemetryService.h"
#include "telemetry/UdpTelemetrySink.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QMetaMethod>
#include <QTemporaryDir>
#include <QUdpSocket>
#include <QVector>

#include <cstdio>
#include <functional>

using namespace ta::rel;
using namespace ta::telemetry;

extern int g_checks;
extern int g_failures;
void check(bool ok, const char* name, const QString& detail = QString());

namespace {

// A scratch port. The live range port (7755) is deliberately not used by the
// harness: a test must never contend with a running range.
constexpr quint16 kTestPort = 47756;
// The node's LEGACY INBOUND command port. Named here only so it can be
// asserted silent.
constexpr quint16 kLegacyCommandPort = 7756;

bool waitFor(const std::function<bool()>& predicate, int budgetMs = 2000)
{
    QElapsedTimer t;
    t.start();
    while (!predicate() && t.elapsed() < budgetMs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return predicate();
}

// A local in-memory journal, so this harness writes no files. The reliability
// harness has its own richer fault-injecting double; this one only has to keep
// the store off the disk.
class MemJournal : public IJournalFile
{
public:
    QByteArray data;
    bool opened = false;
    bool open() override { opened = true; return true; }
    bool isOpen() const override { return opened; }
    qint64 write(const QByteArray& bytes) override { data += bytes; return bytes.size(); }
    bool flush() override { return true; }
    bool sync() override { return true; }
    QString path() const override { return QStringLiteral(":memory:"); }
    QString lastErrorDetail() const override { return QString(); }
};

ShotCore officialShot(qint16 number, qint16 scoreTenths)
{
    ShotCore s;
    s.shotNumber = number;
    s.withinStage = number;
    s.stageId = 1;
    s.xHundredthMm = -125;
    s.yHundredthMm = 250;
    s.scoreTenths = scoreTenths;
    s.directionCentiDeg = 4500;
    s.splitMs = 8000;
    s.windowId = 1;
    s.targetMode = 1;
    s.externalId = number;
    return s;
}

SessionHeader header()
{
    SessionHeader h;
    h.sessionId = QStringLiteral("99999999-0000-4000-8000-00000000ffff");
    h.appVersion = QStringLiteral("0.9.0-test");
    h.athlete = QStringLiteral("A. Bailie");
    h.lane = QStringLiteral("Lane 1");
    h.discipline = Discipline::AirRifle10m;
    h.config.officialShots = 60;
    h.config.seriesSize = 10;
    h.config.matchMs = 4500000;
    return h;
}

} // namespace

void run_udp_sink_tests()
{
    std::printf("\n-- udp sink --\n");

    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        check(false, "could not create a scratch settings dir");
        return;
    }
    const QString ini = tmp.filePath(QStringLiteral("node.ini"));

    // Stand in for the RMS observer.
    QUdpSocket receiver;
    const bool bound = receiver.bind(kTestPort, QUdpSocket::ShareAddress);
    check(bound, "the observing socket binds", receiver.errorString());
    if (!bound)
        return;

    // Nothing may ever arrive on the node's legacy inbound command port.
    QUdpSocket legacyWatcher;
    const bool legacyBound = legacyWatcher.bind(kLegacyCommandPort, QUdpSocket::ShareAddress);

    QVector<QByteArray> received;
    QObject::connect(&receiver, &QUdpSocket::readyRead, [&] {
        while (receiver.hasPendingDatagrams()) {
            QByteArray d;
            d.resize(int(receiver.pendingDatagramSize()));
            receiver.readDatagram(d.data(), d.size());
            received.append(d);
        }
    });

    // ── the real path ──────────────────────────────────────────────────
    UdpTelemetrySink sink;
    sink.setDestination(QHostAddress::LocalHost, kTestPort);

    MemJournal file;
    ManualClock clock;
    SessionStore store;
    store.setClockForTesting(&clock);
    store.setJournalFileForTesting(&file);

    NodeTelemetryService node(NodeIdentity::forSettingsFile(ini), &sink);
    node.setAppVersion(QStringLiteral("0.9.0-test"));
    node.setProductIdentity(QStringLiteral("Tech Aim"));
    node.setDeviceIdentity(QStringLiteral("TechAim-EST/4100"));
    node.setTargetConnected(true);
    node.setProgramme(QStringLiteral("issf.10m.air-rifle.qualification60"),
                      QStringLiteral("issf"), QStringLiteral("issf.10m.air-rifle"));
    node.attachStore(&store);
    node.start();
    node.flushOutbox();

    check(waitFor([&] { return received.size() >= 2; }),
          "announce and status cross a real UDP socket");

    // Now a real accepted shot, through the real store.
    store.beginSession(header());
    store.submit(DomainEvent(PreparationStarted{0}));
    store.submit(DomainEvent(SightingStarted{0}));
    store.submit(DomainEvent(OfficialMatchStarted{1}));
    received.clear();
    const SubmitResult r =
        store.submit(DomainEvent(ShotAccepted{officialShot(1, 104)}));
    node.flushOutbox();

    check(r.ok, "the node accepted the shot");
    check(waitFor([&] {
              for (const QByteArray& d : received)
                  if (ta::rms::decode(d).type == ta::rms::MessageType::AcceptedShot)
                      return true;
              return false;
          }),
          "the accepted shot arrives on the wire");

    ta::rms::AcceptedShot wire;
    bool found = false;
    for (const QByteArray& d : received) {
        const ta::rms::DecodedMessage m = ta::rms::decode(d);
        if (m.type == ta::rms::MessageType::AcceptedShot) {
            wire = m.shot;
            found = true;
        }
    }
    check(found, "the shot decoded with the shared contract");
    if (found) {
        check(wire.shotSequence == 1, "the sequence survived the wire");
        check(qAbs(wire.authoritativeScore - 10.4) < 1e-9,
              "the NODE's score survived the wire unchanged");
        check(wire.sessionId == header().sessionId, "the sessionId survived the wire");
        check(wire.programmeId == QLatin1String("issf.10m.air-rifle.qualification60"),
              "the programmeId survived the wire");
        check(wire.nodeId == node.nodeId(), "the nodeId survived the wire");
        check(wire.bootId == node.bootId(), "the bootId survived the wire");
    }

    // Every datagram the node emits must be understood by the contract.
    int rejects = 0;
    for (const QByteArray& d : received)
        if (ta::rms::decode(d).type == ta::rms::MessageType::Unknown)
            ++rejects;
    check(rejects == 0, "no datagram the node emitted is rejected by the decoder",
          QString::number(rejects));

    check(sink.sentCount() > 0, "the sink counted what it sent");
    check(sink.failedCount() == 0, "no send failed on loopback");

    // ── the sink cannot receive ────────────────────────────────────────
    {
        // The one property this milestone must keep: node → RMS only.
        const QMetaObject* mo = &UdpTelemetrySink::staticMetaObject;
        bool anyReceiveSlot = false;
        for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
            const QByteArray n = mo->method(i).name().toLower();
            if (n.contains("read") || n.contains("receive") || n.contains("pending"))
                anyReceiveSlot = true;
        }
        check(!anyReceiveSlot, "the telemetry sink exposes no receive path");

        if (legacyBound) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            check(!legacyWatcher.hasPendingDatagrams(),
                  "the node's legacy inbound command port (7756) stays silent");
        } else {
            check(true, "legacy command port already held by another process - not asserted");
        }
    }

    // ── a failing destination never reaches the match ──────────────────
    {
        // Point the sink at an address the host cannot route to. The shot must
        // still be accepted and journalled.
        UdpTelemetrySink deadSink;
        deadSink.setDestination(QHostAddress(QStringLiteral("203.0.113.1")), 1);
        MemJournal f2;
        ManualClock c2;
        SessionStore s2;
        s2.setClockForTesting(&c2);
        s2.setJournalFileForTesting(&f2);
        NodeTelemetryService n2(NodeIdentity::forSettingsFile(ini), &deadSink);
        n2.attachStore(&s2);
        n2.start();

        SessionHeader h2 = header();
        h2.sessionId = QStringLiteral("88888888-0000-4000-8000-00000000eeee");
        s2.beginSession(h2);
        s2.submit(DomainEvent(OfficialMatchStarted{1}));
        const SubmitResult r2 = s2.submit(DomainEvent(ShotAccepted{officialShot(1, 104)}));
        n2.flushOutbox();
        check(r2.ok, "a shot is accepted while telemetry goes nowhere");
        check(s2.state().officials.size() == 1,
              "...and reaches the authoritative record regardless");
        check(n2.queuedCount() == 0, "...and nothing accumulates in the outbox");
    }
}
