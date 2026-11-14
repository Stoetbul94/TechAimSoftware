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
| Final event fully reachable | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | Appearance only — no clipped content at 1536 × 960. Reaching it BY SCROLLING was not exercised |
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
| Highlighted card equals selected event | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | For the state on screen. Transitioning between every event type was not exercised |
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
| Choose-folder action is obvious | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29 | Affordance only — the picker itself was not opened |
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
| Final event reachable | PASS | HUMAN VISUAL APPROVAL — ARNOLD BAILIE | Appearance only; scrolling not stepped through (UI-HOME-002) | `d4674d0` |
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
| Defects fully closed | 7 |
| Defects appearance-approved, interaction unverified | 3 — UI-HOME-002, 003, 004 |
| Checks NOT TESTED — three unreviewed window sizes | 13 |
| Checks NOT TESTED — German catalogue | 4 |
| Behaviour not stepped through during review | 11 |

**Version B is accepted.** Every reported defect has a fix, passing automated
checks and an approved rendered result. Seven are fully closed. **Three —
UI-HOME-002, 003 and 004 — are appearance-approved but interaction-unverified**
and carry the status `RESOLVED — AUTOMATED EVIDENCE AND VISUAL LAYOUT APPROVAL;
MANUAL INTERACTION CHECK NOT PERFORMED`. Scrolling by wheel and touch,
transitioning between every event type, and the folder picker were never driven
by hand. That is a verification gap, not a design question.

**What acceptance does not assert.** Three supported window sizes were never
opened; German was never run; and interaction — wheel and touch scrolling,
switching between every event type, the folder picker — was not stepped
through as a test script. Those lines stay NOT TESTED or HUMAN VISUAL CHECK
REQUIRED above. None of them blocks the approved design, and none should be
quietly upgraded later without someone actually looking.

No further homepage styling changes are to be made unless a new defect is
found — see `UI-DEC-012`.


---

## 4. SETA athlete mode and home display settings (UI-DEC-016 / UI-DEC-017)

Branch `feature/seta-athlete-mode` · 2026-11-08 · Spec:
`docs/seta/SETA-ATHLETE-MODE-SPEC.md`

| Check | Status | Evidence |
|---|---|---|
| Athlete home decides nothing about a match (no MODREADER / APPSETTINGS / programme mapping in the file) | PASS | `tst_brandpackage.cpp` athlete checks — reliability **2521 / 0** |
| Athlete START commits through `applySetaProgramme`, then the shared `startFromLanding()` | PASS | same |
| Operator Start runs the same `startFromLanding()` (extracted verbatim) | PASS | same |
| Only catalogue-offered programmes become tiles | PASS | same |
| Returning home after a match re-locks the operator page | PASS (source) · HUMAN VISUAL CHECK REQUIRED (behaviour) | same |
| Athlete mode is off unless `athlete_mode=1` | PASS | same |
| Dark tokens unchanged; light is opt-in; accent unchanged by theme | PASS | reliability + `tests/qml` **299 / 0** |
| Global tokens untouched — match screen stays dark | PASS | reliability |
| Athlete home renders: dark/ready/selected, light/not connected, PIN pad, Display panel | PASS — RENDERED COMPONENT | `docs/seta/evidence/athlete-home-eval.png` (uirender, SETA palette) |
| No QML errors from changed files at launch, athlete mode off and on (offscreen, Demo) | PASS | launch log 2026-11-08 |
| Full flow in the running app: PIN → operator page → Back; START → match | HUMAN VISUAL CHECK REQUIRED | not driven by hand |
| Operator homepage in **light** theme | HUMAN VISUAL CHECK REQUIRED | not rendered — the homepage needs the full app context |
| German catalogue on the athlete home | NOT TESTED | new strings are not yet in `techaim_de_DE.ts` |
| 1366 × 768 and smaller | NOT TESTED | |

### 4a. Athlete groups and keyboard PIN (UI-DEC-018) — 2026-11-14

| Check | Status | Evidence |
|---|---|---|
| DSB / ISSF / Practice switcher shown; empty groups hidden | PASS | hand-run 2026-11-14, `docs/seta/evidence/athlete-groups-practice-2026-11-14.jpg` |
| ISSF group: 10m Air Rifle and Air Pistol MATCH-60, 60 shots · 75 min | PASS | hand-run 2026-11-14 |
| Practice group: 10 presets, times match the operator preview (shared `legacyMatchTimeText`) | PASS | hand-run + `tst_brandpackage.cpp` UI-DEC-018 checks — reliability **2788 / 0** |
| Switching group clears the selection | PASS (source) | same |
| ISSF AR60 from the athlete home starts a 60-shot match (log `ruleset=LEGACY discipline=AR10 shots=60`) | PASS | hand-run 2026-11-14 |
| Practice AP Match-10 starts a 10-shot pistol match (log `discipline=AP10 shots=10`) | PASS | hand-run 2026-11-14 |
| DSB 1.10 LG40 still starts with its own authority after group switching | PASS | hand-run 2026-11-14, log `ruleset=DSB … shots=40` |
| PIN by keyboard: wrong code rejected, Backspace, 1234 + Enter unlocks, Esc closes | PASS | hand-run 2026-11-14 |
| Full flow PIN → operator page → Back; START → match → Home re-locks | PASS | hand-run 2026-11-09 and 2026-11-14 (supersedes the "not driven by hand" line above) |
| 50 m lane groups (`game_distance=50`) | NOT TESTED | only the 10 m lane was run |
| German text for the group labels / new tile text | PASS | hand-run in German 2026-11-14: athlete home, Training/ISSF tabs, PIN, recovery dialog, match banner, Standstörung window |
| Practice tile times equal the clock (PRACTICE-TIME-001) | PASS | tiles show 15/30/45/60 and 90 min (config defaults); `tst_brandpackage` |
| German match report: labels, no clipping | PASS | re-checked 2026-11-15, `docs/seta/evidence/german-match-report-after-recovery-2026-11-15.jpg` — all labels German, "Ringwertung" fits |
| Report MPI / Group / Teiler update live, dash instead of NaN (REPORT-STALE-001) | PASS | same run: "— / —" before a backend shot, value after shot 5 |
| Report MPI / Group / Teiler / total time cover the WHOLE match after crash recovery | **FAIL — OPEN (REPORT-RECOVERY-001)** | same run: score 50.80 counts all 5 shots, but MTP −0.3/−1.5 and Teiler 153.8 are shot 5 alone and Gesamtzeit 0:32 is post-resume only — the cards read the C++ backend lists, which recovery does not refill (globalMatchModel is the full record) |
| Match clock keeps elapsed time across a second recovery | **FAIL — OPEN (RECOVERY-CLOCK-001)** | 2026-11-15: clock was 74:39 before power loss, recovery dialog said Restzeit 75:00 and the match resumed at ~75:00; first recovery (14 Nov) kept time correctly. Needs the ISSF/EST recovery rules before any change |
| Recovered shots keep their shot times | FAIL — OPEN | after recovery the shot list shows 0 s for every recovered shot |
| Report date in German | FAIL — OPEN | header shows "Sun 2026-11-15" (English weekday) |
