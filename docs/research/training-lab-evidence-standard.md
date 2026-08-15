# Tech Aim Training Lab — evidence standard

**Status:** ACTIVE · **Established:** 2026-07-30 · **Applies to:** every Training
Lab programme, current and future.

This is the single evidence policy for the Training Lab. Wind Map was the pilot;
this document generalises what Wind Map learned so that no programme can quietly
introduce a coaching diagnosis the data cannot support.

**Governed programmes:** Technical Blocks · Call & Diagnose · Group Pattern Coach
· Position Transition · Wind Map · all future Training Lab programmes.

It does **not** govern official competition scoring, ISSF match workflows, EST
malfunction handling, or Finals. Those are governed by `docs/issf-rules/`.

---

## 1. Claim classification

Every athlete-facing statement, metric, threshold and recommendation in the
Training Lab carries exactly one of these labels in
`training-lab-evidence-register.md`.

| Label | Meaning | What it permits |
|---|---|---|
| **RESEARCH-SUPPORTED** | Directly supported by a verified source, **within that source's population, discipline and stated limitations**. | May be stated as an established finding, with the population named. |
| **REASONED PRODUCT RULE** | A conservative deterministic Tech Aim rule. Useful for training. **Not externally validated.** | May drive a hedged observation and a suggested training test. |
| **COACH-APPROVED PRODUCT RULE** | A reasoned product rule reviewed and approved by a **named** qualified shooting coach, recorded in §8. | As above, plus it may be presented as coaching-endorsed practice. |
| **FUTURE VALIDATION REQUIRED** | A hypothesis, or a feature whose evidence is not yet good enough. | May be measured and displayed as raw data. **May not produce a strong athlete-facing conclusion.** |

### Rules that are not negotiable

- **A product rule is never described as scientifically proven**, validated,
  research-based, evidence-based, or an ISSF rule.
- **An association is never described as causation.** "A is associated with B"
  and "A caused B" are different claims; only the first is ever available from
  impact data.
- **A source's population is part of its claim.** A finding in 58 novices at
  10 m does not transfer to an elite athlete at 50 m by restating it.
- **Coach approval does not create scientific validation.** It upgrades
  REASONED to COACH-APPROVED. Nothing becomes RESEARCH-SUPPORTED that way.
- **Absence of a source is stated, not papered over.** Where nothing could be
  verified, the register says so and the claim is downgraded.

---

## 2. General Training Lab limitations

These apply to **every** programme and are the reason most Training Lab output
is descriptive rather than diagnostic.

1. **Shot-impact data alone normally cannot determine the exact technical cause
   of a shot.** This is not modesty — it is what the measurement is. See §2.1.
2. A group pattern may be associated with position, hold, aiming behaviour,
   trigger execution, timing, fatigue, wind, equipment, ammunition, or **several
   of these at once**.
3. Tech Aim **may** describe the pattern it measured.
4. Tech Aim **may** recommend a controlled training test.
5. Tech Aim **may not** diagnose a specific technical fault unless the required
   evidence source is actually available. Today it is not: Tech Aim has no
   aim-trace, no force-plate, no trigger-release signal and no video.
6. Sight, hold, ammunition and major technique changes remain **athlete and
   coach decisions**. The Training Lab may raise them; it may not decide them.
7. **Repeated within-athlete observations across comparable sessions are
   stronger than a single short session.** A one-session finding is provisional.
8. **Sample size and data quality are always visible** next to any conclusion
   they support.

### 2.1 Why limitation 1 has research support

Ihalainen, Kuitunen, Mononen & Linnamo (2016) analysed 13 795 shots from 40
international- and national-level air-rifle shooters. Four technical variables —
**stability of hold, cleanness of triggering, aiming accuracy and timing of
triggering** — together accounted for **81% of the variance in shooting score**.
Postural balance accounted for **less than 1% directly**.

Every one of those four dominant variables is measured by **aim-trace
instrumentation, not by the target face**. Tech Aim records where the shot
landed — the outcome of those variables, not the variables themselves. So the
information that explains most of the score is, by construction, absent from
Tech Aim's input.

That is why the Training Lab describes outcomes and proposes tests, and does not
name causes. Full record: `training-lab-evidence-register.md` source **S5**.

---

## 3. Shared feedback model

Where a programme produces a conclusion, the **underlying feedback model** must
carry equivalent fields. The UI may collapse or omit fields for brevity; it may
not invent, contradict or restate them differently.

| Field | Contains |
|---|---|
| **WHAT HAPPENED** | A factual observation from the recorded data. Measured values only. |
| **EVIDENCE** | Sample size, data quality, evidence level, and what evidence is missing. |
| **WHAT IT MAY MEAN** | A cautious interpretation that **distinguishes observation from cause**. |
| **NEXT TRAINING STEP** | A controlled exercise to gather better information or practise the relevant skill. |
| **COACH DECISION** | The boundary where technique, sight, hold or equipment decisions belong with a qualified coach. |
| **LIMITATIONS** | What the current data cannot establish. |

Wind Map implements this fully (`WindMapVerdict`). The other three programmes
carry some fields implicitly; §4 of each programme document records which.

**Do not force this structure where it makes the interface verbose.** A single
measured sentence in a block review does not need five headings. The requirement
is on the *model*, not the layout.

### 3.1 Evidence vocabulary

Wind Map's four levels are the reference vocabulary. Other programmes may use
them where the meaning fits, and **must not** reuse a level where it does not.

| Level | Meaning |
|---|---|
| **INSUFFICIENT** | Below the minimum sample. No conclusion is offered. |
| **INDICATIVE** | Enough data to describe, not enough to compare. |
| **COMPARATIVE** | Two adequately sampled sets were compared within this session. |
| **REPEATED** | The same pattern recurred across comparable sessions. **Not yet available in any programme** — cross-session comparison is not implemented. |

---

## 4. Source quality

**Prefer:** systematic reviews · peer-reviewed primary studies · official ISSF
technical material · recognised sport-science publications · official
manufacturer ballistic data for physical context · verified shooting-coach
review.

**Treat cautiously, and never as the basis of a strong athlete-facing claim:**
commercial coaching articles · informal blogs · forum discussions · unsupported
target-diagnosis diagrams · generic social-media advice.

Manufacturer ballistic information may explain **physical plausibility**. It
never proves an athlete-specific result.

### 4.1 Verification status vocabulary

The register records how far each source was actually checked. This distinction
is enforced by test, because "cited" and "read" are not the same thing.

| Status | Meaning |
|---|---|
| **VERIFIED — RECORD READ** | The publisher, PubMed or Europe PMC record was retrieved and the abstract read. Sample sizes and findings quoted here come from it. |
| **VERIFIED — INDEX ONLY** | Bibliographic details (authors, journal, volume, pages, year, DOI) confirmed against Crossref or an equivalent index. **The abstract or full text was not read**, so no sample size, effect size or quotation is taken from it. |
| **NOT VERIFIED** | Could not be confirmed. **Must not be cited**, and any claim resting on it is removed or downgraded. |

**Mathematical properties are not sources.** Where a rule follows from
arithmetic or standard statistics, the register names it as a mathematical
property and shows the derivation, rather than dressing it as research.

---

## 5. Prohibited claims

No Training Lab output — screen, PDF or manual — may:

- name a **specific technical fault** (breathing, trigger snatch, shoulder
  pressure, natural point of aim, head position, follow-through, flinch) as the
  cause of an observed impact pattern;
- issue a **sight-adjustment instruction** ("move your sights", "come up two
  clicks");
- instruct an **equipment or ammunition change**;
- describe an athlete or a group as **poor, bad, weak, inadequate or
  unacceptable**;
- present a **product rule as research, ISSF policy or a validated standard**;
- assert **fatigue** — or any other internal state — from score movement alone;
- state a conclusion whose **sample size is not shown alongside it**.

The permitted form is always the pair: *this is what was measured* → *here is a
controlled test that would tell you more*.

---

## 6. UI, PDF and manual consistency

All three surfaces consume the **same central feedback model**. Conclusions are
composed in C++ and formatted — never authored — in QML, PDF JavaScript or
manual prose.

| Surface | Rule |
|---|---|
| On-screen analysis | Formats model fields. No thresholds, no classification, no verdict text. |
| PDF report | Same model, same verdict IDs, same evidence level, same limitations. |
| Operator manual | Describes the model's behaviour. Does not restate conclusions in different words. |

Tests enforce: same verdict ID in UI and PDF models · wording sourced centrally ·
identical evidence level · identical limitations · identical coach-decision text
· prohibited claims absent.

---

## 7. Future programmes

Before **First Shot & Re-entry**, **Consistency Chain**, any mental-training
exercise, aim-trace integration, SCATT analysis, Shadow Shooting or personalised
diagnostics is implemented, it requires:

1. an evidence review;
2. claim classification for every athlete-facing statement;
3. a mathematical or behavioural specification;
4. a prohibited-claim list;
5. sample requirements;
6. coach review where the programme makes coaching recommendations;
7. test cases;
8. an athlete-facing wording review.

**No future programme may introduce unsupported coaching diagnoses silently.**
A programme document must exist in `docs/research/` before its controller ships.

Aim-trace and SCATT integration are the notable case: they would supply exactly
the variables §2.1 identifies as dominant and currently missing. If that data
arrives, several claims now barred become available — but only after this
process, not automatically.

---

## 8. Coach review

Coach review is recorded in `training-lab-evidence-register.md` §Coach review,
one entry per rule reviewed, with: reviewer name · qualification or relevant
experience · review date · programme · rule reviewed · wording reviewed ·
outcome (accepted / accepted provisionally / rejected) · requested changes ·
supporting notes.

A coach's approval converts **REASONED PRODUCT RULE** into **COACH-APPROVED
PRODUCT RULE**. It does not convert it into externally validated research.

**No coach review has taken place.** There are currently **zero**
COACH-APPROVED PRODUCT RULES in the Training Lab. Any document, screen or report
implying otherwise is wrong. **Do not invent coach approval** — an entry without
a named reviewer and a date is not an approval, and the register test fails on
one.

---

## 9. Enforcement

`tests/docs/check_training_lab_evidence.py` proves:

- every implemented programme has an evidence document;
- every athlete-facing claim in the register carries a classification;
- every threshold has a recorded origin;
- every RESEARCH-SUPPORTED claim names a source whose status is
  VERIFIED — RECORD READ;
- every product rule is labelled honestly, and no rule claims research backing
  it does not have;
- no coach approval exists without a named reviewer and date;
- no impact-only pattern produces a prohibited diagnosis;
- UI, PDF and manual wording derives from one feedback model;
- future programmes require an evidence review before implementation.

---

## 10. Related documents

- `training-lab-evidence-register.md` — the central register and source list
- `technical-blocks-evidence.md` · `call-and-diagnose-evidence.md` ·
  `group-pattern-coach-evidence.md` · `position-transition-evidence.md`
- `wind-map-feedback-evidence.md` — the pilot programme
- `../training-lab-wind-map-verdict-rules.md` — Wind Map threshold classifications
- `../training-lab-architecture.md` — how the programmes are built
