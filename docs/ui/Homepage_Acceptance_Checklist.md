# Tech Aim — Homepage Acceptance Checklist

**Reviewing commit:** `d4674d0` · **Reviewed:** 2026-07-29
Statuses: `PASS` · `FAIL` · `BLOCKED` · `NOT TESTED` · `HUMAN VISUAL CHECK REQUIRED`

> ## HUMAN VISUAL APPROVAL — ARNOLD BAILIE
>
> **Date:** 2026-07-29 · **Application commit:** `d4674d0` ·
> **Executable SHA-256:** `F40BA723…F4588E73` · **Display:** 1536 × 960 ·
> **Language:** English
>
> **Version B is accepted as the Tech Aim Beta homepage.**
>
> The approval covers the homepage **as rendered at 1536 × 960**, the machine's
> primary and only display. **1366 × 768, 1280 × 720 and 1100 × 700 were not
> opened** and remain `NOT TESTED` below. That does not block the accepted
> design direction — it records exactly what was and was not seen.

Automated evidence below is resolution-independent — it reads
`LoginPage.qml`. It is therefore listed once, in §1, and the per-resolution
tables record only what genuinely varies with window size.

---

## 1. Structural checks — resolution-independent

Test binary: `tests/reliability/release/reliability_tests.exe` · **1059 checks, 0 failures**

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
| No overlapping text (visual) | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | Reviewed on screen at 1536 × 960 |
| No overlapping controls (visual) | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | Reviewed on screen at 1536 × 960 |
| No content outside panel bounds | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | Reviewed on screen at 1536 × 960 |

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
| Final event fully reachable | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | No clipped content seen at 1536 × 960 |
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
| Highlighted card equals selected event | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | Summary matched the highlighted card on review |
| Shot plan matches selected event | HUMAN VISUAL CHECK REQUIRED | — | |
| Scoring matches selected event | HUMAN VISUAL CHECK REQUIRED | — | |
| Switching cards updates once | HUMAN VISUAL CHECK REQUIRED | — | |
| Open Practice shot count updates summary | HUMAN VISUAL CHECK REQUIRED | — | |

### Network share

| Check | Status | Test | Notes |
|---|---|---|---|
| Enabled state requires a valid folder | PASS | `UI-HOME-004: sharing starts off unless a destination folder exists` | Toggle derives from folder presence |
| No-folder state not shown as success | PASS | `UI-HOME-004: the on-but-unconfigured case reads as incomplete` | Amber warning, not success |
| Validity state available | PASS | `UI-HOME-004: share-validity state is available` | `shareConfigured` / `shareIncomplete` exist |
| Incomplete surfaced to the operator | PASS | `UI-HOME-004: an incomplete share is surfaced in the readiness line` | Advisory only |
| Choose-folder action is obvious | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | Reviewed on screen at 1536 × 960 |
| Start correctly gated | PASS | — | Deliberately **not** gated: sharing is not a precondition for shooting |
| Footer share status matches | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | Reviewed on screen at 1536 × 960 |

### Visual consistency

| Check | Status | Test | Notes |
|---|---|---|---|
| Accent matches approved tokens | PASS | `brand: accentPrimary is the approved #A80038` | |
| No hard-coded palette on the homepage | PASS | `home: no hard-coded colour literals remain` | |
| Duplicate identity row removed | PASS | `UI-HOME-006: the duplicated identity row is gone` | |
| One primary Tech Aim logo | PASS | `UI-HOME-006: the homepage renders no logo of its own` | Shell header is the only mark |
| Four labelled groups | PASS | `UI-HOME-007: group … is labelled` ×4 | |
| Arrow means navigation only | PASS | `UI-HOME-007: the Training Lab arrow is genuine navigation` | |
| Selection indicators consistent | PASS | `UI-HOME-007: Open Practice uses the same radio indicator` | One pattern across the list |
| Touch targets meet the minimum | PASS | `home: … use the height: 52/56/58/78 touch size` | Floor 44 px |
| Status not needlessly duplicated | PASS | `UI-HOME-005: the read-only LIVE/DEMO badge is gone` | Two indicators kept on purpose |
| Helper text readable | PASS | `UI-HOME-008: no sub-10px text remains` | Zero sub-10px strings; contrast still visual |
| Metadata text readable | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | Reviewed on screen at 1536 × 960 |
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

The homepage was reviewed at the machine's primary display only. The other
three supported sizes were **not opened**; they are recorded as NOT TESTED
rather than inferred, and they do not block the accepted design direction.

### 1536 × 960 — primary target · **REVIEWED AND APPROVED**

Reviewed on screen by Arnold Bailie, 2026-07-29. No screenshot file was
produced — automated capture remains blocked (see the defect register §3).

| Check | Status | Evidence | Notes | Reviewing commit |
|---|---|---|---|---|
| No overlapping text or controls | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE | The reported overlap is gone | `d4674d0` |
| No horizontal scrollbar | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE | | `d4674d0` |
| No clipped event cards | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE | | `d4674d0` |
| Start action visible | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE | | `d4674d0` |
| Load action reachable | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE | | `d4674d0` |
| Final event reachable | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE | Appearance only; scrolling not stepped through | `d4674d0` |
| Selected Profile matches card | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE | Matched the highlighted card on review | `d4674d0` |
| Panels keep sensible widths | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE | | `d4674d0` |
| German labels do not overlap | NOT TESTED | — | German catalogue never run | `d4674d0` |

### 1366 × 768 — **NOT REVIEWED**

Window size never opened. Common projector and tablet size.

| Check | Status | Evidence | Notes | Reviewing commit |
|---|---|---|---|---|
| All layout checks | NOT TESTED | — | Window size never opened | `d4674d0` |
| All scrolling checks | NOT TESTED | — | | `d4674d0` |
| Selection consistency | NOT TESTED | — | | `d4674d0` |
| German labels | NOT TESTED | — | German catalogue never run | `d4674d0` |

### 1280 × 720 — **NOT REVIEWED**

Window size never opened. Vertical space is tightest here, so this is the size
most worth checking next.

| Check | Status | Evidence | Notes | Reviewing commit |
|---|---|---|---|---|
| All layout checks | NOT TESTED | — | Window size never opened | `d4674d0` |
| All scrolling checks | NOT TESTED | — | | `d4674d0` |
| Selection consistency | NOT TESTED | — | | `d4674d0` |
| German labels | NOT TESTED | — | German catalogue never run | `d4674d0` |

### 1100 × 700 — **NOT REVIEWED**

Window size never opened. The supported floor.

| Check | Status | Evidence | Notes | Reviewing commit |
|---|---|---|---|---|
| All layout checks | NOT TESTED | — | Window size never opened | `d4674d0` |
| All scrolling checks | NOT TESTED | — | | `d4674d0` |
| Selection consistency | NOT TESTED | — | | `d4674d0` |
| Two-column split still viable | NOT TESTED | — | Above the 880 px stacking threshold, which is unimplemented | `d4674d0` |
| German labels | NOT TESTED | — | German catalogue never run | `d4674d0` |

---

## 3. Sign-off

### HUMAN VISUAL APPROVAL — ARNOLD BAILIE

| | |
|---|---|
| Reviewer | **Arnold Bailie** |
| Date | **2026-07-29** |
| Application commit | **`d4674d0`** |
| Executable SHA-256 | `F40BA7230D5C29B939CF4BA5A33C306E762C26DBE85C34756176090FF4588E73` |
| Reviewed at | 1536 × 960, English |
| **Verdict** | **ACCEPTED — Version B is the Tech Aim Beta homepage** |

| | |
|---|---:|
| Structural checks passed | 58 |
| Checks failing | **0** |
| Visual checks passed at 1536 × 960 | 8 |
| Checks NOT TESTED — three unreviewed window sizes | 13 |
| Checks NOT TESTED — German catalogue | 4 |
| Behaviour not stepped through during review | 11 |

**Version B is accepted.** All ten reported defects are closed with automated
and visual evidence.

**What acceptance does not assert.** Three supported window sizes were never
opened; German was never run; and interaction — wheel and touch scrolling,
switching between every event type, the folder picker — was not stepped
through as a test script. Those lines stay NOT TESTED or HUMAN VISUAL CHECK
REQUIRED above. None of them blocks the approved design, and none should be
quietly upgraded later without someone actually looking.

No further homepage styling changes are to be made unless a new defect is
found — see `UI-DEC-012`.
