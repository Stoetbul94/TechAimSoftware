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
// ─────────────────────────────────────────────────────────────────────────────

namespace ta {
namespace target {

enum class AcqState { Synchronizing, Acquiring, Fault };

enum class PollKind {
    NoChange,      // counter unchanged - nothing to do
    Synchronized,  // baseline adopted FROM the target; emits no shot events
    NewShots,      // genuine new shot(s); the ONLY kind that may fetch coords
    Fault,         // anomaly that cannot be resolved safely; latched
    ReadError      // transport failure; the link layer handles it
};

enum class FaultCause { None, CounterJumped, CounterWentBackwards };

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

}} // namespace ta::target
