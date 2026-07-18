#ifndef TA_REL_JOURNALWRITER_H
#define TA_REL_JOURNALWRITER_H

// Session Reliability Layer — synchronous journal writer (M1, step 10).
//
// M1 scope: a testable, dependency-injected writer. It assigns sequence
// numbers, serializes deterministically, maintains the hash chain, appends
// one complete LF-terminated line per event, and applies the per-event
// durability policy (registry) with an optional override. No retry queue,
// no degraded mode, no production integration — those are M2.
//
// Append result semantics (exact):
//   ok == true   line fully appended AND the requested durability level
//                (flush / sync) succeeded; envelope in the outcome is the
//                completed authoritative envelope; writer state advanced.
//   ok == false, lineAppended == false
//                nothing of the line reached the file; writer state
//                (sequence, chain) is UNCHANGED and the same event may be
//                retried later (M2 queue).
//   ok == false, lineAppended == true
//                the bytes were appended but flush/sync failed — the event
//                occupies its seq and hash-chain slot; writer state
//                advanced; durability was NOT achieved.
//   A PARTIAL physical write poisons the writer (failed() == true): the
//   on-disk tail is torn and further appends would corrupt the chain; the
//   owner must reopen/recover. A failed append is never reported as ok.
//
// Timestamps are CALLER-SUPPLIED (tw / tm) — the writer never reads a
// clock, which keeps every test deterministic (production clocks arrive
// with SessionStore in M2).

#include "reliability/core/ReliabilityResult.h"
#include "reliability/events/EventEnvelope.h"

#include <QFile>
#include <QString>

namespace ta {
namespace rel {

// Injection seam for fault tests (spec §25.5). Implementations report
// bytes actually written so short writes are observable.
class IJournalFile {
public:
    virtual ~IJournalFile() = default;
    virtual bool open() = 0;                       // append mode
    virtual bool isOpen() const = 0;
    virtual qint64 write(const QByteArray& bytes) = 0;  // -1 = error
    virtual bool flush() = 0;
    virtual bool sync() = 0;
    virtual QString path() const = 0;
    virtual QString lastErrorDetail() const = 0;
};

// Production implementation over QFile + the M0 StorageSync boundary.
class FileJournalFile : public IJournalFile {
public:
    explicit FileJournalFile(const QString& path);
    bool open() override;
    bool isOpen() const override;
    qint64 write(const QByteArray& bytes) override;
    bool flush() override;
    bool sync() override;
    QString path() const override;
    QString lastErrorDetail() const override;

private:
    QFile m_file;
};

struct WriterMetrics {
    quint64 appendCount = 0;       // successful line appends
    quint64 failureCount = 0;
    qint64 lastSerializeMicros = -1;
    qint64 lastAppendMicros = -1;  // write() call only
    qint64 lastFlushMicros = -1;   // -1 = not performed for that event
    qint64 lastSyncMicros = -1;
    qint64 totalAppendMicros = 0;
    qint64 totalFlushMicros = 0;
    qint64 totalSyncMicros = 0;
    qint64 maxAppendMicros = 0;
    qint64 maxSyncMicros = 0;
};

struct JournalIdentity {
    QString sessionId;
    QString lane;
    QString appVersion;
    QString deviceId;
};

struct AppendOutcome {
    bool ok = false;
    bool lineAppended = false;     // see semantics above
    ErrorInfo error;
    EventEnvelope envelope;        // completed authoritative envelope on ok
};

class JournalWriter {
public:
    // `file` is injected and NOT owned (caller controls lifetime — DI seam).
    JournalWriter(const JournalIdentity& identity, IJournalFile* file);

    // Auto-sequence append (M1 callers): the writer assigns the next dense
    // sequence itself. Registry durability class for the event type.
    AppendOutcome append(const DomainEvent& event,
                         const QString& wallIso, qint64 monoMs);
    // Auto-sequence append with an explicit durability override.
    AppendOutcome append(const DomainEvent& event,
                         const QString& wallIso, qint64 monoMs,
                         DurabilityClass durability);

    // Explicit-sequence append (M2 SessionStore): the CALLER owns logical
    // sequence assignment (needed for the degraded retry queue, where the
    // logical seq is fixed at submit time but the write order — and thus the
    // hash chain — reflects drain order). The writer still owns the write-
    // order hash chain (m_lastHash) and poisoning. The seq the store passes
    // need not be contiguous with the previous WRITTEN line (a gap means aux
    // events were dropped, §9D); the writer accepts any seq and records it.
    AppendOutcome appendSeq(quint64 seq, const DomainEvent& event,
                            const QString& wallIso, qint64 monoMs,
                            DurabilityClass durability);

    quint64 nextSequence() const { return m_autoSeq; }
    QByteArray lastHash() const { return m_lastHash; }
    bool failed() const { return m_poisoned; }
    const WriterMetrics& metrics() const { return m_metrics; }

private:
    AppendOutcome fail(ReliabilityError code, const QString& detail,
                       bool lineAppended, quint64 seq);

    JournalIdentity m_identity;
    IJournalFile* m_file = nullptr;
    quint64 m_autoSeq = 0;         // auto-sequence counter (M1 append path)
    bool m_headerWritten = false;  // seq-0 header seen (explicit-seq path)
    QByteArray m_lastHash;         // ph for the next WRITTEN line
    bool m_poisoned = false;
    WriterMetrics m_metrics;
};

} // namespace rel
} // namespace ta

#endif // TA_REL_JOURNALWRITER_H
