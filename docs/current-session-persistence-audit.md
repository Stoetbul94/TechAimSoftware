# Current Session-Persistence & Recovery Audit

Documentation-only audit (no code changed). Every claim cites file/function/
line as of branch `fix/series-review-navigation` (`1033330`). Where runtime
behaviour cannot be proven statically it is marked **UNVERIFIED** with a
proposed manual test.

---

## 1. Executive summary

The application is **offline-first by construction but has no recovery
system**. Exactly one artefact on disk is complete enough to rebuild a
session — the finals journal `finals_session.jsonl` — and **nothing ever
reads it back**. Qualification's only persistence is a per-shot rewritten
XML save file (`Match_*.tch`) that stores raw coordinates without scores or
3P positions, sourced from backend lists that are known to be mode-swap
corrupted for 3P. All controller state, all QML models, all timers and the
entire report pipeline live in RAM. **Every persisted file is written
relative to the current working directory** — a read-only install location
(Program Files) silently disables all of it, including the finals journal
(`writeJournal` skips without any warning when `open()` fails,
Finals3PController.cpp:79-84 region).

Bottom line: after a crash or power loss, finals data *survives on disk but
cannot be loaded*; qualification data partially survives in a lossy format;
no mode can resume a timer or a phase. The finals journal is the right
foundation to build on.

## 2. Current architecture diagram

```
                       ┌────────────────────────────── RAM ONLY ──────────────────────────────┐
 hardware / demo click │                                                                       │
        │              │  TachusWidget backend lists (mode-swapped)   [volatile, NOT a record] │
        ▼              │  Finals3PController state machine            [volatile, authoritative │
 MODREADER registers ──┤                                               for finals until crash] │
        │              │  QML: globalMatchModel / globalSlighterModel [volatile, authoritative │
        ▼              │       globalModelOfData / right-panel models  for qualification]      │
 CenterPane scoring    │  Report builders (Finals/Coach/Summary/Match)[derived, rebuildable    │
 (QML, ring geometry)  │                                               from models only]       │
        │              └───────────────┬───────────────────────────────────────────────────────┘
        ▼                              │
   mode routing                        ▼ DISK (all relative to CWD unless noted)
   ├─ qualification ─► models ─► Match_<user>.tch      (XML, truncate-rewrite EVERY shot; x/y/time only)
   ├─ finals ────────► FINALS3P ─► finals_session.jsonl (append-only, per-event, full shot payloads)
   └─ kiosk (opt.) ──► <serverPath>/<lane>.txt, shootdata file, <lane>_status.csv (append per shot)
                                       │
                                       ▼ outputs only (not machine-readable back)
                            PDFs (user-picked path / network path), %TEMP% log
```

## 3. Shot data-flow (one accepted shot)

| # | Step | Evidence | Persistence class |
|---|------|----------|-------------------|
| 1 | Raw event: Modbus registers polled / demo click | `TachusWidget` read cycle; demo append `tachuswidget.cpp:404-408` (`m_xCordList.append(modifiedX)`) | volatile (backend lists; **mode-swapped** by `changeSighterMode`, tachuswidget.cpp:1195-1240) |
| 2 | Count change → QML | `shootCountChanged(count)` (tachuswidget.h:394) → `CenterPane.qml` `function onShootCountChanged` (~line 248) | volatile |
| 3 | Coordinate mapping + scoring | `CenterPane.qml::calculateShootingSocre()` (~line 2000 region) | derived |
| 4 | Mode routing | `ShootingPage.onPointAddedToSeries` (~547): finals → `FINALS3P.registerShot(x,y,score,seq,dir)`; else qualification path | — |
| 5 | Controller validation (finals only) | `Finals3PController::registerShot` (Finals3PController.cpp:406-455): window/duplicate/limit chain | volatile |
| 6 | Accepted-shot signal | finals: `shotAccepted(record)`; qualification: direct model appends via `rightPanel.addToSeries` (RightPanel.qml:1063+) | — |
| 7 | QML router | ShootingPage finals `Connections` (~697): appends record to models | — |
| 8 | `globalMatchModel` | ShootingPage.qml:158-164 — **authoritative full-match record** (xmm/ymm/calculatedscore/timeComsumed/position) | **volatile, authoritative** |
| 9 | `globalSlighterModel` | ShootingPage.qml:130-136 | volatile |
| 10 | Right-panel table/totals | RightPanel `listModel`, `grandTotal` etc. | volatile, display-only |
| 11 | Target display buffer | `globalModelOfData` (per-window/position) | volatile, display-only |
| 12 | Report input | Summary/Match: read models directly; Coach: `feedCoachReport()` (ShootingPage.qml:643) from globalMatchModel; Finals: `FINALS3P.buildReport()` from controller-retained records (Finals3PController.cpp:40-50) | derived |
| 13 | Local file write | Qualification: `APPSETTINGS.saveMatch()` **on every shot** (CenterPane.qml:333, inside `onShootCountChanged`) → `Match_*.tch`; Finals: `writeJournal("shotAccepted", record)` per shot (Finals3PController.cpp:553). Kiosk (if server networking enabled): `updateSetaShootData` appends `shootdata,count,x,y,score,sighter` (tachuswidget.cpp:1431-1456) | persisted |
| 14 | RAM-only remainder | scores+positions of qualification shots (the .tch has neither), all controller/timer state, right-panel totals, coach report, finals retained records | volatile |

## 4. Storage inventory

No `QSaveFile` exists anywhere (repo-wide grep: zero hits) — **no write in
the application is atomic**. No SQLite/QSqlDatabase in app code.

| File | Written by | When | Mode | Path | Atomic | Contains official shots | Rebuild-capable |
|---|---|---|---|---|---|---|---|
| `finals_session.jsonl` | `Finals3PController::writeJournal` (cpp:79-96) | every command/shot/reject/missing/stage/status event | append, open+close per line | **CWD** (`kJournalPath` relative, cpp:16) | no fsync; line-atomic in practice, partial tail possible on power loss | YES (full record incl. x/y, score, number, windowId, seriesIndex, timestamp, running totals) | **YES (data), NO (no reader exists)** |
| `finals_session_<ts>.jsonl` | `archiveExistingJournal` (cpp:68-76) | rename at `startFinal()` | rename | CWD | n/a | yes (historic) | same |
| `Match_<user>.tch` / `Match_<datetime>.tch` | `AppSettings::saveMatch` (appsettings.cpp:192-236) | **every accepted shot** (CenterPane.qml:333) + exit-Save (main.qml Save action) | XML, **Truncate rewrite** | CWD | **no** (truncate+rewrite window on every shot) | partially: x/y/time/timestamp only — **no score, no 3P position, no per-shot sighter flag**; sourced from mode-swapped backend lists | single-position matches: lossy-partial via `uploadGame`; 3P: **no** |
| `config.ini` | read `AppSettings(fileName)` (cpp:20-58; ctor arg `"config.ini"` main.cpp:138) | startup read; no runtime writes found (setValue lines commented, cpp:25-26) | INI read | CWD | n/a | no | config only |
| Registry `HKCU\Software\Seta\shootingApp` | `QSettings("Seta","shootingApp")` (appsettings.cpp:294-330) | EULA flag, last load-file folder | registry | Windows registry | Qt-managed | no | no |
| `licence.txt` | read-only checks (tachuswidget.cpp:178, 352, 1563) | startup licence path (expiry gate currently disabled) | read | CWD | n/a | no | no |
| `%TEMP%/tachus_log<date>.log` | `LogFile` (logfile.h:26) | diagnostic appends throughout | append | `QStandardPaths::TempLocation` — the **only** QStandardPaths use in app code | no | fragments (counts/coords in log text) | no |
| `<serverPath>/<lane>.txt`, seta shootdata file, `<lane>_status.csv` | tachuswidget.cpp:1421, 1448; status path appsettings.cpp:1043 | per shot / control polling, **only when server networking enabled** | append / poll | network share from config (`getSetaServerPath`) | no | x/y/score/sighter per line (kiosk feed) | not designed for it |
| PDFs (summary/match/finals/coach) | CUSTOMPRINT / PdfExporter | user action (save dialog; kiosk network path via `createPdf(path)`) | write | user-selected / `getPrintPDFFilePath()` (appsettings.cpp:976) | no | rendered only — **not parseable back**; not a recovery source | no |
| `userHistory.txt` | appsettings.cpp:851-880 | login-name history | rewrite | CWD | no | no | no |
| `%TEMP%/tachus_seta.lock` | `QLockFile` (main.cpp:63) | single-instance guard | lock | temp dir | Qt-managed | no | no |

## 5. Runtime file paths (Windows)

Launch directory in practice is `release/` next to the executable (dev) —
**but every relative path above resolves to whatever the process CWD is**,
not `applicationDirPath()`. Example resolved paths when launched from the
repo: `...\seta10\release\finals_session.jsonl`, `...\release\Match_Arnold
Bailie.tch`, `...\release\config.ini`.

- **Program Files install**: CWD writes fail (no permission). Consequences:
  `writeJournal` silently returns (open fails, no error surfaced —
  Finals3PController.cpp:91-95); `saveMatch` silently skips (open guard,
  appsettings.cpp:202); `config.ini` still *reads* if placed next to exe
  AND the shortcut sets CWD there — otherwise defaults apply silently.
- **Shortcut with different "Start in"**: session files land in an
  unexpected folder; a desktop-shortcut launch with `Start in: C:\Windows\
  System32` would scatter or lose files. **UNVERIFIED at runtime** — manual
  test: launch via shortcut with altered Start-in and check where
  `finals_session.jsonl` appears.
- **Android**: relative-CWD writes are not viable; only the `%TEMP%` log
  already uses `QStandardPaths`. Everything else must move to
  `AppDataLocation` for portability.
- **Multiple users / copies**: single machine is protected by the lock file
  (temp-dir scoped — i.e. per user session). Two copies run by *different*
  Windows users can run simultaneously; if their CWD is the same shared
  folder, `.tch`/journal collide. Same risk when running from a network
  share.

## 6. QML model lifetime

All are `ListModel`s declared in `ShootingPage.qml` (globalMatchModel:158,
globalSlighterModel:130, globalModelOfData:120, finalsIncidentModel:154,
finalsShotListModel:156) or `RightPanel.qml` (`listModel`, series models);
coach data lives in `CoachReportBridge::m_report/m_shots` (C++, RAM).

Created at page instantiation; cleared by `beginPreparationPhase()`
(ShootingPage.qml:836+) at each session start and by
`rightPanel.resetRightPanelModels()`. Populated per shot as in §3.

**QML ListModel survives:** page navigation: **yes** (ShootingPage stays
instantiated) · floating-window closure: **yes** (windows are overlay items)
· app restart: **no** · power loss: **no**.

`globalMatchModel` is the **only complete copy** of qualification shot data
(score+position included); the `.tch` file is not equivalent (§4). Finals
models are reconstructible in principle from the journal — no code does so.
MissingShot records exist in the controller (`m_missingShots`) and journal
lines only.

## 7. Controller-state inventory

Repo-wide search for `snapshot|restore|serialize|saveState|loadState|
resume|recover`: the only hits are Finals RMS **stubs**
(`applyAuthoritativeState`, `synchroniseClock` — Finals3PController.cpp,
no-ops) and `resumeTrainingSimulation` (pause toggle, not persistence).
**No workflow supports restart recovery.**

| State | Qualification (3P/Prone/Air) | Finals 3P |
|---|---|---|
| phase / mode | `sligterMode`, `matchFinished`, `p3Position` (derived from count), `p3BreaksDone` — ShootingPage QML properties | `m_stage`, `m_window`, `m_targetMode` (Finals3PController.h:209-211) |
| shot counts / next number | model counts | `m_shotsInStage`, `m_officialShotCount`, `stageShotNumberBase()` |
| subtotals / total | right-panel `grandTotal` etc. (recomputed from models) | `m_cumulativeTotal`, `m_stageSubtotalsMap`, `m_stageStatus` |
| timers | QML timers (below) | `m_mono` QElapsedTimer + scaled offsets (h:201-207) |
| duplicate cache | `finalsShotSeq` (QML), backend counter | `m_lastExternalId` per window |
| session metadata | `userName`, APPSETTINGS getters | `m_athleteName`, config struct |
| retained records for report | models | `m_officialShotRecords`, `m_rejectionRecords`, `m_missingShots` (h:235-241) |

All of the above is RAM-only. Finals state is *mirrored* to the journal as
events but never reloaded.

## 8. Timer-state inventory

| Mode | Source | Persisted | Reconstructible | Restart effect |
|---|---|---|---|---|
| Qualification prep/match clocks | QML `Timer`/`gameTimer` (CenterPane) + `rightPanel.timeInSec` 1-s counter (RightPanel.qml:822) | no | no | silently reset to full |
| Finals all phases | `QElapsedTimer m_mono` + scaled segment deadlines (Finals3PController::scaledNow, tick) | no (journal lines carry `issuedAt` scaled ms — enough to *compute* elapsed at last event, but nothing does) | data exists in journal; **no code** | `startFinal()` starts from zero |

Wall-clock is only used for display/timestamps; no timer uses wall-clock as
its base, so the system **cannot distinguish** a 2-second restart, a
10-minute power-off, an official pause, or a crash. No policy exists.

## 9. Startup and shutdown behaviour

**Startup** (`main.cpp:38-160`): QApplication → `QLockFile` single-instance
gate (63-69, styled dialog on conflict) → `AppSettings("config.ini")` (138)
→ MainWindow/TachusWidget construction (never shown) → context properties
(CUSTOMPRINT/COACHREPORT/FINALS3P/FINALSAUDIO…) → `engine.load(main.qml)` →
LoginPage. **No session file is read, no journal is scanned, no cleanup or
archive runs at startup.** (The finals journal archive happens inside
`startFinal()`, not at boot.)

**Normal shutdown**: window close → dialogManager Save-Match? dialog
(main.qml:~480) → optional `APPSETTINGS.saveMatch()` → `Qt.quit()`. No
`aboutToQuit` handlers (grep: none in app code), no controller stop, no
explicit flush (files are opened/closed per write, so nothing is pending),
**no clean-shutdown marker** — a future recovery system cannot tell a clean
exit from a crash.

**Abnormal shutdown**: process kill / crash — all per-write files already
closed; at most an in-progress line/rewrite is lost; RAM state gone. Power
loss — additionally, OS page cache not yet flushed can drop the *last few*
journal lines (no fsync) and can leave a **half-rewritten `.tch`** (truncate
+rewrite has a corruption window on *every shot*). Forced Windows restart ≈
power loss.

## 10. Finals journal analysis

- **Path**: `kJournalPath = "finals_session.jsonl"` (cpp:16) — relative →
  CWD. Not next-to-exe by API; only by convention of launching from
  `release/`.
- **Lifecycle**: `startFinal()` → `archiveExistingJournal()` renames any
  existing file to `finals_session_yyyyMMdd_HHmmss_zzz.jsonl` (cpp:68-76) →
  `writeJournal("finalStarted")`. Archives accumulate unbounded in CWD.
- **Event types written** (grep `writeJournal(`): `finalStarted`,
  `stageEntered`, `command`, `windowOpened`/`windowClosed`, `shotAccepted`,
  `shotRejected`, `missingShot`, `stageStatus`, `finalCompleted`.
- **Line schema**: payload map + injected fields `journalType`, `stage`,
  `targetMode`, `nextShotNumber`, `stageSubtotal`, `cumulativeTotal`
  (cpp:79-96). An accepted-shot line therefore carries: shot number,
  within-stage number, x/y mm, decimal score, direction, ISO timestamp,
  split time, windowId, seriesIndex, target mode, eventId/externalId,
  simulated flag, stage, running subtotal + cumulative total. Command lines
  carry command id/type/text/issuedAt. **Missing: a session ID / header
  line** (athlete, discipline, config, app version) — athlete name is never
  journalled.
- **Durability**: per line `QFile::open(Append|Text)` → write → destructor
  close. Data reaches the OS on every event (survives process kill);
  **no `flush()+fsync`**, so power loss can drop trailing lines or leave a
  torn final line. There is no reader today, so a torn line is currently
  harmless — any future parser must tolerate it.
- **Read-back**: repo-wide, the string `finals_session` appears only in the
  controller (writer), the report view **caption text**, and the test
  harness (which parses it to assert ordering). **The application never
  reads the journal. The report builder reads controller RAM only**
  (`buildReport`, cpp:40-50).

**Classification: audit-grade, write-only.** Data-wise it is *fully
recovery-capable* for finals (every accepted shot + phase + totals per
line); implementation-wise there is zero recovery capability.

## 11. Recovery scenario matrix (current code)

| # | Scenario | Outcome | Class |
|---|---|---|---|
| 1 | Normal close after 10 match shots | `.tch` holds x/y/time (written on shot 10); scores/positions absent; manual Load reconstructs coordinates and re-scores via backend restore path (`uploadGame` → lists → `loadGameInMatchMode`) | **partially recoverable** (single-position); 3P: positions lost |
| 2 | Crash immediately after a shot | same as 1 (the `.tch` rewrite for that shot had completed before the crash *if* the write finished) | partially recoverable / **unknown** in the torn-write window |
| 3 | Power loss after a shot | `.tch` may be torn mid-rewrite (truncate first); finals journal may lose last line(s) | qualification: **possible total loss of the save file**; finals: data survives minus tail |
| 4 | Restart during Kneeling Match | no auto-restore; manual `.tch` Load = coordinates only, no position tags, timer restarts full | partially recoverable |
| 5 | Restart during Finals Standing S1 | journal complete on disk; controller starts Idle; nothing reads journal | **report-only data available; unrecoverable in-app** |
| 6 | Restart after completed final, before PDF | retained records were RAM; report needs `FINALS3P.buildReport()` from RAM → empty | report-only data available (journal), not renderable |
| 7 | Journal exists, models empty | ignored; renamed away at next `startFinal()` | unrecoverable (by design absence) |
| 8 | Torn last JSONL line | nothing parses it today → no effect | n/a now; future parser must skip |
| 9 | App copied to another PC | works if the whole folder (config.ini, licence.txt) is copied; session files come along only if in that folder | config-portable; sessions incidental |
| 10 | Read-only install dir | journal + `.tch` + userHistory writes **silently do nothing**; app runs, looks normal | **unrecoverable + silent** (worst case) |
| 11 | Two copies simultaneously | same user/machine: blocked by `%TEMP%` lock (main.cpp:63); different users or network-share CWD: both run, files collide | partially protected |
| 12 | Stale unfinished journal at new match | finals: archived by rename ✓; qualification `.tch`: overwritten (`match_file=clear`) or new timestamped file | handled (finals), lossy (qualification) |

## 12. Source-of-truth matrix

| Data | Authoritative now | Disk copy | Wins on disagreement |
|---|---|---|---|
| Qualification official shots | `globalMatchModel` (QML RAM) | `.tch` (lossy, backend-sourced) | model (reports/panels read it) |
| Sighting shots | `globalSlighterModel` | none (finals: journal) | model |
| Finals official shots | `Finals3PController` retained records (+ models mirrored, dev-asserted equal) | journal (complete) | controller |
| Score totals | qualification: right-panel recompute from model; finals: controller `m_cumulativeTotal` | journal lines (finals) | controller/model |
| Match phase / stage | ShootingPage properties / controller `m_stage` | journal `stageEntered` (finals) | RAM |
| Timer | QML timers / `m_mono` | none | RAM |
| Report data | builders reading RAM | none | RAM |
| Completed-session archive | — | journal archives (finals only); `.tch` (lossy) | n/a |
| Backend MODREADER lists | never authoritative (mode-swapped; established in the coach-feed fix, `feedCoachReport` comment ShootingPage.qml:635-642) | — | — |
| PDF | output only — **not** a recovery source (no parser) | — | — |

## 13. Gap analysis

**Critical**
1. **All session writes are CWD-relative and fail silently** —
   Finals3PController.cpp:16/91; appsettings.cpp:196-202. Affects all
   disciplines. Impact: scenario 10 (silent total loss). Minimum fix: write
   under `QStandardPaths::AppDataLocation` + surface write failures.
   **Fix before 3P Training: yes** (training will rely on the same store).
2. **`.tch` is truncate-rewritten on every accepted shot** —
   CenterPane.qml:333 + appsettings.cpp:202 (`Truncate`). Corruption window
   per shot; non-atomic; I/O churn. Minimum: atomic replace (QSaveFile) or
   retire in favour of a journal. Fix before training: yes.
3. **`.tch` content is wrong for 3P** — sourced from backend lists
   (appsettings.cpp:218-227) that swap on sighter/match transitions
   (tachuswidget.cpp:1195+), and stores no score/position/sighter-flag.
   Affects 3P qualification. Minimum: source any save from
   `globalMatchModel`. Fix before training: yes (training is 3P).
4. **No restore path exists for any mode** (search §7). Impact: scenarios
   4-7. Minimum: journal reader + controller/model replay. This *is* the
   recovery project. Fix before training: the store, yes; full resume UI
   can land with training.

**High**
5. Journal has no session header (no session ID, athlete, discipline,
   config, app version) — replay cannot self-identify. Minimum: one header
   line per session.
6. No fsync/flush policy — power loss can drop accepted shots' tail lines
   (Finals3PController.cpp:91-95). Minimum: `flush()+FlushBuffer`/fsync on
   `shotAccepted` lines at least.
7. Timers unrecoverable in every mode (§8); journal lacks wall-clock anchor
   pairs (scaled-ms only + per-line wall timestamps exist on shots but not
   a monotonic↔wall mapping line). Minimum: journal a `clockAnchor` event.
8. Completed finals cannot re-render a report after restart (scenario 6).
9. `autoSaveMatch` operator-precedence bug: `open(WriteOnly|Text) |
   QIODevice::Truncate` (appsettings.cpp:249) — condition always truthy,
   Truncate never applied. Currently harmless (**no callers found** in
   QML/C++), but a trap. Qualification-only.
10. No qualification journal at all — the authoritative model has no
    event-grade disk mirror.

**Medium**
11. Journal archives accumulate unbounded in CWD; no retention policy.
12. No clean-shutdown marker (crash vs clean exit indistinguishable).
13. No recovery UI (Resume/Discard) — required by the finals doc's
    persistence section (docs/3p-finals-discipline.md, "On restart: offer
    Resume Final…") but never built.
14. Kiosk lane files duplicate shot data with yet another schema
    (tachuswidget.cpp:1440-1456) — not integrated with any recovery story.

**Low**
15. `.tch` naming/format (XML, brand-era extension); registry uses two
    brand names (appsettings.cpp:294-330); `userHistory.txt` in CWD.

## 14. Recovery architecture options (proposal only — not implemented)

| Criterion | **A: Extend existing JSONL** | B: SQLite only | C: SQLite snapshot + JSONL journal |
|---|---|---|---|
| Offline | yes | yes | yes |
| Effort | **low-medium** — writer is proven; add header line, fsync policy, AppData path, a reader/replayer, and a qualification journal using the same writer | high — schema design, migrations, replace two write paths, transaction policy | highest — both systems + consistency between them |
| Crash safety | append-only is inherently torn-tail-tolerant; with per-shot flush ≈ one-shot max loss | good (WAL) but a single DB file is a single corruption target on hard power loss | best (journal replays over snapshot) |
| Write performance | one small append per event (already the case) | fine | fine |
| Report regeneration | **natural fit**: `FinalsReportBuilder` already consumes QVariant lists — a journal replay produces exactly its inputs; qualification reports read models a replay can refill | needs query→model adapters | same as A via journal |
| Android portability | yes (AppDataLocation + plain files) | yes (Qt SQL plugin needed) | yes |
| RMS compatibility | events stream naturally to a server later (event log ≙ SIUS-style feed) | needs an event export anyway | best long-term |
| Migration complexity | none (same format, new location + header) | full | full |
| Testing burden | reuse of the 178-check harness pattern (it already parses the journal) | new | largest |

**Recommendation: Option A** — extend the JSONL journal. The finals journal
already contains everything needed per event and the test harness already
parses it; the report builder's QVariant inputs make replay a thin adapter;
the codebase has zero SQL today and no query-shaped needs yet. Required
extensions: (1) move to `QStandardPaths::AppDataLocation` with surfaced
write errors, (2) session-header line with session ID/athlete/discipline/
config/app version, (3) flush-on-shot durability, (4) clockAnchor +
clean-shutdown marker events, (5) the same writer for qualification (and 3P
Training), (6) a startup scanner + Resume/Discard dialog (already envisaged
in docs/3p-finals-discipline.md). Option C remains the natural RMS-era
evolution — the journal format chosen now should be designed so a later
SQLite snapshot can be added *underneath* it without changing the event
schema.

## 15. Recommended next step

Approve the Option-A design pass (SessionStore/journal spec + path
migration + replay adapters) as its own phase before `feature/3p-training`,
since training sessions (long, interruptible) are the primary beneficiary.
No implementation has been started.

---
*Unverified items (manual tests proposed):* shortcut-CWD behaviour (§5);
torn-`.tch` recovery behaviour of `uploadGame` on a corrupt file; whether
kiosk lane files are enabled in the user's current config
(`isServerNetworkEnabled`).
