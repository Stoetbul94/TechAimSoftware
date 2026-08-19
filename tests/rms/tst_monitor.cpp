// The read-only observer's behaviour under real network conditions:
// discovery, heartbeats, duplicates, reordering, node loss, node return,
// node restart and RMS restart.
//
// Time is injected everywhere, so none of this sleeps and none of it is
// timing-dependent.

#include "test_support.h"

#include "rms/RangeListModel.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"

#include <cstdio>

using namespace ta::rms;

namespace {

// A signal counter, so the harness stays QT = core network. QSignalSpy would
// pull in testlib for the sake of an integer.
struct SignalCounter {
    int count = 0;
    template <typename Sender, typename Signal>
    SignalCounter(Sender* s, Signal sig)
    {
        QObject::connect(s, sig, [this] { ++count; });
    }
};

const char* kNode = "TA-NODE-001";
const char* kBootA = "boot-a";
const char* kSession = "sess-1";

QByteArray announce(const char* boot = kBootA, const char* lane = "Lane 1")
{
    NodeAnnounce a;
    a.nodeId = QLatin1String(kNode);
    a.bootId = QLatin1String(boot);
    a.laneId = QLatin1String(lane);
    a.deviceIdentity = QStringLiteral("TechAim-EST/4100");
    a.appVersion = QStringLiteral("0.9.0");
    a.productIdentity = QStringLiteral("Tech Aim");
    return encode(a);
}

QByteArray status(int shots, quint64 seq, double total = 0.0,
                  const char* boot = kBootA,
                  MatchPhase phase = MatchPhase::Match,
                  const char* session = kSession)
{
    NodeStatus s;
    s.nodeId           = QLatin1String(kNode);
    s.bootId           = QLatin1String(boot);
    s.laneId           = QStringLiteral("Lane 1");
    s.sessionId        = QLatin1String(session);
    s.programmeId      = QStringLiteral("issf.10m.air-rifle.qualification60");
    s.rulesetId        = QStringLiteral("issf");
    s.targetStandardId = QStringLiteral("issf.10m.air-rifle");
    s.athleteName      = QStringLiteral("A. Bailie");
    s.connection       = ConnectionState::TargetConnected;
    s.phase            = phase;
    s.shotsAccepted    = shots;
    s.shotsExpected    = 60;
    s.totalScore       = total;
    s.health           = QStringLiteral("OK");
    s.statusSeq        = seq;
    return encode(s);
}

QByteArray shot(int seq, const char* eventId, double score = 10.0,
                const char* boot = kBootA, const char* session = kSession)
{
    AcceptedShot sh;
    sh.eventId      = QLatin1String(eventId);
    sh.nodeId       = QLatin1String(kNode);
    sh.bootId       = QLatin1String(boot);
    sh.laneId       = QStringLiteral("Lane 1");
    sh.sessionId    = QLatin1String(session);
    sh.programmeId  = QStringLiteral("issf.10m.air-rifle.qualification60");
    sh.shotSequence = seq;
    sh.rawXMm       = 0.5;
    sh.rawYMm       = -0.5;
    sh.authoritativeScore = score;
    sh.integerScore = int(score);
    sh.acquisitionStatus = QStringLiteral("ACCEPTED");
    return encode(sh);
}

} // namespace

void run_monitor_tests()
{
    std::printf("\n-- discovery and heartbeat --\n");
    {
        RangeMonitor m;
        SignalCounter added(&m, &RangeMonitor::nodeAdded);

        check(m.nodeCount() == 0, "a fresh observer knows about no nodes");

        m.ingestDatagram(announce(), 1000);
        check(m.nodeCount() == 1, "an announce discovers a node");
        check(added.count == 1, "discovery emits nodeAdded exactly once");

        m.ingestDatagram(announce(), 2000);
        check(m.nodeCount() == 1, "a repeated announce does not duplicate the node");
        check(added.count == 1, "...and does not re-announce it");

        const TargetNodeRecord* r = m.nodeById(QLatin1String(kNode));
        check(r != nullptr && r->laneId == QLatin1String("Lane 1"),
              "the node's lane assignment is observed");
        check(r && r->deviceIdentity == QLatin1String("TechAim-EST/4100"),
              "device identity is observed separately from node identity");

        m.ingestDatagram(status(17, 1, 164.2), 3000);
        r = m.nodeById(QLatin1String(kNode));
        check(r && r->shotsAcceptedByNode == 17, "heartbeat carries the node's shot count");
        check(r && qAbs(r->totalScoreByNode - 164.2) < 1e-9,
              "heartbeat carries the node's authoritative total");
        check(r && r->phase == MatchPhase::Match, "heartbeat carries the match phase");
        check(r && r->programmeId == QLatin1String("issf.10m.air-rifle.qualification60"),
              "heartbeat carries the stable programmeId");

        // A late heartbeat must not drag the lane backwards.
        m.ingestDatagram(status(3, 1, 30.0), 3100);
        r = m.nodeById(QLatin1String(kNode));
        check(r && r->shotsAcceptedByNode == 17,
              "a stale heartbeat (lower statusSeq) is dropped, not applied");
        check(r && r->staleStatusDropped == 1, "the dropped stale heartbeat is counted");

        m.ingestDatagram(status(21, 2, 201.0), 3200);
        r = m.nodeById(QLatin1String(kNode));
        check(r && r->shotsAcceptedByNode == 21, "a newer heartbeat is applied");
    }

    std::printf("\n-- accepted shots --\n");
    {
        RangeMonitor m;
        SignalCounter observed(&m, &RangeMonitor::shotObserved);
        m.ingestDatagram(announce(), 0);
        m.ingestDatagram(shot(1, "e1", 10.4), 100);
        m.ingestDatagram(shot(2, "e2", 9.8), 200);

        const TargetNodeRecord* r = m.nodeById(QLatin1String(kNode));
        check(r && r->ledger.observedCount() == 2, "accepted shots are recorded");
        check(observed.count == 2, "each new shot is signalled once");
        check(r && qAbs(r->ledger.observedScoreSum() - 20.2) < 1e-9,
              "observed shot scores are summed AS REPORTED - never recomputed");
        check(r && r->ledger.latestReceived().shotSequence == 2,
              "the latest received shot is available for the live view");
    }

    std::printf("\n-- duplicate suppression --\n");
    {
        RangeMonitor m;
        m.ingestDatagram(announce(), 0);
        for (int i = 1; i <= 17; ++i)
            m.ingestDatagram(shot(i, qPrintable(QStringLiteral("e%1").arg(i))), 100 + i);

        const QByteArray dup = shot(17, "e17");
        m.ingestDatagram(dup, 300);
        m.ingestDatagram(dup, 301);   // UDP really does this

        const TargetNodeRecord* r = m.nodeById(QLatin1String(kNode));
        check(r && r->ledger.observedCount() == 17,
              "shot #17 delivered three times is held exactly once");
        check(r && r->ledger.duplicatesSuppressed() == 2,
              "both duplicate deliveries are counted, not silently dropped");
        check(r && r->ledger.highestSequence() == 17,
              "duplicates do not advance the sequence high-water mark");

        // Same sequence, different eventId: a protocol violation, and the
        // first observation must win.
        m.ingestDatagram(shot(17, "e17-conflict", 5.0), 400);
        r = m.nodeById(QLatin1String(kNode));
        check(r && r->ledger.sequenceConflicts() == 1,
              "a second shot claiming sequence #17 is flagged as a conflict");
        check(r && r->ledger.observedCount() == 17, "...and does not create a second entry");
        check(r && qAbs(r->ledger.shotsInOrder().at(16).authoritativeScore - 10.0) < 1e-9,
              "...and does not overwrite the score already observed for #17");
    }

    std::printf("\n-- out-of-order arrival --\n");
    {
        RangeMonitor m;
        m.ingestDatagram(announce(), 0);
        m.ingestDatagram(shot(16, "e16"), 100);
        m.ingestDatagram(shot(18, "e18"), 200);   // #18 before #17

        const TargetNodeRecord* r = m.nodeById(QLatin1String(kNode));
        check(r && r->ledger.missingSequences() == QList<int>({1,2,3,4,5,6,7,8,9,10,
                                                               11,12,13,14,15,17}),
              "a sequence not yet received is reported as a GAP, not assumed absent");

        m.ingestDatagram(shot(17, "e17"), 300);   // the late one turns up
        r = m.nodeById(QLatin1String(kNode));
        check(r && r->ledger.outOfOrderAccepted() == 1, "late arrival is accepted and counted");

        const QList<AcceptedShot> ordered = r->ledger.shotsInOrder();
        check(ordered.size() == 3, "all three shots are held");
        check(ordered.at(0).shotSequence == 16
              && ordered.at(1).shotSequence == 17
              && ordered.at(2).shotSequence == 18,
              "the display order is SEQUENCE order, not arrival order");
        check(r && r->ledger.highestSequence() == 18,
              "a late lower sequence does not pull the high-water mark back");
    }

    std::printf("\n-- node loss and return --\n");
    {
        RangeMonitor m;
        m.setOfflineTimeoutMs(6000);
        m.ingestDatagram(announce(), 0);
        m.ingestDatagram(status(10, 1, 98.0), 1000);
        for (int i = 1; i <= 10; ++i)
            m.ingestDatagram(shot(i, qPrintable(QStringLiteral("e%1").arg(i))), 1000 + i);

        m.evaluateLiveness(5000);
        const TargetNodeRecord* r = m.nodeById(QLatin1String(kNode));
        check(r && !r->isOffline(), "a node heard from recently is not marked offline");

        m.evaluateLiveness(20000);
        r = m.nodeById(QLatin1String(kNode));
        check(r && r->isOffline(), "a silent node is marked OFFLINE by RMS");
        check(r && r->offlineEpisodes == 1, "the offline episode is counted");
        check(m.nodeCount() == 1, "an offline node stays on the range - it is not deleted");
        check(r && r->ledger.observedCount() == 10,
              "going offline does NOT discard what RMS observed");
        check(r && r->shotsAcceptedByNode == 10,
              "the node's last authoritative state is retained while it is away");

        m.evaluateLiveness(21000);
        r = m.nodeById(QLatin1String(kNode));
        check(r && r->offlineEpisodes == 1, "staying offline does not re-count the episode");

        // ── the node returns, same process, same session ────────────────
        m.ingestDatagram(status(24, 5, 233.1), 30000);
        r = m.nodeById(QLatin1String(kNode));
        check(r && !r->isOffline(), "the returning node is live again");
        check(r && r->shotsAcceptedByNode == 24,
              "RMS continues from the NODE's authoritative count, not its own");
        check(r && r->ledger.observedCount() == 10,
              "RMS still holds only the 10 shots it actually saw");
        check(r && r->unobservedShotCount() == 14,
              "the 14 shots fired while RMS was away are reported as UNOBSERVED, not lost silently");
        check(r && r->nodeRestarts == 0,
              "a network dropout is not mistaken for a node restart");
    }

    std::printf("\n-- node restart --\n");
    {
        RangeMonitor m;
        m.ingestDatagram(announce(kBootA), 0);
        m.ingestDatagram(status(10, 7, 98.0), 100);
        m.ingestDatagram(shot(1, "e1"), 200);

        // Same nodeId, NEW bootId: the node's application restarted.
        m.ingestDatagram(announce("boot-b"), 5000);
        const TargetNodeRecord* r = m.nodeById(QLatin1String(kNode));
        check(m.nodeCount() == 1, "a restarted node is the SAME node, not a new lane");
        check(r && r->nodeRestarts == 1, "the restart is detected via bootId");
        check(r && r->lastStatusSeq == 0,
              "statusSeq is per-boot, so it is reset - a restart must not look stale");

        // statusSeq 1 after a restart is NEWER than statusSeq 7 before it.
        m.ingestDatagram(status(3, 1, 29.0, "boot-b"), 5100);
        r = m.nodeById(QLatin1String(kNode));
        check(r && r->shotsAcceptedByNode == 3,
              "the first heartbeat after a restart is applied, not discarded as stale");

        // A straggler from the OLD boot, delivered late. Datagrams do not
        // stop in flight when an application restarts.
        const IngestOutcome late = m.ingestDatagram(status(10, 8, 98.0, kBootA), 5200);
        r = m.nodeById(QLatin1String(kNode));
        check(late.staleBoot, "a datagram from a superseded boot is identified as stale");
        check(r && r->nodeRestarts == 1,
              "...and is NOT counted as a second restart");
        check(r && r->staleBootDropped == 1, "...and the drop is counted");
        check(r && r->bootId == QLatin1String("boot-b"),
              "...and does not drag the node back to its previous boot identity");
        check(r && r->shotsAcceptedByNode == 3,
              "...and cannot overwrite the current run's state with the old run's");

        // The guard must not block a genuine SECOND restart.
        m.ingestDatagram(announce("boot-c"), 6000);
        r = m.nodeById(QLatin1String(kNode));
        check(r && r->nodeRestarts == 2, "a genuinely new bootId is still a restart");
    }

    std::printf("\n-- new session on the node --\n");
    {
        RangeMonitor m;
        m.ingestDatagram(announce(), 0);
        for (int i = 1; i <= 5; ++i)
            m.ingestDatagram(shot(i, qPrintable(QStringLiteral("e%1").arg(i))), 100 + i);

        m.ingestDatagram(shot(1, "f1", 9.1, kBootA, "sess-2"), 500);
        const TargetNodeRecord* r = m.nodeById(QLatin1String(kNode));
        check(r && r->ledger.sessionId() == QLatin1String("sess-2"),
              "a shot from a new sessionId re-bases the ledger");
        check(r && r->ledger.observedCount() == 1, "the new session starts from its own shot 1");
        check(r && r->ledger.highestSequence() == 1,
              "the previous session's sequence numbers do not leak into the new one");
    }

    std::printf("\n-- RMS restart --\n");
    {
        RangeMonitor m;
        m.ingestDatagram(announce(), 0);
        m.ingestDatagram(status(30, 3, 291.4), 1000);
        for (int i = 1; i <= 30; ++i)
            m.ingestDatagram(shot(i, qPrintable(QStringLiteral("e%1").arg(i))), 1000 + i);

        RangeListModel model(&m);
        // The model was constructed after the fact; drive it from the monitor
        // the way a fresh RMS process would be driven — by observing again.
        m.reset();
        check(m.nodeCount() == 0, "an RMS restart drops everything RMS knew");
        check(model.rowCountProperty() == 0, "...and the dashboard is empty");

        // The node is untouched and keeps broadcasting. RMS rebuilds purely
        // from what it now observes.
        m.ingestDatagram(announce(), 40000);
        m.ingestDatagram(status(41, 9, 398.7), 40100);
        m.ingestDatagram(shot(41, "e41", 10.2), 40200);

        const TargetNodeRecord* r = m.nodeById(QLatin1String(kNode));
        check(m.nodeCount() == 1, "the node reappears from its own broadcasts alone");
        check(model.rowCountProperty() == 1, "the dashboard rebuilds itself");
        check(r && r->shotsAcceptedByNode == 41,
              "the NODE's count is authoritative after an RMS restart");
        check(r && r->ledger.observedCount() == 1, "RMS holds only the shot it saw since restarting");
        check(r && r->unobservedShotCount() == 40,
              "the 40 shots RMS missed are declared unobserved - an honest dashboard, not a wrong one");
    }

    std::printf("\n-- rejection accounting --\n");
    {
        RangeMonitor m;
        SignalCounter rejected(&m, &RangeMonitor::datagramRejected);
        m.ingestDatagram(QByteArray("garbage"), 0);
        m.ingestDatagram(QByteArray("{\"protocolVersion\":99,\"type\":\"node.status\","
                                    "\"nodeId\":\"n\",\"bootId\":\"b\"}"), 0);
        check(m.rejectedDatagrams() == 2, "rejected datagrams are counted for the operator");
        check(rejected.count == 2, "each rejection is signalled with a reason");
        check(m.nodeCount() == 0, "a rejected datagram never creates a lane");
    }

    std::printf("\n-- dashboard model --\n");
    {
        RangeMonitor m;
        RangeListModel model(&m);
        check(model.readOnly(), "the model declares itself READ-ONLY to the UI");

        m.ingestDatagram(announce(), 0);
        m.ingestDatagram(status(17, 1, 164.2), 100);
        for (int i = 1; i <= 17; ++i)
            m.ingestDatagram(shot(i, qPrintable(QStringLiteral("e%1").arg(i)), 9.6), 100 + i);

        check(model.rowCountProperty() == 1, "the dashboard shows the observed lane");
        const QModelIndex idx = model.index(0, 0);
        check(model.data(idx, RangeListModel::LaneLabelRole).toString()
                  == QLatin1String("Lane 1"), "lane column");
        check(model.data(idx, RangeListModel::ShotsLabelRole).toString()
                  == QLatin1String("17/60"), "shots column reads 17/60");
        check(model.data(idx, RangeListModel::ScoreLabelRole).toString()
                  == QLatin1String("164.2"),
              "score column shows the NODE's total, formatted only");
        // QStringLiteral, not QLatin1String: the separator is a multi-byte
        // UTF-8 character and Latin-1 would compare it as two.
        check(model.data(idx, RangeListModel::ProgrammeLabelRole).toString()
                  == QStringLiteral("10 m Air Rifle · Qualification 60"),
              "programme column is derived from the stable id");
        check(model.data(idx, RangeListModel::OfficialRole).toBool(),
              "an issf ruleset is shown as an official course");
        check(model.data(idx, RangeListModel::PhaseRole).toString()
                  == QLatin1String("MATCH"), "phase column");
        check(!model.data(idx, RangeListModel::OfflineRole).toBool(), "lane is not offline");
        check(model.onlineCount() == 1 && model.offlineCount() == 0, "summary counts");

        const QVariantMap detail = model.nodeDetail(0);
        check(detail.value(QStringLiteral("latestSequence")).toInt() == 17,
              "the detail pane shows the latest shot sequence");
        check(detail.value(QStringLiteral("observedShots")).toInt() == 17,
              "the detail pane separates observed from node-accepted");
        check(model.recentShots(0, 5).size() == 5, "the recent-shot list is bounded");
        check(model.recentShots(0, 5).first().toMap()
                  .value(QStringLiteral("sequence")).toInt() == 17,
              "the recent-shot list is newest first");
        check(model.nodeDetail(99).isEmpty(), "an out-of-range row yields nothing, not a crash");

        m.evaluateLiveness(999999);
        check(model.data(idx, RangeListModel::OfflineRole).toBool(),
              "the dashboard reflects the offline conclusion");
        check(model.offlineCount() == 1, "summary counts follow");
    }
}
