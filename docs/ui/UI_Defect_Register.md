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
| UI-HOME-004 | Homepage — network share | Could show "Share enabled" while no folder was selected | P0 | Arnold's review | **OPEN** | — | none | none | A fix was prepared (default the toggle off until a folder exists; open the picker on enable) and **declined** pending a decision on the wanted behaviour. See UI-DEC-008, which remains `PROPOSED`. Three options are recorded there. |
| UI-HOME-005 | Homepage — status | LIVE and Connected repeated across heading, badge and footer | P1 | Arnold's review | **OPEN** | — | none | none | Not started. LIVE/DEMO currently appears in `headerBar`, in the operating-mode segmented control, and in `contentFooter`. Connection appears on the connect button and in the footer. |
| UI-HOME-006 | Homepage — header | Duplicate Tech Aim branding in the Start-session header | P1 | Arnold's review | **PARTIALLY RESOLVED** | `8022033` | none | none | The duplicate **identity row** (target icon + "TECH AIM" + "ELECTRONIC TARGET CONTROL") was removed and the bar cut 74 → 56 px. A **logo image remains** in `headerBar` alongside the shell header's mark. Remainder outstanding. |
| UI-HOME-007 | Homepage — event cards | Inconsistent selection / navigation indicators | P1 | Arnold's review | **PARTIALLY RESOLVED** | `8022033` | `home: event group … is present` ×4 | none | Four labelled groups added; `EventCard` carries a radio indicator; the Training Lab arrow is genuine navigation. **Open Practice still has no selection indicator.** See UI-DEC-009. |
| UI-HOME-008 | Homepage — typography | Helper and metadata text too small or low contrast | P1 | Arnold's review | **PARTIALLY RESOLVED** | `41c09a3` | `tokens: typography roles are defined` | none | The action bar and summary now use `theme.type.*` roles. 8 px and 9 px helper text survives elsewhere on the page (mode hint, card metadata, network path). Full pass outstanding. |
| UI-HOME-009 | Homepage — Open Practice | Expanded card too tall, contributing to clipping | P1 | Arnold's review | **OPEN** | — | none | none | Not started. The card is still `selected ? 148 : 78`. Note the clipping *cause* was UI-HOME-002 and is fixed independently; this remains a proportion issue. |
| UI-HOME-010 | Homepage — action bar | Bar unbalanced, controls overlapped | P0 | Arnold's review | RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | `41c09a3` | as UI-HOME-001 | none | Same root cause and fix as UI-HOME-001. Left region = readiness/validation; right = Load saved session (210) + Start (268). Start is the strongest action and right-aligned. |

## 2. Summary

| Status | Count | IDs |
|---|---:|---|
| RESOLVED — AUTOMATED AND VISUAL EVIDENCE | **0** | — |
| RESOLVED — AUTOMATED EVIDENCE, HUMAN VISUAL CHECK REQUIRED | 4 | 001, 002, 003, 010 |
| PARTIALLY RESOLVED | 3 | 006, 007, 008 |
| OPEN | 3 | 004, 005, 009 |
| BLOCKED | 0 | — |

**No defect is fully closed.** The four fixed ones await visual evidence; six
have outstanding work.

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
| UI-HOME-004 | none | — | Not implemented |
| UI-HOME-005 | none | — | Not implemented |
| UI-HOME-006 | identity row absent from `headerBar` | `tst_homepage_layout` | Residual logo image is a visual judgement — **human check** |
| UI-HOME-007 | four labelled groups present | `tst_brandpackage.cpp` | Indicator consistency is visual — **human check** |
| UI-HOME-008 | typography roles defined and used in the action bar | `tst_brandpackage.cpp` | Readability is a visual judgement — **human check** |
| UI-HOME-009 | none | — | Not implemented |
| UI-HOME-010 | as UI-HOME-001 | `tst_homepage_layout` | Visual balance — **human check** |
