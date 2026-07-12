# 50m Rifle 3P FINAL — workflow map and roadmap (design doc)

Single-athlete **Finals Training Mode** for one TechAim lane unit, implementing
the ISSF 50m Rifle 3-Positions Final (2025 edition rules, effective 07/2026)
as a training simulation. **Everything here is gated on `isFinalsMatch` and is
fully separate from the 3P qualification logic (`is3PMatch`,
`docs/3p-discipline.md`) and from every other discipline.**

Written and agreed BEFORE implementation. Decisions below were confirmed by
the user on 2026-07-12; items marked **[PROVISIONAL]** are still open.

## Scope decisions (locked)

| # | Decision |
|---|---|
| 1 | Single athlete on one unit shoots the **full 35-shot program**. No ranking, no elimination — those arrive with the future Range Management System (RMS; see below). |
| 2 | The **app plays the CRO**: auto-running phase timers with command prompts — "LOAD… START… STOP" etc. — shown on screen with **voice/beep cues**. (Under the RMS, a human range officer takes this role.) |
| 3 | Ceremony (introductions, "TAKE YOUR POSITIONS") is simulated by voice — kept simple and skippable. |
| 4 | In Stage 1 the **athlete switches the target Match↔Sighter on screen** (their responsibility, per the rules). |
| 5 | **Decimal scoring only**, start-from-zero, cumulative — no integer-primary anywhere in finals mode. |
| 6 | Penalties (green card −2 etc.) are **out of scope** until asked; they belong to the RMS rules layer. |
| 7 | Entry point: a new event card in the 50m Rifle → 3 Positions flow — **"FINAL (35)"** beside MATCH-60. |
| 8 | Finals report: simple per-stage breakdown (see Report section). |
| 9 | A **large dedicated shot-time countdown** is always visible during timed fire, with a voice counter. |
| 10 | Tie-breaking machinery: deferred entirely to the RMS phase. |
| P1 | **[PROVISIONAL]** Match shots unfired when a clock expires score **0.0**, and the app hard-blocks an 11th match shot per position in Stage 1. User was uncertain — revisit before Phase B is finalised. |

## The 35-shot program (shot accounting)

| Shots | Phase | Time authority |
|---|---|---|
| 1–10 | Kneeling match | shared 22:00 Stage-1 clock, athlete-paced |
| 11–20 | Prone match | same 22:00 clock |
| 21–25 | Standing series 1 | 250 s, on command |
| 26–30 | Standing series 2 | 250 s, on command |
| 31–35 | Standing single shots | 50 s each, on command |

Numbering is continuous 1–35. Under the real rules, eliminations happen after
shots 30, 31, 32, 33, 34 — on a single lane the athlete simply fires all 35;
the elimination points are announced (voice/text) as information only.

## State machine

```
IDLE
 └─ startFinals()
CEREMONY (skippable)                      voice: "ATHLETES TO THE LINE", intro,
 └─ 30 s hold, kneeling, no dry fire      "TAKE YOUR POSITIONS"
PREP_SIGHT (kneeling)                     "FIVE MINUTES PREPARATION AND
 ├─ 5:00 countdown, unlimited sighters     SIGHTING TIME… START"
 ├─ 4:30 → voice "30 SECONDS"
 └─ 5:00 → "STOP"; target auto-resets to MATCH (app = EST officer)
STAGE1 (22:00 countdown, athlete-paced)   "FINALISTS HAVE TWENTY-TWO MINUTES…",
 ├─ K_MATCH   shots 1–10  (block #11)      +5 s → "MATCH FIRING START"
 ├─ athlete taps SIGHT → P_SIGHT (unlimited sighters, prone)
 ├─ athlete taps MATCH → P_MATCH  shots 11–20 (block #21)
 ├─ athlete taps SIGHT → S_SIGHT (unlimited sighters, standing, time remaining)
 ├─ 17:00 → "FIVE MINUTES" · 21:30 → "THIRTY SECONDS"
 └─ 22:00 → "STOP"; unfired match shots → 0.0 [P1]; target → MATCH
SERIES(n) for n = 1,2                     30 s gap, then "FOR THE NEXT
 ├─ LOAD (5 s) → START → 250 s countdown   COMPETITION SERIES… LOAD", "START"
 ├─ 5 shots max; shots gated outside the window
 └─ STOP at 250 s or 5th shot; brief announcer note (after series 2:
    "8th and 7th would be eliminated here")
SINGLE(k) for k = 31…35                   "FOR THE NEXT COMPETITION SHOT…
 ├─ LOAD (5 s) → START → 50 s countdown    LOAD", "START"
 └─ STOP at 50 s or when the shot fires; elimination note after each
FINALS_COMPLETE                           "STOP… UNLOAD", "RESULTS ARE FINAL"
 └─ finals report available
```

Hard rules encoded in the machine:
- **Shots only register inside an open firing window** (sighting phases, Stage-1
  match fire, a running series/single-shot window). Anything outside is ignored
  with a log line — same guard pattern as the qualification 60-shot cap.
- Phase transitions in Stage 1 are athlete-driven (SIGHT/MATCH buttons); series
  and single-shot windows are clock-driven by the auto-CRO.
- Timers are countdowns owned by the finals state machine; the qualification
  match clock is not used.

## Data model

- Record shots append to `globalMatchModel` exactly as qualification does
  (score, xmm/ymm, direction, timestamps, time consumed) **plus a finals phase
  tag** per shot (K, P, S1, S2, X1…X5) — stored in the existing `position`-style
  role pattern so ListModel role-locking is respected.
- Sighters go to `globalSlighterModel` as usual; the display buffer
  (`globalModelOfData`) is cleared per phase (clean face per position/series),
  mirroring the qualification pattern that all the 3P fixes rely on.
- Cumulative decimal total = sum of `calculatedscore` over the record —
  start-from-zero by construction (fresh session).
- Every phase change / command is emitted as a discrete event (function +
  log line) so the future RMS can drive or mirror the same machine (the
  `*FromServer` hooks and the UDP `shootdata` broadcast already exist).

## Display rules

- **Decimal only** — `10.4`, total `104.7`. No integer brackets (opposite of
  qualification 3P; both gated on their own flags, so they never interact).
- Header phase stepper for finals: `PREP · KNEEL · PRONE · STAND · S1 · S2 ·
  SHOT-OFF` style progression (final naming at implementation).
- **Big countdown** — per-phase remaining time (22:00 / 250 s / 50 s) rendered
  large (placement chosen during implementation; no user preference), with
  voice/beep cues at the rule-mandated warnings and a short voice count near
  zero.
- Command banner — the current CRO prompt ("LOAD", "START", "STOP") always
  visible while relevant.
- SIGHT/MATCH toggle — visible only during Stage 1, only when legal.
- Right-panel shot list numbers 1–35 continuously; series grouping K / P / S1 /
  S2 / singles.

## Report (keep simple)

Per-stage breakdown: Kneeling 10 · Prone 10 · Series 1 (5) · Series 2 (5) ·
Singles 31–35 — each with decimal subtotal — plus cumulative total, shot-by-shot
list with per-shot time used, and the standard target plot. Reuses the Report
window/components (`ReportWindow` tab or its own `FinalsReportView`).

## Audio

Command prompts and warnings are voice cues (with beep fallback). Implementation
choice — pre-recorded clips vs Qt TextToSpeech — is a Phase D decision (depends
on what the Qt 6.5.3 MinGW kit ships). Until then, on-screen prompts + system
beeps are the functional baseline.

## Roadmap

| Phase | Deliverable | Notes |
|---|---|---|
| A | Finals state machine + timers + prompts skeleton | New event card, `isFinalsMatch`, phase stepper, countdowns, text prompts + beeps. Dry-runnable end-to-end without scoring changes. |
| B | Stage-1 shot handling | 10+10 enforcement, SIGHT/MATCH toggle, window gating, 0.0 fill at STOP **[P1 to confirm]**, decimal totals. |
| C | Standing series + single shots | LOAD/START/STOP cycle, 250 s/50 s windows, elimination-point announcements. |
| D | Finals report + audio + ceremony polish | Voice implementation decision here. |
| E | RMS integration (separate software, later) | Ranking, eliminations, tie-breaking, penalties, range-wide control. Out of scope for this app. |

One commit (or small series) per phase, each build- and launch-verified, on
branch `feature/3p-finals`.
