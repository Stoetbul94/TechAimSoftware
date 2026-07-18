# Session Reliability Layer — M2 implementation note (finals write path)

Blueprint: docs/session-reliability-implementation-spec.md (§3, §9, §10,
§12, §17, §28 M2). Scope: **finals persists through the SessionStore;
behaviour identical; degraded mode live.** The legacy per-line
`writeJournal` is gone. Qualification is untouched (M4).

## What changed

- **Finals3PController** owns a `ta::rel::SessionStore` and submits typed
  domain events at every point it used to call `writeJournal`. The legacy
  `writeJournal` / `archiveExistingJournal` / `reportJournalFailure` are
  deleted. The controller's own state machine stays authoritative for
  scoring and timing — a reducer rejection is a logged journal-fidelity
  diagnostic, never a behaviour change.
- **New store layer** under `src/reliability/store/`: `SessionStore`
  (façade), `JournalManager` (file lifecycle + archive-on-close),
  `PersistenceRetryQueue` (RAM buffer), `MonotonicClock` (clock injection).
- **Event catalogue extended** (appended after every M1 type so variant
  indexes never move): `StageEntered`, `StageStatusChanged`,
  `TargetModeChanged`, `WindowOpened`, `WindowClosed`, `CommandIssued`,
  `ShotRejected`, `MissingShotRecorded`, and the persistence markers
  `PersistenceDegraded`, `PersistenceRestored`, `AuxEventsDropped`,
  `CleanShutdown`.
- **Degraded persistence is live** (spec §9C-E): a fired shot always
  scores even when the journal write fails.
- **PersistenceBanner.qml** (view-only) + `Finals3PController.persistenceHealth`
  Q_PROPERTY surface the degraded state to the operator; `journalWriteFailed`
  still routes to the M0 error dialog.

## SessionStore contract (spec §3)

`beginSession(header)` starts the monotonic clock, opens
`Sessions/Current/session_<utc>_<uuid8>.jsonl`, and writes `SessionStarted`
at seq 0 (must be durable — a session that cannot record its own start does
not proceed). `submit(event)` is the never-refuse path:

1. assign a dense logical seq, apply to the reducer (in seq order);
2. if the reducer **rejects** (illegal transition) → refuse to the caller,
   reclaim the seq, persist nothing (the event was invalid, not unlucky);
3. otherwise persist: while the retry queue is non-empty, queue behind it
   (order preservation); else attempt a direct durable write. A clean write
   failure queues the event and enters Degraded; an appended-but-unsynced
   write reports Degraded without re-queuing (the bytes exist); a torn tail
   goes Failed (RAM only).

`SubmitResult.persistedDurably` tells the caller whether the write reached
disk. `closeSession(reason)` drains, writes `SessionClosed` + `CleanShutdown`,
and archives the file to `Sessions/Archive/<yyyy>/<MM>/`.

## Sequence & the degraded window (spec §9D/§9E)

Logical seq is dense and assigned at submit. The write-order hash chain is
owned by `JournalWriter` (its `appendSeq` takes a store-supplied seq so the
logical order and the physical write/chain order can differ across a
degraded episode). Dropped aux events leave a permanent gap in the physical
seq stream; every drop is declared by an `AuxEventsDropped{firstSeq,lastSeq,
count}` marker (coalesced into contiguous runs). The **validator** now
accepts a forward gap provisionally and, at end-of-journal, requires
`gapped-seqs == declared-dropped-seqs` — an undeclared gap is
`CorruptInternal` (loss is recorded, never silent). Because `AuxEventsDropped`
is only ever emitted inside a `PersistenceDegraded → PersistenceRestored`
episode, this implies "gaps only inside a degraded window" without depending
on the physical order of the bracketing markers.

## Retry queue capacity (spec §9E / D11)

Officials (durability class **Sync** — official shots, missing shots,
corrections, penalties, stage/lifecycle) are elastic and **never dropped**.
Aux (Flush/Append — sighters, window, command, timer, rejected) are bounded
at 4096; when full the oldest aux is dropped and counted. The pump drains
in seq order; on drain it writes the `AuxEventsDropped` markers then
`PersistenceRestored` and returns to Healthy. The controller drives the pump
opportunistically on its 50 ms UI tick while degraded.

## Finals event mapping (legacy → typed)

| Legacy `writeJournal` | M2 submit |
|---|---|
| `finalStarted` + archive | `beginSession()` → `SessionStarted` (seq 0) |
| `stageEntered` | phase event (`Preparation/Sighting/OfficialMatchStarted`) + `StageEntered` |
| `shotAccepted` (official) | `ShotAccepted{ShotCore}` |
| `shotAccepted` (sighter) | `SighterAccepted{ShotCore}` |
| `shotRejected` | `ShotRejected` |
| `missingShot` | `MissingShotRecorded` |
| `stageStatus` | `StageStatusChanged` |
| `command` | `CommandIssued` |
| `windowOpened`/`windowClosed` | `WindowOpened`/`WindowClosed` |
| `finalCompleted` | `MatchCompleted{reduced totals}` |

Finals decimal scores are exact tenths, so `ScoreTenths::fromDouble` and the
controller's `double` accumulation agree exactly — the dual-state invariant
(`reducer totalTenths == round(controller total × 10)`) holds and is asserted
in the harness after a driven run. The reducer's coarse phase model
(Preparation/Sighting/OfficialMatch) is fed from stage entry so shot/sighter
acceptance is legal in the reduced state.

## Numeric conversion boundary

Coordinates `xMm/yMm` → `CoordinateHundredthMm` (0.01 mm), score →
`ScoreTenths` (0.1), direction → `CentiDegrees` (0.01°), split → ms. A
negative/absent external id is normalised to 0 so the reducer's duplicate
check never false-matches simulated shots. The shot score ceiling was
widened from 109 to 110 tenths to admit the controller's defensive 11.0
acceptance limit (the SRL must never reject a shot the controller accepted).

## Tests

- **Reliability harness** (`tests/reliability`): **566 checks, 0 failures**
  (was 528 at M1). New `tst_store.cpp` module: clean session end-to-end,
  never-refuse-to-score (illegal event refused + seq reclaimed), the full
  degraded drill (writes fail → shots score → queue accumulates → drain →
  Healthy → journal validates Clean, Degraded/Restored markers present),
  bounded-aux drops (officials never dropped, `AuxEventsDropped` accounts
  for every drop), archive-on-close into `Sessions/Archive/2026/07`, plus
  the validator's degraded-gap reconciliation (undeclared gap rejected,
  declared gap Clean, phantom declaration rejected).
- **Finals harness** (`tests/finals`): **180 checks, 0 failures** (178
  parity + 2 new). The 178 pre-existing checks pass unchanged against the
  envelope journal; the two additions verify the live controller journal
  validates **Clean** through the SRL validator end-to-end and the
  dual-state invariant holds.
- Full app builds clean (qmake warning-free).

## Known limitations / deferred (M3+)

- No startup replay, recovery UI, or resume — a crashed finals session
  leaves its journal in `Sessions/Current`; M3 adds the RecoveryCoordinator
  and resume/archive/discard flow.
- The retry pump is driven by the finals UI tick; the dedicated
  writer-thread graduation (spec §11 D7) is not triggered.
- The full persistence-health model (Slow watchdog, per-5-min critical
  dialog, ReadOnly gate) is partially realized — M2 drives
  Healthy/Degraded/Failed; the remaining states land with M3/M7.
- Qualification still has no event capture (M4); the `.tch` autosave is
  untouched.
- `TargetModeChanged` is defined but not emitted (target mode travels on
  each ShotCore); reserved for future explicit-mode auditing.
