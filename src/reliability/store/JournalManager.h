#ifndef TA_REL_JOURNALMANAGER_H
#define TA_REL_JOURNALMANAGER_H

// Session Reliability Layer — journal file lifecycle (M2, spec §2).
//
// Owns the session file + its JournalWriter, and performs archive-on-close.
// Deliberately thin: rotation/segmenting (Training) is deferred. Kept apart
// from JournalWriter because "which file, and where it goes when the session
// closes" is a different test surface from "emit these exact bytes".
//
// Two construction modes:
//   - path mode (production): owns a FileJournalFile at the given path;
//     closeAndArchive() moves the closed file into Sessions/Archive/yyyy/MM.
//   - injected mode (tests): borrows an IJournalFile* (e.g. an in-memory
//     fault double); archiving is a no-op the test controls.

#include "reliability/journal/JournalWriter.h"
#include "reliability/storage/StoragePaths.h"

#include <memory>

namespace ta {
namespace rel {

class JournalManager {
public:
    // Production: own a real file at `path` under Sessions/Current.
    JournalManager(const JournalIdentity& identity, const QString& path);
    // Tests: borrow an injected file (NOT owned).
    JournalManager(const JournalIdentity& identity, IJournalFile* injected);

    JournalWriter& writer() { return m_writer; }
    const WriterMetrics& metrics() const { return m_writer.metrics(); }
    QString currentPath() const { return m_path; }
    bool poisoned() const { return m_writer.failed(); }

    // Append one event at an explicit (store-owned) logical sequence.
    AppendOutcome append(quint64 seq, const DomainEvent& event,
                         const QString& wallIso, qint64 monoMs,
                         DurabilityClass durability);

    // Move the closed session file into Sessions/Archive/<yyyy>/<MM>/. No-op
    // (success) in injected mode. Idempotent: a second call after a move is a
    // success. utcYear/utcMonth partition the archive (spec §14).
    StorageResult closeAndArchive(const QString& utcYear, const QString& utcMonth);

private:
    JournalIdentity m_identity;
    QString m_path;                       // empty in injected mode
    std::unique_ptr<FileJournalFile> m_owned;   // path mode only
    IJournalFile* m_file = nullptr;       // owned or injected
    JournalWriter m_writer;
    bool m_archived = false;
};

} // namespace rel
} // namespace ta

#endif // TA_REL_JOURNALMANAGER_H
