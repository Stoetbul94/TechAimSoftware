#include "StoragePaths.h"
#include "StorageSync.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace ta {
namespace rel {

QString StoragePaths::s_rootOverride;

StorageResult StorageResult::success(const QString& detail)
{
    StorageResult r;
    r.technicalDetail = detail;
    return r;
}

StorageResult StorageResult::failure(StorageError err,
                                     const QString& operatorMessage,
                                     const QString& technicalDetail,
                                     const QString& affectedPath,
                                     bool recoverable)
{
    StorageResult r;
    r.ok = false;
    r.error = err;
    r.operatorMessage = operatorMessage;
    r.technicalDetail = technicalDetail;
    r.affectedPath = affectedPath;
    r.recoverable = recoverable;
    return r;
}

void StoragePaths::setRootOverrideForTesting(const QString& rootDir)
{
    s_rootOverride = rootDir;
}

QString StoragePaths::applicationDataRoot()
{
    if (!s_rootOverride.isEmpty())
        return s_rootOverride;
    // Non-roaming application data (spec §14). Requires organizationName /
    // applicationName to be set (done in main.cpp at startup).
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

QString StoragePaths::dir(const char* leaf)
{
    return applicationDataRoot() + QLatin1Char('/') + QLatin1String(leaf);
}

QString StoragePaths::currentSessionsDirectory()   { return dir("Sessions/Current"); }
QString StoragePaths::archivedSessionsDirectory()  { return dir("Sessions/Archive"); }
QString StoragePaths::corruptedSessionsDirectory() { return dir("Sessions/Corrupt"); }
QString StoragePaths::backupsDirectory()           { return dir("Backups"); }
QString StoragePaths::reportsDirectory()           { return dir("Reports"); }
QString StoragePaths::exportsDirectory()           { return dir("Exports"); }
QString StoragePaths::logsDirectory()              { return dir("Logs"); }
QString StoragePaths::settingsDirectory()          { return dir("Settings"); }
QString StoragePaths::supportBundlesDirectory()    { return dir("SupportBundles"); }
QString StoragePaths::derivedIndexesDirectory()    { return dir("DerivedIndexes"); }
QString StoragePaths::legacyImportDirectory()      { return dir("Sessions/Archive/Legacy"); }

QString StoragePaths::finalsJournalPath()
{
    return currentSessionsDirectory() + QStringLiteral("/finals_session.jsonl");
}

QString StoragePaths::archivedFinalsJournalPath(const QString& timestamp)
{
    return archivedSessionsDirectory()
            + QStringLiteral("/finals_session_%1.jsonl").arg(timestamp);
}

QString StoragePaths::archivedSessionPath(const QString& fileName)
{
    return archivedSessionsDirectory() + QLatin1Char('/') + fileName;
}

QString StoragePaths::legacyWorkingDirectoryJournalPath()
{
    return QDir::currentPath() + QStringLiteral("/finals_session.jsonl");
}

QString StoragePaths::sessionJournalFileName(const QString& utcCompact,
                                             const QString& uuid8)
{
    return QStringLiteral("session_%1_%2.jsonl").arg(utcCompact, uuid8);
}

QString StoragePaths::currentSessionJournalPath(const QString& fileName)
{
    return currentSessionsDirectory() + QLatin1Char('/') + fileName;
}

QString StoragePaths::archivedSessionMonthPath(const QString& utcYear,
                                               const QString& utcMonth,
                                               const QString& fileName)
{
    const QString monthDir = archivedSessionsDirectory()
            + QLatin1Char('/') + utcYear + QLatin1Char('/') + utcMonth;
    if (!QDir().mkpath(monthDir))
        return QString();
    return monthDir + QLatin1Char('/') + fileName;
}

StorageResult StoragePaths::ensureRequiredDirectories()
{
    const QString root = applicationDataRoot();
    if (root.isEmpty()) {
        return StorageResult::failure(
            StorageError::PathUnavailable,
            QStringLiteral("No application data location is available on this system."),
            QStringLiteral("QStandardPaths::AppLocalDataLocation returned an empty path "
                           "(organizationName/applicationName not set, or platform failure)."),
            QString(), /*recoverable*/ false);
    }

    const QStringList required = {
        currentSessionsDirectory(), archivedSessionsDirectory(),
        corruptedSessionsDirectory(), legacyImportDirectory(),
        backupsDirectory(), reportsDirectory(), exportsDirectory(),
        logsDirectory(), settingsDirectory(), supportBundlesDirectory(),
        derivedIndexesDirectory()
    };
    for (const QString& d : required) {
        if (!QDir().mkpath(d)) {
            return StorageResult::failure(
                StorageError::DirectoryCreateFailed,
                QStringLiteral("The session storage folder could not be created."),
                QStringLiteral("QDir::mkpath failed"),
                d);
        }
    }
    return StorageResult::success(QStringLiteral("directories ready under ") + root);
}

StorageResult StoragePaths::probeWritableStorage()
{
    // Write + platform-sync + delete a probe file where the journals will
    // live. Any failure means official persistence would silently fail —
    // exactly what M0 exists to prevent.
    const QString probePath = currentSessionsDirectory()
            + QStringLiteral("/.techaim_write_probe");
    QFile probe(probePath);
    if (!probe.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const bool perm = probe.error() == QFileDevice::PermissionsError
                          || probe.error() == QFileDevice::OpenError;
        return StorageResult::failure(
            perm ? StorageError::PermissionDenied : StorageError::FileOpenFailed,
            QStringLiteral("Session storage is not writable."),
            QStringLiteral("probe open failed: %1").arg(probe.errorString()),
            probePath);
    }
    const QByteArray payload = QByteArrayLiteral("techaim-write-probe\n");
    if (probe.write(payload) != payload.size() || !probe.flush()) {
        const QString detail = probe.errorString();
        probe.close();
        probe.remove();
        return StorageResult::failure(
            StorageError::NotWritable,
            QStringLiteral("Session storage rejected a test write."),
            QStringLiteral("probe write/flush failed: %1").arg(detail),
            probePath);
    }
    if (!StorageSync::syncFile(probe)) {
        // Sync failure is durability-relevant but the write itself worked;
        // report it (M2's health model will grade this — M0 surfaces it).
        probe.close();
        probe.remove();
        return StorageResult::failure(
            StorageError::NotWritable,
            QStringLiteral("Session storage does not support durable writes."),
            QStringLiteral("platform sync (fsync/FlushFileBuffers) failed on probe"),
            probePath);
    }
    probe.close();
    if (!probe.remove()) {
        // Not fatal for writability, but never leave probe litter unreported.
        return StorageResult::failure(
            StorageError::NotWritable,
            QStringLiteral("Session storage probe could not be cleaned up."),
            QStringLiteral("probe remove failed: %1").arg(probe.errorString()),
            probePath);
    }
    return StorageResult::success(QStringLiteral("probe ok in ")
                                  + currentSessionsDirectory());
}

StorageResult StoragePaths::initialize()
{
    StorageResult r = ensureRequiredDirectories();
    if (!r.ok)
        return r;
    return probeWritableStorage();
}

StorageResult StoragePaths::migrateLegacyJournals(const QString& sourceDir)
{
    // Preservation only (M0): journals written by pre-M0 builds into the
    // process CWD are moved into Sessions/Archive/Legacy so they can never
    // be lost to a working-directory change. Contents are preserved exactly;
    // nothing is deleted unless the atomic rename succeeded; existing
    // destinations are never overwritten.
    QDir src(sourceDir);
    if (!src.exists())
        return StorageResult::success(QStringLiteral("legacy scan: source dir absent"));

    const QStringList candidates = src.entryList(
        QStringList() << QStringLiteral("finals_session*.jsonl"),
        QDir::Files, QDir::Name);
    if (candidates.isEmpty())
        return StorageResult::success(QStringLiteral("legacy scan: none found in ")
                                      + sourceDir);

    QStringList report;
    QStringList failures;
    for (const QString& name : candidates) {
        const QString from = src.absoluteFilePath(name);
        QString to = legacyImportDirectory() + QStringLiteral("/legacy_") + name;
        // never overwrite: numbered suffix on collision
        for (int n = 1; QFileInfo::exists(to); ++n) {
            to = legacyImportDirectory() + QStringLiteral("/legacy_%1_%2")
                     .arg(n).arg(name);
        }
        if (QFile::rename(from, to)) {
            report << QStringLiteral("moved %1 -> %2").arg(from, to);
            continue;
        }
        // Cross-volume or locked: copy, verify size, keep the original.
        if (QFile::copy(from, to)
                && QFileInfo(to).size() == QFileInfo(from).size()) {
            report << QStringLiteral("copied (original kept) %1 -> %2").arg(from, to);
            continue;
        }
        QFile::remove(to);   // never leave a half copy
        failures << from;
        report << QStringLiteral("FAILED to migrate %1 (left untouched)").arg(from);
    }

    if (!failures.isEmpty()) {
        return StorageResult::failure(
            StorageError::MigrationFailed,
            QStringLiteral("Some previous session journals could not be moved to "
                           "the new storage location. They remain untouched at "
                           "their old location."),
            report.join(QStringLiteral("; ")),
            failures.join(QStringLiteral("; ")));
    }
    return StorageResult::success(report.join(QStringLiteral("; ")));
}

} // namespace rel
} // namespace ta
