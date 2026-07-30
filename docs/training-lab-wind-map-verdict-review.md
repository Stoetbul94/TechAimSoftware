# Wind Map — Stage 6.1.4 verdict language review sheet

**Status:** AWAITING ARNOLD'S REVIEW · **Prepared:** 2026-07-30
**Analytics version:** `windmap-analytics-v2` (all ten cases confirmed)

Nothing below is an approval. Every result was read from the application's own
`analysisModel()` — the same model the screen consumes — so this sheet states
what the app **will** show, not what it is expected to show.

---

## How to run the review

The application is already running against an isolated Demo capture profile
holding ten seeded sessions. To restart it:

```bash
cd /c/Users/User/Downloads/TechAimSoftware-repo/seta10/release && ./TechAim.exe --documentation-capture --data-root "C:/Users/User/AppData/Local/Temp/TechAimWindMapReview614"
```

For each case: **resume it from the recovery dialog → press "Complete session"
→ the analysis opens** with that case's verdict as the primary result.

**Nothing in the seeded data is fabricated.** Each session was written by
driving the real `WindMapController` — same events, same hash chain, same
reducer state as a live session. The seeder creates *data*, never screen input.

### Why re-seeding was necessary

The journals store **shot records only**. The analysis version is not stored;
it is computed at read time, so any journal is re-analysed by whatever engine
reads it. Older seeds would therefore have been silently re-analysed under v2
with no visible marker. To remove that ambiguity the profile was **wiped and
re-seeded**, and each session now reports its analytics version in the sheet
below and in the application's session block.

---

## The ten cases, with what each produces

| # | Case | Category the app produces | Scope badge | Verdicts |
|---|---|---|---|---|
| 1 | **C** — Insufficient sample | Insufficient sample | CONDITION: N · 1.5 m/s | 1 |
| 2 | **H** — Fragmented data | Conditions changed too often | PRONE | 1 |
| 3 | **I** — One condition, Indicative | No valid comparison | CONDITION: W · 2.0 m/s | 1 |
| 4 | **D** — Compact but offset | Compact but offset | CONDITION: W · 2.0 m/s | 1 |
| 5 | **E** — Wider under condition | Wider under a condition | CONDITION: NE · 5.0 m/s | 1 |
| 6 | **F** — Similar across conditions | Similar across conditions | PRONE | 1 |
| 7 | **G** — Dispersion elevated | Dispersion elevated across conditions | PRONE | 2 |
| 8 | **J** — 3P position-specific | Position difference | **SESSION-LEVEL POSITION COMPARISON** | 4 |
| 9 | **I** — Missing firing direction | *(as case 3; no firing direction recorded)* | | |
| 10 | **D · E · J** — Valid firing direction | *(firing direction recorded: D and E north, J east)* | | |

Two further sessions are present for context and are **not** part of the ten:
**A** (44 counted shots, four conditions, sighters, long condition note) and
**B** (3P across all three positions).

**Case C was rebuilt for this stage.** It previously carried three conditions,
which made it produce *Conditions changed too often* rather than *Insufficient
sample* — the same verdict as case H. It now has exactly two conditions: a
well-sampled calm reference and a second condition three shots short of the
comparison minimum.

---

## What to check in every case

Each verdict should read as six things an athlete can follow without knowing
what radial RMS, covariance or a coefficient of variation is:

| Field | What it should do |
|---|---|
| **WHAT HAPPENED** | A factual observation, in plain words |
| **EVIDENCE** | Sample counts and the evidence level, with its plain-language explanation |
| **WHAT IT MAY MEAN** | A cautious reading that separates observation from cause |
| **NEXT TRAINING STEP** | Something the athlete can actually go and do |
| **COACH DECISION** | Where applicable — the boundary that belongs to a coach |
| **LIMITATIONS** | What this data cannot establish |
| **Scope** | Session, Position or Condition — visible on the badge |

Technical values are allowed in the **measurements** line beneath, not in the
headline.

---

## Case-specific checks

### 1 · C — Insufficient sample
Must state **all four**: how many shots were recorded, how many are required,
how many more are needed, and that **no comparison has been made**.

### 2 · H — Fragmented data
Must explain that **conditions were changed too frequently** for a useful
comparison — not merely that samples were small.

### 3 · I — Indicative
Must describe the group that exists **without implying a condition difference
has been proven**. Only one condition was recorded, so there is nothing to
compare against, and the verdict must say so.

### 4 · D — Compact but offset
Must state the observed group-centre difference, **both** sample counts, that a
**repeated pattern has not yet been established**, and that the conditions
should be repeated **before** changing sight or hold strategy.

### 5 · E — Wider under condition
Must say **dispersion was greater**, give **how many shots supported each
group**, and state that the software **cannot determine** whether wind, timing,
hold or another factor produced it. It must **not lead with "radial RMS"** —
that belongs in the measurements line.

### 6 · F — Similar across conditions
Must **not** claim wind had no effect. The correct reading is that this session
did not establish a pattern, which is not the same as showing there is none.

### 7 · G — Dispersion elevated
Must use **"Dispersion remained elevated across the recorded conditions."**
Must **never** say poor, bad, weak, unacceptable or technically inadequate.
Must state that the rule behind it is a **provisional Tech Aim training rule**.

### 8 · J — 3P position result · **required to close UI-WIND-006**
Cycle **Session Overview → Kneeling → Prone → Standing** and confirm:

- the session-level comparison is labelled **SESSION-LEVEL POSITION COMPARISON**;
- Kneeling shows Kneeling-specific feedback;
- Prone shows Prone-specific feedback;
- Standing shows Standing-specific feedback;
- **the session-level verdict is not repeated as a position verdict.**

### 9 · I — Missing firing direction
Must show: *"Relative wind direction unavailable — firing direction was not
recorded."* **No direction may be inferred.**

### 10 · D, E, J — Relative firing direction
Must show the **original compass wind direction**, the **athlete-relative
direction** derived from it, and the original recorded value must be
**unchanged**. D and E fire north (0°); J fires east (90°).

---

## Approval record — TO BE COMPLETED AFTER REVIEW

Nothing in this section may be filled in before Arnold reports a result.

| | |
|---|---|
| Reviewed build | |
| Resolution | |
| Sessions reviewed | |
| Verdict categories reviewed | |
| 3P positions reviewed | |
| Accepted wording | |
| Wording requiring changes | |
| Unresolved defects | |
| Reviewer | |
| Date | |

**Do not record a category, a position or a resolution that was not actually
opened.** A partial review is a valid and useful outcome; an overstated one is
not.

### Gate

- **Stage 6.2 (Wind Map branded PDF) is blocked** until Arnold approves the
  athlete-facing wording.
- **UI-WIND-006 stays OPEN** until the 3P position cycle in case 8 is confirmed.
- Formal coach review does **not** block the PDF, but every affected threshold
  remains **REASONED PRODUCT RULE — COACH REVIEW REQUIRED** until a named
  reviewer completes `docs/research/training-lab-coach-review-pack.md`.
