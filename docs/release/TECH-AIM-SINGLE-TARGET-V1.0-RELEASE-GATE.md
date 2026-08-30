# Tech Aim Single Target — Windows v1.0 release gate

**Verdict: GREEN. Blockers 0.**

Assembled 2026-08-30 at the close of Phase A. Every line below was checked
against a current test run, a current source read, or a current artefact —
none of it against memory.

---

## 1. Blockers

| | |
|---|---|
| **BLOCKER F** — explicit sighter/official persistence | **CLOSED** |
| **BLOCKER G** — the 10 m Final report, view and PDF | **CLOSED** |
| **A** — UI-LASTSHOT-DWELL-001 | **CLOSED** |
| **B** — CRO-ORDER-001 | **CLOSED** |
| **C** — CRO-REPEAT-002 | **CLOSED** |
| **D** — position-change sighter tagging | **CLOSED** by F |
| **OPEN BLOCKERS** | **0** |

Full record, with what each one was and what closed it:
[`V1.0-RELEASE-BLOCKER-INVENTORY.md`](V1.0-RELEASE-BLOCKER-INVENTORY.md).

---

## 2. The physical baseline this release inherits

**RC3F-DIAG, commit `39df782`** — 2026-08-29, three tablets, three athletes,
live 50 m targets, ISSF 50 m Rifle 3 Positions Indoor Qualification and 3P
Final.

| | |
|---|---|
| Accepted physical shots | **385** |
| Distinct coordinates | **385** |
| Feed requested / started / completed | **385 / 385 / 385** |
| Acquisition faults, read failures, counter jumps, desyncs | **0** |
| Qualifications completed | 3 × 60 official |
| Finals completed | 3 |
| Mid-qualification reconnect | 1, reconciled, session completed |

Detail: [`RC3F-FIELD-BASELINE.md`](RC3F-FIELD-BASELINE.md).

**No additional live target test was performed for this close-out.** That is a
recorded release decision, not an omission:
[`V1.0-PHYSICAL-EVIDENCE-INHERITANCE.md`](V1.0-PHYSICAL-EVIDENCE-INHERITANCE.md).

### Why the inheritance holds

Every post-RC3F change is confined to presentation, command text, persistence
metadata and reporting. None can alter a coordinate, a score, a clock or a shot
count. Verified per commit: the diffs since `39df782` touch `CenterPane.qml`,
`ShootingPage.qml`, `ReportWindow.qml`, `WindowManager.qml`, the report views,
`src/finals10m/`, `src/app/BrandPackage.*`, `customprint.*`, `tests/`,
`tools/` and `docs/` — and nothing else.

**If acquisition, scoring, Modbus, counter reconciliation, paper feed,
competition timing or the 3P state machines are ever modified, this evidence
stops applying** and a new physical test is required before any physical claim
may be repeated.

---

## 3. Post-RC3F changes, and how each was validated

| Change | Validation |
|---|---|
| `UI-LASTSHOT-DWELL-001` — 2.5 s presentation hold | **AUTOMATED.** 22 checks: what stays immediate, what may be held, that the deferred half touches no state, that a shot during the hold collapses it first |
| `CRO-ORDER-001` — STOP before MATCH FIRING | **AUTOMATED.** Asserted by position within the sighting-expiry block |
| `CRO-REPEAT-002` — one MATCH FIRING per session | **AUTOMATED.** Latch asserted, and asserted not re-armed by a position change |
| Appearance: System / Light / Dark | **AUTOMATED**, plus verified persisting across restart from the running binary |
| **F** — explicit `shotRole` persistence | **AUTOMATED ROUND TRIP**, AR and AP: the journal is reduced through `JournalValidator` + `ReplayEngine`; officials survive as officials, sighters as sighters, total unchanged, persisted total is the officials alone |
| **G** — the 10 m Final report | **AUTOMATED + REPLAY + RENDER.** The report DTO is compared field by field and row by row before and after reduction; the real DTO from a complete AR and AP Final is rendered to an A4 page and inspected |
| Journalled shot time | **AUTOMATED.** The defect below was found by the round-trip comparison and is asserted row by row |
| Teiler removal | **AUTOMATED.** No Tech Aim view contains the word or calls the accessor; the shot table's columns still sum to 100% and still align |

### The defect this round found

The 10 m controller measured each shot's split for the screen, then measured it
**again** for the journal — after the clock anchor had already advanced. Every
persisted shot time was therefore near zero, and every recovered session
reported 0 s. **Live operation was never affected**; the number the operator
saw was always right. Measured once now. See the report matrix, note ⁵.

---

## 4. Report and persistence

Full matrix, per mode, distinguishing recovery from completed-result access:
[`V1.0-REPORT-PERSISTENCE-MATRIX.md`](V1.0-REPORT-PERSISTENCE-MATRIX.md).

Routing, which is what BLOCKER G was really about:

| Session | Report | Cross-routing possible? |
|---|---|---|
| 10 m AR Final | `Finals10mReportView` | no |
| 10 m AP Final | `Finals10mReportView` | no |
| 50 m 3P Final | `FinalsReportView` | no |
| Qualification | Summary + Match | no |
| Open Practice | Summary + Match ¹ | no |
| Training Lab | own renderers, own PDFs | never enters this window |

¹ There is no separate practice report and there never was. Open Practice
claims no ISSF course; this is design, not a gap.

`prepare()` pins a finals session to its own family's tab whatever the caller
asked for, so this is a property of the window rather than of its callers.

---

## 5. Teiler

**Operator-facing occurrences in Tech Aim: 0.**

Removed from four views; the measurement retained untouched in `TachusWidget`;
the decision recorded as `BrandPackage::showTeilerMetric` (false for Tech Aim,
and false for the reserved OEM package because no SETA requirement exists).
`UI-DEC-018` and [`Teiler_Presentation_Decision.md`](../design/Teiler_Presentation_Decision.md).

One occurrence remains in source, in `TachusWidget::getPDFString()`, whose only
caller reaches it after an unconditional `return`. Dead code inside the
acquisition freeze — documented rather than edited.

---

## 6. Automated regression

**TOTAL: 6 948 checks, 0 failures.**

| Suite | Checks | Failures | Was |
|---|---|---|---|
| Reliability (events, journal, reducer, replay, recovery, storage, acquisition integrity, logging, target hardware, brand) | **2 602** | 0 | 2 598 |
| Training Lab | **568** | 0 | 568 |
| 50 m 3P Finals | **379** | 0 | 379 |
| **10 m Finals** | **229** | 0 | **193** |
| **QML** | **440** | 0 | **398** |
| Manuals | **1 514** | 0 | 1 479 |
| Project memory | **228** | 0 | 220 |
| Training Lab evidence | **903** | 0 | 903 |
| Deployment package | **85** | 0 | 85 |

Headless C++ harnesses were run with the Qt `bin` on `PATH` and
`QT_QPA_PLATFORM=offscreen`, and each `=== N checks, M failures ===` line was
read — never inferred from an exit code.

### The one check that is not green, and why it is not a blocker

`tests/docs/check_generated_manuals.py` requires the manual build manifest's
`documentationSourceCommit` to equal current `HEAD`. The manuals were last
built at `488d506`, **57 commits before this round began**, and the check has
been red ever since — before this work, and not because of it.

The gate is structurally unsatisfiable in a clean tree: building the manuals
writes a manifest naming the current HEAD, and committing that manifest moves
HEAD past it. It is documentation provenance. It touches no scoring,
acquisition, rule, persistence or report behaviour, and so does not meet the
blocker standard. **Recorded as documentation-only, not fixed for v1.0.**

---

## 7. Runtime

| | |
|---|---|
| TypeError | **0** |
| ReferenceError | **0** |
| New acquisition warnings | **0** |
| Pre-existing binding warnings | **7, unchanged** |

The seven are `CoachDetailedView.qml:349,:364`, `CoachPrintView.qml:278,:310,
:320,:342` and `IncidentWindow.qml:335` — undefined bindings in coach and
incident views, unchanged across every build since RC3B. None is in an
acquisition, scoring, timing or competition path. **Accepted limitation,
documented, deliberately not fixed during a freeze.**

---

## 8. Rule authority

**ISSF Rule Book 2026, Edition 2025, Second Print 07/2026, effective 1 July
2026.** Verified from the official PDF on 2026-08-27 and frozen for v1.0.

| Discipline | Rule |
|---|---|
| 10 m AR/AP Final | 6.17.2 |
| 50 m 3P Final | 6.17.3 |
| 50 m 3P Qualification (indoor, 1:30:00) | 6.11.9.2 |
| Range commands, including the 10- and 5-minute warnings | 6.11.1.1 / 6.11.1.2(e) / 6.11.1.3 |
| EST malfunctions and interruptions | `docs/issf-rules/est-malfunctions.md` |

A future announcement, LA28 included, must **not** alter a currently effective
course.

---

## 9. Accepted limitations

Each is recorded, bounded, and none meets the blocker standard.

| | Why it is accepted |
|---|---|
| The `.tch` does not carry the shot role | The authoritative journal always has. Changing a persistence format during a freeze is not a v1.0 act. |
| A completed Final cannot be reopened *as a Final* | The result is persisted, and viewable and exportable at completion — which is v1.0's contract. A history browser was never designed or promised, and the crash-recovery path must not be repurposed as one. |
| 3P position-change sighters tagged `(counted)` in the backend | Course interpretation is correct; the tag only complicates forensic reading. |
| Tie-breaking (6.17.3 i, 6.17.2 b) | Every clause compares athletes across lanes. A single-target application cannot decide it and must not fabricate a ranking. Belongs to Range Management. |
| Seven binding warnings | See §7. |
| Generated-manual provenance | See §6. |
| `UI-LAYOUT-001` | Instrumented, never reproduced; no screenshot from RC3B/C/D/F shows it. Reopen only with evidence. |
| No visual approval of the new report pages | The pages are rendered evidence, not an operator sign-off. Stated, not glossed. |

---

## 10. What this gate does NOT claim

- ~~"All v1.0 features physically tested."~~ They were not.
- ~~"The 10 m Final report was physically tested."~~ Its underlying shots come
  from a physical baseline; **the report path itself is replay- and
  render-validated.**
- ~~"The 3P Final state flow is field-verified."~~ The Final *completed* in the
  field; its internal stage flow was not captured, because the journals were
  not collected.
- ~~"An operator has approved the new report pages."~~ No one has seen them on
  the machine.
- Any ranking, placing, medal or elimination result. One lane cannot know them.

---

## 11. Verdict

**BLOCKERS: 0. GATE: GREEN.**

Windows **1.0.0-RC1** was built, packaged and portably validated against this
gate. Its identity and hashes are frozen in
[`TECH-AIM-SINGLE-TARGET-V1.0-FREEZE.md`](TECH-AIM-SINGLE-TARGET-V1.0-FREEZE.md).

Scope of this release line is Windows, single target. SETA, Android and the RMS
interface are **not** started and are not covered by any statement above.
