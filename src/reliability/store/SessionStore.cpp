#include "SessionStore.h"

#include "reliability/events/EventRegistry.h"
#include "reliability/reducer/SessionReducer.h"
#include "reliability/storage/StoragePaths.h"

#include <QFile>
#include <QFileInfo>

namespace ta {
namespace rel {

const char* healthName(Health h)
{
    switch (h) {
    case Health::Healthy:  return "Healthy";
    case Health::Slow:     return "Slow";
    case Health::Degraded: return "Degraded";
    case Health::Critical: return "Critical";
    case Health::ReadOnly: return "ReadOnly";
    case Health::Failed:   return "Failed";
    }
    return "Unknown";
}

SessionStore::SessionStore(QObject* parent)
    : QObject(parent)
{
}

SessionStore::~SessionStore() = default;

const WriterMetrics* SessionStore::metrics() const
{
    return m_manager ? &m_manager->metrics() : nullptr;
}

SessionStore::SeqTimes SessionStore::stamp() const
{
    return SeqTimes{m_clock->wallIso(), m_clock->nowMs()};
}

void SessionStore::setHealth(Health h)
{
    if (m_health == h)
        return;
    m_health = h;
    emit persistenceHealthChanged(h);
}

namespace {
DurabilityClass durabilityOf(const DomainEvent& event)
{
    const EventMeta* meta = EventRegistry::metaForEvent(event);
    return meta ? meta->durability : DurabilityClass::Sync;
}
} // namespace

bool SessionStore::reduceInto(quint64 seq, const DomainEvent& event,
                              const SeqTimes& t, ErrorInfo* err)
{
    // Build the envelope the reducer expects (hash fields irrelevant to it).
    EventEnvelope env;
    env.payloadVersion = eventPayloadVersion(event);
    env.sessionId = m_identity.sessionId;
    env.lane = m_identity.lane;
    env.seq = seq;
    env.wallTimestampIso = t.wallIso;
    env.monotonicMs = t.monoMs;
    env.eventType = QString::fromLatin1(eventTypeId(event));
    env.payload = event;
    env.previousHash = QByteArray(kHashHexLength, '0');
    env.currentHash = QByteArray(kHashHexLength, '0');

    const ReduceResult r = SessionReducer::apply(m_state, env);
    if (!r.ok) {
        if (err)
            *err = r.error;
        return false;
    }
    m_state = r.state;
    return true;
}

ReliabilityResult SessionStore::beginSession(const SessionHeader& header)
{
    if (m_active)
        return ReliabilityResult::failure(ReliabilityError::InvalidArgument,
            QStringLiteral("A session is already active."),
            QStringLiteral("beginSession called while active"));
    if (header.sessionId.isEmpty())
        return ReliabilityResult::failure(ReliabilityError::InvalidArgument,
            QStringLiteral("A session id is required."),
            QStringLiteral("empty sessionId"));

    // clock
    if (m_injectedClock) {
        m_clock = m_injectedClock;
    } else {
        m_ownClock = std::make_unique<SystemMonotonicClock>();
        m_clock = m_ownClock.get();
    }
    m_clock->start();

    m_identity.sessionId = header.sessionId;
    m_identity.lane = header.lane;
    m_identity.appVersion = header.appVersion;
    m_identity.deviceId = header.deviceId;

    // filename bits from the wall clock (UTC) + uuid8 from the session id
    const QString wall = m_clock->wallIso();   // yyyy-MM-ddTHH:mm:ss.zzz
    m_archiveYear = wall.left(4);
    m_archiveMonth = wall.mid(5, 2);
    const QString compact = wall.left(4) + wall.mid(5, 2) + wall.mid(8, 2)
            + QStringLiteral("T") + wall.mid(11, 2) + wall.mid(14, 2)
            + wall.mid(17, 2);
    QString uuid8 = header.sessionId;
    uuid8.remove(QLatin1Char('-'));
    uuid8 = uuid8.left(8);

    // journal manager (injected in tests, real file in production)
    m_queue = std::make_unique<PersistenceRetryQueue>(m_auxCap);
    if (m_injectedFile) {
        m_manager = std::make_unique<JournalManager>(m_identity, m_injectedFile);
    } else {
        const QString fileName =
            StoragePaths::sessionJournalFileName(compact, uuid8);
        const QString path = StoragePaths::currentSessionJournalPath(fileName);
        m_manager = std::make_unique<JournalManager>(m_identity, path);
    }

    // header event at seq 0 — must be durable
    SessionStarted started;
    started.sessionId = header.sessionId;
    started.schemaVersion = kJournalSchemaVersion;
    started.appVersion = header.appVersion;
    started.createdAtIso = wall;
    started.athlete = header.athlete;
    started.lane = header.lane;
    started.targetId = header.targetId;
    started.discipline = header.discipline;
    started.matchType = header.matchType;
    started.config = header.config;
    started.deviceId = header.deviceId;

    m_state = SessionState();
    m_nextSeq = 0;
    m_health = Health::Healthy;
    m_degradedMarkerPending = false;
    m_peakQueued = 0;

    const quint64 seq = m_nextSeq++;
    const SeqTimes t = stamp();
    ErrorInfo rerr;
    if (!reduceInto(seq, DomainEvent(started), t, &rerr))
        return ReliabilityResult::failureFrom(rerr);

    const AppendOutcome out = m_manager->append(seq, DomainEvent(started),
        t.wallIso, t.monoMs, DurabilityClass::Sync);
    if (!out.ok) {
        // The header must land durably; a session that cannot record its
        // own start is not recoverable and must not proceed silently.
        emit journalWriteFailed(m_manager->currentPath(), out.error.technicalDetail);
        return ReliabilityResult::failureFrom(out.error);
    }
    m_active = true;
    emit eventApplied(DomainEvent(started), false);
    emit stateChanged();
    return ReliabilityResult::success();
}

ReliabilityResult SessionStore::resumeSession(const RecoveredMatchState& recovered)
{
    if (m_active)
        return ReliabilityResult::failure(ReliabilityError::InvalidArgument,
            QStringLiteral("A session is already active."),
            QStringLiteral("resumeSession called while active"));
    if (recovered.sessionId.isEmpty() || recovered.journalPath.isEmpty())
        return ReliabilityResult::failure(ReliabilityError::InvalidArgument,
            QStringLiteral("Nothing to resume."),
            QStringLiteral("empty recovered session id/path"));
    if (!isWellFormedHashHex(recovered.lastLineHash))
        return ReliabilityResult::failure(ReliabilityError::InvalidArgument,
            QStringLiteral("The recovered session is missing its chain state."),
            QStringLiteral("lastLineHash not 32 hex"));

    // Drop any torn tail so appends chain onto the last VALID line. The
    // original bytes are preserved alongside as <name>.tornbak first (§15).
    if (recovered.validByteLength >= 0) {
        QFileInfo info(recovered.journalPath);
        if (info.exists() && info.size() > recovered.validByteLength) {
            QFile::copy(recovered.journalPath, recovered.journalPath
                        + QStringLiteral(".tornbak"));
            QFile f(recovered.journalPath);
            if (f.open(QIODevice::ReadWrite)) {
                f.resize(recovered.validByteLength);
                f.close();
            }
        }
    }

    if (m_injectedClock) {
        m_clock = m_injectedClock;
    } else {
        m_ownClock = std::make_unique<SystemMonotonicClock>();
        m_clock = m_ownClock.get();
    }
    m_clock->start();

    m_identity.sessionId = recovered.sessionId;
    m_identity.lane = recovered.state.lane;
    m_identity.appVersion = recovered.state.appVersion;
    m_identity.deviceId = recovered.state.deviceId;

    m_queue = std::make_unique<PersistenceRetryQueue>(m_auxCap);
    if (m_injectedFile)
        m_manager = std::make_unique<JournalManager>(m_identity, m_injectedFile);
    else
        m_manager = std::make_unique<JournalManager>(m_identity,
                                                     recovered.journalPath);
    // Seed the write-order chain + sequence from the recovered tail.
    m_manager->writer().resumeFrom(recovered.lastLineHash,
                                   recovered.lastValidSeq + 1);

    // Adopt the reducer-rebuilt state verbatim (sole authority — never
    // reconstructed from anything but the reducer).
    m_state = recovered.state;
    m_nextSeq = recovered.lastValidSeq + 1;
    m_health = Health::Healthy;
    m_degradedMarkerPending = false;
    m_peakQueued = 0;
    m_active = true;

    // Bracket the resume in the journal so replay is idempotent (§15): a crash
    // during recovery leaves RecoveryStarted without RecoveryCompleted and the
    // next run simply recovers again.
    {
        RecoveryStarted started;
        started.fromSeq = recovered.lastValidSeq;
        const quint64 seq = m_nextSeq++;
        const SeqTimes t = stamp();
        ErrorInfo e;
        reduceInto(seq, DomainEvent(started), t, &e);
        m_manager->append(seq, DomainEvent(started), t.wallIso, t.monoMs,
                          DurabilityClass::Sync);
    }
    {
        RecoveryCompleted done;
        done.resumedAtSeq = recovered.lastValidSeq;
        done.truncatedTail = recovered.recoveryClass == RecoveryClass::Recoverable;
        const quint64 seq = m_nextSeq++;
        const SeqTimes t = stamp();
        ErrorInfo e;
        reduceInto(seq, DomainEvent(done), t, &e);
        const AppendOutcome out = m_manager->append(seq, DomainEvent(done),
            t.wallIso, t.monoMs, DurabilityClass::Sync);
        if (!out.ok) {
            emit journalWriteFailed(m_manager->currentPath(),
                                    out.error.technicalDetail);
            // The resume itself is durable state in RAM; the marker will be
            // retried on the next submit's degraded path.
        }
    }

    emit stateChanged();
    return ReliabilityResult::success();
}

void SessionStore::enterDegradedOnce()
{
    setHealth(Health::Degraded);
    if (m_degradedMarkerPending)
        return;
    m_degradedMarkerPending = true;
    emitMarker(DomainEvent(PersistenceDegraded{m_queue->size()}), /*writeNow*/ false);
}

void SessionStore::emitMarker(const DomainEvent& marker, bool writeNow)
{
    const quint64 seq = m_nextSeq++;
    const SeqTimes t = stamp();
    ErrorInfo err;
    reduceInto(seq, marker, t, &err);   // markers never reduce-fail
    const DurabilityClass dur = durabilityOf(marker);
    if (writeNow) {
        m_manager->append(seq, marker, t.wallIso, t.monoMs, dur);
    } else {
        QueuedEvent q;
        q.seq = seq;
        q.event = marker;
        q.durability = dur;
        q.wallIso = t.wallIso;
        q.monoMs = t.monoMs;
        q.official = official(dur);
        m_queue->enqueue(q);
    }
    emit eventApplied(marker, false);
}

SubmitResult SessionStore::submit(const DomainEvent& event)
{
    SubmitResult result;
    if (!m_active) {
        result.error = ReliabilityResult::failure(ReliabilityError::InvalidArgument,
            QStringLiteral("No active session."),
            QStringLiteral("submit called before beginSession")).error;
        return result;
    }

    const quint64 seq = m_nextSeq;
    result.seq = seq;
    const DurabilityClass dur = durabilityOf(event);
    const SeqTimes t = stamp();

    // Never refuse to score (§9C applies to PERSISTENCE failures). A reducer
    // rejection means the event is ILLEGAL — it is refused to the caller and
    // NOT persisted; the sequence it would have taken is reclaimed so the
    // journal stays dense.
    ErrorInfo rerr;
    if (!reduceInto(seq, event, t, &rerr)) {
        result.ok = false;
        result.error = rerr;
        result.persistedDurably = false;
        return result;   // m_nextSeq NOT advanced
    }
    m_nextSeq++;
    result.ok = true;

    // Persist. While the queue is non-empty we MUST queue (preserve order);
    // otherwise attempt a direct durable write.
    if (!m_queue->isEmpty()) {
        QueuedEvent q{seq, event, dur, t.wallIso, t.monoMs, official(dur)};
        m_queue->enqueue(q);
        m_peakQueued = qMax(m_peakQueued, m_queue->size());
        result.persistedDurably = false;
    } else {
        const AppendOutcome out =
            m_manager->append(seq, event, t.wallIso, t.monoMs, dur);
        if (out.ok) {
            result.persistedDurably = true;
        } else if (m_manager->poisoned()) {
            // Torn tail — the file is unusable. RAM-only from here (§9E).
            QueuedEvent q{seq, event, dur, t.wallIso, t.monoMs, official(dur)};
            m_queue->enqueue(q);
            m_peakQueued = qMax(m_peakQueued, m_queue->size());
            setHealth(Health::Failed);
            emit criticalPersistenceFailure(
                QStringLiteral("The session journal is damaged; shots are being "
                               "held in memory only."));
            emit journalWriteFailed(m_manager->currentPath(),
                                    out.error.technicalDetail);
            result.persistedDurably = false;
        } else if (out.lineAppended) {
            // Bytes exist but durability (flush/fsync) was not confirmed. Do
            // NOT requeue (that would duplicate the line); report degraded.
            setHealth(Health::Degraded);
            result.persistedDurably = false;
        } else {
            // Clean write failure: nothing on disk. Queue + degrade.
            QueuedEvent q{seq, event, dur, t.wallIso, t.monoMs, official(dur)};
            m_queue->enqueue(q);
            m_peakQueued = qMax(m_peakQueued, m_queue->size());
            enterDegradedOnce();
            emit journalWriteFailed(m_manager->currentPath(),
                                    out.error.technicalDetail);
            result.persistedDurably = false;
        }
    }

    emit eventApplied(event, false);
    if (result.ok)
        emit stateChanged();
    return result;
}

bool SessionStore::pumpRetryQueue()
{
    if (!m_active || m_queue->isEmpty())
        return true;
    if (m_manager->poisoned())
        return false;   // cannot write to a torn file

    while (!m_queue->isEmpty()) {
        const QueuedEvent item = m_queue->front();
        const AppendOutcome out = m_manager->append(
            item.seq, item.event, item.wallIso, item.monoMs, item.durability);
        if (out.ok || out.lineAppended) {
            m_queue->popFront();   // bytes on disk either way
        } else {
            return false;          // still failing — stay degraded
        }
    }
    onDrainComplete();
    return true;
}

void SessionStore::onDrainComplete()
{
    // Record any dropped aux as AuxEventsDropped markers (never silent, §9E),
    // then close the episode with PersistenceRestored. Storage is writable
    // now, so these markers are written immediately.
    const QVector<DroppedRun> runs = m_queue->takeDroppedRuns();
    for (const DroppedRun& run : runs) {
        AuxEventsDropped d;
        d.firstSeq = run.firstSeq;
        d.lastSeq = run.lastSeq;
        d.count = run.count;
        emitMarker(DomainEvent(d), /*writeNow*/ true);
    }
    emitMarker(DomainEvent(PersistenceRestored{m_peakQueued}), /*writeNow*/ true);
    m_degradedMarkerPending = false;
    m_peakQueued = 0;
    setHealth(Health::Healthy);
}

ReliabilityResult SessionStore::closeSession(CloseReason reason)
{
    if (!m_active)
        return ReliabilityResult::failure(ReliabilityError::InvalidArgument,
            QStringLiteral("No active session to close."),
            QStringLiteral("closeSession called while inactive"));

    // Best-effort drain before closing so nothing is left only in RAM.
    pumpRetryQueue();

    ErrorInfo cerr;
    {
        const quint64 closeSeq = m_nextSeq++;
        const SeqTimes t = stamp();
        reduceInto(closeSeq, DomainEvent(SessionClosed{reason}), t, &cerr);
        m_manager->append(closeSeq, DomainEvent(SessionClosed{reason}),
                          t.wallIso, t.monoMs, DurabilityClass::Sync);
    }
    {
        const quint64 shutSeq = m_nextSeq++;
        const SeqTimes t = stamp();
        reduceInto(shutSeq, DomainEvent(CleanShutdown{}), t, &cerr);
        m_manager->append(shutSeq, DomainEvent(CleanShutdown{}),
                          t.wallIso, t.monoMs, DurabilityClass::Sync);
    }

    const StorageResult archived =
        m_manager->closeAndArchive(m_archiveYear, m_archiveMonth);
    m_active = false;
    if (!archived.ok)
        return ReliabilityResult::failure(ReliabilityError::FileWriteFailed,
            archived.operatorMessage, archived.technicalDetail,
            archived.affectedPath);
    return ReliabilityResult::success();
}

} // namespace rel
} // namespace ta
