# Stage 5.2 + 6.1 â€” Training shell boundary and Wind Map analysis review

**Phase:** STAGE 5.2 Â· STAGE 6.1 Â· **Date:** 2026-07-29
**Status:** implemented; automated evidence complete, **human visual check outstanding**

---

## 1. Stage 5.2 â€” the shared Training shell boundary

### 1.1 What was wrong

`ShootingPage.qml`'s `statusStrip` is the **competition** top bar. Each of its
rows grew its own gate as each Training programme landed, and the terms fell
out of step:

| Row | Gate before 5.2 | Technical Blocks | Call & Diagnose | Position Transition |
|---|---|:--:|:--:|:--:|
| Identity (`currentGameDisplay` + `currentmatchDisplay`) | `!isWindMapMatch` | **LEAKED** | **LEAKED** | **LEAKED** |
| Phase stepper | `â€¦&& !isTrainingMatch && !isWindMapMatch` | gated | **LEAKED** | **LEAKED** |
| Official shot counter | `â€¦&& !isTrainingMatch` | gated | **LEAKED** | **LEAKED** |
| Phase chip | `â€¦&& !isTrainingMatch` | gated | **LEAKED** | **LEAKED** |

The identity row carries **"FINAL 35"** â€” `currentGameDisplay1/2` hold whatever
the last selected event card set. **All four programmes were exposed to the
headline symptom**, which is why Technical Blocks was registered as
UI-TRAIN-003 despite not being in the brief.

### 1.2 The fix

**One boundary, asked once.** Every competition row in the band is gated on
`isTrainingModeAny`, which already means *any Training Lab programme is
active*. No per-programme `!isXMatch` term remains in the band â€” a test
asserts that, so the gates cannot drift apart again.

`TrainingTopBar.qml` replaces `WindMapTopBar.qml` and serves all four
programmes. It **reads no controller directly**: programme, athlete,
discipline, position, phase and progress are all passed in by whichever
programme owns the screen. That is what makes one shared bar safe rather than
four copies that drift.

It occupies the same 42 px band, so every anchor chaining from
`statusStrip.bottom` is unchanged and no layout moved. Every Training capture
screen now states **NOT AN OFFICIAL COMPETITION RESULT**.

**No Finals screen changed.** `FinalsHud`, `Finals10mHud` and
`Finals10mRightPanel` keep their original gates; a test asserts no Training
binding leaked into either HUD.

---

## 2. Stage 6.1 â€” the analysis review

### 2.1 One calculation authority

`WindMapAnalyticsEngine` (pure QtCore) is the only place a metric is computed.
`WINDMAP.analysisModel()` projects its output into a QVariantMap; the analysis
view formats that map. **QML computes nothing** â€” a test scans the view for
`Math.sqrt`, running sums, `mean`/`average`/`stdDev` and rejects them.

```
SessionState (reducer)  ->  WindMapAnalyticsEngine  ->  analysisModel()  ->  view
                                                                         \->  PDF (6.2)
```

### 2.2 Sections implemented

1. **Session overview** â€” the tested Stage 5.1 counts, positions represented,
   and a data-quality **statement of coverage** (not a score).
2. **Condition-coloured target plot** â€” counted shots by exact condition,
   sighters hollow behind a toggle, condition filter, position filter (3P
   tabs), target centre, reference centre, per-group MPI diamonds, mean-radius
   circle where the sample supports one, legend with condition + n + colour.
   **No aim arrow.**
3. **Condition comparison table** â€” label, n, mean score, MPI X/Y, shift, mean
   radius, group diameter, H/V spread, evidence.
4. **Observed group-centre shift** â€” magnitude and plain-words direction, or
   *"Withheld â€” N more shots needed"*.
5. **Direction sectors** â€” all eight plus Calm and No reading; an unshot sector
   reads **No data**, never a zeroed statistic.
6. **Speed bands** â€” Calm Â· 0â€“2.0 Â· 2.0â€“4.0 Â· 4.0â€“7.0 Â· over 7.0 Â· No reading.
7. **Timeline** â€” shot, type, position, score, X/Y, condition, with
   condition-change and sighterâ†’counted boundary markers.
8. **3P tabs** â€” Kneeling Â· Prone Â· Standing Â· Overview.
9. **What the data suggests** â€” the engine's findings rendered verbatim with
   category, n, observation, next-session action.
10. **Actions** â€” Export PDF Â· New Wind Map session Â· Home. *Complete session*
    is no longer the only action after completion.

### 2.3 Withheld metrics

A metric whose sample does not support it is **absent from the model**, not
zero. The `has*` flag travels with it and the shortfall is emitted instead, so
no view can print a misleading number. Thresholds: MPI nâ‰¥3, dispersion nâ‰¥5,
comparison nâ‰¥5 on both sides.

### 2.4 Engine â†” view-model equality

Test 19 runs one session through **both** the engine and `analysisModel()` and
compares every position, group, metric, shift, finding, timeline entry and
limitation value by value at 1e-12. This is the evidence that the screen â€” and
the Stage 6.2 PDF â€” cannot disagree with the engine.

---

## 2a. UI-WIND-002 â€” the analysis was unreachable

**Reported:** a completed Wind Map session showed only counted-shot
information â€” no plot, MPI comparison, shift, wind rose, speed bands,
timeline, 3P tabs, findings or next-session feedback.

**Root cause.** `WindMapAnalysisView.qml` used `ScrollBar` while importing
only `QtQuick`. `ScrollBar` lives in `QtQuick.Controls`, so the type never
resolved and the component could not be created. With the analysis view
absent, the only overlay left at completion was the capture HUD's basic
review â€” counts and the raw shot table â€” which is exactly what was seen.

Confirmed with `qmllint`; the fault is **one file, one line**. The other three
Wind Map QML files report zero unresolved types.

**Not a sample-size issue.** The analysis was unreachable at any n.

### 2a.1 Why the Stage 6.1 evidence did not catch it

Three gaps, recorded plainly:

1. The Stage 6.1 guards were **static string checks** over the QML source.
   A file that never loads still contains all the right strings, so every
   check passed while the screen could not exist.
2. The launch check greps stderr for QML errors, but the analysis view is
   only instantiated when a session reaches **phase 6** â€” a startup launch
   never gets there. The check was real; it could not reach the code path.
3. `qmllint` was never run over the new files, though it finds this in
   milliseconds.

**What now prevents a repeat**

- An **import-coverage guard**: every `QtQuick.Controls` type used in a Wind
  Map QML file must be backed by its import.
- An **end-to-end test** (test 23) that drives the real controller through the
  full workflow and asserts the completed phase is reached and the analysis
  model is populated â€” not a string check.
- Test 24 proves a normally-created session and a recovered one take the
  **same** analysis path.
- `qmllint` over the Wind Map QML is now part of the phase checklist.

### 2a.2 Navigation

Five always-visible sections â€” **Overview Â· Target Plot Â· Conditions Â·
Timeline Â· Findings** â€” so nothing sits behind a hidden tab. For 3P the
position row reads **Kneeling Â· Prone Â· Standing Â· Session Overview**.

The first screen carries the factual overview, data-quality status, positions
represented and a **What the Data Suggests preview**, so feedback is visible
without hunting for it.

### 2a.3 Insufficient samples

The analysis opens and explains itself at any n:

- A position with nothing plottable shows a clear placeholder naming the
  thresholds, never a blank panel.
- Each condition row explains itself in words â€” *"3 shots recorded. 2 more
  shots are required for a group comparison."* â€” worded from the model's own
  `shotsNeeded` values.
- A withheld statistic renders as an em dash. **Never 0.0.**

---

## 4. Stage 6.1.1 â€” analysis UX and performance redesign

Arnold's manual review found the Stage 6.1 analysis visible and functioning but
not usable: slow or blank first paint, two competing rows of navigation, raw
technical metrics, an abstract plot, session-level findings shown under every
position, and an Export PDF button that looked implemented.

### 4.1 Measured root cause â€” not guessed

Timed on this machine over a 48-shot session (44 counted + 4 sighters):

| Path | Cost |
|---|---|
| `WindMapAnalyticsEngine::analyse()` | **0.025 â€“ 0.046 ms** per run |
| `analysisModel()` cold (build + project) | **0.434 ms**, exactly one build |
| `analysisModel()` cached | **0.0012 ms** per fetch |

**The entire C++ path costs under half a millisecond.** The delay was never the
analytics. It was QML:

| Cause | Evidence |
|---|---|
| Every section instantiated at once | sections were gated with `visible:`, which still creates every delegate |
| One delegate per shot, three times over | timeline, plot markers and appendix rows were each a flat `Repeater` |
| Plot span recomputed per binding | `plotBox.span` looped every row and each shot's `x`/`y` binding depended on it â€” O(nÂ²) evaluations |
| Canvas repaint loops | **not a cause** â€” the plot uses Items, not Canvas |

**A correction to my own first measurement.** I initially timed
`analysisModel()` in a 50-iteration loop and reported 0.007 ms as the *uncached*
cost. That was wrong: iteration 0 populated the cache and the other 49 were
cache hits, which is why it read six times faster than the engine it wraps. The
cold cost is now measured on the first call only, with the build counter
asserted at exactly 1.

### 4.2 What changed

**Performance.** `analysisModel()` memoises on `(sessionId, shot count, phase,
condition changes)` â€” a real invalidator, not a permanent cache; a test starts a
second session on the same controller and asserts the analysis is the new one's.
Each page is behind a `Loader`; Shot Details uses a virtualised `ListView`; the
plot span is recomputed on data change, not per binding. A loading state reads
*"Preparing your Wind Map analysisâ€¦"* instead of a blank frame.

**Navigation (UI-WIND-004).** Five section pills over a second identical row
become **three pages** â€” SUMMARY Â· COMPARE CONDITIONS Â· SHOT DETAILS â€” plus
**one** labelled position filter (Session Overview / Kneeling / Prone /
Standing) captioned *"filters the page below"*. Tech Aim red marks the selected
page and nothing else in the navigation.

**Plain language (UI-WIND-007).** Summary answers *What happened Â· What this
means Â· Next training step Â· Evidence*. Coordinates read as **right/left** and
**high/low** â€” never a signed X/Y. Technical values sit behind *SHOW TECHNICAL
MEASUREMENTS*, each with a definition.

**Target (UI-WIND-005).** `WindMapTargetPlot` adds reference rings, a centre
cross, HIGH/LOW/LEFT/RIGHT labels, a millimetre scale marker, and a legend
defining **every** marker â€” including the hollow sighter ring that was
previously unexplained. No aiming arrow.

**Scoped findings (UI-WIND-006, P0).** `Finding` now carries `FindingScope`
(Session Â· Position Â· Condition) with the position and condition it belongs to,
tagged where each finding is raised. A position selection shows only that
position's findings; the cross-position comparison is Session-scoped and
labelled **SESSION-LEVEL POSITION COMPARISON**. This is an addition to the
record â€” no formula and no threshold changed, and a test re-asserts the
hand-checked dispersion answers to prove it.

**PDF (UI-WIND-008).** A subdued, disabled *PDF â€” COMING NEXT* with no
MouseArea and no signal.

### 4.3 Evidence

| Suite | Result |
|---|---|
| Reliability | **2042 / 0** |
| Training Â· Finals 10m Â· 3P Finals Â· Docs | 567 / 143 / 233 / 204, all 0 failures |
| `qmllint` | 0 unresolved types across all Wind Map QML |
| Launch check | app loads, Demo, isolated profile, no Wind Map QML warning |

A stale `onExportPdfRequested` binding in `ShootingPage` failed the **entire**
QML engine load â€” caught by the launch check, which is exactly the gap that let
UI-WIND-002 through.

### 4.4 Visual status

**MANUAL-ASSISTED VISUAL CHECK REQUIRED â€” the redesign has not been seen.**
UI-WIND-003â€¦008 remain **OPEN**. Nothing here is approved on automated evidence.

Still to review at **1536 Ã— 960** (the only resolution ever opened): Summary,
Compare Conditions, Shot Details, Session Overview, Kneeling, Prone, Standing,
the loading state, first and subsequent target renders, a 40+ shot timeline,
insufficient samples, one condition only, and multiple conditions.

**On-screen timings are NOT measured.** The figures above are the C++ path only.
First-paint and page-switch timings need the running application, and the
1-second Summary target is therefore **unverified**.

---

## 3. Evidence

| Suite | Result |
|---|---|
| Reliability | **2042 / 0** |
| Training | 567 / 0 |
| Finals 10m | 143 / 0 |
| 3P Finals | 233 / 0 |
| Documentation | 204 / 0 |
| Application build | clean |
| Launch check | no new QML error or warning |

### 3.1 Visual status

**UI-WIND-001 is CLOSED** â€” Arnold approved the corrected **50 m 3P capture
workflow** at **1536 Ã— 960 logical**, build `5404585`. 50 m Prone was not
opened and the full 40-shot layout was not fired; neither is claimed.

**The analysis review itself has still not been seen.** UI-WIND-002 stays
**OPEN â€” HUMAN VISUAL EVIDENCE** until the corrected build is reviewed.

**MANUAL-ASSISTED VISUAL CHECK REQUIRED.** Automatic capture remains blocked by
endpoint security; no synthetic-input or antivirus workaround was attempted.
The launch check proves the QML types resolve, instantiate and emit no errors â€”
it does not prove the analysis reads correctly.

Needing a human at the machine, **none yet reviewed**:

- Prone analysis Â· 3P Kneeling Â· 3P Prone Â· 3P Standing Â· Overview
- A condition with insufficient samples (the withheld state)
- A 40+ shot session (timeline and table length)
- Long condition labels
- All filters (condition, sighters, position tabs)
- All graphs (plot, sectors, speed bands)

**Resolutions:** only **1536 Ã— 960 logical** has been opened at all, and that
was for the Stage 5.1 capture screen. Every other resolution is **NOT TESTED**.

---

## 4. Work remaining for the PDF (Stage 6.2)

The model is ready and asserted equal to the engine. Outstanding:

- `WindMapReportView.qml` on the shared Tech Aim report components (white A4)
- Header, athlete/session details, summary, data quality, plot, MPI comparison,
  direction and speed analysis, 3P position pages, timeline, what the data
  suggests, next session, limitations, **raw shot appendix**, and the
  not-an-official-result line
- `grabToImage` pages â†’ `CUSTOMPRINT.create*Pdf`, harvesting `result.image`
  **inside** each grab callback
- Equality assertions extended to the PDF data model
- Status **GENERATED â€” HUMAN VISUAL CHECK REQUIRED** until a real PDF is
  rendered and inspected

**No separate PDF calculation may be introduced.** The report consumes
`analysisModel()`.

---

## 5. Stage 6.1.3 - verdict UI, verified research, relative direction

**Research.** All four citations verified against the publisher or PubMed
record. Two claims did NOT survive and were removed: an uncited assertion
about aim-trace instrumentation, and any numeric wind-deflection figure for
.22 LR at 50 m. See `docs/research/wind-map-feedback-evidence.md`.

**Thresholds.** All nine classified `REASONED PRODUCT RULE - COACH REVIEW
REQUIRED`. None is research-validated. The 40 mm rule is retained as
`PROVISIONAL TECH AIM RULE` with relative wording (Option A) - reasoning in
`docs/training-lab-wind-map-verdict-rules.md` section 3.6.

**Verdict UI.** Summary renders one prioritised primary verdict plus
secondary observations; each condition card carries its own verdict; a
position selection excludes session-scoped verdicts.

**Firing direction.** Optional, never mutates the recorded compass value,
derived on read, absent by default in every existing journal.

**Evidence:** reliability 2126/0, training 567/0, finals10m 143/0,
3P finals 233/0, docs 204/0. Ten seeded review cases on disk.

### 5.1 Visual status

**HUMAN VISUAL CHECK REQUIRED.** No verdict text has been seen on screen.
UI-WIND-006 remains OPEN pending a 3P position cycle. The verdict language is
NOT approved. No coach has reviewed any threshold.