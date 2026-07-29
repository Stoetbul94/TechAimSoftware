# Training Lab Release 2 — Wind Map · Specification Review

**Phase:** TRAINING LAB RELEASE 2 — WIND MAP · **Stage 1 of 8: specification review**
**Reviewed at commit:** `98b7175` · **Date:** 2026-07-29
**Status:** ✅ **APPROVED — all specification questions answered 2026-07-29**

> Stage 1 is complete. The approved decisions are recorded in §7. The
> implementation specification produced from them is
> `docs/training-lab-wind-map-implementation-spec.md`.
> **No application code has been written**; implementation is gated on review
> of that specification.

---

## 1. What the review found

### 1.1 There is no wind rule content anywhere

`docs/issf-rules/` contains eleven files. **None mentions wind.** Every match
for "wind" in the repository is `window` — command windows, `IncidentWindow`,
scroll containers. Verified across `docs/`, `src/` and all QML.

There is also **no Wind Map prior art**: no controller, no types header, no
events, no QML, no tests.

`CLAUDE.md` requires that when the applicable rules file is incomplete or
absent, work **stops and requests the missing official rule** before changing
discipline behaviour. Two things follow, and they pull in different directions:

- Wind Map is a **Training Lab programme**, not a competition discipline. It
  does not change scoring, match phases, timing or results, so the strict
  discipline gate is arguably not engaged.
- But wind is only meaningful at **50 m outdoors**, where ISSF has rules about
  wind indicators on the range, and a training tool that misrepresents them
  would be worse than no tool.

**Recommendation:** treat this as needing Arnold's product decision rather
than an ISSF rule lookup — with one exception, §7 Q6.

### 1.2 The programme pattern is well established

Three Training Lab programmes exist and share a consistent shape. A fourth
should follow it exactly rather than invent anything:

| Concern | Existing pattern |
|---|---|
| Types | `src/training/<X>Types.h` — plain structs, Qt-free where possible |
| Controller | `src/training/<X>Controller.{h,cpp}` — owns the state machine, exposed to QML as a context property |
| Analytics | `src/training/<X>Analytics.{h,cpp}` — pure computation, no presentation |
| Events | `src/reliability/events/DomainEvent.h` + `EventTypes.h` — one struct per event, appended to the variant |
| Recovery | a restorer registered with the discipline recovery dispatcher in `main.qml` |
| Gate | a `<x>Confirmed` flag on `LoginPage`, cleared by every event/discipline selector |
| Report | a `<X>ReportView.qml` on the shared Report System components |
| Tests | the training harness |

`PositionTransitionController` (T4) is the newest and the closest model: it is
3P-only, has a setup view, a HUD, a review step and a PDF.

Existing training events, for reference:
`TrainingBlockStarted/Completed`, `CallDiagnoseSessionStarted/Started/
ShotReceived/CallRecorded/NoteSaved/Completed`,
`PositionTransitionSessionStarted/…/Completed`.

### 1.3 The homepage is closed

UI-DEC-012 closed `LoginPage.qml` to styling change. Wind Map will need a
catalogue entry and a setup view, which is **content**, not styling — but it
must be added through the existing Training Lab catalogue mechanism
(`practiceView`), not by re-laying-out the page, and in its own commits.

---

## 2. What Wind Map is — APPROVED

**Visible name: `Wind Map — Post-Session Review`.**

A Training Lab programme for outdoor 50 m shooting in which the athlete or
coach records the wind condition observed for each shot, so that the group and
the wind can be reviewed together afterwards — answering "where did I hit when
the wind was doing *that*".

**Approved 2026-07-29.** Wind Map Release 1 is a **post-session training and
review** programme. It is explicitly **not** a live sight-correction assistant,
a live coaching command system, a competition workflow, a pre-session weather
survey, or an official ISSF event mode.

## 3. Approved shape

Superseded in detail by the implementation specification; kept here as the
one-line summary of what was approved.

| Element | Proposal |
|---|---|
| Discipline scope | **50 m Rifle Prone and 50 m Rifle 3 Positions only.** Never 10 m. |
| Programme id | `wind_map`, kind `Training` |
| Wind entry | Manual: direction selector + speed in m/s + Calm + optional note |
| Stored values | `directionDegrees`, `speedMetresPerSecond`, `source`, `recordedTimestamp` |
| Capture model | **Standing condition.** The athlete sets it; it stays active until changed; each accepted shot takes an **immutable snapshot** |
| Sighters | Recorded, visually identifiable, recoverable, reviewable, **excluded from counted-shot statistics by default** |
| Analytics | **Descriptive only.** Correlation, never causation. Sample size always shown |
| 3P | Kneeling, Prone and Standing analysed **separately**; position-specific analytics are authoritative |
| Report | Target plot, direction/speed summaries, sighter separation, position-separated 3P, data limitations, "What You Should Take" |

## 4. Non-negotiable constraints (from the existing architecture)

1. **Never a qualification or Final session.** Classified and started through
   the TRAINING owner, exactly as Technical Blocks, Call & Diagnose and
   Position Transition are.
2. **Scoring is untouched.** `CenterPane.qml::calculateShootingSocre()` does
   not change. Wind is recorded *alongside* a shot, never an input to its
   score.
3. **Analytics stay neutral and C++-side.** Every reported value derives in
   C++; QML only formats. No presentation logic in the engine.
4. **Journal is append-only**, events flow through `SessionStore`, and a
   restorer must exist before the programme ships — recovery is not optional.
5. **Recovery fails safe.** An unknown discipline must never fall back to
   Finals.
6. **3P separation** gated on `is3PMatch`, per `docs/3p-discipline.md`.
7. **No homepage styling change** (UI-DEC-012).
8. **No networking.** Wind data is entered by a person; there is no sensor
   integration in scope, and none may be added without its own approval.

## 5. Risks

| # | Risk | Mitigation |
|---|---|---|
| 1 | Wind entry interrupts the shot rhythm, making the training worse than none | Q3 — decide the entry model before building it |
| 2 | Manually entered wind is subjective and inconsistent | Q2 — a small fixed vocabulary beats free numbers |
| 3 | Sample sizes per condition are tiny, so "analysis" becomes noise | Q5 — decide the minimum sample before a conclusion is shown |
| 4 | Implying a causal link between wind and group that the data cannot support | Analytics must describe, never advise |
| 5 | Scope creep into sensor integration or forecast data | Out of scope; constraint 8 |

## 6. Proposed stage plan (stages 2–8)

Only after §7 is answered:

| Stage | Deliverable |
|---|---|
| 2 | `WindMapTypes.h` + `WindMapController` — state machine, no UI |
| 3 | Domain events + reducer + snapshot |
| 4 | Recovery restorer + dispatcher registration |
| 5 | `WindMapAnalytics` — pure C++, neutral |
| 6 | Catalogue entry, setup view, HUD, review — Training Lab mechanism only |
| 7 | Report / PDF on the shared Report System |
| 8 | Harness tests, build, regression, focused commits |

Each stage is its own commit. No stage skips its tests.

---

## 7. Approved decisions — 2026-07-29

| # | Question | Approved answer |
|---|---|---|
| Q1 | What is Wind Map for? | **Post-session training and review.** Not live correction, not coaching commands, not competition, not a weather survey, not an official ISSF mode. Visible name `Wind Map — Post-Session Review`. |
| Q2 | How is wind recorded? | **Manual entry.** Touch-friendly direction control (labels N…NW) but **stored as numeric degrees**; speed in m/s; a Calm option; an optional short note. Domain carries `source` so a future `WeatherStation` can be added — **no device integration now**. |
| Q3 | When is it recorded? | **Standing condition.** Set before shooting, stays active until changed, and every accepted shot automatically takes an **immutable snapshot**. The athlete never re-enters an unchanged condition. |
| Q4 | Who records it? | The athlete, on the same machine. No second device, so no networking. |
| Q5 | What may the analytics claim? | **Describe only.** Counts, mean point of impact, displacement, group size and centre, distribution by sector and speed band, per-position comparison, timeline, neutral observations, insufficient-sample warnings. **Never** sight clicks, aiming instructions, correction values, causal claims, or conclusions from tiny samples. Every comparison shows its sample size. |
| Q6 | ISSF constraints? | **Not blocking.** Wind Map is a Training Lab programme and changes no official scoring, timing or workflow. It must never be described as an official ISSF mode, and **if implementation later touches official range wind indicators, equipment requirements or competition-operation claims, stop and obtain the official rule source first.** |
| Q7 | Discipline scope? | **50 m Rifle Prone and 50 m Rifle 3 Positions.** Not 10 m Air Rifle, not 10 m Air Pistol. For 3P, Kneeling / Prone / Standing are analysed separately and are authoritative; a combined overview may exist but must not pool the three into one conclusion. |

### Wording rule (from Q5)

Permitted: *"Shots recorded under this wind condition were grouped
predominantly left of the session reference centre."*

Prohibited: *"This wind pushed the shots left."*

The difference is causation. The programme records what was observed and where
the shots landed; it does not assert that one produced the other.

## 8. Outcome

All seven questions are answered and stage 1 is closed. The approved scope is
close to the smallest coherent first release recommended here, with one
deliberate difference: wind speed is captured as a **numeric m/s value** rather
than a fixed band vocabulary, with banding applied at analysis time. That keeps
the stored data future-proof for a weather-station source without changing the
entry effort.

**Next:** `docs/training-lab-wind-map-implementation-spec.md`. Implementation
is gated on review of that document.
