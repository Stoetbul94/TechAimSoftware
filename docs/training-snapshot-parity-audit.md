# Stage 4.1 — Training Programme Snapshot Parity

**Phase:** STAGE 4.1 · **Audited and implemented at:** `d023d21` → this change
**Date:** 2026-07-29

Stage 4 gave Wind Map a proven snapshot/replay guarantee and reported that the
three older Training Lab programmes did not have one. This phase closes that
gap. No programme was redesigned; no workflow, scoring or analytics changed.

---

## 1. The defect

`ReplayEngine::replay(validPrefix, useSnapshot)` defaults to `useSnapshot =
true`, and `RecoveryCoordinator` uses that default. When a `StateSnapshot` is
present, replay starts from the snapshot's embedded state and folds only
`snapIdx + 1 …`. **Anything absent from the serialized snapshot is lost.**

Before this change, `serializeSessionState()` wrote **none** of:

- `sessionKind`
- any `training*` field, `trainingBlocks`, `trainingSighters` / `trainingSighterPos`
- any `cd*` field or `cdShots`
- any `pt*` field or `ptRecords`

and `SessionState::operator==` compared none of them.

Nothing in production emits a snapshot — `buildStateSnapshot()` is called only
by tests — so the defect has never fired. That is **accidental safety**, and
the point of this phase is that enabling periodic snapshots later must not
corrupt anything.

### 1.1 The worst part was `sessionKind`

`RecoveryCoordinator` classifies a candidate with
`c.sessionKind = s.sessionKind`, and the discipline dispatcher routes Training
sessions on it. Losing it at a snapshot boundary would make a recovered
Training session look like a **competition** session — a worse failure than
losing the projection, because it routes recovery to the wrong controller.

---

## 2. Fields added

### 2.1 Session classification
`sessionKind`.

### 2.2 Technical Blocks
`training` object — `active`, `completed`, `programId`, `blockCount`,
`shotsPerBlock`, `visibility`, `focus`, `currentBlock`, `currentPosition`,
`inSighterPhase`, `sighterPosition`, `sighterBeforeBlock`.

`trainingBlocks` array — per block: `blockIndex`, `position`, `completed`,
`note`, and the full `shots` array (each a complete `ShotCore`, so score and
`splitMs` cadence data survive as recorded values).

`trainingSighters` array — written as one array of `{ShotCore…, position}`
objects rather than the in-memory parallel vectors, so the shot list and its
position list **cannot desync** across a boundary.

### 2.3 Call & Diagnose
`callDiagnose` object — `active`, `completed`, **`callingActive`**,
`programId`, `focus`, `shotCount`, `currentPosition`, `threePositions`,
`sessionNote`.

`cdShots` array — per record: the **actual** `ShotCore`, `shotNumber`,
`position`, **`hasCall`**, `calledXHundredthMm`, `calledYHundredthMm`,
`callSplitMs`, `note`.

`hasCall == false` is the *awaiting the athlete's call* state: the actual shot
is already recorded but has not been revealed. Both it and `callingActive` are
persisted so an interrupted session resumes in the same phase.

### 2.4 Position Transition
`positionTransition` object — `active`, `completed`, `programId`, `sequence`,
`focus`, `verificationShots`, `repeats`, `checklistMode`, `currentPosition`,
`currentRepeat`, **`inSetup`**, **`verifying`**, `sessionNote`.

`ptRecords` array — per record: `position`, `repeat`, `setupDurationMs`,
`readyMonoMs`, `note`, `completed`, plus three separate arrays: `sighters`,
`verifShots` and `checklist`.

`inSetup` / `verifying` plus the separate `sighters` and `verifShots` arrays
are what keep **setup, sighters and counted verification shots
distinguishable** after a crash. `setupDurationMs` and `readyMonoMs` are the
rhythm/cadence inputs — recorded values, never re-derived, so they must
persist.

---

## 3. State version decision

**Bumped 4 → 5.**

The format genuinely changed: new keys were added. A v4 snapshot *exists* — the
golden fixture regenerated in Stage 4 — and does not contain them, so a reader
must be able to tell the two apart. Extending v4 in place would have made
"v4" mean two different things.

Backward compatibility is unchanged in behaviour: every new key is read
optionally and a v1–v4 snapshot restores to "no programme", which is exactly
what those snapshots meant.

---

## 4. Backward-compatibility evidence

| Evidence | Result |
|---|---|
| The committed **v4** fixture was read and replayed before regeneration | ✅ `committed fixture readable` and the replay assertion passed; only the byte-golden differed |
| Explicit v4-compat test: serialize v5, strip every v5 key, relabel to v4, re-read | ✅ loads, restores to "no programme", not an error |
| Full v5 round-trip | ✅ lossless |
| Malformed `training` value (object → array) | ✅ **rejected**, not guessed |
| Competition journal with no Training events | ✅ all projections empty |

### 4.1 Why the golden fixture bytes changed

`fixture_finals_clean.jsonl` contains a `StateSnapshot`, whose payload is a
serialized `SessionState`. Adding keys changes those bytes. The fixture was
regenerated **through the harness's own `--write-fixtures` path**, not by
hand, after confirming the previous fixture still deserialized and replayed.
`stateVersion` 4 → 5; size 6773 → 7596 bytes.

---

## 5. Snapshot-boundary test cases

Every case places a `StateSnapshot` mid-programme and asserts the snapshot fast
path equals fold-from-zero. That comparison uses `SessionState::operator==`,
which now includes all three projections — **a field that is folded but not
serialised fails there**.

| Programme | Cases |
|---|---|
| Technical Blocks | crash during sighters · crash during a block · crash immediately after a shot · snapshot between blocks · snapshot across a position change · completed session |
| Call & Diagnose | crash before a shot · **crash while awaiting a call** · crash after call confirmation · crash during reveal · snapshot between multiple records · completed session |
| Position Transition | crash during PositionSetup · crash during Sighters · crash during VerificationActive · crash during PositionReview · snapshot across K→P · repeated transitions · completed session |
| Shared | v4 compatibility · v5 round-trip · malformed snapshot fails safely · competition journal unchanged |

**Fold-from-zero equivalence is asserted in every boundary case** — 17 of them.

### 5.1 The two critical correctness rules

**Call & Diagnose:** a shot awaiting a call recovers into that exact phase.
Asserted directly — `hasCall` stays false, `callingActive` stays true, and the
hidden actual shot's coordinates and score are preserved exactly, neither
revealed nor discarded.

**Position Transition:** setup, sighters and counted verification shots stay
distinguishable. Asserted directly — `inSetup` / `verifying` survive, and a
record's `sighters` and `verifShots` arrays stay separate across the boundary.

---

## 6. Production snapshot policy — unchanged

- **Snapshots are NOT emitted in production.** `buildStateSnapshot()` is
  called only by tests. Verified by grep at the end of this phase.
- **The fast replay path defaults to enabled** (`useSnapshot = true`), and
  `RecoveryCoordinator` relies on that default.
- **All four Training Lab programmes are now snapshot-safe** — Technical
  Blocks, Call & Diagnose, Position Transition and Wind Map.
- **Enabling periodic snapshot emission remains a separate reviewed
  decision.** This phase did not enable it, and does not recommend enabling it
  without its own review of write cost, cadence and failure behaviour.

---

## 7. What did not change

- No programme was redesigned; no operator workflow changed.
- No scoring or analytics formula changed.
- No QML changed.
- No existing equality or replay check was weakened — the equality operator
  only gained comparisons.
- Qualification, Finals and Wind Map suites remain green.
