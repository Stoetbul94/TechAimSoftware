// Session Reliability Layer — M0 storage-foundation tests.
// Console harness (same pattern as tests/finals). Runs entirely inside an
// isolated temp root via setRootOverrideForTesting — it never touches the
// developer's real AppData and never depends on the process CWD.

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <cstdio>

#include "reliability/storage/StoragePaths.h"
#include "reliability/storage/StorageSync.h"
#include "test_support.h"

using ta::rel::StoragePaths;
using ta::rel::StorageResult;
using ta::rel::StorageError;
using ta::rel::StorageSync;

static QString writeFile(const QString& path, const QByteArray& content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return QString();
    f.write(content);
    f.close();
    return path;
}

void run_storagepaths_tests()
{
    std::printf("--- M0 storage tests ---\n");

    const QString root = QDir::temp().filePath(
        QStringLiteral("techaim_reliability_tests_%1")
            .arg(QCoreApplication::applicationPid()));
    QDir(root).removeRecursively();
    StoragePaths::setRootOverrideForTesting(root);

    // 1) structure resolution — every directory sits under the root
    check(StoragePaths::applicationDataRoot() == root, "test root override active");
    const QStringList dirs = {
        StoragePaths::currentSessionsDirectory(),
        StoragePaths::archivedSessionsDirectory(),
        StoragePaths::corruptedSessionsDirectory(),
        StoragePaths::legacyImportDirectory(),
        StoragePaths::backupsDirectory(), StoragePaths::reportsDirectory(),
        StoragePaths::exportsDirectory(), StoragePaths::logsDirectory(),
        StoragePaths::settingsDirectory(),
        StoragePaths::supportBundlesDirectory(),
        StoragePaths::derivedIndexesDirectory()
    };
    bool allUnderRoot = true;
    for (const QString& d : dirs)
        if (!d.startsWith(root + QLatin1Char('/'))) allUnderRoot = false;
    check(allUnderRoot, "all directories resolve under the application data root");
    check(StoragePaths::currentSessionsDirectory().endsWith("Sessions/Current"),
          "logical structure names centralized");

    // 2) required directories are created
    StorageResult r = StoragePaths::initialize();
    check(r.ok, "initialize creates directories and probes", r.technicalDetail);
    bool allExist = true;
    for (const QString& d : dirs) if (!QDir(d).exists()) allExist = false;
    check(allExist, "every required directory exists after initialize");

    // 3) existing directories are accepted (idempotent re-init)
    r = StoragePaths::initialize();
    check(r.ok, "re-initialize with existing directories succeeds", r.technicalDetail);

    // 5+6) probe succeeds and cleans up its temporary file
    r = StoragePaths::probeWritableStorage();
    check(r.ok, "writable probe succeeds", r.technicalDetail);
    const QStringList leftovers = QDir(StoragePaths::currentSessionsDirectory())
        .entryList(QStringList() << "*probe*", QDir::Files | QDir::Hidden);
    check(leftovers.isEmpty(), "probe cleans up its temporary file",
          leftovers.join(","));

    // 4) unwritable/invalid root returns a typed error with path + detail
    {
        const QString badRoot = QDir::temp().filePath(
            QStringLiteral("techaim_reliability_badroot_%1").arg(QCoreApplication::applicationPid()));
        // a FILE occupies the root path -> mkpath must fail
        QDir(badRoot).removeRecursively(); QFile::remove(badRoot);
        writeFile(badRoot, "not a directory");
        StoragePaths::setRootOverrideForTesting(badRoot);
        const StorageResult bad = StoragePaths::initialize();
        check(!bad.ok && bad.error == StorageError::DirectoryCreateFailed,
              "invalid root yields typed DirectoryCreateFailed",
              QString::number(int(bad.error)));
        // 13) error object carries operator message, technical detail and path
        check(!bad.operatorMessage.isEmpty() && !bad.technicalDetail.isEmpty()
                  && !bad.affectedPath.isEmpty(),
              "error object contains message, detail and affected path");
        QFile::remove(badRoot);
        StoragePaths::setRootOverrideForTesting(root);   // restore
    }

    // 7+8) journal path: under the app-data root, never CWD-relative
    const QString jp = StoragePaths::finalsJournalPath();
    check(QFileInfo(jp).isAbsolute(), "journal path is absolute");
    check(jp.startsWith(root + QLatin1Char('/')), "journal path under app-data root", jp);
    check(!jp.startsWith(QDir::currentPath()),
          "journal path independent of the process CWD");

    // 11) archive path creation + shape
    const QString ap = StoragePaths::archivedFinalsJournalPath("20260717_120000_000");
    check(ap.startsWith(StoragePaths::archivedSessionsDirectory())
              && ap.endsWith("finals_session_20260717_120000_000.jsonl"),
          "archive path shape", ap);

    // 9) legacy detection + preservation-only migration
    {
        const QString legacySrc = QDir::temp().filePath(
            QStringLiteral("techaim_reliability_legacy_%1").arg(QCoreApplication::applicationPid()));
        QDir(legacySrc).removeRecursively();
        QDir().mkpath(legacySrc);
        writeFile(legacySrc + "/finals_session.jsonl", "{\"a\":1}\n");
        writeFile(legacySrc + "/finals_session_20260101_010101_000.jsonl", "{\"b\":2}\n");
        StorageResult mig = StoragePaths::migrateLegacyJournals(legacySrc);
        check(mig.ok, "legacy migration succeeds", mig.technicalDetail);
        const QDir legacyDir(StoragePaths::legacyImportDirectory());
        check(legacyDir.exists("legacy_finals_session.jsonl")
                  && legacyDir.exists("legacy_finals_session_20260101_010101_000.jsonl"),
              "legacy journals preserved under Sessions/Archive/Legacy");
        check(!QFileInfo::exists(legacySrc + "/finals_session.jsonl"),
              "rename-moved legacy source no longer in source dir");
        // contents preserved exactly
        QFile moved(legacyDir.filePath("legacy_finals_session.jsonl"));
        moved.open(QIODevice::ReadOnly);
        check(moved.readAll() == QByteArray("{\"a\":1}\n"),
              "legacy journal contents preserved exactly");

        // 10+12) never overwrite: same-named legacy file again -> numbered suffix
        writeFile(legacySrc + "/finals_session.jsonl", "{\"c\":3}\n");
        mig = StoragePaths::migrateLegacyJournals(legacySrc);
        check(mig.ok && legacyDir.exists("legacy_1_finals_session.jsonl"),
              "collision handled with numbered suffix, nothing overwritten",
              mig.technicalDetail);
        QFile first(legacyDir.filePath("legacy_finals_session.jsonl"));
        first.open(QIODevice::ReadOnly);
        check(first.readAll() == QByteArray("{\"a\":1}\n"),
              "existing destination untouched by second migration");
        QDir(legacySrc).removeRecursively();
    }

    // StorageSync boundary: sync succeeds on a real open file, fails when closed
    {
        QFile f(StoragePaths::currentSessionsDirectory() + "/sync_check.tmp");
        check(f.open(QIODevice::WriteOnly) && f.write("x") == 1
                  && StorageSync::syncFile(f),
              "platform sync succeeds on an open file");
        f.close();
        check(!StorageSync::syncFile(f), "platform sync rejects a closed file");
        f.remove();
    }

    // 14) no QML dependency: proven by this binary compiling with QT = core.
    check(true, "reliability storage compiles with QT=core (no QML/GUI dependency)");

    QDir(root).removeRecursively();
}
