# TechAim Session Reliability Layer — Implementation Specification

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
| Companion design document | `docs/recovery-system-architecture.md` |
| As-built source | `src/reliability/{core,events,journal,reducer,replay,recovery,storage,store}` |
| Test evidence | `tests/reliability/` (864 checks, 0 failures) |

Original preamble (retained as written):

Baseline: Recovery Architecture **V2** (docs/recovery-system-architecture.md,
review sections R1–R4). This document is the buildable blueprint; where it
is more specific than V2 it **supersedes** V2 wording, noted inline as
`[SUPERSEDES]`. It introduces no third architecture.

Supersessions of earlier statements:
- V1 "reject on persistence failure" — already superseded by V2 R3.1;
  finalized in §9/§10 here.
- V2 "CRC32 + chained SHA-256 (two mechanisms)" — `[SUPERSEDES]` merged into
  **one chained-hash field that also serves as the corruption checksum**
  (§5): QtCore has no CRC32 (only CRC-16 `qChecksum`), a hand-rolled CRC32
  next to an already-required SHA-256 is redundant code to maintain for 15
  years. Corruption detection + tamper evidence both come from the chain.
- V1 Phase-15 milestone file lists — replaced by §28's exact scopes.

---

---

## Design versus as-built status

Verified against the source tree at commit `cc69939`.

### Implemented as designed

| Specification section | As-built evidence |
|---|---|
| §1 project structure — a self-contained reliability module | `src/reliability/` with the sub-modules this document names |
| §2–3 class catalogue and public APIs | `SessionStore`, `JournalWriter/Reader/Validator`, `SessionReducer`, `ReplayEngine`, `RecoveryCoordinator`, `StoragePaths` all exist under those names |
| §4 domain event catalogue | `events/EventTypes.h` — a typed `DomainEvent` variant |
| §5 envelope and file format, **one chained-hash field** | `journal/HashChain.*`; **no CRC32 anywhere in `src/`**, as this document's `[SUPERSEDES]` note required |
| §6 integer numeric representation | shot coordinates in hundredth-mm, scores in tenths |
| §7–8 `SessionState` + reducer fold | `reducer/SessionState.*`, `reducer/SessionReducer.*` |
| §9–10 shot acceptance flow and persistence health model | `SessionStore::submit()` returning a `SubmitResult`; degraded/queue/health handling |
| §11 journal writer threading | `journal/JournalWriter.*` |
| §13 snapshot policy — snapshots as in-journal events | `StateSnapshot` event type |
| §14 storage paths | `storage/StoragePaths.*`, rooted at `AppLocalDataLocation` |
| §15 recovery startup algorithm | `recovery/RecoveryCoordinator.*` + the startup dispatch in `main.cpp` |
| §16 timer recovery policy `[DECISION D1]` | implemented in the recovery path |
| §17 controller integration map | each discipline controller submits typed events |
| §18 QML integration | recovery surfaced through the dialog framework |

### Implemented differently

| Specification said | As built | Why |
|---|---|---|
| A fixed event catalogue sized for the disciplines then in scope | **70 event types** | Grown across Phases A–F for EST incidents, qualification, 10m finals and the Training Lab. The append-at-END rule was honoured so no prior index or hash position moved. |
| §19 reports fed from the reliability layer | Reports are assembled by discipline-specific builders (`FinalsReportBuilder`, the Training report models) reading controller state | The reporting layer grew its own structure; it consumes the same durable record but is not a reliability-layer component. |
| §20 event bus | No general-purpose event bus exists | Qt signals between controller and QML proved sufficient; a separate bus was never needed. |

### Superseded

| Element | Status |
|---|---|
| **"Required design decision — CompetitionEngine: Option B"** | **Superseded — never built.** No `CompetitionEngine` exists. Discipline logic remained in the per-discipline controllers with the reliability layer beneath them. The invariant was reached without it. |
| V1 "reject on persistence failure" | Superseded before implementation, as this document already recorded; the shipped behaviour is the degraded/queue/health model of §9–10. |

### Still pending

| Element | Status |
|---|---|
| **§21 RMS & spectator contract (interface only)** | **Not started.** No spectator or RMS interface exists in `src/`. It remains a separate future product. |
| Android | **Not started.** No Android target. |
| Live-hardware verification of the recovery path | **PHYSICAL TARGET DEPENDENT** — verified by tests, not against a live target mid-match. |


## Required design decision — CompetitionEngine: **Option B**

**B. Existing discipline controllers temporarily perform the engine role
behind a shared interface.** A full `CompetitionEngine` abstraction is
postponed (re-evaluated after Training ships).

Code evidence:
- `Finals3PController` (src/finals/, 178-check harness) already IS a
  competition engine for finals: it validates commands, applies ISSF rules,
  controls legal transitions and owns timing (registerShot validation chain
  cpp:406-455; stage machine enterStage cpp:669+). Wrapping it in a new
  engine now would either rewrite proven code (explicitly forbidden) or add
  a pass-through layer with zero behaviour — a meaningless wrapper.
- Qualification has **no controller class to lift**: its flow lives in QML
  (`ShootingPage.beginPreparationPhase` qml:836+, position derivation
  qml:53-55, timers in CenterPane/RightPanel). To capture events at all,
  M4 must create a thin C++ `QualificationFlow` — that class is the natural
  second implementer of the shared interface, created because event capture
  needs it, not because architecture diagrams want it.
- The shared interface (`ICompetitionSource`, §3) gives the SessionStore a
  single integration contract today and is exactly the seam a future
  CompetitionEngine would slot behind — so postponement costs nothing.

## Section 1 — Target project structure

```
src/
  reliability/            # THE independent layer (QtCore ONLY)
    Reliability.pri       # included by Seta.pro AND every test .pro
    events/               # typed event structs + registry + serializer
    core/                 # SessionState, reducers, Result/error types
    journal/              # writer, reader, validator, hash chain
    replay/               # replay engine (pure)
    recovery/             # RecoveryCoordinator (QtCore part)
    storage/              # StoragePaths, ArchiveService, platform sync
    bus/                  # EventBus (QtCore signals)
  finals/                 # existing; gains ICompetitionSource adapter (M2/M3)
  qualification/          # NEW (M4): QualificationFlow (ICompetitionSource)
  bridge/                 # existing coach/pdf bridges (unchanged)
  analytics/              # existing frozen engine (unchanged)
qml (repo root)           # unchanged location; gains adapters usage only
tests/
  finals/                 # existing harness (extended M2/M3)
  reliability/            # NEW (M1): core harness, same console pattern
  fixtures/               # committed journals: v1 goldens, corrupt, legacy
```

| Directory | Responsibility | Allowed deps | Forbidden deps |
|---|---|---|---|
| reliability/events | event structs, versions, (de)serialization | QtCore | everything else |
| reliability/core | SessionState, reducers, Result | QtCore, events | QML, GUI, finals, hardware |
| reliability/journal | file format, chain, validation | QtCore, events, storage | core reducers (reads envelopes only) |
| reliability/replay | fold events→state | core, events, journal(read) | anything with I/O side effects beyond reading |
| reliability/recovery | scan/classify/coordinate | journal, replay, storage | QML (dialog goes through a signal) |
| reliability/storage | paths, archive, fsync adapter | QtCore | all |
| reliability/bus | pub/sub | QtCore, events | all |
| finals, qualification | domain legality, event production | reliability (events+store API) | journal internals, storage internals |
| qml/adapters (in main tree) | view models, dialogs | reliability public API | journal/storage internals |

Library target: **`techaim_reliability` via `Reliability.pri`** for M1–M7
(the repo is qmake; the proven sharing pattern is exactly how
`tests/finals` already consumes `src/finals`). Graduating to a real static
lib (`TEMPLATE=lib` + SUBDIRS restructure of Seta.pro) is deliberately
deferred — restructuring the monolithic .pro mid-reliability-work is
avoidable risk. `[DECISION D16]`

## Section 2 — Class catalogue

Legend: L = lives in reliability library; Q = QObject.

| Class | ns | Responsibility | Lifetime/Ownership | Thread | Q | L | Forbidden |
|---|---|---|---|---|---|---|---|
| `SessionStore` | ta::rel | façade: submit→persist→reduce→notify; owns live SessionState + health | app singleton, owned by main.cpp | UI thread (M1 model, §11) | Q | L | domain rules, file-format knowledge, rendering |
| `SessionState` (+`QualificationState`,`FinalsState`) | ta::rel | authoritative value type | value; copied for snapshots | — | – | L | behaviour (pure data + invariant helpers) |
| `SessionReducer` | ta::rel | pure fold `apply(state,event)` | stateless (free fns per discipline) | any | – | L | I/O, clocks, config reads |
| `JournalManager` | ta::rel | session-file lifecycle: open/rotate/archive-on-close; owns Writer | owned by store | UI | Q | L | reducing, validating history |
| `JournalWriter` | ta::rel | envelope+hash+append+flush/sync; latency metrics | owned by manager | UI | – | L | interpreting payloads |
| `JournalReader` | ta::rel | stream envelopes from file | stack/temporary | any | – | L | mutation |
| `JournalValidator` | ta::rel | chain/seq/sid/version checks; classification | stack | any | – | L | repair (only classifies; truncation is Coordinator's call) |
| `ReplayEngine` | ta::rel | header→(snapshot)→fold→SessionState | stack | any | – | L | UI, controllers |
| `RecoveryCoordinator` | ta::rel | startup scan, classify, preserve `.raw`, drive resume/archive/discard; idempotency | app singleton | UI | Q | L | dialog rendering (emits signals; QML shows) |
| `StoragePaths` | ta::rel | resolve tree, probe writability, migration scan | static-ish singleton | any | – | L | file content knowledge |
| `ArchiveService` | ta::rel | move closed/corrupt sessions, retention, backups | owned by store | UI | – | L | — (BackupService **merged in** — separate class added no value) |
| `PersistenceHealthMonitor` | ta::rel | health state machine (§10), watchdog, metrics | owned by store | UI | Q | L | writing |
| `PersistenceRetryQueue` | ta::rel | bounded aux / elastic official RAM queue + retry pump | owned by store | UI | – | L | dropping officials (compile-time impossible: officials path has no drop branch) |
| `EventSerializer`/`EventRegistry` | ta::rel | type⇄JSON payload, per-type version table | static tables | any | – | L | envelope concerns |
| `SchemaMigrator` | ta::rel | vN→vN+1 payload upgraders | static | any | – | L | guessing (unknown newer version ⇒ classify, never mutate) |
| `EventBus` | ta::rel | typed pub/sub, replay-flagged delivery | app singleton | UI | Q | L | persistence, ordering changes |
| `ICompetitionSource` | ta::rel | interface: produce events, `primeFrom(state)`, `exportStateForInvariant()` | implemented by controllers | UI | – | L (iface) | — |
| `Finals3PController` (existing, adapted) | — | finals engine; implements ICompetitionSource | as today | UI | Q | – | direct file writes (removed M2) |
| `QualificationFlow` (new, M4) | ta::comp | qualification engine; implements ICompetitionSource | app | UI | Q | – | file writes, rendering |
| `SessionListModelAdapter` | ta::qml | QAbstractListModel over shot records (recovery views; legacy ListModels remain live views until M8+) | QML-exposed | UI | Q | – | mutation of state |
| `SessionProjection` | ta::rel | read-model builders (report inputs) | stack | any | – | L | layout |

Rejected as meaningless wrappers: separate `BackupService` (merged into
ArchiveService), separate `JournalManager` vs `Writer` was kept **only**
because rotation/archival vs byte-emission are genuinely different test
surfaces; `ControllerStateAdapter` merged into `ICompetitionSource`.

## Section 3 — Public APIs (header-style)

```cpp
namespace ta::rel {

// ── results & errors (§24) ────────────────────────────────────────────
enum class Err { None, PathUnavailable, PermissionDenied, DiskFull,
                 SerializeFailed, ChecksumMismatch, ChainBroken,
                 SchemaMismatch, ReducerRejected, IllegalTransition,
                 RecoveryFailed, QueueOverflow, ArchiveFailed };
struct Result {                      // value type, movable, no exceptions
    bool ok = false; Err err = Err::None; QString detail;
    static Result success(); static Result failure(Err, QString detail);
};
struct SubmitResult : Result {       // what the athlete-facing caller needs
    quint64 seq = 0;                 // assigned sequence
    bool persistedDurably = false;   // false ⇒ accepted-but-degraded (§9C)
};

enum class Health { Healthy, Slow, Degraded, Critical, ReadOnly, Failed };

// ── the façade ────────────────────────────────────────────────────────
class SessionStore : public QObject {
    Q_OBJECT
public:
    explicit SessionStore(StoragePaths&, EventBus&, QObject* parent=nullptr);
    // lifecycle — synchronous; UI thread only (asserted)
    Result       beginSession(const SessionHeader& header);      // writes seq 0
    SubmitResult submit(DomainEvent event);                      // by value: sink,
                                                                 // moved into journal+reducer
    Result       closeSession(CloseReason reason);
    // reads — const refs valid until next submit on the same thread
    const SessionState& state() const;
    Health persistenceHealth() const;
    int    unpersistedCount() const;          // retry-queue depth
    // testing / DI hooks
    void setFileInterface(IJournalFile* f);   // fault injection (owned by caller)
    void setClock(IMonotonicClock* c);        // deterministic tests
signals:
    void eventApplied(const ta::rel::DomainEvent& e, bool replayed);
    void stateChanged();
    void persistenceHealthChanged(ta::rel::Health);
    void criticalPersistenceFailure(QString operatorMessage);
};

// ── competition integration ──────────────────────────────────────────
struct ICompetitionSource {
    virtual ~ICompetitionSource() = default;
    virtual void primeFrom(const SessionState&) = 0;             // restore
    virtual QVariantMap exportStateForInvariant() const = 0;     // test-only equality
};

// ── recovery ─────────────────────────────────────────────────────────
struct RecoveryCandidate { QString sessionId, path, athlete, discipline,
                           interruptedAtIso; int officialShots;
                           enum Class { Clean, TornTail, Gapped,
                                        Corrupted, NewerVersion } cls; };
class RecoveryCoordinator : public QObject {
    Q_OBJECT
public:
    QVector<RecoveryCandidate> scan();                 // startup, before QML shows
    Result resume(const QString& sessionId,
                  SessionStore&, ICompetitionSource&); // replay+prime+RecoveryCompleted
    Result archive(const QString& sessionId, bool discarded);
signals:
    void recoveryChoicesReady(QVector<ta::rel::RecoveryCandidate>);
};
} // ns
```

Contracts: **all core APIs synchronous on the UI thread** (M1 model, §11);
no exceptions cross API boundaries (Result everywhere); `DomainEvent` is a
move-friendly value type (std::variant, §4); no QVariantMap in the core API
(it survives only in `exportStateForInvariant`, a test hook, and at the QML
boundary adapters). Ownership: store owns manager/queue/monitor; caller
owns injected test doubles.

## Section 4 — Domain event catalogue

`DomainEvent = std::variant<...all structs below...>` + envelope (§5).
Units per §6. `Dur` = durability class (§12): **S** = fsync, **F** = flush,
**A** = append-only. `Bus→` = forwarded to RMS/spectator adapters.
All events start at payload version `pv:1`. Producer legend:
FC=Finals3PController, QF=QualificationFlow, ST=SessionStore itself,
RC=RecoveryCoordinator, UI=operator action via engine.

**Session lifecycle**
| Event | Fields (type — required unless `?`) | Prod | Reducer effect | Dur | Bus→ |
|---|---|---|---|---|---|
| `SessionStarted` | sessionId(str), schemaVersion(int), appVersion(str), createdAtIso(str), athlete(str), lane?(str), targetId?(str), discipline(enum), matchType(str), disciplineConfig(typed struct per discipline), deviceId?(str) | ST | initialise state | S | yes |
| `SessionSuspended` | reason(enum: AppSuspend/UserHide) | ST | mark lifecycle | S | no |
| `SessionResumedMarker` | — | ST | clear suspended | F | no |
| `MatchCompleted` | totalTenths(i32), officialCount(i16) | FC/QF | lifecycle→Complete; validate totals match reduced state | S | yes |
| `SessionClosed` | reason(enum Clean/Abort/Archive) | ST | lifecycle→Closed | S | yes |
| `CleanShutdown` | — | ST | marker only | S | no |

**Shots** (core payload `ShotCore`: shotNumber(i16, 0=sighter),
withinStage(i16), stageId(i16), seriesIndex(i8), xHundredthMm(i32),
yHundredthMm(i32), scoreTenths(i16), directionCentiDeg(i32),
splitMs(i32), windowId(i16), targetMode(i8), externalId(i64),
simulated(bool))
| Event | Fields | Prod | Reducer | Dur | Bus→ |
|---|---|---|---|---|---|
| `ShotAccepted` | ShotCore | FC/QF | append official record; totals += scoreTenths; nextOfficialShot++ | **S** | yes |
| `SighterAccepted` | ShotCore(shotNumber=0) | FC/QF | append sighter | F (§12 decision) | yes |
| `ShotRejected` | reason(enum), externalId(i64), x?,y? | FC/QF | append incident | A | no |
| `MissingShotRecorded` | expectedNumber(i16), stageId(i16), reason(enum) | FC | append missing; DNS projection | S | yes |

**Timing**
| `TimerStarted` | timerId(enum Prep/Match/Stage1/Window), durationMs(i64) | FC/QF | set deadline | F | yes |
| `TimerPaused` / `TimerResumed` | timerId, atMonoMs(i64) | FC/QF/UI | pause bookkeeping | S | yes |
| `TimerExpired` | timerId | FC/QF | stage consequences via engine events that follow | F | yes |
| `ClockAnchor` | monoMs(i64), wallIso(str) | ST (start/stage/pause/resume/suspend) | append anchor list | F | no |

**Stages & positions**
| `PreparationStarted` / `SightingStarted` / `OfficialMatchStarted` | stageId(i16) | FC/QF | lifecycle/stage | S | yes |
| `StageEntered` | stageId(i16) | FC | stage state | S | yes |
| `StageCompleted`/`StageStatusChanged` | stageId(i16), status(i8) | FC/QF | persist verdicts | S | yes |
| `PositionChanged` | positionIndex(i8) | QF | 3P K/P/S | S | yes |
| `TargetModeChanged` | mode(i8) | FC/QF | mode | F | yes |
| `WindowOpened`/`WindowClosed` | windowId(i16) | FC | duplicate scope | F | no |
| `CommandIssued` | commandType(enum), text(str), seqNo(i16) | FC | command history | F | yes |

**Corrections & jury (schema now, UI later — reducers implemented M1)**
| `ShotInvalidated` | targetSeq(u64 of the ShotAccepted), reason(str), authority(enum Jury/Operator) | UI | record stays, flagged invalid; totals recomputed excluding it | S | yes |
| `ShotRescored` | targetSeq(u64), newScoreTenths(i16), newX?,newY?(i32), reason(str), authority | UI | totals adjusted; original values retained in correction history | S | yes |
| `SeriesAdjusted` | stageId, deltaTenths(i32), reason, authority | UI | subtotal adjustment entry | S | yes |
| `PenaltyIssued`/`WarningIssued` | deltaTenths(i32)/0, rule(str), authority | UI | penalties list; totals | S | yes |
| `CrossShotRecorded` | ShotCore, sourceLane?(str) | UI | flagged non-scoring record | S | yes |
| `EquipmentMalfunctionRecorded` | note(str), allowedTimeMs?(i64) | UI | incident + timing allowance hook | S | yes |

**Recovery & persistence**
| `RecoveryStarted` | fromSeq(u64) | RC | marker | S | no |
| `RecoveryCompleted` | resumedAtSeq(u64), oldAnchor(ClockAnchor), newAnchor(ClockAnchor), truncatedTail(bool) | RC | lifecycle Active; timer rebase | S | yes |
| `PersistenceDegraded`/`PersistenceRestored` | queuedCount(i32) | ST | health history (written when writable again) | S/F | no |
| `AuxEventsDropped` | firstSeq,lastSeq(u64), count(i32) | ST | data-loss marker (§9E) | S | no |
| `StateSnapshot` | full serialized SessionState (typed, §13) | ST | replay shortcut; reducer verifies equality with folded state | S | no |

**Reports/exports/integration**
| `ReportGenerated` | reportType(enum), builderVersion(i16), path?(str) | app | audit only | A | yes |
| `ExportQueued` (future RMS outbox) | kind(enum), refSeq(u64) | ST | reserved | F | no |

UI-only transients (window geometry, HUD toggles, right-panel page) are
**not** events — recovery restores them best-effort from `uiHints` inside
`StateSnapshot` only.

## Section 5 — Envelope and file format

One UTF-8 (no BOM) JSONL file per session; line ending **LF only** on all
platforms; one event per line.

Envelope fields (writer emits in this construction order; **readers never
depend on order** — hashing is over written bytes):
`sv` journal schema version (int, =1) · `pv` payload version (int) ·
`sid` session UUID (str) · `lane` (str, may be "") · `seq` (u64, 0 = header)
· `tw` wall ISO-8601 with ms (str) · `tm` monotonic ms since session start
(i64) · `t` event type (str) · `p` payload (object) · `av` app version
(str, header only) · `dev` device id (str?, header only) · `ph` previous
hash (32 hex) · `h` current hash (32 hex).

**Hashing procedure `[SUPERSEDES V2 dual CRC32+SHA]`:**
1. Writer serializes the envelope **without `h`** via
   `QJsonDocument::toJson(Compact)` → `line_core` (bytes as produced by the
   running Qt — key order irrelevant because verification uses stored bytes).
2. `h = hex( SHA-256( ph_utf8 + line_core ) )[0..31]` (Qt
   `QCryptographicHash::Sha256`; truncation to 128 bits).
3. Written line = `line_core` with final `}` replaced by
   `,"h":"<h>"}` + `\n`. (Byte splice — no re-serialization.)
4. Genesis: `ph` of seq 0 = 32×`"0"`.
Verification: locate the trailing `,"h":"…"}` by suffix scan, recover
`line_core` bytes exactly as stored, recompute step 2, compare; then check
`ph(line n) == h(line n-1)`. This detects corruption (recompute mismatch)
AND tampering (chain break), is Qt-version-proof (verifies stored bytes),
and Windows/Android-identical (UTF-8+LF fixed above). After app upgrades,
old journals validate unchanged because no canonicalization is involved.

Examples (illustrative spacing; real lines are compact):
```json
{"sv":1,"pv":1,"sid":"6f1c…","lane":"","seq":0,"tw":"2026-07-17T21:02:11.412","tm":0,"t":"SessionStarted","p":{"athlete":"Arnold Bailie","discipline":"FINAL3P","matchType":"FINAL 35","config":{"kneelingShots":10,"proneShots":10,"seriesShots":5,"singleShots":1,"stage1Ms":1320000}},"av":"4.0.0","ph":"00000000000000000000000000000000","h":"3fa4b0c2…"}
{"sv":1,"pv":1,"sid":"6f1c…","seq":41,"tw":"2026-07-17T21:19:03.008","tm":1011596,"t":"ShotAccepted","p":{"shotNumber":14,"withinStage":4,"stageId":5,"seriesIndex":1,"xHundredthMm":142,"yHundredthMm":-510,"scoreTenths":103,"directionCentiDeg":28734,"splitMs":9400,"windowId":4,"targetMode":1,"externalId":214,"simulated":false},"ph":"9be2…","h":"77aa…"}
{"sv":1,"pv":1,"sid":"6f1c…","seq":97,"tw":"2026-07-17T21:44:51.550","tm":2560142,"t":"ShotRescored","p":{"targetSeq":41,"newScoreTenths":104,"reason":"Jury decision 7.11.2","authority":1},"ph":"c1d0…","h":"08e3…"}
```

## Section 6 — Numeric representation

| Quantity | Persisted type | Unit | Conversion boundary | Rounding |
|---|---|---|---|---|
| decimal score | `scoreTenths` i16 | 0.1 ring | scoring engine output → event creation (once) | half away from zero |
| coordinates | `xHundredthMm`,`yHundredthMm` i32 | 0.01 mm | same | half away from zero |
| direction | `directionCentiDeg` i32 | 0.01° | same | same |
| splits/durations | `splitMs`, `durationMs` i32/i64 | ms | controller clocks already ms | truncation (ms native) |
| totals | i32 tenths (sum of scoreTenths) | 0.1 | reducer arithmetic only (integer) | exact |

Floating point remains permitted for: display formatting (QML divides by
10/100), charts, coach analytics inputs, live overlay math. It is
**forbidden** as an authoritative persisted value or inside reducers.
Rule applied identically in live scoring→event, persistence, replay,
reports (builders consume tenths and format), RMS payloads.

## Section 7 — SessionState

Discipline-specific state: **`std::variant<QualificationState,
FinalsState /*, TrainingState later*/>`** — typed, exhaustively switched,
serialized per-alternative with its own version int. Rejected: untyped map
(the debt V2 killed), inheritance (slicing/clone ceremony), templates
(no compile-time discipline set at call sites), opaque blob (blocks
reducer validation).

```cpp
struct ShotRecord { /* ShotCore fields, §4 */ bool invalidated=false;
                    qint16 rescoredTenths=-1; };
struct CorrectionEntry { quint64 targetSeq; QString type, reason; qint16 fromTenths, toTenths; };
struct ClockAnchorRec { qint64 monoMs; QString wallIso; QString cause; };

struct SessionCore {
  // identity
  QString sessionId; int schemaVersion; QString appVersion, createdAtIso;
  // configuration + who/where
  Discipline discipline; QString matchType, athlete, lane, targetId, deviceId;
  DisciplineConfig config;                       // typed per discipline
  // lifecycle
  enum class Life { Active, Suspended, Interrupted, Complete, Closed } life;
  // records
  QVector<ShotRecord> officials, sighters;
  QVector<QVariant-free typed structs> missing, incidents, penalties, warnings;
  QVector<CorrectionEntry> corrections;
  // totals (integer, reducer-owned — no other component computes them)
  qint32 totalTenths; QMap<int,qint32> stageSubtotalTenths; QMap<int,int> stageStatuses;
  // timing
  QVector<ClockAnchorRec> anchors; bool paused; qint64 pausedAccumMs;
  qint64 lastEventMonoMs;
  // reliability metadata
  quint64 lastSeq; Health lastKnownHealth; QString recoveredFromSeq;
  QVariantMap uiHints;                           // ONLY inside snapshots; non-authoritative
};
struct QualificationState { int positionIndex; bool sighterMode; int p3BreaksDone;
                            qint64 matchDeadlineMonoMs; int version=1; };
struct FinalsState { int stageId, targetMode, windowId; qint64 lastExternalId;
                     int shotsInStage; qint64 stageStartMono, segmentEndMono;
                     int seqStep; int version=1; };
struct SessionState { SessionCore core;
                      std::variant<QualificationState,FinalsState> disc; };
```
Concrete shapes: **50m Prone Q** = QualificationState{positionIndex:-1,…},
config{shots:60,seriesSize:10,matchMs:...}; **3P Q** = positionIndex 0..2 +
p3BreaksDone, config{shots:60, perPosition:20}; **Finals** = FinalsState
mirroring today's controller fields 1:1 (Finals3PController.h:209-241) so
`primeFrom` is a direct assignment.

## Section 8 — Reducer design

```cpp
// pure, deterministic, side-effect free; no clock, no config reads
ReduceResult apply(const SessionState& s, const DomainEvent& e);
struct ReduceResult { SessionState next; Result status; };
```
Order of checks per event: (1) **envelope validation** already done by
journal layer (seq/sid/chain) — reducers assume a valid envelope;
(2) **pre-reduction event validation** (fields present, fixed-point ranges);
(3) **transition legality** (event legal in `s.core.life` + discipline
state — tables shared conceptually with controllers; on replay a violation
⇒ `Err::IllegalTransition` ⇒ classification Corrupted at that seq);
(4) apply; (5) **post-reduction invariants** (totals == Σ valid shots;
counts monotone). Unknown event with `pv` ≤ known: apply via
`SchemaMigrator` upgrade then reduce. Unknown **type** or newer `pv`:
replay stops → NewerVersion classification (never guess). Duplicate `seq`:
journal layer drops before reducers see it. Corrections: reducer marks
records (never deletes), appends CorrectionEntry, recomputes integer
totals. Relationship: controllers/engine **decide** (live legality &
timing), reducers **record** (deterministic state), SessionStore
**sequences** them; totals exist only in reducer output — controllers
read them back from `state()` (M2+ finals `m_cumulativeTotal` becomes a
mirror asserted equal, then removed in M3).

## Section 9 — Shot acceptance flow

**A. Normal official shot** (order is the contract):
```
hardware/demo → MODREADER count → CenterPane scoring (unchanged)
 → controller duplicate+legality validation (unchanged)
 → build typed ShotAccepted (fixed-point conversion HERE)
 → SessionStore.submit:
      seq assign → serialize once → hash chain → append → flush → fsync   [≤10ms typ.]
      → reducer apply → state swap
      → eventApplied(replayed=false) on EventBus
 → view routers append models → HUD/panel/face
 → audio (bus subscriber) → RMS/spectator adapters (bus, persisted events only)
 → athlete sees the shot                      ← disk already has it
```
**B. Sighter**: identical, durability F (flush, no fsync).
**C. Persistence write failure** (the V2 rule, exact):
```
append/flush/fsync FAILS
 → event goes to PersistenceRetryQueue (officials: elastic; aux: bounded)
 → reducer applies anyway  ⇒ SHOT IS SCORED, marked persistedDurably=false
 → HealthMonitor → Degraded (§10) → critical alarm banner + dialog
 → bus publishes to VIEWS; RMS adapter withholds unpersisted events
```
**D. Retry success**: pump (timer, 250 ms backoff→5 s) re-appends queued
events **in seq order** to the journal (their original seq/hash chain is
recomputed at write time — chain reflects write order, `seq` preserves
logical order; validator accepts monotone `seq` with gaps *only* between a
`PersistenceDegraded`/`Restored` pair) → on drain: `PersistenceRestored`
written, health → Recovering → Healthy, banner clears, RMS adapter flushes.
**E. Queue capacity**: aux events beyond 4096: oldest aux dropped and
counted; on restore an `AuxEventsDropped` marker is journalled — **loss is
recorded, never silent**. Officials are never dropped (elastic, ~500 B
each; a full day ≈ <1 MB RAM). If even RAM allocation fails the app is
already dying; the store attempts a last-ditch emergency append of
officials to `%TEMP%/techaim_emergency_<sid>.jsonl` before abort.

## Section 10 — Persistence health model

| State | Entry | Exit | Match continues | UI | RMS | Shutdown |
|---|---|---|---|---|---|---|
| Healthy | default; last N writes ok & fast | — | yes | none | live | normal |
| Slow | p95 fsync > 50 ms (watchdog) | p95 back < 25 ms | yes | subtle amber dot in HUD strip | live | normal |
| Degraded | any write failure; queue non-empty | queue drained | yes | **persistent red banner** "PERSISTENCE DEGRADED — data buffered in memory (N events)" + one modal ack | withheld | flush attempt + emergency file |
| Critical | queue age > 5 min OR aux drops occurred | drain | yes | banner + repeating dialog every 5 min + audio alert cue | withheld | same |
| Recovering | retry pump draining | drained/failure | yes | banner turns amber "restoring…" | withheld | — |
| ReadOnly | startup probe failed | operator fixes path | **new sessions blocked** behind explicit override "Continue WITHOUT persistence" (dialog, typed confirmation) | blocking dialog | n/a | n/a |
| Failed | emergency path also failed | restart | yes (RAM only) | full-screen-edge red frame + dialog | withheld | best-effort |

Operator warning design: a 28 px banner strip docked above the bottom
action bar (never over the target rings), TechAim red `#d0392b`, white
text, click → detail dialog (queue depth, last error, retry countdown).
Health is also a `persistenceState` chip in the finals HUD top strip.

## Section 11 — Journal writer threading

Evaluated: (1) sync-on-UI ✅ chosen for M1–M6; (2) dedicated thread;
(3) async acknowledged; (4) hybrid. Rationale: event rate is human-speed
(one official shot per several seconds); fsync on SSD/eMMC is 1–10 ms
typical; the degraded path (§9C) already makes the API tolerant of
slowness because callers never *wait* on retries. Antivirus/network-folder
stalls are covered by the watchdog → Slow → operator-visible, and by the
graduation criterion. **Graduation trigger** `[DECISION D7]`: measured
p99 fsync > 100 ms on target hardware OR Android field telemetry shows
UI-frame drops attributable to writes ⇒ implement model (2) — a dedicated
writer thread with an acknowledged bounded queue — behind the SAME
`submit()` API (SubmitResult.persistedDurably then reports queue-ack).
Instrumentation from M1: per-write append/flush/fsync µs, rolling p50/p95/
p99, queue depth, retry count, `lastDurableIso` — exposed via
`PersistenceHealthMonitor` and dumped into support bundles.

## Section 12 — Durability table

| Category | append | flush | fsync | snapshot trigger | backup trigger |
|---|---|---|---|---|---|
| Official shots, MissingShot, corrections, penalties | ✓ | ✓ | **✓** | – | – |
| Stage/position transitions, timer pause/resume, lifecycle (Start/Complete/Close/Suspend/CleanShutdown), RecoveryCompleted, StateSnapshot | ✓ | ✓ | ✓ | stage/lifecycle events also *emit* a snapshot per §13 | Close/Complete → archive copy |
| Sighters, TargetMode, Window, Command, TimerStarted/Expired, ClockAnchor | ✓ | ✓ | – | – | – |
| ShotRejected, diagnostics, ReportGenerated | ✓ | – | – | – | – |

**Sighter decision `[DECISION D2]`: flush-not-fsync.** Justification for
professional use: sighters carry no result authority (ISSF results are
official shots only); the exposure window is *only* hard power loss in the
1–2 s before the OS flushes, losing at most the final sighter — which the
athlete re-fires. Giving sighters fsync would double sync traffic in
sighting periods (rapid strings) for zero result-integrity gain. Finals
sighting counts (displayed diagnostics) tolerate ±1 after catastrophic
power loss; the recovery dialog states it.

## Section 13 — Snapshot policy

`StateSnapshot` payload = full typed `SessionState` serialization
(per-alternative discipline version ints included), `pv:1`, plus
`foldedSeq` (u64: the seq this snapshot equals after folding).
Triggers `[DECISION D10]`: `StageEntered`/`PositionChanged`,
`MatchCompleted`, `SessionSuspended`, `CleanShutdown`; **plus** every 500
events for TRAINING discipline only. NOT per-50-events for match modes
(V2 R3.10). Uncompressed (typ. 8–40 KB; max budget 256 KB — exceeding is a
validator warning). Participates fully in the hash chain like any line.
Replay: scan file backwards for last `"t":"StateSnapshot"` with valid
hash; deserialize; verify `foldedSeq` continuity; fold the tail. Validation
extra: during full-file validation (tests/support), fold-from-zero must
equal every snapshot (drift detector). Training rotation: segment
`session_<id>.part<N>.jsonl` when a segment exceeds 5 000 events or 2 MB;
new segment begins with a copied header (same sid, `seq` continues,
`ph` continues across the boundary) + fresh snapshot.

## Section 14 — Storage paths

Root: `QStandardPaths::writableLocation(AppDataLocation)`
(org "TechAim", app "TechAim" set in main.cpp at M0).
Windows: `C:/Users/<u>/AppData/Local/TechAim/TechAim/`;
Android: `/data/user/0/<pkg>/files/` (app-private; scoped-storage safe,
no permissions).

```
<root>/
  sessions/current/    session_<utcYYYYMMDDTHHMMSS>_<uuid8>.jsonl (+ .partN)
  sessions/archive/<yyyy>/<MM>/<same name>.jsonl
  sessions/corrupted/  <name>.jsonl + <name>.raw (untouched original bytes)
  backups/             rolling copy of archive (ArchiveService, retention 90d default)
  reports/             app-kept copies of generated PDFs (optional feature flag)
  exports/             RMS outbox (reserved)
  logs/                relocated diagnostics (M7)
  support/             support bundles (zip: journals+metrics+log tail)
  index/               derived SQLite (M8+, rebuildable, deletable)
  settings/            reserved for config migration (config.ini untouched for now)
  VERSION              layout version = 1
```
Naming: UTC timestamp prefix (sortable) + 8-char uuid suffix (collision-
free); lane id lives in the header, not the filename (single-lane app).
Collisions: uuid regeneration on the astronomically-unlikely clash.
Permissions: M0 startup probe writes+fsyncs+deletes a probe file; failure
⇒ ReadOnly health + blocking dialog (§10). Read-only *installation* is
irrelevant by construction (we never write next to the exe). Migration:
M0 scans CWD for `finals_session*.jsonl` (→ `sessions/archive/legacy/`)
and `Match_*.tch` (left in place; importer in M4 §27).

## Section 15 — Recovery startup algorithm

```
main(): StoragePaths.init+probe
 → RecoveryCoordinator.scan(sessions/current/):
     for each file: read header → JournalValidator.run():
        per line: locate ",\"h\":" suffix → recompute hash → chain check
                  → seq check (+ degraded-gap allowance §9D) → pv check
     classify: Clean | TornTail | Gapped | Corrupted | NewerVersion
     Complete-but-unarchived (has MatchCompleted+CleanShutdown, no close):
        → auto-archive silently, not offered as resume
 → if candidates: QML recovery dialog (dialogManager) BEFORE LoginPage:
     newest first; [Resume] [Archive] [Discard] (+ "Archive all others")
 → Resume(sid):
     1. if TornTail: copy file → sessions/corrupted/<name>.raw is NOT used;
        instead copy → <name>.tornbak alongside, truncate tail in place
     2. append RecoveryStarted (fsync)
     3. ReplayEngine: last valid snapshot → fold tail → SessionState
        (SchemaMigrator upgrades old pv payloads during fold)
     4. store.adoptState(state)  → views refilled via replayed eventApplied
        bulk OR direct model refill adapter (implementation detail of M3/M5)
     5. controller.primeFrom(state)   (Finals M3; Qualification M5)
     6. timer rebase per §16; append ClockAnchor + RecoveryCompleted (fsync)
     7. navigate: LoginPage skipped; ShootingPage restored; uiHints applied
 → crash DURING recovery: RecoveryCompleted absent ⇒ session still
   INTERRUPTED; RecoveryStarted lines are idempotent no-ops on re-fold ⇒
   simply runs again. (Idempotency rule.)
Corrupted: move both original (.raw, byte-exact) and a note into
sessions/corrupted/; offer nothing except support-bundle export.
Gapped: resume offered "up to shot N (of M) — later data unreadable".
NewerVersion: read-only card — [Archive] [Support bundle], never resume.
Multiple unfinished: list dialog; Training may legitimately hold several.
```

## Section 16 — Timer recovery policy `[DECISION D1]`

**Default: interruption time does not count** — remaining time restored to
its value at the last journalled event. Per mode:
- Qualification match clock: remaining = duration − (elapsed at last event,
  from `tm` of the last event + TimerStarted anchor − Σ pauses). Restart
  freezes the clock across the outage. ISSF practice for range failures is
  jury-controlled time compensation — this default is the conservative
  baseline the jury can adjust (SeriesAdjusted/allowance events later).
- Finals command windows: the interrupted window reopens with its
  remaining duration (computed from WindowOpened `tm` + duration − last
  event `tm`); command sequence resumes at the same step.
- Preparation/sighting: same freeze rule (athlete lost nothing).
- Jury pause: explicit TimerPaused/Resumed events — bookkept exactly, not
  affected by this policy.
- Training: freeze (obvious choice; no jury).
- Full power failure vs 2 s restart: indistinguishable by design and
  treated identically — the wall-clock gap between last `tw` and recovery
  is DISPLAYED in the resume dialog so the operator/jury can decide on
  manual adjustments.
Determinism inputs: `TimerStarted`(durationMs) + every event's `tm` +
`ClockAnchor` pairs + pause events. Neither clock alone: monotonic (`tm`)
orders in-session; wall (`tw`) only informs humans and cross-session
display; the new run writes a fresh ClockAnchor in RecoveryCompleted
mapping old-tm → new-monotonic base.

## Section 17 — Controller integration map

| Component | Remains | Changes | Becomes view | Legacy/removed |
|---|---|---|---|---|
| `Finals3PController` | all legality/timing logic; signals | M2: `writeJournal`→`store.submit(typed)`; totals mirrored then read from state (M3); implements ICompetitionSource.primeFrom (direct FinalsState assignment, §7) | — | private journal code deleted M2 |
| Qualification flow (QML) | UX flow | M4: thin C++ `QualificationFlow` created; QML calls it; it emits typed events | — | scattered QML bookkeeping shrinks |
| `tachuswidget` backend lists | live hardware buffering | untouched | never authoritative (already established) | `.tch` feed severed M4 |
| MODREADER signals | unchanged | — | — | — |
| QML timers | display | M5: authoritative deadlines come from state; QML timers render remaining | yes | — |
| `globalMatchModel` etc. | M2–M4 dual (view+legacy authority) | refilled from state on recovery | **yes (end-state)** | authority label removed M5 |
| right-panel totals | display recompute | reads reducer totals via adapter (M5) | yes | duplicate arithmetic removed M5 |
| Finals report state (`m_officialShotRecords`…) | M2 | M3: builder consumes SessionState projection | — | controller-retained copies removed M3 |

Restoration interface: `ICompetitionSource::primeFrom(const SessionState&)`.
Dual-state invariant (temporary, M2–M5): after EVERY harness event,
`exportStateForInvariant() == reducerStateProjection()` — a failing test,
never a field bug (V2 R1.6).

## Section 18 — QML integration

- `SessionStoreQml` thin QObject exposing: `stateVersion` (bump counter),
  `totalTenths`, `persistenceHealth`, `unpersistedCount`, invokable getters.
- Recovery dialog + persistence banner: driven by RecoveryCoordinator /
  health signals through the existing `dialogManager` + a new
  `PersistenceBanner.qml` (view-only).
- Shot lists: existing role-locked ListModels REMAIN as live views (their
  routers now subscribe to `eventApplied`); on recovery they are refilled
  by iterating `state().officials/sighters` through the same router code
  path (no second fill logic). `SessionListModelAdapter`
  (QAbstractListModel) is introduced only where new UI needs it (session
  browser M6); wholesale ListModel replacement is explicitly out of scope
  until M8+.
- Timer displays bind to controller properties as today; controllers are
  primed, so QML changes are minimal.
- QML never touches files: enforced by grep-gate test in CI (`QFile` in
  .qml ⇒ fail, exception list empty).

## Section 19 — Reports

All builders take a `SessionProjection` built from `SessionState`:
- **Finals Report**: `FinalsReportBuilder` input assembled from state
  (records incl. corrections applied); builder logic unchanged (tenths→
  formatting adjusts in the adapter).
- **Summary/Match Reports**: M6 adds a projection that fills the exact
  model shapes these QML views read today (same role names) — views
  unchanged, source swapped.
- **Coach Report**: `feedCoachReport` reads the projection instead of
  `globalMatchModel` (same list shape).
- **Training Report** (future): new builder over the same projection API.
Reproducibility guarantee: every report = f(journal) — golden tests render
report-input JSON from fixture journals byte-identically. `ReportGenerated`
events carry `builderVersion`; regenerating an old session uses the CURRENT
builder (report layout is presentation, not evidence — the journal is the
evidence). Schema note stored in the PDF footer: sessionId + builderVersion.

## Section 20 — Event bus

Publisher: **only SessionStore** (post-apply). Subscribers: view routers,
audio (FinalsAudioService moves here, M7 optional), HUD, report cache
invalidation, coach module, RMS adapter, spectator adapter, diagnostics.
Delivery: synchronous Qt direct connections on the UI thread — ordering =
submission order, guaranteed. Failure isolation: subscribers are slots;
a misbehaving subscriber is a bug caught in tests, not a runtime firewall
(no exceptions in Qt slots; watchdog on slot duration in debug builds).
Replay: `eventApplied(e, replayed=true)` — views consume; audio, RMS,
spectator adapters ignore replayed events. **Persistence precedes
publication** for durably-persisted events; in Degraded mode events are
published to *views* immediately (the lane state DOES contain them) but
the RMS/spectator adapters subscribe through a `DurableTap` that releases
an event only when its journal write is confirmed — an external system can
never hold a shot the lane's journal doesn't.

## Section 21 — RMS & spectator contract (interface only)

Sync unit = the journal envelope verbatim: `{sid, lane, seq, t, pv, p, h}`
+ transport ack `{sid, seq}`. Ingestion idempotent by `(sid,seq)`; `h`
lets the server verify chain integrity per lane. Reconnect: server sends
`lastSeq(sid)`, lane replays from there out of the journal file (offline
queue = the journal itself; no second outbox for events — `exports/` is
reserved for non-event artifacts). Duplicate/out-of-order delivery:
server buffers by seq, applies in order, acks high-water mark. Authority:
RMS is read-only over match state; jury actions flow to the lane operator
who journals correction events locally; RMS sees them like any event.
Completion: `MatchCompleted`+`SessionClosed` events signal finality;
championship mode may later add a journal-final signature (§23).
Spectators = the same DurableTap feed, read-only, no acks.

## Section 22 — Android lifecycle

Hook: `QGuiApplication::applicationStateChanged` handled in main.cpp →
SessionStore. On `Suspended/Hidden`: append `SessionSuspended` +
`ClockAnchor` + `StateSnapshot`, fsync (one batch). On resume: append
`SessionResumedMarker`. Process killed after that ⇒ recovery treats it as
a clean interruption (snapshot is the last line). Screen lock = Hidden ⇒
same path. Device reboot / low-battery shutdown ⇒ the suspend snapshot
usually landed (Android delivers state changes before kill in the normal
path); if not, standard torn-tail recovery applies. Storage temporarily
unavailable ⇒ ordinary Degraded flow (§10). Scoped storage: app-private
`AppDataLocation` needs no permissions; user exports use Qt's SAF-backed
dialogs. `IPlatformSync`: `FlushFileBuffers(HANDLE)` (Win) /
`fsync(fileno)` (POSIX/Android) — the only platform-conditional code.

## Section 23 — Security & auditability (honest scope)

Provided: corruption detection + tamper EVIDENCE (chained truncated
SHA-256 — an editor must recompute the whole suffix chain; casual result
tampering becomes detectable and laborious), byte-exact preservation of
corrupt/torn originals, session UUID + device id + app version in every
header, correction events as an immutable audit trail.
NOT provided (stated plainly): tamper *proof* — an attacker with file
access and knowledge of the format CAN recompute the chain. No secrecy
(journals are plaintext). No non-repudiation.
Reserved for championship mode (architecture-ready, not implemented):
`sig` field slot in the envelope of `SessionClosed` (device-held key
signing the final chain hash), RMS receipt counter-signature. The chain
design makes this a one-field addition — no format break. `[DECISION D15]`

## Section 24 — Error model

`Err` enum + `Result` (§3). Severity/behaviour table:

| Err | Severity | Operator message | Recoverable | Caller behaviour |
|---|---|---|---|---|
| PathUnavailable / PermissionDenied | Critical(startup) / Degraded(runtime) | "Storage location unavailable…" | operator action | startup: ReadOnly gate; runtime: queue+retry |
| DiskFull | Degraded→Critical | "Storage full — buffering in memory" | free space | queue+retry |
| SerializeFailed | Bug-severity | "Internal error (event S/N)" | no | reject event to caller; assert in debug |
| ChecksumMismatch / ChainBroken | per-position (§15) | recovery dialog wording | tail: yes; interior: no | classify; never repair silently |
| SchemaMismatch | Info(older)/Blocker(newer) | "Session from a newer version" | migrate / read-only | migrator / NewerVersion card |
| ReducerRejected / IllegalTransition | replay: Corrupted; live: bug | — | — | live submit returns failure; harness catches |
| RecoveryFailed | High | "Could not restore — session archived" | archive path | archive + support bundle offer |
| QueueOverflow (aux) | Critical | banner count | drains | AuxEventsDropped marker |
| ArchiveFailed | Medium | toast + log | retry next start | non-blocking |

## Section 25 — Test strategy (per milestone gates in §28)

Console harnesses (finals-harness pattern), all CI-able:
1. **Unit**: serializer round-trip per event type; hash procedure vectors
   (fixed inputs → fixed h, committed); reducer table tests per event;
   fixed-point conversion/rounding vectors.
2. **Golden replay**: committed fixture journals per discipline → replayed
   SessionState serialized → byte-compare to committed golden; report
   projections likewise.
3. **Property-style**: random event scripts (seeded) → journal → replay ==
   live fold; corrections interleaved.
4. **Crash sweeps**: (a) kill-after-every-event: prefix replay equals
   expected prefix state, no official lost beyond durability contract;
   (b) power-loss byte sweep: truncate final line at EVERY byte offset —
   classification ∈ {Clean, TornTail}, never interior corruption, resume
   succeeds, exactly ≤1 non-durable event lost.
5. **Fault injection** (`IJournalFile` double): disk-full, EACCES, short
   write, flush fail, fsync stall(500 ms) → health transitions match §10,
   queue drains in order, `AuxEventsDropped` correctness, officials never
   dropped (assert queue introspection).
6. **Corruption fuzz**: single-bit flips per line; line deletion; line
   duplication; reorder — validator classifications exact.
7. **1000-cycle**: loop{random events, kill, recover, resume} with
   invariants each cycle; wall-time budget < 60 s suite.
8. **Cross-version**: v1 fixtures replayed by future code (regression);
   synthetic pv=99 → NewerVersion; migrated-payload goldens.
9. **Android lifecycle** (desktop-simulated): suspend-marker path, resume,
   kill-after-suspend recovery classified Clean.
10. **Manual competition smoke** (per milestone checklist §28): live
   power-cut mid-string; restart mid-final S1; restart during 3P position
   change; report regeneration next day.
Pass criteria are exact: zero lost fsync-class events in all sweeps; ≤1
flush-class event lost only under power-loss simulation; all goldens
byte-identical; suite runtimes within budget.

## Section 26 — Performance budgets (targets, to be measured in M1/M2)

| Metric | Target | Ceiling |
|---|---|---|
| submit() incl. fsync (official) | < 10 ms typical | 25 ms p99 (watchdog at 50) |
| append+flush (aux) | < 1 ms | 5 ms |
| replay 100 events | < 5 ms | 20 ms |
| replay 10 000 Training events (with snapshot) | < 50 ms | 250 ms |
| startup scan (≤ 5 files) | < 50 ms | 200 ms |
| snapshot size | 8–40 KB typ. | 256 KB warn |
| RAM retry queue | aux cap 4096 (~2 MB); officials elastic | — |
| support bundle | < 5 MB | 20 MB |

## Section 27 — Migration plan

| Legacy artifact | Policy |
|---|---|
| CWD `finals_session*.jsonl` | **archived** verbatim to `sessions/archive/legacy/` at M0 first run (no envelope retrofit — they remain audit history, readable by a v0 tolerant reader in the session browser, never resumable) |
| `Match_*.tch` | **read-only import** (M4): x/y/time → synthetic journal flagged `imported:true`, scores recomputed live as `uploadGame` does today; per-shot autosave writer **removed** in M4; `uploadGame` UI kept one release, then deprecated |
| `globalMatchModel` authority | dual-state M2–M4 with invariant tests; authority formally transferred M5 (docs + assertions flipped) |
| tachuswidget mode-swapped lists | untouched (hardware buffer); never a persistence source; kiosk writers marked deprecated, retired with RMS adapter (post-M8) |
| CWD storage in general | M0: all NEW writes to AppData; one-time legacy scan; CWD never written again by reliability code |
| report inputs | swapped per §19 in M3 (finals) / M6 (qualification) |
| direct file writers (`writeJournal`, `.tch`) | deleted in M2 / M4 respectively |

No attempt to synthesize data legacy files never contained (positions/
scores absent from `.tch` stay absent; imports are marked partial).

## Section 28 — Milestones (exact scopes)

**M0 — StoragePaths + surfaced storage errors**
Goal: no reliability data ever CWD-relative; failures visible.
Files added: `src/reliability/storage/StoragePaths.{h,cpp}`,
`PlatformSync.{h,cpp}`; `Reliability.pri`; `tests/reliability/` skeleton.
Files modified: `main.cpp` (org/app names, probe call, ReadOnly gate
dialog), `Finals3PController.cpp` (journal path from StoragePaths + error
signal — behaviourally identical otherwise), `Seta.pro`(+pri).
APIs: `StoragePaths::root()/dir(Kind)/probe()`. Tests: path resolution,
probe failure simulation, legacy scan. Acceptance: journal lands in
AppData; read-only root → blocking dialog; harness green.
Manual: run once, find files under AppData; Program-Files-style read-only
simulation. Non-goals: envelopes, events, recovery.
Commit: `feat(reliability): AppData storage paths with surfaced errors (M0)`

**M1 — Reliability core**
Goal: the library exists and is proven without touching app behaviour.
Files added: events/ (typed events, serializer, registry, migrator),
core/ (SessionState, reducers incl. corrections, Result), journal/
(writer+reader+validator+chain), replay/, bus/, `tests/reliability/*`
(harness + fixtures + hash vectors + sweeps 25.1–25.8 subset).
Files modified: none in app. Acceptance: full M1 test list green;
goldens committed; budgets measured & recorded in the doc.
Commit: `feat(reliability): core event/journal/replay engine with chained-hash journal (M1)`

**M2 — Finals write path on the SRL**
Goal: finals persists through SessionStore; behaviour identical; degraded
mode live. Files modified: `Finals3PController.{h,cpp}` (submit typed
events; delete writeJournal/archiveExistingJournal; totals mirrored +
invariant), `main.cpp` (store wiring), `ShootingPage.qml` (banner hookup),
new `PersistenceBanner.qml`. Tests: finals harness ported to assert
journal-envelope output; §25.5 fault injection through the live path;
invariant equality after every event. Acceptance: 178-check parity + new
checks; measured submit latency in budget. Manual: full demo final;
yank-the-folder degraded drill.
Commit: `feat(finals): route finals persistence through the reliability layer (M2)`

**M3 — Finals recovery**
Goal: restart mid-final → Resume works. Files added:
recovery/RecoveryCoordinator, `RecoveryDialog.qml` glue via dialogManager;
Files modified: Finals3PController (`primeFrom`), main.cpp (startup scan),
FinalsReportView adapter (state projection input). Tests: kill-after-
every-event (finals script), byte sweep, resume idempotency (crash during
recovery), timer-rebase determinism. Acceptance: resume lands on correct
stage/window/remaining-time in harness; report of a recovered session ==
golden. Manual: restart during S1 window; complete + report next launch.
Commit: `feat(finals): crash recovery with resume/archive/discard (M3)`

**M4 — Qualification event capture**
Goal: authoritative qualification events; `.tch` autosave retired.
Files added: `src/qualification/QualificationFlow.{h,cpp}` (+registration),
`.tch` importer. Files modified: ShootingPage/CenterPane/RightPanel (calls
into QualificationFlow at existing decision points), CenterPane.qml:333
autosave removed, appsettings saveMatch demoted to importer support.
Tests: event capture scripts per discipline (AR/AP/Prone/3PQ), reducer
goldens, importer fixture. Acceptance: full 60-shot 3P demo produces a
valid journal; no `.tch` written. Manual: all four disciplines smoke.
Commit: `feat(qualification): authoritative event capture, retire per-shot .tch autosave (M4)`

**M5 — Qualification recovery**
Goal: resume any discipline incl. 3P positions + timers; authority
transfer completed. Files modified: QualificationFlow (primeFrom), QML
timer bindings to restored deadlines, router refill path, docs/assertions
flip (`globalMatchModel` = view). Tests: kill sweeps ×4 disciplines,
position-boundary restarts, timer goldens. Manual: 3P restart at 19/20/21
shots; prone restart mid-series.
Commit: `feat(qualification): full-session recovery for all disciplines (M5)`

**M6 — Reports & session browser**
Goal: any archived/recovered session renders its reports.
Files added: `SessionProjection`, `SessionBrowser.qml` (+ ListModel
adapter). Files modified: Summary/Match/Coach report inputs → projection.
Tests: report-input goldens from fixtures; browse+regenerate flow.
Commit: `feat(reports): reports as projections + archived session browser (M6)`

**M7 — Hardening**
Goal: production confidence. Adds: full fault-injection suite, 1000-cycle
sim, Android lifecycle hooks + suspend snapshot, support-bundle export,
log relocation, legacy-scan polish, performance report, docs refresh.
Acceptance: §25 complete, budgets confirmed, CLAUDE.md updated.
Commit: `chore(reliability): hardening, lifecycle handling and fault-injection suite (M7)`

Split re-evaluated after API definition: **unchanged** — each milestone
compiles alone, ships alone, and M2/M4 keep the dual-state window as short
as the invariant tests allow.

## Section 29 — Dependency graph

```
hardware adapters (MODREADER/tachuswidget)
        ↓ (signals only)
competition sources: Finals3PController · QualificationFlow   [ICompetitionSource]
        ↓ typed DomainEvents (submit)
SessionStore façade ──► JournalManager/Writer ──► files (StoragePaths)
        │                        ▲
        ├─► SessionReducer ──► SessionState
        │                        ▲
        │            ReplayEngine ── JournalReader/Validator ◄─ RecoveryCoordinator
        └─► EventBus ──► view routers/ListModels · HUD · audio · reports(projections)
                          · coach · DurableTap ──► RMS/spectator adapters (future)
```
Forbidden (enforced by include-guard test + .pri layering): reliability/*
must not include QML/QtQuick, audio, report builders, discipline headers,
or ModReader headers. No cycles: events depend on nothing; core on events;
journal on events+storage; store on all reliability parts; app on store.

## Section 30 — Final decision register

| # | Decision | Selected | Rejected | Reason | Consequence | Milestone |
|---|---|---|---|---|---|---|
| D1 | Timer interruption | does-not-count freeze; gap shown to operator | wall-clock-continues; CRO-always-decides | conservative, deterministic, jury can adjust via events | rebase logic in recovery | M3/M5 |
| D2 | Sighter durability | flush, no fsync | fsync-everything | no result authority; halves sync traffic in strings | ≤1 sighter lost on power cut, disclosed | M2 |
| D3 | Recovery scope order | finals-first (M3), all by M5 | all-at-once | finals = richest state machine, best harness | qualification waits 2 milestones | M3–M5 |
| D4 | Roadmap | M0–M7 as §28 | merge M4+M5 | independent releasability | 8 commits | all |
| D5 | Degraded mode | never-refuse-to-score, RAM queue + alarm | reject-on-failure (V1) | a fired shot must score | health model §10 | M2 |
| D6 | CompetitionEngine | **Option B** — controllers behind ICompetitionSource; full engine postponed | A (now), C (nothing) | Finals3PController already is the engine; qualification needs a new class anyway | seam exists for future engine | M2/M4 |
| D7 | Threading | synchronous UI-thread; graduation trigger p99>100 ms | writer thread now | simplest safe; API hides evolution | watchdog + metrics from M1 | M1 (revisit M7) |
| D8 | Hash algorithm | SHA-256 truncated 128-bit, chained | full SHA-256 (64 hex/line), BLAKE (not in QtCore) | QtCore-native; 128 bits ample for evidence | 32-hex `h`/`ph` fields | M1 |
| D9 | Checksum algorithm | **merged into chained hash** `[SUPERSEDES V2]` | separate CRC32 | no CRC32 in QtCore; redundant mechanism | one field, one procedure | M1 |
| D10 | Snapshot policy | stage/lifecycle-triggered events in-journal; +500-event cadence for Training only | per-50-events; separate files | 60-shot matches need none; no file pairs | §13 | M1/M2 |
| D11 | Retry queue capacity | aux bounded 4096 w/ recorded drops; officials elastic (+ emergency spill) | hard bound for all | officials must be undroppable | §9E | M2 |
| D12 | SQLite role | derived, rebuildable index only (M8+) | authoritative store | audit + V2 reasoning | `index/` dir reserved | M8+ |
| D13 | Legacy .tch | importer (read-only, flagged partial); autosave writer removed; deprecate after one release | conversion; keep writer | can't synthesize absent data | §27 | M4 |
| D14 | Android suspend | snapshot+fsync on applicationStateChanged(Suspended) | rely on torn-tail recovery | OS kill is the common case | §22 | M7 |
| D15 | Crypto signing | reserved `sig` slot in SessionClosed envelope; NOT implemented | sign now | no key-management story yet; chain suffices for evidence | one-field future add | future |
| D16 | Library packaging | `Reliability.pri` shared include; static lib later | SUBDIRS restructure now | qmake monolith risk during critical work | revisit post-M7 | M0 |

---

*(Original closing instruction, retained for the record: "STOP. Specification
only." That instruction applied when this document was written and under
review. It was subsequently acted on — the specification was implemented
through M0–M3 and Phases A–F. See "Design versus as-built status" above.)*
