#ifndef TA_REL_PERSISTENCERETRYQUEUE_H
#define TA_REL_PERSISTENCERETRYQUEUE_H

// Session Reliability Layer — RAM retry queue (M2, spec §9C-E, §11 D11).
//
// Holds events that could not be persisted while storage is unavailable so
// the reducer can still score them (never-refuse-to-score, §9C). On restore
// the store drains this queue IN SEQUENCE ORDER back to the journal.
//
// Capacity policy (spec §9E, decision D11):
//   - "official" events (durability class Sync — official shots, missing
//     shots, corrections, penalties, stage/lifecycle) are ELASTIC: never
//     dropped (a fired shot must always be recoverable).
//   - "aux" events (Flush/Append — sighters, window, command, timer,
//     rejected, diagnostics) are bounded at kAuxCap; when full, the OLDEST
//     aux is dropped and its seq recorded. Loss is recorded, never silent:
//     drained drops become AuxEventsDropped markers (§9E), coalesced into
//     contiguous runs so the validator can reconcile them exactly.

#include "reliability/events/DomainEvent.h"
#include "reliability/events/EventTypes.h"

#include <QList>
#include <QString>
#include <QVector>

namespace ta {
namespace rel {

struct QueuedEvent {
    quint64 seq = 0;
    DomainEvent event;
    DurabilityClass durability = DurabilityClass::Sync;
    QString wallIso;
    qint64 monoMs = 0;
    bool official = false;         // Sync class => never dropped
};

// A coalesced run of dropped aux seqs (contiguous, fully dropped).
struct DroppedRun {
    quint64 firstSeq = 0;
    quint64 lastSeq = 0;
    qint32 count = 0;
};

class PersistenceRetryQueue {
public:
    // Aux cap per spec §9E / decision D11 (~2 MB at ~500 B/event).
    static constexpr int kAuxCap = 4096;

    explicit PersistenceRetryQueue(int auxCap = kAuxCap) : m_auxCap(auxCap) {}

    // Enqueue a failed event. If it is aux and the aux population would
    // exceed the cap, the OLDEST queued aux is dropped and recorded first.
    void enqueue(const QueuedEvent& item);

    bool isEmpty() const { return m_items.isEmpty(); }
    int size() const { return m_items.size(); }
    int officialCount() const;
    int auxCount() const { return m_auxCount; }

    // Peek the front (lowest-seq) item for a drain attempt.
    const QueuedEvent& front() const { return m_items.first(); }
    // Remove the front item after it has been successfully re-appended.
    void popFront();

    // Coalesced dropped runs accumulated so far (ascending). The store emits
    // one AuxEventsDropped per run at drain, then clears them.
    QVector<DroppedRun> takeDroppedRuns();
    bool hasDrops() const { return !m_droppedSeqs.isEmpty(); }
    int totalDropped() const { return m_totalDropped; }

private:
    QList<QueuedEvent> m_items;     // ascending by enqueue (== seq) order
    int m_auxCap;
    int m_auxCount = 0;
    QList<quint64> m_droppedSeqs;   // ascending; coalesced on take
    int m_totalDropped = 0;
};

} // namespace rel
} // namespace ta

#endif // TA_REL_PERSISTENCERETRYQUEUE_H
