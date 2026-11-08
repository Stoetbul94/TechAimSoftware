# SETA Athlete Mode — specification

Branch `feature/seta-athlete-mode`, cut from SETA 1.0.0-EVAL4 (`9e3acd5`).
Decisions: **UI-DEC-016** (athlete mode), **UI-DEC-017** (home display settings).

## 1. Purpose

A minimal, hard-to-get-wrong home screen for an athlete shooting alone on a
SETA lane. **Same engine, same rules, same scoring, same target connection —
fewer controls.** Athlete mode only hides and reorders presentation; every
start goes through the operator homepage's existing start path.

## 2. What must not change

Scoring (`calculateShootingSocre`), ring geometry, DSB/ISSF rule data,
`CompetitionCatalogue` runtime mapping, timing, acquisition, paper feed,
recovery, the journal, and the target connection code. The COM port is the one
saved by the operator; athlete mode never opens or changes a port by itself.

## 3. Configuration (`config.ini`, `[App_Settings]`, read at startup)

| Key | Default | Meaning |
|---|---|---|
| `athlete_mode` | `0` | `1` opens the athlete home instead of the operator homepage |
| `operator_pin` | *(empty)* | PIN that unlocks the operator homepage. Empty = no PIN (a confirm only) |
| `athlete_programmes` | *(empty)* | Comma-separated `programmeId`s shown as tiles. Empty = the default set for the lane distance (§4) |
| `home_theme` | `dark` | `dark` or `light` for the home screens. Written by the home Display settings |

Language stays where it already lives (`ui_language`, `LanguageService`).

## 4. Default programme tiles

Only programmes that exist in the catalogue **and** pass
`programmeAvailable()` are shown; an unknown id in the config is skipped.

| Lane (`game_distance`) | Default tiles |
|---|---|
| 10 m | DSB 1.10 LG 40, 1.10 LG 60, 1.20 3×10, 1.20 3×20, 2.10 LP 40, 2.10 LP 60 |
| 50 m | DSB 1.80 KK-Liegend, 1.40 KK 3×20, 1.60 KK 3×40, 2.20 Pistole 60 |

## 5. Athlete home

```
┌───────────────────────────────────────────────────────────┐
│ Ready to shoot                       ● Target ready       │
│                                                           │
│ YOUR NAME  [ Max Mustermann                           ]   │
│                                                           │
│ CHOOSE YOUR EVENT                                         │
│ [ 1.10  Luftgewehr  40 ] [ 1.10  Luftgewehr  60 ]         │
│ [ 1.20  3-Stellung 3×20] [ 2.10  Luftpistole 40 ]         │
│                                                           │
│ [⚙ Display]   [Operator]             [   START   ]        │
└───────────────────────────────────────────────────────────┘
```

- **Status pill**, one of three, plain words: *Target ready* (green),
  *Target not connected — check the cable* (red), *DEMO — no live target*
  (amber). Demo must stay unmissable (result integrity).
- **START** is enabled only with a name and a selected event. It commits the
  tile through `applySetaProgramme()` and then runs the landing page's own
  start handler — the same function the operator's Start button calls, so the
  start gate, connection checks, rule-authority save and polling start are
  shared, not duplicated.
- **Operator** opens the PIN pad (or a confirm when no PIN is set). Unlocking
  shows the full operator homepage, which carries a *Back to athlete view*
  button.
- **Display** opens Theme (Dark / Light) and Language (English / Deutsch).

## 6. Display settings (UI-DEC-017)

Available on **both** home screens. Theme applies to the **home screens only**
(athlete home and operator homepage) through a separate token instance; the
match screen, reports and HUDs stay dark until they are migrated to tokens
(UI-DEC-004 scope). Language switches the whole application live through the
existing `LANGUAGE.selectLanguage()`.

## 7. Increments

| # | Scope | Status |
|---|---|---|
| 1 | Config keys, home light/dark tokens, athlete home, operator PIN, Display popup | this change |
| 2 | Match screen in athlete mode: hide Settings / Training Lab / coach entries, large score + shots left + clock | not started |
| 3 | Simple end-of-match screen: total, Print, PDF, New match | not started |
| 4 | Physical test on a SETA target (EVAL4 checklist C, E, H + athlete flow) | not started |
