# Group Pattern Coach — evidence audit

**Programme:** Group Pattern Coach (T3) · **Audited:** 2026-07-30
**Policy:** `training-lab-evidence-standard.md` ·
**Register:** `training-lab-evidence-register.md` (GP-01 … GP-04)

Audited first because its output is **reused inside Technical Blocks and
Position Transition**. Every claim it makes is inherited by those programmes, so
a loose statement here would leak into two more surfaces.

---

## 1. What the programme actually does

`src/training/GroupPatternAnalyzer.{h,cpp}` is a pure-C++ function:

```
analyzeGroup(xs, ys, ringSpacingMm) -> GroupPatternResult
```

It takes shot coordinates **in shot order** plus the discipline's ring spacing,
and returns measured geometry (MPI, mean radius, extreme spread, H/V sample SD,
principal-axis angle, centre offset, early→late drift, expansion ratio) together
with a list of **properties** — a label, a measured evidence sentence and a
rule-based confidence word.

It requires **5 shots**. QML performs no classification; it formats the result.

**The header already states the boundary**: it never claims a technical cause.
That intent is honoured throughout the implementation — this audit found **no
prohibited diagnosis anywhere in the engine**.

---

## 2. Research verification

### 2.1 What was searched for

Shooting-group statistics · dispersion analysis · mean point of impact · mean
radius · extreme spread · outlier treatment · clustering · reliability of small
samples · impact-pattern interpretation.

### 2.2 What was found

**Nothing peer-reviewed for pattern interpretation.** Searches for the
interpretation of shooting-group *shapes* return commercial coaching pages,
manufacturer blogs and forum threads — precisely the material the standard §4
says must never carry a strong athlete-facing claim. The well-known
"target-diagnosis charts" that map a group shape to a technical fault have **no
verified evidential basis** and are **not used** anywhere in Tech Aim.

For dispersion *measurement* the position is different, but the support is
**mathematical rather than empirical**:

**M1 — order statistics of the sample range.** For n samples from a normal
distribution the expected range is `d2(n)·σ`, with d2 = 1.128 (n = 2), 2.326
(n = 5), 3.078 (n = 10), 3.735 (n = 20). Two consequences bear directly on this
engine:

1. **Extreme spread is determined by exactly two shots.** All other shots are
   discarded. Mean radius uses every shot.
2. **Expected extreme spread grows with sample size.** Comparing the diameters
   of differently sized groups is biased even when the underlying dispersion is
   identical.

### 2.3 The constraining source

**S5 — Ihalainen et al. (2016)**, 40 elite/national air-rifle shooters, 13 795
shots: stability of hold, cleanness of triggering, aiming accuracy and timing of
triggering explained **81% of the variance in shooting score**. All four are
**aim-trace variables**. Group Pattern Coach sees none of them — it sees where
the shots landed.

This is the formal reason a group shape cannot name a cause: the dominant
causal variables are not in the input.

---

## 3. Category audit

Common to every category: **minimum sample 5 shots** (GP-03), **classification
REASONED PRODUCT RULE**, **coach review REQUIRED — NOT DONE**, and the universal
prohibition — no breathing, shoulder pressure, trigger, sight-picture, natural
point of aim or follow-through cause, and no sight-adjustment instruction.

| Category | Detection rule (implemented) | Geometry proves | May suggest | Cannot diagnose |
|---|---|---|---|---|
| **Tight & centred** | diameter ≤ 2.0 × ring **and** offset ≤ 0.5 × ring | Shots fell close together, near the centre | Nothing further needed this block | Which technique produced it |
| **Tight but offset** | diameter ≤ 2.0 × ring, offset > 0.5 × ring | Shots were consistent but displaced from centre | Aim reference, zero, position or ammunition | Which of those. **Never a sight instruction** |
| **Wide** | diameter > 2.0 × ring, no dominant direction | Shot-to-shot variation was large relative to ring spacing | Repeat with more shots | Any cause |
| **Horizontal string** | max(H/V ratio, anisotropy) ≥ 1.6 **and** principal axis ≤ 25° | Spread was greater along the horizontal axis | A directional influence, incl. wind | Trigger, cant, position |
| **Vertical string** | ratio ≥ 1.6 **and** axis ≥ 65° | Spread was greater along the vertical axis | A directional influence | **Not breathing. Not shoulder pressure. Not elevation error.** |
| **Diagonal string** | ratio ≥ 1.6, axis 25°–65° | Spread was greater along an oblique axis | A directional influence | Any cause |
| **Two clusters** | n ≥ 6, largest projected gap > 4 × median gap, ≥ 2 shots each side | The shots separated into two groups along the main axis | Something changed mid-block | What changed |
| **Progressive drift** | drift ≥ 0.75 × ring **and** ≥ 0.6 × mean radius | The centre of the first third differs from the last third | A change through the block | **Not fatigue.** Not concentration |
| **Expansion** | late/early mean radius ≥ 1.5 | Later shots sat further from the centre than early ones | A change through the block | **Not fatigue** |
| **Contraction** | ratio ≤ 0.66 | Later shots sat closer to the centre | Settling, warm-up | Any cause |
| **Isolated outlier** | max distance > median + 3.5 × MAD **and** > 1.8 × second-largest | One shot lay far outside the others | Worth discussing that shot | Why. **Never "a bad shot"** |

### 3.1 Permitted vs prohibited feedback, by example

**Permitted** — the form the engine already uses:

> "Vertical spread dominant. Spread was 2.1× greater along one axis
> (H 3.2 mm, V 6.8 mm)." *(moderate)*
> "This describes the measured group shape. It does not identify the technical
> cause."
> Possible next test: repeat the group while recording aim trace or with coach
> observation.

**Prohibited**:

> ~~"A vertical string indicates a breathing error."~~
> ~~"Your shoulder pressure is inconsistent."~~
> ~~"Move your sights left two clicks."~~
> ~~"This is a poor group."~~

The programme's existing **coach discussion prompts** are the model to follow
elsewhere: `"Did the aiming picture appear to move mainly vertically?"` is a
**question to the athlete**, not a claim about them. It invites the athlete to
supply the information the target face cannot.

---

## 4. Threshold origins

**Every threshold below is a Tech Aim product decision.** No verified source
supports any of them, and none is an ISSF rule.

| Threshold | Value | Origin |
|---|---|---|
| Minimum sample | 5 shots | Product decision. A floor, not a sufficiency (M1) |
| String detection | ratio ≥ 1.6 | Product decision |
| Axis classification | ≤ 25° / ≥ 65° | Product decision |
| Two clusters | gap > 4 × median gap, n ≥ 6, ≥ 2 per side | Product decision; robust-by-design (median, not SD) |
| Isolated outlier | > median + 3.5 × MAD **and** > 1.8 × second | Product decision; MAD is a standard robust estimator |
| Expansion / contraction | ≥ 1.5 / ≤ 0.66 | Product decision |
| Drift | ≥ 0.75 × ring **and** ≥ 0.6 × mean radius | Product decision |
| Tight | diameter ≤ 2.0 × ring | Product decision |
| Centred | offset ≤ 0.5 × ring | Product decision |
| Confidence: strong | ratio ≥ 2.5 **and** n ≥ 8 | Product decision |
| Confidence: moderate | ratio ≥ 1.8 | Product decision |

Scaling the tight/wide thresholds by **ring spacing** rather than absolute
millimetres is a genuinely good design choice: it makes the same rule behave
sensibly at 10 m and 50 m without a per-target constant.

---

## 5. Audit findings

### 5.1 Algorithms requiring NO change

- MPI, mean radius, H/V sample SD, covariance and principal axis — correct.
- **Robust statistics throughout**: median and MAD for outliers, median gap for
  clustering. The comment explaining why the two-cluster test avoids the overall
  SD (a symmetric bimodal set inflates it) is exactly right.
- Guards for identical points, zero spread and division by zero.
- Ring-spacing scaling.
- Coach discussion prompts — questions, never diagnoses.
- The disclaimer on every consumer surface.

### 5.2 Algorithms requiring review

**GP-A — extreme spread as the headline "group diameter" (GP-04).** Safe *inside*
this programme, because Technical Blocks and Position Transition compare blocks
of equal shot count. **Not safe** where sample sizes differ — which is exactly
what Wind Map does (defect **EVID-WM-001**). Recommendation: keep extreme spread
as a displayed figure, but use **mean radius** for any cross-set comparison.

**GP-B — the confidence vocabulary (GP-02).** "Strong" and "moderate" read as
statistical confidence. They are a ratio-and-count lookup with no probability
attached. Recommendation: align with the standard's evidence vocabulary
(INSUFFICIENT / INDICATIVE / COMPARATIVE), or rename to "evidence" rather than
"confidence". This is a **wording** change; the underlying rule is fine.

**GP-C — drift and expansion at small n.** Drift compares thirds; expansion
compares halves. In a 5-shot block a "third" is 1 shot. The guard
`std::max(1, n/3)` prevents a crash but not the statistical weakness: a
single-shot-vs-single-shot comparison is being labelled "progressive drift".
Recommendation: require **n ≥ 9** for drift and **n ≥ 8** for expansion, or
downgrade the confidence at low n. **This changes behaviour and needs Arnold's
decision.**

### 5.3 Overstated claims found

**None.** This is the cleanest programme in the Training Lab. No prohibited
diagnosis, no sight advice, no fatigue claim, no evaluative judgement of the
athlete. The word "wide" is used descriptively and is never paired with "poor".

### 5.4 Wording changes required

**None mandatory.** GP-B (confidence vocabulary) is recommended, not required.

---

## 6. Test gaps

| Gap | Effect |
|---|---|
| No test asserts the **absence of prohibited words** in any generated label, evidence sentence or prompt | A future edit could introduce "breathing" or "trigger" into a label and every test would still pass |
| No test asserts the **disclaimer is present** on every consumer surface | It could be dropped from a report view silently |
| No test covers **GP-C**: drift or expansion at n = 5 vs n = 12 | The small-n weakness is undetected |
| No test asserts the **prompts remain questions** (end with "?") | A prompt could become an assertion |

The first two are closed by this phase's documentation checks at the register
level; the code-level assertions belong in `tests/training/tst_training.cpp` in
the implementation phase.

---

## 7. PDF and manual implications

- Group Pattern output appears in the **Training PDF** and the **Position
  Transition PDF**. Both must carry the disclaimer wherever a pattern label
  appears — it is not sufficient to print it once on the summary page.
- The **operator manual** must describe the categories as *measured shapes* and
  must not offer an interpretation table. Any manual passage that pairs a shape
  with a cause is a defect against standard §5.
- A **Wind Map PDF (Stage 6.2)** consuming group geometry inherits GP-04 and
  therefore EVID-WM-001. **That defect should be resolved before the Wind Map
  PDF is built**, so the report does not print a sample-size artefact as a
  finding.

---

## 8. Summary

| | |
|---|---|
| Athlete-facing claims | 11 categories + 1 confidence vocabulary + 1 sample floor + 1 diameter definition |
| Research-supported claims | 0 — and none is asserted |
| Reasoned product rules | 4 register entries (GP-01 … GP-04) covering all thresholds |
| Coach-approved rules | 0 |
| Overstated claims | 0 |
| Mandatory wording changes | 0 |
| Algorithms requiring review | 3 (GP-A, GP-B, GP-C) |
| Open defects | EVID-WM-001 (inherited by Wind Map) |
