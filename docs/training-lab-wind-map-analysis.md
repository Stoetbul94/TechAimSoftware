# Stage 5.2 + 6.1 — Training shell boundary and Wind Map analysis review

**Phase:** STAGE 5.2 · STAGE 6.1 · **Date:** 2026-07-29
**Status:** implemented; automated evidence complete, **human visual check outstanding**

---

## 1. Stage 5.2 — the shared Training shell boundary

### 1.1 What was wrong

`ShootingPage.qml`'s `statusStrip` is the **competition** top bar. Each of its
rows grew its own gate as each Training programme landed, and the terms fell
out of step:

| Row | Gate before 5.2 | Technical Blocks | Call & Diagnose | Position Transition |
|---|---|:--:|:--:|:--:|
| Identity (`currentGameDisplay` + `currentmatchDisplay`) | `!isWindMapMatch` | **LEAKED** | **LEAKED** | **LEAKED** |
| Phase stepper | `…&& !isTrainingMatch && !isWindMapMatch` | gated | **LEAKED** | **LEAKED** |
| Official shot counter | `…&& !isTrainingMatch` | gated | **LEAKED** | **LEAKED** |
| Phase chip | `…&& !isTrainingMatch` | gated | **LEAKED** | **LEAKED** |

The identity row carries **"FINAL 35"** — `currentGameDisplay1/2` hold whatever
the last selected event card set. **All four programmes were exposed to the
headline symptom**, which is why Technical Blocks was registered as
UI-TRAIN-003 despite not being in the brief.

### 1.2 The fix

**One boundary, asked once.** Every competition row in the band is gated on
`isTrainingModeAny`, which already means *any Training Lab programme is
active*. No per-programme `!isXMatch` term remains in the band — a test
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

## 2. Stage 6.1 — the analysis review

### 2.1 One calculation authority

`WindMapAnalyticsEngine` (pure QtCore) is the only place a metric is computed.
`WINDMAP.analysisModel()` projects its output into a QVariantMap; the analysis
view formats that map. **QML computes nothing** — a test scans the view for
`Math.sqrt`, running sums, `mean`/`average`/`stdDev` and rejects them.

```
SessionState (reducer)  ->  WindMapAnalyticsEngine  ->  analysisModel()  ->  view
                                                                         \->  PDF (6.2)
```

### 2.2 Sections implemented

1. **Session overview** — the tested Stage 5.1 counts, positions represented,
   and a data-quality **statement of coverage** (not a score).
2. **Condition-coloured target plot** — counted shots by exact condition,
   sighters hollow behind a toggle, condition filter, position filter (3P
   tabs), target centre, reference centre, per-group MPI diamonds, mean-radius
   circle where the sample supports one, legend with condition + n + colour.
   **No aim arrow.**
3. **Condition comparison table** — label, n, mean score, MPI X/Y, shift, mean
   radius, group diameter, H/V spread, evidence.
4. **Observed group-centre shift** — magnitude and plain-words direction, or
   *"Withheld — N more shots needed"*.
5. **Direction sectors** — all eight plus Calm and No reading; an unshot sector
   reads **No data**, never a zeroed statistic.
6. **Speed bands** — Calm · 0–2.0 · 2.0–4.0 · 4.0–7.0 · over 7.0 · No reading.
7. **Timeline** — shot, type, position, score, X/Y, condition, with
   condition-change and sighter→counted boundary markers.
8. **3P tabs** — Kneeling · Prone · Standing · Overview.
9. **What the data suggests** — the engine's findings rendered verbatim with
   category, n, observation, next-session action.
10. **Actions** — Export PDF · New Wind Map session · Home. *Complete session*
    is no longer the only action after completion.

### 2.3 Withheld metrics

A metric whose sample does not support it is **absent from the model**, not
zero. The `has*` flag travels with it and the shortfall is emitted instead, so
no view can print a misleading number. Thresholds: MPI n≥3, dispersion n≥5,
comparison n≥5 on both sides.

### 2.4 Engine ↔ view-model equality

Test 19 runs one session through **both** the engine and `analysisModel()` and
compares every position, group, metric, shift, finding, timeline entry and
limitation value by value at 1e-12. This is the evidence that the screen — and
the Stage 6.2 PDF — cannot disagree with the engine.

---

## 3. Evidence

| Suite | Result |
|---|---|
| Reliability | **1953 / 0** |
| Training | 567 / 0 |
| Finals 10m | 143 / 0 |
| 3P Finals | 233 / 0 |
| Documentation | 204 / 0 |
| Application build | clean |
| Launch check | no new QML error or warning |

### 3.1 Visual status

**MANUAL-ASSISTED VISUAL CHECK REQUIRED.** Automatic capture remains blocked by
endpoint security; no synthetic-input or antivirus workaround was attempted.
The launch check proves the QML types resolve, instantiate and emit no errors —
it does not prove the analysis reads correctly.

Needing a human at the machine, **none yet reviewed**:

- Prone analysis · 3P Kneeling · 3P Prone · 3P Standing · Overview
- A condition with insufficient samples (the withheld state)
- A 40+ shot session (timeline and table length)
- Long condition labels
- All filters (condition, sighters, position tabs)
- All graphs (plot, sectors, speed bands)

**Resolutions:** only **1536 × 960 logical** has been opened at all, and that
was for the Stage 5.1 capture screen. Every other resolution is **NOT TESTED**.

---

## 4. Work remaining for the PDF (Stage 6.2)

The model is ready and asserted equal to the engine. Outstanding:

- `WindMapReportView.qml` on the shared Tech Aim report components (white A4)
- Header, athlete/session details, summary, data quality, plot, MPI comparison,
  direction and speed analysis, 3P position pages, timeline, what the data
  suggests, next session, limitations, **raw shot appendix**, and the
  not-an-official-result line
- `grabToImage` pages → `CUSTOMPRINT.create*Pdf`, harvesting `result.image`
  **inside** each grab callback
- Equality assertions extended to the PDF data model
- Status **GENERATED — HUMAN VISUAL CHECK REQUIRED** until a real PDF is
  rendered and inspected

**No separate PDF calculation may be introduced.** The report consumes
`analysisModel()`.
