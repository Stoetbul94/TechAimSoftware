# Wind Map — Recovery Architecture Audit and Decision

**Phase:** TRAINING LAB RELEASE 2 · **Stage 4 — recovery and dispatch**
**Audited at commit:** `bf5b78e` · **Date:** 2026-07-29

Stage 3 kept Wind Map projections out of snapshots on the grounds that the
other Training programmes do the same. That reasoning was challenged, and the
challenge was correct. This document records what the recovery path actually
does and what was changed.

---

## 1. Audit findings

### 1.1 Recovery does NOT replay from event zero

`ReplayEngine::replay(validPrefix, useSnapshot)` — `src/reliability/replay/ReplayEngine.cpp`:

```cpp
if (useSnapshot) {
    const int snapIdx = lastSnapshotIndex(validPrefix);
    if (snapIdx >= 0) {
        …deserializeSessionState(snap.stateJson, &start)…
        return foldFrom(start, validPrefix[snapIdx].seq,
                        validPrefix, snapIdx + 1, /*usedSnapshot*/ true);
    }
    // a corrupt snapshot payload falls through to fold-from-zero
}
return foldFrom(SessionState(), 0, validPrefix, 0, false);
```

**`useSnapshot` defaults to `true`** (`ReplayEngine.h`), and
`RecoveryCoordinator` calls `ReplayEngine::replay(rep.validEnvelopes)` — i.e.
with the fast path enabled, at both call sites (lines 89 and 270).

So recovery begins at **the last `StateSnapshot`**, not at event zero, and
folds only `snapIdx + 1 …` onward. **Any state not carried in the serialized
snapshot is lost at that boundary.** Fold-from-zero happens only when there is
no snapshot, or when the snapshot payload fails to deserialize.

### 1.2 No production code emits a snapshot — today

`buildStateSnapshot()` exists in `SessionState.cpp` and `StateSnapshot` is
fully typed, validated, serialized and folded. But nothing in `src/` submits
one: the only producers are the test fixtures.

**This is why the existing programmes have not been bitten.** It is an
accident of the current write path, not a designed guarantee.

### 1.3 The existing Training projections are NOT snapshot-serialised

`serializeSessionState()` wrote no `training*`, `cd*` or `pt*` fields at all,
and `SessionState::operator==` compared none of them. Verified by grep: zero
occurrences of `trainingBlocks`, `cdShots` or `ptRecords` in `SessionState.cpp`.

**Consequence:** the moment anything starts emitting periodic snapshots, every
Technical Blocks, Call & Diagnose and Position Transition projection silently
truncates at the snapshot boundary. That is a **pre-existing latent defect**,
reported in §5 — deliberately not fixed here, because widening this change to
three other programmes without their own test matrices would be worse than
naming the problem.

### 1.4 Other audit answers

| Question | Answer |
|---|---|
| When are snapshots created? | Never, in production. `buildStateSnapshot()` is available and used only by tests. |
| What is replayed after a snapshot? | Only events **after** the snapshot's index; the snapshot supplies the starting state. |
| Does recovery begin at event zero? | **No** — only when there is no snapshot, or the snapshot is corrupt. |
| How do existing Training projections survive? | They do not. They survive only because no snapshot is ever written (§1.2/§1.3). |
| Do incomplete Training sessions use a different path? | No. `RecoveryCoordinator` classifies them (`sessionKind == "Training"`, programme id from `trainingProgramId` / `cdProgramId` / `ptProgramId`) but replays through the same `ReplayEngine::replay`. |
| Do archived/completed sessions differ? | Completion is detected from the replayed state (`trainingCompleted` or `lifecycle == Complete`) and such sessions are auto-archived rather than offered for resume. The replay path itself is identical. |

---

## 2. Decision — OPTION A

**Wind Map projections are snapshot-serialised.** State version **3 → 4**.

Option B was rejected: complete replay is **not** an intentional, tested rule.
It is an artefact of snapshots never being emitted, and the brief explicitly
forbids relying on an undocumented convention.

### 2.1 Snapshot fields added

`windMap` object — `active`, `completed`, `programId`, `disciplineId`,
`threePositions`, `currentPosition`, `positionSequence`, `conditionChanges`,
`nextShotId`, and the **standing condition** (`windValid`, `windCalm`,
`windDirDeg`, `windSpeedHundredthMs`, `windSource`, `windRecordedMs`,
`windNote`).

`windMapShots` array — one object per recorded shot: the full `ShotCore`,
`shotId`, `position`, `sighter`, and **that shot's own immutable wind
snapshot**.

Field order is frozen and every field is always written, so the bytes stay
deterministic.

### 2.2 Backward compatibility

A v1–v3 snapshot has neither key. Both are read optionally and restore to
"no Wind Map session" — never an error, never an inferred reading. Proven by
a dedicated compatibility test that strips the v4 keys and re-reads.

The committed golden fixtures were regenerated through the harness's own
`--write-fixtures` switch (`stateVersion` 3 → 4). The **pre-existing v3
fixture still replayed correctly** before regeneration — only the byte-golden
differed — which is itself the backward-compatibility evidence.

### 2.3 Proof that pre-snapshot state cannot be lost

Three independent mechanisms, all now active:

1. **Test 18** places a snapshot *after* a sighter and three counted shots
   spanning two conditions, then adds more events in the tail. It asserts all
   four shots are present with their original snapshots. Without the `windMap`
   keys this fails immediately.
2. **Test 19** replays the identical journal twice — snapshot fast path and
   fold-from-zero — and asserts `noSnap.state == s`. Because Wind Map is now
   part of `SessionState::operator==`, any divergence at the boundary fails
   the test.
3. **`ReplayEngine::snapshotsAgreeWithFold()`** compares a snapshot's embedded
   state against the fold. Adding Wind Map to `operator==` turns that existing
   check into a real check on the Wind Map projection instead of a no-op.

Point 3 is why `operator==` had to change *together with* serialization:
including the fields in equality but not in the snapshot would have made this
check fail loudly — which is the correct failure mode, and the reason the two
must never drift apart.

---

## 3. Sync / Broadcast review

Reviewed per event rather than assigned for consistency.

| Event | Durability | Broadcast | Reason |
|---|---|---|---|
| `WindMapSessionStarted` | **Sync** | Broadcast | Session identity. If it is lost, later events fold against no session and the reducer rejects them. |
| `WindConditionChanged` | **Sync** | Broadcast | Frequent and individually low-stakes — **but it is the value every later shot snapshots**. Losing it changes what subsequent shots recorded, which is a data-integrity loss, not a UI glitch. Flush would be cheaper and wrong. |
| `WindMapSighterAccepted` | **Sync** | Broadcast | A recorded shot. Durable before anything acts on it. |
| `WindMapShotAccepted` | **Sync** | Broadcast | As above. |
| `WindMapPositionChanged` | **Sync** | Broadcast | Determines which position later shots are attributed to; 3P separation depends on it. |
| `WindMapSessionCompleted` | **Sync** | Broadcast | Lifecycle terminal. Drives auto-archive versus resume-offer. |

### 3.1 Broadcast safety

**Broadcasting `WindConditionChanged` cannot affect official scoring, another
lane, or a central match controller.** Verified:

- **Scoring:** Wind Map events carry no score input.
  `CenterPane.qml::calculateShootingSocre()` is untouched, and `ShotCore`
  scores are recorded as delivered, never recomputed from wind.
- **Other lanes / central controller:** there is no networking in this
  release. `BroadcastClass::Broadcast` marks an event as visible to in-process
  UI subscribers on this lane only; the lane app's existing UDP shot broadcast
  is a separate, pre-existing path that Wind Map does not touch.
- **Competition state:** Wind Map is `sessionKind = Training` and is never a
  qualification or Final session, so no competition result can depend on it.

Broadcast is retained because the in-session UI must reflect the standing
condition; it grants no reach beyond this lane's own process.

---

## 4. Dispatcher and registration coverage

| Path | Status |
|---|---|
| Typed-event variant (`DomainEvent`) | ✅ all 6, appended at the end |
| Serializer (write arms) | ✅ all 6 |
| Deserializer (read arms) | ✅ all 6 |
| Event-name mapping (`kType`) | ✅ all 6 |
| Mutation classification (`ReducerClass`) | ✅ all 6 `Mutating` |
| Sync / Broadcast policy | ✅ all 6, justified per event in §3 |
| Journal validation (`validate()`) | ✅ all 6, incl. discipline/position/wind-shape rules |
| Replay reducer | ✅ all 6 fold arms |
| Snapshot serialization | ✅ state v4 |
| `SessionState::operator==` | ✅ all Wind Map fields |
| Catalogue coverage test | ✅ all 6 in `tst_serializer`'s round-trip catalogue, which asserts it covers every `DomainEvent` alternative |

**No event can serialize and then be silently ignored during recovery:** the
catalogue test asserts one entry per variant alternative, and every event has
a reducer arm. An event with no fold arm would leave the state unchanged and
be caught by the recovery matrix.

**Failing closed:** unsupported disciplines, positions outside the discipline,
unknown wind sources and contradictory snapshots (a "no reading" claiming to
be calm) are all rejected at validation or in the reducer, so no partially
valid session is produced.

### 4.1 Deliberately deferred

The **QML restorer branch** (`main.qml::dispatchRecovery` →
`shootingPage.restoreWindMapSession`) is **not** implemented. There is no
`WindMapController` yet, so the branch would call into nothing. The C++
recovery path — replay, snapshot, classification — is complete and tested;
the QML hop lands with the controller.

---

## 5. Pre-existing defect — reported, not fixed

**Technical Blocks, Call & Diagnose and Position Transition projections are
not snapshot-serialised** (§1.3). They are safe only while nothing emits a
snapshot. If periodic snapshots are ever enabled, those three programmes will
silently lose everything before the boundary — the exact failure Wind Map was
just protected against.

Not fixed here: it is three more programmes, each needing its own field set,
equality entries and recovery matrix. Recommended as its own phase.

---

## 6. Fixed-point speed — authoritative representation

Stored as **hundredths of a metre per second** (`qint32`). The journal writer
has no `double` overload; every numeric payload field is an integer so replay
is bit-exact and hashes are stable.

| Rule | Implementation |
|---|---|
| UI and reports display m/s | `WindConditionSnapshot::speedMetresPerSecond()` |
| Journals store the integer | `windSpeedHundredthMs` |
| No manual scaling in QML | conversion exists **only** in the two helpers |
| NaN / infinite / negative rejected before conversion | `metresPerSecondToHundredths()` returns false |
| Deterministic rounding | `std::llround` — half away from zero |
| Round-trips exactly | asserted in `tst_windmap` |
| Maximum bounded | `kMaxWindSpeedHundredthMs = 100000` (1000.00 m/s) |
| Overflow rejected | anything above the ceiling returns false |

`metresPerSecondToHundredths(double, qint32*) -> bool` ·
`hundredthsToMetresPerSecond(qint32) -> double`

The raw hundredths value is storage and is never shown to an operator.
