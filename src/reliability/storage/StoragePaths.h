#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Session Reliability Layer — M0: centralized storage paths.
//
// THE single owner of every reliability-related filesystem location. No other
// component may construct session-storage path strings. QtCore ONLY — no QML,
// no controllers, no reports, no audio, no hardware, no app logging (callers
// log; this class returns typed results).
//
// Root: QStandardPaths::AppLocalDataLocation (non-roaming; on Windows
// C:/Users/<u>/AppData/Local/<org>/<app>, on Android the app-private files
// dir). NEVER the executable directory, NEVER the process CWD.
//
// Layout (M0; names are the API — spec docs/session-reliability-
// implementation-spec.md §14, casing per the approved M0 brief):
//   <root>/Sessions/Current | Sessions/Archive | Sessions/Corrupt
//          Backups | Reports | Exports | Logs | Settings
//          SupportBundles | DerivedIndexes
// ─────────────────────────────────────────────────────────────────────────────

#include <QString>
#include <QStringList>

namespace ta {
namespace rel {

enum class StorageError {
    None,
    PathUnavailable,          // QStandardPaths gave nothing usable
    DirectoryCreateFailed,
    PermissionDenied,
    NotWritable,              // probe write/sync failed
    FileOpenFailed,
    MigrationFailed
};

// Typed result for every storage operation. Never a bare bool/QString.
struct StorageResult {
    bool ok = true;
    StorageError error = StorageError::None;
    QString operatorMessage;   // shown to the operator (dialog-ready)
    QString technicalDetail;   // logged (errno text, action report, ...)
    QString affectedPath;
    bool recoverable = true;   // false => retry without operator action is pointless

    static StorageResult success(const QString& detail = QString());
    static StorageResult failure(StorageError err,
                                 const QString& operatorMessage,
                                 const QString& technicalDetail,
                                 const QString& affectedPath,
                                 bool recoverable = true);
};

class StoragePaths {
public:
    // ── initialization (call once at startup, before any journal write) ──
    // Resolves the root, creates the full directory tree, probes writability
    // (write + platform sync + delete of a probe file). Does NOT migrate
    // legacy files — that is an explicit separate step (production main only).
    static StorageResult initialize();

    // ── directories ──────────────────────────────────────────────────────
    static QString applicationDataRoot();
    static QString currentSessionsDirectory();
    static QString archivedSessionsDirectory();
    static QString corruptedSessionsDirectory();
    static QString backupsDirectory();
    static QString reportsDirectory();
    static QString exportsDirectory();
    static QString logsDirectory();
    static QString settingsDirectory();
    static QString supportBundlesDirectory();
    static QString derivedIndexesDirectory();
    static QString legacyImportDirectory();      // Sessions/Archive/Legacy

    // ── session-file helpers ─────────────────────────────────────────────
    // M0 filename strategy (documented, conservative): the live finals
    // journal keeps its historical name inside Sessions/Current; uniqueness
    // across sessions is provided — exactly as before — by the archive-on-
    // start rename to a millisecond-timestamped name. The M1 session-identity
    // model (UUID filenames) replaces this.
    static QString finalsJournalPath();                          // Sessions/Current/finals_session.jsonl
    static QString archivedFinalsJournalPath(const QString& timestamp); // Sessions/Archive/finals_session_<ts>.jsonl
    static QString archivedSessionPath(const QString& fileName); // Sessions/Archive/<fileName>
    // Where the pre-M0 application wrote journals (process CWD).
    static QString legacyWorkingDirectoryJournalPath();

    // ── M2 session-identity filenames (spec §14) ─────────────────────────
    // The SessionStore writes one file per session, named
    // session_<utcCompact>_<uuid8>.jsonl inside Sessions/Current. The
    // timestamp is a sortable UTC prefix; the 8-char uuid suffix makes it
    // collision-free. lane id lives in the header, not the filename.
    static QString sessionJournalFileName(const QString& utcCompact,
                                          const QString& uuid8);
    static QString currentSessionJournalPath(const QString& fileName); // Sessions/Current/<fileName>
    // Month-partitioned archive path (Sessions/Archive/<yyyy>/<MM>/<fileName>);
    // the month directory is created if absent (returns empty on failure).
    static QString archivedSessionMonthPath(const QString& utcYear,
                                            const QString& utcMonth,
                                            const QString& fileName);

    // ── operations ───────────────────────────────────────────────────────
    static StorageResult ensureRequiredDirectories();
    static StorageResult probeWritableStorage();
    // Preservation-only migration (M0 policy): every finals_session*.jsonl in
    // `sourceDir` is MOVED (rename; copy on cross-volume failure, original
    // kept) into Sessions/Archive/Legacy/ with a `legacy_` prefix. Existing
    // destinations are never overwritten (collision => numbered suffix).
    // The action report is returned in technicalDetail for the caller to log.
    static StorageResult migrateLegacyJournals(const QString& sourceDir);

    // ── testing only ─────────────────────────────────────────────────────
    // Redirects the root for tests. Deliberately verbose name; production
    // code must never call it (grep-gated in review).
    static void setRootOverrideForTesting(const QString& rootDir);

private:
    static QString dir(const char* leaf);
    static QString s_rootOverride;
};

} // namespace rel
} // namespace ta
