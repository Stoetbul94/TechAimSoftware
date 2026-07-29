# Wind Map — Implementation Specification

**Programme:** `Wind Map — Post-Session Review`
**Phase:** TRAINING LAB RELEASE 2 · **Stage 2 of 8 — implementation specification**
**Written at commit:** `fb9cdde` · **Date:** 2026-07-29
**Status:** 📋 **AWAITING REVIEW — no application code may be written until this is reviewed**

Derived from the approved decisions in
`docs/training-lab-wind-map-spec-review.md` §7. Where this document and the
review disagree, the review wins.

---

## 1. Non-goals — read first

Wind Map Release 1 **does not**:

1. Recommend sight clicks, hold-offs or aiming corrections.
2. Assert that wind caused any shot or group.
3. Operate live as a correction assistant or coaching command system.
4. Participate in any competition workflow, or affect any official result.
5. Read a weather station, anemometer or any external device.
6. Use the network. Wind is entered by the athlete, on the same machine.
7. Alter scoring, ring geometry, match timing or match phases.
8. Apply to 10 m Air Rifle or 10 m Air Pistol.
9. Pool the three 3P positions into a single conclusion.
10. Present itself as an official ISSF mode.

> If implementation later touches official range wind indicators, equipment
> requirements or competition-operation claims — **stop and obtain the official
> rule source first** (review §7 Q6).

---

## 2. Domain model

`src/training/WindMapTypes.h` — plain structs, Qt-light, no presentation.

```cpp
enum class WindSource : quint8 {     // future-proofing, Release 1 = Manual only
    Manual       = 0,
    WeatherStation = 1,              // RESERVED — not implemented
};

struct WindCondition {               // what the athlete set
    bool    calm            = false; // true => direction/speed not meaningful
    qint16  directionDegrees = 0;    // 0..359, 0 = N, clockwise. Ignored when calm
    qint32  speedHundredthMs = 0;    // AUTHORITATIVE: hundredths of m/s
    WindSource source       = WindSource::Manual;
    qint64  recordedMsSinceEpoch = 0;
    QString note;                    // optional, short
    bool    valid           = false; // false => "No wind reading recorded"
};

struct WindMapShot {                 // one shot + its IMMUTABLE snapshot
    qint32  shotId          = 0;     // monotonic within the session
    qint64  shotMsSinceEpoch = 0;
    qint8   position        = 0;     // 0=n/a(Prone), 1=Kneeling, 2=Prone, 3=Standing
    bool    sighter         = false;
    double  xMm             = 0.0;   // as delivered by the shot pipeline
    double  yMm             = 0.0;
    double  calculatedScore = 0.0;   // recorded, NEVER recomputed here
    WindCondition wind;              // snapshot taken at accept time
};
```

### 2.0 Fixed-point speed — AUTHORITATIVE (corrected in stage 3)

Speed is stored as **hundredths of a metre per second** in a `qint32`, not as
a double. The journal writer has no `double` overload: every numeric payload
field in this catalogue is an integer so replay is bit-exact and journal
hashes are stable. `2.5 m/s` stores as `250`, matching the hundredth-mm
convention already used for shot coordinates.

| Rule | Where |
|---|---|
| UI and reports display **m/s** | `speedMetresPerSecond()` |
| Journals store the **integer** | `speedHundredthMs` |
| No manual scaling in QML | conversion exists only in the two helpers below |
| NaN / infinite / negative rejected **before** conversion | `metresPerSecondToHundredths()` |
| Deterministic rounding | `std::llround` (half away from zero) |
| Maximum bounded, overflow rejected | `kMaxWindSpeedHundredthMs = 100000` (1000.00 m/s) |

```cpp
bool   metresPerSecondToHundredths(double metresPerSecond, qint32* out);
double hundredthsToMetresPerSecond(qint32 hundredths);
```

The raw hundredths value is storage. **It must never be shown to an operator.**

### 2.1 Direction convention

`0° = North`, increasing **clockwise**. The UI shows `N NE E SE S SW W NW`;
those are labels over the stored degrees, never the stored value. Sector
mapping for analysis is in §7.2.

### 2.2 The snapshot rule

When a shot is accepted, the controller **copies** the current `WindCondition`
into the shot. Later edits to the standing condition **must not** alter any
recorded shot. This is the single most important correctness property of the
programme, and §11 tests it explicitly.

### 2.3 Missing data is explicit

If no condition has been set when a shot arrives, the shot stores
`WindCondition{ valid = false }`, surfaced everywhere as
**"No wind reading recorded"**. It is never inferred, never back-filled, and
never carried forward from a neighbouring shot — including during recovery.

`calm = true` is a **recorded observation** and is not the same as
`valid = false`. Calm shots participate in analysis as their own condition.

---

## 3. Events

`src/reliability/events/EventTypes.h` + the `DomainEvent` variant, following
the existing struct shape (`kType`, `kVersion`, fields, `validate()`).

| Event | Payload | When |
|---|---|---|
| `WindMapSessionStarted` | `disciplineId`, `is3P`, `positionSequence` | Start pressed |
| `WindConditionChanged` | full `WindCondition` | Athlete sets/updates the condition |
| `WindMapSighterAccepted` | `WindMapShot` with `sighter=true` | Sighter arrives |
| `WindMapShotAccepted` | `WindMapShot` with `sighter=false` | Counted shot arrives |
| `WindMapPositionChanged` | `fromPosition`, `toPosition` | 3P only |
| `WindMapSessionCompleted` | `countedShots`, `sighterShots`, `conditionChanges` | Session ends |

**Every accepted-shot event carries the whole snapshot**, not a reference to a
condition id. A journal line must be interpretable on its own; a reference
would make a shot's wind depend on replaying earlier events correctly, which is
exactly the coupling that makes recovery fragile.

Registration: `EventRegistry.cpp` (`DurabilityClass::Sync`,
`BroadcastClass::Broadcast`), `EventSerializer.cpp` (write + read arms).

### 3.1 Journal format

One JSON object per line, in the existing envelope, e.g.:

```json
{"seq":42,"type":"WindMapShotAccepted","v":1,"ts":"...","payload":{
  "shotId":17,"shotMs":1753791234567,"position":3,"sighter":false,
  "xMm":-3.42,"yMm":1.08,"score":10.2,
  "wind":{"valid":true,"calm":false,"dirDeg":270,"speedMs":2.5,
          "source":"Manual","recordedMs":1753791180000,"note":"gusting"}}}
```

`"wind":{"valid":false}` is the explicit no-reading form.

---

## 4. Reducer state

```cpp
struct WindMapState {
    QString  disciplineId;          // "PRONE50" | "3P50"
    bool     is3P = false;
    qint8    currentPosition = 0;
    WindCondition currentWind;      // the standing condition
    QVector<WindMapShot> shots;     // sighters and counted, in arrival order
    qint32   nextShotId = 1;
    qint32   conditionChanges = 0;
    Phase    phase = Phase::Setup;
};
```

Folding rules — pure, no I/O, no Qt GUI:

| Event | Effect |
|---|---|
| `WindMapSessionStarted` | sets discipline, `is3P`, phase → `Recording` |
| `WindConditionChanged` | replaces `currentWind`; `++conditionChanges`. **Never touches `shots`.** |
| `WindMapSighterAccepted` / `WindMapShotAccepted` | appends the shot **exactly as recorded**; `++nextShotId` |
| `WindMapPositionChanged` | sets `currentPosition` |
| `WindMapSessionCompleted` | phase → `Completed` |

### 4.1 Snapshot serialization — state v4 (CORRECTED in stage 4)

Wind Map projections **are** snapshot-serialised, unlike the other Training
programmes. `ReplayEngine::replay` defaults to the snapshot fast path and
folds only the tail after the last `StateSnapshot`, so state absent from a
snapshot is lost at that boundary. The other programmes are safe only because
nothing currently emits a snapshot — an accident, not a guarantee.

State version 3 → 4 adds a `windMap` object (standing condition + session
fields) and a `windMapShots` array (each shot with **its own** immutable
snapshot). Wind Map fields are also part of `SessionState::operator==`, which
turns `ReplayEngine::snapshotsAgreeWithFold()` into a real check on the
projection. Serialization and equality must always change together.

Full audit and proof: `docs/training-lab-wind-map-recovery-audit.md`.

---

## 5. Controller phases

`src/training/WindMapController.{h,cpp}`, exposed to QML as `WINDMAP`.

```
Setup ──startWindMap()──▶ Recording ──complete()──▶ Review ──close()──▶ Completed
                              │  ▲
              setWindCondition│  │ onShotAccepted() (snapshot taken here)
                              ▼  │
                          (standing condition updated)
```

| Phase | Meaning |
|---|---|
| `Setup` | Discipline + 3P sequence chosen; nothing journaled yet |
| `Recording` | Session live; conditions set, shots snapshotted |
| `Review` | Post-session analysis on screen; journal closed to new shots |
| `Completed` | Session finished; report available |

Q_PROPERTY surface (mirroring `PositionTransitionController`):
`phase`, `active`, `inSetup`, `recording`, `reviewOpen`, `completed`,
`currentDirectionDegrees`, `currentSpeed`, `currentCalm`, `currentNote`,
`hasWindReading`, `positionName`, `countedShots`, `sighterCount`,
`conditionChanges`, `sessionId`, `sessionOperatingMode`, `lastError`,
`lastStartError`.

`Q_INVOKABLE`: `startWindMap(athlete)`, `setWindCondition(dirDeg, speedMs, calm, note)`,
`setCalm()`, `changePosition(pos)`, `complete()`, `loadRecoveredState(...)`.

**Start path.** `startWindMap()` classifies the session `kind = Training`,
`disciplineId = "WINDMAP"`, and is dispatched from `LoginPage` through a
`windMapConfirmed` gate exactly like `trainingConfirmed` / `cdConfirmed` /
`ptConfirmed`. It is **never** a qualification or Final session.

---

## 6. Recovery

Restorer `restoreWindMapSession(sessionId)` on `ShootingPage`, registered in
`main.qml::dispatchRecovery()` under `disciplineId === "WINDMAP"`, alongside
`CALLDIAG` and `POSTRANS`. Unknown discipline still fails safe — never Finals.

Restores: discipline · `is3P` · current position · **the standing wind
condition** · every shot with **its own** snapshot · sighter/counted
classification · session phase.

**Never infers.** A shot journaled with `valid=false` is restored as
`valid=false`. The restorer does not fill gaps from neighbouring shots, from
the standing condition, or from anything else. A recovered session must be
byte-identical in meaning to the one that was interrupted.

---

## 7. Analytics

`src/training/WindMapAnalytics.{h,cpp}` — **pure C++, no Qt GUI, no
presentation**. Every reported value derives here; QML only formats.

### 7.1 Inputs and filtering

Counted shots only, by default. Sighters are carried through the model and are
available to the review UI behind an explicit toggle, but **never** enter a
counted-shot statistic. Shots with `valid=false` form their own
"No wind reading recorded" bucket and are excluded from condition comparisons.

### 7.2 Bucketing

**Direction sectors** — 8 sectors of 45°, centred on the compass points:
`N = [337.5, 22.5)`, `NE = [22.5, 67.5)`, … `NW = [292.5, 337.5)`.

**Speed bands** — Release 1 fixed bands, in m/s:

| Band | Range | Boundary rule |
|---|---|---|
| Calm | `calm == true` **only** | `speed == 0` alone is NOT calm — calm must be explicitly recorded |
| Light | `0 < v ≤ 2.0` | exactly **2.0 → Light** |
| Moderate | `2.0 < v ≤ 4.0` | exactly **4.0 → Moderate** |
| Strong | `4.0 < v ≤ 7.0` | exactly **7.0 → Strong** |
| Very strong | `v > 7.0` | |

**APPROVED 2026-07-29.** Each boundary value belongs to the *lower* band, and
each is a named test case. `speed == 0` with `calm == false` is a Light-band
reading of zero, not Calm; Calm is a distinct recorded observation.

Banding is applied **at analysis time**; the journal stores the raw m/s value,
so bands can be revised later without invalidating existing sessions.

### 7.3 Formulas

For a set of shots `S` with coordinates `(xᵢ, yᵢ)` in mm:

- **Sample count** `n = |S|`
- **Mean point of impact** `x̄ = Σxᵢ/n`, `ȳ = Σyᵢ/n`
- **Group centre** = MPI (same quantity; one name in the UI)
- **Group size (extreme spread)** `max‖pᵢ − pⱼ‖` over all pairs
- **Standard dispersion** `σ = √( Σ((xᵢ−x̄)² + (yᵢ−ȳ)²) / n )`
- **Displacement vs reference** `Δx = x̄_condition − x̄_reference`,
  `Δy` likewise

**Session reference centre** = the MPI of **all counted shots in the same
position** (all conditions pooled). Displacement is always stated relative to
that, never to the target centre — the athlete's own zero is the honest
baseline for "where did these land differently".

### 7.4 Minimum sample rules

| Statistic | Minimum n | Below the minimum |
|---|---:|---|
| Sample count | 1 | always shown |
| Mean point of impact | 3 | shown, flagged "indicative only" |
| Group size / dispersion | 5 | **withheld**; "insufficient sample (n = k)" |
| Displacement vs reference | 5 | **withheld** |
| Any comparison between two conditions | 5 **each side** | **withheld** |

**Every figure is displayed with its `n`.** A statistic without its sample size
is a defect.

### 7.5 Wording

Generated observations are **descriptive**, template-driven, and must survive
the review's wording rule:

> ✅ "Shots recorded under this wind condition were grouped predominantly left
> of the session reference centre (n = 12)."
>
> ❌ "This wind pushed the shots left."

#### Scope of the wording checker — CORRECTED 2026-07-29

The checker applies **only to Wind Map generated analytic narrative**: the
conclusion, summary and "What You Should Take" strings produced by
`WindMapAnalytics`. It does **not** run over the application, UI labels,
product identity, documentation or any other report.

**Single words must not be prohibited.** "Tech Aim" contains *aim*; *click*
means a mouse button; *hold* describes timing and UI behaviour; *adjust*
appears in settings; *correct* appears in validation messages. A word-level
list produces false failures on correct code.

**Prohibited phrases** (case-insensitive, whitespace-normalised):

| Causal | Prescriptive |
|---|---|
| `the wind caused` | `you should aim` |
| `the wind pushed` | `aim to the` |
| `caused by the wind` | `hold left` · `hold right` |
| `due to the wind` | `add clicks` · `remove clicks` |
| `wind moved the` | `adjust your sights` · `move your sights` |
| `blown left` · `blown right` | `correct by` · `compensate by` |
| | `this requires a correction` |

**Permitted, and used as positive fixtures:**

- "Shots recorded under this condition grouped left of the reference centre."
- "The sample contains five counted shots."
- "This is an observed association and does not establish causation."
- "Insufficient samples are available for a reliable comparison."

§11 tests both directions: prohibited phrases are rejected, and the permitted
sentences above pass unchanged.

### 7.6 3P separation

Kneeling, Prone and Standing are analysed **independently** — separate
reference centres, separate buckets, separate observations. A combined session
overview may show per-position figures side by side, but **no statistic pools
the three**, and no observation spans them. Gated per
`docs/3p-discipline.md`.

---

## 8. UI workflow

Added through the **existing Training Lab catalogue mechanism**
(`practiceView`). **No homepage styling change** — UI-DEC-012.

| Step | Screen |
|---|---|
| 1 | Training Lab catalogue → `Wind Map — Post-Session Review` (visible only when 50 m Rifle is selected) |
| 2 | **Setup** — discipline confirmation, 3P sequence if applicable |
| 3 | **Recording HUD** — the wind control plus shot progress |
| 4 | **Review** — plot, filters, summaries, timeline, observations |
| 5 | **Report** — PDF export |

### 8.1 The wind control

Touch-friendly, ≥ 44 px targets per the design system:

- an 8-point direction selector (compass ring or 8 buttons) — sets degrees
- a speed field/stepper in m/s
- a **Calm** toggle, which disables direction and speed
- an optional short note
- a clear read-out of the **currently active** condition, since it persists

Setting the condition emits `WindConditionChanged`. It is set **before**
shooting and left alone; the athlete is never asked to re-enter an unchanged
condition.

### 8.2 Review screen

Target plot · wind-condition filter · direction-sector summary · speed-band
summary · MPI markers · sample counts on every figure · **position tabs for
3P** · session timeline · neutral written summary.

**The plot must not draw an arrow from the wind input to an assumed
correction.** It shows recorded conditions and where shots landed —
correlation, not asserted causation.

Sighters are drawn **visually distinct** and are off by default.

---

## 9. Report and PDF

Shared Report System components (`ReportHeader/Footer`, `SectionTitle`,
`MetricCard`) on white A4, exported via `grabToImage` → `CUSTOMPRINT`.
Approved Tech Aim branding.

Sections: session details · discipline · position where relevant · target
plot · wind direction summary · wind speed summary · shot counts · **sighter
separation** · position-separated 3P analysis · **data limitations** ·
neutral observations · **"What You Should Take"**.

**"What You Should Take"** summarises observed patterns in the same
descriptive register as §7.5. It may say what was observed and how large the
sample was. It may **not** give sight-click values, aiming corrections or
causal explanations.

Status stays **`GENERATED — HUMAN VISUAL CHECK REQUIRED`** until a rendered
report is inspected.

---

## 10. Files

| File | Status |
|---|---|
| `src/training/WindMapTypes.h` | new |
| `src/training/WindMapController.{h,cpp}` | new |
| `src/training/WindMapAnalytics.{h,cpp}` | new |
| `src/reliability/events/EventTypes.h` | +6 event structs |
| `src/reliability/events/DomainEvent.h` | variant extended |
| `src/reliability/events/EventRegistry.cpp` | +6 rows |
| `src/reliability/events/EventSerializer.cpp` | +6 write / +6 read arms |
| reducer + snapshot | `WindMapState` folding |
| `WindMapHud.qml`, `WindMapReportView.qml`, `WindMapRightPanel.qml` | new |
| `LoginPage.qml` | catalogue entry + `windMapConfirmed` gate — **content only** |
| `ShootingPage.qml`, `main.qml` | mode entry + restorer registration |
| `Seta.pro`, `qml.qrc` | registration |
| `tests/training/…` | new suite |

---

## 11. Test plan

### Domain and snapshot
1. A shot takes the standing condition at accept time.
2. **Changing the condition afterwards does not alter any recorded shot.**
3. A shot with no condition set stores `valid=false`, not a default.
4. `calm=true` and `valid=false` are distinguishable everywhere.
5. Direction wraps correctly at 0/359.

### Events, journal, recovery
6. All six events round-trip through the serializer byte-exactly.
7. A journal replays to the identical `WindMapState`.
8. Recovery restores position, standing condition, every shot snapshot, and
   sighter/counted classification.
9. **Recovery never infers** a missing reading.
10. An unknown discipline id does not resolve to Wind Map or to Finals.

### Analytics
11. MPI, group size, dispersion and displacement against hand-computed fixtures.
12. Sector boundaries at 22.5/67.5/…/337.5 and the N wrap.
13. Speed-band boundaries at exactly 2, 4 and 7 m/s.
14. Below-minimum samples are **withheld**, not approximated.
15. Every returned figure carries its `n`.
16. Sighters are excluded from counted statistics by default.
17. 3P: three positions never pool; each has its own reference centre.

### Wording
18. No Wind Map generated narrative contains a prohibited **phrase** (§7.5).
19. The four permitted sentences in §7.5 pass the checker unchanged — the
    checker must not reject correct neutral wording.
20. The checker is scoped to Wind Map analytic narrative only: strings
    containing "Tech Aim", "click to choose", "hold time" and similar
    application text are **not** its input and must not be flagged.
21. Every comparison string contains its sample size.

### Boundary
22. Zero counted shots — no crash, no statistics, an explicit empty state.
23. All shots in one condition — comparisons withheld, not zero-valued.

Baselines to hold: reliability **1059/0**, docs **986/0 + 204/0**, plus the
new Wind Map suite. No existing baseline may regress.

---

## 12. Open items — not blocking stage 3

1. **Speed-band boundaries** (§7.2) are a first proposal. Real session data may
   suggest different splits; bands are analysis-time, so revising them costs
   nothing.
2. **Minimum sample thresholds** (§7.4) are conservative starting values.
3. **Compass ring vs 8 buttons** for direction — a UI decision better made
   against a prototype than on paper.
4. `WindSource::WeatherStation` is reserved and unimplemented. Adding it needs
   its own approval, and would introduce a device dependency this release
   deliberately excludes.
