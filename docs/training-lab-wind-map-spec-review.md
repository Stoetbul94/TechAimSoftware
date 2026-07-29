# Training Lab Release 2 — Wind Map · Specification Review

**Phase:** TRAINING LAB RELEASE 2 — WIND MAP · **Stage 1 of 8: specification review**
**Reviewed at commit:** `98b7175` · **Date:** 2026-07-29
**Status:** ⛔ **BLOCKED — AWAITING ARNOLD'S SPECIFICATION DECISIONS**

> **No code has been written and none should be** until §7 is answered. This
> document is the output of stage 1; stages 2–8 are gated on it.

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

## 2. What Wind Map appears to be

**Stated understanding, for Arnold to confirm or correct.** Nothing below is
agreed.

A Training Lab programme for outdoor 50 m shooting in which the athlete or
coach records the wind condition observed for each shot, so that the group and
the wind can be reviewed together afterwards — answering "where did I hit when
the wind was doing *that*".

That is the minimum coherent reading of "Wind Map". **It may not be what is
wanted**, which is why §7 Q1 asks before anything is designed around it.

## 3. Proposed shape — NOT APPROVED

Recorded so the questions in §7 have something concrete to react to.

| Element | Proposal |
|---|---|
| Discipline scope | 50 m Rifle only (Prone and 3P). Wind is meaningless at 10 m indoors. |
| Programme id | `wind_map`, kind `Training` |
| Per-shot capture | direction + strength, recorded against the shot |
| Entry | before or after each shot, one action, not a form |
| Analytics | group centre and dispersion **per wind condition**, compared against the no-wind or overall group |
| 3P | per position, following `docs/3p-discipline.md` |
| Report | shot map with wind annotation, plus a per-condition summary table |

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

## 7. ⛔ Questions that block stage 2

**Q1 — What is Wind Map for?** Confirm or correct §2. Is it per-shot wind
recording for post-session review, a live aiming-correction aid, a pre-session
range assessment, or something else? Everything downstream depends on this.

**Q2 — How is wind recorded?** A fixed vocabulary (e.g. direction to 8 points
+ 3–4 strength bands) is consistent and fast; free numeric entry (m/s and
degrees) is precise but slow and unreliable when typed between shots. Which?

**Q3 — When is it recorded?** Per shot, per series, or on change only?
Per-shot is richest and most disruptive; on-change is least disruptive and
leaves gaps to interpolate.

**Q4 — Who records it?** The athlete between shots, or a coach on a second
device? *(A second device implies networking, which is out of scope — if the
answer is "coach", the coach must be at the same machine.)*

**Q5 — What may the analytics claim?** Describe only ("in left wind your group
centred 4 mm right"), or advise ("hold 4 mm left")? Advice is a coaching
judgement the application is not positioned to make, and I would not implement
it without an explicit decision.

**Q6 — Are there ISSF constraints to record?** Wind indicators on the range
are regulated. If Wind Map is ever used at a sanctioned event, there may be
rules about what an athlete may consult during a match. **This is the one
question that may need an official rule lookup** — if so, the rule reference
is needed before stage 2.

**Q7 — Scope confirmation.** 50 m only? Both Prone and 3P, or 3P only in the
first release?

---

## 8. Recommendation

**Answer Q1, Q2, Q3, Q5 and Q7 to unblock stage 2.** Q4 and Q6 can be settled
during stage 2 without stalling it.

I recommend the smallest coherent first release: **50 m, per-shot fixed-
vocabulary wind, recorded by the athlete, describe-only analytics, Prone and
3P**. It is the version most likely to be used on a range, and it can be
extended once there is real session data to look at.
