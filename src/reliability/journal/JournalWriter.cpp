#include "JournalWriter.h"

#include "HashChain.h"
#include "reliability/events/EventRegistry.h"
#include "reliability/events/EventSerializer.h"
#include "reliability/storage/StorageSync.h"

#include <QElapsedTimer>

namespace ta {
namespace rel {

// ── FileJournalFile ───────────────────────────────────────────────────

FileJournalFile::FileJournalFile(const QString& path)
    : m_file(path)
{
}

bool FileJournalFile::open()
{
    // No QIODevice::Text — the writer emits LF explicitly; Text mode would
    // rewrite it to CRLF on Windows and break byte stability.
    return m_file.open(QIODevice::WriteOnly | QIODevice::Append);
}

bool FileJournalFile::isOpen() const
{
    return m_file.isOpen();
}

qint64 FileJournalFile::write(const QByteArray& bytes)
{
    return m_file.write(bytes);
}

bool FileJournalFile::flush()
{
    return m_file.flush();
}

bool FileJournalFile::sync()
{
    // QFile::flush first: fsync only pushes what the OS already has.
    if (!m_file.flush())
        return false;
    return StorageSync::syncFile(m_file);
}

QString FileJournalFile::path() const
{
    return m_file.fileName();
}

QString FileJournalFile::lastErrorDetail() const
{
    return m_file.errorString();
}

// ── JournalWriter ─────────────────────────────────────────────────────

JournalWriter::JournalWriter(const JournalIdentity& identity, IJournalFile* file)
    : m_identity(identity)
    , m_file(file)
    , m_lastHash(HashChain::genesisPreviousHash())
{
}

AppendOutcome JournalWriter::fail(ReliabilityError code, const QString& detail,
                                  bool lineAppended, quint64 seq)
{
    ++m_metrics.failureCount;
    AppendOutcome out;
    out.ok = false;
    out.lineAppended = lineAppended;
    out.error = ReliabilityResult::failure(code,
        QStringLiteral("The session journal could not be written."),
        detail, m_file ? m_file->path() : QString(),
        static_cast<qint64>(seq)).error;
    return out;
}

AppendOutcome JournalWriter::append(const DomainEvent& event,
                                    const QString& wallIso, qint64 monoMs)
{
    const EventMeta* meta = EventRegistry::metaForEvent(event);
    return append(event, wallIso, monoMs,
                  meta ? meta->durability : DurabilityClass::Sync);
}

AppendOutcome JournalWriter::append(const DomainEvent& event,
                                    const QString& wallIso, qint64 monoMs,
                                    DurabilityClass durability)
{
    const AppendOutcome out = appendSeq(m_autoSeq, event, wallIso, monoMs,
                                        durability);
    // The seq is consumed the moment a line is physically appended — even if
    // the subsequent flush/fsync failed the bytes (and their seq) are on disk.
    // A clean failure (nothing written) leaves the seq reusable on retry.
    if (out.lineAppended)
        ++m_autoSeq;
    return out;
}

AppendOutcome JournalWriter::appendSeq(quint64 seq, const DomainEvent& event,
                                       const QString& wallIso, qint64 monoMs,
                                       DurabilityClass durability)
{
    if (m_poisoned)
        return fail(ReliabilityError::CorruptJournal,
                    QStringLiteral("writer poisoned by an earlier partial write; "
                                   "the on-disk tail is torn"),
                    false, seq);
    if (!m_file)
        return fail(ReliabilityError::InvalidArgument,
                    QStringLiteral("no journal file injected"), false, seq);
    if (!m_file->isOpen() && !m_file->open())
        return fail(ReliabilityError::FileOpenFailed,
                    m_file->lastErrorDetail(), false, seq);

    // The header rule: seq 0 must be SessionStarted with a matching sid.
    if (seq == 0) {
        const auto* started = std::get_if<SessionStarted>(&event);
        if (!started)
            return fail(ReliabilityError::InvalidEvent,
                        QStringLiteral("first event must be SessionStarted, got %1")
                            .arg(QLatin1String(eventTypeId(event))),
                        false, seq);
        if (started->sessionId != m_identity.sessionId)
            return fail(ReliabilityError::SessionMismatch,
                        QStringLiteral("header sessionId '%1' != writer identity '%2'")
                            .arg(started->sessionId, m_identity.sessionId),
                        false, seq);
    }

    EventEnvelope env;
    env.schemaVersion = kJournalSchemaVersion;
    env.payloadVersion = eventPayloadVersion(event);
    env.sessionId = m_identity.sessionId;
    env.lane = m_identity.lane;
    env.seq = seq;
    env.wallTimestampIso = wallIso;
    env.monotonicMs = monoMs;
    env.eventType = QString::fromLatin1(eventTypeId(event));
    env.payload = event;
    if (seq == 0) {
        env.appVersion = m_identity.appVersion;
        env.deviceId = m_identity.deviceId;
        m_headerWritten = true;
    }
    env.previousHash = m_lastHash;

    QElapsedTimer timer;
    timer.start();
    QByteArray core;
    ReliabilityResult sr = EventSerializer::serializeCoreWithoutCurrentHash(env, &core);
    if (!sr.ok) {
        ++m_metrics.failureCount;
        AppendOutcome out;
        out.ok = false;
        out.lineAppended = false;
        out.error = sr.error;
        out.error.sequence = static_cast<qint64>(seq);
        return out;
    }
    env.currentHash = HashChain::computeLineHash(env.previousHash, core);
    QByteArray line;
    sr = EventSerializer::serializeCompleteEnvelope(env, &line);
    if (!sr.ok) {
        ++m_metrics.failureCount;
        AppendOutcome out;
        out.ok = false;
        out.lineAppended = false;
        out.error = sr.error;
        return out;
    }
    line += '\n';
    m_metrics.lastSerializeMicros = timer.nsecsElapsed() / 1000;

    // Physical append.
    timer.restart();
    const qint64 written = m_file->write(line);
    m_metrics.lastAppendMicros = timer.nsecsElapsed() / 1000;
    m_metrics.totalAppendMicros += m_metrics.lastAppendMicros;
    if (m_metrics.lastAppendMicros > m_metrics.maxAppendMicros)
        m_metrics.maxAppendMicros = m_metrics.lastAppendMicros;

    if (written != line.size()) {
        if (written > 0) {
            // Torn tail on disk — refuse to continue on this file.
            m_poisoned = true;
            return fail(ReliabilityError::FileWriteFailed,
                        QStringLiteral("partial write: %1 of %2 bytes (%3)")
                            .arg(written).arg(line.size())
                            .arg(m_file->lastErrorDetail()),
                        false, seq);
        }
        return fail(ReliabilityError::FileWriteFailed,
                    QStringLiteral("write failed: %1").arg(m_file->lastErrorDetail()),
                    false, seq);
    }

    // The line is fully appended: the write-order hash chain advances even if
    // durability below fails (the bytes exist; the health model reacts).
    m_lastHash = env.currentHash;
    ++m_metrics.appendCount;
    m_metrics.lastFlushMicros = -1;
    m_metrics.lastSyncMicros = -1;

    if (durability == DurabilityClass::Flush || durability == DurabilityClass::Sync) {
        timer.restart();
        const bool flushed = m_file->flush();
        m_metrics.lastFlushMicros = timer.nsecsElapsed() / 1000;
        m_metrics.totalFlushMicros += m_metrics.lastFlushMicros;
        if (!flushed) {
            AppendOutcome out = fail(ReliabilityError::FlushFailed,
                                     m_file->lastErrorDetail(), true, env.seq);
            out.envelope = env;
            return out;
        }
    }
    if (durability == DurabilityClass::Sync) {
        timer.restart();
        const bool synced = m_file->sync();
        m_metrics.lastSyncMicros = timer.nsecsElapsed() / 1000;
        m_metrics.totalSyncMicros += m_metrics.lastSyncMicros;
        if (m_metrics.lastSyncMicros > m_metrics.maxSyncMicros)
            m_metrics.maxSyncMicros = m_metrics.lastSyncMicros;
        if (!synced) {
            AppendOutcome out = fail(ReliabilityError::SyncFailed,
                                     m_file->lastErrorDetail(), true, env.seq);
            out.envelope = env;
            return out;
        }
    }

    AppendOutcome out;
    out.ok = true;
    out.lineAppended = true;
    out.envelope = env;
    return out;
}

} // namespace rel
} // namespace ta
