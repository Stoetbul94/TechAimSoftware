# Position Transition — evidence audit

**Programme:** Position Transition (T4, 3P only) · **Audited:** 2026-07-30
**Policy:** `training-lab-evidence-standard.md` ·
**Register:** `training-lab-evidence-register.md` (PT-01 … PT-08)

This programme carries the **most causal risk in the Training Lab**, because
timing plus group width invites a settling explanation that the data cannot
support — and because the verified literature actively argues against the
mechanism it used to hint at. **That hint was corrected on 2026-07-30**; this
document records both the finding and the fix.

---

## 1. What the programme actually does

For each 3P position (and each repeat) it records: setup duration, Position
Ready timestamp, sighters and their duration, a verification block, and a
per-position checklist.

From those it derives: setup duration · sighter count and duration ·
ready-to-first-shot time · verification duration · verification group metrics
(via `TrainingBlockMetrics` and `GroupPatternAnalyzer`) · first-shot score and
distance from MPI · cadence mean and SD · a rhythm label · checklist counts.

Session level: longest setup, shortest ready-to-first-counted-shot time, most
sighters, widest group, a combined timing-and-dispersion observation, a rhythm
comparison, and a mandatory closing caveat.

---

## 2. Research verification

### 2.1 What was searched for

Postural control · rifle stability · settling time · pre-shot routines · motor
routines · task switching · fatigue · attentional control · first-shot
performance · rhythm and consistency.

### 2.2 What was verified — and it constrains rather than supports

**S5 — Ihalainen, Kuitunen, Mononen & Linnamo (2016).** 40 international and
national air-rifle shooters, 13 795 shots. Stability of hold, cleanness of
triggering, aiming accuracy and timing of triggering accounted for **81% of the
variance in shooting score**. **Postural balance accounted for under 1%
directly**, though it correlated with holding stability (R = 0.55).

**S6 — Era, Konttinen, Mehto, Saarela & Lyytinen (1996).** Posture control over
the 7.5 s before the shot, across top-level, national-level and naive shooters.
Skill groups separated clearly. But comparing each shooter's **best 20 against
their worst 20 shots**, only the **amateurs** showed a notable balance
difference; for top-level shooters a posture-stabilisation miss was seldom the
reason for a poor result.

**S2 — Mononen, Konttinen, Viitasalo & Era (2007).** 58 novices, 10 m. The
balance-accuracy relationship held **only at the inter-individual level**
(r −0.29 to −0.45) — between athletes, not within one athlete across shots.

### 2.3 What this means for the programme

Read together, these three say something specific and inconvenient:

> Postural steadiness distinguishes **shooters of different levels**. It does
> **not** reliably explain **one shooter's good and bad shots** — and the better
> the shooter, the less it explains.

Position Transition measures **one athlete across their own session**. That is
exactly the within-athlete case where the evidence is weakest. Anything this
programme says about settling, steadiness or readiness is therefore **weakly
supported at best** and must stay descriptive.

### 2.4 What could not be verified

**No source was verified for pre-performance-routine duration or consistency in
ISSF 3P position transitions.** A pre-performance-routine meta-analysis exists in
the sports literature, but it was not read to a standard permitting a claim, and
it is not shooting-specific. **No claim in this programme rests on it.**

**No source establishes an optimal setup duration, settling time or
ready-to-first-shot time for any ISSF position.** Every timing statement is
therefore a measurement, never a target.

---

## 3. Claim-by-claim audit

| # | Athlete-facing claim | Classification | Verdict |
|---|---|---|---|
| PT-01 | Longest setup / fastest first shot / most sighters | REASONED PRODUCT RULE | Accepted — measured superlatives |
| PT-02 | "Low" / "Moderate" / "High rhythm variability", with CV and interval count | REASONED PRODUCT RULE | **CORRECTED 2026-07-30** |
| PT-03 | First shot in group / separated | FUTURE VALIDATION REQUIRED | **Downgraded** — display only |
| PT-04 | Widest verification group by position | REASONED PRODUCT RULE | Accepted with PT-05 |
| PT-05 | Cross-position comparison caveat | RESEARCH-SUPPORTED | Accepted — mandatory |
| PT-06 | "…this data cannot show whether they are related. Next training step: …" | FUTURE VALIDATION REQUIRED | **CORRECTED 2026-07-30** |
| PT-07 | Checklist and counts | REASONED PRODUCT RULE | Accepted |
| PT-08 | "…does not identify the technical cause." | RESEARCH-SUPPORTED | Accepted — mandatory |

### 3.1 What the programme already gets right

**PT-05 is exactly the sentence the evidence demands**, and it is already there:

> "Positions have different stability demands — compare each position to itself
> across repeats, not against another position."

The user's instruction — *do not compare Kneeling, Prone and Standing as though
they have identical stability demands; prefer within-position and within-athlete
comparisons* — is **already implemented**, and now has verified support (S2, S5,
S6). It must never be removed, and the code comment that calls rhythm
"procedural, so comparable across positions" should be read narrowly: procedure
may be comparable, **stability is not**.

**PT-08** likewise states the boundary on every position review.

**No fatigue claim exists anywhere in this programme.**

---

## 4. Threshold origins

**All Tech Aim product decisions.** None is research-derived; none is an ISSF rule.

| Threshold | Value | Where | Origin |
|---|---|---|---|
| Rhythm: Low variability | CV < 0.20 | `PositionTransitionController.cpp` | Product decision, no recorded rationale |
| Rhythm: Moderate variability | CV < 0.40 | same | Product decision, no recorded rationale |
| Rhythm minimum sample | 3 shots with timing | `:512` | Product decision — **a CV over 2 intervals is extremely noisy** |
| First shot "in group" | ≤ 1.5 × mean radius | `:560` | Product decision |
| First shot "separated" | > 2.0 × mean radius | `:561` | Product decision |
| Group pattern minimum | 5 shots | `GroupPatternAnalyzer.h:48` | Inherited (GP-03) |

---

## 5. Wording changes — ALL APPLIED 2026-07-30

### PT-06 — the settling hint · **CORRECTED**

Previous:

> "Standing reached the first shot quickest yet spread the widest — worth
> checking whether the position was fully settled before firing."

**Problem.** This is the **only causal hint in the Training Lab**. It proposes a
mechanism ("not fully settled") for an observed group width, from two measured
facts co-occurring **in one position, in one session**. S5 found postural balance
explained under 1% of score variance directly; S6 found that for trained
shooters a posture-stabilisation miss was seldom the reason for a poor result.
The verified evidence points **away** from the proposed mechanism, and the
programme measures precisely the within-athlete case where it is weakest.

The existing code comment ("both facts measured; the reading stays hedged") shows
the right intent. The hedge is not strong enough: *"worth checking whether the
position was fully settled"* still names a cause and invites the athlete to
accept it.

**Implemented replacement** — same two facts, no mechanism, a controlled test
instead:

> "Standing had both the shortest ready-to-first-counted-shot time (12 s) and
> the widest verification group in this session. These are two separate
> measurements and this data cannot show whether they are related. Next training
> step: repeat Standing with a deliberately longer interval before the first
> counted shot and compare the group with today's."

A documentation check now fails the build if the phrase "fully settled" returns
to the source, or if the "cannot show whether they are related" boundary is
removed.

That keeps the observation, keeps its usefulness, replaces the implied cause
with a test the athlete can actually run, and remains within the shared feedback
model (WHAT HAPPENED → LIMITATIONS → NEXT TRAINING STEP).

### PT-02 — rhythm labels · **CORRECTED**

Previous: **Steady** · **Variable** · **Inconsistent**, shown without the
measurement behind them.

**Problem.** "Inconsistent" is evaluative and reads as a criticism. No verified
source establishes that a low cadence CV is desirable in 3P, and a CV computed
over as few as 2 intervals (n = 3 shots) is very noisy. The label is presented
with the same weight regardless of sample size.

**Proposed replacement** — descriptive, with the measurement shown:

| Previous | Implemented |
|---|---|
| Steady | "Low rhythm variability" |
| Variable | "Moderate rhythm variability" |
| Inconsistent | "High rhythm variability" |

The label now never appears without `rhythmBasis` — "CV 0.34 over 4 shot
intervals" — so the reader can see how thin the sample is.

**Not implemented, and referred to the coach:** suppressing the label below 5
shots with timing rather than 3. That changes behaviour, the brief did not
mandate it, and it is a question a coach can answer better than the literature
can. It appears in `training-lab-coach-review-pack.md` §3.

### PT-03 — first shot in / out of group

**Downgraded to FUTURE VALIDATION REQUIRED.** The first shot is compared against
a mean radius **that includes the first shot**, which biases the comparison
toward "in group"; and at 5–8 verification shots the mean radius is unstable.
It may remain as a displayed measurement. It may not drive a conclusion, and it
must not feed a "first shot problem" narrative until it is validated.

---

## 6. Test gaps

| Gap | Effect |
|---|---|
| ~~PT-05 duplicated as a QML literal — **EVID-PT-001**~~ | **FIXED 2026-07-30.** Composed once as `PositionTransitionController::crossPositionCaveat()`; the report view binds `POSTRANS.crossPositionCaveat`. The checker now fails if ANY central caveat appears as a QML literal |
| **No test asserts PT-05 is present** in every session-observation list | The single most important sentence in the programme could be dropped silently |
| **No test asserts PT-08 is present** on every position review | Same, for the per-position disclaimer |
| No test asserts the absence of causal vocabulary ("caused", "because", "due to") in generated observations | PT-06's replacement could regress |
| No test covers the rhythm label at exactly n = 3 | The noisiest case is unverified |
| No test covers PT-03's self-inclusion bias | Undetected |
| No test covers repeats: the same position at repeat 1 vs repeat 2 | The **within-position comparison** the evidence actually supports is the least tested path |

The last one is notable: PT-05 tells the athlete to compare a position with
itself across repeats, and that is precisely the comparison with the weakest
test coverage.

---

## 7. PDF and manual implications

- The **Position Transition PDF** must carry PT-05 and PT-08 wherever it prints
  a position comparison or a position review. Printing them once on a summary
  page is not sufficient.
- The PDF inherits **Group Pattern Coach** output and therefore GP-01's
  prohibitions.
- The **operator manual** must not present a target setup duration, settling
  time or first-shot time. Describing what is measured is correct; implying a
  goal is not.
- Manual diagram **DG-08** (`docs/manual/diagrams/DG-08_position_transition.svg`)
  should be checked for any implied optimum during the manual regeneration step.

---

## 8. Summary

| | |
|---|---|
| Athlete-facing claims | 8 register entries |
| Research-supported | 2 (PT-05, PT-08) — both of them **limitations**, which is the honest outcome |
| Reasoned product rules | 4 |
| Future validation required | 2 (PT-03, PT-06) |
| Coach-approved rules | 0 |
| Overstated claims | 2 (PT-06 causal hint, PT-02 "Inconsistent") |
| Mandatory wording changes | 2 (PT-06, PT-02) |
| Algorithms requiring change | 0 — timing and geometry are correctly computed |
| Algorithms requiring review | PT-03 (self-inclusion bias), rhythm minimum sample |
| Test gaps | 6, two protecting mandatory caveats |
