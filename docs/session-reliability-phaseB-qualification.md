# Phase B — Qualification persistence (B0 trace + shared seam)

Migrates the qualification live-scoring paths (10m Air Rifle, 10m Air Pistol,
50m Rifle Prone) through `SessionStore` so the **journal + reducer become the
authoritative state**, mirroring the finals write-path (M2). Recovery/restore
of qualification UI is **Phase D** — Phase B only proves the journal contains
everything a later deterministic restore needs.

See `docs/issf-rules/` for the discipline rules and
`docs/issf-rules/est-malfunctions.md` for EST handling.

## B0 — Existing live-scoring flow map (as-is, before any change)

```
MODREADER (Modbus backend, C++)                         ← raw target measurement enters here
  onShootCountChanged(count)         [CenterPane.qml:248]
  getXCord(i) / getYCord(i)          [CenterPane.qml:276]  raw mm coords
  dedup guard: backEndShootCount < shooutIndex [CenterPane.qml:272]  ← duplicate rejection
  map mm → pixel (itemPoint)         [CenterPane.qml:292]
  calculateShootingSocre(x,y,…)      [CenterPane.qml:1817] ← score calc (ISSF ring geometry)
      calculatedSccore (DECIMAL)     [decimal/integer: raw is decimal; integer via floor at aggregation]
      MODREADER.setScore(...)
  addIfinRange → polarSeries.append  [CenterPane.qml:1723]  chart only
  emit pointAddedToSeries
    ShootingPage.onPointAddedToSeries [ShootingPage.qml:543]
      if isFinalsMatch:  FINALS3P.registerShot(...)   ← ALREADY journalled (M2)
      else (QUALIFICATION):
        hard-cap: globalMatchModel.count >= matchShootCount → drop [ShootingPage.qml:564]  ← official cap (≤60)
        rightPanel.addToSeries(x,y,score,xmm,ymm)      [RightPanel.qml:1082]
          classify by sligterMode:                     ← sighter vs official classification
            sighter  → globalSlighterModel.append      [RightPanel.qml:1112]
            official → globalMatchModel.append          [RightPanel.qml:1121]
          always     → globalModelOfData.append (display buffer) [RightPanel.qml:1129]
          shot number = model index (implicit)          ← official number = globalMatchModel.count
          totals: grandTotal (decimal) / grandTotalExculdeDec (integer) [RightPanel.qml:1089-1091]
          ▲ NO JOURNAL — the UI ListModels are the authoritative record today
```

Phases / timers (qualification, as-is):
- Prep+Sighting: `beginPreparationPhase()` → `centerPanel.startPreparationCountdown()`,
  `totalSighterTime = APPSETTINGS.getPrepTimeCount()` (default **15 min**)
  [ShootingPage.qml:894, appsettings.h:173].
- End of sighting: `onSighterModeTimerEnds → changedToMatchMode()` sets
  `sligterMode = false`, starts the match clock.
- Match clock: `centerPanel` game timer, `totalGameTime =
  APPSETTINGS.getTimeCount(matchShootCount)` (AR/AP **75 min**, Prone **50 min**)
  [LoginPage.qml:298].
- Completion: auto-finish watcher + the hard-cap guard when
  `globalMatchModel.count >= matchShootCount`.

Discipline selection (per the scope gotcha): `gameMode` 0 = pistol, 1 = rifle;
`gameRange` 10/50; `gameSubMode` 0 = Prone / 1 = 3P. So AR10 = mode1/range10,
AP10 = mode0/range10, Prone50 = mode1/range50/subMode0.

## The finals pattern this mirrors (M2, reference)

`Finals3PController` owns a `SessionStore`; on each shot it builds a `ShotCore`,
`submitEvent(ShotAccepted|SighterAccepted)` → `m_store->submit(...)`, then
`emit shotAccepted(record)` → the QML router appends to the models
[Finals3PController.cpp:930-937]. Submit-then-emit **is** the durability
boundary.

## B0 — Shared seam (design)

A new **`QualificationController`** (`src/qualification/`, context property
`QUAL`) — the discipline-agnostic persistence seam for all three qualification
disciplines. It mirrors `Finals3PController`'s `SessionStore` usage but is
simpler (no stages / command windows / audio). It contains **no ISSF rule**:
durations, shot counts and decimal-vs-integer are passed in as parameters from
QML (which reads them from `APPSETTINGS` / the discipline selectors).

Responsibilities (C++):
- own `SessionStore`; surface `persistenceHealth` + the store's health signals
  (reuse `PersistenceBanner`);
- `startSession(discipline, matchType, athlete, officialShots, matchMs,
  prepMs, sighterLimit, lane, targetId)` → `SessionStarted` header +
  `beginSession`;
- `beginPreparation()` / `beginSighting()` / `beginOfficialMatch()` →
  `PreparationStarted` / `SightingStarted` / `OfficialMatchStarted` (+ optional
  `TimerStarted` anchors);
- `submitSighter(xMm,yMm,scoreTenths,externalId,directionCentiDeg,simulated)`
  and `submitOfficial(...)` → build `ShotCore` → submit
  `SighterAccepted`/`ShotAccepted` → **on success** `emit shotAccepted(record)`;
- `completeMatch()` → `MatchCompleted`; `closeSession()` → `CleanShutdown` /
  archive.

QML wiring (activated per-discipline in B1–B3, NOT in B0):
`ShootingPage.onPointAddedToSeries` (qualification branch) calls
`QUAL.submitSighter/submitOfficial(...)`; a `Connections { target: QUAL }
onShotAccepted` routes the record to `rightPanel.addToSeries(...)` — so the
model append happens **only after** a durable submit. Classification
(sighter/official) comes from `sligterMode`; the official shot number is the
reducer's `officials.size()+1` (authoritative), no longer the UI index.

## Durability boundary (B0 contract)

**A shot is not considered accepted until its `ShotAccepted` /
`SighterAccepted` event has been submitted to `SessionStore`.** Sequence:

1. raw measurement → validate/classify/score (unchanged QML pipeline);
2. `QUAL.submit*` builds the typed event and calls `SessionStore::submit`;
3. `SessionStore` applies the never-refuse-to-score policy (spec §9C):
   official shots are journalled, or durably queued (elastic queue) when
   persistence is degraded — they are never dropped;
4. on submit acceptance the reducer state updates and `shotAccepted` is
   emitted → the QML models/target-face/right-panel update;
5. if persistence is degraded, health is surfaced (`PersistenceBanner`) — the
   shot is not silently lost and is retried by the existing `SessionStore`
   policy.

Duplicate prevention (identical to finals):
- **Normal live scoring:** `CenterPane.backEndShootCount` gates on the backend's
  strictly-increasing cumulative detection counter; the same raw index is never
  processed twice. The counter value at receipt is carried as the event
  `externalId`.
- **Delayed persistence retry:** the retry queue re-submits already-classified
  events; it does not re-read hardware, so no new shot identity is created.
- **Application crash:** unsubmitted shots are lost by definition (they never
  reached the journal); everything submitted before the crash is durable.
- **Recovery replay:** the reducer rejects a duplicate official `shotNumber` and
  a duplicate non-zero `externalId` [SessionReducer.cpp], so re-applying the
  valid prefix cannot double-count.
- **Resumed hardware input:** on resume the controller seeds its next
  official number from the reducer's `officials.size()`, and `backEndShootCount`
  from the recovered shot count, so a re-reported index is ignored.

## Implementation increments

- **B0** (this): trace (above) + `QualificationController` skeleton + a harness
  test proving a full qualification session journals & replays. **No ShootingPage
  rewiring** — additive only, existing behaviour untouched.
- **B1** 10m Air Rifle: wire `onPointAddedToSeries` (AR10) → `QUAL`; manual
  scoring + crash test. Decimal.
- **B2** 10m Air Pistol: same seam, **integer** scoring/aggregation; explicit
  decimal/integer separation tests.
- **B3** 50m Rifle Prone: decimal, 50-min clock, **no Final transition**.

## Phase C — reducer/replay readiness audit

Recovery-state matrix for the three disciplines (all identical unless noted).
`A` = available directly in the replayed `SessionState`; `D` = derivable
deterministically; `+` = added by Phase C; `N/A` = not applicable.

| Field | AR10 | AP10 | PRONE50 | Source |
|---|---|---|---|---|
| discipline | A | A | A | `SessionStarted.discipline` |
| athlete | A | A | A | header |
| firing point | A | A | A | header `lane`/`targetId` (empty until RMS) |
| current phase | A | A | A | `SessionState.phase` |
| prep/sighting duration | **+** | **+** | **+** | `TimerStarted(Preparation).durationMs` (Phase C) |
| prep/sighting elapsed | **+**/D | " | " | `lastEventMonoMs − timer.startedAtMonoMs` |
| prep/sighting remaining | **+**/D | " | " | `durationMs − elapsed` |
| match duration | A | A | A | `config.matchMs` + `TimerStarted(Match)` |
| match elapsed | **+**/D | " | " | anchor from `TimerStarted(Match)` |
| match remaining | **+**/D | " | " | `durationMs − elapsed` |
| sighters | A | A | A | `SessionState.sighters` |
| official shots | A | A | A | `SessionState.officials` |
| official numbering | A | A | A | `shot.shotNumber` |
| official total | A | A | A | `totalTenths` (reducer-derived) |
| coordinates | A | A | A | `shot.x/yHundredthMm` |
| integer/decimal mode | D | D | D | from discipline (AP10 integer; AR10/PRONE50 decimal) |
| next official shot | D | D | D | `officials.size() + 1` |
| completion state | A | A | A | `lifecycle` (Complete iff `MatchCompleted`) |
| incident state | A | A | A | `estIncidents` (Phase A) |
| target reassignment | A | A | A | `EstIncidentRecord` (N/A normally) |
| Jury time credit | A | A | A | `EstIncidentRecord.timeCreditMs` (N/A normally) |
| recovery phase | A | A | A | `EstIncidentRecord` flags (N/A normally) |

**Gap found & fixed (Phase C):** the qualification write-path emitted no timer
events, so the recovered `timer` state was empty and the prep duration was not
journalled (`buildRecoveredState` only anchors on finals `WindowOpened`). Fix —
the **minimum** change, reusing the existing `TimerStarted` event (no new event,
no serialization/snapshot change): `QUAL` now emits
`TimerStarted(Preparation, prepMs)` at prep start and `TimerStarted(Match,
matchMs)` at official-match start. The reducer's `TimerState`
(`durationMs`/`startedAtMonoMs`) is now authoritative; recovery computes the
**frozen remaining** = `durationMs − (lastEventMonoMs − startedAtMonoMs)` — never
the full duration. Live UI behaviour is unchanged (the QML countdown still runs
independently). Verified by 15 replay checks (crash in sighting / just after
match start / mid-match / after shot 59 / 60 / AP10+PRONE50 anchors), all
asserting frozen remaining < full and correct next-shot state.

## Not in scope (STOP conditions)
50m 3P Qualification and 25m Pistol persistence/recovery are blocked on rules
(`docs/issf-rules/50m-rifle-3p-qualification.md`, `25m-pistol.md`). 25m Rapid
Fire Pistol is excluded entirely.

## B1 — 10m Air Rifle (implemented)

- **Interception point:** `ShootingPage.onPointAddedToSeries` (the same point
  the finals branch uses — no second parallel path), gated on
  `qualDisciplineId === "AR10"`.
- **Lifecycle wiring:** `beginPreparationPhase` → `QUAL.startSession` +
  `beginPreparation` + `beginSighting`; `changedToMatchMode` →
  `beginOfficialMatch`; `changedToMatchFinish` → `completeMatch` +
  `closeSession`.
- **Projection:** a `Connections{ target: QUAL } onShotAccepted` calls the
  existing `rightPanel.addToSeries` with the shot's stashed polar display args —
  the model append happens ONLY after a durable submit.
- **Cap:** the controller enforces the caller-supplied official count (60), so a
  61st official is refused at the durable boundary (never journalled).
- **Verified:** reliability harness (qualification block, incl. 60/61 cap,
  coord preservation, interrupted-recoverable), finals regression, full app
  build, and a manual Demo AR10 session — 3 sighters (officials stayed 0/60) +
  4 officials (10.9/10.9/10.8/10.9 = 43.5, numbered 1–4, each once), journal
  `discipline=AR10`, 3 sighters + 4 officials, hash chain VALID; hard-kill →
  restart detected the session as *10m Air Rifle, 4/60, Clean*; Resume routed to
  the Phase-A "recovery not yet implemented" branch (AR10 never enters the
  finals restorer). Full AR10 UI restore = **Phase D**.

## Change history
| Date | Change | Components |
|---|---|---|
| 2026-07-19 | B0: traced qualification scoring path; specified the shared `QualificationController` seam + durability boundary. | CenterPane, ShootingPage, RightPanel (trace); src/qualification (planned) |
| 2026-07-19 | B1: 10m Air Rifle live scoring journalled through `QUAL`. Persistence ✅; recovery restore = Phase D. | QualificationController (cap), ShootingPage wiring, tst_qualification |
| 2026-07-19 | B2: 10m Air Pistol journalled through the same seam with **integer/full-ring** scoring (QML floors before submit; engine stays scoring-agnostic). Gate generalized AR10→any migrated qual discipline. Manual verified (journal scoreTenths all ×10, distinct AP10 discipline). | ShootingPage AP10 branch, tst_qualification |
| 2026-07-19 | B3: 50m Rifle Prone journalled (decimal, 50-min clock, no Final/3P). One-line gate addition (50m rifle, sub-mode 0). Manual verified (journal discipline=PRONE50, no finals/3P events, crash detected as 50m Rifle Prone). **Phase B (AR10/AP10/Prone) persistence complete; recovery restore = Phase D.** | ShootingPage gate, tst_qualification |
| 2026-07-20 | **D1: AR10 recovery.** QUAL gains scan/resume/discard + recovered-state projections (shots/remaining/phase/maxExternalId); shared QML restorer — `enterQualificationMode(config, startFresh)` (canonical entry, A vs B split) + `restoreQualificationSession` (deterministic order: configure → adopt journal → project sighters → canonical `changedToMatchMode` [QUAL re-journal suppressed via `qualRecoveryInProgress`] → project officials → rebase frozen clock → seed `qualShotSeq` past recovered externalIds). `CenterPane.polarForMm` reuses the live mm→polar transform; `restore*Remaining` rebase the phase clocks. RecoveryDialog shows Phase/Time-left (+AP10 integer format). Fixed latent `m_matchSavedFile` null-deref (fresh-start parity `saveMatch(true)` + C++ guard). Deferred-clear audit: qualification's only face clears are synchronous (`changedToSigherMode`/`changedToMatchMode`, both run BEFORE projection); the finals `Qt.callLater` clear is gated `isFinalsMatch`. | QUAL controller, ShootingPage, CenterPane, main.qml, RecoveryDialog, appsettings |
| 2026-07-20 | **D2: AP10 recovery** — dispatcher gate + AP10 resume test (integer round-trip, coords intact, next=4); manual drill verified. | main.qml, tst_qualification |
| 2026-07-20 | **D3: PRONE50 recovery** — dispatcher gate + Prone resume test (decimal, 50-min config, no finals/3P state, 60/60 completed-session edge refuses shot 61); manual drill verified. **Phase D complete for AR10/AP10/PRONE50.** | main.qml, tst_qualification |
