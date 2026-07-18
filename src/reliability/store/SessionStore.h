#ifndef TA_REL_SESSIONSTORE_H
#define TA_REL_SESSIONSTORE_H

// Session Reliability Layer — SessionStore façade (M2, spec §3, §9, §10).
//
// The single production entry point: submit typed events → persist → reduce
// → notify. It owns the live SessionState, the journal manager, the RAM
// retry queue and the persistence-health state. QtCore only (QObject for
// signals). UI-thread, synchronous (spec §11 model — the API hides a future
// writer thread behind the same submit() contract).
//
// Never-refuse-to-score (spec §9C): submit() ALWAYS applies the event to the
// reducer, even when the journal write fails — a fired shot must score. The
// SubmitResult reports whether the write was durable. Failed writes queue
// (officials elastic, aux bounded) and drain when storage recovers, bracketed
// by PersistenceDegraded/Restored markers with AuxEventsDropped for any loss.

#include "JournalManager.h"
#include "MonotonicClock.h"
#include "PersistenceRetryQueue.h"
#include "reliability/reducer/SessionState.h"
#include "reliability/recovery/RecoveryTypes.h"

#include <QObject>

#include <memory>

namespace ta {
namespace rel {

// Persistence health (spec §10; M2 drives the Healthy/Degraded/Failed subset).
enum class Health { Healthy, Slow, Degraded, Critical, ReadOnly, Failed };

const char* healthName(Health h);

// Everything needed to open a session and write its header (seq 0).
struct SessionHeader {
    QString sessionId;          // UUID string (caller-generated)
    QString appVersion;
    QString athlete, lane, targetId, deviceId, matchType;
    Discipline discipline = Discipline::None;
    DisciplineConfig config;
};

struct SubmitResult {
    bool ok = false;            // reducer accepted the event
    quint64 seq = 0;            // assigned logical sequence
    bool persistedDurably = false;   // false => accepted-but-degraded (§9C)
    ErrorInfo error;            // reducer/serialize error, when !ok
};

class SessionStore : public QObject {
    Q_OBJECT
public:
    // Production: writes a real journal file under Sessions/Current.
    explicit SessionStore(QObject* parent = nullptr);
    ~SessionStore() override;

    // ── dependency injection (tests) ─────────────────────────────────────
    // Borrow a deterministic clock and/or an in-memory journal file. Must be
    // set BEFORE beginSession. Ownership stays with the caller.
    void setClockForTesting(IMonotonicClock* clock) { m_injectedClock = clock; }
    void setJournalFileForTesting(IJournalFile* file) { m_injectedFile = file; }
    void setAuxQueueCapForTesting(int cap) { m_auxCap = cap; }

    // ── lifecycle ────────────────────────────────────────────────────────
    // Starts the monotonic clock, opens the journal, writes SessionStarted at
    // seq 0 (must be durable — returns failure otherwise).
    ReliabilityResult beginSession(const SessionHeader& header);
    // M3 resume: reopen an existing (recovered) journal in append mode, adopt
    // the reducer-rebuilt state, seed the hash chain + sequence from the last
    // valid line (truncating any torn tail first), and record RecoveryStarted
    // / RecoveryCompleted. After this the session continues exactly as a live
    // one — submit() appends onto the recovered file.
    ReliabilityResult resumeSession(const RecoveredMatchState& recovered);

    // The main path. Never refuses to score.
    SubmitResult submit(const DomainEvent& event);
    // Writes MatchCompleted is the caller's job; this writes SessionClosed +
    // CleanShutdown and archives the file to Sessions/Archive/yyyy/MM.
    ReliabilityResult closeSession(CloseReason reason);

    // ── reads ────────────────────────────────────────────────────────────
    const SessionState& state() const { return m_state; }
    Health persistenceHealth() const { return m_health; }
    int unpersistedCount() const { return m_queue ? m_queue->size() : 0; }
    int droppedCount() const { return m_queue ? m_queue->totalDropped() : 0; }
    quint64 nextSequence() const { return m_nextSeq; }
    bool active() const { return m_active; }
    QString currentJournalPath() const
    { return m_manager ? m_manager->currentPath() : QString(); }
    const WriterMetrics* metrics() const;

    // ── retry pump ───────────────────────────────────────────────────────
    // Attempt to drain the retry queue in sequence order. Returns true when
    // the queue is (or becomes) empty. Production drives this from a QTimer
    // (250 ms → 5 s backoff); tests call it directly after clearing the fault.
    bool pumpRetryQueue();

signals:
    void eventApplied(const ta::rel::DomainEvent& event, bool replayed);
    void stateChanged();
    void persistenceHealthChanged(ta::rel::Health health);
    void criticalPersistenceFailure(QString operatorMessage);
    void journalWriteFailed(QString path, QString detail);

private:
    struct SeqTimes { QString wallIso; qint64 monoMs; };
    SeqTimes stamp() const;
    void setHealth(Health h);
    // Apply an event to the reducer at `seq` with a fixed timestamp (so the
    // reduced and written envelopes are byte-identical); updates state/lastSeq.
    bool reduceInto(quint64 seq, const DomainEvent& event,
                    const SeqTimes& t, ErrorInfo* err);
    // Create a store-generated marker at the next seq: reduce it and, when
    // storage is writable, append it. Used for PersistenceDegraded/Restored/
    // AuxEventsDropped. `writeNow` false queues it instead.
    void emitMarker(const DomainEvent& marker, bool writeNow);
    void enterDegradedOnce();
    void onDrainComplete();
    bool official(DurabilityClass d) const { return d == DurabilityClass::Sync; }

    // identity / archive partition (derived at beginSession)
    JournalIdentity m_identity;
    QString m_archiveYear, m_archiveMonth;

    std::unique_ptr<SystemMonotonicClock> m_ownClock;
    IMonotonicClock* m_clock = nullptr;
    IMonotonicClock* m_injectedClock = nullptr;
    IJournalFile* m_injectedFile = nullptr;
    int m_auxCap = PersistenceRetryQueue::kAuxCap;

    std::unique_ptr<JournalManager> m_manager;
    std::unique_ptr<PersistenceRetryQueue> m_queue;

    SessionState m_state;
    quint64 m_nextSeq = 0;
    Health m_health = Health::Healthy;
    bool m_active = false;
    bool m_degradedMarkerPending = false;   // one Degraded marker per episode
    int m_peakQueued = 0;
};

} // namespace rel
} // namespace ta

#endif // TA_REL_SESSIONSTORE_H
