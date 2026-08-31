#ifndef TECHAIM_COMPETITIONCLOCK_H
#define TECHAIM_COMPETITIONCLOCK_H

// B3 — the authoritative competition clock.
//
// THE PROBLEM THIS EXISTS TO FIX.
//
// Competition elapsed time for Open Practice, Qualification, 50 m Prone and
// 50 m 3P Qualification was counted by a QML Timer doing `gameTime++` once per
// second. The UI tick WAS the clock. On Windows that is survivable: the
// application owns a dedicated lane PC and stays in the foreground. On Android
// it is not, because Qt stops delivering timer events while the activity is
// backgrounded - behind a USB permission dialog, a notification shade, a
// screen-off - and a stopped tick means competition time simply does not
// accrue. The athlete silently gains every second the tablet was not in front.
//
// A clock that under-counts in the athlete's favour is worse than one that
// fails loudly, because nothing about the session looks wrong afterwards.
//
// THE FIX. Elapsed time is derived from a monotonic source and the UI tick
// only READS it. The tick may fire late, early, or not at all; the answer does
// not change. This mirrors what Finals3PController and Finals10mController
// already do - one monotonic clock, remaining recomputed rather than
// decremented - and brings the qualification clock onto the same footing.
//
// WHAT IT DELIBERATELY DOES NOT DO.
//
// It does not decide when a competition starts, stops or pauses. Backgrounding
// an Android activity is NOT a competition event, and this class has no
// opinion about it: callers start and stop the clock from competition state,
// exactly as they did when the Timer was the clock. All that changes is where
// the elapsed number comes from.
//
// It does not own the phase, the duration, or the remaining time. Callers keep
// computing `remaining = total - elapsed`, including the case where elapsed
// legitimately goes negative after an authorised Jury time credit.
//
// The clock is injectable (IMonotonicClock) so tests can advance time by hours
// without waiting, and so the background case can be tested at all - there is
// no way to background a headless harness.

#include "reliability/store/MonotonicClock.h"

#include <QObject>

namespace ta {

class CompetitionClock : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit CompetitionClock(QObject* parent = nullptr);

    // Takes ownership of nothing: the clock must outlive this object. Used by
    // tests to substitute ManualClock; production passes nothing and gets the
    // QElapsedTimer-backed SystemMonotonicClock.
    void setClockForTesting(rel::IMonotonicClock* clock);

    // Anchor and run. `elapsedSecondsNow` is the elapsed value the caller is
    // already holding - 0 for a fresh start, the restored value after a crash
    // recovery, or a Jury-adjusted value. From here elapsedSeconds() returns
    // that anchor plus real time passed.
    Q_INVOKABLE void start(int elapsedSecondsNow);

    // Stop accruing. The elapsed value is frozen where it stands and survives
    // until the next start(), so a paused match resumes rather than restarts.
    Q_INVOKABLE void stop();

    // Re-anchor WITHOUT changing running state. For the two places that assign
    // elapsed time from outside while the clock may be running: loading a saved
    // match, and restoring/adjusting remaining time. Without this the monotonic
    // base would still refer to the old anchor and the assignment would be
    // silently undone on the next tick.
    Q_INVOKABLE void reanchor(int elapsedSecondsNow);

    // Authoritative elapsed seconds. May be negative - see the Jury credit note
    // above; that is the existing, correct behaviour and is preserved.
    Q_INVOKABLE int elapsedSeconds() const;

    bool running() const { return m_running; }

signals:
    void runningChanged();

private:
    qint64 monoNowMs() const;

    rel::SystemMonotonicClock m_ownClock;
    rel::IMonotonicClock*     m_clock = nullptr;   // never null after construction
    bool    m_running       = false;
    qint64  m_anchorMonoMs  = 0;    // monotonic reading when the anchor was set
    int     m_anchorElapsed = 0;    // elapsed seconds at that moment
    int     m_frozenElapsed = 0;    // elapsed while stopped
};

} // namespace ta

#endif // TECHAIM_COMPETITIONCLOCK_H
