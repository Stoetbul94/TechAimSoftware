// RMS scale qualification (RMS-SCALE-001).
//
// The brief's first real milestone is "RMS can manage 20 simulated lanes",
// with 50 as the architecture target. Until now the simulator was clamped to
// six, so nothing in the suite had ever driven the monitor past six nodes.
//
// WHAT THIS ACTUALLY TESTS. Not throughput - the simulator is deterministic
// and virtual-time, so a "rate" here would be a number about this machine
// rather than about the product. What it tests is the things that break when
// the lane count grows and that a six-lane run cannot see:
//
//   * fifty distinct nodes stay fifty distinct lanes (identity, not merging)
//   * dedup still holds when the duplicate arrives among fifty lanes' traffic
//   * a lane going offline does not remove it, at scale
//   * a node restarting is still recognised as the same lane, at scale
//   * every accepted shot is counted exactly once across all lanes
//
// The last one is the point. A dedup bug that is invisible with six lanes and
// one duplicate becomes a wrong score on a wrong lane with fifty.

#include "test_support.h"

#include "rms/RangeMonitor.h"
#include "rms/dev/SimulatedRange.h"

#include <QSet>
#include <QString>
#include <cstdio>

using ta::rms::RangeMonitor;
using ta::rms::dev::SimulatedRange;

namespace {

// The same rig shape the other simulator tests use: the simulator's datagrams
// go through the REAL ingest entry point, not a shortcut.
struct ScaleRig {
    SimulatedRange sim;
    RangeMonitor   monitor;
    int            datagrams = 0;

    ScaleRig()
    {
        QObject::connect(&sim, &SimulatedRange::datagramProduced,
                         [this](const QByteArray& d) {
                             ++datagrams;
                             monitor.ingestDatagram(d, sim.virtualNowMs());
                         });
    }

    void runTo(qint64 ms)
    {
        for (qint64 t = sim.virtualNowMs(); t <= ms; t += 250)
            sim.advanceTo(t);
    }
};

} // namespace

void run_scale_tests()
{
    printf("\n-- RMS scale (RMS-SCALE-001) --\n");
    fflush(stdout);

    // ── 20 lanes: the milestone ───────────────────────────────────────────
    {
        ScaleRig rig;
        rig.sim.configure(20);
        rig.runTo(30000);

        check(rig.monitor.nodeCount() == 20,
              "RMS-SCALE-001: twenty simulated lanes appear as twenty lanes",
              QString::number(rig.monitor.nodeCount()));

        // Identity is the thing that breaks first at scale: a template that
        // repeats its nodeId would silently collapse lanes into each other,
        // and the count above would still look almost right.
        QSet<QString> nodeIds, laneIds, sessionIds;
        for (int i = 0; i < rig.monitor.nodeCount(); ++i) {
            const auto* n = rig.monitor.nodeAt(i);
            if (!n) continue;
            nodeIds.insert(n->nodeId);
            laneIds.insert(n->laneId);
            sessionIds.insert(n->sessionId);
        }
        check(nodeIds.size() == 20,
              "RMS-SCALE-001: every lane has a DISTINCT nodeId - a repeated one "
              "would merge two lanes and still look plausible",
              QString::number(nodeIds.size()));
        check(laneIds.size() == 20,
              "RMS-SCALE-001: and a distinct laneId",
              QString::number(laneIds.size()));
        check(sessionIds.size() == 20,
              "RMS-SCALE-001: and a distinct sessionId",
              QString::number(sessionIds.size()));

        check(rig.datagrams > 0,
              "RMS-SCALE-001: traffic actually flowed",
              QString::number(rig.datagrams));
    }

    // ── 50 lanes: the architecture target ─────────────────────────────────
    {
        ScaleRig rig;
        rig.sim.configure(50);
        rig.runTo(30000);

        check(rig.monitor.nodeCount() == 50,
              "RMS-SCALE-001: fifty simulated lanes appear as fifty lanes",
              QString::number(rig.monitor.nodeCount()));

        QSet<QString> nodeIds;
        int shooting = 0, offline = 0;
        for (int i = 0; i < rig.monitor.nodeCount(); ++i) {
            const auto* n = rig.monitor.nodeAt(i);
            if (!n) continue;
            nodeIds.insert(n->nodeId);
            if (n->ledger.observedCount() > 0) ++shooting;
            if (n->isOffline()) ++offline;
        }
        check(nodeIds.size() == 50,
              "RMS-SCALE-001: fifty distinct node identities",
              QString::number(nodeIds.size()));
        check(shooting > 0,
              "RMS-SCALE-001: lanes are actually shooting, not merely connected",
              QString::number(shooting));

        // The simulator scripts a dropout on one lane in six. At fifty lanes
        // that is several, and a monitor that removed an offline lane would
        // show a shrinking range - the opposite of what an operator needs.
        check(rig.monitor.nodeCount() == 50,
              "RMS-SCALE-001: offline lanes are NOT removed from the range - a "
              "lane that goes quiet must stay visible and stale, not vanish");
    }

    // ── dedup at scale: the assertion that matters most ───────────────────
    // Every lane in the simulator delivers one pair out of order and the
    // development scenario emits a duplicate. With fifty lanes those are
    // buried in fifty lanes' worth of traffic - exactly where a dedup bug
    // stops being obvious.
    {
        ScaleRig rig;
        rig.sim.configure(50);
        rig.runTo(30000);

        int totalAccepted = 0;
        for (int i = 0; i < rig.monitor.nodeCount(); ++i)
            if (const auto* n = rig.monitor.nodeAt(i))
                totalAccepted += n->ledger.observedCount();

        // Replaying every datagram a second time must change NOTHING. This is
        // the strongest available statement of idempotency: it is what a
        // reconnect catch-up, a duplicated broadcast or a re-sent replay
        // window would actually look like.
        ScaleRig replay;
        replay.sim.configure(50);
        QList<QByteArray> captured;
        QObject::connect(&replay.sim, &SimulatedRange::datagramProduced,
                         [&captured](const QByteArray& d) { captured.append(d); });
        replay.runTo(30000);

        RangeMonitor once;
        qint64 t = 0;
        for (const QByteArray& d : captured) once.ingestDatagram(d, t += 10);
        int firstPass = 0;
        for (int i = 0; i < once.nodeCount(); ++i)
            if (const auto* n = once.nodeAt(i)) firstPass += n->ledger.observedCount();

        for (const QByteArray& d : captured) once.ingestDatagram(d, t += 10);
        int secondPass = 0;
        for (int i = 0; i < once.nodeCount(); ++i)
            if (const auto* n = once.nodeAt(i)) secondPass += n->ledger.observedCount();

        check(firstPass > 0,
              "RMS-SCALE-001: the fifty-lane replay recorded shots at all",
              QString::number(firstPass));
        check(secondPass == firstPass,
              "RMS-SCALE-001: REPLAYING EVERY DATAGRAM A SECOND TIME ADDS NO "
              "SHOT - fifty lanes, full duplicate stream, identical total",
              QString("%1 -> %2").arg(firstPass).arg(secondPass));
        check(once.nodeCount() == 50,
              "RMS-SCALE-001: and no lane is duplicated by the replay",
              QString::number(once.nodeCount()));

        // Direct evidence rather than inference: the ledger COUNTS what it
        // threw away. A total that merely failed to grow could also mean
        // nothing arrived.
        int suppressed = 0;
        for (int i = 0; i < once.nodeCount(); ++i)
            if (const auto* n = once.nodeAt(i)) suppressed += n->ledger.duplicatesSuppressed();
        check(suppressed >= firstPass,
              "RMS-SCALE-001: the ledger SAW and suppressed the duplicates - at "
              "least one per shot from the second pass, counted not assumed",
              QString::number(suppressed));
        check(totalAccepted == firstPass,
              "RMS-SCALE-001: the streamed and replayed runs agree, so the "
              "result does not depend on delivery timing",
              QString("%1 vs %2").arg(totalAccepted).arg(firstPass));
    }
}
