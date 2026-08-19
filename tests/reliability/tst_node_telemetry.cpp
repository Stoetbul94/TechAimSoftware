// RMS node telemetry — milestone 2.
//
// Everything here is driven through a REAL SessionStore: events are submitted
// the way the controllers submit them, and the publisher observes the same
// eventApplied seam it observes in production. Nothing fakes a shot into the
// telemetry layer directly, because a telemetry-only shot route would prove
// nothing about the path that actually matters.
//
// That this file compiles in a QT = core harness is itself the assertion that
// the publisher carries no GUI and no socket dependency.

#include "reliability/store/SessionStore.h"
#include "rms/RmsProtocol.h"
#include "telemetry/NodeIdentity.h"
#include "telemetry/NodeTelemetryService.h"
#include "test_support.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QVector>

#include <cstdio>

using namespace ta::rel;
using namespace ta::telemetry;

namespace {

// Records what the node put on the wire, and can be made to fail on demand.
class RecordingSink : public ITelemetrySink
{
public:
    QVector<QByteArray> sent;
    bool failNext = false;
    int  failures = 0;

    bool send(const QByteArray& datagram) override
    {
        if (failNext) {
            ++failures;
            return false;
        }
        sent.append(datagram);
        return true;
    }

    QVector<ta::rms::DecodedMessage> decodedOf(ta::rms::MessageType type) const
    {
        QVector<ta::rms::DecodedMessage> out;
        for (const QByteArray& d : sent) {
            const ta::rms::DecodedMessage m = ta::rms::decode(d);
            if (m.type == type)
                out.append(m);
        }
        return out;
    }
    void clear() { sent.clear(); }
};

SessionHeader qualHeader(const QString& sid = QStringLiteral("11111111-0000-4000-8000-00000000aaaa"))
{
    SessionHeader h;
    h.sessionId = sid;
    h.appVersion = QStringLiteral("0.9.0-test");
    h.athlete = QStringLiteral("A. Bailie");
    h.lane = QStringLiteral("Lane 1");
    h.targetId = QStringLiteral("T1");
    h.deviceId = QStringLiteral("TEST-DEVICE");
    h.matchType = QStringLiteral("60");
    h.discipline = Discipline::AirRifle10m;
    h.config.officialShots = 60;
    h.config.seriesSize = 10;
    h.config.matchMs = 4500000;
    return h;
}

// One rig: a real store, a real publisher, a recording sink, a fixed clock.
struct Rig {
    MemoryJournalFile file;
    ManualClock clock;
    SessionStore store;
    RecordingSink sink;
    NodeTelemetryService telemetry;

    explicit Rig(const QString& settingsIni)
        : telemetry(NodeIdentity::forSettingsFile(settingsIni), &sink)
    {
        store.setClockForTesting(&clock);
        store.setJournalFileForTesting(&file);
        telemetry.setClockForTesting([] { return qint64(1700000000000LL); });
        telemetry.setAppVersion(QStringLiteral("0.9.0-test"));
        telemetry.setProductIdentity(QStringLiteral("Tech Aim"));
        telemetry.setDeviceIdentity(QStringLiteral("TechAim-EST/4100"));
        telemetry.setTargetConnected(true);
        telemetry.setProgramme(QStringLiteral("issf.10m.air-rifle.qualification60"),
                               QStringLiteral("issf"),
                               QStringLiteral("issf.10m.air-rifle"));
        telemetry.attachStore(&store);
        telemetry.start();
        telemetry.flushOutbox();
    }

    // Up to and including the sighting phase — where a sighter is legal.
    void beginSighting(const SessionHeader& h)
    {
        store.beginSession(h);
        store.submit(DomainEvent(PreparationStarted{0}));
        store.submit(DomainEvent(SightingStarted{0}));
    }
    void beginMatch(const SessionHeader& h)
    {
        beginSighting(h);
        store.submit(DomainEvent(OfficialMatchStarted{1}));
    }
};

} // namespace

void run_node_telemetry_tests()
{
    std::printf("--- RMS node telemetry ---\n");

    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        check(false, "node telemetry: could not create a scratch settings dir");
        return;
    }
    const QString ini = tmp.filePath(QStringLiteral("node.ini"));

    // ── node identity ──────────────────────────────────────────────────
    {
        const NodeIdentity first = NodeIdentity::forSettingsFile(ini);
        check(!first.nodeId().isEmpty(), "a station mints a nodeId on first run");
        check(first.nodeIdWasGenerated(), "...and says that it generated one");
        check(first.nodeId().startsWith(QLatin1String("TA-NODE-")),
              "the nodeId is recognisable at a glance", first.nodeId());

        // A second construction IS a restart: same station, new process.
        const NodeIdentity second = NodeIdentity::forSettingsFile(ini);
        check(second.nodeId() == first.nodeId(),
              "the nodeId is STABLE across an application restart");
        check(!second.nodeIdWasGenerated(), "...and is not minted a second time");
        check(second.bootId() != first.bootId(),
              "the bootId is FRESH on every process start");
        check(!second.bootId().isEmpty(), "the bootId is always populated");

        // Nothing about the identity may come from a port, an address or a lane.
        const QString id = first.nodeId();
        check(!id.contains(QLatin1String("COM")), "the nodeId is not a COM port");
        check(!id.contains(QLatin1String("192.168")), "the nodeId is not an IP address");
        check(!id.contains(QLatin1String("Lane")), "the nodeId is not a lane name");

        // A different station gets a different identity.
        const QString otherIni = tmp.filePath(QStringLiteral("other.ini"));
        check(NodeIdentity::forSettingsFile(otherIni).nodeId() != first.nodeId(),
              "two stations do not share a node identity");
    }

    // ── announce and status ────────────────────────────────────────────
    {
        Rig rig(ini);
        const auto announces = rig.sink.decodedOf(ta::rms::MessageType::NodeAnnounce);
        check(announces.size() == 1, "starting the node announces it exactly once");
        if (!announces.isEmpty()) {
            const ta::rms::NodeAnnounce& a = announces.first().announce;
            check(a.protocolVersion == ta::rms::kProtocolVersion,
                  "the announce carries the protocol version the observer expects");
            check(a.nodeId == rig.telemetry.nodeId(), "the announce carries the nodeId");
            check(a.bootId == rig.telemetry.bootId(), "the announce carries the bootId");
            check(a.deviceIdentity == QLatin1String("TechAim-EST/4100"),
                  "device identity is carried SEPARATELY from node identity");
            check(a.productIdentity == QLatin1String("Tech Aim"), "product identity is carried");
        }

        const auto statuses = rig.sink.decodedOf(ta::rms::MessageType::NodeStatus);
        check(statuses.size() == 1, "starting the node publishes one status");
        if (!statuses.isEmpty()) {
            const ta::rms::NodeStatus& s = statuses.first().status;
            check(s.connection == ta::rms::ConnectionState::TargetConnected,
                  "a connected target reports TARGET_CONNECTED");
            check(s.phase == ta::rms::MatchPhase::Idle,
                  "a station with no session open reports IDLE");
            check(s.statusSeq == 1, "statusSeq starts at 1 within a boot");
        }

        // Losing the target is reported at once, and is NOT the same as being
        // unreachable — a node can be perfectly reachable with a dead target.
        rig.sink.clear();
        rig.telemetry.setTargetConnected(false);
        rig.telemetry.flushOutbox();
        const auto afterLoss = rig.sink.decodedOf(ta::rms::MessageType::NodeStatus);
        check(afterLoss.size() == 1, "losing the target publishes a status immediately");
        if (!afterLoss.isEmpty())
            check(afterLoss.first().status.connection
                      == ta::rms::ConnectionState::TargetDisconnected,
                  "a lost target reports TARGET_DISCONNECTED");

        // OFFLINE is RMS's conclusion, never a node's claim.
        bool anyOffline = false;
        for (const QByteArray& d : rig.sink.sent)
            if (d.contains("\"connection\":\"OFFLINE\""))
                anyOffline = true;
        check(!anyOffline, "a node NEVER declares itself OFFLINE");
    }

    // ── accepted shots ─────────────────────────────────────────────────
    {
        Rig rig(ini);
        rig.beginMatch(qualHeader());
        rig.sink.clear();

        rig.store.submit(DomainEvent(ShotAccepted{testjournal::shot(1, 104, -125, 250)}));
        rig.telemetry.flushOutbox();

        const auto shots = rig.sink.decodedOf(ta::rms::MessageType::AcceptedShot);
        check(shots.size() == 1, "ONE accepted shot produces ONE shot.accepted event");
        if (!shots.isEmpty()) {
            const ta::rms::AcceptedShot& s = shots.first().shot;
            check(s.shotSequence == 1,
                  "shotSequence is the node's accepted sequence, not a packet count");
            check(qAbs(s.authoritativeScore - 10.4) < 1e-9,
                  "the NODE's accepted score is transported verbatim");
            check(s.integerScore == 10, "the integer ring value accompanies it");
            check(qAbs(s.rawXMm + 1.25) < 1e-9 && qAbs(s.rawYMm - 2.5) < 1e-9,
                  "raw coordinates are converted from the stored fixed point");
            check(s.sessionId == qualHeader().sessionId, "sessionId is preserved");
            check(s.programmeId == QLatin1String("issf.10m.air-rifle.qualification60"),
                  "the stable programmeId is preserved");
            check(s.nodeId == rig.telemetry.nodeId(), "the shot names its node");
            check(s.bootId == rig.telemetry.bootId(), "the shot names its boot");
            check(!s.eventId.isEmpty(), "the shot carries an eventId");
        }

        // The status that follows a shot must already reflect it: a range
        // officer reading the count must not be a heartbeat behind.
        const auto statuses = rig.sink.decodedOf(ta::rms::MessageType::NodeStatus);
        check(!statuses.isEmpty(), "a shot is followed by a status");
        if (!statuses.isEmpty()) {
            const ta::rms::NodeStatus& st = statuses.last().status;
            check(st.shotsAccepted == 1, "the status carries the authoritative accepted count");
            check(st.shotsExpected == 60, "...and the course length");
            check(qAbs(st.totalScore - 10.4) < 1e-9,
                  "...and the authoritative running total");
            check(st.phase == ta::rms::MatchPhase::Match, "...and the match phase");
            check(st.rulesetId == QLatin1String("issf")
                      && st.targetStandardId == QLatin1String("issf.10m.air-rifle"),
                  "...and the rule authority and target standard");
        }
    }

    // ── event identity and deduplication ───────────────────────────────
    {
        Rig rig(ini);
        rig.beginMatch(qualHeader());
        rig.sink.clear();

        rig.store.submit(DomainEvent(ShotAccepted{testjournal::shot(1, 100)}));
        rig.store.submit(DomainEvent(ShotAccepted{testjournal::shot(2, 98)}));
        rig.telemetry.flushOutbox();
        auto shots = rig.sink.decodedOf(ta::rms::MessageType::AcceptedShot);
        check(shots.size() == 2, "two accepted shots produce two events");
        const QString firstId = shots.isEmpty() ? QString() : shots.first().shot.eventId;
        check(shots.size() == 2 && shots.at(0).shot.eventId != shots.at(1).shot.eventId,
              "different shots get different eventIds");

        // A duplicated internal delivery of the SAME accepted shot must not
        // become a second observation on the range display.
        rig.sink.clear();
        rig.telemetry.publishStatus();   // unrelated traffic in between
        const int before = rig.telemetry.shotsPublished();
        emit rig.store.eventApplied(DomainEvent(ShotAccepted{testjournal::shot(1, 100)}), false);
        rig.telemetry.flushOutbox();
        check(rig.telemetry.shotsPublished() == before,
              "a duplicated internal delivery publishes NO second accepted shot");
        check(rig.sink.decodedOf(ta::rms::MessageType::AcceptedShot).isEmpty(),
              "...and nothing reaches the wire");

        // A retransmission of the same shot must keep its identity, or RMS
        // could not suppress it.
        Rig replay(ini);
        replay.beginMatch(qualHeader());
        replay.sink.clear();
        replay.store.submit(DomainEvent(ShotAccepted{testjournal::shot(1, 100)}));
        replay.telemetry.flushOutbox();
        const auto again = replay.sink.decodedOf(ta::rms::MessageType::AcceptedShot);
        check(!again.isEmpty() && again.first().shot.eventId == firstId,
              "the same shot in the same session reproduces the SAME eventId",
              again.isEmpty() ? QString() : again.first().shot.eventId);
    }

    // ── sighter classification ─────────────────────────────────────────
    {
        Rig rig(ini);
        // A sighter is submitted during SIGHTING, where it is legal, so the
        // reducer really accepts it and the publisher really sees it.
        rig.beginSighting(qualHeader());
        rig.sink.clear();

        const SubmitResult sighter =
            rig.store.submit(DomainEvent(SighterAccepted{testjournal::shot(0, 95)}));
        check(sighter.ok, "the sighter is accepted by the node");
        check(rig.store.state().sighters.size() == 1,
              "...and reaches the node's own sighter record");
        rig.telemetry.flushOutbox();
        check(rig.sink.decodedOf(ta::rms::MessageType::AcceptedShot).isEmpty(),
              "a SIGHTER is not published as a match shot");
        check(rig.telemetry.shotsPublished() == 0, "...and does not count as one");

        rig.store.submit(DomainEvent(OfficialMatchStarted{1}));
        rig.store.submit(DomainEvent(ShotAccepted{testjournal::shot(1, 105)}));
        rig.telemetry.flushOutbox();
        const auto shots = rig.sink.decodedOf(ta::rms::MessageType::AcceptedShot);
        check(shots.size() == 1, "the official that follows it is published");
        check(!shots.isEmpty() && shots.first().shot.shotSequence == 1,
              "...as sequence 1 - the sighter did not consume a match number");
        const auto statuses = rig.sink.decodedOf(ta::rms::MessageType::NodeStatus);
        check(!statuses.isEmpty() && statuses.last().status.shotsAccepted == 1,
              "the accepted count counts officials only");
    }

    // ── recovery replay is history, not telemetry ──────────────────────
    {
        Rig rig(ini);
        rig.beginMatch(qualHeader());
        // Drain the phase-transition traffic FIRST, then clear, so what
        // follows measures the replayed event and nothing else.
        rig.telemetry.flushOutbox();
        rig.sink.clear();
        check(rig.telemetry.queuedCount() == 0, "the outbox is drained before the replay");

        const int before = rig.telemetry.shotsPublished();
        emit rig.store.eventApplied(DomainEvent(ShotAccepted{testjournal::shot(7, 101)}), true);
        rig.telemetry.flushOutbox();
        check(rig.telemetry.shotsPublished() == before,
              "a REPLAYED event publishes nothing - replay is history being rebuilt");
        check(rig.sink.sent.isEmpty(), "...and puts nothing on the wire",
              QString::number(rig.sink.sent.size()));
    }

    // ── the node is never affected by telemetry ────────────────────────
    {
        // No sink at all is the "RMS was never installed" case; a failing sink
        // is the "RMS is gone / the network is down" case. Neither may touch
        // the match.
        MemoryJournalFile file;
        ManualClock clock;
        SessionStore store;
        store.setClockForTesting(&clock);
        store.setJournalFileForTesting(&file);
        NodeTelemetryService orphan(NodeIdentity::forSettingsFile(ini), nullptr);
        orphan.attachStore(&store);
        orphan.start();

        store.beginSession(qualHeader(QStringLiteral("22222222-0000-4000-8000-00000000bbbb")));
        store.submit(DomainEvent(OfficialMatchStarted{1}));
        const SubmitResult r = store.submit(DomainEvent(ShotAccepted{testjournal::shot(1, 103)}));
        check(r.ok, "a shot is accepted with NO telemetry sink at all");
        check(r.persistedDurably, "...and is still durably journalled");
        check(store.state().officials.size() == 1,
              "...and reaches the authoritative record");
        check(orphan.flushOutbox() == 0, "a sinkless publisher sends nothing");

        Rig failing(ini);
        failing.beginMatch(qualHeader(QStringLiteral("33333333-0000-4000-8000-00000000cccc")));
        failing.sink.failNext = true;
        const SubmitResult r2 =
            failing.store.submit(DomainEvent(ShotAccepted{testjournal::shot(1, 99)}));
        failing.telemetry.flushOutbox();
        check(r2.ok, "a shot is accepted while every telemetry send is FAILING");
        check(failing.store.state().officials.size() == 1,
              "...and still reaches the authoritative record");
        check(failing.telemetry.sendFailures() > 0, "...while the failure is counted");
        check(failing.telemetry.queuedCount() == 0,
              "a failed datagram is dropped, not retried into a growing backlog");
    }

    // ── the outbox is bounded ──────────────────────────────────────────
    {
        Rig rig(ini);
        rig.beginMatch(qualHeader(QStringLiteral("44444444-0000-4000-8000-00000000dddd")));
        // Never flush: simulate RMS being unreachable for a long time while a
        // match runs. Memory must not grow without bound.
        for (int i = 0; i < NodeTelemetryService::kOutboxCapacity * 3; ++i)
            rig.telemetry.publishStatus();
        check(rig.telemetry.queuedCount() <= NodeTelemetryService::kOutboxCapacity,
              "the outbox never exceeds its capacity",
              QString::number(rig.telemetry.queuedCount()));
        check(rig.telemetry.droppedCount() > 0, "the overflow is counted, not silent");
        // The survivors must be the NEWEST — a live range view wants recent
        // observations, not the start of the match.
        rig.sink.clear();
        rig.telemetry.flushOutbox();
        const auto statuses = rig.sink.decodedOf(ta::rms::MessageType::NodeStatus);
        check(!statuses.isEmpty()
                  && statuses.last().status.statusSeq == rig.telemetry.statusSeq(),
              "the newest observation survives the overflow");
    }

    // ── node restart, seen from the node side ──────────────────────────
    {
        Rig first(ini);
        first.beginMatch(qualHeader(QStringLiteral("55555555-0000-4000-8000-00000000eeee")));
        const QString nodeId = first.telemetry.nodeId();
        const QString bootA = first.telemetry.bootId();

        Rig second(ini);   // a second process on the same station
        check(second.telemetry.nodeId() == nodeId,
              "after a restart the station is the SAME node");
        check(second.telemetry.bootId() != bootA,
              "...announced under a NEW boot identity");
        check(second.telemetry.statusSeq() == 1,
              "statusSeq restarts at 1, because it is monotonic per boot");
    }

    // ── the wire is the shared contract, not a private dialect ─────────
    {
        Rig rig(ini);
        rig.beginMatch(qualHeader(QStringLiteral("66666666-0000-4000-8000-00000000ffff")));
        rig.sink.clear();
        rig.store.submit(DomainEvent(ShotAccepted{testjournal::shot(1, 107)}));
        rig.telemetry.flushOutbox();

        int decoded = 0;
        for (const QByteArray& d : rig.sink.sent) {
            const ta::rms::DecodedMessage m = ta::rms::decode(d);
            check(m.type != ta::rms::MessageType::Unknown,
                  "every datagram the node emits decodes with the shared contract",
                  m.rejectReason);
            ++decoded;
        }
        check(decoded > 0, "the node emitted something to check");

        // The node must never emit a message the observer would reject on
        // version grounds, and must never emit a command.
        bool anyCommand = false;
        for (const QByteArray& d : rig.sink.sent)
            if (d.contains("command"))
                anyCommand = true;
        check(!anyCommand, "the node emits no command-shaped message");
    }
}
