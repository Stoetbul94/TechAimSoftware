# Wind Map — evidence base for athlete feedback

**Phase:** STAGE 6.1.2 · **Date:** 2026-07-30
**Purpose:** the reasoning behind what the Wind Map verdict system is allowed
to say, and the limits it must respect.

---

## 0. How to read this document — and a warning about it

This file was drafted by the implementing agent from general knowledge of the
motor-learning and shooting-analysis literature. **It has not been checked
against the primary sources.** Every entry therefore carries a
**verification status**:

| Status | Meaning |
|---|---|
| **ESTABLISHED** | A broadly accepted position in the field. Safe to rely on as a *design constraint*, but the citation details still need confirming before publication. |
| **NEEDS VERIFICATION** | The claim is used to justify a product rule and the specific source, year or finding must be confirmed before this document is published or quoted to a customer. |
| **REASONING** | Not a literature claim at all — a first-principles argument from the data Wind Map actually holds. Judge it on its logic, not on a citation. |

**Nothing here may be quoted externally until a human has verified the
sources.** Where a rule below depends on a claim, the rule is deliberately
*conservative*: it withholds a conclusion rather than asserting one. That way
a citation that turns out weaker than expected makes the product cautious, not
wrong.

I have deliberately **not** invented precise page numbers, sample sizes or
effect sizes. Where I do not reliably know a figure, this document says so.

---

## 1. Impact-only data cannot diagnose technique

**Claim.** The position of a shot hole records the *outcome* of everything
that happened — hold, aim, trigger release, follow-through, ammunition,
equipment and the air the bullet passed through. It does not separate them.

**Status:** REASONING (with ESTABLISHED support).

The reasoning is sufficient on its own: Wind Map records only the shot
coordinate, the score, and an athlete's observation of wind. Several distinct
physical stories produce an identical coordinate. No inference from that
coordinate can distinguish them.

Supporting position from the field — **NEEDS VERIFICATION** for exact
citation: shooting-performance research consistently treats aim-trace /
hold-stability instrumentation (e.g. SCATT, Noptel-type systems) as measuring
something the target face cannot, which is precisely why those systems exist
alongside electronic targets.

**How Tech Aim uses it.** No verdict may name a technique fault. The wide-
groups verdict refers the athlete to Group Pattern Coach or an aim-trace tool
instead of asserting a cause.

**What it does not prove.** It does not prove wind had no effect. It proves
this data cannot isolate one.

---

## 2. Feedback about results helps — and can also create dependence

**Claim.** Augmented feedback (telling a performer about their result) aids
skill acquisition, but feedback given too frequently or too immediately can
degrade *retention*: the learner comes to rely on the external signal instead
of their own internal reference.

**Status:** ESTABLISHED as a field position; **NEEDS VERIFICATION** for
citation. This is commonly associated with the *guidance hypothesis*
(Salmoni, Schmidt & Walter, mid-1980s) and with Schmidt & Lee's motor-learning
textbook treatment of knowledge of results. **I have not verified the year,
wording or exact findings**, and they must be checked before publication.

**How Tech Aim uses it.** Wind Map is a **post-session review** programme, not
a live coaching feed. Nothing is asserted during shooting. The verdict always
ends by proposing the athlete's own *next test*, rather than a correction to
apply now.

**What it does not prove.** It does not establish an optimal feedback
frequency for shooting, and Wind Map does not claim one.

---

## 3. Small-bore rifle at 50 m is genuinely wind-sensitive

**Claim.** A .22 LR bullet at 50 m has a comparatively long flight time and
low velocity, so lateral air movement can measurably displace the group
centre. This is why range flags exist and why wind reading is coached.

**Status:** ESTABLISHED in practice; **NEEDS VERIFICATION** for any numeric
claim.

**How Tech Aim uses it.** It justifies recording wind at all, and justifies
treating an observed group-centre difference as *worth investigating*.

**What it does not prove — and this is the important part.** It does **not**
license converting an observed displacement into a predicted correction.
Published ballistic tables assume a specific bullet, muzzle velocity,
uniform full-value wind and a stable shooter. A training session has none of
those guarantees. **A manufacturer's table is not evidence about this
athlete's group**, and Wind Map never presents one.

---

## 4. Within-athlete repeatability is what makes a pattern real

**Claim.** A difference observed once, in one session, under conditions the
athlete themselves estimated, is an *observation*. It becomes evidence of a
repeatable pattern only when the same directional difference appears again in
a comparable, separately completed session.

**Status:** REASONING, resting on ESTABLISHED statistical principle.

The reasoning: a single session confounds condition with everything that
drifts over a session — fatigue, position settling, ammunition lot, light,
the athlete's own adaptation. Repetition across sessions is what separates a
condition-associated effect from session drift.

**How Tech Aim uses it.** The `REPEATED` evidence level is **reserved** and
can never be assigned from one session. A single-session comparison can reach
`COMPARATIVE` at most, and the wording says the pattern should be repeated
before any strategy changes.

**What it does not prove.** It does not tell us how many sessions are enough.
Wind Map does not claim a number.

---

## 5. Association is not causation

**Claim.** Two groups differing while two different conditions were *recorded*
shows association. It does not show the condition caused the difference.

**Status:** ESTABLISHED (elementary statistical principle).

Specific to Wind Map, three confounds are always present:

1. **The condition is an athlete estimate**, not a measurement, and covers the
   firing point rather than the whole bullet path.
2. **Conditions are not randomly assigned.** They occur in time order, so any
   drift over the session is perfectly confounded with condition order.
3. **Sample sizes are small** — five to fifteen shots per group is normal.

**How Tech Aim uses it.** No verdict says *caused*, *because of* or *due to*.
The permitted construction is *"observed alongside"* / *"while this condition
was recorded"*. A prohibited-phrase test enforces this.

---

## 6. Why a coach must confirm before a sight or hold change

**Claim.** A sight or hold change based on an unrepeated, small-sample,
self-estimated observation can degrade performance — it may encode session
drift as a permanent offset.

**Status:** REASONING.

**How Tech Aim uses it.** The `COMPACT BUT OFFSET` verdict carries an explicit
**coach decision** line, and the software never proposes a click value, a
hold-off or an aiming point. That boundary is also a product-safety one: the
software has no access to the athlete's equipment, zero history or ammunition,
so it is not in a position to recommend a change to any of them.

---

## 7. What this evidence base does NOT support

Recorded plainly, so a future phase cannot quietly cross these lines:

- Predicting a correction in clicks or mils from an observed displacement.
- Claiming a specific probability, confidence interval or significance level.
  **No inferential statistic is computed**, so none may be reported.
- Attributing a single shot to wind. A single shot has no group centre.
- Comparing one position against another as a wind experiment: the positions
  differ in stability demand *and* were shot at different times under
  different conditions.
- Treating "No reading" as calm, or a measured 0.0 m/s as calm.

---

## 8. Verification checklist before this document is published

- [ ] Confirm the guidance-hypothesis citation (authors, year, journal) and
      that the retention claim is stated as summarised in §2.
- [ ] Confirm a citable source for aim-trace instrumentation measuring what
      target-face data cannot (§1).
- [ ] Decide whether any numeric wind-deflection figure is quoted at all; if
      so, source it and state its assumptions (§3).
- [ ] Have a coach review §6 for practical accuracy.
- [ ] Re-check that every rule in
      `docs/training-lab-wind-map-verdict-rules.md` still follows from a
      surviving claim.

Until every box is ticked, this document is **internal design rationale**, not
a citable reference.
