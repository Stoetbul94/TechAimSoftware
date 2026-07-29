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
| UI-WIND-001 | Wind Map capture — 50 m 3P (and Prone) | Wind Map 3P capture screen renders Final 35 / Ceremony / timing state | **P0 — workflow boundary** | Arnold's manual Stage 5 review of the running build, 2026-07-29 | OPEN | — | — | Real application screenshots (Arnold) | See §1b for the confirmed root cause. A Training Lab programme must never present as an ISSF Final. |

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

### 1c. The same hole exists in two sibling programmes

`statusStrip` gates on `isTrainingMatch` only. **Call & Diagnose**
(`isCallDiagnoseMatch`) and **Position Transition** (`isPositionTransitionMatch`)
are equally ungated and will show the same inherited competition state.
This was found while diagnosing UI-WIND-001 and is recorded here so it is not
lost; it is **out of scope for Stage 5.1**, which was scoped to Wind Map.
Recommended as a focused follow-up.

## 2. Summary

| Status | Count | IDs |
|---|---:|---|
| RESOLVED — AUTOMATED AND VISUAL EVIDENCE | **7** | 001, 005, 006, 007, 008, 009, 010 |
| RESOLVED — AUTOMATED EVIDENCE AND VISUAL LAYOUT APPROVAL; MANUAL INTERACTION CHECK NOT PERFORMED | **3** | 002, 003, 004 |
| RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | 0 | — |
| PARTIALLY RESOLVED | 0 | — |
| OPEN | **1** | UI-WIND-001 |
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
