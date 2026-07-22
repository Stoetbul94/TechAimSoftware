# Session Reliability Layer — M3 implementation note (recovery & resume)

Blueprint: docs/session-reliability-implementation-spec.md (§3, §13, §15,
§16, §28 M3). Objective (verbatim): *"At any point during a match, if the
application crashes, loses power, or is terminated, the operator can restart
TechAim and safely continue the match."*

## The architecture, unchanged

The M1 typed journal is the only source of truth. Recovered state is rebuilt
**exclusively** through the reducer — nothing here serializes, deserializes,
or snapshots controller internals:

```
Journal → Validator → Reducer → Recovered State → Controller
```

`Finals3PController::loadRecoveredState` only *injects* the reducer-rebuilt
state; it never replays UI events, presses buttons, or re-issues commands.

## Components created

- **`src/reliability/replay/ReplayEngine`** — folds the validator's valid
  prefix through the reducer to reconstruct `SessionState`. Snapshot-optimized
  (start from the last `StateSnapshot`'s embedded state + fold the tail) with a
  fold-from-zero drift check; the two must agree. Replay tolerates the
  validator-vetted declared-drop gaps (§9E) — the live reducer stays strict.
- **`src/reliability/recovery/RecoveryTypes`** — `RecoveryClass`
  (Clean / Recoverable / Corrupt / Fatal), `RecoveryCandidate` (dialog
  metadata, all derived from the reducer state), `RecoveredMatchState` (the
  reducer state + resume metadata; the byte offset of the valid prefix for
  torn-tail truncation).
- **`src/reliability/recovery/RecoveryCoordinator`** — owns recovery: scans
  `Sessions/Current`, validates each journal, classifies it, replays it, and
  exposes candidates (C++ and a QML `scanForQml()`). Auto-archives
  cleanly-closed sessions; moves discarded/corrupt ones to `Sessions/Corrupt`
  (byte-exact original preserved). **Discipline-agnostic** — Qualification and
  Training can adopt it unchanged.

## Components modified

- **`SessionStore::resumeSession`** — reopens the recovered journal in append
  mode, truncates any torn tail (backing it up as `<name>.tornbak` first),
  seeds the write-order hash chain + sequence from the last valid line
  (`JournalWriter::resumeFrom`), adopts the reducer-rebuilt state, and records
  `RecoveryStarted` / `RecoveryCompleted`. After this the session continues as
  a live one — `submit()` appends onto the recovered file.
- **`Finals3PController`** — `loadRecoveredState(RecoveredMatchState)` restores
  stage, athlete, shot counters, totals, stage subtotals/statuses, window id
  and report records from the reducer state, refills the UI models via
  re-emitted display signals (no re-journaling), and resumes the journal. QML
  wiring: `scanForRecovery()`, `resumeFromRecovery(id)`, `discardRecovery(id)`.
- **`RecoveryDialog.qml`** + `main.qml` startup scan — before LoginPage, an
  unfinished match is surfaced with Resume (Clean/Recoverable) or Discard;
  a non-resumable session shows the reason and withholds Resume.

## Recovery classification (spec Task 3)

| Validator classification | Recovery verdict | Resume |
|---|---|---|
| Clean | **Clean** | yes — resume directly |
| TailTruncated (valid prefix) | **Recoverable** | yes — up to last valid event |
| CorruptInternal (has header) | **Corrupt** | no — wizard / partial (deferred) |
| UnsupportedVersion / SessionMismatch / no header | **Fatal** | never |

## Resume flow (spec §15)

```
start → RecoveryCoordinator.scan(Sessions/Current)
      → JournalValidator (JSON, seq, hash chain, schema, snapshot linkage,
        persistence markers, degraded reconciliation, totals, ordering)
      → ReplayEngine (snapshot → fold tail → SessionState)
      → RecoveredMatchState
      → SessionStore.resumeSession (truncate tail, seed chain, adopt state,
        RecoveryStarted/RecoveryCompleted)
      → Finals3PController.loadRecoveredState (inject; refill UI)
      → continue match
```

Idempotency (spec §15): a crash *during* recovery leaves `RecoveryStarted`
without `RecoveryCompleted`; the next run simply recovers again — replaying
the resumed journal reproduces the resumed state exactly (tested).

## Timer recovery (spec §16, M3 scope)

Default freeze: on resume the monotonic clock restarts and the operator/jury
continues the current stage. **Scores, stage, counts and totals are exact**;
only the sub-stage countdown is reset conservatively. The recovery dialog
shows the saved time so the jury can adjust. Shot-second-accurate window
rebase (finals `WindowOpened` tm + duration) is deferred — finals does not yet
emit `TimerStarted` events, so the reducer holds no window deadline to rebase.

## Test results

- **Reliability harness**: **594 checks, 0 failures** (was 566 at M2). New
  `tst_recovery.cpp`: replay == live state (byte-identical); **restart after
  EVERY event** replays to the exact prefix state; **power-loss byte sweep**
  over the final line always recovers a valid prefix; snapshot replay ==
  fold-from-zero (no drift); degraded-then-recovered replay (all officials +
  totals survive aux drops); coordinator scan/classify/rebuild; **full resume
  flow** (crash → recover → resume → continue) with idempotent re-replay;
  Fatal (newer schema) not resumable.
- **Finals harness**: **189 checks, 0 failures** (was 180 at M2). New
  controller-recovery block: a crashed finals journal is discovered, classified
  Clean+resumable, rebuilt via the reducer, and injected into a **fresh
  controller** whose official count, total and stage match the crashed match
  exactly; the resumed journal validates Clean and carries the recovery
  markers.
- Full app builds clean.

## Acceptance (spec Task 12)

A recovered match is indistinguishable from one that never crashed: same
totals, same stage, same reducer state, same replay result, and the resumed
journal remains a single valid hash chain. Proven by the state-equality and
byte-identity assertions above.

## Manual crash-recovery drill (2026-07-18)

Release build, real 3P Final: fired 5 sighters + 3 official kneeling shots
(total **31.9**, stage KneelingMatch), then **hard-killed** the process
(`taskkill /F` — a crash). On restart the **recovery dialog appeared with the
exact reducer-replayed state**: *50m Rifle 3 Positions Final, Arnold Bailie,
3 / 35, Total 31.9, Journal Clean*. Resume rebuilt the controller state
exactly — the app log confirms *"recovered session … stage KneelingMatch
officials 3 total 31.9"* — and the journal resumed with `RecoveryStarted` /
`RecoveryCompleted` markers, still validating Clean.

### UI-identical fix (2026-07-18, follow-up)

Root cause (investigated): `isFinalsMatch` — the single flag driving the
finals view, the RightPanel layout, and the `enabled: isFinalsMatch` shot
router — was set only inside `beginPreparationPhase()`, welded to
`FINALS3P.resetFinal() + startFinal()` (a fresh session). Recovery had to skip
the whole function to avoid wiping state, so it never engaged finals mode.

Minimal architectural fix (no parallel path): the finals mode-engagement was
extracted into a shared `enterFinalsMode(startFresh)` in ShootingPage.qml.
`beginPreparationPhase()` calls `enterFinalsMode(true)` (canonical path
unchanged); recovery calls `enterFinalsMode(false)` **before** `resumeFromRecovery()`,
so `isFinalsMatch` is true when the re-emitted recovered shots arrive and they
route into the finals models. A firing-state restoration
(`Finals3PController::restoreStageFiringState()`) re-establishes the correct
window + target mode for the recovered stage so the athlete can fire the next
shot immediately.

**Re-verified manual drill:** 3 official kneeling shots (total 31.9) → hard
kill → restart → recovery dialog exact → Resume → the ShootingPage renders the
**actual finals layout** (SERIES K, finals HUD), all 5 sighters + 3 officials
populated, TOTAL 31.9 / KNEELING 3/10. Fired one more official (10.7): accepted,
**TOTAL incremented to 42.6**, count 4/35. The resumed journal recomputes a
valid SHA-256 chain across all 34 lines. **The "UI identical" criterion (Task 13)
now PASSES.**

## Known limitations / deferred

- Recovery Wizard for `Corrupt`/`Fatal` sessions (recover-to-last-valid, view
  validation report, export journal) is stubbed — the dialog withholds Resume
  and shows the reason; the partial-recovery wizard lands later.
- Sub-stage timer rebase is conservative (freeze + operator adjust), not
  window-second-accurate — needs finals `TimerStarted` events (future).
- Command history (`m_events`) is not reconstructed on resume (commands are
  journal markers, not reducer state); the post-recovery report timeline may
  be shorter. Scoring/continuation is unaffected.
- Qualification and Training recovery is not wired (the coordinator is generic
  and ready; only finals is connected). Hardware-disconnection handling is
  untouched.
