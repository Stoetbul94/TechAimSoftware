// The development simulator, end to end: it must produce a range that the
// read-only observer can reconstruct, including the awkward parts — a lane
// that drops out and returns, a node that restarts, duplicates and reordering.
//
// This is also the milestone's dashboard evidence in headless form: the same
// RangeListModel the QML binds to is asserted here.

#include "test_support.h"

#include "rms/RangeListModel.h"
#include "rms/RangeMonitor.h"
#include "rms/dev/SimulatedRange.h"

#include <cstdio>

using namespace ta::rms;

namespace {

// Drives the simulated range and the observer over the same virtual clock.
struct Rig {
    RangeMonitor monitor;
    RangeListModel model{&monitor};
    dev::SimulatedRange sim;

    Rig()
    {
        QObject::connect(&sim, &dev::SimulatedRange::datagramProduced,
                         [this](const QByteArray& d) {
                             monitor.ingestDatagram(d, sim.virtualNowMs());
                         });
    }

    void runTo(qint64 ms)
    {
        for (qint64 t = sim.virtualNowMs(); t <= ms; t += 250) {
            sim.advanceTo(t);
            monitor.evaluateLiveness(t);
        }
    }

    const TargetNodeRecord* lane(int oneBased) const
    {
        return monitor.nodeById(QStringLiteral("TA-NODE-%1")
                                    .arg(oneBased, 3, 10, QLatin1Char('0')));
    }
};

} // namespace

void run_simulator_tests()
{
    std::printf("\n-- simulated range --\n");

    {
        Rig rig;
        rig.sim.configure(6);
        rig.runTo(10000);

        check(rig.monitor.nodeCount() == 6, "six simulated nodes are discovered");
        check(rig.model.rowCountProperty() == 6, "the dashboard shows six lanes");
        check(rig.monitor.rejectedDatagrams() == 0,
              "the simulator emits nothing the decoder rejects");

        const TargetNodeRecord* l1 = rig.lane(1);
        check(l1 && l1->laneId == QLatin1String("Lane 1"), "lane assignment observed");
        check(l1 && l1->nodeId != l1->laneId,
              "node identity is separate from lane identity");
        check(l1 && l1->programmeId == QLatin1String("issf.10m.air-rifle.qualification60"),
              "lane 1 runs an ISSF 60-shot qualification");
        check(l1 && l1->shotsAcceptedByNode > 0, "the node reports accepted shots");
        check(l1 && l1->totalScoreByNode > 0.0, "the node reports a running total");

        const TargetNodeRecord* l6 = rig.lane(6);
        check(l6 && l6->shotsExpected == -1, "an unlimited programme reports -1 expected");
        check(l6 && l6->rulesetId == QLatin1String("techaim"),
              "a Tech Aim preset is not labelled as an ISSF event");
    }

    // ── duplicates and reordering, produced by the simulator itself ────
    {
        Rig rig;
        rig.sim.configure(6);
        rig.runTo(14000);

        const TargetNodeRecord* l1 = rig.lane(1);
        check(l1 && l1->ledger.duplicatesSuppressed() >= 1,
              "the re-sent shot #3 is suppressed by the observer");
        check(l1 && l1->ledger.outOfOrderAccepted() >= 1,
              "the held-back shot #5 arrives after #6 and is accepted late");
        check(l1 && l1->ledger.missingSequences().isEmpty(),
              "once the late shot lands there are no gaps");

        const QList<AcceptedShot> ordered = l1->ledger.shotsInOrder();
        bool ascending = true;
        for (int i = 1; i < ordered.size(); ++i)
            if (ordered.at(i).shotSequence <= ordered.at(i - 1).shotSequence)
                ascending = false;
        check(ascending, "the observer's shot list is strictly ascending by sequence");
        check(l1 && l1->ledger.observedCount() == l1->ledger.highestSequence(),
              "no shot is held twice and none is missing");
    }

    // ── lane 3 loses the network, then returns ──────────────────────────
    {
        Rig rig;
        rig.sim.configure(6);
        rig.runTo(dev::SimulatedRange::laneDropoutStartMs() + 9000);

        const TargetNodeRecord* l3 = rig.lane(3);
        check(l3 && l3->isOffline(), "the lane that went silent is shown OFFLINE");
        check(rig.model.offlineCount() >= 1, "the summary bar counts it");
        check(rig.monitor.nodeCount() == 6, "an offline lane is not removed from the range");

        const int keptShots = l3 ? l3->ledger.observedCount() : -1;
        const int keptNodeCount = l3 ? l3->shotsAcceptedByNode : -1;
        check(keptShots > 0, "its observed shots are retained while it is away");

        rig.runTo(dev::SimulatedRange::laneDropoutEndMs() + 4000);
        l3 = rig.lane(3);
        check(l3 && !l3->isOffline(), "the returning lane comes back online");
        check(l3 && l3->nodeRestarts == 0,
              "a network dropout is NOT reported as a node restart");
        check(l3 && l3->shotsAcceptedByNode > keptNodeCount,
              "RMS resumes from the node's authoritative count, which moved on without it");
        check(l3 && l3->unobservedShotCount() > 0,
              "the shots fired during the outage are declared unobserved");
        check(l3 && l3->ledger.observedCount() >= keptShots,
              "nothing observed before the outage was discarded");
    }

    // ── lane 5's application restarts ───────────────────────────────────
    {
        Rig rig;
        rig.sim.configure(6);
        rig.runTo(dev::SimulatedRange::nodeRestartAtMs() + 8000);

        const TargetNodeRecord* l5 = rig.lane(5);
        check(l5 && l5->nodeRestarts == 1, "a new bootId is detected as a node restart");
        check(rig.monitor.nodeCount() == 6,
              "a restarted node does not appear as a seventh lane");
        check(l5 && l5->bootId.endsWith(QLatin1Char('b')),
              "the node's new boot identity is adopted");
        check(l5 && l5->shotsAcceptedByNode > 0,
              "the node's own state after the restart is what RMS shows");
    }

    // ── determinism ─────────────────────────────────────────────────────
    {
        Rig a, b;
        a.sim.configure(6);
        b.sim.configure(6);
        a.runTo(20000);
        b.runTo(20000);
        check(a.model.renderTextDashboard() == b.model.renderTextDashboard(),
              "two runs of the simulated range produce an identical dashboard");
        check(a.sim.datagramsEmitted() == b.sim.datagramsEmitted(),
              "...from an identical datagram count");
    }

    // ── lane count is clamped to 3..kMaxSimulatedLanes ──────────────────
    // The upper bound WAS six, because kPlan has six entries and reading past
    // it is undefined behaviour. The plan is now a repeating template with
    // per-lane unique identities, so the bound moved to the simulator's own
    // constant. The assertion moved with it rather than being deleted: a
    // clamp that is not asserted is a clamp that silently stops existing.
    {
        Rig few;
        few.sim.configure(1);
        few.runTo(6000);
        check(few.monitor.nodeCount() == 3, "fewer than three lanes is clamped up to three");

        Rig many;
        many.sim.configure(999);
        many.runTo(6000);
        check(many.monitor.nodeCount() == ta::rms::dev::SimulatedRange::kMaxSimulatedLanes,
              "an absurd lane count is clamped down to the simulator maximum",
              QString::number(many.monitor.nodeCount()));
    }

    // ── dashboard evidence, printed ─────────────────────────────────────
    {
        Rig rig;
        rig.sim.configure(6);
        rig.runTo(20000);
        std::printf("\n     [dashboard at virtual t=20s]\n");
        const QStringList lines =
            rig.model.renderTextDashboard().split(QLatin1Char('\n'));
        for (const QString& l : lines)
            if (!l.isEmpty())
                std::printf("     %s\n", qPrintable(l));
        std::printf("\n");
        std::fflush(stdout);
        check(rig.model.renderTextDashboard().contains(QLatin1String("Lane 1")),
              "the rendered dashboard names its lanes");
        check(rig.model.renderTextDashboard().contains(
                  QStringLiteral("10 m Air Rifle · Qualification 60")),
              "the rendered dashboard names the programme");
    }
}
