# Tech Aim — Homepage Acceptance Checklist

**Reviewing commit:** `41c09a3` · **Reviewed:** 2026-07-29
Statuses: `PASS` · `FAIL` · `BLOCKED` · `NOT TESTED` · `HUMAN VISUAL CHECK REQUIRED`

> **Nothing here is signed off.** No screenshot of the Version B homepage
> exists. Structural properties are proven by source tests; everything that
> requires *seeing* or *driving* the page is `HUMAN VISUAL CHECK REQUIRED` or
> `BLOCKED`. Automated capture is blocked by endpoint security and no bypass
> was attempted.

Automated evidence below is resolution-independent — it reads
`LoginPage.qml`. It is therefore listed once, in §1, and the per-resolution
tables record only what genuinely varies with window size.

---

## 1. Structural checks — resolution-independent

Test binary: `tests/reliability/release/reliability_tests.exe` · **1041 checks, 0 failures**

### Layout

| Check | Status | Test | Notes |
|---|---|---|---|
| Action bar has separated regions | PASS | `UI-HOME-001: the action row is not anchored to BOTH edges` | The overlap cause; row is now right-anchored, fixed 490 px |
| Readiness has its own region | PASS | `UI-HOME-001: the readiness width is derived from the action row's fixed width` | Cannot go negative |
| Readiness text elides | PASS | `UI-HOME-010: readiness text elides rather than overflowing` | |
| Action row sized for its children | PASS | `UI-HOME-001: the action row is as tall as its 56 px children` | Was 52 |
| Start action outside scroll containers | PASS | `layout: the primary Start action lives inside the action bar` | Structurally un-clippable |
| Footer does not cover the action bar | PASS | `layout: the panels stop at the action bar` | Panels anchor to `actionBar.top` |
| Right panel keeps the greater width | PASS | `layout: the event panel keeps the greater share of the width` | 44 % / 56 % |
| No overlapping text (visual) | HUMAN VISUAL CHECK REQUIRED | — | Geometry is proven; appearance is not |
| No overlapping controls (visual) | HUMAN VISUAL CHECK REQUIRED | — | |
| No content outside panel bounds | HUMAN VISUAL CHECK REQUIRED | — | |

### Scrolling

| Check | Status | Test | Notes |
|---|---|---|---|
| Event list is a Flickable | PASS | `UI-HOME-002: the event list is a Flickable` | ScrollView mis-measured its content |
| `contentHeight` bound explicitly | PASS | `UI-HOME-002: contentHeight is bound explicitly` | |
| Vertical scrollbar provided | PASS | `UI-HOME-002: a vertical scrollbar is provided` | |
| Scrollbar shown while overflowing | PASS | `UI-HOME-002: the scrollbar is shown while content overflows` | |
| Bottom padding exists | PASS | `UI-HOME-002: the final card has bottom padding` | 20 px |
| Content clipped to panel | PASS | `UI-HOME-002: content is clipped to the panel` | |
| No horizontal scrolling configured | PASS | `UI-HOME-002: content width is pinned` | `contentWidth: width` |
| Mouse-wheel scrolling works | HUMAN VISUAL CHECK REQUIRED | — | No input path available |
| Touch / flick scrolling works | HUMAN VISUAL CHECK REQUIRED | — | |
| Final event fully reachable | HUMAN VISUAL CHECK REQUIRED | — | Structure supports it |
| Open Practice expansion reachable | HUMAN VISUAL CHECK REQUIRED | — | |
| Scroll does not alter selection | HUMAN VISUAL CHECK REQUIRED | — | No code path couples them |

### Selection consistency

| Check | Status | Test | Notes |
|---|---|---|---|
| Single source of truth exists | PASS | `UI-HOME-003: selectedProgrammeKind() exists` (+3) | |
| Selected Profile uses it | PASS | `UI-HOME-003: the Selected Profile summary uses the single source` | |
| Summary heading uses it | PASS | `UI-HOME-003: the summary heading uses the single source` | |
| Start wording uses it | PASS | `UI-HOME-003: the Start button wording uses the single source` | |
| Recap uses it | PASS | `UI-HOME-003: the action-bar recap uses the single source` | |
| Hardcoded "— ISSF" gone | PASS | `UI-HOME-003: the hardcoded '<discipline> - ISSF' summary is gone` | The reported defect |
| Practice not labelled ISSF | PASS | `UI-HOME-003: practice is labelled as practice` | |
| All six event kinds handled | PASS | `UI-HOME-003: event kind … is handled` ×6 | |
| Controller receives same event | PASS | `UI-HOME-003: controller dispatch … is unchanged` ×3 | Dispatch untouched |
| Highlighted card equals selected event | HUMAN VISUAL CHECK REQUIRED | — | Needs a running page |
| Shot plan matches selected event | HUMAN VISUAL CHECK REQUIRED | — | |
| Scoring matches selected event | HUMAN VISUAL CHECK REQUIRED | — | |
| Switching cards updates once | HUMAN VISUAL CHECK REQUIRED | — | |
| Open Practice shot count updates summary | HUMAN VISUAL CHECK REQUIRED | — | |

### Network share

| Check | Status | Test | Notes |
|---|---|---|---|
| Enabled state requires a valid folder | **FAIL** | `UI-HOME-004` | **UI-HOME-004 OPEN** — fix declined pending a decision (UI-DEC-008 `PROPOSED`) |
| No-folder state not shown as success | **FAIL** | — | As above |
| Validity state available | PASS | `UI-HOME-004: share-validity state is available` | `shareConfigured` / `shareIncomplete` exist |
| Incomplete surfaced to the operator | PASS | `UI-HOME-004: an incomplete share is surfaced in the readiness line` | Advisory only |
| Choose-folder action is obvious | HUMAN VISUAL CHECK REQUIRED | — | Card body opens the picker |
| Start correctly gated | PASS | — | Deliberately **not** gated: sharing is not a precondition for shooting |
| Footer share status matches | HUMAN VISUAL CHECK REQUIRED | — | |

### Visual consistency

| Check | Status | Test | Notes |
|---|---|---|---|
| Accent matches approved tokens | PASS | `brand: accentPrimary is the approved #A80038` | |
| No hard-coded palette on the homepage | PASS | `home: no hard-coded colour literals remain` | |
| Duplicate identity row removed | PASS | `UI-HOME-006: the duplicated identity row is gone` | |
| One primary Tech Aim logo | **HUMAN VISUAL CHECK REQUIRED** | — | **UI-HOME-006 PARTIAL** — a logo image remains in `headerBar` |
| Four labelled groups | PASS | `UI-HOME-007: group … is labelled` ×4 | |
| Arrow means navigation only | PASS | `UI-HOME-007: the Training Lab arrow is genuine navigation` | |
| Selection indicators consistent | **HUMAN VISUAL CHECK REQUIRED** | — | **UI-HOME-007 PARTIAL** — Open Practice has none |
| Touch targets meet the minimum | PASS | `home: … use the height: 52/56/58/78 touch size` | Floor 44 px |
| Status not needlessly duplicated | **FAIL** | — | **UI-HOME-005 OPEN** |
| Helper text readable | **HUMAN VISUAL CHECK REQUIRED** | — | **UI-HOME-008 PARTIAL** — 8/9 px survives outside the action bar |
| Metadata text readable | HUMAN VISUAL CHECK REQUIRED | — | |
| Focus state visible | HUMAN VISUAL CHECK REQUIRED | — | Components implement it; homepage inline markup largely does not |
| Disabled state understandable | HUMAN VISUAL CHECK REQUIRED | — | |
| German labels do not overlap | HUMAN VISUAL CHECK REQUIRED | — | Never run with the German catalogue |

### Functional preservation

| Check | Status | Evidence | Notes |
|---|---|---|---|
| No scoring logic changed | PASS | `git diff a74d3fd..41c09a3 -- CenterPane.qml` empty | |
| No SessionStore logic changed | PASS | reliability 1041/0; no `src/reliability` diff | |
| No recovery logic changed | PASS | recovery suite passes unchanged | |
| Controller dispatch unchanged | PASS | `UI-HOME-003: controller dispatch … is unchanged` ×3 | |
| Athlete selection works | HUMAN VISUAL CHECK REQUIRED | — | No input path |
| COM-port selection works | HUMAN VISUAL CHECK REQUIRED | — | |
| Connection status updates | HUMAN VISUAL CHECK REQUIRED | — | |
| Live/Demo switching works | HUMAN VISUAL CHECK REQUIRED | — | |
| Saved-session loading works | HUMAN VISUAL CHECK REQUIRED | — | Code path untouched |
| Official / Final / Training Lab / Open Practice selection | HUMAN VISUAL CHECK REQUIRED | — | |
| Start session / Start training work | HUMAN VISUAL CHECK REQUIRED | — | |

---

## 2. Per-resolution evidence

All four are blocked on the same cause. Expected screenshot names are recorded
so evidence can be dropped in and the tables closed.

### 1536 × 960 — primary target

| Check | Status | Screenshot | Notes | Reviewing commit |
|---|---|---|---|---|
| No overlapping text or controls | BLOCKED | `vb-home-1536x960.png` (absent) | MANUAL-ASSISTED SCREENSHOT CAPTURE REQUIRED | `41c09a3` |
| No horizontal scrollbar | BLOCKED | as above | | `41c09a3` |
| No clipped event cards | BLOCKED | as above | | `41c09a3` |
| Start action visible | BLOCKED | as above | Structurally guaranteed; unverified visually | `41c09a3` |
| Load action reachable | BLOCKED | as above | | `41c09a3` |
| Final event reachable | BLOCKED | as above | | `41c09a3` |
| Selected Profile matches card | BLOCKED | as above | | `41c09a3` |
| Panels keep sensible widths | BLOCKED | as above | | `41c09a3` |
| German labels do not overlap | NOT TESTED | — | German catalogue never run | `41c09a3` |

### 1366 × 768

| Check | Status | Screenshot | Notes | Reviewing commit |
|---|---|---|---|---|
| All layout checks | BLOCKED | `vb-home-1366x768.png` (absent) | MANUAL-ASSISTED SCREENSHOT CAPTURE REQUIRED | `41c09a3` |
| All scrolling checks | BLOCKED | as above | | `41c09a3` |
| Selection consistency | BLOCKED | as above | | `41c09a3` |
| German labels | NOT TESTED | — | | `41c09a3` |

### 1280 × 720

| Check | Status | Screenshot | Notes | Reviewing commit |
|---|---|---|---|---|
| All layout checks | BLOCKED | `vb-home-1280x720.png` (absent) | MANUAL-ASSISTED SCREENSHOT CAPTURE REQUIRED | `41c09a3` |
| All scrolling checks | BLOCKED | as above | Vertical space tightest here | `41c09a3` |
| Selection consistency | BLOCKED | as above | | `41c09a3` |
| German labels | NOT TESTED | — | | `41c09a3` |

### 1100 × 700 — supported floor

| Check | Status | Screenshot | Notes | Reviewing commit |
|---|---|---|---|---|
| All layout checks | BLOCKED | `vb-home-1100x700.png` (absent) | MANUAL-ASSISTED SCREENSHOT CAPTURE REQUIRED | `41c09a3` |
| All scrolling checks | BLOCKED | as above | | `41c09a3` |
| Two-column split still viable | BLOCKED | as above | Above the 880 px stacking threshold, which is unimplemented | `41c09a3` |
| German labels | NOT TESTED | — | | `41c09a3` |

---

## 3. Sign-off

| | |
|---|---|
| Structural checks passed | 40 |
| Checks failing (open defects) | 3 — network share ×2, duplicate status ×1 |
| Checks requiring human visual review | 30 |
| Checks blocked on screenshots | 30 (per-resolution) |
| **Overall** | **NOT ACCEPTED** |

Acceptance requires: screenshots at all four resolutions, UI-HOME-004/005/009
closed, UI-HOME-006/007/008 completed, and Arnold's visual approval.
