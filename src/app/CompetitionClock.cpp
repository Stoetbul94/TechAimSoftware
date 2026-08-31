#include "app/CompetitionClock.h"

namespace ta {

CompetitionClock::CompetitionClock(QObject* parent)
    : QObject(parent)
    , m_clock(&m_ownClock)
{
    // The monotonic origin is set once, here, and never restarted. Restarting
    // it would move every anchor that had already been taken against it.
    m_ownClock.start();
}

void CompetitionClock::setClockForTesting(rel::IMonotonicClock* clock)
{
    m_clock = clock ? clock : static_cast<rel::IMonotonicClock*>(&m_ownClock);
}

qint64 CompetitionClock::monoNowMs() const
{
    return m_clock->nowMs();
}

void CompetitionClock::start(int elapsedSecondsNow)
{
    m_anchorElapsed = elapsedSecondsNow;
    m_anchorMonoMs  = monoNowMs();
    m_frozenElapsed = elapsedSecondsNow;
    if (!m_running) {
        m_running = true;
        emit runningChanged();
    }
}

void CompetitionClock::stop()
{
    if (!m_running)
        return;
    // Freeze where we actually are, not where the last UI tick thought we
    // were. A stop that arrives between ticks must not discard that fraction's
    // worth of whole seconds.
    m_frozenElapsed = elapsedSeconds();
    m_running = false;
    emit runningChanged();
}

void CompetitionClock::reanchor(int elapsedSecondsNow)
{
    m_anchorElapsed = elapsedSecondsNow;
    m_anchorMonoMs  = monoNowMs();
    m_frozenElapsed = elapsedSecondsNow;
}

int CompetitionClock::elapsedSeconds() const
{
    if (!m_running)
        return m_frozenElapsed;
    const qint64 deltaMs = monoNowMs() - m_anchorMonoMs;
    // Truncating division: elapsed only advances on a WHOLE second, so the
    // displayed countdown never jumps a second early. Negative anchors (Jury
    // credit) are preserved because the anchor is added, not clamped.
    return m_anchorElapsed + static_cast<int>(deltaMs / 1000);
}

} // namespace ta
