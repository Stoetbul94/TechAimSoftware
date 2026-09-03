// WHAT THE WINDOW SAYS ABOUT CONTROL (R2B §28-§31).
//
// The panel is status only. The single most important thing it does is refuse
// to look operable when it is not: this build has no control transport wired,
// and a quiet empty panel would read as "connected, nothing happening". These
// tests hold it to saying so in words.
//
// The second thing they hold it to is not deriving one lane's state from
// another's. A range where nineteen lanes are fine and one is not must not be
// summarised into a single reassuring colour.

#include "control_harness.h"
#include "test_support.h"

#include "rms/control/ControlStatusModel.h"

#include <cstdio>

using namespace ta::rms;
using namespace ta::rms::control;
using ta::rms::test::ControlHarness;

namespace {

QByteArray key32() { return QByteArray(32, '\x66'); }

QVariant cell(const ControlStatusModel& m, int row, int role)
{ return m.data(m.index(row, 0), role); }

void testNoTransportSaysSo()
{
    ControlHarness h(key32());
    h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.addNode(QStringLiteral("TA-NODE-002"), QStringLiteral("Lane 2"));
    h.connectAll();                     // in-process only: NOT a transport
    h.pumpAllStatus(h.now());

    ControlStatusModel m;
    m.setSources(&h.monitor(), &h.coordinator());
    // Deliberately NOT attached, which is this build's true state.

    check(!m.transportAttached(), "status: no transport is attached");
    check(m.statusLine().contains(QLatin1String("NOT ENABLED")),
          "status: the banner says CONTROL CHANNEL NOT ENABLED");
    check(m.statusLine().contains(QLatin1String("observes only")),
          "status: and that this build observes only");

    // Even though the coordinator's own channels ARE authenticated in-process,
    // every lane reads NOT CONNECTED. The panel describes the range, not the
    // software's opinion of itself.
    check(m.authenticatedCount() == 0,
          "status: no lane is reported as authenticated without a transport");
    check(m.laneCount() == 2, "status: both lanes are listed");
    bool allNotConnected = true;
    for (int i = 0; i < m.laneCount(); ++i)
        if (cell(m, i, ControlStatusModel::ChannelRole).toString()
            != QLatin1String("NOT CONNECTED"))
            allNotConnected = false;
    check(allNotConnected, "status: every lane reads NOT CONNECTED");

    // Neutral, not red: a build without a control transport is behaving
    // correctly, and a false alarm teaches operators to ignore the colour.
    check(m.tone() == QLatin1String("neutral"),
          "status: not-enabled is neutral, not a warning");
}

void testAttachedReportsPerLane()
{
    ControlHarness h(key32());
    for (int i = 1; i <= 4; ++i)
        h.addNode(QStringLiteral("TA-NODE-%1").arg(i, 3, 10, QChar('0')),
                  QStringLiteral("Lane %1").arg(i));
    // Lane 3's control link is dead, so it will not authenticate.
    h.setControlLink(QStringLiteral("TA-NODE-003"), false);
    check(h.connectAll() == 3, "status: three of four channels authenticate");

    for (int i = 1; i <= 4; ++i)
        h.node(QStringLiteral("TA-NODE-%1").arg(i, 3, 10, QChar('0')))->fire(10, h.now());
    // Lane 2 lost telemetry for four of its shots.
    h.pumpTelemetry(h.now());
    h.node(QStringLiteral("TA-NODE-002"))->setTelemetryLink(false);
    h.node(QStringLiteral("TA-NODE-002"))->fire(4, h.now() + 100);
    h.pumpTelemetry(h.now() + 100);
    h.pumpAllStatus(h.now() + 200);

    ControlStatusModel m;
    m.setSources(&h.monitor(), &h.coordinator());
    m.setTransportAttached(true);

    check(m.authenticatedCount() == 3, "status: three lanes authenticated");
    check(m.laneCount() == 4, "status: four lanes listed");
    check(m.lanesBehind() == 1, "status: exactly one lane is behind");
    check(m.unobservedShots() == 4, "status: four shots unobserved");
    check(m.statusLine().contains(QLatin1String("3 of 4")),
          "status: the banner names the count, not a colour");
    check(m.statusLine().contains(QLatin1String("4 shots")),
          "status: and how many shots are missing");
    check(m.tone() == QLatin1String("warn"),
          "status: a partially-connected range is a warning");

    // Per lane, and NOT averaged.
    int notConnected = 0, behind = 0;
    for (int i = 0; i < m.laneCount(); ++i) {
        // AUTH FAILURE, not a vague "not connected": the handshake was
        // attempted and refused, and an operator chasing a lane needs the
        // difference between "no answer" and "answered wrongly".
        if (cell(m, i, ControlStatusModel::ChannelRole).toString()
            == QLatin1String("AUTH FAILURE")) ++notConnected;
        if (cell(m, i, ControlStatusModel::UnobservedRole).toInt() > 0) ++behind;
    }
    check(notConnected == 1, "status: one lane individually reads AUTH FAILURE");
    check(behind == 1, "status: one lane individually reads behind");
}

// ── §13: the restart sequence is visible, not a silent gap ─────────────────
void testRestartSequenceIsShown()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.connectAll();
    n->fire(4, h.now());
    h.pumpTelemetry(h.now());
    h.pumpAllStatus(h.now());
    h.coordinator().serviceNodes(&h.monitor(), h.now());

    ControlStatusModel m;
    m.setSources(&h.monitor(), &h.coordinator());
    m.setTransportAttached(true);
    check(cell(m, 0, ControlStatusModel::ChannelRole).toString()
              == QLatin1String("CURRENT"),
          "restart-ui: a settled lane reads CURRENT");
    check(cell(m, 0, ControlStatusModel::RestartsRole).toInt() == 0,
          "restart-ui: with no restarts yet");

    // The node restarts and RMS is told, but has not yet reconnected.
    n->restart();
    h.pumpAllStatus(h.now() + 500);
    h.coordinator().noteBootIdentity(QStringLiteral("TA-NODE-001"), n->bootId(),
                                     h.now() + 500);
    m.refresh();
    check(cell(m, 0, ControlStatusModel::ChannelRole).toString()
              == QLatin1String("RESTART DETECTED"),
          "restart-ui: the moment RMS learns of it, the lane reads RESTART DETECTED");
    check(m.lanesRecovering() == 1, "restart-ui: counted as recovering");
    check(m.statusLine().contains(QLatin1String("recovering from a node restart")),
          "restart-ui: and the banner says so in words");
    check(m.authenticatedCount() == 0,
          "restart-ui: the lane is not reported as authenticated while it is not");

    // The automatic pass takes it the rest of the way.
    h.coordinator().serviceNodes(&h.monitor(), h.now() + 600);
    m.refresh();
    check(cell(m, 0, ControlStatusModel::ChannelRole).toString()
              == QLatin1String("CURRENT"),
          "restart-ui: after reauthentication and replay it is CURRENT again");
    check(cell(m, 0, ControlStatusModel::RestartsRole).toInt() == 1,
          "restart-ui: and the restart is still reported, not forgotten");
    check(m.lanesRecovering() == 0, "restart-ui: nothing is recovering any more");
    check(m.authenticatedCount() == 1, "restart-ui: authenticated once more");
}

void testSyncQualityAndWatermarkSurface()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"), 250);
    h.connectAll();
    n->fire(6, h.now());
    h.pumpTelemetry(h.now());
    h.pumpAllStatus(h.now());

    ControlStatusModel m;
    m.setSources(&h.monitor(), &h.coordinator());
    m.setTransportAttached(true);

    // Before any measurement the quality is UNUSABLE - the honest default. An
    // unmeasured clock is not a good clock.
    check(cell(m, 0, ControlStatusModel::SyncQualityRole).toString()
              == QLatin1String("UNUSABLE"),
          "status: an unmeasured clock reads UNUSABLE, not GOOD");

    const qint64 t0 = h.now();
    h.coordinator().measureTimeSync(n->nodeId(), t0, t0 + 6, t0 + 3 + 250, t0 + 3 + 250);
    h.coordinator().catchUp(n->nodeId(), &h.monitor(), h.now());
    m.refresh();

    check(cell(m, 0, ControlStatusModel::SyncQualityRole).toString()
              == QLatin1String("GOOD"),
          "status: a measured symmetric path reads GOOD");
    check(cell(m, 0, ControlStatusModel::ReconciledToRole).toInt() == 6,
          "status: the reconciliation watermark is shown");
    check(cell(m, 0, ControlStatusModel::LaneLabelRole).toString()
              == QLatin1String("Lane 1"),
          "status: the lane is named by its lane label, not its node id");
}

} // namespace

void run_control_status_tests()
{
    std::printf("\n-- control-channel status panel --\n");
    testNoTransportSaysSo();
    testAttachedReportsPerLane();
    testRestartSequenceIsShown();
    testSyncQualityAndWatermarkSurface();
    std::fflush(stdout);
}
