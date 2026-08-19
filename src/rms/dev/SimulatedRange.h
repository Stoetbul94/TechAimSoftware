#ifndef TA_RMS_DEV_SIMULATEDRANGE_H
#define TA_RMS_DEV_SIMULATEDRANGE_H

// ─────────────────────────────────────────────────────────────────────────────
// DEVELOPMENT ONLY — a fake range of target nodes.
//
// This file lives under src/rms/dev/ and is compiled only when
// TECHAIM_RMS_DEV_SIMULATOR is defined. It exists so RMS can be built and
// demonstrated without a live target, and it is the ONLY place in the RMS
// tree that plays the node's role.
//
// IT CANNOT CONTAMINATE PRODUCTION TARGET LOGIC. It is in the RMS product
// tree, not the target application's; it imports nothing from the target's
// acquisition, scoring or SessionStore code; and it emits datagrams into the
// same read-only `RangeMonitor::ingestDatagram` entry point the network uses.
// The application prints a standing banner whenever it is active, and the
// dashboard shows a SIMULATED badge, so a simulated range can never be
// mistaken for a real one.
//
// The scores it produces are the SIMULATED NODE's scores. RMS still never
// computes one — the simulator is standing in for the authority, not
// bypassing it.
//
// DETERMINISTIC. No wall clock and no QRandomGenerator: virtual time is
// advanced explicitly and the number sequence is a seeded LCG, so the harness
// asserts exact counts with no sleeps and no flakiness.
// ─────────────────────────────────────────────────────────────────────────────

#include "rms/dev/TargetShotFixtures.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVector>

namespace ta {
namespace rms {
namespace dev {

class SimulatedRange : public QObject
{
    Q_OBJECT
public:
    explicit SimulatedRange(QObject* parent = nullptr);

    // Which scripted scenario to play.
    //
    //   Development   the harness scenario. Lane 3 drops out, lane 5's
    //                 application restarts. Asserted by tst_simulator; do not
    //                 change it to suit a demonstration.
    //   FieldTestDemo the field-test scenario a human clicks through. Lane 3
    //                 goes offline and STAYS offline, so an operator arriving
    //                 at any moment sees an offline lane. Lane 4 goes offline
    //                 and returns, so it carries the unseen-shot warning. Both
    //                 windows are explicit, not left to a restart's timing.
    enum class Scenario { Development, FieldTestDemo };

    // Builds the scripted scenario. `laneCount` is clamped to 3..6, matching
    // the milestone's "3-6 target nodes".
    void configure(int laneCount, Scenario scenario = Scenario::Development);

    // The scenario declares this lane's athlete done — finished or counted out.
    // The node then stops SHOOTING but keeps answering: an athlete leaving the
    // competition does not break their target station, and the lane must not
    // start looking like a fault.
    //
    // This is the SCRIPT speaking, not a deduction. Nothing here reads a score,
    // a rank or a shot count to decide it, and RMS is told by the same
    // development injection that stamps the value SIMULATED.
    void concludeLane(int laneNumber);

    // Advances virtual time to `nowMs`, emitting every datagram the scenario
    // schedules in between. Safe to call with any cadence — the internal step
    // is fixed, so the output depends only on elapsed virtual time.
    void advanceTo(qint64 nowMs);

    qint64 virtualNowMs() const { return m_nowMs; }
    int datagramsEmitted() const { return m_emitted; }

    // Scenario landmarks, so the harness and the UI agree on what should be
    // visible when, without duplicating the numbers.
    static qint64 laneDropoutStartMs() { return 14000; }
    static qint64 laneDropoutEndMs()   { return 34000; }
    static qint64 nodeRestartAtMs()    { return 26000; }

signals:
    // One encoded datagram, byte-identical to what a node would broadcast.
    void datagramProduced(const QByteArray& payload);

private:
    struct SimNode {
        QString nodeId;
        QString bootId;
        QString laneId;
        QString sessionId;
        QString programmeId;
        QString rulesetId;
        QString targetStandardId;
        QString athlete;
        QString device;
        int     shotsExpected = -1;
        int     shotsAccepted = 0;
        double  totalScore = 0.0;
        quint64 statusSeq = 0;
        int     eventCounter = 0;
        bool    announced = false;
        qint64  nextStatusMs = 0;
        qint64  nextShotMs = 0;
        // Scenario switches
        bool    dropsOut = false;      // goes silent, then returns
        // An explicit silent window, when the scenario wants one that is not
        // the shared landmark. -1 means "use dropsOut and the landmarks".
        qint64  silentFromMs = -1;
        qint64  silentToMs = -1;
        bool    restarts = false;      // comes back with a new bootId
        bool    emittedDuplicate = false;
        bool    swapPending = false;   // holds one shot back to arrive late
        bool    concluded = false;     // scripted: stops shooting, keeps answering
        // Correlated fixture data for this lane's target, when the scenario
        // uses it. Null in the development scenario.
        const FixtureShot* fixtures = nullptr;
        int     fixtureCount = 0;
        QByteArray heldShot;
    };

    Scenario m_scenario = Scenario::Development;

    void stepOnce(qint64 tMs);
    // `silent` suppresses TRANSMISSION only. The node's own state still
    // advances — a target does not stop shooting because the network dropped.
    void emitAnnounce(SimNode& n, qint64 tMs, bool silent);
    void emitStatus(SimNode& n, qint64 tMs, bool silent);
    void emitShot(SimNode& n, qint64 tMs, bool silent);
    bool isSilent(const SimNode& n, qint64 tMs) const;
    double nextScore();

    QVector<SimNode> m_nodes;
    qint64 m_nowMs = 0;
    int    m_emitted = 0;
    quint32 m_rand = 20260819u;   // seeded LCG; fixed so runs are repeatable
};

} // namespace dev
} // namespace rms
} // namespace ta

#endif // TA_RMS_DEV_SIMULATEDRANGE_H
