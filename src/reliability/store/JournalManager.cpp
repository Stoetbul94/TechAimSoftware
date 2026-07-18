#include "JournalManager.h"

#include "reliability/storage/StoragePaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace ta {
namespace rel {

JournalManager::JournalManager(const JournalIdentity& identity, const QString& path)
    : m_identity(identity)
    , m_path(path)
    , m_owned(std::make_unique<FileJournalFile>(path))
    , m_file(m_owned.get())
    , m_writer(identity, m_owned.get())
{
}

JournalManager::JournalManager(const JournalIdentity& identity, IJournalFile* injected)
    : m_identity(identity)
    , m_file(injected)
    , m_writer(identity, injected)
{
}

AppendOutcome JournalManager::append(quint64 seq, const DomainEvent& event,
                                     const QString& wallIso, qint64 monoMs,
                                     DurabilityClass durability)
{
    return m_writer.appendSeq(seq, event, wallIso, monoMs, durability);
}

StorageResult JournalManager::closeAndArchive(const QString& utcYear,
                                              const QString& utcMonth)
{
    if (m_archived)
        return StorageResult::success(QStringLiteral("already archived"));
    // Injected (test) mode: nothing to move on disk.
    if (m_path.isEmpty()) {
        m_archived = true;
        return StorageResult::success(QStringLiteral("injected mode: no file to archive"));
    }
    // Close the owned file so the move is not blocked by an open handle.
    if (m_owned)
        m_owned.reset();   // closes and releases the QFile
    m_file = nullptr;

    if (!QFile::exists(m_path)) {
        m_archived = true;
        return StorageResult::success(QStringLiteral("no session file to archive"));
    }

    const QFileInfo info(m_path);
    const QString dest = StoragePaths::archivedSessionMonthPath(
        utcYear, utcMonth, info.fileName());
    if (dest.isEmpty())
        return StorageResult::failure(StorageError::DirectoryCreateFailed,
            QStringLiteral("The session could not be archived."),
            QStringLiteral("could not create archive month directory"),
            m_path);
    if (QFileInfo::exists(dest))
        return StorageResult::failure(StorageError::MigrationFailed,
            QStringLiteral("The session could not be archived."),
            QStringLiteral("archive destination already exists"), dest);

    if (QFile::rename(m_path, dest)) {
        m_archived = true;
        return StorageResult::success(
            QStringLiteral("archived %1 -> %2").arg(m_path, dest));
    }
    // Cross-volume / locked: copy + verify + remove source.
    if (QFile::copy(m_path, dest)
            && QFileInfo(dest).size() == info.size()) {
        QFile::remove(m_path);
        m_archived = true;
        return StorageResult::success(
            QStringLiteral("archived (copied) %1 -> %2").arg(m_path, dest));
    }
    QFile::remove(dest);   // never leave a half copy
    return StorageResult::failure(StorageError::MigrationFailed,
        QStringLiteral("The session could not be archived."),
        QStringLiteral("rename and copy both failed"), m_path);
}

} // namespace rel
} // namespace ta
