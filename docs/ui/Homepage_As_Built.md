# Tech Aim — Homepage As-Built

**Describes the homepage as implemented**, not the concept mockups.
**Reviewed at commit:** `41c09a3` · **Principal file:** `LoginPage.qml`

> Appearance is **HUMAN VISUAL CHECK REQUIRED**. No screenshot of this page
> exists — see §28.

---

## 1. Purpose

The idle entry screen. The operator identifies the athlete, confirms the target
connection and operating mode, chooses what is about to be shot, and starts it.
It is the only screen where the operating mode may be changed, because changing
it requires a restart and is only permitted with no session active.

## 2. Screen structure

```
┌ shell Header.qml (40 px) — product branding ─────────────────────────┐  not this page
├ headerBar (56) — "Start session" + LIVE/DEMO badge ──────────────────┤  fixed
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

Page title `Start session` plus a LIVE/DEMO badge. 56 px.
The duplicated identity row was removed (UI-DEC-005); **a logo image remains**
(UI-HOME-006, partially resolved).
*Token: `_bgHeader` → `backgroundSecondary`. Test: `UI-HOME-006` ×3.*

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
Also mirrored in `contentFooter` (UI-HOME-005, open).

## 8. Live / Demo operating mode — `opModeRow`

Two 52 px pills. Selecting one opens `opModeConfirm`
(Cancel / Restart Later / Restart Now). *Dependency: `OPMODE.selectMode()`,
`applyModeChange()`, `requestRestart()`.* `opModeHint` shows the restart
requirement — still 8 px (UI-HOME-008, partially resolved).

## 9. Network Share — `networkShareCard`

56 px card, cloud glyph, enabled/disabled text, path or
"No folder selected — click to choose", and a 46 × 26 toggle. Clicking the body
opens `networkFolderDialog`.
*Dependency: `MODREADER.setIsServerNetworkEnabled()`, `getNetworkPath()`.*

> **UI-HOME-004 is OPEN.** The card can still show an enabled state with no
> folder. `shareConfigured` / `shareIncomplete` exist and feed the readiness
> line, but the card does not yet consume them. See UI-DEC-008 (`PROPOSED`).

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
The card is `selected ? 148 : 78` — **UI-HOME-009 open**, the expanded height
has not been reduced.

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

1. UI-HOME-004 — network share can show enabled with no folder (**open**).
2. UI-HOME-005 — LIVE and Connected repeated (**open**).
3. UI-HOME-006 — a logo image remains in the page header (**partial**).
4. UI-HOME-007 — Open Practice has no selection indicator (**partial**).
5. UI-HOME-008 — 8/9 px helper text remains outside the action bar (**partial**).
6. UI-HOME-009 — expanded Open Practice card still 148 px (**open**).
7. The athlete history `▾` is inert with no feedback when no history exists.
8. No single-column layout below 880 px.
9. Loading states are undesigned.

## 27. Deferred UI migration

Homepage only (UI-DEC-004). Not migrated: live shooting UI, HUDs, dialogs,
Settings, report system. They keep legacy styling; `Theme.qml` retains every
legacy property so they are unaffected. Component adoption on the homepage is
the natural first step of the next UI phase.

## 28. Human visual review status

**HUMAN VISUAL CHECK REQUIRED — no screenshot of this page exists.**

Automated capture is blocked by endpoint security, which blocks both synthetic
input and the screen-capture helper. No bypass was attempted. Everything above
is derived from the source at `41c09a3` and from tests that read that source;
nothing here is derived from an image of the running page.

Unverified until screenshots exist: actual absence of overlap, actual scrolling
by wheel and touch, real selection propagation, layout at 1366/1280/1100,
German label behaviour, and overall visual quality.
