// TWENTY AND FIFTY LANES UNDER FULL CONTROL (R2B §21-§27).
//
// R1 qualified fifty lanes of READ-ONLY telemetry. That said nothing about
// what happens when RMS also has to authenticate fifty channels, address fifty
// nodes individually, start them together and reconcile the ones that fell
// behind - which is the load a real range actually puts on it.
//
// THE TWO SIZES ARE DELIBERATE. Twenty is a large club range and fifty is the
// declared ceiling; a system that only works at the size it was developed at
// has not been qualified, it has been demonstrated.
//
// THE RULE THIS SUITE ENFORCES. A fan-out NEVER collapses to a single
// boolean. "Started 48 of 50" must be visible as 48 and 2, with the two named,
// because an operator told "all started" while two lanes sit idle has been
// misinformed at the one moment it costs a match.
//
// SCOPE. This is a software measurement on one machine with a direct in-process
// link. It proves the protocol, the addressing and the bookkeeping hold at
// fifty; it says NOTHING about wire timing, radio conditions or real hardware,
// and no timing claim may be drawn from it.

#include "control_harness.h"
#include "test_support.h"

#include <QSet>

#include <cstdio>

using namespace ta::rms;
using namespace ta::rms::control;
using ta::rms::test::ControlHarness;

namespace {

QByteArray key32() { return QByteArray(32, '\x3e'); }

QString laneNode(int i)
{ return QStringLiteral("TA-NODE-%1").arg(i, 3, 10, QChar('0')); }

// Builds a range of `count` lanes, each with its own session and its own clock
// offset, all authenticated.
void buildRange(ControlHarness& h, int count)
{
    for (int i = 1; i <= count; ++i) {
        // A different offset per lane, including negatives: a bug that assumed
        // one shared clock would pass a range where every lane agreed.
        const qint64 offset = ((i % 7) - 3) * 1100 + i;
        h.addNode(laneNode(i), QStringLiteral("Lane %1").arg(i), offset);
    }
}

void runFullControlAt(int count, const char* label)
{
    ControlHarness h(key32());
    buildRange(h, count);

    check(h.connectAll() == count,
          QStringLiteral("%1: all %2 control channels authenticate")
              .arg(QLatin1String(label)).arg(count));

    // Every lane addressed INDIVIDUALLY - a distinct command id each, which is
    // what makes a per-lane outcome possible at all.
    QSet<QString> commandIds;
    FanOutResult prepare = h.coordinator().sendToMany(
        h.nodeIds(), QLatin1String(cmd::kPrepareSession), QJsonObject{}, h.now());
    for (const CommandOutcome& o : prepare.outcomes)
        commandIds.insert(o.commandId);
    check(prepare.accepted() == count,
          QStringLiteral("%1: every lane accepts PREPARE_SESSION").arg(QLatin1String(label)));
    check(commandIds.size() == count,
          QStringLiteral("%1: every lane got its OWN command id, not a broadcast")
              .arg(QLatin1String(label)));

    // Athlete assignment, per lane, with per-lane content.
    int assigned = 0;
    for (int i = 1; i <= count; ++i) {
        const auto o = h.coordinator().send(
            laneNode(i), QStringLiteral("Lane %1").arg(i),
            QLatin1String(cmd::kAssignAthlete),
            QJsonObject{{"athlete", QStringLiteral("Athlete %1").arg(i)}}, h.now());
        if (o.accepted) ++assigned;
    }
    check(assigned == count,
          QStringLiteral("%1: every lane accepts its athlete").arg(QLatin1String(label)));

    // Time sync for every lane, on a symmetric path, then ONE scheduled start.
    for (int i = 1; i <= count; ++i) {
        const qint64 off = h.node(laneNode(i))->clockOffsetMs();
        const qint64 t0 = h.now();
        // A symmetric 8 ms path with no processing delay, so the estimate is
        // exact and the assertion below is about the SCHEDULING, not about
        // rounding in the measurement.
        h.coordinator().measureTimeSync(laneNode(i), t0, t0 + 8,
                                        t0 + 4 + off, t0 + 4 + off);
    }
    const qint64 startAt = h.now() + 60000;
    const FanOutResult start = h.coordinator().startAt(h.nodeIds(), startAt, h.now());
    check(start.allAccepted(),
          QStringLiteral("%1: all %2 lanes accept the scheduled start")
              .arg(QLatin1String(label)).arg(count));

    bool together = true;
    for (int i = 1; i <= count; ++i) {
        const auto* n = h.node(laneNode(i));
        if (n->scheduledStartNodeMs() - n->clockOffsetMs() != startAt) together = false;
    }
    check(together,
          QStringLiteral("%1: all %2 lanes scheduled the same real instant")
              .arg(QLatin1String(label)).arg(count));

    // A match. Every fourth lane loses telemetry part-way and keeps shooting.
    for (int i = 1; i <= count; ++i)
        h.node(laneNode(i))->fire(20, h.now());
    h.pumpTelemetry(h.now());

    int darkLanes = 0;
    for (int i = 1; i <= count; ++i) {
        if (i % 4 != 0) continue;
        ++darkLanes;
        h.node(laneNode(i))->setTelemetryLink(false);
    }
    for (int i = 1; i <= count; ++i)
        h.node(laneNode(i))->fire(40, h.now() + 1000);
    h.pumpTelemetry(h.now() + 1000);
    for (int i = 1; i <= count; ++i)
        h.node(laneNode(i))->setTelemetryLink(true);
    h.pumpAllStatus(h.now() + 2000);

    // RMS must KNOW it is behind before it recovers anything.
    int behind = 0, unobserved = 0;
    for (int i = 0; i < h.monitor().nodeCount(); ++i) {
        const auto* rec = h.monitor().nodeAt(i);
        if (rec->unobservedShotCount() > 0) { ++behind; unobserved += rec->unobservedShotCount(); }
    }
    check(behind == darkLanes,
          QStringLiteral("%1: RMS identifies exactly the %2 lanes that went dark")
              .arg(QLatin1String(label)).arg(darkLanes));
    check(unobserved == darkLanes * 40,
          QStringLiteral("%1: and exactly how many shots it is missing").arg(QLatin1String(label)));

    const int recovered = h.coordinator().reconcileAll(&h.monitor(), h.now() + 2000);
    check(recovered == darkLanes * 40,
          QStringLiteral("%1: one automatic pass recovers all %2 missed shots")
              .arg(QLatin1String(label)).arg(darkLanes * 40));

    bool complete = true;
    for (int i = 0; i < h.monitor().nodeCount(); ++i) {
        const auto* rec = h.monitor().nodeAt(i);
        if (rec->ledger.observedCount() != 60 || !rec->ledger.missingSequences().isEmpty()
            || rec->unobservedShotCount() != 0)
            complete = false;
    }
    check(complete,
          QStringLiteral("%1: every one of the %2 lanes holds a complete 60-shot record")
              .arg(QLatin1String(label)).arg(count));

    // The audit accounts for every state-changing command and no diagnostics.
    const int expectedAudit = count      // PREPARE_SESSION
                              + count    // ASSIGN_ATHLETE
                              + count;   // START_AT
    check(h.coordinator().audit().size() == expectedAudit,
          QStringLiteral("%1: the audit holds all %2 state-changing commands")
              .arg(QLatin1String(label)).arg(expectedAudit));
}

// ── partial failure is REPORTED, never averaged away ────────────────────────
void testPartialFailureIsNamed()
{
    ControlHarness h(key32());
    buildRange(h, 20);
    h.connectAll();

    // Three lanes lose their control link. Not their telemetry - a range can
    // lose one and keep the other, and the operator needs to be told which.
    const QStringList dead{laneNode(3), laneNode(11), laneNode(20)};
    for (const QString& id : dead)
        h.setControlLink(id, false);

    const FanOutResult r = h.coordinator().sendToMany(
        h.nodeIds(), QLatin1String(cmd::kPrepareSession), QJsonObject{}, h.now());

    check(!r.allAccepted(), "partial: the fan-out does not report success");
    check(r.accepted() == 17 && r.failed() == 3,
          "partial: 17 accepted and 3 failed, counted separately");

    QSet<QString> named;
    for (const CommandOutcome& o : r.failures()) named.insert(o.nodeId);
    check(named == QSet<QString>(dead.begin(), dead.end()),
          "partial: the failures name exactly the three unreachable lanes");

    bool allUnreachable = true;
    for (const CommandOutcome& o : r.failures())
        if (o.reasonCode != QLatin1String("UNREACHABLE")) allUnreachable = false;
    check(allUnreachable,
          "partial: reported as unreachable - not as a refusal the node never made");

    // And the seventeen that worked genuinely applied it.
    int applied = 0;
    for (int i = 1; i <= 20; ++i)
        if (h.node(laneNode(i))->applied().contains(QString::fromLatin1(cmd::kPrepareSession)))
            ++applied;
    check(applied == 17, "partial: seventeen lanes actually applied the command");

    // The three failures are in the audit as failures.
    int failedEntries = 0;
    for (const CommandAuditEntry& e : h.coordinator().audit())
        if (!e.accepted) ++failedEntries;
    check(failedEntries == 3, "partial: all three failures are recorded in the audit");
}

// ── one lane's refusal must not touch its neighbours ────────────────────────
void testOneRefusalDoesNotAffectOthers()
{
    ControlHarness h(key32());
    buildRange(h, 20);
    h.connectAll();
    for (int i = 1; i <= 20; ++i) {
        const qint64 off = h.node(laneNode(i))->clockOffsetMs();
        const qint64 t0 = h.now();
        h.coordinator().measureTimeSync(laneNode(i), t0, t0 + 8, t0 + 4 + off, t0 + 4 + off);
    }

    // Lane 7 is already started, so it will refuse the range-wide start.
    h.coordinator().startAt(QStringList{laneNode(7)}, h.now() + 10000, h.now());
    const qint64 lane7 = h.node(laneNode(7))->scheduledStartNodeMs();

    const qint64 startAt = h.now() + 20000;
    const FanOutResult r = h.coordinator().startAt(h.nodeIds(), startAt, h.now());
    check(r.accepted() == 19 && r.failed() == 1,
          "isolation: nineteen start, one refuses");
    check(r.failures().first().nodeId == laneNode(7),
          "isolation: the refusal names lane 7");
    check(h.node(laneNode(7))->scheduledStartNodeMs() == lane7,
          "isolation: lane 7 keeps the start it already had");

    bool othersScheduled = true;
    for (int i = 1; i <= 20; ++i) {
        if (i == 7) continue;
        const auto* n = h.node(laneNode(i));
        if (n->scheduledStartNodeMs() - n->clockOffsetMs() != startAt) othersScheduled = false;
    }
    check(othersScheduled, "isolation: the other nineteen scheduled correctly regardless");
}

void testTwentyLanes() { runFullControlAt(20, "20 lanes"); }
void testFiftyLanes()  { runFullControlAt(50, "50 lanes"); }

} // namespace

void run_control_scale_tests()
{
    std::printf("\n-- full control at 20 and 50 lanes --\n");
    testTwentyLanes();
    testFiftyLanes();
    testPartialFailureIsNamed();
    testOneRefusalDoesNotAffectOthers();
    std::fflush(stdout);
}
