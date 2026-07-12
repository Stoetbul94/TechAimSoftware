# 3P FINAL — Phase-B implementation plan

**STATUS: IMPLEMENTED.** Both review gates were approved (Decision 1:
`advanceStage1()`; Decision 2: [P1] = Option B). Commits: **B1** `dccff03`
(schema, record builder, rejection reasons, advanceStage1 mechanism),
**B2+B3** `c18574d` (Stage-1 ingestion, model routing, role-union pre-lock,
finals UI totals/shot feed/incident banner, session journal), **B4** (tests +
this doc). Test suite: **91 checks, 0 failures** (`tests/finals/`).
Implementation notes vs plan: the duplicate key is the backend cumulative
detection counter (`MODREADER.getShootCount()` at receipt, strictly
increasing per firing window); the finals shot list/totals live on the finals
panel rather than a right-panel reskin (full right-panel treatment deferred
to Phase C/D polish); restart-recovery UI remains deferred (journal is
written).

## 1. Shot-ingestion data path

**Where shots enter today (qualification):** hardware/demo → C++
`TachusWidget` (coordinate lists, `shootCountChanged`) → `CenterPane` reads
`getXCord/getYCord(i)` → `calculateShootingSocre()` (the shared scoring
engine) → `pointAddedToSeries(x, y, score)` → `ShootingPage.
onPointAddedToSeries` → `rightPanel.addToSeries` → models.

**Finals interception — one branch, at the existing junction:** in
`ShootingPage.onPointAddedToSeries`, `isFinalsMatch` routes to
`FINALS3P.registerShot(xmm, ymm, decimalScore, externalShotId)` instead of
`rightPanel.addToSeries`. Everything upstream (detection, coordinates, the
scoring engine) is untouched and shared. The Phase-A demo-click shortcut in
`CenterPane` (straight to `simulateShot`) is removed so demo clicks flow
through the real scoring path.

**Controller validates, QML routes.** The controller stays structurally
model-free: it validates window state + stage limit + duplicate id and emits
`shotAccepted(shotMap)` / `shotRejected(rejectionMap)` with the complete
finals schema. A thin QML router in `ShootingPage` (bind-only, no logic):
- `onShotAccepted`: `sighter` → append to `globalSlighterModel`; match →
  append to `globalMatchModel`; both with the full schema (§2).
- `onShotRejected`: append to a dedicated `finalsIncidentModel` (and log
  file); official models and totals untouched; non-blocking warning banner.

The schema map itself is built in C++ (a small `Finals3PShotRecord` helper in
`src/finals/`) so it is unit-testable without QML.

## 2. Finals shot schema

Every role present in the **first** append (ListModel role-locking), reusing
existing role names where they exist:

| Role | Reuse/new | Notes |
|---|---|---|
| `calculatedscore` | existing | decimal score (string-formatted like qualification) |
| `score` | existing | raw radius field kept for component compatibility |
| `xmm` / `ymm` | existing | target-face mm |
| `direction` | existing | angle, as qualification |
| `timestamp` | existing | absolute wall-clock ISO time |
| `timeComsumed` | existing (sic) | **defined for finals as seconds since the current firing window opened** (§6) |
| `position` | existing | 0=K 1=P 2=S — keeps target plots/components working |
| `finalsStageId` / `finalsStageLabel` | new | FinalsStage value + name |
| `finalsShotNumber` | new | official 1–35; 0 for sighters |
| `finalsWindowId` | new | which command window accepted it |
| `finalsSeriesIndex` | new | 0 K, 1 P, 2 S1, 3 S2, 4–8 singles |
| `shotWithinStage` | new | 1-based within the stage |
| `targetModeAtReceipt` | new | 0 sighter / 1 match |
| `isSighter` | new | explicit flag |
| `isFinalsShot` | new | true for every finals append (lets shared components branch) |

`accepted` / `rejectionReason` do **not** go into the official models — those
models are accepted-only by design. Rejections live in `finalsIncidentModel`
(`reason, stageId, xmm, ymm, timestamp, externalShotId, windowId`).

## 3. Stage-1 enforcement

- Kneeling / Prone: max **10 accepted match shots** each; the 11th detection
  is rejected `StageShotLimitReached` (already enforced and tested at
  controller level; Phase B adds only the model routing).
- Standing sighting: unlimited sighters within remaining Stage-1 time.
- Sighters never consume official numbers; numbering stays continuous 1–35
  and is assigned by the controller from **accepted** shots only — raw
  detections never advance the counter.

## 4. Target-mode / position transitions ← APPROVED (Decision 1)

Legal chain (already enforced in the controller):

```
PREP_SIGHT : SIGHTER only; automatic MATCH at prep expiry (EST officer)
STAGE 1    : K_MATCH → P_SIGHT → P_MATCH → S_SIGHT (→ ready-MATCH)
             no backward transitions; illegal attempts → incident event
             + visible non-blocking warning
```

**Problem with a generic SIGHT/MATCH toggle:** "SIGHT" does not say *which*
sighting (prone vs standing). The controller currently disambiguates by
current stage — deterministic internally, but the UI intent is implicit.

**Proposed mechanism (for approval): one explicit stage-labelled ADVANCE
action instead of the toggle.** The legal chain is strictly linear, so a
single "advance" intent is always unambiguous. The finals panel shows one
primary button whose label names the specific transition:

| Current stage | Button label | Controller call |
|---|---|---|
| KneelingMatch | `CHANGE TO PRONE — SIGHTERS` | `advanceStage1()` |
| ProneSighting | `START PRONE MATCH` | `advanceStage1()` |
| ProneMatch | `CHANGE TO STANDING — SIGHTERS` | `advanceStage1()` |
| StandingSighting | `TARGET TO MATCH — READY` | `advanceStage1()` |

`advanceStage1()` becomes the public API; `setTargetMode` remains internal.
If the athlete advances **before** reaching the stage's shot limit (e.g.
leaves kneeling after 7 shots — physically allowed, costs the unfired shots
at 22:00 per [P1]), the controller emits `advanceConfirmationRequired`
(shots fired / limit) and the UI asks once: "3 kneeling shots unfired —
change position anyway?" → `confirmStage1Advance()`. Advancing at the limit
needs no confirmation. Every advance is a structured event (RMS-visible).

## 5. Score calculation

Decimal only, computed by the **same** `calculateShootingSocre()` engine as
qualification (no finals-specific maths). No integer-primary anywhere in
finals. Cumulative total = sum of `calculatedscore` over **accepted official
shots** in `globalMatchModel`, maintained as a controller-side running total
(incremented on accept) and recomputable from the model (used by tests to
cross-check). Tablet and future RMS both consume the per-shot decimal record,
so identical rules by construction.

## 6. Time-used definitions (no ambiguous reuse)

Per accepted shot: `timeComsumed` = seconds since the current firing window
opened (documented meaning for finals — qualification uses split-time
semantics); `timeSincePrevShotMs` = since the previous accepted shot in the
same window (new role if needed by reports, else derived); `timestamp` =
absolute wall clock; `stageElapsedMs` = since stage entry. All computed by
the controller from its monotonic clock.

## 7. UI integration (shooting page in finals mode)

- Target plot: existing overlay draws from the display buffer; finals clears
  it per firing window (clean face per position/series, like qualification
  positions) and plots accepted shots + sighters with the existing components.
- Right panel: stage subtotal + cumulative decimal total (no integer
  bracket), next official shot number (from controller), shot list grouped
  K / P / S1 / S2 / 31–35, sighters visually distinct (grey `S` rows),
  continuous 1–35 SN labels supplied by `finalsShotNumber` — no derived
  numbering arithmetic in QML.
- Sighting vs match display driven by `isSighter`/`targetModeAtReceipt`.

## 8. Rejection and incident handling

Reasons: `WindowClosed, WrongTargetMode, StageShotLimitReached,
DuplicateShot, InvalidShotData, FinalsNotActive` (enum extended from
Phase A's three). Rejected shots: never touch totals or official models;
append to `finalsIncidentModel` + log line + non-blocking warning banner on
the finals panel (auto-dismiss, tap to review incidents). Duplicate
detection: `externalShotId` (backend shot counter) must be strictly
increasing per window; repeats → `DuplicateShot`.

## 9. Persistence boundaries

After every accepted shot and stage transition, append one JSON line to a
session journal (`finals_session.jsonl` beside the existing save files):
shot record / stage / target mode / next official number / stage subtotals /
cumulative total / command sequence number. Append-only, crash-safe, cheap.
**Restart recovery UI is explicitly deferred** (the journal makes it possible
later without schema change); on a fresh finals start an existing journal is
archived with a timestamp — never silently reused or zeroed while old shots
remain.

## 10. [P1] decision — RESOLVED: Option B (Decision 2)

Both options keep raw models clean (no synthetic entries — `MissingShot
{ expectedShotNumber, stage, reason: TimeExpired }` records either way):

- **Option A — report incomplete:** totals include only fired shots; report
  shows "Stage incomplete — N shots not fired". Pure detected-data integrity;
  but the cumulative total then differs in *meaning* from a competition
  result (where an unfired commanded shot scores zero).
- **Option B — provisional 0.0 in the report layer:** report lists
  "Shot 24 — DNS / Not fired — 0.0 provisional" from MissingShot records;
  cumulative total is arithmetically identical (zeros add nothing) and the
  stage-incomplete flag is kept.

**Recommendation: Option B**, *subject to confirming the rule text* that an
unfired commanded final shot is scored zero. Data-integrity consequences are
contained because the 0.0 exists only in the report layer, always labelled
DNS/provisional and sourced from MissingShot records — a detected shot and a
missing shot can never be confused in the raw data, and the RMS can apply its
own rule later from the same records. Option A remains one config flip away
(`fillUnfiredShotsWithZero` stays the switch, default per your decision).

## 11. Phase-B tests (extend `tests/finals/`)

Controller + record-builder level (QML-free): 10 kneeling accepted / 11th
rejected · unlimited prone sighters · 10 prone accepted · numbering 1–20
exact after Stage 1 · sighters unnumbered · `WrongTargetMode` rejection ·
outside-window rejection · `DuplicateShot` via repeated externalShotId ·
cumulative decimal total exact (sum cross-check) · per-stage subtotals exact
· schema map contains every §2 role on the FIRST record · no synthetic zeros
at expiry · MissingShot records emitted with correct expected numbers ·
journal line written per accepted shot. App level (manual + launch checks):
model writes occur only for accepted shots; qualification modes byte-
identical (regression run of a 3P qualification match).

## 12. Commit plan

- **B1** — finals shot schema + `Finals3PShotRecord` builder + rejection-
  reason enum extension + `advanceStage1()` transition mechanism (post-review)
- **B2** — Stage-1 ingestion: `onPointAddedToSeries` branch, QML router,
  model routing, incident model, duplicate guard
- **B3** — UI totals/numbering integration + non-blocking incident banner +
  journal writes
- **B4** — Phase-B test suite + documentation update

Each commit build- and launch-verified; no Phase-C (series/singles shot
handling) work mixed in.
