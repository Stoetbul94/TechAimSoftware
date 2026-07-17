# Session Reliability Layer — M0 implementation note (storage foundation)

Blueprint: docs/session-reliability-implementation-spec.md (§14, §28 M0).
Scope: paths, probe, error surfacing, finals-journal path migration, legacy
preservation. Nothing else.

## Folder structure (created centrally by `ta::rel::StoragePaths`)

```
<AppDataRoot>/
  Sessions/Current | Sessions/Archive | Sessions/Archive/Legacy | Sessions/Corrupt
  Backups | Reports | Exports | Logs | Settings | SupportBundles | DerivedIndexes
```
Root = `QStandardPaths::AppLocalDataLocation` (non-roaming) with
`organizationName/applicationName = TechAim/TechAim` set in `main.cpp`.
Resolved examples — Windows:
`C:/Users/<u>/AppData/Local/TechAim/TechAim/Sessions/Current/finals_session.jsonl`;
Android (future build, same code): `<app-private files>/Sessions/Current/…`.
Side effect: Qt's own `cache/` also appears under this root once org/app
names are set (QML disk cache) — expected, unrelated to session storage.

## Startup flow (main.cpp)

1. Set org/app names (all existing QSettings uses pass explicit strings —
   unaffected).
2. `StoragePaths::initialize()` = create tree + write/sync/delete probe in
   `Sessions/Current`.
3. On failure: typed `StorageResult` logged + frameless TechAim
   **Retry/Exit** dialog (pre-QML); loop until success or operator exit —
   never silent.
4. `migrateLegacyJournals(CWD)` — preservation only; report logged.

## M0 journal filename strategy (documented, conservative)

The live finals journal keeps its historical name
(`Sessions/Current/finals_session.jsonl`); cross-session uniqueness is what
it always was — the archive-on-start rename to
`Sessions/Archive/finals_session_<ms-timestamp>.jsonl`. The M1 session
identity model (UUID filenames) replaces this. Event schema, content and
ordering are byte-identical to pre-M0.

## Legacy migration policy

`finals_session*.jsonl` found in the process CWD are MOVED (atomic rename;
copy-with-size-verify fallback keeps the original) into
`Sessions/Archive/Legacy/legacy_<name>`; existing destinations are never
overwritten (numbered suffix); failures are surfaced and the source is left
untouched. Purpose is preservation — pre-M0 journals cannot resume sessions
(no recovery system exists until M3).

## Error handling

`ta::rel::StorageResult { ok, StorageError, operatorMessage,
technicalDetail, affectedPath, recoverable }`. Finals journal open/write/
archive failures now log every occurrence (`qWarning` + path) and emit
`Finals3PController::journalWriteFailed(path, detail)` once per session →
TechAim error dialog via main.qml. The full persistence-health state
machine, retry queue and HUD states are deliberately M2.

## Platform sync boundary

`ta::rel::StorageSync::syncFile(QFile&)`: Windows `FlushFileBuffers`,
POSIX/Android `fsync` (same call site), other platforms fall back to
`QFile::flush` only (documented: no hard-power durability there). M0 uses
it in the startup probe only; per-event durability policy is M1/M2 (spec
§11/§12).

## Tests

`tests/reliability/reliability_tests.pro` (**QT = core** — compiling proves
the no-QML/GUI dependency requirement): 23 checks, 0 failures — structure
resolution, directory creation/idempotence, probe success + cleanup, typed
error for an invalid root (message/detail/path populated), journal path
absolute/under-root/CWD-independent, archive path shape, legacy migration
(preservation, exact contents, collision suffixes, never-overwrite),
sync-adapter behaviour. Tests run in an isolated temp root via
`setRootOverrideForTesting` (a deliberately verbose, test-only API).
Finals harness: 178 checks, 0 failures (now running against an isolated
override root; two journal-read sites use `StoragePaths::finalsJournalPath`).

## Verified at runtime (2026-07-17)

Startup log: `M0 storage ready: root=C:/Users/User/AppData/Local/TechAim/
TechAim (probe ok …)`; legacy scan moved 6 real pre-M0 journals from
`release/` into `Sessions/Archive/Legacy/` with contents intact. App
launches clean (0 runtime errors).

## Known limitations / deliberately deferred

- No session UUIDs, envelopes, hash chain, snapshots, replay, recovery UI
  (M1/M3); no persistence-health model or retry queue (M2); qualification
  persistence untouched (M4).
- Journal write failure mid-session: logged + one dialog; writes are not
  retried (M2).
- `.tch` save/load and kiosk lane files are outside M0 scope (M4 / RMS).
- StorageSync is exercised by the probe only; per-event fsync policy lands
  with the M1 writer.
