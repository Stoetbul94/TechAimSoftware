#include "PaperFeedCoordinator.h"

namespace ta {
namespace target {

QString feedSkipReasonName(FeedSkipReason r)
{
    switch (r) {
    case FeedSkipReason::NotSkipped:         return QStringLiteral("fed");
    case FeedSkipReason::NotLiveMode:        return QStringLiteral("not Live mode");
    case FeedSkipReason::DurationZero:       return QStringLiteral("feed duration is zero");
    case FeedSkipReason::DuplicateShot:      return QStringLiteral("duplicate shot identity");
    case FeedSkipReason::RecoveryReplay:     return QStringLiteral("recovery replay");
    case FeedSkipReason::HistoricalLoad:     return QStringLiteral("historical session load");
    case FeedSkipReason::NotAccepted:        return QStringLiteral("shot not accepted");
    case FeedSkipReason::TargetDisconnected: return QStringLiteral("target disconnected");
    }
    return QStringLiteral("unknown");
}

PaperFeedCoordinator::PaperFeedCoordinator(MotorCommand motor, LogSink log)
    : m_motor(std::move(motor)), m_log(std::move(log))
{
}

void PaperFeedCoordinator::log(const QString& line) const
{
    if (m_log) m_log(line);
}

double PaperFeedCoordinator::sanitiseDuration(double seconds, QString* why)
{
    if (!(seconds == seconds)) {            // NaN
        if (why) *why = QStringLiteral("not a number - treated as disabled");
        return 0.0;
    }
    if (seconds < 0.0) {
        if (why) *why = QStringLiteral("negative duration refused - treated as disabled");
        return 0.0;
    }
    if (seconds > kMaxFeedSeconds) {
        if (why) *why = QStringLiteral("above the %1 s ceiling - clamped")
                            .arg(kMaxFeedSeconds, 0, 'f', 1);
        return kMaxFeedSeconds;
    }
    // 0.0 is legal and MEANS disabled; 1.0 and other decimals pass through
    // untouched, so the operator's existing values are never rewritten.
    return seconds;
}

void PaperFeedCoordinator::beginSession(const QString& sessionId)
{
    m_sessionId = sessionId;
    // Scope, do not clear globally: identities are already namespaced by
    // session, and clearing here keeps the set small across a long day.
    m_recentIdentities.clear();
    m_recentOrder.clear();
    m_queue.clear();
    log(QStringLiteral("paper feed: session %1 started").arg(sessionId));
}

void PaperFeedCoordinator::endSession()
{
    log(QStringLiteral("paper feed: session %1 ended (%2 feeds issued, %3 duplicates prevented)")
            .arg(m_sessionId).arg(m_feedsIssued).arg(m_duplicatesPrevented));
    m_sessionId.clear();
    m_queue.clear();
}

bool PaperFeedCoordinator::rememberIdentity(const QString& sessionId, qint64 identity)
{
    const QString key = QStringLiteral("%1#%2").arg(sessionId).arg(identity);
    if (m_recentIdentities.contains(key)) return false;   // already seen
    m_recentIdentities.insert(key);
    m_recentOrder.enqueue(key);
    while (m_recentOrder.size() > kRecentIdentityLimit)
        m_recentIdentities.remove(m_recentOrder.dequeue());
    return true;
}

FeedDecision PaperFeedCoordinator::onShotAccepted(const FeedRequest& req)
{
    FeedDecision d;

    // ── the exclusions, in the order that matters ──────────────────────────
    // Replay first: during recovery the shots ARE accepted and durably
    // recorded, so every other check would pass. Feeding paper for a shot
    // fired an hour ago is the worst failure this class can have.
    if (m_ctx.replaying) {
        d.skip = FeedSkipReason::RecoveryReplay;
        log(QStringLiteral("paper feed skipped: shot %1 - %2")
                .arg(req.shotIdentity).arg(feedSkipReasonName(d.skip)));
        return d;
    }
    if (!m_ctx.liveMode) {
        d.skip = FeedSkipReason::NotLiveMode;
        log(QStringLiteral("paper feed skipped: shot %1 - %2")
                .arg(req.shotIdentity).arg(feedSkipReasonName(d.skip)));
        return d;
    }
    if (!m_ctx.targetConnected) {
        d.skip = FeedSkipReason::TargetDisconnected;
        log(QStringLiteral("paper feed skipped: shot %1 - %2")
                .arg(req.shotIdentity).arg(feedSkipReasonName(d.skip)));
        return d;
    }

    // Duplicate protection BEFORE duration, so a repeated protocol frame is
    // recorded as a prevented duplicate rather than as a zero-duration skip.
    const QString sid = req.sessionId.isEmpty() ? m_sessionId : req.sessionId;
    if (!rememberIdentity(sid, req.shotIdentity)) {
        ++m_duplicatesPrevented;
        d.skip = FeedSkipReason::DuplicateShot;
        log(QStringLiteral("paper feed skipped: shot %1 - duplicate feed prevented")
                .arg(req.shotIdentity));
        return d;
    }

    // Shot type selects the duration. RC1 stored a sighter duration and then
    // passed only the match duration to the thread, so a sighter fed for the
    // match time; the duration now travels WITH the request.
    QString why;
    const double raw = (req.kind == ShotKind::Sighter)
                           ? m_ctx.sighterDurationSeconds
                           : m_ctx.matchDurationSeconds;
    const double duration = sanitiseDuration(raw, &why);
    if (!why.isEmpty())
        log(QStringLiteral("paper feed: duration %1 s %2").arg(raw).arg(why));

    if (duration <= 0.0) {
        d.skip = FeedSkipReason::DurationZero;
        log(QStringLiteral("paper feed skipped: shot %1 (%2) - %3")
                .arg(req.shotIdentity)
                .arg(req.kind == ShotKind::Sighter ? QStringLiteral("sighter")
                                                   : QStringLiteral("counted"))
                .arg(feedSkipReasonName(d.skip)));
        return d;
    }

    FeedRequest queued = req;
    queued.sessionId = sid;
    queued.durationSeconds = duration;
    m_queue.enqueue(queued);

    d.accepted = true;
    d.durationSeconds = duration;
    log(QStringLiteral("paper feed requested: shot %1 (%2) duration %3 s - queued (depth %4)")
            .arg(req.shotIdentity)
            .arg(req.kind == ShotKind::Sighter ? QStringLiteral("sighter")
                                               : QStringLiteral("counted"))
            .arg(duration, 0, 'f', 2)
            .arg(m_queue.size()));

    pumpQueue();
    return d;
}

void PaperFeedCoordinator::pumpQueue()
{
    // Re-entrancy guard. A shot arriving while the motor is running must not
    // start a second, overlapping register write — it waits its turn.
    if (m_motorBusy) return;
    if (!m_motor) {
        if (!m_queue.isEmpty())
            log(QStringLiteral("paper feed: no motor command bound - %1 request(s) dropped")
                    .arg(m_queue.size()));
        m_queue.clear();
        return;
    }

    m_motorBusy = true;
    while (!m_queue.isEmpty()) {
        const FeedRequest r = m_queue.dequeue();
        log(QStringLiteral("paper feed started: shot %1 duration %2 s")
                .arg(r.shotIdentity).arg(r.durationSeconds, 0, 'f', 2));
        const bool ok = m_motor(r.durationSeconds);
        if (ok) {
            ++m_feedsIssued;
            log(QStringLiteral("paper feed completed: shot %1").arg(r.shotIdentity));
        } else {
            // The shot is already safely recorded; a motor failure is a
            // hardware fault to report, never a reason to lose the shot.
            log(QStringLiteral("paper feed FAILED: shot %1 - motor command returned false")
                    .arg(r.shotIdentity));
        }
    }
    m_motorBusy = false;
}

bool PaperFeedCoordinator::requestManualFeed(double durationSeconds)
{
    QString why;
    const double duration = sanitiseDuration(durationSeconds, &why);
    if (!why.isEmpty())
        log(QStringLiteral("manual feed: duration %1 s %2").arg(durationSeconds).arg(why));

    if (!m_motor) {
        log(QStringLiteral("manual feed requested but no motor command is bound"));
        return false;
    }
    if (duration <= 0.0) {
        log(QStringLiteral("manual feed requested with zero duration - nothing done"));
        return false;
    }
    if (m_motorBusy) {
        log(QStringLiteral("manual feed requested while the motor is already feeding - ignored"));
        return false;
    }
    // Logged as MANUAL, distinctly from the automatic path, so the two can
    // never be confused when reading a field log.
    log(QStringLiteral("manual feed requested: duration %1 s").arg(duration, 0, 'f', 2));
    m_motorBusy = true;
    const bool ok = m_motor(duration);
    m_motorBusy = false;
    log(ok ? QStringLiteral("manual feed completed")
           : QStringLiteral("manual feed FAILED - motor command returned false"));
    return ok;
}

}} // namespace ta::target
