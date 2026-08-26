# Tech Aim — Homepage As-Built

**Describes the homepage as implemented**, not the concept mockups.
**Reviewed at commit:** `d4674d0` · **Principal file:** `LoginPage.qml`

> **HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29.** Accepted as the Tech
> Aim Beta homepage at commit `d4674d0`. Scope and limits in §28.

---

## 1. Purpose

The idle entry screen. The operator identifies the athlete, confirms the target
connection and operating mode, chooses what is about to be shot, and starts it.
It is the only screen where the operating mode may be changed, because changing
it requires a restart and is only permitted with no session active.

## 2. Screen structure

```
┌ shell Header.qml (40 px) — product branding ─────────────────────────┐  not this page
├ headerBar (56) — "Start session" ────────────────────────────────────┤  fixed
├ leftPanel (44%) ─────────────┬ rightPanel (56%) ─────────────────────┤
│ panelTitle "Session setup"   │ "Choose an event" + subtitle          │  fixed
│ ┌ setupScroll (Flickable) ─┐ │ weaponRow / subDisciplineRow          │  fixed
│ │ athlete                  │ │ ┌ eventScroll (Flickable) ──────────┐ │
│ │ target connection        │ │ │ OFFICIAL ISSF MATCH               │ │
│ │ operating mode + hint    │ │ │ FINALS                            │ │
│ │ network share            │ │ │ TRAINING LAB                      │ │  scrolls
│ │ summaryCard              │ │ │ PRACTICE                          │ │
│ └──────────────────────────┘ │ └───────────────────────────────────┘ │
├ actionBar (88, full width) ──────────────────────────────────────────┤  fixed
│ readinessBlock          [Load saved session 210] [Start 268]         │
├ contentFooter (34) ──────────────────────────────────────────────────┤  fixed
```

Panels anchor their bottom to `actionBar.top`, so the bar can never be overlaid.

## 3. Product header — `headerBar`

Page title `Start session` only. 56 px. No logo and no mode badge: the shell
header above carries the Tech Aim mark (UI-DEC-005 / UI-HOME-006), and the
operating mode is stated by the mode control in Session setup and by the footer
strip (UI-HOME-005). Two mode indicators are kept deliberately — Demo/Live
confusion is a result-integrity risk.
*Token: `_bgHeader` → `backgroundSecondary`. Test: `UI-HOME-005` ×3, `UI-HOME-006` ×5.*

## 4. Session setup panel — `leftPanel` / `setupScroll`

`surfacePrimary`, `radiusLarge`, 3 px accent leading edge, 22 px inset.
Fields live in a `Flickable` so they can never clip.
*Tokens: `surfacePrimary`, `borderSubtle`, `panelPadding`. Test: `home: the setup column scrolls`.*

## 5. Athlete selector — `athleteBox` / `name_text_field`

`TextInput`, max 20 chars, 52 px, placeholder "Athlete name" when empty and
unfocused. A `▾` opens `userHistoryList` **only when history exists**
(`APPSETTINGS.getUserHistoryCount() > 0`) — on a fresh install the control is
inert with no feedback. *Dependency: `APPSETTINGS`.*

## 6. Target connection — `connRow` / `port_name_text_field`

COM port text field (max 5 chars) + connect/disconnect button, 52 px. Visible
only when `showComportConnector`. *Dependency: `MODREADER.connectedModbus()` /
`disconnectModbus()` / `isModBusConnected()`.*

## 7. Connection status — `connStatusBtn`

148 × 52. Connected → `successBackground`/`successText` "Connected".
Otherwise "Not connected" (Live) or "Demo · not needed" (Demo).
Also summarised once in `contentFooter`; the page heading no longer repeats it
(UI-HOME-005).

## 8. Live / Demo operating mode — `opModeRow`

Two 52 px pills. Selecting one opens `opModeConfirm`
(Cancel / Restart Later / Restart Now). *Dependency: `OPMODE.selectMode()`,
`applyModeChange()`, `requestRestart()`.* `opModeHint` states the restart
requirement at `helperText` size in `warningText` — it was 8 px muted grey,
the least visible text on the screen (UI-HOME-008).

## 9. Network Share — `networkShareCard`

56 px card, cloud glyph, enabled/disabled text, path or
"No folder selected — click to choose", and a 46 × 26 toggle. Clicking the body
opens `networkFolderDialog`.
*Dependency: `MODREADER.setIsServerNetworkEnabled()`, `getNetworkPath()`.*

The toggle derives from whether a folder exists, so an enabled-success state
cannot occur without one. On-but-unconfigured renders as an amber
"Share incomplete" in the card, the footer and the readiness line. Enabling
with no folder opens the picker. **Sharing never gates Start** — it is a
convenience, not a precondition for shooting (UI-DEC-008 ACCEPTED).
*Test: `UI-HOME-004` ×8.*

## 10. Selected profile summary — `summaryCard`

One card: `selectedProgrammeLabel()` heading, `selectedProgrammeName()` at 17 px
bold, an optional 3P position-flow line, a divider, then `infoTiles` — a
3-column grid of quiet label/value pairs (shot plan, scoring, prep, match,
distance, rules; or the programme's own metrics when Training/C&D/PT is
confirmed). Replaced six equally loud bordered tiles.
*Test: `UI-HOME-003` summary checks.*

## 11–15. Event selection groups — `eventScroll` / `eventColumn`

Four labelled groups in order:

| Group | Contents | Notes |
|---|---|---|
| **OFFICIAL ISSF MATCH** | `EventCard{4}` — 60 shots | |
| **FINALS** | `EventCard{6}` 3P FINAL (35), `EventCard{7}` 10m FINAL (24) | Heading visible only when a final applies to the current discipline |
| **TRAINING LAB** | gateway card → `practiceView = 1` | Arrow = genuine navigation |
| **PRACTICE** | Open Practice card | |

`EventCard`: 78 px, badge · title · one subtitle · radio indicator; selected =
`accentSubtle` fill + `accentPrimary` 2 px border. Weapon (`weaponRow`, 58 px)
and distance/sub-discipline (`subDisciplineRow`, 44 px) selectors sit above the
scroll region and do **not** scroll. *Test: `UI-HOME-007` ×5.*

## 16. Shot-plan selection

Inside the Open Practice card, revealed when selected: 84 × 48 preset chips
(10/20/30/40/No limit; 20/40/No limit at 50 m) setting `gameEvent` 0–3/5.
Collapsed, the card is 78 px — identical to every other event card. Selected,
it grows by the preset row plus one gap only (`78 + 48 + 8`), and carries the
same radio indicator as every EventCard (UI-HOME-007). The expanded content
stays inside the event Flickable. *Test: `UI-HOME-007` ×3, `UI-HOME-009` ×2.*

## 17. Bottom action bar — `actionBar`

88 px, full width, outside every scroll container.
Left `readinessBlock`, right `actionRow` (fixed 490 px, right-anchored).
*Test: `UI-HOME-001` / `UI-HOME-010` ×8.*

## 18. Readiness status — `readinessBlock`

"READY TO START", or "CHECK BEFORE STARTING" in `warningText` when
`shareIncomplete`. Second line is `readinessSummary()` — athlete · programme ·
mode, or the share warning. **Advisory only; never blocks Start**, because
sharing is a convenience, not a precondition for shooting.

## 19. Load saved session

210 × 56 secondary button. `APPSETTINGS.uploadGame()` then restores user, mode,
event and type, and calls `startButtonClickedOnLoadGame()`. **Unchanged.**

## 20. Start session / Start training

268 × 56 primary. Label from `startButtonText()`. The handler dispatches on
`ptConfirmed` → `POSTRANS.startPositionTransition`, `cdConfirmed` →
`CALLDIAG.startCallDiagnose`, `trainingConfirmed` → `TRAINING.startTraining`,
otherwise the qualification path. **Dispatch is byte-identical to before
Version B.** *Test: three `controller dispatch … is unchanged` checks.*

## 21. Scroll behaviour

| Region | Behaviour |
|---|---|
| `setupScroll` | Flickable, `StopAtBounds`, scrollbar when overflowing |
| `eventScroll` | Flickable, `contentHeight` bound to `eventColumn.height`, scrollbar `AlwaysOn` while overflowing, 20 px bottom padding, `contentWidth: width` so no horizontal scroll |
| header, weapon row, action bar, footer | never scroll |

## 22. Responsive behaviour

Left 44 % / right 56 %. Rules in `docs/design/Screen_Layout_Rules.md`.
**Not implemented:** single-column stacking below 880 px. Behaviour at
1366/1280/1100 is **unverified** — see §28.

## 23. Selection-state data flow

```
weapon / distance / sub-discipline / event card click
        ↓ sets gameMode · gameRange · gameSubMode · gameEvent
        ↓ clears trainingConfirmed / cdConfirmed / ptConfirmed
selectedProgrammeKind()
        ├→ selectedProgrammeLabel()  → summary heading
        ├→ selectedProgrammeName()   → summary name + readiness recap
        └→ startButtonText()         → Start button
gameEvent → getGameEventText / getMatchTime → infoTiles
gameMode / gameEvent → main.qml updateGameType() → shootingPage.setCurrentGameType()
```

One derivation, four consumers. *Test: `UI-HOME-003` ×19.*

## 24. Design tokens used

`backgroundPrimary` `backgroundSecondary` `surfacePrimary` `surfaceSecondary`
`inputBackground` `borderSubtle` `borderStrong` `accentPrimary` `accentHover`
`accentPressed` `accentSubtle` `textOnAccent` `textPrimary` `textSecondary`
`textDisabled` `successText` `successBackground` `errorBackground`
`warningBackground` `warningText` `scrim`; typography `label` `body`
`buttonText`; spacing via literals still in places.

**Zero hard-coded colour literals** — asserted by
`home: no hard-coded colour literals remain`.

## 25. Reusable components used

**None.** `TaButton`, `TaStatusChip` and `TaWarningBanner` exist in
`src/ui/components/` and are exercised by the gallery, but the homepage still
implements these patterns inline. Swapping verified markup for new components
in the same phase that introduced them would risk regression for no user-visible
gain. See §27.

## 26. Known limitations

All ten reported defects (UI-HOME-001…010) now have a fix and automated
evidence. **None is fully closed** — closure needs a real screenshot and none
exists (§28). Remaining limitations are separate from that defect list:

1. The athlete history `▾` is inert with no feedback when no history exists.
2. No single-column layout below 880 px (rule defined, not built).
3. Loading states are undesigned.
4. Keyboard focus is implemented in the shared components but not in most of
   the homepage's inline markup.
5. The page has never been run with the German catalogue.

## 27. Deferred UI migration

Homepage only (UI-DEC-004). Not migrated: live shooting UI, HUDs, dialogs,
Settings, report system. They keep legacy styling; `Theme.qml` retains every
legacy property so they are unaffected. Component adoption on the homepage is
the natural first step of the next UI phase.

## 28. Human visual review status

### HUMAN VISUAL APPROVAL — ARNOLD BAILIE

| | |
|---|---|
| Reviewer | Arnold Bailie |
| Date | 2026-07-29 |
| Application commit | `d4674d0` |
| Executable SHA-256 | `F40BA7230D5C29B939CF4BA5A33C306E762C26DBE85C34756176090FF4588E73` |
| Reviewed at | 1536 × 960, English |
| Verdict | **ACCEPTED** as the Tech Aim Beta homepage (UI-DEC-012) |

Reviewed and approved: layout, spacing, the action bar, the event groups, the
selected-programme summary, the network share state, typography and branding.

### What the approval does not cover

- **1366 × 768, 1280 × 720 and 1100 × 700 were never opened** — NOT TESTED.
- **German was never run** — the catalogue has not been loaded on this page.
- **Interaction was not stepped through** as a test script: wheel and touch
  scrolling, switching between every event type, and the folder picker. The
  three defects that concern those interactions (UI-HOME-002, 003, 004) are
  therefore recorded as `RESOLVED — AUTOMATED EVIDENCE AND VISUAL LAYOUT
  APPROVAL; MANUAL INTERACTION CHECK NOT PERFORMED` rather than fully
  closed. Structure and rendered result are verified; the behaviour is not.
- **No screenshot file exists.** Automated capture remains blocked by endpoint
  security, which blocks both synthetic input and the screen-capture helper.
  No bypass was attempted. The approval is a reviewer sign-off against the
  running build, not an image artefact.

Everything else in this document is derived from the source at `d4674d0` and
from tests that read that source.


## Header bar — Settings entry (2026-08-26)

The start page's 56 px header carries the page title on the left and, since
UI-THEME-001, a **Settings** control on the right: a bordered pill with a
drawn gear glyph and the label `Settings`, hover-lit via `surfaceSecondary`
and `borderStrong`.

The glyph is drawn in QML rather than imported. The icon set in `images/` has
no light-theme variant, and a dark-only PNG would go invisible on the light
canvas — the same failure the header logo had (UI-THEME-LOGO-001).

Clicking it opens `rootItem.openAppearanceDialog()`, which uses the one
dialog framework (`dialogManager` / `TechAimDialog`) with three buttons —
System, Light, Dark. The active choice is carried as the accent button and
marked with a check, so the current setting is visible without a second
control. Dismissing changes nothing; the cancel result is the current value.

The setting is presentation only and is deliberately placed before session
entry, which is the one moment changing appearance can disturb nothing.
