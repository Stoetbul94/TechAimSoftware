# Technical Blocks — evidence audit

**Programme:** Technical Blocks (T1) · **Audited:** 2026-07-30
**Policy:** `training-lab-evidence-standard.md` ·
**Register:** `training-lab-evidence-register.md` (TB-01 … TB-08)

---

## 1. What the programme actually does

A session is divided into **blocks** of a fixed number of counted shots, with one
declared **technical focus** and one **visibility mode**. Sighters are separated
and excluded from every result. After each block the athlete reviews it; at the
end the session is summarised.

Measured per block (`src/training/TrainingBlockMetrics.{h,cpp}`): shot count,
total and mean score, score SD, MPI, mean radius, extreme spread, H/V sample SD,
and — where every shot carried a stamp — mean and SD of the shot-to-shot
interval. Geometry is delegated to the accepted `CoachAnalyticsEngine`;
group *shape* is delegated to `GroupPatternAnalyzer` (see
`group-pattern-coach-evidence.md`).

Cross-block (`compareBlocks`): best score block, tightest group block, most
repeatable block, first-to-last centre drift and group-size change.

**The header comment states the intent plainly** — measured/observed language
only, no causes, no diagnoses. This audit confirms the implementation honours it.

---

## 2. Research verification

### 2.1 What was searched for

Deliberate practice · blocked versus variable practice · repetition and
retention · feedback frequency · knowledge of results · knowledge of performance
· contextual interference · skill transfer · fatigue and practice quality.

### 2.2 What was verified

**S7 — Shea & Morgan (1979)**, *J Exp Psychol Hum Learn Mem* 5(2):179–187,
DOI 10.1037/0278-7393.5.2.179. **VERIFIED — INDEX ONLY.** The origin of the
contextual-interference effect: blocked practice favours acquisition, random
practice favours retention and transfer. Cited here for provenance only — the
abstract was not read, so **no sample size or effect size is claimed from it**.

**S8 — Barreiros, Figueiredo & Godinho (2007)**, *Eur Phys Educ Rev*
13(2):195–208. **VERIFIED — RECORD READ.** Reviews contextual interference in
**applied** settings and finds that roughly **60% of applied studies fail to
reproduce the laboratory benefit**. Effect strength depends on task type,
inter-trial interval and feedback conditions.

**S9 — Ammar et al. (2023)**, *Educ Res Rev* 39:100537. **VERIFIED — INDEX
ONLY.** A systematic review and meta-analysis whose title characterises the
contextual-interference benefit in sports practice as a myth. Cited for
existence only — **no effect size is claimed from it**.

**S10 — Winstein & Schmidt (1990)**, *J Exp Psychol Learn Mem Cogn*
16(4):677–691. **VERIFIED — RECORD READ.** Reduced (50%) knowledge of results
improved retention relative to 100%, despite poorer acquisition performance.

**S4 — Salmoni, Schmidt & Walter (1984)**, *Psychol Bull* 95(3):355–386.
**VERIFIED — RECORD READ.** The guidance hypothesis.

**S11 — Guadagnoli & Kohl (2001)**, *J Motor Behav* 33(2):217–224.
**VERIFIED — RECORD READ.** 64 participants; requiring self-estimation of error
before feedback produced the best retention.

**Deliberate practice.** Ericsson, Krampe & Tesch-Römer (1993), *Psychol Rev*
100(3):363–406, DOI 10.1037/0033-295X.100.3.363, is the canonical source.
**Macnamara & Maitra (2019)**, *R Soc Open Sci* 6(8):190327
(**VERIFIED — RECORD READ**), attempted a double-blind replication and **did not
replicate the core finding** that accumulated deliberate practice corresponded
to each skill level; the effect was substantial but considerably smaller.
**Consequence: no Tech Aim claim rests on deliberate-practice theory.** It is
recorded here so that a future phase does not reach for it as settled.

### 2.3 What this evidence permits

| Claim | Verdict |
|---|---|
| "Blocked practice is the optimal training structure" | **NOT PERMITTED.** S8 finds most applied studies fail to reproduce the benefit; S9's title calls it a myth. |
| "Random/variable practice is better" | **NOT PERMITTED.** Same evidence cuts both ways; nothing here is shooting-specific. |
| "Hiding the score improves learning" | **NOT PERMITTED.** S4 and S10 make it plausible on laboratory tasks. They do not establish it for ISSF shooting. |
| "Structured blocks with a declared focus are a reasonable way to organise a training session" | **PERMITTED as a product design decision** — and that is exactly how the programme presents it. |
| "Performance decreased during the later blocks" | **PERMITTED** — a measurement. |
| "Fatigue caused the decline" | **NOT PERMITTED** under any circumstance. |

**The most important finding of this audit: the block structure has no research
mandate.** It is a sound, conventional way to organise practice — and it must be
described as a Tech Aim default, never as the research-preferred format.

---

## 3. Claim-by-claim audit

| # | Athlete-facing claim | Classification | Verdict |
|---|---|---|---|
| TB-01 | "wider horizontally than vertically" / "taller vertically" / "similar" | REASONED PRODUCT RULE | Accepted |
| TB-02 | "The group centre was <right/left> and <high/low>" | REASONED PRODUCT RULE | Accepted — **no sight advice anywhere** |
| TB-03 | "N mm larger/smaller than Block X" · "Group size grew/reduced N%" | REASONED PRODUCT RULE | Accepted — **no fatigue claim** |
| TB-04 | "Block N had the highest average score / tightest group / most consistent scores" | REASONED PRODUCT RULE | Accepted — caveat recommended (§5) |
| TB-05 | Block-structure defaults | REASONED PRODUCT RULE | **WORDING CHANGE REQUIRED** |
| TB-06 | Visibility modes | REASONED PRODUCT RULE | Accepted — no benefit claim displayed |
| TB-07 | Cadence figures | REASONED PRODUCT RULE | Accepted — numbers only, no label |
| TB-08 | Sighter separation | REASONED PRODUCT RULE | Accepted |

### 3.1 What the programme already gets right

**There is no fatigue statement anywhere.** The user's required distinction —
*"Performance decreased during the later blocks"* is allowed, *"Fatigue caused
the decline"* is not — is **already satisfied**. `sessionObservations()` reports
group-size change and centre drift as measurements and stops there.

**There is no sight-adjustment text anywhere**, despite TB-02 reporting exactly
the MPI offset that would tempt one.

**The technical focus is explicitly an athlete intention, not a diagnosis** —
`TrainingProgramTypes.h:52-53` states that the UI must never claim the focus
caused a particular shot. That comment is a model for the rest of the Lab.

**Cadence (TB-07) is reported as numbers with no label.** Compare Position
Transition, which applies evaluative rhythm words to the same underlying measure.
Technical Blocks made the more conservative choice.

---

## 4. Threshold origins

**All Tech Aim product decisions.** None is research-derived; none is an ISSF rule.

| Threshold | Value | Where | Origin |
|---|---|---|---|
| H vs V dominance | 1.25× | `TrainingProgramController.cpp:659-662` | Product decision |
| Block-delta reporting floor | 0.5 mm | `:672` | Product decision |
| Group-size-change floor | 1% | `:697` | Product decision |
| Centre-drift floor | 0.5 mm | `:701` | Product decision |
| Default 10 m Air Rifle | 5 blocks × 6 shots | `TrainingProgramTypes.h:129-132` | Product decision (Refined Research Specification v0.2) |
| Default 10 m Air Pistol | 6 × 5 | `:133-136` | Product decision |
| Default 50 m Prone | 5 × 6 | `:137-140` | Product decision |
| Default 50 m 3P | 2 × 6 per position (36 total) | `:141-146` | Product decision |
| Configuration guardrails | 1–12 blocks, 3–20 shots, ≤ 120 total | `:75-79` | Product decision — safety limits, not training advice |
| Estimated time hints | "35–45 min" etc. | `:131` ff. | Product decision |

**Guardrails are not thresholds in the evidential sense** — they prevent
nonsensical configurations and are correctly rejected rather than silently
corrected. No claim attaches to them.

---

## 5. Wording changes required

### TB-05 — block defaults must be labelled as defaults

The setup screen offers per-discipline block structures. Nothing currently tells
the athlete these are **Tech Aim defaults** rather than a recommended or
evidence-based structure. Given S8 and S9, presenting a blocked structure as
preferred practice would be a claim the evidence does not support.

**Required:** the setup screen and the manual describe them as *Tech Aim default
configurations, adjustable*, with no implication of optimality. No numbers change.

**Coach review is the correct route to strengthen this** — block length and
repetition count are exactly the kind of question a qualified coach can answer
where the literature cannot. This is the **highest-priority coach-review item in
the Training Lab**.

### Recommended, not required

**TB-04 — sample-size caveat on block rankings.** "Block 3 had the tightest
group" is a ranking over 5–6 blocks of 5–6 shots, which is dominated by sampling
noise. The statement is true and should stay, but a caveat in the same block —
"differences between blocks this size are small relative to normal
shot-to-shot variation" — would prevent it being read as evidence that something
was better in that block. This mirrors the caveats already mandatory in
Position Transition (PT-05) and Call & Diagnose (CD-04).

**TB-06 — visibility modes.** No benefit claim is currently shown, which is
correct. If a future phase adds explanatory text, it must say *why the mode
exists* ("the score is hidden so you judge the shot yourself"), never *that it
works*.

---

## 6. Test gaps

| Gap | Effect |
|---|---|
| No test asserts the **absence of fatigue vocabulary** in generated observations | The programme's cleanest property is unprotected against a future edit |
| No test asserts the **absence of sight-adjustment language** near TB-02 | Same, for the most tempting claim in the programme |
| No test covers the **block-delta floor at exactly 0.5 mm** | Boundary unverified |
| No test covers `compareBlocks` with **ties** (two blocks with identical diameter) | Ranking behaviour undefined in output terms |
| No test covers **blocks with mixed timing availability** (some shots stamped, some not) | `hasTiming` gating is only exercised in the all-or-nothing case |

---

## 7. PDF and manual implications

- The **Training PDF** prints block and session observations. It inherits
  Group Pattern Coach's disclaimer requirement wherever a pattern label appears.
- The **operator manual** must describe block defaults as defaults (TB-05) and
  must not present a rationale for the block structure that implies research
  backing.
- Manual diagram **DG-06**
  (`docs/manual/diagrams/DG-06_technical_blocks.svg`) should be reviewed during
  manual regeneration for any implied optimality of the default structure.

---

## 8. Summary

| | |
|---|---|
| Athlete-facing claims | 8 register entries |
| Research-supported | 0 — and none is asserted |
| Reasoned product rules | 8 |
| Coach-approved rules | 0 |
| Overstated claims | 1 (TB-05, by presentation rather than by wording) |
| Mandatory wording changes | 1 (TB-05) |
| Recommended wording changes | 2 (TB-04 caveat, TB-06 future text) |
| Algorithms requiring change | **0** |
| Algorithms requiring review | 0 (the drift/expansion small-n issue lives in Group Pattern Coach, GP-C) |
| Test gaps | 5 |
| Notable | **No fatigue claim, no sight advice** — the programme already meets the standard's hardest requirements |
