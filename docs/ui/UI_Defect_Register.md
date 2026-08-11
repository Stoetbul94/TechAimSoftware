# Tech Aim — UI Defect Register

Defects raised from Arnold's reviews of the running build. §1 covers the
Version B homepage (`UI-HOME-*`); §1a covers the Training Lab Wind Map
programme (`UI-WIND-*`).

**Closure rule.** A defect is `RESOLVED — AUTOMATED AND VISUAL EVIDENCE` only
when it has a fixed commit, passing build/tests, a real application screenshot,
and a passing acceptance-checklist line. Code changing is not closure.

**Approval status.** The homepage was reviewed on screen and approved —
**HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29** — against application
commit `d4674d0`. **Version B is accepted.** Seven defects are fully closed;
three (UI-HOME-002, 003, 004) concern *interaction* that was never driven by
hand and carry a status that says so. Details in §3.

Status values:
- `RESOLVED — AUTOMATED AND VISUAL EVIDENCE` — fix, passing checks, and the
  behaviour itself seen to work.
- `RESOLVED — AUTOMATED EVIDENCE AND VISUAL LAYOUT APPROVAL; MANUAL INTERACTION
  CHECK NOT PERFORMED` — fix, passing checks, and the *rendered result*
  approved, but the interaction the defect concerns was never driven by hand.
- `RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED`
- `PARTIALLY RESOLVED` · `OPEN` · `BLOCKED`

**Why the middle status exists.** Three of these defects are about behaviour —
scrolling, changing selection, choosing a folder. Looking at a static screen
cannot settle them. Calling them fully resolved on visual approval alone would
overstate the evidence, so they carry a status that says exactly what is and
is not known.

---

## 1. Register

| Defect ID | Screen | Description | Severity | Original evidence | Status | Fixed commit | Automated evidence | Visual evidence | Notes |
|---|---|---|---|---|---|---|---|---|---|
| UI-HOME-001 | Homepage — action bar | "READY TO START" and "Load saved session" overlapped in the lower-left | P0 | Arnold's review of the running build | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `41c09a3` | `home: the Start action sits INSIDE the bottom action bar` | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 (§3) | Root cause: `actionRow` carried **both** `anchors.left` and `anchors.right`, so it spanned the full bar from x=22 over the recap text. Height was 52 against 56 px children. Now fixed 490 px, right-anchored; the readiness block's width is derived from that fixed width, not from a live anchor. |
| UI-HOME-002 | Homepage — event panel | No usable vertical scrolling; Open Practice clipped, lower content unreachable | P0 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE AND VISUAL LAYOUT APPROVAL; MANUAL INTERACTION CHECK NOT PERFORMED | `41c09a3` | `home: the setup column scrolls`; structural check of `eventScroll` | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 (§3) | `ScrollView` → `Flickable` with `contentHeight` bound explicitly to the Column. A ScrollView measures its content's implicit height, which was unreliable with conditionally-visible cards and a card that changes height on selection. Adds always-on scrollbar while overflowing, 20 px bottom padding, no horizontal scrolling. Scrolling STRUCTURE is automated-test verified (Flickable, bound contentHeight, scrollbar, bottom padding, no horizontal scroll) and the visible layout is approved — no clipped content, final card reachable. **Mouse-wheel and touch/flick behaviour were not manually exercised.** |
| UI-HOME-003 | Homepage — selection state | Selected card and Selected Profile summary could disagree | P0 | Arnold's review: Open Practice highlighted while the summary read "10m Air Pistol — ISSF" | RESOLVED — AUTOMATED EVIDENCE AND VISUAL LAYOUT APPROVAL; MANUAL INTERACTION CHECK NOT PERFORMED | `41c09a3` | selection-state tests (§4) | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 (§3) | Confirmed real: the summary was hardcoded to `getDisciplineName() + " — ISSF"` for every non-training event. Now derived from `selectedProgrammeKind()`. Practice reads "— Open Practice" and no longer claims ISSF. Controller dispatch unchanged. Static selected-state presentation is visually approved — the summary matched the highlighted card — and state propagation is automated-test verified through `selectedProgrammeKind()` and the three controller-dispatch checks. **Every event transition was not manually exercised.** |
| UI-HOME-004 | Homepage — network share | Could show "Share enabled" while no folder was selected | P0 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE AND VISUAL LAYOUT APPROVAL; MANUAL INTERACTION CHECK NOT PERFORMED | `41c09a3` + `d4674d0` | `UI-HOME-004` ×8 | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 (§3) | Card gating landed in `41c09a3`; the register previously recorded this as OPEN, which was **wrong** — corrected here. `d4674d0` finished the surround: the no-folder prompt was drawn in the ERROR colour and the footer reported a bare "Share on". Sharing stays advisory and never gates Start. The displayed Network Share state is visually approved and the validity logic is automated-test verified (enabled requires a folder; on-but-unconfigured reads as incomplete in card, footer and readiness line; never gates Start). **The folder picker was not manually exercised.** |
| UI-HOME-005 | Homepage — status | LIVE and Connected repeated across heading, badge and footer | P1 | Arnold's review | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `d4674d0` | `UI-HOME-005` ×3 | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 (§3) | Read-only badge removed from the page title. **Two indicators kept deliberately**: the operating-mode control (where the mode can be changed) and the footer strip. Demo/Live confusion is a result-integrity risk, so a test asserts both survivors remain. |
| UI-HOME-006 | Homepage — header | Duplicate Tech Aim branding in the Start-session header | P1 | Arnold's review | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `8022033` + `d4674d0` | `UI-HOME-006` ×5 | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 (§3) | `8022033` removed the duplicated identity row and cut the bar 74→56 px; `d4674d0` removed the residual logo image. The homepage now renders no logo of its own (UI-DEC-005). |
| UI-HOME-007 | Homepage — event cards | Inconsistent selection / navigation indicators | P1 | Arnold's review | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `8022033` + `d4674d0` | `UI-HOME-007` ×5 | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 (§3) | Four labelled groups and the EventCard radio landed in `8022033`; `d4674d0` gave Open Practice the same radio in the same position. The Training Lab arrow remains genuine navigation. |
| UI-HOME-008 | Homepage — typography | Helper and metadata text too small or low contrast | P1 | Arnold's review | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `41c09a3` + `d4674d0` | `UI-HOME-008` ×2 | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 (§3) | **Zero sub-10px strings remain**, asserted by test. Micro-labels moved to the `label` role, metadata and helper text to `helperText`; the restart hint went from 8px muted grey to 11px warning colour; the network path stopped being monospace. |
| UI-HOME-009 | Homepage — Open Practice | Expanded card too tall, contributing to clipping | P1 | Arnold's review | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `d4674d0` | `UI-HOME-009` ×2 | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 (§3) | Collapsed height now matches every other event card exactly (78). Selecting it adds only the preset row plus one gap (56) instead of growing to 148. |
| UI-HOME-010 | Homepage — action bar | Bar unbalanced, controls overlapped | P0 | Arnold's review | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `41c09a3` | as UI-HOME-001 | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 (§3) | Same root cause and fix as UI-HOME-001. Left region = readiness/validation; right = Load saved session (210) + Start (268). Start is the strongest action and right-aligned. |


## 1a. Wind Map register (Training Lab Release 2)

| Defect ID | Screen | Description | Severity | Original evidence | Status | Fixed commit | Automated evidence | Visual evidence | Notes |
|---|---|---|---|---|---|---|---|---|---|
| UI-WIND-001 | Wind Map capture — 50 m **3P** | Wind Map 3P capture screen renders Final 35 / Ceremony / timing state | **P0 — workflow boundary** | Arnold's manual Stage 5 review of the running build, 2026-07-29 | **CLOSED — AUTOMATED AND VISUAL EVIDENCE** | `8a1fe26` | `tst_windmap_qml.cpp` §9 ×10, §10 ×4 | **HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29**, build `5404585`, **50 m 3P capture workflow**, at **1536 × 960 logical** (§1d) | Root cause in §1b. Approval covers the **3P capture workflow only** — see §1d for exactly what was and was not exercised. |

| UI-TRAIN-001 | Call & Diagnose capture | Call & Diagnose may inherit competition / Final presentation state | **P0 — workflow boundary** | Stage 5.1 audit of UI-WIND-001, 2026-07-29 | OPEN | — | — | Not yet reviewed on screen | Confirmed by source audit (§1e). Leaks the identity row, the phase stepper, the official shot counter AND the phase chip. |
| UI-TRAIN-002 | Position Transition capture | Position Transition may inherit competition / Final presentation state | **P0 — workflow boundary** | Stage 5.1 audit of UI-WIND-001, 2026-07-29 | OPEN | — | — | Not yet reviewed on screen | Confirmed by source audit (§1e). Same four leaks as UI-TRAIN-001. |
| UI-TRAIN-003 | Technical Blocks capture | Technical Blocks inherits the competition identity row | **P1 — workflow boundary** | Stage 5.1 audit of UI-WIND-001, 2026-07-29 | OPEN | — | — | Not yet reviewed on screen | Found by the same audit and **not in the original brief**. Narrower than 001/002 — the stepper, counter and chip are already gated on `isTrainingMatch` — but the identity row still shows `currentGameDisplay` / `currentmatchDisplay`, so "FINAL 35" can appear. |
| UI-WIND-002 | Wind Map — completed session | Completed Wind Map session does not visibly present the Stage 6.1 analysis and feedback workflow during a normal manually-created 3P session | **P0 — feature unreachable** | Arnold's manual Stage 6.1 review, 2026-07-29, build `5404585` | **OPEN — HUMAN VISUAL EVIDENCE** | — | — | Arnold saw counted-shot information only; no plot, MPI comparison, shift, wind rose, speed bands, timeline, 3P tabs, findings or next-session feedback | Confirmed root cause in §1f. |
| UI-WIND-003 | Wind Map analysis — all sections | Analysis sections initially appear blank or load slowly without visible progress feedback | P1 — usability | Arnold's manual Stage 6.1 review, 2026-07-29, screenshots | **RESOLVED — VISUAL EVIDENCE; MEASURED ON-SCREEN TIMINGS NOT SUPPLIED** | `305f5b1` | `tst_windmap_perf.cpp` ×12 | **HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-30**, build `106b088`, **1536 × 960 logical** (§1h) — Summary, Compare Conditions and Shot Details all opened without a blank screen | Root cause in §1g: the C++ path costs under 0.5 ms; the fault was QML. The **blank-screen** half is closed on Arnold's review. The **timing** half is not: no `WINDMAP-PERF` figures were returned, so the 1-second Summary target and the cached-switch target remain **unverified**. The loading state itself was not separately exercised. |
| UI-WIND-004 | Wind Map analysis — navigation | Two competing rows of pill navigation; too complex for an average athlete | P1 — usability | Arnold's manual Stage 6.1 review, 2026-07-29, screenshots | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `ba6745d` | `tst_windmap_qml.cpp` §13 | **HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-30**, build `106b088`, **1536 × 960 logical** (§1h) | Five section pills plus a second same-looking position row. "Overview" and "Session Overview" could both read as active. |
| UI-WIND-005 | Wind Map analysis — target plot | Plot lacks an intuitive target reference, complete legend, direction labels, scale and plain-language explanation | P1 — interpretability | Arnold's manual Stage 6.1 review, 2026-07-29, screenshots | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `ba6745d` | `tst_windmap_qml.cpp` §13 | **HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-30**, build `106b088`, **1536 × 960 logical** (§1h) | Abstract dark rectangle with unexplained hollow and coloured dots; no rings, no HIGH/LOW/LEFT/RIGHT, no mm scale. |
| UI-WIND-006 | Wind Map analysis — 3P findings | A session-level finding may display unchanged under Kneeling, Prone and Standing, making its scope unclear | **P0 — misleading** | Arnold's manual Stage 6.1 review, 2026-07-29, screenshots | **OPEN — HUMAN VISUAL EVIDENCE** (code fixed at `5902ec6`, awaiting 3P review) | — | `tst_windmap_analytics.cpp` §13 ×5 · `tst_windmap_qml.cpp` §13 | **NOT closed.** The 2026-07-30 review did not cover Kneeling, Prone or Standing, and this defect can only be settled by cycling the 3P position filter and confirming the session-level statement does not reappear as a position result | The engine's `Finding` carried no scope, so the view could not tell a session-level comparison from a position-specific one. Raised to P0: an athlete could read a cross-position comparison as a statement about Kneeling alone. |
| UI-WIND-007 | Wind Map analysis — labels | Technical labels and metrics presented without athlete-friendly definitions or interpretation | P1 — interpretability | Arnold's manual Stage 6.1 review, 2026-07-29, screenshots | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `ba6745d` | `tst_windmap_qml.cpp` §13 | **HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-30**, build `106b088`, **1536 × 960 logical** (§1h) | MPI, mean radius, H/V spread, standard deviation and group-centre vectors shown raw. |
| UI-WIND-008 | Wind Map analysis — actions | Export PDF appears enabled although branded PDF export is not implemented | P2 — honesty of affordance | Arnold's manual Stage 6.1 review, 2026-07-29, screenshots | RESOLVED — AUTOMATED AND VISUAL EVIDENCE | `ba6745d` | `tst_windmap_qml.cpp` §13 | **HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-30**, build `106b088`, **1536 × 960 logical** (§1h) | Styled as a completed primary action; opens a placeholder message. |

### 1d. UI-WIND-001 — visual evidence

**HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29**, in the running
application, Demo mode, isolated documentation-capture profile.

| | |
|---|---|
| Build reviewed | `5404585` |
| Resolution reviewed | **1536 × 960 logical** (primary display, after DPI scaling) |
| Other resolutions | **NOT TESTED** — not opened |

**Exercised and approved — 50 m Rifle 3 Positions only:** Training Lab →
Wind Map setup → planned 40-shot configuration → *Fire sighters first* →
confirm setup → start Wind Map → start sighters → record sighters → record
counted shots → position workflow.

**The corrected Training presentation is approved:** no FINAL 35, no
Ceremony, no Final timer, no Final stage controls, no official Final shot
counter.

**NOT tested — do not claim otherwise:**

- **50 m Prone** was not opened.
- **The full 40-shot layout was not fired**; the shot count reached was well
  below 40, so the long-session layout remains unverified.
- Long condition labels, condition filtering, the sighter toggle and the
  timeline were not reached, because the analysis screen never appeared
  (**UI-WIND-002**).

### 1f. UI-WIND-002 — confirmed root cause

**`WindMapAnalysisView.qml` uses `ScrollBar` but imports only `QtQuick`.**
`ScrollBar` lives in `QtQuick.Controls`, so the type never resolves and the
component cannot be created. Confirmed with `qmllint` against the reviewed
build:

```
WindMapAnalysisView.qml:118:33: ScrollBar was not found. Did you add all
                                import paths? [import]
WindMapAnalysisView.qml:118:13: unknown attached property scope ScrollBar
WindMapAnalysisView.qml:118:13: Type ScrollBar is used but it is not resolved
```

Scope check — the fault is in **one file, one line**. `WindMapHud.qml`,
`WindMapRightPanel.qml` and `TrainingTopBar.qml` each report **zero**
unresolved types.

**Why it was not caught.** Three separate gaps, all mine:

1. The Stage 6.1 guards were **static string checks** over the QML source.
   A file that never loads still contains all the right strings, so every
   check passed while the screen could not exist.
2. The launch check greps stderr for known error phrasings. It reported
   "no new QML errors" — but the analysis view is only instantiated when a
   Wind Map session reaches phase 6, which a **startup** launch never does.
   The check was real; it simply could not reach this code path.
3. `qmllint` was never run over the new files, though it finds this in
   milliseconds.

**What the athlete saw instead.** With the analysis view absent, the only
overlay left at completion is the capture HUD's own basic review — counts and
the raw shot table — which is exactly what was reported.

**Not a sample-size issue.** The analysis must open and explain itself at any
n, showing available metrics, withheld metrics, the current sample and how
many more shots each withheld statistic needs.

### 1g. UI-WIND-003 — measured, not guessed

Measurements are recorded in `docs/training-lab-wind-map-analysis.md` §4 once
taken. The candidate causes under investigation, from the Stage 6.1 source:

| Candidate | Status |
|---|---|
| `analysisModel()` re-runs the engine on every call | **to measure** — it calls `WindMapAnalyticsEngine::analyse()` unconditionally |
| Every section instantiated at once | **confirmed by inspection** — sections are gated with `visible:`, which still creates every delegate |
| Large `Repeater`s | **confirmed by inspection** — timeline, plot shots, appendix rows are all `Repeater`, one delegate per shot |
| Plot `span` recomputed per binding | **confirmed by inspection** — `plotBox.span` loops every row and each shot's `x`/`y` binding depends on it |
| Nested full-height Flickables | to check |
| Canvas repaint loops | not applicable — the plot uses Items, not Canvas |

**Rule for this phase: no cause is claimed without a measurement or a cited
line of source.**

### 1h. Stage 6.1.1 — visual evidence

**HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-30**, running application,
Demo mode, isolated documentation-capture profile.

| | |
|---|---|
| Build reviewed | `106b088` |
| Resolution reviewed | **1536 × 960 logical** |
| Other resolutions | **NOT TESTED** — not opened |

**Reviewed and approved:** Summary · Compare Conditions · Shot Details · the
short insufficient-sample session · the target visual · the disabled
*PDF — COMING NEXT* state.

**NOT reviewed — not claimed anywhere:**

- **Not every seeded session.** Only the short insufficient-sample session is
  confirmed; the 44-shot Prone session and the 3P session are not.
- **No 3P position was cycled** — Session Overview, Kneeling, Prone and
  Standing are unconfirmed. **This is why UI-WIND-006 stays OPEN.**
- **No on-screen timings were supplied.** The `WINDMAP-PERF` marks were
  instrumented but no figures were returned, so first-paint, page-switch and
  scroll performance remain **unverified**.
- No other resolution was opened.

### 1e. UI-TRAIN-001/002/003 — four-programme audit

Audited by reading every gate on `statusStrip` in `ShootingPage.qml` after the
Stage 5.1 fix. `statusStrip` is the COMPETITION top bar; a row with no gate for
a programme renders that programme's screen with inherited competition state.

| Row | Gate before Stage 5.2 | Technical Blocks | Call & Diagnose | Position Transition | Wind Map |
|---|---|:--:|:--:|:--:|:--:|
| Identity (`currentGameDisplay` + `currentmatchDisplay` + athlete) | `!isWindMapMatch` | **LEAKS** | **LEAKS** | **LEAKS** | fixed 5.1 |
| Phase stepper (SIGHTING/MATCH · SIGHT/KNEEL/PRONE/STAND) | `!isFinals10mMatch && !isTrainingMatch && !isWindMapMatch` | gated | **LEAKS** | **LEAKS** | fixed 5.1 |
| Official shot counter (`globalMatchModel.count` / `matchShootCount`) | `!isFinals10mMatch && !isTrainingMatch` | gated | **LEAKS** | **LEAKS** | fixed 5.1 |
| Phase chip | `!isFinalsMatch && !isFinals10mMatch && !isTrainingMatch` | gated | **LEAKS** | **LEAKS** | fixed 5.1 |

The identity row is the one that carries **"FINAL 35"**: `currentGameDisplay1/2`
and `currentmatchDisplay` hold whatever the last selected event card set, and
selecting the 3P Final card leaves them there. **All four programmes were
exposed to it**; three still are.

**Root cause is structural, not per-programme.** Each gate was written by
adding one more `!isXMatch` term as each programme landed. The correct
boundary is a single one: *is any Training Lab programme active?* — which
already exists as `isTrainingModeAny`.

### 1b. UI-WIND-001 — confirmed root cause

`ShootingPage.qml`'s `statusStrip` is the **competition** top bar. It carries
**no visibility gate of its own**, and its three inner rows gate only on
`isFinals10mMatch`, `isFinalsMatch` and `isTrainingMatch` (Technical Blocks).
Wind Map is in none of those gates, so during a Wind Map session the strip
renders competition state:

| Element | Why it appears |
|---|---|
| **FINAL 35** | `currentGameDisplay1/2` and `currentmatchDisplay` still hold whatever the last selected **event card** set. Selecting the 3P Final card (`gameEvent === 6`) runs `main.qml::updateGameType()` → `setFinalsGameType()`, which writes `"FINAL 35"` and `matchShootCount = 35`. Entering Training Lab never clears it. |
| **0 / 35 counter** | `globalMatchModel.count + " / " + matchShootCount` — gated only on `!isFinals10mMatch && !isTrainingMatch`. |
| **SIGHTING / MATCH**, **SIGHT / KNEEL / PRONE / STAND** | the phase stepper `Row` — same gate. |
| **Phase chip** | gated on `!isFinalsMatch && !isFinals10mMatch && !isTrainingMatch`. |

The defect is therefore **inherited presentation state**, not a stray Finals
controller: `FinalsHud`, `Finals10mHud`, `Finals10mRightPanel` and the finals
command overlays are all correctly gated on `isFinalsMatch` /
`isFinals10mMatch`, which `enterWindMapMode()` clears. `Finals3PController`
is never started, no finals event is journalled, and the Wind Map session
remains `sessionKind=Training` / `programId=wind_map` throughout. **The
defect is presentational — no Wind Map data was recorded as a Final.**

### 1c. The same hole exists in the sibling programmes

Found while diagnosing UI-WIND-001 and deliberately left out of scope for
Stage 5.1, which was scoped to Wind Map. Now registered as **UI-TRAIN-001**,
**UI-TRAIN-002** and **UI-TRAIN-003** and audited in full in §1e.

## 2. Summary

| Status | Count | IDs |
|---|---:|---|
| CLOSED / RESOLVED — AUTOMATED AND VISUAL EVIDENCE | **12** | UI-HOME 001, 005, 006, 007, 008, 009, 010 · **UI-WIND-001, 004, 005, 007, 008** |
| RESOLVED — AUTOMATED EVIDENCE AND VISUAL LAYOUT APPROVAL; MANUAL INTERACTION CHECK NOT PERFORMED | **3** | 002, 003, 004 |
| RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | 0 | — |
| PARTIALLY RESOLVED | 0 | — |
| RESOLVED — VISUAL EVIDENCE; MEASURED TIMINGS NOT SUPPLIED | **1** | **UI-WIND-003** |
| OPEN | **5** | **UI-WIND-002** · **UI-WIND-006** (P0, awaiting 3P review) · UI-TRAIN-001, UI-TRAIN-002, UI-TRAIN-003 |
| BLOCKED | 0 | — |

**Every defect has a fix and passing automated evidence, and the rendered
result is approved.** Seven are fully closed. Three are appearance-approved
but interaction-unverified: scrolling by wheel and touch, transitioning
between every event type, and the folder picker were never driven by hand.

**This does not reopen Version B.** The design is accepted (UI-DEC-012). What
remains is a verification gap, not a design question, and it is cheap to close
whenever someone sits in front of the application and exercises those three
interactions.

## 3. Visual evidence

### HUMAN VISUAL APPROVAL — ARNOLD BAILIE

| | |
|---|---|
| Reviewer | Arnold Bailie |
| Date | 2026-07-29 |
| Application commit | `d4674d0` |
| Executable SHA-256 | `F40BA7230D5C29B939CF4BA5A33C306E762C26DBE85C34756176090FF4588E73` |
| Build | 0.9.0.0, 2026-07-29 08:43 |
| Display | 1536 × 960 (the machine's primary and only display) |
| Language | English |
| Verdict | **APPROVED** — Version B accepted as the Tech Aim Beta homepage |

### What the approval covers

The running homepage as rendered on the reviewer's screen: layout, spacing,
the action bar, the event groups, the selected-programme summary, the network
share state, typography and branding.

### What it does not cover — recorded honestly

1. **Only the primary display size was reviewed.** 1366 × 768, 1280 × 720 and
   1100 × 700 were **not opened**. They are `NOT TESTED` in the acceptance
   checklist. This does not block the approved design direction; it means the
   homepage is approved at the size it was seen at.
2. **Interaction was not systematically exercised.** Wheel and touch
   scrolling, switching between every event type, and the folder picker were
   not stepped through as a test script.
3. **German was not reviewed.** The page has never been run with the German
   catalogue.
4. **No screenshot file was produced.** Automated capture remains blocked by
   endpoint security, which blocks both synthetic input and the screen-capture
   helper — it blocked the helper outright with
   `This script contains malicious content and has been blocked by your
   antivirus software`. **No bypass was attempted and none should be.** The
   approval above is a reviewer sign-off, not an image artefact, and is
   recorded as such.

### Registered screenshots

### Registered screenshots

| Filename | Resolution | App commit | Executable SHA-256 | Language | Mode | Selected event | Review status |
|---|---|---|---|---|---|---|---|
| `manual-preview/ui-audit/raw/01-home.png` | 1536 × 912 | `a74d3fd` | `EBA82B0F…42FD653D` | English | Demo | Open Practice, 10 shots | **Pre-Version-B.** Evidence for the UI-0 audit only. **Must not** be used as Version B evidence. |

No Version B screenshot file exists. The approval in this section is a
reviewer sign-off against the running build, not a captured image.

### Not evidence

`manual-preview/ui-audit/concepts/home-screen-A-B-C.html` is a wireframe
stamped **CONCEPT MOCKUP — NOT CURRENT APPLICATION** on every frame. It is
git-ignored and must never be cited as application evidence.

### Still worth capturing

Screenshots remain useful for the operator manuals and for reviewing the three
unreviewed window sizes. Place PNGs in `manual-preview/ui-audit/raw/` named
`vb-home-<width>x<height>.png`, captured from the isolated documentation-capture
profile (Demo, English, synthetic athlete), and register them above. They are no
longer a gate on the design approval.

## 4. Regression traceability

| Defect | Automated evidence | Where | If none, why |
|---|---|---|---|
| UI-HOME-001 | action-bar region separation; Start inside the action bar | `tst_brandpackage.cpp` + `tst_homepage_layout` (§5) | — |
| UI-HOME-002 | `Flickable` present, `contentHeight` bound, bottom padding, no horizontal scroll | `tst_homepage_layout` | Wheel/touch behaviour cannot be exercised without an input path — **human check** |
| UI-HOME-003 | single-source functions exist; summary/Start/recap all call them; practice is not labelled ISSF | `tst_homepage_layout` | Live propagation to the controller needs a running app — **human check** |
| UI-HOME-004 | share gating, incomplete state, footer agreement, advisory-not-gating | `tst_homepage_layout` | Folder-picker interaction needs a running app — **human check** |
| UI-HOME-005 | badge removed; both surviving indicators asserted | `tst_homepage_layout` | Whether the remaining two read clearly is visual — **human check** |
| UI-HOME-006 | identity row absent from `headerBar` | `tst_homepage_layout` | Residual logo image is a visual judgement — **human check** |
| UI-HOME-007 | four labelled groups present | `tst_brandpackage.cpp` | Indicator consistency is visual — **human check** |
| UI-HOME-008 | typography roles defined and used in the action bar | `tst_brandpackage.cpp` | Readability is a visual judgement — **human check** |
| UI-HOME-009 | collapsed height matches other cards; 148px expansion gone | `tst_homepage_layout` | Proportion is a visual judgement — **human check** |
| UI-HOME-010 | as UI-HOME-001 | `tst_homepage_layout` | Visual balance — **human check** |

---

## 1g. UI pass 2026-08-10 — left-pane programme label and header reserve

### UI-TRAIN-001 — partially closed, code complete, awaiting on-screen review

Stage 5.2 gated the competition `statusStrip` on `isTrainingModeAny`, and the
register recorded the defect as OPEN pending review. Arnold's 2026-08-10
session showed **the badge was still wrong**, because the left pane renders the
SAME inherited state through a DIFFERENT alias:

```qml
property alias currentmatchDisplay: leftPanel.matchDisplay   // ShootingPage
Rectangle { ... Text { text: matchText.text } }              // LeftPanel, ungated
```

`matchDisplay` holds whatever the last selected event card wrote ("MATCH 60",
"FINAL 35"); entering a Training Lab programme never clears it. The statusStrip
fix could not reach it. **The gate was applied to one of two consumers.**

Fixed by giving the left pane an explicit `programmeLabel`, bound in
`ShootingPage` from the SAME derivation `TrainingTopBar` uses, so the two bars
can never disagree. Empty means a genuine competition event, where
`matchDisplay` is correct. The badge also drops competition red for a neutral
treatment under a Training Lab programme — a training block is not a match and
should not borrow the match colour.

| Evidence | Status |
|---|---|
| Component render, training vs competition side by side | `docs/ui/evidence-UI-TRAIN-001-programme-badge.png` |
| Application builds and launches, no QML errors, no binding loops | verified 2026-08-10 |
| Full integrated screen inside a live Call & Diagnose session | **NOT PERFORMED** |

Status: **RESOLVED — AUTOMATED/COMPONENT EVIDENCE, HUMAN VISUAL CHECK
REQUIRED.** Not closed. UI-TRAIN-002 and UI-TRAIN-003 inherit the same fix
(the label is derived for all four programmes) but are likewise unreviewed.

### UI-HDR-001 — connection panel covered the header honesty line (NEW)

| Field | Value |
|---|---|
| Area | Shooting screen — header strip |
| Symptom | With the target connected, the connection status covered "NOT AN OFFICIAL COMPETITION RESULT" |
| Severity | **P1 — honesty text obscured** |
| Source | Arnold, 2026-08-10 |

`TargetStatusPanel` was anchored with a bare `rightMargin: 200` at `z: 500`, so
it floated over whatever the header contained. Measured at the 1536 px test
width: the panel spanned x ≈ 875…1336 while the centred honesty line spanned
≈ 618…918 — overlapping ≈ 875…918, with the panel drawn on top. The
right-hand progress row (`rightMargin: 16`) collided outright.

Fixed by reserving a slot rather than nudging either element:
`ShootingPage.headerStatusReserve` is consumed by `TrainingTopBar.rightReserve`
and by the competition counter row, and the panel is capped at 380 px and
anchored into that slot. Reserve falls to zero when the panel expands, because
it then sits over the target face by design.

| Evidence | Status |
|---|---|
| Header render at 1536 px with the slot occupied | `docs/ui/evidence-UI-HDR-001-header-reserve.png` |
| All five connection states | **NOT INDIVIDUALLY RENDERED** |

Status: **RESOLVED — COMPONENT EVIDENCE, HUMAN VISUAL CHECK REQUIRED.**

### Items assessed and NOT changed

**Left-pane icons (brief item 3).** Already one unified family — a single
`navModel` of Feather-style 24×24 stroke paths rendered through one `VIcon`
with consistent sizing (22/26 px), stroke, spacing and padding, visible in the
evidence render. The legacy raster PNGs in the same file (`play.png`,
`match_60_40_box.png`, `pistol_box*.png`, `sighter_selected.png`) are
`opacity: 0` or `visible: false` invisible layout anchors, not drawn icons. No
mismatched icon style was found. **No change made** — a rewrite would have
churned a component that already meets the requirement.

**Discipline imagery (brief item 2).** NOT DONE. Recorded honestly: photographic
or AI-generated artwork cannot be produced here. The appropriate solution is
vector target faces drawn to true ISSF ring geometry per discipline, which
scales without blur or distortion and matches the existing vector language.
This needs its own scoped task and the ring geometry currently
⏳ awaiting official rule (see `docs/release/scoring-geometry-verification.md`)
— drawing a 50 m face from an unconfirmed constant would put an unverified
number on screen.

### 1h. Integrated verification 2026-08-10 (real application, emulated target)

Driven through the running build: Home → RIFLE → 50 m → PRONE → Training Lab →
Call & Diagnose → Confirm setup → Start calling, with the target emulator
supplying a connected target over Modbus TCP.

**UI-TRAIN-001 — CLOSED.** The integrated Call & Diagnose screen shows the left
pane reading **CALL & DIAGNOSE** in the neutral badge. No MATCH-60, no
inherited competition state, no clipping. The discipline plate renders above
it with the full name **50M RIFLE PRONE**. Verified in the same session that
the competition path still shows **MATCH-60** in competition red for a genuine
50 m Rifle Prone ISSF match, so the gate discriminates rather than merely
suppresses.

**UI-HDR-001 — CLOSED.** Header at 1389 px with the panel in **READY**:

```
TRAINING LAB Call & Diagnose │ Philemon │ 50m Rifle Prone
        NOT AN OFFICIAL COMPETITION RESULT
                    CALL & DIAGNOSE   ● READY  Emulated / network target
```

Four elements, no overlap, honesty line fully readable, mode chip and status
panel each clear. Also confirmed on the COMPETITION header (counter row +
SIGHTING chip clear of the READY panel). Emulated target — **not** physical
USB verification.

### UI-CD-001 — Call & Diagnose right panel contradicts the header (NEW)

| Field | Value |
|---|---|
| Area | Call & Diagnose — SIGHTERS right panel |
| Symptom | Panel reads `Target: Not connected` while the header reads `READY` and acquisition is live |
| Severity | **P1 — contradictory target state** |
| Source | Integrated verification, 2026-08-10 |
| Status | **OPEN** |

Two surfaces on one screen disagree about the target. The header binds to the
shared `MODREADER` state; the C&D panel evidently reads a different or stale
source. This is the same class of defect as LOGIN-LINK-001 — a second consumer
that did not get the shared authority — and it must be traced to that source
rather than patched at the label.

### Discipline plate — position glyph cropped

The plate's position-glyph row is not visible at left-pane scale: the 96 px
card crops it. Cosmetic at Prone (one glyph), but it is the ONLY thing that
distinguishes 3P from Prone, so 3P would lose its differentiator. Card height
or art aspect needs adjusting. **OPEN.**

## 1h. Shared vector icons — VIcon (2026-08-11)

### UI-ICON-001 — VIcon geometry clipped when rendered below authoring viewBox size

| Field | Value |
|---|---|
| Area | `VIcon.qml` — shared vector icon component, every consumer |
| Symptom | Icons drawn smaller than their authoring grid lost geometry off the right and bottom edges and rendered soft |
| Severity | P1 — shared component, silently wrong at production size in every screen that uses it |
| Source | Production-size render during the UI-ART-001 review |
| Status | **CLOSED — HUMAN VISUAL APPROVED** |
| Fixed commit | `11fd19e` (clipping) + `c76d959` (compact stroke refinement) |
| Automated evidence | `docs/ui/evidence-vicon-production-icons.png`, `docs/ui/evidence-vicon-stroke-snap.png`; `tests/qml` 46 checks / 0 failures; passing Release build |
| Visual evidence | **HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-08-11**, on native 1:1 renders at the real 22/26 px production sizes |

**Symptom, measured.** The rifle silhouette on the discipline plate is drawn at
92 px from a 126-unit authoring grid. It rendered **66 px wide instead of 92,
and 16 px tall instead of 21** — the whole exposed barrel and the front sight
tunnel were missing. Every other shared icon was affected in the same way: the
Group/MPI rings and the Home roof had flattened right edges, and the Settings
gear teeth were soft.

**Root cause.** `VIcon` wrapped its `Shape` in `layer.enabled: true` while
applying the authoring-grid-to-item `Scale` as an item transform. A layer
rasterises the item's contents in its own *unscaled* coordinates and clips them
to the item's pixel bounds, and only then is the transform applied to the
resulting texture. Any icon drawn below its authoring-grid size therefore lost
`1 - width/viewBoxW` of itself before it was ever scaled. For the rifle that is
92/126 = 0.73, which is exactly the 66/91 measured. Drawn *above* grid size the
same mechanism magnified a grid-resolution texture, which is why large blow-ups
pixelated.

**Fix.** The layer is removed, so the geometry rasterises at final resolution.
No change to the scale maths, the stroke-width convention or any path data.

**Why it was not caught earlier.** The UI-ART-001 evidence was rendered with
the art at 180 px, where the rifle is 126 px — exactly the viewBox, scale 1.0,
and the defect cannot appear. It only manifests below grid size, which is what
production uses.

**Scope.** This is a rendering defect in shared infrastructure and is
deliberately kept separate from UI-ART-001, which concerns the rifle *drawing*.
No path data was touched by this fix.

**Follow-up — compact stroke width (`c76d959`).** With the clipping gone the
22 px compact nav icons were still soft. Stroke widths are authored in grid
units, so the viewBox-to-item scale resolved `1.9` to **1.74 device px** at
22 px: every stroke edge was a partial pixel. The 26 px variant happened to
land near 2.06 px and was already sharp, which is why only compact looked
wrong. The device width is now snapped to a whole pixel (floor 1) and converted
back to grid units — one uniform rule, no geometry moved, filled silhouettes
(`strokeWidth: 0`) untouched. Measured full-white/ink, higher is crisper:

| Icon | 22 px before | 22 px after | 26 px before | 26 px after |
|---|---|---|---|---|
| Stats | 0.18 | **0.48** | 0.51 | 0.51 |
| Report | 0.35 | 0.34 | 0.34 | 0.34 |
| Settings | 0.27 | **0.33** | 0.33 | 0.33 |
| Home | 0.36 | **0.40** | 0.40 | 0.38 |

The approved 26 px appearance is held: its device stroke moves 2.06 → 2.00 px,
the ratios are unchanged and the largest single-pixel delta anywhere in the row
is 27/255, on antialiased edges only.

**A note on evidence, because it cost a review cycle.** The first evidence
sheet for this defect was nearest-neighbour magnified. That is the right tool
for detecting *clipping* and the wrong one for judging *sharpness* — magnified
pixels always look blocky, and the sheet was read as "the icons are blurry"
when they were not. Native 1:1 renders are the acceptance evidence for
sharpness; any magnified view must be separately labelled as a diagnostic.

## 1i. Discipline artwork — match-rifle silhouette (2026-08-11)

### UI-ART-001 — The rifle silhouette reads as a military weapon

| Field | Value |
|---|---|
| Area | `DisciplineArt.qml` — rifle silhouette on the discipline plate (left pane) |
| Symptom | Angular, chunky outline with a short thick barrel; scans as a service weapon on a sport-shooting product |
| Severity | P2 — brand/identity, not function |
| Source | Arnold's review of the running build; two prior attempts rejected |
| Status | **CLOSED — HUMAN VISUAL APPROVED** |
| Fixed commit | this change (silhouette + production-size simplification) |
| Automated evidence | `docs/ui/evidence-match-rifle-production.png`; IoU 0.816 against the reference at matched scale; `tests/qml` 46 checks / 0 failures; passing Release build |
| Visual evidence | **HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-08-11**, on the 132 px production tile |

**Root cause, in the terms the two failed attempts established.** The path was
straight segments end to end, so every organic edge of a stock arrived as a
facet. The cheek piece and buttplate were drawn inside the stock outline, so
the two features that actually identify a match rifle were invisible. And the
barrel was short and thick, which is the single strongest military cue.

**Fix.** `riflePath` was deleted and rebuilt from zero against
`docs/ui/issf-match-rifle-reference.png`, which UI-DEC-013 makes the visual
authority. The reference was measured column by column and rescaled from
1212×290 px to a 126×30 authoring grid, aspect preserved to 0.5%. Organic edges
are cubics; the cheek piece and buttplate are separate subpaths with real air
around them; the exposed barrel is 27.4% of overall length at 13.5:1.

**Deliberate departures from the reference, so they are not read as errors.**

| Departure | Reference | As drawn | Why |
|---|---|---|---|
| Rear sight rail | 0.2 units thick | 0.95 | 0.2 units is 0.15 px at the 92 px width the left pane draws; it would vanish and take the raised-sight read with it |
| Forend vent slots | 0.6 units | 0.9 | same reason |
| Forend nose | ends at x 93.6 | ends at x 91.5 | holds the exposed barrel above 27% of length, per the brief |
| Free-float gap | closes into a 2-unit bridge at the forend nose | left open | a 1.5 px bridge is not a feature at tile size, it is a rendering artefact |
| Receiver/barrel junction | stepped shoulder with a bolt boss | single chamfer | below the resolution of the tile |

**Fill-rule constraint, recorded because it is easy to break.** `VIcon` uses
Qt's default odd-even fill. The separate pieces must never *overlap* the main
outline or they cancel to holes; the visible gaps the design calls for are what
guarantees this. The cheek-piece posts stop 0.25 units short of the rail for
exactly that reason. The three genuine holes — trigger guard and two forend
vents — are nested subpaths.

**Production-size simplification, and why it was needed.** The traced
silhouette measured well (IoU 0.816) and looked right at 2×, but the acceptance
gate is the **132 px production tile**, and there several members fell below one
device pixel and could only render as grey. At the 92 px rifle width the scale
is 0.73, so:

| Feature | Authored | Device px at 92 px | Action |
|---|---|---|---|
| Forend ventilation slots | 0.9 units | **0.66** | **removed** — never resolvable, contributed only a grey band down the forend |
| Rear sight rail | 0.9 units | **0.66** | thickened to 1.4 units (1.02 px) |
| Barrel, plain section | 1.6 units | 1.17 | thickened to 1.95 units (1.42 px), **upward only** so the free-float gap below is not narrowed |

Everything else is untouched. **Every critical ISSF cue is retained**: the
separate adjustable buttplate and its lower hook, the raised cheek piece with
visible air between it and the comb, the anatomical competition stock, the deep
near-vertical grip, the raised rear diopter, the front sight tunnel, and a
still-slender barrel at 17:1 over the exposed length. The forend is now one
clean competition profile rather than a vent-textured band — less detail, but a
stronger ISSF shape, which is the right trade at tile size.

Rejected during the pass: thickening the barrel further to 2.25 units. It added
ink without adding solid pixels (solid/ink 0.42 → 0.40) and was backed out.
Measured in the rifle band at 92 px, solid/ink went **0.40 → 0.42**.

**Closure.** The reference image `docs/ui/issf-match-rifle-reference.png` was
the visual authority throughout (UI-DEC-013); the **132 px production render
was the acceptance gate**, not the 2× view and not the similarity score; and
the silhouette was approved on that render. Numerical similarity was treated as
diagnostic evidence only — a mask can score well and still read wrong at UI
size, so it was never allowed to stand in for the human decision.

**Reproducing the evidence.** From the repository root, with the Qt `bin` on
`PATH`:

```
tools/uirender/release/uirender.exe tools/uirender/scene_disciplines.qml <out>.png 620 300
```

The scene renders where it lives — nothing is copied to the repository root —
and no environment variables are needed. Build instructions, the per-scene
canonical sizes and the rule for writing new scenes are in
`tools/uirender/README.md`.

Three harness limitations recorded in earlier revisions of this entry — the
scene needing to be copied to the repository root, `QT_QPA_PLATFORM=offscreen`
having to be set by hand to avoid a segfault, and text rendering as empty boxes
— were fixed in the renderer itself and no longer apply. **This changes nothing
about the evidence status below:** the renderer still draws the same QML
through the same Qt Quick scene graph without being the application, so its
output remains automated evidence and not human visual approval.
