// RMS AUTOMATIC CATCH-UP, RESTART RECOVERY AND THE COMMAND AUDIT (R2B §6-§13).
//
// R2 proved the control channel could carry a replay request. What it did NOT
// prove is the thing a range actually depends on: that when a lane goes quiet
// mid-match and comes back, RMS notices by itself, asks for exactly what it
// missed, and ends up holding every shot the athlete fired - without an
// operator knowing any of it happened.
//
// THE SHAPE OF THE PROOF. Every scenario here fires shots into a simulated
// node that speaks the REAL protocol, cuts something, restores it, and then
// compares RMS's ledger against the node's own history. The node is
// authoritative throughout; RMS is only ever allowed to be a mirror that
// caught up.
//
// WHAT IS DELIBERATELY NOT ASSERTED. Nothing here claims a physical target
// behaved this way. These are software properties of the control plane,
// measured against a simulator - which is why the qualification document
// records them as such and the physical column stays open.

#include "control_harness.h"
#include "test_support.h"

#include "rms/RmsJsonStore.h"

#include <QJsonDocument>
#include <QTemporaryDir>

#include <cstdio>

using namespace ta::rms;
using namespace ta::rms::control;
using ta::rms::test::ControlHarness;

namespace {

QByteArray key32() { return QByteArray(32, '\x71'); }

// ── the ordinary case: nothing is missing, so nothing is asked ───────────────
void testNoGapNoRequest()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    check(h.connectAll() == 1, "catchup: control channel authenticates");

    n->fire(10, h.now());
    h.pumpTelemetry(h.now());
    h.pumpAllStatus(h.now());

    const int before = h.coordinator().replayBatchesReceived();
    const int got = h.coordinator().catchUp(n->nodeId(), &h.monitor(), h.now());
    check(got == 0, "catchup: a current node recovers nothing");
    // The stronger claim: it did not even ASK. A reconciliation pass that
    // requests a replay from every node on every tick would put a fifty-lane
    // range's worth of pointless traffic on the wire during a live match.
    check(h.coordinator().replayBatchesReceived() == before,
          "catchup: a current node is not asked for a replay at all");

    const auto* rec = h.monitor().nodeById(n->nodeId());
    check(rec && rec->ledger.observedCount() == 10,
          "catchup: RMS holds all ten shots without replay");
}

// ── the case the whole feature exists for ────────────────────────────────────
void testOfflineStretchIsRecovered()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.connectAll();

    n->fire(5, h.now());
    h.pumpTelemetry(h.now());
    h.pumpAllStatus(h.now());
    check(h.monitor().nodeById(n->nodeId())->ledger.observedCount() == 5,
          "catchup: five shots observed live");

    // THE LINK DROPS AND THE ATHLETE KEEPS SHOOTING. This is the scenario, and
    // the athlete's record must not depend on the network having been up.
    n->setTelemetryLink(false);
    n->fire(12, h.now() + 1000);
    h.pumpTelemetry(h.now() + 1000);       // nothing arrives; the link is down

    const auto* mid = h.monitor().nodeById(n->nodeId());
    check(mid->ledger.observedCount() == 5,
          "catchup: RMS still holds only five while the link is down");

    // The link comes back. The node's status is what tells RMS it is behind -
    // the twelve missing shots cannot announce their own absence.
    n->setTelemetryLink(true);
    h.pumpAllStatus(h.now() + 2000);
    const auto* behind = h.monitor().nodeById(n->nodeId());
    check(behind->shotsAcceptedByNode == 17,
          "catchup: the node reports seventeen accepted");
    check(behind->unobservedShotCount() == 12,
          "catchup: RMS reports twelve unobserved BEFORE recovering them");

    const int got = h.coordinator().catchUp(n->nodeId(), &h.monitor(), h.now() + 2000);
    check(got == 12, "catchup: exactly the twelve missed shots are recovered");

    const auto* after = h.monitor().nodeById(n->nodeId());
    check(after->ledger.observedCount() == 17,
          "catchup: RMS now holds every shot the node fired");
    check(after->unobservedShotCount() == 0,
          "catchup: nothing is left unobserved");
    check(after->ledger.missingSequences().isEmpty(),
          "catchup: no sequence hole remains");
}

// ── one lost datagram in the middle, which the count alone cannot see ────────
void testMiddleHoleIsRecovered()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.connectAll();

    // Shot #3's datagram is lost, #4 and #5 arrive. The node's accepted count
    // and RMS's HIGHEST SEQUENCE now both read 5 - so a shortfall test alone
    // would declare RMS current while a shot is missing. This is exactly why
    // the gap check looks at holes as well as at the count.
    n->fire(5, h.now());
    int seq = 0;
    for (const QByteArray& d : n->drainTelemetry()) {
        ++seq;
        if (seq == 3) continue;            // the lost datagram
        h.monitor().ingestDatagram(d, h.now());
    }
    h.pumpAllStatus(h.now());

    const auto* before = h.monitor().nodeById(n->nodeId());
    check(before->ledger.observedCount() == 4, "catchup: four of five observed");
    check(before->ledger.highestSequence() == 5,
          "catchup: the highest sequence already reads five");
    check(before->ledger.missingSequences() == QList<int>{3},
          "catchup: shot three is the hole");

    const int got = h.coordinator().catchUp(n->nodeId(), &h.monitor(), h.now());
    check(got == 1, "catchup: the single missing middle shot is recovered");
    check(h.monitor().nodeById(n->nodeId())->ledger.missingSequences().isEmpty(),
          "catchup: the hole is closed");
}

// ── replay must not duplicate: it is safe to run twice ──────────────────────
void testCatchUpIsIdempotent()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.connectAll();

    n->setTelemetryLink(false);
    n->fire(8, h.now());
    n->setTelemetryLink(true);
    h.pumpAllStatus(h.now());

    check(h.coordinator().catchUp(n->nodeId(), &h.monitor(), h.now()) == 8,
          "catchup: eight recovered on the first pass");
    const int held = h.monitor().nodeById(n->nodeId())->ledger.observedCount();

    // A second pass. Because replayed events keep their ORIGINAL eventId, the
    // ledger recognises them and suppresses them - so an operator who
    // reconciles twice cannot inflate a score.
    const int second = h.coordinator().catchUp(n->nodeId(), &h.monitor(), h.now());
    check(second == 0, "catchup: a second pass recovers nothing new");
    check(h.monitor().nodeById(n->nodeId())->ledger.observedCount() == held,
          "catchup: the shot count is unchanged by a repeated pass");
}

// ── a session longer than one batch ─────────────────────────────────────────
void testMultipleBatches()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.connectAll();

    // More than kMaxReplayEvents, so the node cannot answer in one batch and
    // the caller must follow hasMore. A 60-shot match never needs this; a
    // day's training log on one lane does.
    n->setTelemetryLink(false);
    n->fire(kMaxReplayEvents + 47, h.now());
    n->setTelemetryLink(true);
    h.pumpAllStatus(h.now());

    const int got = h.coordinator().catchUp(n->nodeId(), &h.monitor(), h.now());
    check(got == kMaxReplayEvents + 47,
          "catchup: every event is recovered across several batches");
    check(h.coordinator().replayBatchesReceived() >= 2,
          "catchup: it genuinely took more than one batch");
}

// ── automatic: no operator action ───────────────────────────────────────────
void testReconcileAllIsAutomatic()
{
    ControlHarness h(key32());
    for (int i = 1; i <= 6; ++i)
        h.addNode(QStringLiteral("TA-NODE-%1").arg(i, 3, 10, QChar('0')),
                  QStringLiteral("Lane %1").arg(i));
    check(h.connectAll() == 6, "catchup: six lanes authenticate");

    // Three lanes lose telemetry and keep shooting; three stay live.
    for (int i = 1; i <= 6; ++i) {
        auto* n = h.node(QStringLiteral("TA-NODE-%1").arg(i, 3, 10, QChar('0')));
        if (i % 2 == 0) n->setTelemetryLink(false);
        n->fire(9, h.now());
    }
    h.pumpTelemetry(h.now());
    h.pumpAllStatus(h.now());

    // ONE call, no per-lane operator decision.
    const int recovered = h.coordinator().reconcileAll(&h.monitor(), h.now());
    check(recovered == 27, "catchup: reconcileAll recovers all three dark lanes");

    bool allComplete = true;
    for (int i = 0; i < h.monitor().nodeCount(); ++i) {
        const auto* rec = h.monitor().nodeAt(i);
        if (!rec || rec->ledger.observedCount() != 9 || rec->unobservedShotCount() != 0)
            allComplete = false;
    }
    check(allComplete, "catchup: every one of the six lanes holds nine shots");
}

// ── the node's application restarts mid-match ───────────────────────────────
void testRestartRecovery()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.connectAll();

    n->fire(11, h.now());
    h.pumpTelemetry(h.now());
    h.pumpAllStatus(h.now());
    const QString firstBoot = n->bootId();

    // A command applied BEFORE the restart, with an id we will deliberately
    // reuse afterwards.
    const QString reused = QStringLiteral("cmd-across-restart");
    const auto before = h.coordinator().send(n->nodeId(), n->laneId(),
                                             QLatin1String(cmd::kAssignAthlete),
                                             QJsonObject{{"athlete", "A"}},
                                             h.now(), reused);
    check(before.accepted, "restart: the pre-restart command is accepted");

    // The node process restarts: same node, same session, NEW boot. The shots
    // it already took are still its own - it recovered them from its store,
    // which is what the target application's own resume path does.
    n->restart();
    check(n->bootId() != firstBoot, "restart: the boot identity changed");
    check(n->eventCount() == 11, "restart: the node still holds its session");

    // THE STALE CHANNEL. RMS's own client still believes it is authenticated -
    // nothing told it otherwise - but the node behind it is a new process that
    // never saw that handshake. The node REFUSES, which is the behaviour that
    // matters: a command must not be applied on the strength of an
    // authentication the current process never performed.
    check(h.coordinator().isAuthenticated(n->nodeId()),
          "restart: RMS still believes the old channel is authenticated");
    const auto onStale = h.coordinator().send(n->nodeId(), n->laneId(),
                                              QLatin1String(cmd::kStop),
                                              QJsonObject{}, h.now() + 100);
    check(!onStale.accepted, "restart: the node refuses a command on the stale channel");
    check(onStale.reasonCode == QLatin1String(reason::kNotAuthenticated),
          "restart: refused specifically as NOT_AUTHENTICATED");
    check(!n->applied().contains(QString::fromLatin1(cmd::kStop)),
          "restart: and the node did not apply it");

    check(h.coordinator().connectNode(n->nodeId()),
          "restart: RMS re-authenticates to the restarted node");

    n->fire(4, h.now() + 5000);
    h.pumpTelemetry(h.now() + 5000);
    h.pumpAllStatus(h.now() + 5000);

    const auto* rec = h.monitor().nodeById(n->nodeId());
    check(rec->bootId == n->bootId(), "restart: RMS tracks the new boot");
    check(rec->nodeRestarts == 1, "restart: exactly one restart is observed");
    check(rec->ledger.observedCount() == 15,
          "restart: the session continues across the restart, all 15 held");
    check(rec->unobservedShotCount() == 0, "restart: nothing is unobserved after it");

    // AND THE HONEST PART. The restarted process has no memory of the command
    // ids it already handled - that cache lived in the endpoint that died. The
    // SAME command id is therefore applied a SECOND time across a restart.
    // Asserted rather than hidden: it is a real limit of v1, and the
    // qualification document records it as one.
    const int appliedBefore = n->applied().count(QString::fromLatin1(cmd::kAssignAthlete));
    const auto after = h.coordinator().send(n->nodeId(), n->laneId(),
                                            QLatin1String(cmd::kAssignAthlete),
                                            QJsonObject{{"athlete", "A"}},
                                            h.now() + 6000, reused);
    check(after.accepted, "restart: the reused command id is accepted again");
    check(n->applied().count(QString::fromLatin1(cmd::kAssignAthlete)) == appliedBefore + 1,
          "restart: KNOWN LIMIT - it was APPLIED again; idempotency is per boot");

    // Within ONE boot it is still suppressed, which is what R2 proved and what
    // makes an ordinary retry safe.
    const int sameBoot = n->applied().count(QString::fromLatin1(cmd::kAssignAthlete));
    h.coordinator().send(n->nodeId(), n->laneId(),
                         QLatin1String(cmd::kAssignAthlete),
                         QJsonObject{{"athlete", "A"}}, h.now() + 6001, reused);
    check(n->applied().count(QString::fromLatin1(cmd::kAssignAthlete)) == sameBoot,
          "restart: within one boot the repeat is still not re-applied");
}

// ── the watermark survives an RMS crash ─────────────────────────────────────
void testWatermarkPersistsAcrossRmsRestart()
{
    QTemporaryDir dir;
    check(dir.isValid(), "watermark: temporary directory");
    const QString path = dir.path() + QStringLiteral("/control_state.json");

    QString sessionId;
    {
        ControlHarness h(key32());
        auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
        h.connectAll();
        n->fire(20, h.now());
        h.pumpTelemetry(h.now());
        h.pumpAllStatus(h.now());
        h.coordinator().catchUp(n->nodeId(), &h.monitor(), h.now());
        sessionId = n->sessionId();

        h.coordinator().send(n->nodeId(), n->laneId(),
                             QLatin1String(cmd::kPrepareSession), QJsonObject{},
                             h.now());

        RmsJsonStore store(path);
        const StoreResult r = h.coordinator().saveTo(store);
        check(r.ok, "watermark: state saved", r.detail);
    }

    // A NEW coordinator, as after an RMS crash and relaunch.
    {
        RangeControlCoordinator fresh(QStringLiteral("RMS-1"), key32());
        RmsJsonStore store(path);
        const StoreResult r = fresh.loadFrom(store);
        check(r.ok, "watermark: state reloaded", r.detail);

        const ReconciliationWatermark w = fresh.watermark(QStringLiteral("TA-NODE-001"));
        check(w.nodeId == QLatin1String("TA-NODE-001"), "watermark: node identity survived");
        check(w.sessionId == sessionId, "watermark: session identity survived");
        check(w.highestSequence == 20, "watermark: reconciled position survived");
        check(!w.lastBootId.isEmpty(), "watermark: the boot it applied to survived");

        // The audit came back with it, and carries no secret.
        const auto audit = fresh.audit();
        check(audit.size() == 1, "audit: the one state-changing command survived");
        check(audit.first().commandType == QLatin1String(cmd::kPrepareSession),
              "audit: it is the PREPARE_SESSION");
        check(audit.first().accepted, "audit: recorded as accepted");
    }

    // A document from a NEWER RMS is refused rather than half-read.
    {
        RmsJsonStore store(path);
        QJsonObject doc;
        const StoreResult r = store.load(0, &doc);
        check(!r.ok && r.error == StoreError::SchemaTooNew,
              "watermark: an older build refuses a newer document");
    }
}

// ── the audit records refusals too ──────────────────────────────────────────
void testAuditRecordsFailures()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.connectAll();

    h.coordinator().send(n->nodeId(), n->laneId(),
                         QLatin1String(cmd::kPrepareSession), QJsonObject{}, h.now());
    // Diagnostics change nothing and are not recorded: an audit buried under
    // heartbeats is an audit nobody reads.
    h.coordinator().send(n->nodeId(), n->laneId(),
                         QLatin1String(cmd::kPing), QJsonObject{}, h.now());
    h.coordinator().send(n->nodeId(), n->laneId(),
                         QLatin1String(cmd::kRequestStatus), QJsonObject{}, h.now());

    // The control link dies. The command CANNOT have been applied, and the
    // audit must say so rather than record an optimistic success.
    h.setControlLink(n->nodeId(), false);
    const auto lost = h.coordinator().send(n->nodeId(), n->laneId(),
                                           QLatin1String(cmd::kStop), QJsonObject{},
                                           h.now() + 100);
    check(!lost.accepted, "audit: an undeliverable command is not accepted");
    check(lost.reasonCode == QLatin1String("UNREACHABLE"),
          "audit: it is recorded as unreachable, not as a refusal by the node");

    const auto a = h.coordinator().audit();
    check(a.size() == 2, "audit: two state-changing commands, no diagnostics");
    check(a.at(0).accepted && a.at(1).accepted == false,
          "audit: one accepted, one failed");

    // No secret material, by construction and by assertion.
    const QJsonObject doc = h.coordinator().saveState();
    const QByteArray raw = QJsonDocument(doc).toJson(QJsonDocument::Compact);
    check(!raw.contains("mac") && !raw.contains("nonce") && !raw.contains("key"),
          "audit: the persisted audit contains no mac, nonce or key");
    check(!raw.contains(key32()), "audit: the range key is not in the document");
}

} // namespace

void run_catchup_tests()
{
    std::printf("\n-- catch-up, restart recovery and audit --\n");
    testNoGapNoRequest();
    testOfflineStretchIsRecovered();
    testMiddleHoleIsRecovered();
    testCatchUpIsIdempotent();
    testMultipleBatches();
    testReconcileAllIsAutomatic();
    testRestartRecovery();
    testWatermarkPersistsAcrossRmsRestart();
    testAuditRecordsFailures();
    std::fflush(stdout);
}
