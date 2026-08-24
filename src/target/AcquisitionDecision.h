#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// The acquisition decision — extracted so it can be tested, and CALLED by
// production so the test is not exercising a copy.
//
// TachusWidget::checkForNewShots() delegates here. There is exactly one
// implementation of "what does this counter reading mean", and both the
// application and the harness run it.
//
// Why it is a free function on a small state struct: the decision needs no Qt,
// no widget, no Modbus and no window. Keeping it that way is what makes it
// testable at all - the previous version lived inside a QWidget that pulls in
// the whole vendored QModMaster, which is why the four gates could only ever be
// proven by a human clicking through the real application.
//
// HISTORY THIS ENCODES
//   RESTART-001  a baseline the application ASSUMED rather than READ let a
//                counter mismatch silently reject every shot, forever.
//   SYNC-001     adopting the baseline looked, to the caller, exactly like new
//                shots arriving - two phantom shots, two scores, two feeds.
//                Hence an explicit Kind: only NewShots may fetch coordinates.
//   ACQ-FLUSH-001 the 10-shot flush zeroed the APPLICATION baseline at once and
//                left the HARDWARE counter to be zeroed 2 600 ms later by a
//                detached thread. The 100 ms poll ran ~26 times inside that
//                window, read baseline 0 against target 10, and correctly
//                concluded that ten shots had been missed - so it stopped
//                acquisition. Measured 12/12 on three tablets, 2026-08-23,
//                every time 57-156 ms after the reset. The guard was right;
//                the flush was lying to it. There is now an explicit
//                ResettingCounter state: the application knows it asked for
//                the reset, the baseline moves only when the TARGET confirms
//                it, and no interval exists in which the two disagree by
//                accident.
//   ACQ-DESYNC-002 reconnect adopted the target counter without re-proving the
//                relationship to the captured coordinates. getShootCount()
//                then ran one index ahead of the arrays for the rest of the
//                session, every later shot read past the end, and the -1
//                sentinel scored 10.8. planBaselineAdoption() is where an
//                adoption must now prove it keeps the invariant.
// ─────────────────────────────────────────────────────────────────────────────

namespace ta {
namespace target {

enum class AcqState {
    Synchronizing,
    Acquiring,
    ResettingCounter,  // WE asked the target to zero its counter; awaiting proof
    Fault
};

enum class PollKind {
    NoChange,      // counter unchanged - nothing to do
    Synchronized,  // baseline adopted FROM the target; emits no shot events
    NewShots,      // genuine new shot(s); the ONLY kind that may fetch coords
    ResetPending,  // our own reset has not landed yet; judge nothing
    ResetComplete, // the target confirmed the reset; baseline may move to 0
    Fault,         // anomaly that cannot be resolved safely; latched
    ReadError      // transport failure; the link layer handles it
};

enum class FaultCause {
    None,
    CounterJumped,
    CounterWentBackwards,
    ResetNotConfirmed,     // the target never reported 0 within the deadline
    ShotDuringReset,       // a shot landed inside the reset window
    AdoptionWouldDesync    // a baseline adoption cannot keep INVARIANT A
};

struct PollDecision {
    PollKind   kind = PollKind::NoChange;
    AcqState   nextState = AcqState::Acquiring;
    int        newBaseline = 0;
    int        firstNewShot = 0;   // valid only when kind == NewShots
    int        lastNewShot = 0;    // valid only when kind == NewShots
    FaultCause cause = FaultCause::None;
    int        delta = 0;
};

// `hardwareCounter` is what the TARGET reports. `baseline` is what the
// application currently believes. `state` is the current acquisition state.
inline PollDecision decidePoll(AcqState state, int baseline, int hardwareCounter)
{
    PollDecision d;
    d.delta = hardwareCounter - baseline;
    d.newBaseline = baseline;

    // SYNCHRONIZING: whatever the target says becomes the truth, and NOTHING is
    // emitted. Residue from a previous session is absorbed here rather than
    // replayed as shots or left to poison every later comparison.
    if (state == AcqState::Synchronizing) {
        d.kind = PollKind::Synchronized;
        d.nextState = AcqState::Acquiring;
        d.newBaseline = hardwareCounter;
        return d;
    }

    // RESETTING: the application itself asked the target to zero its counter,
    // so the disagreement below is DELIBERATE and must not be read as lost
    // shots. Nothing is emitted and the baseline does not move until the
    // target has actually reported 0 - that is what removes ACQ-FLUSH-001's
    // 2.6-second window rather than papering over it. The caller owns the
    // deadline; a reset that never lands becomes ResetNotConfirmed, which is
    // a fault an operator can see, not a silent stall.
    if (state == AcqState::ResettingCounter) {
        if (hardwareCounter == 0) {
            d.kind = PollKind::ResetComplete;
            d.nextState = AcqState::Acquiring;
            d.newBaseline = 0;
            return d;
        }
        if (hardwareCounter == baseline) {
            d.kind = PollKind::ResetPending;
            d.nextState = AcqState::ResettingCounter;
            return d;
        }
        // The counter moved to something that is neither "not yet reset" nor
        // "reset". A shot fired inside the window is the plausible cause. Its
        // coordinate slot cannot be trusted once the counter recycles, so this
        // is reported rather than guessed at.
        d.kind = PollKind::Fault;
        d.nextState = AcqState::Fault;
        d.cause = FaultCause::ShotDuringReset;
        return d;
    }

    // A latched fault does not clear itself because a later poll looks normal.
    // Clearing silently would hide the very gap the operator was warned about.
    if (state == AcqState::Fault) {
        d.kind = PollKind::Fault;
        d.nextState = AcqState::Fault;
        return d;
    }

    if (d.delta == 0) {
        d.kind = PollKind::NoChange;
        d.nextState = AcqState::Acquiring;
        return d;
    }

    if (d.delta == 1) {
        d.kind = PollKind::NewShots;
        d.nextState = AcqState::Acquiring;
        d.firstNewShot = baseline + 1;
        d.lastNewShot = hardwareCounter;
        d.newBaseline = hardwareCounter;
        return d;
    }

    // Everything else is an anomaly and MUST NOT be discarded in silence. The
    // baseline is deliberately NOT advanced: adopting the gap would hide the
    // fact that shots may have been missed.
    d.kind = PollKind::Fault;
    d.nextState = AcqState::Fault;
    d.cause = (d.delta < 0) ? FaultCause::CounterWentBackwards
                            : FaultCause::CounterJumped;
    return d;
}

// ─────────────────────────────────────────────────────────────────────────────
// WHAT SHOULD THIS POLL TICK DO AT ALL?
//
// Extracted for the same reason decidePoll() was: the gating lived as a run of
// early returns inside a QWidget slot, so no test could reach it and a wrong
// gate was invisible until hardware proved it.
//
// LOGIN-LINK-001 (2026-08-10). One early return owned TWO unrelated concerns:
//
//     if (m_onLoginPage) return;   // skipped acquisition AND link health
//
// Shot acquisition SHOULD be suspended on the home page. Link health should
// not be - a cable pulled on the home screen is exactly as broken as one
// pulled mid-series. In the field the target was unplugged on the home page,
// the application kept reporting a healthy COM7, Windows re-enumerated the
// adapter to COM8 on replug, and nothing noticed either event.
// ─────────────────────────────────────────────────────────────────────────────

enum class PollAction {
    Idle,           // nothing to do this tick
    Reconnect,      // link is down - drive rediscovery (ANY page)
    ProbeLiveness,  // home page - prove the target still answers
    Acquire         // shooting page, link healthy - read the shot counter
};

struct PollActionDecision {
    PollAction action = PollAction::Idle;
    int nextLivenessTick = 0;
};

// `livenessTick` is the caller's current counter; `livenessPeriod` is how many
// 100 ms ticks between home-page probes. A probe is a real Modbus read, so it
// runs at ~1 Hz rather than every tick.
inline PollActionDecision decidePollAction(bool isLive,
                                           bool linkConnected,
                                           bool onLoginPage,
                                           int livenessTick,
                                           int livenessPeriod)
{
    PollActionDecision d;
    d.nextLivenessTick = livenessTick;

    // Demo/simulation has no target to poll; shots come from the UI.
    if (!isLive) {
        d.action = PollAction::Idle;
        return d;
    }

    // Link health FIRST, and deliberately before the page test: reconnection
    // must be reachable from the home page. This ordering IS the fix.
    if (!linkConnected) {
        d.action = PollAction::Reconnect;
        return d;
    }

    if (onLoginPage) {
        const int t = livenessTick + 1;
        if (t >= livenessPeriod) {
            d.action = PollAction::ProbeLiveness;
            d.nextLivenessTick = 0;
        } else {
            d.action = PollAction::Idle;
            d.nextLivenessTick = t;
        }
        return d;
    }

    // Shooting screen: acquisition proper. The counter read is itself the
    // liveness evidence here, so no separate probe is needed.
    d.action = PollAction::Acquire;
    d.nextLivenessTick = 0;
    return d;
}


// ─────────────────────────────────────────────────────────────────────────────
// OUR OWN COUNTER RESET (ACQ-FLUSH-001)
//
// The target's coordinate slots are indexed by its shot counter, so the counter
// is recycled every FLUSH_SHOOT_COUNT shots. That recycle is a deliberate act
// by this application - it is not an anomaly and must never be judged by the
// lost-shot guard. The sequence is now:
//
//     counter reaches the flush point
//         -> shouldReset, remembering what the target should still read
//         -> write the hardware register, then verify by READING IT BACK
//         -> baseline moves to 0 only on ResetComplete
//         -> not confirmed within the deadline -> ResetNotConfirmed fault
//
// The application baseline never holds a value it has not proved.
// ─────────────────────────────────────────────────────────────────────────────

struct ResetRequest {
    bool shouldReset = false;
    int  expectedHardwareCounter = 0;   // what the target must still report
    int  shotsBeingRetired = 0;         // added to the prior-total on completion
};

inline ResetRequest decideCounterReset(int hardwareBaseline, int flushAt)
{
    ResetRequest r;
    if (flushAt > 0 && hardwareBaseline == flushAt) {
        r.shouldReset = true;
        r.expectedHardwareCounter = hardwareBaseline;
        r.shotsBeingRetired = hardwareBaseline;
    }
    return r;
}

// The deadline is the caller's clock; keeping the arithmetic here means the
// harness proves the same rule the application runs.
inline bool resetDeadlineExpired(long long elapsedMs, long long deadlineMs)
{
    return deadlineMs > 0 && elapsedMs >= deadlineMs;
}

// ─────────────────────────────────────────────────────────────────────────────
// BASELINE ADOPTION (ACQ-DESYNC-002)
//
// Adopting a counter is not merely an assignment. On 2026-08-23 a reconnect on
// Tablet-02 read "target reports 1" while the application had captured 10
// coordinates, assigned the baseline and resumed. Every later shot then asked
// for a coordinate index one past the end of the arrays, and the accessor's -1
// sentinel scored 10.8 for the rest of the session - three times, in three
// sessions, including a plain 50 m Prone qualification with no finals code and
// no incident report anywhere near it.
//
// An adoption must therefore state what it does to the relationship between
// the hardware counter, the retired total and the captured coordinates, and
// say plainly when shots were counted that this application never captured.
// ─────────────────────────────────────────────────────────────────────────────

struct AdoptionPlan {
    int  newBaseline = 0;        // the hardware counter, adopted
    int  newPriorTotal = 0;      // retired shots, rebased to keep the identity
    bool identityHolds = false;  // newPriorTotal + newBaseline == capturedShots
    bool shotsCountedWhileBlind = false;   // the target counted, we did not
    int  uncapturedShots = 0;
    FaultCause cause = FaultCause::None;
};

// `capturedShots` is how many coordinate records the application actually
// holds - the only count that can be proved from data rather than believed.
inline AdoptionPlan planBaselineAdoption(int hardwareCounter,
                                         int capturedShots,
                                         bool afterLinkLoss)
{
    AdoptionPlan p;
    p.newBaseline = hardwareCounter < 0 ? 0 : hardwareCounter;
    const int rebased = capturedShots - p.newBaseline;
    p.newPriorTotal = rebased > 0 ? rebased : 0;
    p.identityHolds = (p.newPriorTotal + p.newBaseline == capturedShots);

    // A counter above zero when the link comes back means the target counted
    // shots while this application was blind. They were never captured and
    // cannot be reconstructed - the slots are overwritten once the counter
    // recycles. That is an EST interruption for the operator to record, not a
    // number for the software to invent.
    if (afterLinkLoss && p.newBaseline > 0) {
        p.shotsCountedWhileBlind = true;
        p.uncapturedShots = p.newBaseline;
        p.cause = FaultCause::AdoptionWouldDesync;
    }
    return p;
}

// ─────────────────────────────────────────────────────────────────────────────
// COORDINATE INDEX VALIDITY (ACQ-SENTINEL-003)
//
// getXCord()/getYCord() answered -1 for an index they did not hold. -1 is a
// legal coordinate: -1.00 mm on both axes is 1.41 mm from centre, which on a
// 50 m rifle target scores 10.8. An internal indexing error therefore left the
// application as a plausible score with no error anywhere. Validity is asked
// BEFORE scoring is reached, and the answer is a bool, not a number that has
// to be recognised.
// ─────────────────────────────────────────────────────────────────────────────

inline bool coordinateIndexValid(int oneBasedIndex, int xCount, int yCount)
{
    return oneBasedIndex >= 1 && oneBasedIndex <= xCount && oneBasedIndex <= yCount;
}

}} // namespace ta::target
