# Tech Aim Training Lab — coach review pack

**Status:** AWAITING REVIEW · **Prepared:** 2026-07-30 ·
**Policy:** `training-lab-evidence-standard.md` ·
**Register:** `training-lab-evidence-register.md`

Documentation only. Nothing here changes the software.

---

## For the reviewing coach

Tech Aim's Training Lab measures **where shots landed** and **when they were
fired**. It has no aim trace, no force plate, no trigger-release signal and no
video. That limit is the reason almost everything below is worded as a
description rather than a diagnosis.

The published research we were able to verify constrains us more than it helps
us. The most relevant single finding — Ihalainen, Kuitunen, Mononen & Linnamo
(2016), 40 international and national air-rifle shooters, 13 795 shots — is that
**stability of hold, cleanness of triggering, aiming accuracy and timing of
triggering account for 81% of the variance in shooting score**. All four are
measured by aim-trace instrumentation. None of them is visible to Tech Aim.

So the rules below are **Tech Aim's own product decisions**. They are not
research findings and not ISSF rules, and the software says so. What we need
from you is whether they are **useful and safe to say to an athlete** — not
whether they are scientifically proven, because they are not.

**Please do not feel obliged to accept anything.** A "Reject" on any rule is a
useful outcome; the alternative is that the software keeps saying something a
coach would not.

### What your decision does

| Outcome | Effect |
|---|---|
| **Accept** | The rule becomes a COACH-APPROVED PRODUCT RULE, recorded with your name and the date. It still does **not** become research. |
| **Accept provisionally** | Stays a reasoned product rule; your reservation is recorded and drives the next revision. |
| **Change** | We implement your wording or threshold, then bring it back to you. |
| **Reject** | The rule is removed or the feature stops producing a conclusion. |

**No approval is recorded until you have actually reviewed the item.** Every
entry below has empty reviewer, qualification and date fields, and an automated
check fails the build if any of them is filled in without the others.

---

## 1. TB-05 — Technical Blocks default structure

**Priority: highest.** This is the question the literature cannot answer and a
coach can.

| | |
|---|---|
| **Current rule** | Per-discipline defaults: 10 m Air Rifle 5 blocks × 6 shots · 10 m Air Pistol 6 × 5 · 50 m Prone 5 × 6 · 50 m 3P 2 blocks × 6 shots per position (36 total). Configurable 1–12 blocks, 3–20 shots, ≤ 120 total. |
| **Athlete-facing wording** | "Tech Aim default — you or your coach may set a different number of blocks and shots per block." |
| **Classification** | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| **Verified research support** | **None for the numbers.** Shea & Morgan (1979) established the contextual-interference effect in a laboratory. Barreiros, Figueiredo & Godinho (2007) found roughly **60% of applied studies fail to reproduce it**. Ammar et al. (2023) is a systematic review whose title characterises the sports-practice benefit as a myth. |
| **Limitation** | No source establishes an optimal block length, block count or repetition scheme for any ISSF discipline. Nothing here is shooting-specific. |
| **Proposed wording** | As implemented — defaults presented as a starting point, never as optimal, ideal, research-proven or universally recommended. |
| **Test examples** | 3P default produces K1→K2→P1→P2→S1→S2, 36 counted shots. A 7-block 3P configuration is **rejected** with a message rather than silently corrected. |

**Questions for you:** Are these sensible starting structures for the athletes
you coach? Is 6 shots a useful block for 50 m? Should 3P default to more than
2 blocks per position? Is 120 total shots the right safety ceiling?

| Decision | |
|---|---|
| Accept / Accept provisionally / Reject / Change | |
| Reviewer name | |
| Qualification or relevant experience | |
| Review date | |
| Notes | |

---

## 2. GP-01 — Group Pattern Coach categories

**Priority: high.** Its output is reused inside Technical Blocks and Position
Transition, so anything wrong here reaches three programmes.

| | |
|---|---|
| **Current rule** | Eleven measured categories: tight centred · tight offset · wide · horizontal / vertical / diagonal string · two clusters · progressive drift · expansion · contraction · isolated outlier. Minimum 5 shots. Thresholds: string ratio ≥ 1.6 with principal axis ≤ 25° or ≥ 65°; two clusters when the largest projected gap > 4× the median gap with ≥ 2 shots each side and n ≥ 6; isolated outlier > median + 3.5×MAD **and** > 1.8× the second largest; expansion ≥ 1.5 / contraction ≤ 0.66; drift ≥ 0.75 ring **and** ≥ 0.6 mean radius; tight ≤ 2.0 ring; centred ≤ 0.5 ring. |
| **Athlete-facing wording** | A label plus measured evidence, e.g. "Vertical spread dominant — spread was 2.1× greater along one axis (H 3.2 mm, V 6.8 mm)", followed by "This describes the measured group shape. It does not identify the technical cause." and a question such as "Did the aiming picture appear to move mainly vertically?" |
| **Classification** | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| **Verified research support** | **None.** No peer-reviewed source for shooting-group pattern interpretation was found. The familiar target-diagnosis charts that map a group shape to a technical fault have no verified basis and are **not used**. |
| **Limitation** | The geometry is a fact; the category name is a Tech Aim convention. A category proves nothing about how the shots were produced. |
| **Proposed wording** | Unchanged — this programme already refuses to name a cause. |
| **Test examples** | A 5-shot group reports "insufficient" rather than a pattern. A symmetric bimodal set is detected by median gap, not by SD. |

**Questions for you:** Are these the categories you would actually discuss with
an athlete? Is 5 shots too few to name a pattern at all? Are the discussion
prompts the right questions? Should "confidence: strong/moderate/low" be
renamed, since it is a rule lookup and not a statistical confidence?

**Also flagged by the audit (GP-C):** drift compares thirds and expansion
compares halves, so in a 5-shot block a "third" is one shot. We propose
requiring n ≥ 9 for drift and n ≥ 8 for expansion. **This changes behaviour**,
so it needs your view and Arnold's.

| Decision | |
|---|---|
| Accept / Accept provisionally / Reject / Change | |
| Reviewer name | |
| Qualification or relevant experience | |
| Review date | |
| Notes | |

---

## 3. PT-02 — Rhythm-variability bands

| | |
|---|---|
| **Current rule** | Coefficient of variation of shot-to-shot intervals across the verification block. CV < 0.20 → "Low rhythm variability"; < 0.40 → "Moderate"; otherwise "High". Requires timing on every shot and ≥ 3 shots. |
| **Athlete-facing wording** | The band, always accompanied by "CV 0.34 over 4 shot intervals". |
| **Classification** | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| **Verified research support** | **None.** No source establishes that a low cadence CV is desirable in 3P. |
| **Limitation** | At the 3-shot minimum the CV rests on **two intervals**, which is very noisy. The band is presented with the same weight regardless of sample size. |
| **Proposed wording** | As implemented. The previous label "Inconsistent" was removed in this phase because it read as a criticism of the shooter rather than a property of the intervals. |
| **Test examples** | Five shots exactly 3 s apart → "Low rhythm variability", CV ≈ 0. Splits of 1.0/12.0/1.5/14.0/1.2 s → "High rhythm variability". |

**Questions for you:** Are 0.20 and 0.40 sensible cuts? **Should the label be
suppressed below 5 shots with timing?** — the audit recommends it, the brief
did not mandate it, and it changes behaviour. Is cadence consistency even a
thing you would coach in a 3P verification block?

| Decision | |
|---|---|
| Accept / Accept provisionally / Reject / Change | |
| Reviewer name | |
| Qualification or relevant experience | |
| Review date | |
| Notes | |

---

## 4. PT-06 — Transition and settling wording

**This one previously said something the evidence does not support**, and was
corrected in this phase. We would like you to confirm the replacement.

| | |
|---|---|
| **Current rule** | When the same position holds both the shortest ready-to-first-counted-shot time and the widest verification group, both facts are reported together. |
| **Previous wording (removed)** | "…reached the first shot quickest yet spread the widest — **worth checking whether the position was fully settled before firing**." |
| **Current wording** | "Standing had both the shortest ready-to-first-counted-shot time (12 s) and the widest verification group in this session. These are two separate measurements and this data cannot show whether they are related. Next training step: repeat Standing with a deliberately longer interval before the first counted shot and compare the group with today's." |
| **Classification** | FUTURE VALIDATION REQUIRED |
| **Verified research support** | **Points against the old wording.** Ihalainen et al. (2016) found postural balance explained **under 1%** of score variance directly. Era, Konttinen, Mehto, Saarela & Lyytinen (1996) compared each shooter's best 20 against their worst 20 shots and found a notable balance difference **only in the amateurs** — for top-level shooters a posture-stabilisation miss was seldom the reason for a poor result. |
| **Limitation** | Two measurements co-occurring in one position in one session is not evidence of a mechanism. Position Transition measures one athlete against themselves — precisely the within-athlete case where the evidence is weakest. |
| **Test examples** | The phrase "fully settled" is absent from the source and a check fails the build if it returns. "A shorter time is not automatically better, and a longer one is not automatically worse" accompanies the timing ranking. |

**Questions for you:** Is the replacement useful, or should the observation be
dropped altogether? Is "repeat with a deliberately longer interval" a test you
would actually set? Would you want a settling-time figure reported at all?

| Decision | |
|---|---|
| Accept / Accept provisionally / Reject / Change | |
| Reviewer name | |
| Qualification or relevant experience | |
| Review date | |
| Notes | |

---

## 5. CD-02 — Call-accuracy awareness bands

| | |
|---|---|
| **Current rule** | Radial call error normalised by the discipline's ring spacing: ≤ 0.5 ring → "Within half a ring"; ≤ 1.0 → "Within one ring"; otherwise "More than one ring". |
| **Athlete-facing wording** | The band plus the measured distance, e.g. "Within half a ring (~0.4 ring spacings)". Directional bias always carries: "This describes a difference in shot perception. It does not indicate that the sights or the shots should be moved." |
| **Classification** | REASONED PRODUCT RULE — COACH REVIEW REQUIRED |
| **Verified research support** | For the **activity**, yes: Guadagnoli & Kohl (2001), 64 participants, found the group required to estimate its own error before receiving feedback performed best in retention. That is exactly what calling a shot before the reveal is. For the **bands**, none — the study was a laboratory force-production task and graded nobody's self-awareness. |
| **Limitation** | No source establishes what call accuracy is good. The bands are a readable scale, not a standard. |
| **Proposed wording** | As implemented. This phase also replaced a single-shot judgement ("good awareness") with "Small call-to-impact difference on this shot, with a low recorded score." |
| **Test examples** | A call 3.9 mm from the impact at 50 m (ring 8.0 mm) → "Within half a ring". Bias needs ≥ 3 calls; trend needs ≥ 5. |

**Questions for you:** Are half-ring and one-ring the right divisions? Should
they differ between 10 m and 50 m? Two internal score cuts — "low-scoring" at
< 8.0 and "central" at ≥ 9.0 — mean quite different things at 10 m and 50 m;
should they be expressed in rings instead?

| Decision | |
|---|---|
| Accept / Accept provisionally / Reject / Change | |
| Reviewer name | |
| Qualification or relevant experience | |
| Review date | |
| Notes | |

---

## 6. Wind Map offset and dispersion thresholds

Corrected in this phase — see `../training-lab-wind-map-dispersion.md`.

| | |
|---|---|
| **Current rule — meaningful centre offset** | A group centre must move at least `max(3.0 mm, 1.0 × the reference group's own mean radius)`. Both bars must be cleared. |
| **Current rule — comparative dispersion** | Compared **radial RMS** ≤ 1.25 × reference → comparatively compact. ≥ 1.50 × → clearly wider. Between the two → **no verdict is forced**. |
| **Current rule — elevated dispersion** | Every described group at or above **1.5 × the discipline ring spacing** (12.0 mm at 50 m) in radial RMS. |
| **Athlete-facing wording** | "Shots under W · 3.0 m/s were more widely dispersed than under Calm (radial RMS 10.8 mm against 6.7 mm)" · "Dispersion remained elevated across the recorded conditions." Never poor, bad, weak, unacceptable or inadequate. |
| **Classification** | REASONED PRODUCT RULE — COACH REVIEW REQUIRED (all three) |
| **Verified research support** | **None for any threshold.** The verified sources support the cautions Wind Map applies, not its numbers. Mononen et al. (2007) found the stability/accuracy relationship held only **between** athletes and not within one — which is the case Wind Map analyses. |
| **Limitation** | The dispersion comparison is now on a sample-size-safe estimator, but the ratios themselves remain unvalidated judgement calls. The elevated-dispersion bar is the weakest rule in the set: when every group is wide there is no within-session reference, so some absolute-ish bar is unavoidable. |
| **Proposed wording** | As implemented, with the provisional status stated to the athlete in the verdict's own limitations. |
| **Test examples** | Two groups with identical radial RMS at n = 5 and n = 20 produce **no** "wider" verdict, even though the 20-shot group's extreme spread is more than 1.25× the other's. A 1.6× wider radial distribution does produce it. |

**Questions for you:** Is a 50% increase in dispersion the right point to tell
an athlete their group was "more widely dispersed"? Is 3 mm a sensible floor
for a meaningful centre shift at 50 m? Is 1.5 ring spacings (12 mm radial RMS)
a reasonable point to say dispersion is "elevated" — or should the observation
be dropped until it can be validated?

| Decision | |
|---|---|
| Accept / Accept provisionally / Reject / Change | |
| Reviewer name | |
| Qualification or relevant experience | |
| Review date | |
| Notes | |

---

## Recording an outcome

When a rule is reviewed, add a row to the **Coach review** table in
`training-lab-evidence-register.md` with reviewer, qualification, date,
programme, rule, wording reviewed, outcome, requested changes and notes — and
update that rule's classification in Part 2 **in the same commit**. An
`ACCEPTED` outcome converts REASONED PRODUCT RULE to COACH-APPROVED PRODUCT
RULE. It does not make it research.

**No coach approval has been recorded. No name has been entered.**
