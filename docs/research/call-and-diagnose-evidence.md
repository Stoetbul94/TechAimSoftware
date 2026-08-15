# Call & Diagnose — evidence audit

**Programme:** Call & Diagnose (T2) · **Audited:** 2026-07-30
**Policy:** `training-lab-evidence-standard.md` ·
**Register:** `training-lab-evidence-register.md` (CD-01 … CD-10)

---

## 1. What the programme actually does

The athlete calls where the shot went **before** the impact is revealed. The
programme then measures the difference between the call and the recorded impact.

`src/training/CallDiagnoseAnalytics.{h,cpp}` computes, per shot: signed x and y
error (call − actual), radial error, and the actual decimal score. Per session:
count, mean, median, smallest, largest, SD, mean |x| and |y|, signed bias,
least-squares trend, Tukey IQR outlier fence, and early-vs-late halves.

`CallDiagnoseController` composes all athlete-facing sentences in C++. QML
formats them.

**The programme name says "Diagnose" but the implementation does not diagnose.**
It describes a perception difference. That gap between name and behaviour is
worth keeping in mind whenever new feedback is added here.

---

## 2. Research verification

### 2.1 What was searched for

Self-assessment accuracy · calibration · metacognition · error awareness ·
internal feedback · augmented feedback · knowledge of results · perceptual
learning · attentional focus.

### 2.2 What was verified

**S11 — Guadagnoli & Kohl (2001)**, *J Motor Behav* 33(2):217–224, PMID
11404216. 64 participants, force-production task, 2 × 2 design: error estimation
required or not × knowledge of results at 100% or 20%. **The group required to
estimate its own error and given 100% feedback performed best in retention.**
The authors concluded that pre-feedback cognitive engagement affects how
learners subsequently use feedback.

**This is the direct evidential basis for the programme's existence.** Calling a
shot before the reveal *is* required error estimation followed by knowledge of
results — the exact condition that produced the best retention in S11.

**S10 — Winstein & Schmidt (1990)** and **S4 — Salmoni, Schmidt & Walter
(1984)** establish the guidance hypothesis: frequent augmented feedback can aid
immediate performance while degrading retention. Relevant context for the
*reveal* design.

### 2.3 What that support does and does not cover

| | |
|---|---|
| **Supported** | That asking an athlete to estimate their own error before revealing the result is a worthwhile training structure. |
| **Not supported** | Any claim about what a *particular call-error magnitude* means. S11's task was force production in a laboratory, not shooting. It measured retention of a motor task; it did not grade anyone's self-awareness. |
| **Not supported** | That call accuracy predicts shooting performance. No verified source establishes this. |
| **Population caution** | 64 participants on a laboratory task. Not shooters, not ISSF, not elite. |

**Nothing in the verified literature licenses a threshold for good or poor call
accuracy.** Every band and cut in this programme is a Tech Aim product decision.

---

## 3. Claim-by-claim audit

| # | Athlete-facing claim | Classification | Verdict |
|---|---|---|---|
| CD-01 | Radial call error in mm | RESEARCH-SUPPORTED *(the activity, not the number)* | Accepted |
| CD-02 | "Within half a ring" / "within one ring" / "more than one ring" | REASONED PRODUCT RULE | Accepted — descriptive, not graded |
| CD-03 | "Calls averaged N mm right/left of the measured impacts" | REASONED PRODUCT RULE | Accepted **with CD-04 adjacent** |
| CD-04 | The bias caveat | REASONED PRODUCT RULE (safety boundary) | Accepted — mandatory |
| CD-05 | Median as typical; Tukey outlier explanation | REASONED PRODUCT RULE | Accepted |
| CD-06 | "Call differences varied by about N mm" | REASONED PRODUCT RULE | Accepted |
| CD-07 | Trend: improved / became larger / broadly stable | REASONED PRODUCT RULE | Accepted — no fatigue claim |
| CD-08 | "Small call-to-impact difference on this shot, with a low recorded score." | REASONED PRODUCT RULE | **CORRECTED 2026-07-30** |
| CD-09 | Awareness-vs-result explanation | REASONED PRODUCT RULE | Accepted — an excellent guard |
| CD-10 | "closest call of the session so far" etc. | REASONED PRODUCT RULE | Accepted |

### 3.1 What this programme already gets right

**CD-04 is the strongest single safeguard in the Training Lab:**

> "This describes a difference in shot perception. It does not indicate that the
> sights or the shots should be moved."

The user's boundary — *"Your calls were consistently left of the recorded
impacts"* is allowed, *"Move your sights left"* is not — is **already
implemented and already enforced in prose**. No sight-adjustment text exists
anywhere in the programme.

**CD-09** is equally good: it tells the athlete explicitly that call accuracy and
shot score are different measurements, pre-empting the misreading that a poor
call means a poor shot.

**Trend wording (CD-07)** already follows the required form — *"Call differences
became larger in the later shots"*, never *"you got tired"*.

---

## 4. Threshold origins

**All Tech Aim product decisions.** None is research-derived; none is an ISSF rule.

| Threshold | Value | Where | Origin |
|---|---|---|---|
| Awareness bands | 0.5 and 1.0 ring spacings | `CallDiagnoseController.cpp:497-504` | Product decision |
| Bias reporting floor | 0.5 mm | `:749-754` | Product decision |
| Bias minimum sample | 3 calls | `CallDiagnoseAnalytics.cpp:63` | Product decision |
| Trend minimum sample | 5 calls | `:78` | Product decision |
| Halves minimum sample | 6 calls | `:109` | Product decision |
| Trend slope floor | 0.02 | `CallDiagnoseController.cpp:645` | Product decision |
| Half-difference floor | 0.5 mm | `:773-778` | Product decision |
| Outlier fence | q3 + 1.5 × IQR, n ≥ 4 | `CallDiagnoseAnalytics.cpp:102-106` | Tukey convention — a documented statistical standard, not shooting-specific |
| H vs V dominance | 1.25× | `CallDiagnoseController.cpp:640-643` | Product decision |
| Average-vs-median narrative | 1.25× + 0.5 mm | `:735` | Product decision |
| "Low-scoring" shot | score < 8.0 | `:795` | Product decision, **no recorded rationale** |
| "Central" shot | score ≥ 9.0 | `:806` | Product decision, **means different things at 10 m and 50 m** |
| Equality tolerance | 0.05 mm | `:550`, `:585-587` | Product decision |

Using the **median as the primary statistic** is the right conservative choice
and should be preserved: a single wild call cannot dominate the athlete's
headline number.

---

## 5. Wording changes required

### CD-08 — "good awareness" · **CORRECTED 2026-07-30**

Current, when a shot scoring under 8.0 was called within half a ring:

> "A low-scoring shot you still called accurately — good awareness."

**Problem.** "Good awareness" is an **evaluative judgement about the athlete**
inferred from **one shot**. It is the only such phrase in the programme, and it
has no basis: S11 supports error estimation as an activity, not a grading of a
person. It also sits oddly beside CD-09, which correctly insists that call
accuracy and score are different measurements.

**Implemented replacement** (measured, same information, no judgement):

> "Small call-to-impact difference on this shot, with a low recorded score."

The register entry stays REASONED PRODUCT RULE; the claim becomes an
observation.

### Recommended, not required

- **"Diagnose" in the programme name.** The programme measures call accuracy and
  explicitly declines to diagnose. Renaming is a product decision well outside
  this phase, but the manual should be clear that "Diagnose" names the athlete's
  activity of diagnosing their own shot, not the software diagnosing them.
- **Score cuts 8.0 / 9.0** should be expressed in ring terms or per discipline;
  9.0 at 10 m and 9.0 at 50 m are not comparable achievements.

---

## 6. Test gaps

| Gap | Effect |
|---|---|
| **No test asserts CD-04 accompanies every CD-03 bias statement** | The caveat could be dropped and the strongest safeguard in the Training Lab would vanish silently |
| No test asserts the absence of sight-adjustment language | A future edit could add "adjust your sights" and pass |
| No test covers CD-08's wording | The evaluative phrase is unguarded |
| No test covers bias at exactly n = 3, or trend at exactly n = 5 | Boundary behaviour unverified |
| No test asserts that revealing never occurs before the call is committed | The core integrity property of the programme rests on phase checks alone |

The first two are the priority: they protect boundaries the user has explicitly
named.

---

## 7. PDF and manual implications

- The **Call & Diagnose PDF** already prints the disclaimer *"Measured
  shot-awareness data only. This is not an official competition result and
  carries no ranking status. Sighters are excluded from all results."*
  (`CallDiagnoseReportView.qml:346`) — correct and to be preserved.
- **CD-04 must appear in the PDF wherever bias appears.** It currently does
  (`CallDiagnoseReportView.qml:213` concatenates `biasCaveat`), but no test
  proves it.
- The **operator manual** must not present call-accuracy bands as a performance
  scale. Describing them as *how the difference is reported* is correct;
  describing them as *how good the athlete is* is not.

---

## 8. Summary

| | |
|---|---|
| Athlete-facing claims | 10 register entries |
| Research-supported | 1 (CD-01 — the activity, within a laboratory population) |
| Reasoned product rules | 9 |
| Coach-approved rules | 0 |
| Overstated claims | 1 (CD-08) |
| Mandatory wording changes | 1 (CD-08) |
| Algorithms requiring change | **0** — the mathematics is sound |
| Algorithms requiring review | Score cuts 8.0 / 9.0 should become discipline-relative |
| Test gaps | 5, two of them protecting user-named boundaries |
