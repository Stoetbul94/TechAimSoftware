#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// The acquisition SEQUENCE — extracted for the same reason decidePoll() was,
// and CALLED by production so the harness is not exercising a copy.
//
// decidePoll() answers "what does this counter reading mean". It cannot answer
// the questions the 2026-08-23 physical test actually failed on, because those
// are questions about a sequence held across polls:
//
//   * we asked the target to zero its counter - has it done so yet?
//   * which shot number does this coordinate belong to?
//   * after a reconnect, do the counter and the captured coordinates still
//     describe the same session?
//
// Those lived as loose members of a 3 000-line QWidget, so no test could reach
// them and both defects stayed invisible until a range day exposed them.
//
// WHAT THIS ENCODES
//   ACQ-FLUSH-001    the flush zeroed the application baseline immediately and
//                    left the hardware counter to a detached thread that slept
//                    2 600 ms first. Every poll in between read baseline 0
//                    against target 10 and stopped acquisition. 12/12 on three
//                    tablets. Here the baseline moves ONLY on proof.
//   ACQ-DESYNC-002   a reconnect adopted "target reports 1" while ten
//                    coordinates were held, and the shot index ran ahead of the
//                    arrays for the rest of the session.
//   ACQ-SENTINEL-003 the shot number is now DERIVED from the number of
//                    coordinates actually captured, so a shot number for which
//                    no coordinate exists cannot be produced at all.
//
// THE ONE INVARIANT (A)
//     shotNumber(n-th captured coordinate) == n
// It is not checked after the fact; it is the definition. Nothing else assigns
// a shot number.
//
// No Qt, no Modbus, no widget: that is what makes it testable, and the reason
// the earlier acquisition gates could only ever be proven by a human clicking
// through the application.
// ─────────────────────────────────────────────────────────────────────────────

#include "AcquisitionDecision.h"

namespace ta {
namespace target {

enum class SeqAction {
    Idle,               // nothing to do
    FetchCoordinates,   // genuine new shot(s): read slots, then confirm each
    AwaitResetProof,    // our reset is outstanding; judge nothing this tick
    IssueCounterReset,  // write the hardware counter register now
    ReportSynchronized, // baseline adopted from the target; NO shot events
    RaiseFault          // latched; acquisition stops until an operator acts
};

struct SeqStep {
    SeqAction  action = SeqAction::Idle;
    int        firstSlot = 0;            // FetchCoordinates: first slot index
    int        lastSlot = 0;             // FetchCoordinates: last slot index
    int        resetRegisterValue = 0;   // IssueCounterReset: value to write
    FaultCause cause = FaultCause::None;
    // Diagnostics carried out so a log line never re-derives what was decided.
    int        hardwareCounter = 0;
    int        baseline = 0;
    int        capturedShots = 0;
    int        uncapturedShots = 0;
    bool       shotsCountedWhileBlind = false;
};

class AcquisitionSequencer
{
public:
    // flushAt mirrors FLUSH_SHOOT_COUNT. resetDeadlineMs bounds how long the
    // target may take to confirm OUR reset before it becomes a visible fault.
    // resetDeadlineMs must comfortably exceed the slowest target the field has
    // shown. The RC2g build hard-coded a 2 600 ms wait for exactly one target;
    // the deadline is not a wait, it is the point at which silence becomes a
    // reported fault, so it is set well beyond any healthy response.
    explicit AcquisitionSequencer(int flushAt = 10, long long resetDeadlineMs = 6000)
        : m_flushAt(flushAt), m_resetDeadlineMs(resetDeadlineMs) {}

    // ── state, all readable so a log line never has to guess ──────────────
    AcqState state() const          { return m_state; }
    // The reason acquisition stopped, kept for as long as the fault is latched:
    // an operator who looks a minute later must still be told why, and the
    // pure decision reports None once a fault is merely being re-observed.
    FaultCause faultCause() const   { return m_faultCause; }
    int  hardwareBaseline() const   { return m_baseline; }
    int  priorTotal() const         { return m_priorTotal; }
    int  capturedShots() const      { return m_captured; }
    // INVARIANT A. The logical shot count IS the number of coordinates held.
    int  shotCount() const          { return m_captured; }
    bool resetOutstanding() const   { return m_state == AcqState::ResettingCounter; }

    // A session/numbering reset: everything returns to a proved-empty state and
    // the next poll re-adopts the target counter.
    void resetAll()
    {
        m_state = AcqState::Synchronizing;
        m_baseline = 0; m_priorTotal = 0; m_captured = 0;
        m_afterLinkLoss = false;
        m_resetExpected = 0; m_resetIssuedAtMs = 0;
        m_lastResetWriteMs = 0; m_resetWrites = 0; m_resetWriteFailed = false;
        m_faultCause = FaultCause::None;
    }

    // Coordinates are kept elsewhere (the widget owns the lists); tell the
    // sequencer how many there are so INVARIANT A is anchored to real data
    // rather than to a number this class incremented on its own.
    void setCapturedShots(int n) { m_captured = n < 0 ? 0 : n; }

    // The link dropped, or came back. A restored link must say out loud, at
    // the next adoption, whether the target counted shots we never captured.
    void noteLinkLost()     { m_state = AcqState::Synchronizing; }
    void noteLinkRestored() { m_state = AcqState::Synchronizing; m_afterLinkLoss = true; }
    void clearFault()       { m_state = AcqState::Synchronizing; }

    // ── one poll tick ─────────────────────────────────────────────────────
    SeqStep poll(int hardwareCounter, long long nowMs)
    {
        SeqStep s;
        s.hardwareCounter = hardwareCounter;
        s.baseline = m_baseline;
        s.capturedShots = m_captured;

        const PollDecision d = decidePoll(m_state, m_baseline, hardwareCounter);

        switch (d.kind) {
        case PollKind::ResetComplete:
            // Proved. Only now does the application count move, and the retired
            // shots join the prior total. There has been no moment in which the
            // two disagreed by accident.
            m_priorTotal += m_resetExpected;
            m_baseline = 0;
            m_state = AcqState::Acquiring;
            m_resetExpected = 0; m_resetIssuedAtMs = 0;
            m_lastResetWriteMs = 0; m_resetWrites = 0; m_resetWriteFailed = false;
            s.action = SeqAction::Idle;
            s.baseline = 0;
            return s;

        case PollKind::ResetPending:
            if (resetDeadlineExpired(nowMs - m_resetIssuedAtMs, m_resetDeadlineMs)) {
                m_state = AcqState::Fault;
                m_faultCause = FaultCause::ResetNotConfirmed;
                s.action = SeqAction::RaiseFault;
                s.cause = FaultCause::ResetNotConfirmed;
                return s;
            }
            // Deliberately NOT re-issued on a timer. A write that the transport
            // accepted has been delivered; writing again only restarts whatever
            // the target had already begun, which is how a slow-but-healthy
            // target gets pushed past its own deadline. A write is repeated
            // only when the transport REPORTED it failed - see
            // noteResetWriteFailed().
            if (m_resetWriteFailed && m_resetWrites < kMaxResetWrites) {
                m_resetWriteFailed = false;
                ++m_resetWrites;
                s.action = SeqAction::IssueCounterReset;
                s.resetRegisterValue = 0;
                return s;
            }
            s.action = SeqAction::AwaitResetProof;
            return s;

        case PollKind::Synchronized: {
            const AdoptionPlan p =
                planBaselineAdoption(hardwareCounter, m_captured, m_afterLinkLoss,
                                     m_baseline);
            m_baseline = p.newBaseline;
            m_priorTotal = p.newPriorTotal;
            m_state = AcqState::Acquiring;
            m_afterLinkLoss = false;
            s.action = SeqAction::ReportSynchronized;
            s.baseline = m_baseline;
            s.uncapturedShots = p.uncapturedShots;
            s.shotsCountedWhileBlind = p.shotsCountedWhileBlind;
            return s;
        }

        case PollKind::NewShots:
            m_baseline = d.newBaseline;
            m_state = AcqState::Acquiring;
            s.action = SeqAction::FetchCoordinates;
            s.firstSlot = d.firstNewShot;
            s.lastSlot = d.lastNewShot;
            s.baseline = m_baseline;
            return s;

        case PollKind::Fault:
            m_state = AcqState::Fault;
            if (d.cause != FaultCause::None)
                m_faultCause = d.cause;      // first report wins; it is the truth
            s.action = SeqAction::RaiseFault;
            s.cause = m_faultCause;
            return s;

        case PollKind::NoChange:
        case PollKind::ReadError:
        default:
            s.action = SeqAction::Idle;
            return s;
        }
    }

    // The caller has read a slot and holds real coordinates for it. Returns the
    // shot number to publish - always exactly the count of coordinates held,
    // which is why an out-of-range index cannot be produced.
    int noteCoordinateCaptured()
    {
        ++m_captured;
        return m_captured;
    }

    // The transport reported that the counter-reset write did not go out. The
    // next poll re-issues it, bounded, still inside the same deadline.
    void noteResetWriteFailed() { m_resetWriteFailed = true; }

    // The coordinate read FAILED, or the slot held nothing usable. The shot is
    // not published, not scored and not fed; acquisition stops so the operator
    // learns immediately. ACQ-READ-004: a failed read must never be decoded.
    SeqStep noteCoordinateReadFailed(int slot)
    {
        SeqStep s;
        m_state = AcqState::Fault;
        m_faultCause = FaultCause::None;   // transport, named by the caller
        s.action = SeqAction::RaiseFault;
        s.firstSlot = slot;
        s.lastSlot = slot;
        s.baseline = m_baseline;
        s.capturedShots = m_captured;
        return s;
    }

    // Called once the shots from this poll have been captured. The counter is
    // recycled every flushAt shots because the target coordinate slots are
    // indexed by it - a deliberate act by this application, never an anomaly.
    SeqStep maybeStartCounterReset(long long nowMs)
    {
        SeqStep s;
        s.hardwareCounter = m_baseline;
        s.baseline = m_baseline;
        s.capturedShots = m_captured;
        if (m_state != AcqState::Acquiring) return s;

        const ResetRequest r = decideCounterReset(m_baseline, m_flushAt);
        if (!r.shouldReset) return s;

        m_state = AcqState::ResettingCounter;
        m_resetExpected = r.shotsBeingRetired;
        m_resetIssuedAtMs = nowMs;
        m_lastResetWriteMs = nowMs;
        m_resetWrites = 1;
        m_resetWriteFailed = false;
        s.action = SeqAction::IssueCounterReset;
        s.resetRegisterValue = 0;
        return s;
    }

private:
    static const int kMaxResetWrites = 3;

    int m_flushAt;
    long long m_resetDeadlineMs;

    AcqState m_state = AcqState::Synchronizing;
    int  m_baseline = 0;
    int  m_priorTotal = 0;
    int  m_captured = 0;
    bool m_afterLinkLoss = false;

    FaultCause m_faultCause = FaultCause::None;
    int  m_resetExpected = 0;
    long long m_resetIssuedAtMs = 0;
    long long m_lastResetWriteMs = 0;
    int  m_resetWrites = 0;
    bool m_resetWriteFailed = false;
};

}} // namespace ta::target
