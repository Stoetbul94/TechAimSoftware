# Tech Aim — UI Defect Register

Homepage defects raised from Arnold's review of the running Version B build.

**Closure rule.** A defect is `RESOLVED — AUTOMATED AND VISUAL EVIDENCE` only
when it has a fixed commit, passing build/tests, a real application screenshot,
and a passing acceptance-checklist line. Code changing is not closure.

**Screenshot status.** No screenshot of the Version B homepage exists in the
repository. Automated capture is blocked by endpoint security (see §3), so no
defect below can currently reach full closure, regardless of code state.

Status values: `RESOLVED — AUTOMATED AND VISUAL EVIDENCE` ·
`RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED` ·
`PARTIALLY RESOLVED` · `OPEN` · `BLOCKED`

---

## 1. Register

| Defect ID | Screen | Description | Severity | Original evidence | Status | Fixed commit | Automated evidence | Visual evidence | Notes |
|---|---|---|---|---|---|---|---|---|---|
| UI-HOME-001 | Homepage — action bar | "READY TO START" and "Load saved session" overlapped in the lower-left | P0 | Arnold's review of the running build | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `41c09a3` | `home: the Start action sits INSIDE the bottom action bar` | none | Root cause: `actionRow` carried **both** `anchors.left` and `anchors.right`, so it spanned the full bar from x=22 over the recap text. Height was 52 against 56 px children. Now fixed 490 px, right-anchored; the readiness block's width is derived from that fixed width, not from a live anchor. |
| UI-HOME-002 | Homepage — event panel | No usable vertical scrolling; Open Practice clipped, lower content unreachable | P0 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `41c09a3` | `home: the setup column scrolls`; structural check of `eventScroll` | none | `ScrollView` → `Flickable` with `contentHeight` bound explicitly to the Column. A ScrollView measures its content's implicit height, which was unreliable with conditionally-visible cards and a card that changes height on selection. Adds always-on scrollbar while overflowing, 20 px bottom padding, no horizontal scrolling. |
| UI-HOME-003 | Homepage — selection state | Selected card and Selected Profile summary could disagree | P0 | Arnold's review: Open Practice highlighted while the summary read "10m Air Pistol — ISSF" | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `41c09a3` | selection-state tests (§4) | none | Confirmed real: the summary was hardcoded to `getDisciplineName() + " — ISSF"` for every non-training event. Now derived from `selectedProgrammeKind()`. Practice reads "— Open Practice" and no longer claims ISSF. Controller dispatch unchanged. |
| UI-HOME-004 | Homepage — network share | Could show "Share enabled" while no folder was selected | P0 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `41c09a3` + `d4674d0` | `UI-HOME-004` ×8 | none | Card gating landed in `41c09a3`; the register previously recorded this as OPEN, which was **wrong** — corrected here. `d4674d0` finished the surround: the no-folder prompt was drawn in the ERROR colour and the footer reported a bare "Share on". Sharing stays advisory and never gates Start. |
| UI-HOME-005 | Homepage — status | LIVE and Connected repeated across heading, badge and footer | P1 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `d4674d0` | `UI-HOME-005` ×3 | none | Read-only badge removed from the page title. **Two indicators kept deliberately**: the operating-mode control (where the mode can be changed) and the footer strip. Demo/Live confusion is a result-integrity risk, so a test asserts both survivors remain. |
| UI-HOME-006 | Homepage — header | Duplicate Tech Aim branding in the Start-session header | P1 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `8022033` + `d4674d0` | `UI-HOME-006` ×5 | none | `8022033` removed the duplicated identity row and cut the bar 74→56 px; `d4674d0` removed the residual logo image. The homepage now renders no logo of its own (UI-DEC-005). |
| UI-HOME-007 | Homepage — event cards | Inconsistent selection / navigation indicators | P1 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `8022033` + `d4674d0` | `UI-HOME-007` ×5 | none | Four labelled groups and the EventCard radio landed in `8022033`; `d4674d0` gave Open Practice the same radio in the same position. The Training Lab arrow remains genuine navigation. |
| UI-HOME-008 | Homepage — typography | Helper and metadata text too small or low contrast | P1 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `41c09a3` + `d4674d0` | `UI-HOME-008` ×2 | none | **Zero sub-10px strings remain**, asserted by test. Micro-labels moved to the `label` role, metadata and helper text to `helperText`; the restart hint went from 8px muted grey to 11px warning colour; the network path stopped being monospace. |
| UI-HOME-009 | Homepage — Open Practice | Expanded card too tall, contributing to clipping | P1 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `d4674d0` | `UI-HOME-009` ×2 | none | Collapsed height now matches every other event card exactly (78). Selecting it adds only the preset row plus one gap (56) instead of growing to 148. |
| UI-HOME-010 | Homepage — action bar | Bar unbalanced, controls overlapped | P0 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `41c09a3` | as UI-HOME-001 | none | Same root cause and fix as UI-HOME-001. Left region = readiness/validation; right = Load saved session (210) + Start (268). Start is the strongest action and right-aligned. |

## 2. Summary

| Status | Count | IDs |
|---|---:|---|
| RESOLVED — AUTOMATED AND VISUAL EVIDENCE | **0** | — |
| RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | **10** | 001–010 |
| PARTIALLY RESOLVED | 0 | — |
| OPEN | 0 | — |
| BLOCKED | 0 | — |

**Every defect now has a fix and automated evidence. None is fully closed**,
because full closure requires a real screenshot and none exists — see §3.
Code changing is not closure.

## 3. Screenshot evidence

**BLOCKED — MANUAL-ASSISTED SCREENSHOT CAPTURE REQUIRED.**

Endpoint security (Bitdefender/AMSI) on the development machine blocks both
synthetic input injection and the PowerShell screen-capture helper. It blocked
the helper script outright with
`This script contains malicious content and has been blocked by your antivirus
software`. **No bypass was attempted and none should be.**

Consequence: the application cannot be driven or captured programmatically, so
every defect above is capped at "automated evidence" until screenshots are
supplied by hand.

### Registered screenshots

| Filename | Resolution | App commit | Executable SHA-256 | Language | Mode | Selected event | Review status |
|---|---|---|---|---|---|---|---|
| `manual-preview/ui-audit/raw/01-home.png` | 1536 × 912 | `a74d3fd` | `EBA82B0F…42FD653D` | English | Demo | Open Practice, 10 shots | **Pre-Version-B.** Evidence for the UI-0 audit only. **Must not** be used as Version B evidence. |

No Version B screenshot exists at any resolution.

### Not evidence

`manual-preview/ui-audit/concepts/home-screen-A-B-C.html` is a wireframe
stamped **CONCEPT MOCKUP — NOT CURRENT APPLICATION** on every frame. It is
git-ignored and must never be cited as application evidence.

### To unblock

Place PNGs in `manual-preview/ui-audit/raw/` named
`vb-home-<width>x<height>.png` for 1536×960, 1366×768, 1280×720 and 1100×700,
captured from the isolated documentation-capture profile (Demo, English,
synthetic athlete). They can then be registered here and the acceptance
checklist closed against them.

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
