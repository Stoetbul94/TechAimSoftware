// Tech Aim — ACQUISITION INTEGRITY. The exact 2026-08-23 physical failures.
//
// Every case below is taken from the four-tablet forensic reconstruction, not
// invented. The two blockers it exists to make impossible:
//
//   ACQ-FLUSH-001  every 10th shot the application zeroed its own baseline and
//                  left the hardware counter to a thread that slept 2 600 ms
//                  first. The 100 ms poll read baseline 0 against target 10 and
//                  stopped acquisition. Observed 12/12 on Tablets 01, 02 and
//                  04; measured 57-156 ms from reset to fault.
//   ACQ-DESYNC-002 the operator's fix for the above - unplug and replug the USB
//                  - made the reconnect adopt "target reports 1" while ten
//                  coordinates were held. Every later shot asked for an index
//                  one past the end, getXCord() answered its -1 sentinel, and
//                  -1.00/-1.00 mm scores 10.8 on a 50 m rifle target. Fourteen
//                  such records were written to Tablet-02 journals.
//
// WHAT IS UNDER TEST: ta::target::AcquisitionSequencer and the decision
// functions it calls - the SAME code the application runs. The FakeTarget below
// is a register model (a stand-in for the hardware), not a second copy of the
// algorithm. The ~15 lines of driveOnePoll() mirror the call order in
// TachusWidget::on_pushButton_2_clicked(); tst_acquisition_callorder below
// asserts that the production source still calls them in that order, so the
// two cannot drift apart silently.

#include "target/AcquisitionSequencer.h"
#include "test_support.h"

#include <QFile>
#include <QString>

#include <map>
#include <vector>

using namespace ta::target;

namespace {

// ── the target, as a register model ───────────────────────────────────────
// Mirrors the map the application actually reads: 8192[1] is the shot counter,
// 8193 zeroes it, 16376+8*i holds slot i's coordinates. Nothing here decides
// anything; it only answers reads the way the hardware does.
struct FakeTarget {
    int counter = 0;
    std::map<int, std::pair<double, double> > slotData;   // 1-based slot -> x,y mm
    bool honourReset = true;
    long long resetLatencyMs = 0;
    long long resetAskedAtMs = -1;
    bool coordinateReadFails = false;
    bool coordinateSlotStale = false;    // slot exists but was never rewritten
    int  readsServed = 0;

    void fire(double x, double y)
    {
        ++counter;
        if (!coordinateSlotStale)
            slotData[counter] = std::make_pair(x, y);
    }
    void askReset(long long nowMs) { resetAskedAtMs = nowMs; }
    void tick(long long nowMs)
    {
        if (resetAskedAtMs >= 0 && honourReset
            && nowMs - resetAskedAtMs >= resetLatencyMs) {
            counter = 0;
            resetAskedAtMs = -1;
        }
    }
    int readCounter() { ++readsServed; return counter; }
    bool readSlot(int slot, double& x, double& y) const
    {
        if (coordinateReadFails) return false;
        std::map<int, std::pair<double, double> >::const_iterator it = slotData.find(slot);
        if (it == slotData.end()) return false;
        x = it->second.first; y = it->second.second;
        return true;
    }
};

// ── what the application holds ────────────────────────────────────────────
// The coordinate arrays and the published scores. A score may only ever be
// produced from a coordinate that exists at the published shot number.
struct AppState {
    std::vector<double> xs, ys;
    std::vector<int> publishedShotNumbers;
    int faults = 0;
    FaultCause lastCause = FaultCause::None;
    int uncapturedReported = 0;

    // The one thing the field defect produced: a score with no coordinate.
    bool everScoredWithoutCoordinate = false;
    // The exact observed corruption, kept as its own explicit tripwire.
    int  tenPointEightFromSentinel = 0;
};

// One poll tick, in the order TachusWidget performs it.
void driveOnePoll(AcquisitionSequencer& seq, FakeTarget& t, AppState& app,
                  long long nowMs)
{
    t.tick(nowMs);
    seq.setCapturedShots(static_cast<int>(app.xs.size()));
    const SeqStep step = seq.poll(t.readCounter(), nowMs);

    switch (step.action) {
    case SeqAction::IssueCounterReset:
        t.askReset(nowMs);
        return;
    case SeqAction::RaiseFault:
        ++app.faults; app.lastCause = step.cause;
        return;
    case SeqAction::ReportSynchronized:
        app.uncapturedReported += step.uncapturedShots;
        return;
    case SeqAction::FetchCoordinates:
        break;
    case SeqAction::Idle:
    case SeqAction::AwaitResetProof:
    default:
        return;
    }

    for (int slot = step.firstSlot; slot <= step.lastSlot; ++slot) {
        double x = 0, y = 0;
        if (!t.readSlot(slot, x, y)) {
            // ACQ-READ-004: a failed read is not decoded, not published, not
            // scored and not fed. It stops acquisition instead.
            const SeqStep f = seq.noteCoordinateReadFailed(slot);
            ++app.faults; app.lastCause = f.cause;
            return;
        }
        app.xs.push_back(x);
        app.ys.push_back(y);
        const int shotNo = seq.noteCoordinateCaptured();
        app.publishedShotNumbers.push_back(shotNo);

        // The scoring gate. This is what the -1 sentinel used to slip past.
        if (!coordinateIndexValid(shotNo, static_cast<int>(app.xs.size()),
                                  static_cast<int>(app.ys.size()))) {
            app.everScoredWithoutCoordinate = true;
            if (shotNo >= 1) ++app.tenPointEightFromSentinel;
        }
    }

    const SeqStep reset = seq.maybeStartCounterReset(nowMs);
    if (reset.action == SeqAction::IssueCounterReset)
        t.askReset(nowMs);
}

// Fire one shot and let the acquisition settle. Returns the polls consumed.
int fireAndSettle(AcquisitionSequencer& seq, FakeTarget& t, AppState& app,
                  long long& nowMs, double x, double y, int maxPolls = 120)
{
    t.fire(x, y);
    int polls = 0;
    const int before = static_cast<int>(app.xs.size());
    for (; polls < maxPolls; ++polls) {
        driveOnePoll(seq, t, app, nowMs);
        nowMs += 100;                         // the real poll period
        if (static_cast<int>(app.xs.size()) > before
            && seq.state() == AcqState::Acquiring
            && !seq.resetOutstanding())
            break;
        if (seq.state() == AcqState::Fault) break;
    }
    return polls;
}

void settle(AcquisitionSequencer& seq, FakeTarget& t, AppState& app,
            long long& nowMs, int polls)
{
    for (int i = 0; i < polls; ++i) { driveOnePoll(seq, t, app, nowMs); nowMs += 100; }
}

QString repoFile(const char* rel)
{
    return QString::fromLatin1(RELIABILITY_FIXTURES_DIR "/../../../") + QLatin1String(rel);
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════

void run_acquisition_integrity_tests()
{
    fputs("\n--- ACQ: acquisition integrity (2026-08-23 physical failures) ---\n", stdout);

    // ══ TEST A — 500 SERIES BOUNDARIES ═════════════════════════════════════
    // The defect fired at every 10th shot, 12 times out of 12. 500 boundaries
    // is 5 000 shots: if the flush can stop acquisition even once, this fails.
    {
        AcquisitionSequencer seq; FakeTarget t; AppState app;
        long long now = 0;
        settle(seq, t, app, now, 3);                 // initial synchronize

        const int kBoundaries = 500;
        const int kShots = kBoundaries * 10;
        for (int n = 1; n <= kShots; ++n)
            fireAndSettle(seq, t, app, now, 0.1 * (n % 40) - 2.0, 0.1 * (n % 31) - 1.5);

        check(app.faults == 0,
              "A. 500 series boundaries produce no ACQUISITION_FAULT",
              QStringLiteral("faults=%1").arg(app.faults));
        check(static_cast<int>(app.xs.size()) == kShots,
              "A. every shot is captured exactly once - none lost, none duplicated",
              QStringLiteral("captured=%1 expected=%2")
                  .arg(app.xs.size()).arg(kShots));
        bool numbering = (static_cast<int>(app.publishedShotNumbers.size()) == kShots);
        for (int i = 0; numbering && i < kShots; ++i)
            numbering = (app.publishedShotNumbers[i] == i + 1);
        check(numbering, "A. published shot numbers are 1..5000 with no gap or repeat");
        check(!app.everScoredWithoutCoordinate,
              "A. no score was produced without a coordinate");
        check(seq.shotCount() == static_cast<int>(app.xs.size()),
              "A. INVARIANT A holds after 500 boundaries: shot count == coordinates held");
    }

    // ══ TEST B — RESET TIMING ══════════════════════════════════════════════
    // The field build hard-coded 2 600 ms. The application must not care what
    // the target takes, as long as it is inside the deadline - and must fault
    // visibly, not silently, when it is not.
    {
        const long long latencies[] = { 0, 50, 100, 250, 500, 1000, 2600 };
        for (unsigned k = 0; k < sizeof(latencies)/sizeof(latencies[0]); ++k) {
            AcquisitionSequencer seq; FakeTarget t; AppState app;
            t.resetLatencyMs = latencies[k];
            long long now = 0;
            settle(seq, t, app, now, 3);
            for (int n = 1; n <= 25; ++n)
                fireAndSettle(seq, t, app, now, 1.0, -1.0);
            check(app.faults == 0,
                  QStringLiteral("B. reset latency %1 ms is never read as lost shots")
                      .arg(latencies[k]),
                  QStringLiteral("faults=%1 captured=%2").arg(app.faults).arg(app.xs.size()));
            check(static_cast<int>(app.xs.size()) == 25,
                  QStringLiteral("B. reset latency %1 ms still captures all 25 shots")
                      .arg(latencies[k]));
        }
    }
    {
        // Beyond the deadline the reset is NOT quietly forgotten: it becomes an
        // acquisition fault an operator can see. A silent stall would repeat
        // the original defect with better manners.
        AcquisitionSequencer seq(10, 3000); FakeTarget t; AppState app;
        t.honourReset = false;                    // the target never confirms
        long long now = 0;
        settle(seq, t, app, now, 3);
        for (int n = 1; n <= 10; ++n) fireAndSettle(seq, t, app, now, 1.0, 1.0);
        settle(seq, t, app, now, 60);             // 6 s > the 3 s deadline
        check(app.faults >= 1 && seq.faultCause() == FaultCause::ResetNotConfirmed,
              "B. a reset the target never confirms becomes ResetNotConfirmed, not a stall",
              QStringLiteral("faults=%1").arg(app.faults));
        check(seq.state() == AcqState::Fault,
              "B. and the fault stays latched rather than clearing itself");
    }

    // ══ TEST C — RECONNECT, COUNTERS AGREE ═════════════════════════════════
    // Tablet-01 and Tablet-04: every reconnect adopted 0 -> 0 and neither
    // tablet produced a single corrupted score.
    {
        AcquisitionSequencer seq; FakeTarget t; AppState app;
        long long now = 0;
        settle(seq, t, app, now, 3);
        for (int n = 1; n <= 7; ++n) fireAndSettle(seq, t, app, now, 2.0, 2.0);
        const int captured = static_cast<int>(app.xs.size());

        t.counter = 0;                            // replug: the target restarts at 0
        seq.noteLinkRestored();
        settle(seq, t, app, now, 3);
        fireAndSettle(seq, t, app, now, -3.5, 4.5);

        check(app.faults == 0, "C. reconnect with agreeing counters resumes cleanly");
        check(static_cast<int>(app.xs.size()) == captured + 1,
              "C. the first shot after reconnect is captured exactly once");
        check(app.publishedShotNumbers.back() == captured + 1,
              "C. and is numbered 8, continuing the session",
              QStringLiteral("published=%1").arg(app.publishedShotNumbers.back()));
        check(app.xs.back() == -3.5 && app.ys.back() == 4.5,
              "C. with ITS OWN coordinates, not a neighbour's");
    }

    // ══ TEST D — RECONNECT, COUNTER MISMATCH ═══════════════════════════════
    // The Tablet-02 condition: the athlete fired while the cable was out, so
    // the target counted a shot this application never captured.
    {
        AcquisitionSequencer seq; FakeTarget t; AppState app;
        long long now = 0;
        settle(seq, t, app, now, 3);
        for (int n = 1; n <= 4; ++n) fireAndSettle(seq, t, app, now, 1.0, 1.0);

        t.counter = 0; seq.noteLinkRestored();
        t.fire(9.9, 9.9);                         // fired while we were blind
        settle(seq, t, app, now, 3);

        check(app.uncapturedReported == 1,
              "D. the shot counted while offline is REPORTED, not absorbed in silence",
              QStringLiteral("uncaptured=%1").arg(app.uncapturedReported));
        check(static_cast<int>(app.xs.size()) == 4,
              "D. and is NOT invented as a captured shot");

        fireAndSettle(seq, t, app, now, -2.2, 3.3);
        check(static_cast<int>(app.xs.size()) == 5,
              "D. the next genuine shot is captured");
        check(app.publishedShotNumbers.back() == 5,
              "D. numbered 5 - the index never runs ahead of the arrays",
              QStringLiteral("published=%1").arg(app.publishedShotNumbers.back()));
        check(app.xs.back() == -2.2 && app.ys.back() == 3.3,
              "D. and carries its own real coordinates");
        check(!app.everScoredWithoutCoordinate,
              "D. no score is produced from a missing coordinate");
    }

    // ══ TEST E — THE EXACT TABLET-02 10.8 FAILURE ══════════════════════════
    // Reconstructed from session_20260823T131510_6506fc2d.jsonl and
    // tachus_log23082026-150736.log: ten shots captured, the flush fires, the
    // operator replugs, the target reports 1, and the next genuine shot arrives
    // with real coordinates (the log shows x=-3.9 y=1.0 decoded correctly).
    // The field build published that shot as number 12 into an 11-element array
    // and scored the -1/-1 sentinel as 10.8.
    {
        AcquisitionSequencer seq; FakeTarget t; AppState app;
        long long now = 0;
        settle(seq, t, app, now, 3);
        for (int n = 1; n <= 10; ++n) fireAndSettle(seq, t, app, now, 0.5, -0.5);
        check(static_cast<int>(app.xs.size()) == 10 && app.faults == 0,
              "E. ten shots captured across the flush boundary, no fault");

        // The replug. The target restarts at 0 and immediately counts one shot
        // that this application cannot see.
        t.counter = 0; t.slotData.clear();
        seq.noteLinkRestored();
        t.fire(7.7, 7.7);                         // the shot fired while blind
        settle(seq, t, app, now, 3);

        // The genuine next shot, with the coordinates the log proves the
        // backend decoded correctly.
        fireAndSettle(seq, t, app, now, -3.9, 1.0);

        check(static_cast<int>(app.xs.size()) == 11,
              "E. the genuine shot after the replug is captured exactly once",
              QStringLiteral("captured=%1").arg(app.xs.size()));
        check(app.publishedShotNumbers.back() == 11,
              "E. published as shot 11 - NOT 12 into an 11-element array",
              QStringLiteral("published=%1").arg(app.publishedShotNumbers.back()));
        check(app.xs.back() == -3.9 && app.ys.back() == 1.0,
              "E. carrying the coordinates the backend decoded (-3.9, 1.0)",
              QStringLiteral("x=%1 y=%2").arg(app.xs.back()).arg(app.ys.back()));
        check(app.tenPointEightFromSentinel == 0,
              "E. THE FIELD DEFECT: no -1/-1 sentinel reached scoring");
        check(!app.everScoredWithoutCoordinate,
              "E. no shot number was published without a coordinate behind it");

        // And it stays true for the rest of the session - the field failure was
        // permanent once it started.
        for (int n = 0; n < 20; ++n)
            fireAndSettle(seq, t, app, now, 1.0 + n * 0.1, -1.0 - n * 0.1);
        bool aligned = true;
        for (size_t i = 0; i < app.publishedShotNumbers.size(); ++i)
            aligned = aligned && (app.publishedShotNumbers[i] == static_cast<int>(i) + 1);
        check(aligned, "E. and the numbering stays aligned for the next 20 shots");
        check(app.tenPointEightFromSentinel == 0,
              "E. the repeated-10.8 path does not reappear later in the session");
    }

    // ══ TEST F — FAILED COORDINATE READ ════════════════════════════════════
    {
        AcquisitionSequencer seq; FakeTarget t; AppState app;
        long long now = 0;
        settle(seq, t, app, now, 3);
        for (int n = 1; n <= 3; ++n) fireAndSettle(seq, t, app, now, 1.0, 1.0);
        const int before = static_cast<int>(app.xs.size());

        t.coordinateReadFails = true;
        fireAndSettle(seq, t, app, now, 4.0, 4.0);

        check(static_cast<int>(app.xs.size()) == before,
              "F. a failed coordinate read appends no coordinate");
        check(static_cast<int>(app.publishedShotNumbers.size()) == before,
              "F. publishes no shot");
        check(app.faults >= 1, "F. and raises an explicit acquisition fault");
        check(seq.state() == AcqState::Fault,
              "F. acquisition is stopped, not continued on residue");
    }

    // ══ TEST G — STALE COORDINATE BUFFER ═══════════════════════════════════
    // The counter advances but the slot was never written. Reusing whatever the
    // slot held is exactly how a real shot acquires someone else's score.
    {
        AcquisitionSequencer seq; FakeTarget t; AppState app;
        long long now = 0;
        settle(seq, t, app, now, 3);
        for (int n = 1; n <= 3; ++n) fireAndSettle(seq, t, app, now, 1.0, 1.0);
        const int before = static_cast<int>(app.xs.size());

        t.coordinateSlotStale = true;             // counter moves, slot does not
        fireAndSettle(seq, t, app, now, 0, 0);

        check(static_cast<int>(app.xs.size()) == before,
              "G. a counter with no fresh slot data captures nothing");
        check(app.faults >= 1, "G. and is reported, not silently reused");
    }

    // ══ TEST H — DISCONNECT DURING THE SERIES TRANSITION ═══════════════════
    // The worst moment: the cable goes out between the 10th shot and the
    // target confirming our reset.
    {
        AcquisitionSequencer seq; FakeTarget t; AppState app;
        t.resetLatencyMs = 400;
        long long now = 0;
        settle(seq, t, app, now, 3);
        for (int n = 1; n <= 9; ++n) fireAndSettle(seq, t, app, now, 1.0, 1.0);

        t.fire(2.0, 2.0);                          // the 10th shot
        driveOnePoll(seq, t, app, now); now += 100;   // captured; reset issued
        check(static_cast<int>(app.xs.size()) == 10, "H. the 10th shot is captured");

        seq.noteLinkLost();                        // cable out mid-reset
        t.counter = 0; t.slotData.clear(); t.resetAskedAtMs = -1;
        seq.noteLinkRestored();
        settle(seq, t, app, now, 3);
        fireAndSettle(seq, t, app, now, -6.6, 2.4);

        check(static_cast<int>(app.xs.size()) == 11,
              "H. exactly one shot is captured after the reconnect - no replay");
        check(app.publishedShotNumbers.back() == 11,
              "H. numbered 11 - the index did not desynchronise");
        check(app.xs.back() == -6.6, "H. with its own coordinates");
        check(!app.everScoredWithoutCoordinate, "H. and nothing was scored blind");
    }

    // ══ TEST I — DISCONNECT DURING NORMAL SHOOTING ═════════════════════════
    // Each reconnect policy is stated explicitly rather than left to chance.
    {
        const int counterAfter[] = { 0, 1, 4 };   // same, +1, +N
        const int expectUncaptured[] = { 0, 1, 4 };
        for (unsigned k = 0; k < 3; ++k) {
            AcquisitionSequencer seq; FakeTarget t; AppState app;
            long long now = 0;
            settle(seq, t, app, now, 3);
            for (int n = 1; n <= 5; ++n) fireAndSettle(seq, t, app, now, 1.0, 1.0);

            seq.noteLinkLost();
            t.counter = counterAfter[k];
            for (int i = 1; i <= counterAfter[k]; ++i) t.slotData[i] = std::make_pair(0.0, 0.0);
            seq.noteLinkRestored();
            settle(seq, t, app, now, 3);

            check(app.uncapturedReported == expectUncaptured[k],
                  QStringLiteral("I. reconnect at counter %1 reports %2 uncaptured shot(s)")
                      .arg(counterAfter[k]).arg(expectUncaptured[k]),
                  QStringLiteral("reported=%1").arg(app.uncapturedReported));
            check(static_cast<int>(app.xs.size()) == 5,
                  QStringLiteral("I. reconnect at counter %1 invents no coordinates")
                      .arg(counterAfter[k]));

            t.fire(3.3, -3.3);
            settle(seq, t, app, now, 5);
            check(static_cast<int>(app.xs.size()) == 6,
                  QStringLiteral("I. shooting resumes after a counter-%1 reconnect")
                      .arg(counterAfter[k]),
                  QStringLiteral("captured=%1").arg(app.xs.size()));
            check(app.publishedShotNumbers.back() == static_cast<int>(app.xs.size()),
                  QStringLiteral("I. and the published number still equals the coordinates held (counter %1)")
                      .arg(counterAfter[k]));
        }
    }

    // ══ TEST J — SIGHTER TO COUNTED ════════════════════════════════════════
    // Numbering legitimately restarts. It must restart cleanly, not leave the
    // sequencer holding a count no coordinate supports.
    {
        AcquisitionSequencer seq; FakeTarget t; AppState app;
        long long now = 0;
        settle(seq, t, app, now, 3);
        for (int n = 1; n <= 6; ++n) fireAndSettle(seq, t, app, now, 1.0, 1.0);

        seq.resetAll();                            // sighter -> counted
        app.xs.clear(); app.ys.clear(); app.publishedShotNumbers.clear();
        t.counter = 0; t.slotData.clear();
        settle(seq, t, app, now, 3);

        check(seq.shotCount() == 0, "J. leaving sighter mode leaves no residual count");
        fireAndSettle(seq, t, app, now, 5.5, -5.5);
        check(app.publishedShotNumbers.size() == 1 && app.publishedShotNumbers[0] == 1,
              "J. the first counted shot is shot 1");
        check(app.xs.back() == 5.5, "J. with its own coordinates");
        check(app.faults == 0, "J. and no fault is raised by the transition");
    }

    // ══ TEST L — TRAINING / PRONE50 LENGTH ═════════════════════════════════
    // A 60-shot prone match crosses six boundaries; the field build stopped at
    // the first one.
    {
        AcquisitionSequencer seq; FakeTarget t; AppState app;
        long long now = 0;
        settle(seq, t, app, now, 3);
        for (int n = 1; n <= 60; ++n) fireAndSettle(seq, t, app, now, 0.2 * n - 6.0, 6.0 - 0.2 * n);
        check(app.faults == 0 && static_cast<int>(app.xs.size()) == 60,
              "L. a 60-shot prone match crosses six boundaries with no fault",
              QStringLiteral("faults=%1 captured=%2").arg(app.faults).arg(app.xs.size()));
        bool ordered = true;
        for (int i = 0; i < 60; ++i) ordered = ordered && app.publishedShotNumbers[i] == i + 1;
        check(ordered, "L. shots 1..60 in order, none repeated");
    }

    // ══ INVARIANTS (section 17) ════════════════════════════════════════════
    {
        check(!coordinateIndexValid(0, 5, 5), "INV. index 0 is never valid");
        check(!coordinateIndexValid(6, 5, 5), "INV. an index past the end is never valid");
        check(!coordinateIndexValid(5, 5, 4), "INV. x and y must BOTH hold the index");
        check(coordinateIndexValid(1, 1, 1), "INV. the first index of a one-shot list is valid");
        check(!coordinateIndexValid(1, 0, 0), "INV. nothing is valid in an empty list");

        // The exact field arithmetic: 11 coordinates, index 12 requested.
        check(!coordinateIndexValid(12, 11, 11),
              "INV. the Tablet-02 request (index 12 into 11 coordinates) is invalid");

        const AdoptionPlan clean = planBaselineAdoption(0, 10, true);
        check(!clean.shotsCountedWhileBlind,
              "INV. adopting counter 0 after a link loss reports nothing uncaptured");
        const AdoptionPlan dirty = planBaselineAdoption(1, 10, true);
        check(dirty.shotsCountedWhileBlind && dirty.uncapturedShots == 1,
              "INV. adopting counter 1 after a link loss reports one uncaptured shot");

        const ResetRequest none = decideCounterReset(9, 10);
        check(!none.shouldReset, "INV. no reset is requested at nine shots");
        const ResetRequest due = decideCounterReset(10, 10);
        check(due.shouldReset && due.shotsBeingRetired == 10,
              "INV. the reset at ten retires exactly ten shots");

        // The state that did not exist in the field build.
        PollDecision d = decidePoll(AcqState::ResettingCounter, 10, 10);
        check(d.kind == PollKind::ResetPending,
              "INV. target still reading 10 during OUR reset is Pending, not a jump");
        d = decidePoll(AcqState::ResettingCounter, 10, 0);
        check(d.kind == PollKind::ResetComplete && d.newBaseline == 0,
              "INV. target reading 0 during OUR reset completes it");
        d = decidePoll(AcqState::Acquiring, 0, 10);
        check(d.kind == PollKind::Fault && d.cause == FaultCause::CounterJumped,
              "INV. the same numbers OUTSIDE a reset are still a fault - the guard is intact");
    }

    // ══ CALL-ORDER BINDING ═════════════════════════════════════════════════
    // The driver above mirrors the production call order. This asserts the
    // production source still calls the sequencer, in that order, so the test
    // cannot quietly become a test of something the application no longer does.
    {
        QFile f(repoFile("ModReader/forms/tachuswidget.cpp"));
        const bool opened = f.open(QIODevice::ReadOnly | QIODevice::Text);
        check(opened, "CALLORDER. the production acquisition source is readable",
              f.fileName());
        if (opened) {
            const QString src = QString::fromUtf8(f.readAll());
            const int poll = src.indexOf(QLatin1String("m_seq.poll("));
            const int captured = src.indexOf(QLatin1String("noteCoordinateCaptured()"));
            const int failed = src.indexOf(QLatin1String("noteCoordinateReadFailed("));
            const int reset = src.indexOf(QLatin1String("maybeStartCounterReset("));
            check(poll > 0, "CALLORDER. production polls the shared sequencer");
            check(captured > 0, "CALLORDER. production derives the shot number from it");
            check(failed > 0, "CALLORDER. production reports a failed coordinate read to it");
            check(reset > 0, "CALLORDER. production asks it when to recycle the counter");
            check(poll < captured && captured < reset,
                  "CALLORDER. poll -> capture -> recycle, the order this harness drives");
            check(!src.contains(QLatin1String("QThread::msleep(2600)")),
                  "CALLORDER. the 2 600 ms reset sleep is gone from the acquisition path");
        }
    }
}
