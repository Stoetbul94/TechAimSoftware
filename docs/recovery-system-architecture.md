# TechAim Session Reliability Layer — Recovery System Architecture

> ## HISTORICAL DESIGN SPECIFICATION
>
> This document records the original architecture and design intent.
> The described reliability and recovery foundation was subsequently
> implemented through M0–M3 and reliability Phases A–F.
>
> It is not, by itself, the final as-built verification record.

**Current implementation and verification documentation**

| Topic | Document |
|---|---|
| M0 — storage paths, surfaced failures | `docs/session-reliability-m0-storage.md` |
| M1 — core event/journal library | `docs/session-reliability-m1-core.md` |
| M2 — finals write path, SessionStore | `docs/session-reliability-m2-finals-write-path.md` |
| M3 — replay, recovery coordinator, resume | `docs/session-reliability-m3-recovery.md` |
| Phase B — qualification journaling | `docs/session-reliability-phaseB-qualification.md` |
| EST incidents (cross-discipline rules) | `docs/issf-rules/est-malfunctions.md` |
| As-built source | `src/reliability/{core,events,journal,reducer,replay,recovery,storage,store}` |
| Test evidence | `tests/reliability/` (864 checks, 0 failures) |

Original basis (retained as written): full codebase read + `docs/current-session-persistence-audit.md`.
Scope: every discipline (10m AR/AP, 50m Prone, 3P Qualification, 3P Finals),
future Training, future RMS. Fully offline. The invariant this layer exists
to guarantee: **an accepted shot, once acknowledged to the athlete, is never
lost** — not by crash, power cut, reboot, accidental close, or disconnect.

---

---

## Design versus as-built status

Verified against the source tree at commit `cc69939`. This section is a map,
not a re-litigation of the design.

### Implemented as designed

| Design element | As-built evidence |
|---|---|
| Session Reliability Layer as a separate, QtCore-only module | `src/reliability/` — eight sub-modules, no GUI dependency |
| `SessionStore` as the single source of truth | `src/reliability/store/SessionStore.*` |
| Append-only JSONL journal | `src/reliability/journal/JournalWriter.*` |
| **Amendment 1 — snapshots as events *inside* the journal**, not separate files | `StateSnapshot` is a first-class event type (`events/EventTypes.h`); there is no separate snapshot file |
| Replay engine folding journal → state | `src/reliability/replay/ReplayEngine.*` |
| Recovery workflow + candidate discovery | `src/reliability/recovery/RecoveryCoordinator.*` |
| Storage paths resolved from `AppLocalDataLocation`, failures surfaced not swallowed | `src/reliability/storage/StoragePaths.*`; startup blocks with Retry/Exit |
| Read-only legacy importer, legacy writer retired | `StoragePaths::migrateLegacyJournals()` |
| Validation before use | `journal/JournalValidator.*` |

### Implemented differently

| Design said | As built | Why |
|---|---|---|
| CRC32 **and** chained SHA-256 (two mechanisms) | **One** chained-hash field serving as both corruption checksum and tamper evidence (`journal/HashChain.*`) | The `[SUPERSEDES]` amendment in the implementation specification was carried through: QtCore has no CRC32, and a hand-rolled one beside a required SHA-256 is redundant code to maintain. |
| A modest fixed event catalogue | **70 event types**, grown across Phases A–F | The catalogue expanded as EST incidents, qualification, 10m finals and the Training Lab were added. New events are appended at the END of the variant so no prior index or hash moves. |
| Scope named "future Training" | Training Lab shipped as its own domain with `sessionKind="Training"` and its own store | Training became a real domain (Technical Blocks, Call & Diagnose, Group Pattern, Position Transition) rather than a later addition to the competition path. |

### Superseded

| Design element | Status |
|---|---|
| **`CompetitionEngine` — "Required design decision: Option B"** | **Superseded — never built.** No `CompetitionEngine` type exists. Discipline logic stayed in the per-discipline controllers (`Finals3PController`, `Finals10mController`, `QualificationController`, the Training controllers), with the reliability layer beneath them. The unifying engine was not needed to reach the invariant. |
| Separate snapshot files (the original Option C sketch) | Superseded before implementation by Amendment 1, above. |

### Still pending

| Design element | Status |
|---|---|
| **Phase 13 — Android** | **Not started.** The only `android` token in the build is qmake boilerplate (`unix:!android`) in `Seta.pro`. No Android target exists. |
| **Phase 14 — Future RMS / spectator contract** | **Not started.** No spectator or RMS interface exists in `src/`. The lane application retains UDP shot broadcast and `fromServer` hooks, and the finals report header reserves Lane/Target ID, but the RMS application itself is a separate future product. |
| Physical-hardware verification of the recovery path | **PHYSICAL TARGET DEPENDENT** — crash/recovery has been exercised in tests, not against a live target mid-match. See `docs/manual/target-connection-validation.md`. |


## Phase 1 — Architecture review: confirming and challenging the audit

| Audit conclusion | Verdict after re-reading the code | Consequence |
|---|---|---|
| Finals journal is data-complete but write-only | **Confirmed** (`Finals3PController::writeJournal`, cpp:79-96; no reader) | Reuse the event vocabulary; do not reuse the writer as-is |
| All session writes are CWD-relative and fail silently | **Confirmed** (`kJournalPath` cpp:16; `open()` guards without surfacing) | StoragePaths module is milestone 0 |
| `.tch` per-shot truncate-rewrite is unsafe and 3P-wrong | **Confirmed** (appsettings.cpp:192-236; CenterPane.qml:333) | Retire the writer; keep a read-only legacy importer |
| Recommended Option A: extend JSONL | **Confirmed with two amendments** (below) | — |
| Snapshots optional, "SQLite underneath later" (Option C) | **CHALLENGED** — separate snapshot files create a two-file consistency problem (snapshot/journal version skew, double fsync ordering, partial-pair states). This codebase's sessions are small (≤ a few hundred events); replay is microseconds. The classic reason for snapshot *files* (replay cost) does not exist here. | **Amendment 1: snapshots become events _inside_ the journal** (`StateSnapshot` lines). One file, one ordering, zero cross-file consistency logic. Rotation for long Training sessions carries the latest snapshot forward as the first line of the next segment. |
| Journal format JSONL vs binary vs SQLite | JSONL **confirmed**, with **Amendment 2: a checksummed envelope per line** (the current journal has no integrity data at all). Binary rejected: size is irrelevant (~50–150 KB/session), and human-readability is operationally decisive for a small team doing field debugging (the 178-check harness already greps JSONL). SQLite-as-journal rejected for the hot path: a single DB file is a single corruption target under hard power loss, adds a Qt plugin + migration machinery, and none of the recovery needs are query-shaped. SQLite remains the correct *later* addition as a derived **index** for history browsing / RMS — built FROM journals, never authoritative (see Phase 14). | Format: one self-checkpointing, checksummed, append-only JSONL journal per session. |
| Controllers/QML models/reports as-is | **Challenged as end-state**: today `globalMatchModel` is authoritative for qualification (audit §12). That inversion — a display technology owning official data — is the root defect. | Phase 3 makes `SessionStore` the single owner; models become views. Migration is incremental (Phase 15), not a big-bang QML rewrite. |

## Phase 2 — The Session Reliability Layer (SRL)

A standalone subsystem in `src/reliability/` (pure QtCore, unit-testable
exactly like `src/finals/` + its console harness). **No controller writes a
file after this ships.** All persistence flows through the SRL.

```
                    ┌──────────────────────────────────────────────────────┐
                    │              SESSION RELIABILITY LAYER               │
   controllers      │                                                      │
  (state machines)  │  SessionStore ───── single source of truth (RAM)    │
        │ submit    │      │  apply/notify                                 │
        ▼           │      ▼                                               │
   domain events ──►│  JournalWriter ──► session_<id>.jsonl (append, CRC,  │
                    │      ▲                     fsync policy, rotation)   │
                    │      │ replay                                        │
                    │  JournalReader/Validator ◄── startup scan            │
                    │      │                                               │
                    │  ReplayEngine ──► SessionStore ──► controllers.prime │
                    │      ▲                             models.refill     │
                    │  RecoveryCoordinator (scan/classify/dialog/decide)   │
                    │  StoragePaths (QStandardPaths adapter, per-platform) │
                    │  BackupService (archive/rotate/export copies)        │
                    └──────────────────────────────────────────────────────┘
        views: QML models / right panel / HUD   projections: report builders
```

Responsibilities owned exclusively by the SRL: session persistence,
journaling, recovery, snapshots, backups, validation, replay, corruption
detection, storage paths, durability. Explicit non-responsibilities: domain
legality (controllers), rendering (QML), report layout (builders).

## Phase 3 — Single source of truth: `SessionStore`

Name: **SessionStore** ("Match" is too narrow for Training; "Competition"
is too narrow for practice). Exactly one instance owns the live `Session`.

- **Controllers become pure state machines**: they validate domain legality
  (exactly what `Finals3PController::registerShot` does today) and then
  *submit an event* to the store instead of mutating/journalling privately.
  The store persists first, applies second, notifies third (**write-ahead**:
  an event that cannot be persisted is rejected back to the controller and
  surfaced — never silently accepted).
- **QML models become views**: the existing router pattern already works
  this way for finals (`onShotAccepted` → appends). The store's notification
  signals feed the same routers; `globalMatchModel` et al. stop being
  authoritative and become rebuildable mirrors.
- **Reports become projections**: `FinalsReportBuilder` already is one
  (QVariant in → report out). Qualification report views migrate from
  reading models to reading store projections in a late milestone.
- **Persistence becomes serialization** of store events/state — nothing
  else may own or write official data (the `.tch` writer, the ad-hoc kiosk
  lane writers, and the controller's private `writeJournal` are all
  retired or re-routed through the SRL).

Ordering rule (the loss-prevention core): **journal append (durable per
policy) → state apply → view notify → athlete feedback.** The shot the
athlete sees was on disk before they saw it.

## Phase 4 — Session model

```cpp
struct Session {                          // schemaVersion: 1
  // identity — required to find, dedupe and sync sessions
  QString  sessionId;        // UUIDv4; every journal line carries it
  int      schemaVersion;    // migration gate
  QString  appVersion;       // diagnostic + migration hints
  QString  createdAtIso;     // wall clock at creation
  // who/where — header data for reports and RMS
  QString  athlete;          // reports, recovery dialog text
  QString  lane, targetId;   // reserved (RMS); already reserved in finals report header
  // what — the discipline router for replay
  Discipline discipline;     // AR10|AP10|PRONE50|Q3P50|FINAL3P|TRAINING
  QString  matchType;        // event card (e.g. MATCH-60, FINAL 35, free)
  QVariantMap disciplineConfig; // shots/series counts, timing config actually in force
  // live state — everything a controller needs to be re-primed
  int      stageId;          // finals Stage / qualification phase encoding
  int      targetMode;       // sighter|match
  int      positionIndex;    // 3P K/P/S (derived-but-stored: replay cross-check)
  int      nextOfficialShot; // continuation point
  int      windowId;         // finals firing-window duplicate scope
  qint64   lastExternalId;   // duplicate-guard continuation
  // records — the official data itself
  QVector<ShotRecord> officialShots;  // full existing role union (xmm/ymm/score/…)
  QVector<ShotRecord> sighters;
  QVector<QVariantMap> missingShots, incidents, penalties, warnings;
  //          penalties/warnings empty today — schema slots so ISSF penalties
  //          (deferred feature) and RMS decisions need no format change
  // derived-but-stored (cheap, enables validation: replay must reproduce them)
  double   cumulativeTotal;
  QVariantMap stageSubtotals, stageStatuses;
  // time — the only way any timer policy can be computed after restart
  QVector<ClockAnchor> clockAnchors;  // {monotonicMs, wallIso} at start/stage/pause/resume
  qint64   pausedAccumMs; bool paused;
  qint64   stageStartMono, segmentEndMono;   // finals deadlines (scaled ms)
  // metadata
  QVariantMap coachDiaryRef;  // pointer/inline copy of diary (small)
  QVariantMap uiHints;        // NON-authoritative: last page, right-panel page —
                              //   restored best-effort, never validated
  bool     developerMode;     // affects journal verbosity only
  // lifecycle
  SessionState state;         // Active|Interrupted|Complete|Aborted|Closed
};
```

Justifications for exclusions: **report cache** is not stored (pure
projection; storing it invites divergence); **window geometry** is not
stored (FloatingWindows are transient tools); backend MODREADER lists are
never serialized (audit §12 — not a record).

## Phase 5 — Journal design

One file per session: `session_<sessionId>.jsonl`. Line 1 is always the
header. Envelope for **every** line:

```json
{"v":1,"sid":"<uuid>","seq":42,"tw":"2026-07-17T20:31:04.113","tm":184552,
 "type":"OfficialShotAccepted","p":{ ...payload... },"crc":"a1b2c3d4"}
```

- `v` event-schema version (per-event, enables per-type migration) ·
  `sid` session id · `seq` strictly monotonic from 0 (header) ·
  `tw` wall clock · `tm` monotonic ms since session start ·
  `crc` CRC32 over the canonical JSON of all other fields.
- **Event taxonomy** (superset of today's finals events, renamed to be
  discipline-neutral): `SessionStarted` (header: full descriptor +
  disciplineConfig + appVersion), `PreparationStarted`, `SighterAccepted`,
  `OfficialShotAccepted`, `ShotRejected`, `MissingShotRecorded`,
  `SeriesCompleted`, `StageEntered`, `StageStatusChanged`,
  `PositionChanged`, `TargetModeChanged`, `WindowOpened`, `WindowClosed`,
  `CommandIssued`, `TimerPaused`, `TimerResumed`, `ClockAnchor`,
  `PenaltyIssued`, `WarningIssued`, `StateSnapshot`, `MatchFinished`,
  `ReportGenerated`, `CleanShutdown`, `SessionClosed`,
  `RecoveryCompleted`, `SessionArchived`.
- Format decision: **JSONL** (rationale in Phase 1 table). The writer keeps
  the file handle open for the session (vs today's open/close per line),
  with reopen-on-error and error surfacing through `dialogManager`.

## Phase 6 — Snapshots: yes, as in-journal events

`StateSnapshot` = the full serialized `Session` as an ordinary journal line.

- **Triggers**: every 50 events, AND on every stage/position transition,
  AND on `MatchFinished`, AND on clean shutdown. (Shot-rate math: a 3P
  qualification ≈ 60 shots + ~80 auxiliary events → 3–4 snapshots.)
- **Recovery speed**: scan backwards for the last CRC-valid snapshot, replay
  the (≤ 50) events after it. Estimated total < 20 ms for any realistic
  session — snapshots here buy *validation cross-checks* and *rotation*,
  not speed.
- **Rotation** (Training sessions can be hours): when a segment exceeds
  ~2 MB or 5 000 events, close it as `session_<id>.part<N>.jsonl` and open
  `part<N+1>` whose first two lines are the header + a fresh snapshot. Only
  the newest segment is needed for resume; older parts are audit history.
- Replay-vs-snapshot disagreement (derived totals mismatch) ⇒ journal is
  authoritative event-wise, but the session is flagged and surfaced —
  never silently repaired.

## Phase 7 — Recovery workflow

```
 App start ─► StoragePaths.scan(sessions/current/)
     │
     ├─ none unfinished ──────────────────────────► LoginPage (normal)
     │
     └─ unfinished found (no SessionClosed/CleanShutdown marker)
            │  for each: JournalReader.validate()
            ▼
     classify ──► CLEAN          (header ok, CRCs ok, seq contiguous)
             ──► TORN-TAIL      (valid prefix + torn last line → auto-truncate
             │                    tail, log RecoveryNote, treat as CLEAN)
             ──► GAPPED         (seq hole → replay stops at gap; resumable to
             │                    the gap point only; warn explicitly)
             ──► CORRUPTED      (header/CRC unusable → move to sessions/
             │                    corrupted/, offer nothing but the raw file)
             └─► NEWER-VERSION  (schemaVersion > app's → read-only: offer
                                  report/archive, never resume)
            │
            ▼
   dialogManager (TechAim framework):
   ┌──────────────────────────────────────────────┐
   │ ⚠ Unfinished session found                    │
   │ Arnold Bailie — 3P Qualification — 34/60 shots│
   │ Interrupted 2026-07-17 20:31 (12 min ago)     │
   │        [Discard]  [Archive]  [Resume]         │
   └──────────────────────────────────────────────┘
   Resume  ─► ReplayEngine ─► controllers primed ─► UI restored ─► continue
   Archive ─► move to sessions/archive/… (+ SessionArchived event appended)
   Discard ─► same as Archive but flagged discarded (never delete bytes)
   Multiple unfinished ─► newest first; list dialog; non-chosen offered
                          Archive-all (Training may legitimately have several)
```

**Timer policy on resume** (design decision, default proposed, easily
changed because the journal stores both clock bases): *interruption time
does not count* — remaining time is restored to its value at the last
journalled event; for finals command-windows the current window is
re-opened with its remaining duration; a `RecoveryCompleted` event records
old/new anchors. Alternative policies (wall-clock continues / CRO decides)
are computable from the same `ClockAnchor` data. **Needs your ruling.**

## Phase 8 — Storage layout

Base: `QStandardPaths::AppDataLocation` → Windows
`C:/Users/<u>/AppData/Local/TechAim/TechAim` (Local, not Roaming: sessions
are machine/lane-specific and must not ride profile sync). Android: the
same API → app-private storage, no permissions needed.

```
TechAim/
├── sessions/
│   ├── current/      session_<id>.jsonl (+ .partN)
│   ├── archive/<yyyy>/<mm>/
│   └── corrupted/
├── reports/          optional copies of generated PDFs (user exports stay user-chosen)
├── exports/          RMS/CSV outbox (future)
├── backups/          rolling copies of archive (BackupService)
├── logs/             relocated tachus_log (later milestone)
├── settings/         config migration target (config.ini stays until its own milestone)
└── VERSION           layout version marker for future migrations
```

Writability is verified at startup with an explicit probe; failure surfaces
a blocking TechAim dialog (never silent). Legacy CWD files (`finals_
session*.jsonl`, `Match_*.tch`) are detected once and offered for import/
archive (migration strategy, Phase 15/M0).

## Phase 9 — Durability policy

| Event class | Write | Flush | fsync (`FlushFileBuffers`/`fsync`) | Rationale |
|---|---|---|---|---|
| `OfficialShotAccepted`, `MissingShotRecorded`, `PenaltyIssued`, stage/position transitions, `MatchFinished`, lifecycle + snapshots | immediate | yes | **yes, synchronous** | the never-lose set; cost 1–10 ms on SSD/eMMC, once per shot (shots are seconds apart) — imperceptible latency, negligible battery |
| `SighterAccepted`, `TargetModeChanged`, `WindowOpened/Closed`, `CommandIssued`, timer events | immediate | yes | no (OS cache) | worst case on power cut: lose trailing *auxiliary* lines; sighters are the accepted maximum loss (≤ the last event) |
| `ShotRejected`, dev-verbosity | immediate | yes | no | diagnostics |

No batching (event rates are human-speed). Handle kept open; every write's
return value is checked; on failure the store rejects the event and raises
an error dialog + red HUD state — **a shot is never acknowledged that
wasn't persisted**. Expected added latency per official shot: **< 10 ms**
(measured milestone M2 acceptance criterion; budget 25 ms).

## Phase 10 — Replay engine

Deterministic left-fold, no I/O other than the journal:

```
replay(file):
  header = parse(line0); session = Session(header)
  base   = lastValidSnapshot(file)          // optional
  if base: session = base.state
  for line in lines after base:
      validateEnvelope(line)                // Phase 11
      session = apply(session, event)       // pure function per event type
  return session
```

`apply` lives in the SRL (not in controllers) so replay never needs UI or
hardware. Priming after replay:

```
Session ──► controllers:  FinalsController.primeFrom(session)   [new API]
        │                 QualificationFlow.primeFrom(session)   [new API]
        ├─► models:       routers refill globalMatchModel/sighter/display
        ├─► reports:      builders already consume this shape (no change)
        ├─► statistics:   right panel recomputes from refilled models (existing)
        └─► UI:           page navigation + uiHints (best-effort)
```

Invariant tests: `replay(journal(live_session)) == live_session` (field-by-
field), and controller-primed state passes the same 178-check harness
assertions as a natively-run session.

## Phase 11 — Validation

Per line: CRC32 mismatch ⇒ line invalid. Position in file decides effect:
invalid **final** line = torn tail (auto-truncate, resumable); invalid
**interior** line = corruption (session → corrupted/, no resume). Sequence:
`seq` must increment by exactly 1; duplicate `seq` with identical CRC ⇒
idempotent drop (logged); duplicate with different CRC or a gap ⇒ stop at
last good prefix (GAPPED class). `sid` must match header on every line.
Schema: per-event `v` with an upgrade table (`v1→v2` pure functions);
unknown *newer* `v` ⇒ NEWER-VERSION class. Domain validation during
replay: shot numbers must be `next==prev+1` within stage rules, stage order
must be legal (reuses controller legality tables) — violation ⇒ corrupted
classification with the offending `seq` reported. Corrupt JSON ⇒ same as
CRC failure.

## Phase 12 — Testing strategy

All console-harness style (like `tests/finals/`), CI-able, no GUI needed:

1. **Truncation sweep**: generate a full session journal, then for every
   byte offset N: truncate to N, run recovery classify+replay, assert
   (a) no crash, (b) no accepted-official-shot loss beyond the durability
   contract, (c) torn-tail auto-truncation only ever removes the final
   line. (This is the "power failure after every shot" test, made total.)
2. **Kill-after-every-event**: drive the store through scripted sessions
   for each of the 5 disciplines, snapshotting the expected Session after
   each event; replay from each prefix must equal the expectation.
3. **Fault injection**: `JournalWriter` takes an injectable file interface —
   simulate disk-full, permission-denied, read-only dir, write-short,
   flush-failure; assert events are rejected and surfaced, never
   half-acknowledged.
4. **Corruption fuzz**: bit-flip every line of valid fixtures → classify
   must never resume corrupted interiors; duplicate-line injection →
   idempotent; reordered lines → GAPPED.
5. **1000-restart simulation**: loop of {run K random events, kill, recover,
   resume} × 1000 with invariant checks each cycle.
6. **Version fixtures**: committed v1 journals replayed by future versions
   (regression), plus a synthetic v99 file ⇒ NEWER-VERSION path.
7. **Manual checklist**: real power-cut on the tablet mid-string; Program
   Files install; two-user collision; Android file-location smoke.

## Phase 13 — Android

The SRL is pure QtCore + `QStandardPaths` → unchanged. The only platform
code is the fsync adapter (`IPlatformSync`: `FlushFileBuffers(HANDLE)` on
Windows / `fsync(fd)` on POSIX/Android) — one small class behind an
interface, selected at compile time. App-private storage needs no Android
permissions; user-facing exports (PDF) go through Qt's SAF-backed dialogs
as they already do. API identical on both platforms by construction.

## Phase 14 — Future RMS

The journal **is** the sync protocol: events are immutable, idempotent
(`sid`+`seq`) and self-describing. The lane's RMS agent (future) tails the
current journal (or the store re-emits events on the wire, as the existing
UDP lane-file hooks foreshadow) and the server ingests with at-least-once
semantics — duplicates are dropped by `sid+seq`. **The lane remains
authoritative forever**: RMS acks never mutate lane state; RMS "decisions"
(penalties etc.) arrive as *proposed events* that the lane journals itself.
The ad-hoc kiosk writers (`<lane>.txt`, shootdata, status.csv) are marked
for retirement into this channel (their own milestone, post-recovery). A
derived SQLite index (sessions/index.db, rebuildable at any time from
journals) is the designated home for history browsing and RMS bookkeeping —
never a source of truth.

## Phase 15 — Implementation roadmap (each milestone compiles, tests, commits independently)

| M | Deliverable | Test gate |
|---|---|---|
| **M0** | `StoragePaths` + writability probe + move the existing finals journal writes to AppData with surfaced errors; legacy-file detection/import stub | harness green; journal appears in AppData; read-only-dir dialog |
| **M1** | SRL core: envelope writer/reader/validator, `Session` model, `SessionStore` skeleton, snapshot events — pure QtCore + own console harness (`tests/reliability/`) | truncation sweep + corruption fuzz on synthetic sessions |
| **M2** | Finals on SRL: controller submits events (replaces `writeJournal`), durability policy live, header/snapshot/clock-anchor events | 178-check harness ported + replay-equivalence checks; latency budget measured |
| **M3** | Finals recovery: `primeFrom(session)`, RecoveryCoordinator + Resume/Discard dialog, timer policy default | kill-after-every-event (finals); manual restart mid-final |
| **M4** | Qualification journaling (all 4 disciplines): shot/sighter/position/timer events through SRL; **retire `.tch` writer** (keep read-only legacy import) | qualification event-fold tests; .tch import fixture |
| **M5** | Qualification recovery incl. 3P positions + timers | kill-sweep (qualification); manual 3P restart |
| **M6** | Reports-after-restart + minimal session browser (archive list → regenerate report projections) | finals report from archived journal == report from live run |
| **M7** | Hardening: fault injection, 1000-restart sim, disk-full/permission UX, docs + CLAUDE.md refresh | full suite green; risk sign-off |
| M8 (later) | Derived SQLite index; kiosk-writer retirement into RMS channel | — |

Training (and RMS) then build **on top of** the SRL, not beside it.

## Risk analysis

| Risk | Sev | Mitigation |
|---|---|---|
| Dual-authority window during migration (store vs models, M2–M5) | High | per-milestone dev-mode equivalence assertions (pattern already proven in FIX4/coach-feed) |
| fsync latency on slow eMMC tablets | Med | measured gate in M2; policy degradable to flush-only per config with explicit warning |
| Replay/controller drift (two implementations of legality) | High | `apply()` reuses controller legality tables; harness runs both paths on identical scripts |
| Timer-policy dispute after real interruptions | Med | policy isolated behind one function; journal keeps both clock bases so any policy is retro-computable |
| Legacy `.tch`/journal orphans confusing users | Low | one-time detection + guided import/archive in M0/M4 |
| Schema churn during Training development | Med | per-event versioning from day one; fixtures committed per version |

## Migration strategy

M0 detects legacy CWD artefacts once: `finals_session*.jsonl` → archived
into the new tree verbatim (they lack envelopes; kept as audit history,
optionally wrapped by a v0-import tool later); `Match_*.tch` → read-only
importer (M4) mapping x/y/time into a synthetic journal marked
`imported=true` (scores recomputed by the existing engine on load, as
`uploadGame` does today). `config.ini` stays where it is until its own
settings milestone — the SRL does not gate on it.

## API proposal (sketch)

```cpp
namespace techaim::reliability {

class SessionStore : public QObject {
  Q_OBJECT
public:
  // lifecycle
  QString beginSession(const SessionDescriptor&);        // writes header, returns sessionId
  Result  submit(const Event&);                          // journal(durable) → apply → notify; Result carries persistence failure
  void    closeSession(CloseReason);                     // MatchFinished/CleanShutdown/SessionClosed
  const Session& current() const;
signals:
  void eventApplied(const Event&);                       // views/routers subscribe
  void persistenceFailed(const QString& reason);         // dialogManager + HUD
};

class RecoveryCoordinator : public QObject {
  Q_OBJECT
public:
  QVector<RecoveryCandidate> scan();                     // classify all unfinished
  Session resume(const QString& sessionId);              // validate+replay, appends RecoveryCompleted
  void archive(const QString& sessionId, bool discarded);
};

// controllers implement:
struct ISessionParticipant {
  virtual void primeFrom(const Session&) = 0;            // restore state machine
  virtual QVariantMap uiHints() const = 0;
};
} // namespace
```

## Class diagram

```
SessionStore ──owns──► Session
     │ uses                    ▲ builds
     ▼                         │
JournalWriter            ReplayEngine ◄──uses── JournalReader/Validator
     │ writes                  ▲
     ▼                         │ scans/classifies
session_<id>.jsonl ◄──reads── RecoveryCoordinator ──dialog──► dialogManager
     ▲ paths                                          primeFrom
StoragePaths ◄─probe                    Finals3PController / QualificationFlow
BackupService ──copies──► archive/backups            │ submit(Event)
                                                     ▼
                                               SessionStore
```

## Sequence — one official shot (target ≤ 25 ms added, expected < 10 ms)

```
hardware/demo ─► scoring ─► controller.validate ─► store.submit(OfficialShotAccepted)
                                     │ reject if illegal      │
                                     ▼                        ▼
                              (unchanged today)      JournalWriter.append+flush+fsync
                                                              │ ok?
                                              no ─► Result::failed ─► error dialog,
                                                              │        shot NOT applied
                                              yes ─► Session.apply ─► eventApplied
                                                              │
                                          routers append models ─► HUD/panel/face update
```

## Session lifecycle state diagram

```
          beginSession            MatchFinished          closeSession
  (none) ────────────► ACTIVE ─────────────────► COMPLETE ───────────► CLOSED
                         │  crash/power/kill            │ report/export
                         ▼                              ▼
                    INTERRUPTED ──resume──► ACTIVE   (archive)
                         │ discard/archive/corrupt
                         ▼
                ARCHIVED / CORRUPTED  (bytes never deleted)
```

## Performance estimates

Journal line ≈ 200–400 B → 3P qualification ≈ 60–120 KB; finals ≈ 40 KB;
8-hour training ≈ 1–3 MB (rotated). Replay ≤ a few thousand lines: < 20 ms.
fsync per official shot: 1–10 ms SSD/eMMC (once per multi-second shot
cycle). Startup scan of `current/`: O(open files), typically 0–2 sessions.
Battery impact: unmeasurable at these rates.

---

### Decisions required from you before M0

1. **Timer resume policy** — proposed default: interruption time does not
   count (Phase 7). Confirm or choose an alternative.
2. **Sighter durability** — proposed: flush-not-fsync (may lose the final
   sighter on hard power cut). Confirm.
3. **Recovery dialog scope** — resume offered for all disciplines from M5,
   finals-first from M3. Confirm milestone order.
4. Approve the roadmap M0–M7 as the implementation sequence.


---
---

# ARCHITECTURE REVIEW (adversarial) and VERSION 2

Reviewed as lead architect against a 15-year commercial horizon (Windows,
Android, multi-lane, RMS, national championships, ISSF, future cloud).
The review's job was to attack V1. It found one correctness flaw, one
portability trap, and several under-engineered areas. V2 below supersedes
V1 where stated; everything else carries forward.

## R1. Subsystem-by-subsystem critique

**1. SessionStore.** Right abstraction, wrong internal shape. V1 fuses
three concerns (state ownership, persistence orchestration, notification)
— a God-object by year 5. Worse, `Session` is QVariantMap-heavy:
stringly-typed payloads mean typos compile fine and fail at replay, and
15 years of "what keys exist?" archaeology. VERDICT: keep the store as a
façade; split internals (Reducer core / JournalManager / Notifier); type
the events.

**2. Journal format — a real trap found.** V1 computes CRC32 over
"canonical JSON". Canonical according to whom? Qt's compact serialization
(key order, double formatting) is NOT a stability contract across Qt
versions — a Qt 7 port could invalidate every historic checksum without a
single corrupted byte. VERDICT: checksum the **exact bytes as written**
(serialize once, CRC the byte string, append). Additionally, for
championship-grade integrity CRC32 detects corruption but not tampering:
add a **per-line chained hash** (each line carries a truncated SHA-256 of
the previous line) — an append-only tamper-evident chain at negligible
cost. Also: store scores/coords as **fixed-point integers** (score×10,
mm×100) in payloads — JSON double round-tripping is the other silent
determinism leak.

**3. Replay engine.** Deterministic enough for regression? It MUST be —
replay *is* the regression tool. V1 implies it; V2 makes it law: reducers
are pure functions, forbidden from reading wall clock, APPSETTINGS, or any
config not embedded in the session header. Golden-master tests replay
fixtures to byte-identical projections.

**4. Event model — under-engineered for ISSF.** V1 has no correction
story. Real championships have IRREGULAR shots, protests, annulments,
cross-shots, jury rescores. In an immutable log these are **compensating
events** — `ShotInvalidated`, `ShotRescored`, `SeriesAdjusted`,
`CrossShotRecorded`, `EquipmentMalfunction` — with reducer rules (totals
recompute; invalidated shots stay in the record flagged, never erased).
SIUS/Meyton have this on day one; so must the schema (even if the UI for
it comes years later).

**5. Recovery workflow.** Two fixes: (a) torn-tail auto-truncation
*destroys evidence* — preserve the original bytes (`.raw` copy) before
truncating; (b) recovery itself can crash — resume must be idempotent:
`RecoveryCompleted` is appended only after successful prime; a crash
mid-resume leaves the session INTERRUPTED and retryable.

**6. Controller integration.** `primeFrom()` creates dual state
(controller internals vs reducer-produced session) — the known drift risk.
The unlimited-time fix (stateless controllers reading the store) is a
rewrite of proven finals code; not now. V2 keeps `primeFrom` but adds an
enforced invariant: after EVERY event in the harness, controller-exported
state must equal reducer state. Drift becomes a test failure, not a field
bug. Named debt, scheduled for a post-Training CQRS milestone.

**7. QML interaction.** Role-locked ListModels + manual routers are
fragile (documented gotchas). The correct end-state is C++
`QAbstractListModel` views over the store. Deliberately deferred (M8+):
recovery does not require it, and touching every delegate now multiplies
risk.

**8. Reports.** Unify inputs: builders consume `Session` (one shape) —
`FinalsReportBuilder` today reads controller-retained copies; after M2 the
store IS that data. Trivial, but stated.

**9. Android — real gap.** On Android, process death is routine (Doze,
low-memory kill), not exceptional; and eMMC fsync can stall 100+ ms.
V2 adds: (a) an application-state hook — on `Suspended`, append a
`Suspended` marker + flush (recovery then treats OS-kill like a clean
interruption); (b) a **latency watchdog** on fsync with a configurable
degrade (fsync→flush) that warns visibly.

**10. RMS.** File-tailing over SMB (the legacy kiosk pattern) is the wrong
transport. V2 introduces one **in-process event bus tap** on the store —
the single integration point for RMS transport, spectator feeds, audio
cues, and analytics. The journal stays the source of truth; the bus is
just distribution. This also answers live spectators: a read-only tap, no
redesign.

**11. Testing.** V1 strong; V2 adds golden-master report determinism and
removes the Qt-serialization dependency from checksums. The
crash-after-every-event sweep stays the centrepiece.

**12. Performance.** Estimates hold. The watchdog converts the one
tail-risk (slow media) from a UI freeze into a policy decision.

**13. Failure scenarios — THE flaw in V1.** V1's write-ahead rule said a
shot that cannot be persisted is *rejected*. That is wrong for a live
match: the athlete has FIRED; refusing to score because the disk is full
turns a storage fault into a stopped championship. Correct invariant in
V2: **never refuse to score a fired shot.** On persistence failure:
accept + score, queue events in a RAM ring, retry continuously, and raise
an unmissable degraded-persistence alarm (HUD banner + dialog). "Never
lose an accepted shot" holds whenever storage works; when storage fails,
the match continues and the operator knows. (This mirrors how ranges
actually run — paper-backup-era instincts.)

**14. Maintainability.** The QVariantMap payloads were the debt magnet —
fixed by typed events. `src/reliability/` becomes a standalone static-lib
target (own .pri, QtCore-only) reusable verbatim by the future RMS server.

**15. API.** `Result submit(Event)` survives; add `persistenceState()`
(Healthy/Degraded/Failed + queue depth), `exportSupportBundle(sid)`, and
keep the signature async-evolvable (Result already decouples callers from
sync/async internals).

## R2. Direct answers

- **Weakest part of V1:** the persistence-failure semantics
  (reject-the-shot) — a correctness inversion; second, the
  CRC-over-canonical-JSON hidden Qt-version dependency.
- **Likely technical debt:** QVariantMap payloads (fixed in V2); the
  `primeFrom` dual-state (contained by invariant tests, scheduled);
  role-locked ListModels (deferred consciously); the `.tch` importer
  (sunset after one release).
- **Over-engineered:** snapshot-every-50-events for 60-shot matches
  (V2: stage-transition + lifecycle triggers only; count-based reserved
  for Training); arguably five corruption classes (kept — they cost
  little and name real states).
- **Under-engineered:** corrections/annulment events; degraded-mode
  policy; Android lifecycle; tamper evidence; torn-tail evidence
  preservation. All fixed in V2.
- **What SIUS/Meyton would do differently:** certified/signed binary logs
  on dedicated hardware; a separate atomically-rewritten state file plus
  shot log (their snapshot/journal *pair* works because they control the
  filesystem); IRREGULAR/correction workflows from day one; stricter
  device separation (target unit scores, display unit displays). On
  commodity tablets, our one-file chained journal is the better trade —
  but we adopt their correction vocabulary and tamper-evidence posture.
- **Unlimited-time redesign:** stateless controllers over the store (pure
  CQRS), C++ model views, generated typed schema, signed journals.
- **Independent libraries:** `techaim-reliability` (envelope/journal/
  replay/validation — QtCore only) — yes, immediately; the analytics
  engine already is one; report builders stay app-side.
- **Too many responsibilities:** V1 SessionStore (split in V2);
  Finals3PController sheds journaling (already planned); its timing could
  split later, not now.
- **Is SessionStore the correct abstraction?** Yes — as a façade over
  Reducer + JournalManager + Notifier, not as a monolith.
- **Is event sourcing right here?** Yes, and not as fashion: ISSF
  corrections REQUIRE compensating events for auditability; replay doubles
  as the regression harness; RMS sync needs idempotent immutable units.
  All three needs point at the same structure.
- **Should any part use SQLite?** Only the derived, rebuildable index
  (M8) for history browsing/RMS bookkeeping. Never the hot path; never
  authoritative.
- **Is CRC32 sufficient?** For accidental corruption, yes. For
  championship integrity, no — hence the chained hash. Both are in V2.
- **Deterministic replay for regression?** Mandatory; enforced by pure
  reducers, fixed-point numbers, config-from-header, golden masters.
- **Can the journal drive match replay, analytics, coaching?** Yes —
  events carry x/y/score/splits/timing; a timed fold gives shot-by-shot
  playback; coach analytics already consumes exactly this shape.
- **Live spectators without redesign?** Yes — the event-bus tap is a
  read-only subscriber; zero changes to authority or storage.

## R3. Version 2 — consolidated deltas over V1

1. **Never-refuse-to-score**: degraded persistence mode (RAM ring queue,
   continuous retry, loud alarm) replaces reject-on-failure. [correctness]
2. **Checksum the written bytes**, not canonical JSON. [portability]
3. **Chained line hashes** — tamper-evident journals. [championships]
4. **Typed event structs** + generic envelope; **fixed-point** scores and
   coordinates in payloads. [maintainability + determinism]
5. **Correction taxonomy** (`ShotInvalidated`/`ShotRescored`/…) with
   reducer rules — schema now, UI later. [ISSF]
6. **Session split**: SessionCore + versioned opaque DisciplineState;
   pure per-discipline reducers, config-from-header only.
7. **Android lifecycle marker** (`Suspended`) + fsync latency watchdog
   with configurable degrade.
8. **Event-bus tap** on the store: one integration point for RMS,
   spectators, audio, analytics.
9. **Torn-tail evidence preserved** (`.raw`) before truncation; resume
   idempotency rule (RecoveryCompleted only after successful prime).
10. Snapshot triggers simplified (stage/lifecycle; count-based only for
    Training); `techaim-reliability` as a standalone static lib;
    `persistenceState()` in the API.

Roadmap impact: M0-M7 order unchanged. M1 grows (typed events, chain,
bus); M2 gains the degraded-mode path + watchdog; M3 gains resume
idempotency; correction events ship as schema+reducers in M1 with no UI.

## R4. V1 vs V2 — why V2 is superior

| Axis | V1 | V2 | Why it matters over 15 years |
|---|---|---|---|
| Failure semantics | persistence failure rejects the shot | match never stops; degraded mode + alarm | turns the worst field failure from "stopped final" into "flagged final" |
| Checksum base | canonical JSON (Qt-version coupled) | exact written bytes | historic journals stay verifiable across Qt upgrades |
| Integrity | corruption detection only | + tamper-evident hash chain | championship/protest credibility |
| Payload typing | QVariantMap | typed structs, fixed-point numbers | compile-time safety; bit-stable replay |
| ISSF operations | no correction path | compensating-event vocabulary | protests/jury decisions without schema breaks |
| Android | path-portable | + lifecycle marker, latency watchdog | survives how Android actually kills apps |
| Integration | RMS "tails files" | one event-bus tap | spectators/RMS/audio without touching authority |
| Store internals | monolith | façade over Reducer/Journal/Notifier | replaceable parts, testable seams |
| Evidence | torn tail truncated | original bytes preserved | audits never ask "what did you delete?" |

V1 was a sound recovery design for the app as it exists. V2 is the same
design made safe for the *business* the app is heading into: it fixes the
one behaviour that could stop a live match, removes the two silent
portability/determinism couplings, and adds the vocabulary (corrections,
tamper evidence, event tap) that championships, Android and the RMS will
demand — all without changing the storage model, the milestone order, or
the offline-first guarantee.
