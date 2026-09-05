// EXACTLY-ONCE ACROSS A RESTART, AND CONTROL AUTHORITY THAT NOTICES (R2C).
//
// THE SCENARIO EVERY TEST HERE IS ABOUT. RMS sends a command. The node applies
// it. The acknowledgement is lost. RMS cannot tell "it never arrived" from "it
// arrived and I did not hear" - those look identical from where it sits. Then
// the node process restarts.
//
// If a retry re-executes, a START_AT restarts a running match and a paper feed
// feeds twice. If RMS does not retry, an operator's instruction is silently
// dropped. Neither is acceptable, so the retry must happen AND must be
// harmless - which is only possible if the node remembers what it already did
// across the restart, and if RMS reuses the ORIGINAL command id.
//
// The second half of the suite is the boot transition itself: RMS must learn
// about a restart from the node's own telemetry rather than by being refused,
// and must re-establish control by itself.

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

QByteArray key32() { return QByteArray(32, '\x4d'); }
const char* kNode = "TA-NODE-001";

int appliedCount(dev::ControlledNode* n, const char* type)
{ return n->applied().count(QString::fromLatin1(type)); }

// ── §4: duplicate command after a NODE restart ──────────────────────────────
void testLostAckThenNodeRestart(const char* commandType, const QJsonObject& payload,
                                const char* label)
{
    ControlHarness h(key32());
    auto* n = h.addNode(QLatin1String(kNode), QStringLiteral("Lane 1"));
    h.connectAll();
    h.pumpAllStatus(h.now());
    h.coordinator().serviceNodes(&h.monitor(), h.now());

    // A usable clock, so START_AT is not refused for an unrelated reason.
    const qint64 t0 = h.now();
    h.coordinator().measureTimeSync(QLatin1String(kNode), t0, t0 + 8, t0 + 4, t0 + 4);

    // THE ACK IS LOST. The node hears the command and acts on it; RMS does not
    // hear the answer.
    h.setSwallowReplies(QLatin1String(kNode), true);
    const QString X = QStringLiteral("op-intent-1");
    const auto first = h.coordinator().send(QLatin1String(kNode), n->laneId(),
                                            QLatin1String(commandType), payload,
                                            h.now(), X);
    h.setSwallowReplies(QLatin1String(kNode), false);

    check(!first.accepted,
          QStringLiteral("%1: RMS could not confirm the command").arg(QLatin1String(label)));
    check(first.reasonCode == QLatin1String("UNREACHABLE"),
          QStringLiteral("%1: recorded as unconfirmed, not as applied")
              .arg(QLatin1String(label)));
    check(appliedCount(n, commandType) == 1,
          QStringLiteral("%1: the NODE applied it exactly once").arg(QLatin1String(label)));
    check(h.coordinator().pendingCommandCount(QLatin1String(kNode)) == 1,
          QStringLiteral("%1: and RMS is holding it as pending, not forgetting it")
              .arg(QLatin1String(label)));

    // THE NODE PROCESS RESTARTS. Same node, same recovered session, new boot.
    const QString oldBoot = n->bootId();
    n->restart();
    check(n->bootId() != oldBoot,
          QStringLiteral("%1: the boot identity changed").arg(QLatin1String(label)));

    // RMS reauthenticates and retries THE SAME id - which is the only retry the
    // node journal can recognise.
    check(h.coordinator().connectNode(QLatin1String(kNode)),
          QStringLiteral("%1: RMS reauthenticates").arg(QLatin1String(label)));
    const auto retry = h.coordinator().send(QLatin1String(kNode), n->laneId(),
                                            QLatin1String(commandType), payload,
                                            h.now() + 100, X);

    check(retry.accepted,
          QStringLiteral("%1: the retry is answered, not refused").arg(QLatin1String(label)));
    check(appliedCount(n, commandType) == 1,
          QStringLiteral("%1: EXACTLY ONCE - the node did not apply it again")
              .arg(QLatin1String(label)));
    check(h.coordinator().pendingCommandCount(QLatin1String(kNode)) == 0,
          QStringLiteral("%1: and it is no longer pending").arg(QLatin1String(label)));
    check(h.coordinator().semanticDoubleExecutions() == 0,
          QStringLiteral("%1: no audit line claims a second execution")
              .arg(QLatin1String(label)));

    // §6: the original outcome came back, not a bare "already executed".
    bool sawRecovery = false;
    for (const CommandAuditEntry& e : h.coordinator().audit()) {
        if (e.commandId != X) continue;
        if (e.kind == AuditKind::AckRecovered) {
            sawRecovery = true;
            check(e.duplicate,
                  QStringLiteral("%1: the recovery line is marked duplicate")
                      .arg(QLatin1String(label)));
        }
    }
    check(sawRecovery,
          QStringLiteral("%1: the audit records the lost ack being RECOVERED")
              .arg(QLatin1String(label)));
}

void testStartAtAcrossRestart()
{
    testLostAckThenNodeRestart(cmd::kStartAt,
                               QJsonObject{{"startAtUtcMs", 1'700'000'030'000.0},
                                           {"rmsToNodeOffsetMs", 0.0},
                                           {"syncQuality", "GOOD"}},
                               "startat-across-restart");
}
void testStopAcrossRestart()
{
    testLostAckThenNodeRestart(cmd::kStop, QJsonObject{}, "stop-across-restart");
}
void testAssignAcrossRestart()
{
    testLostAckThenNodeRestart(cmd::kAssignAthlete,
                               QJsonObject{{"athlete", "M Bailie"}},
                               "assign-across-restart");
}
void testPrepareAcrossRestart()
{
    testLostAckThenNodeRestart(cmd::kPrepareSession, QJsonObject{},
                               "prepare-across-restart");
}

// ── §5: BOTH sides restart ──────────────────────────────────────────────────
void testBothSidesRestart()
{
    QTemporaryDir dir;
    check(dir.isValid(), "both-restart: temporary directory");
    const QString rmsPath = dir.path() + QStringLiteral("/control_state.json");

    ControlHarness h(key32());
    auto* n = h.addNode(QLatin1String(kNode), QStringLiteral("Lane 1"));
    h.connectAll();
    h.pumpAllStatus(h.now());
    h.coordinator().serviceNodes(&h.monitor(), h.now());

    const QString X = QStringLiteral("op-intent-both");
    h.setSwallowReplies(QLatin1String(kNode), true);
    h.coordinator().send(QLatin1String(kNode), n->laneId(),
                         QLatin1String(cmd::kAssignAthlete),
                         QJsonObject{{"athlete", "K Weber"}}, h.now(), X);
    h.setSwallowReplies(QLatin1String(kNode), false);
    check(appliedCount(n, cmd::kAssignAthlete) == 1,
          "both-restart: the node applied it once");

    // RMS persists what it knows before dying.
    RmsJsonStore rmsStore(rmsPath);
    check(h.coordinator().saveTo(rmsStore).ok, "both-restart: RMS state saved");

    // The NODE dies with only its journal file to come back to.
    const QJsonObject nodeFile = n->journal().saveState();
    n->restartFrom(nodeFile);

    // RMS dies too, and comes back with nothing but its own file.
    RangeControlCoordinator fresh(QStringLiteral("RMS-1"), key32());
    fresh.setLink([&h](const QString& id, const QByteArray& f) {
        return h.node(id)->endpoint().onBytes(f, h.now() + 5000).reply;
    });
    check(fresh.loadFrom(rmsStore).ok, "both-restart: RMS state reloaded");
    check(fresh.pendingCommandCount(QLatin1String(kNode)) == 1,
          "both-restart: RMS remembers an answer is still owed");

    // The recovered RMS reauthenticates and retries the SAME id, from disk.
    check(fresh.connectNode(QLatin1String(kNode)),
          "both-restart: the recovered RMS authenticates to the recovered node");
    const auto retry = fresh.send(QLatin1String(kNode), n->laneId(),
                                  QLatin1String(cmd::kAssignAthlete),
                                  QJsonObject{{"athlete", "K Weber"}},
                                  h.now() + 5000, X);

    check(retry.accepted, "both-restart: the retry is answered");
    check(appliedCount(n, cmd::kAssignAthlete) == 1,
          "both-restart: EXACTLY ONCE across an RMS AND a node restart");
    check(fresh.semanticDoubleExecutions() == 0,
          "both-restart: no audit line claims a second execution");
    check(fresh.pendingCommandCount() == 0, "both-restart: nothing is left owed");
}

// ── §7, §8, §10: boot change invalidates control, automatically ────────────
void testBootChangeInvalidatesAndReauthenticates()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QLatin1String(kNode), QStringLiteral("Lane 1"));
    h.connectAll();
    n->fire(6, h.now());
    h.pumpTelemetry(h.now());
    h.pumpAllStatus(h.now());
    h.coordinator().serviceNodes(&h.monitor(), h.now());

    check(h.coordinator().linkState(QLatin1String(kNode)) == ControlLinkState::Current
              || h.coordinator().isAuthenticated(QLatin1String(kNode)),
          "reauth: the lane starts authenticated");
    const QString athleteBefore = QStringLiteral("unchanged");
    const auto* recBefore = h.monitor().nodeById(QLatin1String(kNode));
    const QString laneBefore = recBefore->laneId;
    const QString sessionBefore = recBefore->sessionId;

    // The node restarts. RMS is told nothing directly - only its telemetry
    // changes, which is exactly how a real range would present it.
    n->restart();
    h.pumpAllStatus(h.now() + 1000);

    const int reauthsBefore = h.coordinator().reauthentications();
    h.coordinator().serviceNodes(&h.monitor(), h.now() + 1000);

    check(h.coordinator().restartsObserved(QLatin1String(kNode)) == 1,
          "reauth: the boot change was DETECTED from telemetry, not from a refusal");
    check(h.coordinator().reauthentications() == reauthsBefore + 1,
          "reauth: control authority was re-established automatically");
    check(h.coordinator().isAuthenticated(QLatin1String(kNode)),
          "reauth: and the lane is authenticated again");
    check(h.coordinator().linkState(QLatin1String(kNode)) == ControlLinkState::Current,
          "reauth: reported as CURRENT");
    check(h.coordinator().observedBootId(QLatin1String(kNode)) == n->bootId(),
          "reauth: RMS now tracks the NEW boot");

    // §8: BOOT IS NOT IDENTITY. Nothing about the lane was recreated.
    check(h.monitor().nodeCount() == 1,
          "reauth: a new boot did NOT create a second node");
    const auto* recAfter = h.monitor().nodeById(QLatin1String(kNode));
    check(recAfter->laneId == laneBefore, "reauth: the lane is unchanged");
    check(recAfter->sessionId == sessionBefore, "reauth: the session is unchanged");
    check(recAfter->ledger.observedCount() == 6,
          "reauth: the six shots already held were not discarded");
    Q_UNUSED(athleteBefore);

    // The audit explains the sequence rather than leaving it to be inferred.
    bool sawRestart = false, sawReauth = false;
    for (const CommandAuditEntry& e : h.coordinator().audit()) {
        if (e.kind == AuditKind::NodeRestart)      sawRestart = true;
        if (e.kind == AuditKind::Reauthentication) sawReauth = true;
    }
    check(sawRestart, "reauth: the audit records NODE RESTART");
    check(sawReauth, "reauth: and REAUTHENTICATION");
}

void testStaleChannelIsRetiredBeforeItIsUsed()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QLatin1String(kNode), QStringLiteral("Lane 1"));
    h.connectAll();
    h.pumpAllStatus(h.now());
    h.coordinator().serviceNodes(&h.monitor(), h.now());
    check(h.coordinator().isAuthenticated(QLatin1String(kNode)),
          "stale: authenticated to start with");

    n->restart();
    h.pumpAllStatus(h.now() + 500);

    // Only the boot notification, without the reconnect: the point being that
    // RMS stops believing in the channel the MOMENT it learns of the restart,
    // not later when something is refused.
    h.coordinator().noteBootIdentity(QLatin1String(kNode), n->bootId(), h.now() + 500);
    check(!h.coordinator().isAuthenticated(QLatin1String(kNode)),
          "stale: the old channel is invalid the moment the boot change is seen");
    check(h.coordinator().linkState(QLatin1String(kNode)) == ControlLinkState::RestartDetected,
          "stale: and is reported as RESTART DETECTED");

    // A command now cannot even be attempted on the dead channel.
    const auto o = h.coordinator().send(QLatin1String(kNode), n->laneId(),
                                        QLatin1String(cmd::kStop), QJsonObject{},
                                        h.now() + 600);
    check(!o.accepted, "stale: a command on a retired channel is not sent");
    check(o.reasonCode == QLatin1String(reason::kNotAuthenticated),
          "stale: refused by RMS itself as NOT_AUTHENTICATED");
    check(!n->applied().contains(QString::fromLatin1(cmd::kStop)),
          "stale: and the node never saw it");
}

void testFirstSightIsNotARestart()
{
    ControlHarness h(key32());
    h.addNode(QLatin1String(kNode), QStringLiteral("Lane 1"));
    h.connectAll();
    h.pumpAllStatus(h.now());
    h.coordinator().serviceNodes(&h.monitor(), h.now());
    check(h.coordinator().restartsObserved(QLatin1String(kNode)) == 0,
          "reauth: meeting a node for the first time is not a restart");

    // And a repeated status with the SAME boot changes nothing.
    h.pumpAllStatus(h.now() + 250);
    h.coordinator().serviceNodes(&h.monitor(), h.now() + 250);
    check(h.coordinator().restartsObserved(QLatin1String(kNode)) == 0,
          "reauth: an unchanged boot is not a restart either");
}

// ── §11: restart AND offline catch-up together ──────────────────────────────
void testReplayAfterRestart()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QLatin1String(kNode), QStringLiteral("Lane 1"));
    h.connectAll();
    h.pumpAllStatus(h.now());
    h.coordinator().serviceNodes(&h.monitor(), h.now());

    n->fire(20, h.now());
    h.pumpTelemetry(h.now());
    h.pumpAllStatus(h.now());
    check(h.monitor().nodeById(QLatin1String(kNode))->ledger.observedCount() == 20,
          "replay-after-restart: twenty shots seen live");
    const QString sessionBefore = n->sessionId();

    // The node goes dark, restarts, recovers its session and shoots five more.
    n->setTelemetryLink(false);
    n->restart();
    n->fire(5, h.now() + 2000);
    n->setTelemetryLink(true);

    // The new boot appears in telemetry. One automatic pass does the rest.
    h.pumpAllStatus(h.now() + 3000);
    h.coordinator().serviceNodes(&h.monitor(), h.now() + 3000);

    const auto* rec = h.monitor().nodeById(QLatin1String(kNode));
    check(rec->ledger.observedCount() == 25,
          "replay-after-restart: the ledger reaches 25");
    check(rec->ledger.missingSequences().isEmpty(),
          "replay-after-restart: with no hole");
    check(rec->unobservedShotCount() == 0,
          "replay-after-restart: and nothing unobserved");
    check(rec->ledger.duplicatesSuppressed() >= 0 && rec->ledger.observedCount() == 25,
          "replay-after-restart: 25 EXACTLY ONCE, not 25 plus repeats");
    check(rec->sessionId == sessionBefore,
          "replay-after-restart: the session was not reset");
    check(h.monitor().nodeCount() == 1,
          "replay-after-restart: and no second lane was invented");
}

// ── §14 / §15: many lanes, many restarts ────────────────────────────────────
void runRestartQualification(int lanes, const char* label)
{
    ControlHarness h(key32());
    for (int i = 1; i <= lanes; ++i)
        h.addNode(QStringLiteral("TA-NODE-%1").arg(i, 3, 10, QChar('0')),
                  QStringLiteral("Lane %1").arg(i), ((i % 5) - 2) * 700);
    check(h.connectAll() == lanes,
          QStringLiteral("%1: all %2 lanes authenticate").arg(QLatin1String(label)).arg(lanes));
    h.pumpAllStatus(h.now());
    h.coordinator().serviceNodes(&h.monitor(), h.now());

    auto nodeAt = [&](int i) {
        return h.node(QStringLiteral("TA-NODE-%1").arg(i, 3, 10, QChar('0')));
    };
    auto idAt = [&](int i) {
        return QStringLiteral("TA-NODE-%1").arg(i, 3, 10, QChar('0'));
    };

    // Everyone shoots twenty.
    for (int i = 1; i <= lanes; ++i) nodeAt(i)->fire(20, h.now());
    h.pumpTelemetry(h.now());
    h.pumpAllStatus(h.now());

    // Lane 1: a command whose ACK is lost, then a restart. (§14 first bullet.)
    const QString lostId = QStringLiteral("lost-ack-lane-1");
    h.setSwallowReplies(idAt(1), true);
    h.coordinator().send(idAt(1), nodeAt(1)->laneId(),
                         QLatin1String(cmd::kAssignAthlete),
                         QJsonObject{{"athlete", "A One"}}, h.now(), lostId);
    h.setSwallowReplies(idAt(1), false);
    nodeAt(1)->restart();

    // Lane 2: offline shots across a restart.
    nodeAt(2)->setTelemetryLink(false);
    nodeAt(2)->fire(15, h.now() + 500);
    nodeAt(2)->restart();
    nodeAt(2)->fire(5, h.now() + 900);
    nodeAt(2)->setTelemetryLink(true);

    // Lane 3: an ordinary reconnect - telemetry blinked, no restart.
    nodeAt(3)->setTelemetryLink(false);
    nodeAt(3)->fire(8, h.now() + 600);
    nodeAt(3)->setTelemetryLink(true);

    // Some further independent restarts across the range, one of them with a
    // pending command outstanding.
    QList<int> restarted{1, 2};
    for (int i = 7; i <= lanes; i += 7) {
        if (i == 1 || i == 2 || i == 3) continue;
        const QString pid = QStringLiteral("pending-lane-%1").arg(i);
        h.setSwallowReplies(idAt(i), true);
        h.coordinator().send(idAt(i), nodeAt(i)->laneId(),
                             QLatin1String(cmd::kPrepareSession), QJsonObject{},
                             h.now(), pid);
        h.setSwallowReplies(idAt(i), false);
        nodeAt(i)->restart();
        restarted.append(i);
    }

    // ONE automatic pass. No operator action anywhere in this scenario.
    h.pumpTelemetry(h.now() + 1000);
    h.pumpAllStatus(h.now() + 1000);
    h.coordinator().serviceNodes(&h.monitor(), h.now() + 1000);

    check(h.coordinator().reauthentications() >= restarted.size(),
          QStringLiteral("%1: every restarted lane was reauthenticated")
              .arg(QLatin1String(label)));

    // A duplicate retry of an id the node already handled, on lane 1.
    const auto dup = h.coordinator().send(idAt(1), nodeAt(1)->laneId(),
                                          QLatin1String(cmd::kAssignAthlete),
                                          QJsonObject{{"athlete", "A One"}},
                                          h.now() + 1100, lostId);
    check(dup.accepted,
          QStringLiteral("%1: the duplicate retry is answered").arg(QLatin1String(label)));
    check(appliedCount(nodeAt(1), cmd::kAssignAthlete) == 1,
          QStringLiteral("%1: lane 1 applied its assignment EXACTLY ONCE")
              .arg(QLatin1String(label)));

    // THE THREE ZEROES.
    check(h.coordinator().semanticDoubleExecutions() == 0,
          QStringLiteral("%1: 0 duplicated semantic command executions")
              .arg(QLatin1String(label)));

    int lost = 0, dupShots = 0, behind = 0;
    for (int i = 1; i <= lanes; ++i) {
        const auto* rec = h.monitor().nodeById(idAt(i));
        if (!rec) { ++lost; continue; }
        const int expected = nodeAt(i)->eventCount();
        if (rec->ledger.observedCount() != expected) ++lost;
        if (!rec->ledger.missingSequences().isEmpty()) ++lost;
        if (rec->ledger.sequenceConflicts() > 0) ++dupShots;
        if (rec->unobservedShotCount() != 0) ++behind;
    }
    check(lost == 0, QStringLiteral("%1: 0 lost shots").arg(QLatin1String(label)));
    check(dupShots == 0, QStringLiteral("%1: 0 duplicate accepted shots").arg(QLatin1String(label)));
    check(behind == 0, QStringLiteral("%1: all %2 lanes reconcile")
                           .arg(QLatin1String(label)).arg(lanes));

    // Lane 2 specifically: forty shots across an offline stretch AND a restart.
    check(h.monitor().nodeById(idAt(2))->ledger.observedCount() == 40,
          QStringLiteral("%1: the offline-plus-restart lane holds all forty")
              .arg(QLatin1String(label)));
    check(h.monitor().nodeById(idAt(3))->ledger.observedCount() == 28,
          QStringLiteral("%1: the ordinary reconnect recovered its eight")
              .arg(QLatin1String(label)));
    check(h.monitor().nodeCount() == lanes,
          QStringLiteral("%1: no restart invented a lane").arg(QLatin1String(label)));
    check(h.coordinator().pendingCommandCount() == 0,
          QStringLiteral("%1: nothing is left owed on any lane").arg(QLatin1String(label)));
}

// ── §11: the physical failure, reproduced deterministically ────────────────
//
// 2026-09-05: a node published official sequences 1..5, RMS received 1, 2, 4, 5
// and reported 1 SHOT NOT OBSERVED. The station totalled 27.7, RMS 20.9. RMS
// detected the gap correctly and could not act on it, because that build had no
// control client.
//
// This is that scenario with the control client attached - the shape the second
// physical test is meant to produce.
void testSingleDatagramLossRecoversAutomatically()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QLatin1String(kNode), QStringLiteral("Lane 1"));
    check(h.connectAll() == 1, "gap: the control channel authenticates");
    h.pumpAllStatus(h.now());
    h.coordinator().serviceNodes(&h.monitor(), h.now());

    // Five official shots; the THIRD datagram never arrives.
    h.dropNthTelemetryDatagram(QLatin1String(kNode), 3);
    n->fire(5, h.now());
    h.pumpTelemetry(h.now());
    h.pumpAllStatus(h.now());
    check(h.droppedDatagrams() == 1, "gap: exactly one datagram was dropped");

    const auto* before = h.monitor().nodeById(QLatin1String(kNode));
    check(before->ledger.observedCount() == 4,
          "gap: RMS holds 4 of 5 - the physical state");
    check(before->shotsAcceptedByNode == 5,
          "gap: while the node reports 5 accepted");
    check(before->unobservedShotCount() == 1,
          "gap: RMS reports 1 SHOT NOT OBSERVED");
    check(before->ledger.missingSequences() == QList<int>{3},
          "gap: and names sequence 3");
    // THE DISTINCTION THAT MATTERS. The highest sequence already reads 5, so a
    // node-count-vs-highest-sequence check would call this lane current. Only
    // the missing-sequence logic finds a MIDDLE hole.
    check(before->ledger.highestSequence() == 5,
          "gap: highestSequence already reads 5 - a tail-only check would miss it");

    // ONE automatic pass. No operator button.
    const int recovered = h.coordinator().serviceNodes(&h.monitor(), h.now() + 1000);
    Q_UNUSED(recovered);

    const auto* after = h.monitor().nodeById(QLatin1String(kNode));
    check(after->ledger.observedCount() == 5,
          "gap: automatic catch-up completes the ledger to 5");
    check(after->unobservedShotCount() == 0, "gap: nothing is unobserved");
    check(after->ledger.missingSequences().isEmpty(), "gap: and no hole remains");
    check(after->ledger.sequenceConflicts() == 0,
          "gap: recovery created no sequence conflict");
    check(h.coordinator().semanticDoubleExecutions() == 0,
          "gap: and no command was executed twice recovering it");

    // Running the pass again must change nothing - an operator or a timer may
    // trigger it repeatedly.
    h.coordinator().serviceNodes(&h.monitor(), h.now() + 2000);
    const auto* stable = h.monitor().nodeById(QLatin1String(kNode));
    check(stable->ledger.observedCount() == 5,
          "gap: a second pass leaves the ledger at 5 - no duplicate shot");
}

void testTwentyLaneRestarts() { runRestartQualification(20, "20-lane restart"); }
void testFiftyLaneRestarts()  { runRestartQualification(50, "50-lane restart"); }

} // namespace

void run_reauth_tests()
{
    std::printf("\n-- exactly-once across restart, and control reauthentication --\n");
    testStartAtAcrossRestart();
    testStopAcrossRestart();
    testAssignAcrossRestart();
    testPrepareAcrossRestart();
    testBothSidesRestart();
    testBootChangeInvalidatesAndReauthenticates();
    testStaleChannelIsRetiredBeforeItIsUsed();
    testFirstSightIsNotARestart();
    testReplayAfterRestart();
    testSingleDatagramLossRecoversAutomatically();
    testTwentyLaneRestarts();
    testFiftyLaneRestarts();
    std::fflush(stdout);
}
