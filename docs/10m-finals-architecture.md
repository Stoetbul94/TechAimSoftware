# 10m Air Rifle & Air Pistol FINAL — architecture package (F0)

**Status:** F0 — rules + architecture proposal for review. **No implementation.**
Rules authority: [issf-rules/10m-finals-shared.md](issf-rules/10m-finals-shared.md)
(✅ verified against ISSF Rule Book Edition 2025, Second Print 07/2026, effective
1 July 2026). Per-discipline: [issf-rules/10m-air-rifle-final.md](issf-rules/10m-air-rifle-final.md),
[issf-rules/10m-air-pistol-final.md](issf-rules/10m-air-pistol-final.md).

This document audits the existing code, compares it to the existing 3P Final,
recommends an architecture, and proposes the event/state/scoring/command/
recovery/UI/test designs — **all conditional on the one blocking product
decision in §0.**

---

## 0. The blocking decision — single-athlete vs 8-athlete final

The verified rules describe an **8-athlete elimination final**: eight finalists
fire in parallel, ranking is computed after every shot, the lowest-ranked drops
out after shots 12/14/16/18/20/22/24, and ties at a boundary are broken by
shoot-off single shots (6.17.2). **None of that is meaningful for one athlete.**

Three facts collide:
- **The rules** assume 8 athletes with live cross-athlete ranking.
- **This app is single-lane hardware** — one target, one athlete (per
  `CLAUDE.md` and the RMS notes; a future Range Management System is where
  multi-lane/live-ranking lives).
- **The existing 3P Final is explicitly a single-athlete training mode** with
  **no eliminations at all** (35 shots, one shooter) — the precedent set for
  "finals in this app."

So "implement the 10m Final" could mean any of:

| Mode | What it is | Elimination/ranking | Feasible now? |
|---|---|---|---|
| **(A) Single-athlete training** | One finalist fires the 24-shot course of fire (2×5 + 14 singles) with the exact command timeline + decimal scoring; no opponents. | Not applicable (inert) | ✅ Yes — mirrors 3P Final |
| **(B) RMS-driven 8-athlete** | This lane is one of 8; the other 7 arrive via the existing UDP/`fromServer` hooks; the app shows live ranking + eliminations. | Real, data-driven | ⏳ Needs RMS (future) |
| **(C) Simulated-opponents demo** | The app synthesises 7 opponents so the full elimination/tie/shoot-off flow can be exercised in Demo. | Simulated | ✅ Possible now, but is new scope |

**Recommendation for the decision:** build **(A) first** — the athlete's own
24-shot final, decimal, command-driven, recoverable — reusing the reliability
layer exactly as the 3P Final does; and **design** the ranking/elimination/
tie/shoot-off model now (this document) so that **(B)** slots in when RMS exists,
with **(C)** as an optional Demo layer only if you want to showcase eliminations
before RMS.

**Everything below is written so the single-athlete core (A) is buildable
immediately, and the multi-athlete layer (B/C) is additive and gated.** Where a
section only applies to B/C it is marked **[multi-athlete]**. **This decision is
the #1 item in the Rule Gap report and blocks any elimination/ranking work.**

---

## 1. Existing software audit

Repository scan (grep across `src/`, QML, docs) for anything related to Air
Rifle/Pistol Final, 10m Final, elimination, series/single-shot stage, tie/
shoot-off, command audio, finalist ranking, decimal finals scoring,
presentation, Finals HUD, Finals result table.

**Finding: there is no 10m-final code of any kind, and no multi-athlete
concept anywhere in the reliability layer** (no `athleteId`, `finalist`,
`ranking`, or `eliminat*` symbols in `src/reliability/`). Everything "finals"
is the single-athlete 3P Final.

| Component | Location | Relation to 10m Final | Verdict |
|---|---|---|---|
| **SessionStore / journal / validator / replay / RecoveryCoordinator** | `src/reliability/` | Discipline-agnostic durable event log, hash chain, resume. | **Reusable unchanged** |
| **Reducer + SessionState** | `src/reliability/reducer/` | Single-athlete fold (shots, totals, stage). No ranking/elimination. | **Reusable after extraction** (add finals-10m state; multi-athlete needs new state — see §5) |
| **Event catalogue** (`EventTypes.h`, `DomainEvent`, registry, serializer) | `src/reliability/events/` | Covers session/prep/sighting/shot/command/timer/window/penalty/incident. No elimination/tie/shoot-off/ranking events. | **Reusable + extend** (§4) |
| **`ShotCore`** | `EventTypes.h` | Fixed-point integer tenths (`scoreTenths`), coordinates, window, sighter flag. **No `athleteId`.** | **Reusable unchanged** for single-athlete; **extend** for multi-athlete (§5) |
| **EST incident domain + Phase-E workflow** | `src/incident/`, EST events | Generic raise/decision/recovery/reassign/resume-gate. | **Reusable** (structurally covers 10m EST; finals specifics = gap, §10) |
| **`Finals3PController`** | `src/finals/` | 3P single-athlete state machine. Wrong course of fire, positions, 35 shots, no elimination. | **Conceptually useful only** (see §2) |
| **`FinalsAudioService`** | `src/finals/` | Pure `cueId → <cueid>.wav` mapping, beep fallback, no 3P coupling. | **Reusable unchanged** (service); **new audio assets** required (3P cues invalid) |
| **`FinalsReportBuilder` / `FinalsReportData`** | `src/finals/` | QtCore-only report assembler from stored state (3P stages). | **Reusable after generic extraction** (10m has different stages + eliminations) |
| **`FinalsHud.qml`** | QML | 3P finals HUD projection. | **Conceptually useful only** (10m HUD differs: no positions; ranking/elimination) |
| **`FinalsReportView.qml` / `ReportWindow.qml`** | QML | 4-page 3P finals report view. | **Conceptually useful only** (new 10m report shape) |
| **`FloatingWindow` + `WindowManager`** | QML | Window shell + registry (also used by IncidentWindow). | **Reusable unchanged** |
| **`CenterPane.qml::calculateShootingSocre()`** | QML | ISSF decimal ring geometry (rifle + pistol). | **Reusable unchanged** (decimal, max 10.9) |
| **`LoginPage.qml` discipline selection** | QML | Cascading discipline/event selector; finals gated on `isFinalsMatch`. | **Reusable after extraction** (add a 10m-final selection card) |
| **Qualification `QualificationController` / `QUAL`** | `src/qualification/` | Single-athlete qualification persistence seam. | **Unrelated** (different discipline; pattern reference only) |
| **`TechAimDialog` / `dialogManager`** | QML | Single dialog framework. | **Reusable unchanged** |

---

## 2. `Finals3PController` comparison matrix

Audited from `Finals3PController.h/.cpp`, `Finals3PTypes.h`, `Finals3PConfig.h`.
The 3P Final is a **single-athlete** controller with **3P-specific position
stages** (Kneeling/Prone/Standing), a **22:00 stage-1 clock**, **35 shots**, and
**no eliminations/ranking/ties**. Reuse must take the *reliability wiring and
patterns*, never the 3P *stages or timings*.

| Concern | Reusable? | Reason |
|---|---|---|
| Session lifecycle (`beginJournalSession`, start/abort/reset) | **Pattern — reuse** | Same SessionStore lifecycle; a new controller mirrors it. |
| Journal ownership (`m_store`, `submitEvent`) | **Yes, unchanged** | The store + submit path is discipline-agnostic. |
| Typed events (`ShotAccepted`, `CommandIssued`, `Timer*`, `Window*`) | **Yes** | The 10m final uses the same event structs (plus new ones, §4). |
| Timer infrastructure (single `QElapsedTimer`, `remaining = duration − elapsed`, pause-aware, `timeScale`) | **Pattern — reuse** | Excellent pattern; **3P duration constants are wrong** for 10m. |
| Command scheduling (`beginCommandSequence(gap→LOAD→START)`, `openCommandWindow(durationMs)`) | **Pattern — reuse** | Mechanism fits 10m series/singles; **sequence/durations differ**. |
| Audio commands (`FinalsAudioService`, `commandIssued`→cue) | **Yes (service)** | Service unchanged; **new 10m cue wavs** needed. |
| Window/stage model (`Stage` enum: Kneeling/Prone/Standing…) | **No — new** | 10m has no positions; stages are 2 series + 14 singles + shoot-off. |
| Shot acceptance (`registerShot`→`acceptShot`, window/mode/dup/limit guards, incident gate) | **Pattern — reuse** | Guard structure is ideal; the **stage-capacity rules differ** (5-shot windows / 1-shot windows). |
| Score reducer (integer tenths in the journal; `double` display totals) | **Yes (journal) / fix (display)** | Journal `scoreTenths` is fixed-point — authoritative. **Use integer-tenths accumulation** as authoritative for ranking, not the `double` (§8). |
| Ranking | **No — new [multi-athlete]** | 3P has none. New derived ranking over athlete totals. |
| Elimination | **No — new [multi-athlete]** | 3P has none. New elimination schedule (12/14/…/24). |
| Tie handling | **No — new [multi-athlete]** | 3P has none. New shoot-off single-shot flow. |
| Recovery (`loadRecoveredState`, `scanForRecovery`, `resumeFromRecovery`) | **Pattern — reuse** | Same reducer→inject model; the injected 10m state differs (§9). |
| HUD projection (Q_PROPERTYs → `FinalsHud.qml`) | **Pattern — reuse** | Property-projection style reused; **fields differ** (no position; ranking). |
| Target-face projection (decimal shot map) | **Yes** | Same decimal target face. |
| Incident gating (`EstIncidentController::officialsBlocked`) | **Yes, unchanged** | The 10m controller calls the same gate in its shot path. |
| Session completion (`MatchCompleted`, `reportRequested`) | **Pattern — reuse** | Same completion signal; 10m adds "RESULTS ARE FINAL"/medals. |

**Verdict:** a **new `Finals10mController`** that **reuses the reliability
infrastructure and the timing/command/guard patterns** of `Finals3PController`,
but **does not** inherit from it and **does not** copy its stages, positions,
or durations. The two finals share the engine, not the course of fire.

---

## 3. Proposed architecture — recommendation

Options considered:
- **A. One `Finals10mController` with discipline configuration** (rifle/pistol
  as a config value).
- **B. Separate `AirRifleFinalController` + `AirPistolFinalController`.**
- **C. A generic Finals engine + thin discipline adapters** (unifying 3P + 10m).

### Recommendation: **A — one `Finals10mController`, parameterised by a `Finals10mConfig{discipline}`.**

**This is rule-justified, not a file-count convenience.** The verified rules
(6.17.2) govern rifle and pistol with the **same text**; the comparison matrix
in [10m-finals-shared.md](issf-rules/10m-finals-shared.md#comparison-matrix--air-rifle-vs-air-pistol-verified)
shows the **only** course-of-fire difference is the post-"TAKE YOUR POSITIONS"
hold (**30 s vs 10 s**) plus equipment/dress metadata. Two controllers (B) would
duplicate the entire 24-shot state machine to vary one constant — that is the
*wrong* kind of sharing to avoid, i.e. it would create divergence risk for
identical rules. C (unifying with 3P) is **rejected for now**: the 3P Final
already works (189 tests), and re-basing it into a generic engine is
unjustified scope/risk; a future extraction remains possible if a third finals
format appears.

**Why not fold into `Finals3PController`:** explicitly forbidden by the mandate
and wrong on the merits (different course of fire, no positions, elimination
model). New controller.

### How the recommendation addresses each required concern

- **Source of truth:** the **journal** (SessionStore) is authoritative; the
  reducer folds it into `Finals10mState`. The controller's in-memory state is a
  live mirror, never the record — identical discipline to 3P/qualification.
- **Typed event ownership:** `Finals10mController` submits typed events to its
  own `SessionStore`; new event types (§4) live in the shared
  `src/reliability/events/` catalogue (discipline-agnostic), not in the finals
  folder.
- **Reducer state:** a new `Finals10mState` sub-structure in `SessionState`
  (single-athlete fields now; a `finalists[]` vector gated for multi-athlete).
- **Fixed-point score storage:** authoritative score = **sum of integer
  `scoreTenths`** (already how the journal stores shots). `double` only for
  display. Ranking/tie comparisons operate on integer tenths (§8).
- **Stage/window representation:** a 10m-specific `Stage`/`Window` model
  (§5) — prep+sighting, series 1, series 2, singles 1..14, shoot-off — with the
  existing `WindowOpened/WindowClosed` events marking firing windows.
- **Timing:** one monotonic pause-aware clock (the 3P pattern), with 10m
  durations (250 s series, 50 s singles, 5 s load, 5:00 prep+sighting) in
  `Finals10mConfig`; `timeScale` for accelerated dry runs.
- **Command/audio scheduling:** the 3P `beginCommandSequence`/`openCommandWindow`
  pattern with the 10m command timeline (§7); `FinalsAudioService` unchanged,
  new cue wavs.
- **Ranking / elimination / tie handling:** **[multi-athlete]** derived from
  athlete totals; elimination schedule + shoot-off events (§4/§8). Inert in
  single-athlete mode (A).
- **Recovery:** reducer→inject (the 3P `loadRecoveredState` pattern); crash-point
  table in §9; incident-aware via the existing Phase-E gate.
- **Incident gating:** `registerShot` calls `EstIncidentController::officialsBlocked`
  exactly as `Finals3PController` and `QualificationController` do.
- **QML projection:** Q_PROPERTY projections → a new `Finals10mHud.qml`
  (single-athlete now; ranking/elimination panel gated). Reuses `FloatingWindow`,
  dialog framework, decimal target face.
- **Testability:** a standalone console harness like `tests/finals/` (the 3P
  harness is the template), plus reliability-harness replay tests (§12).
- **Future rule-version changes:** all rule constants (counts, durations,
  elimination schedule, hold delays) live in `Finals10mConfig` +
  `docs/issf-rules/`; a rule edition change updates the config + the rules doc +
  the tests together (the project's existing discipline).

---

## 4. Event model

**First: can existing events represent each 10m-final need?** (Existing
catalogue: `EventTypes.h`.)

| Need | Existing event | Sufficient? |
|---|---|---|
| Final started | `SessionStarted{discipline}` | ✅ Yes (new discipline id) |
| Preparation started | `PreparationStarted` | ✅ Yes |
| Sighting started | `SightingStarted` | ✅ Yes |
| Match stage entered | `StageEntered` / `OfficialMatchStarted` | ✅ Yes |
| Series opened / closed | `WindowOpened` / `WindowClosed` (a series is a firing window) | ✅ Yes |
| Single-shot window opened / closed | `WindowOpened` / `WindowClosed` | ✅ Yes |
| Shot accepted | `ShotAccepted` / `SighterAccepted` (integer tenths) | ✅ Yes |
| Command issued | `CommandIssued{commandType,text,audioCueId,timing}` | ✅ Yes |
| Timer started / paused / resumed | `TimerStarted` / `TimerPaused` / `TimerResumed` | ✅ Yes |
| EST incident actions | Phase-A/E EST events | ✅ Yes (specifics = gap §10) |
| Final completed | `MatchCompleted{totalTenths,officialCount}` | ✅ Yes |
| Penalty (late report, dry-fire, after-STOP, extra shot) | `PenaltyIssued{deltaTenths,rule}` | ✅ Yes |
| Shot nullified (extra/after-STOP/inadvertent) | `ShotInvalidated{targetSeq,reason}` | ✅ Yes |
| Missing / unexpected-zero shot | `MissingShotRecorded` + EST `6.17.1.8` flow | ✅ Yes |
| Ranking calculated | *(derived — pure function of accepted shots)* | ✅ No event needed |
| Athlete eliminated | *(derived at 12/14/… **except** after a shoot-off)* | ⚠️ Mostly derived; needs a record when a shoot-off decides it |
| Tie detected | *(derived from equal totals at a boundary)* | ✅ No event needed |
| Shoot-off started | — | ❌ **New** (a shoot-off window is not an ordinary series/single) |
| Shoot-off shot accepted | `ShotAccepted` with a shoot-off marker | ⚠️ Needs to be **excluded from the cumulative total** |
| Shoot-off resolved / elimination order | — | ❌ **New** (records the tie outcome authoritatively) |

**Proposed NEW events (minimum — [multi-athlete], only for modes B/C):**

Only **two** new event types are proposed, both discipline-agnostic and placed
after every existing type so variant indexes never move (the project's
append-only discipline):

1. **`FinalsShootOffOpened`**
   - *Why existing is insufficient:* a shoot-off is a tie-break firing window
     whose shots must **not** add to the cumulative match total and which
     applies only to a named subset of athletes at a specific elimination
     boundary. `WindowOpened` cannot carry the boundary/place or the athlete
     subset, and a shoot-off shot scored via ordinary `ShotAccepted` would
     corrupt the 24-shot total.
   - *Payload:* `qint16 atShotNumber` (boundary: 12/14/…/24), `qint8
     decidingPlace` (e.g. 8..1), `qint16 windowId`, athlete-subset reference
     (**[multi-athlete]**: list of finalist ids; single-athlete = self).
   - *Reducer transition:* opens a shoot-off sub-window; subsequent
     shoot-off shots are tallied **separately** from the match total.
   - *Replay:* required — determines that shots in this window are tie-break,
     not match, shots.
   - *Snapshot:* adds a small shoot-off sub-state; bump `Finals10mState`
     version (new discipline state, so no impact on existing 3P/qual snapshots).
   - *Compatibility:* additive; no existing event or index changes.

2. **`FinalsPlacementDecided`**
   - *Why existing is insufficient:* elimination is normally derivable, but when
     a **shoot-off** breaks a boundary tie the decision depends on tie-break
     shots that must be recorded as the authoritative resolution (who took which
     place). `MatchCompleted` only carries a single total; there is no event
     that fixes "place P went to athlete X, decided at shot N (by shoot-off?)."
   - *Payload:* `qint8 place` (1..8), `qint16 atShotNumber`, `bool byShootOff`,
     athlete reference (**[multi-athlete]**), `qint32 totalTenths`.
   - *Reducer transition:* records the placement; marks the athlete eliminated
     (or medalled) authoritatively.
   - *Replay:* required for deterministic final standings.
   - *Snapshot:* part of `Finals10mState`; versioned with that state only.
   - *Compatibility:* additive.

**Shoot-off shot accepted:** reuse `ShotAccepted` but distinguish it via its
`windowId` (the shoot-off window from `FinalsShootOffOpened`) and a reserved
`seriesIndex`/`stageId` for shoot-off, so the reducer keeps it out of the 24-shot
cumulative total. **No third new event** — this avoids duplicate shot types.

**Single-athlete mode (A) needs ZERO new events** — it is fully expressible with
the existing catalogue (`SessionStarted`, `PreparationStarted`,
`SightingStarted`, `WindowOpened/Closed`, `ShotAccepted/SighterAccepted`,
`CommandIssued`, `Timer*`, `PenaltyIssued`, `ShotInvalidated`, `MatchCompleted`,
`StateSnapshot`, plus the EST events). The two new events are required **only**
when eliminations/shoot-offs are actually run (B/C).

**No duplicate rifle/pistol event types** — discipline is a field on
`SessionStarted`, never a type suffix.

---

## 5. State model — `Finals10mState`

Only **authoritative** state; everything else is derived or display-only.

**Persistent authoritative (folded from the journal):**
- `discipline` (`ARF10` / `APF10`)
- `stage` (enum, §6) and `windowId`
- `currentSeries` (1..2) / `currentSingle` (1..14) / shoot-off flag
- accepted shots (already in the reducer's shot list — integer tenths,
  window/stage/sighter, coordinates)
- per-athlete cumulative **integer tenths** total (single-athlete: one value;
  **[multi-athlete]**: `finalists[8]` each with id, bib/start position R1..R2,
  running tenths, `eliminated` place or `active`)
- active command window (open/closed, `windowId`, opened-at monotonic)
- timer state (`TimerStarted/Paused/Resumed` fold — remaining pause-aware)
- **[multi-athlete]** elimination log (`FinalsPlacementDecided` folds) +
  shoot-off sub-state (`FinalsShootOffOpened` folds)
- completion status (`MatchCompleted` / "RESULTS ARE FINAL")
- incident state (existing `EstIncidentRecord`, unchanged)
- recovery markers (`RecoveryStarted/Completed`, existing)

**Deterministically derived (never stored twice):**
- **ranking** — sort of athlete totals (descending integer tenths); recomputed
  on demand. **[multi-athlete]**
- **next elimination boundary** — from shot count (12/14/…/24).
- **tie at boundary** — equal totals among the bottom-ranked; triggers shoot-off.
- remaining time — `duration − elapsedRunning(pause-aware)` (the 3P formula).
- next shot number / shots-in-stage — from the accepted-shot fold.

**QML display-only (never authoritative):**
- formatted clock string, badge colours, HUD labels, per-shot polar overrides,
  announcer text, elimination banners.

**Do not store the same score or ranking twice.** The single authoritative score
is the integer-tenths sum in the reducer; ranking is always derived from it. The
controller's live `double` mirror (if kept for convenience) is display-only and
must be reconciled against the integer sum, exactly as the 3P plan requires.

---

## 6. Stage / window model (10m-specific)

Proposed `Finals10m::Stage` (replaces 3P positions entirely):

```
Idle → Presentation → PrepAndSighting → Series1 → Series2
     → Single1 → Single2 → … → Single14 → Complete
                    (ShootOff sub-window can interpose at a boundary)
     Aborted
```

- `PrepAndSighting`: 5:00 combined, unlimited sighters, `SightingOpen` window.
- `Series1`/`Series2`: `MatchOpen` window, 5-shot capacity, 250 s.
- `Single1..14`: `MatchOpen` window, 1-shot capacity, 50 s. Eliminations
  evaluated after Single2 (shot 12), Single4 (14), … Single14 (24)
  **[multi-athlete]**.
- `ShootOff`: a `FinalsShootOffOpened` sub-window at a boundary tie; 1-shot
  capacity, normal single-shot timing; shots excluded from the match total.

`seriesIndex` schema (proposed, LOCKED once shipped — mirrors the 3P discipline
of a locked index): `0 = Series1, 1 = Series2, 2..15 = Single1..14, −1 =
sighting, −2 = shoot-off`. (This is a **new** schema for the 10m discipline and
does **not** touch the 3P `finalsSeriesIndex` schema, which stays locked at
`0=K,1=P,2=S1,3=S2,4-8=singles`.)

---

## 7. Command & audio design

**Command timeline (per stage).** Timings are the rulebook **guidelines**
(6.17.2); the authoritative detailed timings are the ISSF HQ "Commands and
Announcements for Finals" doc (**not obtained — gap §10**). All audio assets are
**new** (3P cues are invalid for a 10m final). No audio files are created in F0.

| Command id | Displayed / spoken wording | Source | Lead-in | Active window | Warning | STOP | Asset exists? |
|---|---|---|---|---|---|---|---|
| `AthletesToLine` | "ATHLETES TO THE LINE" | 6.17.1.14.c / 6.17.2 e | ~10 min before start | — | — | — | ❌ new / TTS |
| `Presentation` (announcer) | per-athlete intro + CRO/Jury intro | 6.17.1.12 | — | — | — | — | ❌ new / announcer |
| `TakeYourPositions` | "TAKE YOUR POSITIONS" | 6.17.2 e | after intros | — | — | — | ❌ new / TTS |
| *(hold)* | — | 6.17.2 e | **30 s rifle / 10 s pistol** | — | — | — | n/a |
| `PrepSightingStart` | "FIVE (5) MINUTES PREPARATION AND SIGHTING TIME… START" | 6.17.2 e | after hold | 5:00, unlimited sighters | `ThirtySeconds` @ 4:30 | `Stop`+`Unload` @ 5:00 | ❌ new / TTS |
| `ThirtySeconds` | "30 SECONDS" | 6.17.2 e | 30 s before end | — | — | — | ❌ new / TTS |
| `StopUnload` | "STOP… UNLOAD" | 6.17.2 e | at 5:00 | — | — | — | ❌ new / TTS |
| *(gap)* | — | 6.17.2 e | 60 s | — | — | — | n/a |
| `LoadSeries` | "FOR THE FIRST/NEXT COMPETITION SERIES… LOAD" | 6.17.2 e | after gap | — | — | — | ❌ new / TTS |
| `StartSeries` | "START" | 6.17.2 e | **5 s** after LOAD | **250 s**, 5 shots | — | `Stop` @ 250 s or all-fired | ❌ new / TTS |
| `StopSeries` | "STOP" | 6.17.2 e | 250 s / all-fired | — | announcer 15–20 s | — | ❌ new / TTS |
| `LoadSingle` | "FOR THE NEXT COMPETITION SHOT, LOAD" | 6.17.2 f | after announcer | — | — | — | ❌ new / TTS |
| `StartSingle` | "START" | 6.17.2 f | **5 s** after LOAD | **50 s**, 1 shot | — | `Stop` @ 50 s or fired | ❌ new / TTS |
| `StopSingle` | "STOP" | 6.17.2 f | 50 s / fired | — | announcer | — | ❌ new / TTS |
| `EliminationNotice` (announcer) | "8th place eliminated…" | 6.17.2 g | after boundary | — | — | — | ❌ new / announcer |
| `ShootOffNames` | "(family names)… tie-breaking shot" | 6.17.2 h | at boundary tie | 50 s, 1 shot | — | `Stop` | ❌ new / announcer |
| `FinalStopUnload` | "STOP… UNLOAD" (after shot 24) | 6.17.2 g/i | after 24th | — | — | — | ❌ new / TTS |
| `ResultsAreFinal` | "RESULTS ARE FINAL" | 6.17.2 i | after verification | — | — | — | ❌ new / TTS |
| `MedalPresentation` (announcer) | bronze → silver → gold | 6.17.1.14.q | after results | — | — | — | ❌ new / announcer |

Every command is journalled via `CommandIssued{commandType,text,audioCueId,
issuedAtMs,effectiveAtMs}` (existing). `FinalsAudioService` maps `audioCueId`
→ `<cueid>.wav`, beep fallback — **service reusable unchanged**; a new
`audio/finals10m/` asset set (or TTS) is required.

---

## 8. Scoring & ranking design

- **Score representation:** **fixed-point integer tenths** — the journal's
  `ShotCore.scoreTenths` (0..109; controller admits ≤110 defensively). This is
  already the authoritative form; the 10m final reuses it directly.
- **Decimal precision:** tenths of a ring, max **10.9** (6.4). **Both** AR and
  AP finals are decimal (6.17.2 c). **No floating point is authoritative.**
- **Total computation:** cumulative **sum of `scoreTenths`** over accepted MATCH
  shots (sighters and shoot-off shots excluded). Display total = `sumTenths /
  10.0` formatted to one decimal.
- **Ranking order:** descending integer-tenths total; recomputed after each
  accepted shot **[multi-athlete]**.
- **Elimination boundary:** after shots 12/14/16/18/20/22/24 (6.17.2 g); the
  lowest-ranked active athlete is placed.
- **Tie detection:** equal integer-tenths totals among the athletes at the
  elimination boundary. **Because comparison is on integers, ties are exact**
  (no float epsilon ambiguity) — a design reason to keep integer tenths
  authoritative.
- **Shoot-off ranking:** on a boundary tie, `FinalsShootOffOpened` starts a
  1-shot tie-break for the tied athletes; the higher tie-break `scoreTenths`
  ranks higher; repeat until broken (6.17.2 h). Shoot-off shots **do not** join
  the cumulative total; the resolution is recorded by `FinalsPlacementDecided`.
- **Zero / invalid shot handling:** miss = 0 tenths; after-LOAD-before-START = 0
  (miss, 6.17.1.14.k); after-STOP = nullified via `ShotInvalidated` + 2-pt
  `PenaltyIssued`; extra shot = nullified + 2-pt penalty; no total below zero
  (6.17.1.5).
- **Qualification score excluded:** yes — final starts from zero (6.17.1.5); the
  qualification session is a separate journal and is never summed in.

---

## 9. Recovery requirements

Reuse the reducer→inject recovery model (`loadRecoveredState`) and the
incident-aware Phase-E gate. For each crash point: authoritative replay state,
timer behaviour, command-window behaviour, whether a command must be repeated,
window resume/cancel/re-authorise, Jury involvement. Entries marked
**[gap]** depend on the ISSF HQ command-timing doc or a product decision and
must **not** invent restart behaviour.

| Crash point | Authoritative replay state | Timer | Command window | Repeat command? | Window resume/cancel | Jury |
|---|---|---|---|---|---|---|
| Before preparation | `SessionStarted` only | not started | none | re-issue prep start | n/a | no |
| During preparation | prep started, elapsed folded | resume remaining (pause-aware) | `SightingOpen` | no (sighting continues) | resume | no |
| During sighting | same as prep (combined) | resume remaining | `SightingOpen` | no | resume | no |
| After sighting | prep/sighting complete | at 0 / gap | closed | re-issue first series LOAD | n/a | no |
| Before first series | prep done | gap | closed | re-issue LOAD/START | n/a | no |
| During a series | shots so far folded; `WindowOpened` seq | resume remaining 250 s | `MatchOpen`, capacity 5 − fired | **[gap]** likely resume (rulebook has no crash rule) | resume remaining | maybe **[gap]** |
| Immediately after a series | series shots folded, `WindowClosed` | at 0 | closed | re-issue next LOAD | n/a | no |
| Between series | series 1 folded | gap | closed | re-issue series-2 LOAD | n/a | no |
| Before first single | 2 series folded | gap | closed | re-issue single-1 LOAD | n/a | no |
| During single-shot window | fired-or-not folded; `WindowOpened` | resume remaining 50 s | `MatchOpen`, capacity 1 − fired | **[gap]** resume | resume | maybe **[gap]** |
| After accepted single | shot folded, `WindowClosed` | at 0 | closed | re-issue next LOAD | n/a | no |
| Immediately before elimination | shots to boundary folded | at 0 | closed | evaluate elimination on resume | n/a | **[multi-athlete]** |
| Immediately after elimination | `FinalsPlacementDecided` folded | at 0 | closed | continue to next LOAD | n/a | no |
| During a tie (pre-shoot-off) | boundary tie derivable from totals | at 0 | closed | re-open shoot-off | n/a | yes (announce names) **[multi-athlete]** |
| During a shoot-off | `FinalsShootOffOpened` + shoot-off shots folded | resume remaining | shoot-off window | **[gap]** resume | resume | yes **[multi-athlete]** |
| After medal result, before clean close | `FinalsPlacementDecided`(1..3) folded | stopped | closed | re-issue "RESULTS ARE FINAL" only if not journalled | n/a | no |
| During an unresolved EST incident | `EstIncidentRecord` open; officials blocked | frozen (+credit) | closed | Phase-E workflow resumes; officials gated | per Phase-E | **yes** (existing) |

**Do not invent restart behaviour** where the rulebook is silent (the
**[gap]** rows): the safe provisional default is "resume the window from its
remaining time and let the CRO/Jury re-issue or adjust," but this must be
confirmed against the ISSF HQ command doc before implementation.

---

## 10. UI concept (describe only — no implementation)

Reuses `FloatingWindow`, `WindowManager`, the dialog framework, and the decimal
target face. **Shared rifle/pistol** unless noted; nothing here redesigns
qualification/training screens.

- **Final selection card** (LoginPage) — a new "10m Air Rifle Final / 10m Air
  Pistol Final" card; **shared** layout, discipline label differs.
- **Discipline label** — "10M AIR RIFLE — FINAL" / "10M AIR PISTOL — FINAL".
- **Finalists table** **[multi-athlete]** — 8 rows (bib/start R1..R2, name,
  nation, running total, rank, eliminated/active). Hidden/degenerate in
  single-athlete mode.
- **Current stage** — Prep+Sighting / Series 1–2 / Single n/14 / Shoot-off /
  Complete. **Shared.**
- **Current command** — large command banner + countdown (the 3P HUD style).
  **Shared.**
- **Timer** — series 250 s / single 50 s / prep 5:00, pause-aware. **Shared.**
- **Live ranking** **[multi-athlete]** — sorted totals, updates per shot.
- **Elimination indicator** **[multi-athlete]** — banner "8th place eliminated"
  at each boundary; strike-through eliminated rows.
- **Tie / shoot-off state** **[multi-athlete]** — highlight tied athletes,
  shoot-off sub-panel.
- **Target face** — decimal shot map (rifle/pistol geometry). **Shared.**
- **Athlete score** — last shot (decimal) + cumulative. **Shared.**
- **Jury / Range Officer controls** — command advance, malfunction (1 min),
  incident (existing IncidentWindow), protest/penalty entry. **Shared.**
- **Incident warning** — the existing Phase-E unresolved-incident banner.
  **Shared, reused unchanged.**
- **Results state** — "RESULTS ARE FINAL", medallists (bronze/silver/gold).
  **Shared.**

**Discipline-specific UI:** only the discipline label/title and (if presentation
is modelled) the rifle dress/`walking` vs pistol details. Everything functional
is **shared**.

---

## 11. Test plan (future — not built in F0)

**Automated (a new `tests/finals10m/` console harness, `tests/finals/` as the
template, plus reliability-harness replay tests):**
- Full Rifle Final (24 shots, decimal totals, completion).
- Full Pistol Final (24 shots, **decimal** — regression against the AP-integer
  trap).
- All opening series (2×5, 250 s window capacity + expiry).
- All single shots (14×1, 50 s).
- Every elimination boundary (12/14/16/18/20/22/24) placing the correct athlete.
  **[multi-athlete]**
- Tie at **every** possible elimination boundary → shoot-off resolves it.
  **[multi-athlete]**
- Shoot-off resolution (single tie, multi-way tie, repeated shoot-off).
  **[multi-athlete]**
- Invalid/late shots: after-LOAD-before-START = miss; after-STOP = nullified +
  2-pt; extra shot nullified + 2-pt; before-prep-START = DQ path.
- Score precision: integer-tenths sums, no float drift; max 10.9.
- Ranking determinism: identical journals → identical standings.
- Crash/replay at **every** stage in the §9 table (fold == live state).
- Timer continuity: pause-aware remaining across crash.
- Command idempotency: replaying `CommandIssued` never double-advances.
- Incident blocking: officials refused while an EST incident is unresolved.
- Snapshot compatibility: `Finals10mState` v1 loads; older 3P/qual snapshots
  unaffected.
- Journal hash-chain validation on every produced journal.

**Manual (Demo mode, computer-use drive):**
- Full Rifle Final in Demo.
- Full Pistol Final in Demo (verify decimal totals).
- At least one tie/shoot-off **[multi-athlete or simulated (C)]**.
- Hard-kill during a series → recover → continue.
- Hard-kill during singles → recover → continue.
- Recovery after an elimination **[multi-athlete]**.
- EST incident workflow during a final (Phase-E regression in finals context).
- **Full FINAL3P regression** (the 189-check harness + a manual 3P run) to prove
  the new controller did not disturb the existing final.

---

## 12. RULES REQUIRING USER APPROVAL OR OFFICIAL CONFIRMATION

> Uncertainty is listed explicitly here, not hidden in prose.

### GAP 1 — Single-athlete vs 8-athlete final scope **(BLOCKING)**
- **Question:** Is the 10m Final a single-athlete training simulation (like the
  3P Final), an RMS-driven 8-athlete final, or a simulated-opponents Demo?
- **Discipline:** both (AR & AP). **Stage:** entire competition structure.
- **Impact:** determines whether ranking, elimination, ties, shoot-offs, the
  finalists table, and the two new events (`FinalsShootOffOpened`,
  `FinalsPlacementDecided`) are built now or deferred to RMS.
- **Provisional safe behaviour:** build the single-athlete 24-shot core (A)
  first; design multi-athlete as an additive gated layer.
- **Blocked?** **Yes** — all elimination/ranking/shoot-off work is blocked until
  this is decided.

### GAP 2 — Detailed command timings (ISSF HQ document)
- **Question:** What are the authoritative lead-in/gap/warning timings? The
  rulebook (6.17.2 NOTE) states its timings are *guidelines* and points to the
  separate ISSF HQ "Commands and Announcements for Finals" document, not
  obtained in F0.
- **Discipline:** both. **Stage:** every commanded window.
- **Impact:** exact countdown lead-ins, the 60 s inter-stage gap, announcer
  windows, and audio cue timing.
- **Provisional safe behaviour:** use the rulebook guideline values (5 s LOAD→
  START, 250 s/50 s windows, 60 s gap, 30 s prep warning) as config defaults,
  clearly flagged as guidelines.
- **Blocked?** Partially — the single-athlete core can use guideline timings;
  a competition-accurate build needs the HQ document.

### GAP 3 — Finals-specific EST/malfunction handling vs the generic Phase-E workflow
- **Question:** Should the finals-specific rules — the fixed **2-minute**
  prep/sighting restart (6.17.1.8), **one allowable malfunction per final**
  (6.17.1.6), the **repeat-shot-in-lieu** semantics, and the **+60 s**
  incorrect-command rule (6.17.1.14.p) — be special-cased in the finals
  controller, or should the generic EST workflow be extended?
- **Discipline:** both. **Stage:** any interruption.
- **Impact:** how much of the 10m EST/malfunction procedure is finals-specific
  code vs the shared Phase-E workflow.
- **Provisional safe behaviour:** reuse the generic Phase-E workflow for
  range-caused EST incidents (it structurally covers reserve-target + prep
  restart + resume gating); add a thin finals-specific malfunction path
  (1-min repair, one-per-final, repeat shot) in the controller.
- **Blocked?** No for the core; the malfunction/EST specifics are a design
  decision to confirm before implementing them.

### GAP 4 — "Live aiming" (2026 rule change)
- **Question:** The 2026 rulebook change announcement lists **"live aiming"**
  added for rifle and pistol; its precise procedural effect within 6.17.2 was
  not isolated during F0. Does it change any finals command/timing/display?
- **Discipline:** both. **Stage:** unclear (possibly presentation/aiming).
- **Impact:** possibly a display/broadcast feature; possibly none for scoring.
- **Provisional safe behaviour:** ignore for scoring/timing; revisit if the HQ
  command document or a rule read shows a procedural effect.
- **Blocked?** No — does not affect the core course of fire as verified.

### GAP 5 — Malfunction/late-report penalty representation
- **Question:** Confirm the app should auto-apply the fixed penalties (2-pt late
  report, 1-pt dry fire, 2-pt after-STOP/extra shot, DQ) or record them as
  Jury-authorised `PenaltyIssued` actions (authority model).
- **Discipline:** both. **Stage:** various.
- **Impact:** whether penalties are automatic or operator-authorised (consistent
  with the Phase-E authority model that forbids the app independently applying
  allowances).
- **Provisional safe behaviour:** surface/recommend the penalty, apply it via an
  authorised `PenaltyIssued` action (consistent with Phase E), never silently.
- **Blocked?** No — a design decision, not a rule gap.

---

## Appendix — sources used

- **ISSF Official Statutes, Rules and Regulations — Edition 2025 (Second Print
  07/2026), Effective 1 July 2026** — official ISSF backoffice PDF
  (`ISSF-Rule-Book-2026-Edition-2025-Second-Print-07-2026-Effective-1-July-2026.pdf`),
  text-extracted and read section-by-section: rules 6.17.1, 6.17.1.1–6.17.1.14,
  **6.17.2** (pp. 265–272), 6.13.2 (p.259), 6.15.5, 6.4. Access date 2026-07-21.
- **ISSF news — "ISSF announces changes with 2026 Rulebook"** (effective date
  1 Jan 2026; ten Olympic finals reformatted; live aiming added) — used only to
  establish the **current edition** and the historic-format warning; not cited
  as a rule source.
- Repository source (audited, not modified): `src/finals/*`,
  `src/reliability/events/EventTypes.h`, `src/reliability/reducer/*`,
  `src/incident/*`.
