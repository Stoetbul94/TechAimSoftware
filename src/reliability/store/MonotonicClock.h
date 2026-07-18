#ifndef TA_REL_MONOTONICCLOCK_H
#define TA_REL_MONOTONICCLOCK_H

// Session Reliability Layer — clock abstraction (M2).
//
// The SessionStore never reads a global clock directly; it goes through
// IMonotonicClock so tests are fully deterministic. `nowMs()` is monotonic
// milliseconds since the session began (the store starts the clock at
// beginSession); `wallIso()` is the UTC wall time for the envelope `tw`
// field, formatted exactly as kWallTimestampFormat.

#include "reliability/events/EventEnvelope.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QString>

namespace ta {
namespace rel {

class IMonotonicClock {
public:
    virtual ~IMonotonicClock() = default;
    virtual void start() = 0;              // reset monotonic origin to 0
    virtual qint64 nowMs() const = 0;      // monotonic ms since start()
    virtual QString wallIso() const = 0;   // UTC ISO with ms
};

// Production clock: QElapsedTimer for monotonic ms, system UTC for wall time.
class SystemMonotonicClock : public IMonotonicClock {
public:
    void start() override { m_timer.start(); }
    qint64 nowMs() const override
    {
        return m_timer.isValid() ? m_timer.elapsed() : 0;
    }
    QString wallIso() const override
    {
        return QDateTime::currentDateTimeUtc().toString(
            QLatin1String(kWallTimestampFormat));
    }

private:
    QElapsedTimer m_timer;
};

// Deterministic test clock: monotonic ms and wall time are set by the test.
class ManualClock : public IMonotonicClock {
public:
    void start() override { m_nowMs = 0; }
    qint64 nowMs() const override { return m_nowMs; }
    QString wallIso() const override { return m_wallIso; }

    void setNowMs(qint64 ms) { m_nowMs = ms; }
    void advance(qint64 ms) { m_nowMs += ms; }
    void setWallIso(const QString& iso) { m_wallIso = iso; }

private:
    qint64 m_nowMs = 0;
    QString m_wallIso = QStringLiteral("2026-07-17T10:00:00.000");
};

} // namespace rel
} // namespace ta

#endif // TA_REL_MONOTONICCLOCK_H
