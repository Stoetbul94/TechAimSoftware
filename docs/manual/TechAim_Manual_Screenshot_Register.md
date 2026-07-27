# Tech Aim Manual — Screenshot Register

Document version 1.2 (P0.1) · Application commit `169eef9`

**No screenshots have been captured (P0.1 re-confirmed).** Driving the Qt
desktop application and capturing its window is not possible in this
environment, so every entry below remains **PENDING** with human capture
instructions. Placeholders are *not* committed as images —
`docs/manual/images/` stays empty until real captures exist, so a missing
image can never be mistaken for an approved one.

**Diagrams, by contrast, are COMPLETE** — all 11 are generated from committed
source and embedded in the operator manual (see the Diagrams section).

## Capture rules (apply to every screenshot)

- **Operating mode: Demo.** No physical target, no real results.
- **Synthetic identity only** — athlete **Alex Example**, club **Tech Aim Demo
  Club**. Never a real athlete or club.
- **No personal information**, no real confidential results.
- **No private filesystem paths** — a personal user-profile folder must never
  be visible. Export to a neutral folder before capturing any path.
- **No development artefacts** — no Qt Creator, no console, no repository
  paths.
- **Identity must be current**: the window must read *Tech Aim Electronic
  Target Control*.
- **Window size 1536 × 960** unless the entry says otherwise; PNG.
- Capture the **application window only**, not the whole desktop.

**A screenshot is REJECTED outright if it shows any of:**

| Rejection cause | Why |
|---|---|
| Old **Seta / Seeds** software identity | superseded product name |
| The **"Hello World"** window title | pre-P0 defect, now fixed |
| An obsolete **`Seta.exe`** anywhere (title bar, path, taskbar) | superseded executable |
| The **old end-user agreement artwork** | names an entity other than JAC SHOOTING SOLUTIONS (PTY) LTD — **LEGAL REPLACEMENT REQUIRED BEFORE EXTERNAL BETA** |
| Personal athlete data or real confidential results | privacy |
| Developer filesystem paths | leaks the build environment |

Any capture showing one of these must be deleted and retaken, not cropped.

Brand artwork rules for manual documents: `_shared/brand-assets.md`.

## Callout legend (use consistently)

```
1  Athlete and discipline      5  Target display
2  Operating mode              6  Primary action
3  Connection status           7  Session progress
4  Current phase               8  Home / exit action
```

Numbered circular callouts, Tech Aim red `#C40046` with white numerals,
placed clear of the information they reference. **Never cover a value the
reader needs to read.**

## Register

Status: **PENDING** = not captured · **CAPTURED** = image exists ·
**VALIDATED** = image checked against the capture rules and the manual text.

| ID | Manual section | Lang | Screen | Discipline | Mode | Required state / data | Callouts | Status |
|---|---|---|---|---|---|---|---|---|
| SS-01 | Quick Start 12 · Manual Part 3 | EN | Home screen | — | Demo | fresh launch, no session | 1,2,3,8 | PENDING |
| SS-02 | Part 3 | EN | Athlete / session details | 10m AR | Demo | synthetic athlete selected | 1 | PENDING |
| SS-03 | Part 5 | EN | Discipline selection | — | Demo | all supported disciplines visible | — | PENDING |
| SS-04 | Part 5 | EN | Event selection | 10m AR | Demo | event list for the discipline | — | PENDING |
| SS-05 | Part 7 | EN | Training Lab catalogue | 10m AR | Demo | shows programmes available for a **non-3P** discipline | — | PENDING |
| SS-06 | Part 7 · Part 11 | EN | Training Lab catalogue | 50m 3P | Demo | **Position Transition present** — contrast with SS-05 | — | PENDING |
| SS-07 | Part 8 | EN | Technical Blocks setup | 10m AR | Demo | focus + visibility mode set | — | PENDING |
| SS-08 | Part 8 | EN | Technical Blocks active block | 10m AR | Demo | mid-block, right panel shows Shot n of N; **no match timer, no red 000** | 4,6,7 | PENDING |
| SS-09 | Part 8 | EN | Block Review | 10m AR | Demo | ≥5 counted shots so metrics + pattern populate | 5 | PENDING |
| SS-10 | Part 8 | EN | Technical Blocks final summary | 10m AR | Demo | ≥2 blocks so BLOCK COMPARISON populates | — | PENDING |
| SS-11 | Part 9 | EN | Call & Diagnose awaiting call | 10m AP | Demo | actual impact **hidden**, CONFIRM CALL visible | 4,6 | PENDING |
| SS-12 | Part 9 | EN | Call & Diagnose reveal | 10m AP | Demo | CALL + ACTUAL + connecting vector visible | 5 | PENDING |
| SS-13 | Part 9 | EN | Call & Diagnose summary | 10m AP | Demo | enough shots for typical accuracy + bias | — | PENDING |
| SS-14 | Part 11 | EN | Position Transition setup | 50m 3P | Demo | sequence K→P→S, verification shots set | — | PENDING |
| SS-15 | Part 11 | EN | POSITION SETUP phase | 50m 3P | Demo | setup timer running, checklist visible | 4,6 | PENDING |
| SS-16 | Part 11 | EN | Position sighters | 50m 3P | Demo | after POSITION READY, ≥1 sighter fired | 4 | PENDING |
| SS-17 | Part 11 | EN | Verification active | 50m 3P | Demo | Shot n of N; **no match timer, no red 000** | 4,7 | PENDING |
| SS-18 | Part 11 | EN | Position Review | 50m 3P | Demo | timing cards + group plot + pattern | 5 | PENDING |
| SS-19 | Part 11 | EN | Transition prompt | 50m 3P | Demo | BEGIN TRANSITION TO … visible | 6 | PENDING |
| SS-20 | Part 11 | EN | POSITION TRANSITION COMPLETE | 50m 3P | Demo | **full K→P→S** so HIGHLIGHTS + all three position cards populate | — | PENDING |
| SS-21 | Part 16 · QS 5 | EN | Settings — LANGUAGE | — | Demo | English selected, Deutsch (Beta) visible | — | PENDING |
| SS-22 | Part 4 · Part 16 | EN | Settings — OPERATING MODE | — | Demo | Current mode: Demo | 2 | PENDING |
| SS-23 | Part 14 | EN | Recovery dialog | 10m AR | Demo | force-close mid-session, then relaunch | — | PENDING |
| SS-24 | Part 15 | EN | Incident dialog | 10m AR | Demo | category + scope selection visible | — | PENDING |
| SS-25 | Part 12 | EN | Exported PDF — page 1 | 50m 3P | Demo | Position Transition report, logo + header | — | PENDING |
| SS-26 | Part 12 | EN | Exported PDF — comparison page | 50m 3P | Demo | **check for overflow** after the highlights/rhythm additions | — | PENDING |
| SS-27 | Part 2 · Part 18 | EN | Settings — ABOUT / BUILD | — | Demo | version, commit, built, publisher | — | PENDING |
| SS-28 | German docs | DE | Home screen | — | Demo | Deutsch (Beta) active | 1,2,3 | PENDING |
| SS-29 | German docs | DE | Training Lab active screen | 50m 3P | Demo | shows the **mixed-language** reality honestly | 4,6 | PENDING |
| SS-30 | German docs | DE | Settings — LANGUAGE | — | Demo | Deutsch (Beta) selected + beta note | — | PENDING |
| SS-31 | Part 2 · brand assets | EN | Windows application icon (Explorer + taskbar) | — | — | **BLOCKED — no approved `.ico` exists.** Do not invent, mock or substitute an icon. | — | PENDING — BLOCKED |

## Human capture instructions

0. **Set `app_mode=Demo`** in `release/config.ini` before capturing, and
   **restore `app_mode=Live` immediately afterwards**.
1. Build and launch `TechAim.exe`; confirm Demo in Settings.
2. Set the window to 1536 × 960.
3. Use athlete **Alex Example**, club **Tech Aim Demo Club**.
4. Work through the register in order — SS-05/SS-06 must be captured as a pair
   (non-3P vs 3P) to show the Position Transition gating.
5. For SS-08 and SS-17, **actively confirm** no match countdown and no red
   `000` are present; these screenshots are the visual evidence for that fix.
6. For SS-20, complete a **full** K→P→S session — a partial session leaves the
   highlights and comparison bars unpopulated and misrepresents the feature.
7. For SS-23, force-close mid-session (Task Manager) and relaunch.
8. For SS-28…SS-30, switch to Deutsch (Beta) first and let it apply.
9. Save as `docs/manual/images/SS-NN_short_name.png`.
10. Apply callouts per the legend; update Status to CAPTURED, then VALIDATED
    once checked against the capture rules.

## Diagrams

**All 11 diagrams are COMPLETE.** Source:
`diagrams/make_diagrams.py` (run it to regenerate every SVG). SVG was chosen
over Mermaid because the publication pipeline is Pandoc -> HTML -> headless
Chromium, which does not run JavaScript at print time — a Mermaid fenced block
would appear as raw code in the PDF.

All 11 are embedded in `TechAim_Operator_Manual_EN.md` and therefore appear in
the generated PDF.

| ID | Purpose | Status |
|---|---|---|
| DG-01 | Application workflow: launch → discipline → event/programme → session → results | COMPLETE |
| DG-02 | Live vs Demo, and the source gate that rejects mismatched input | COMPLETE |
| DG-03 | Session lifecycle: new → active → completed → closed | COMPLETE |
| DG-04 | Recovery lifecycle: interruption → detection → resume/discard | COMPLETE |
| DG-05 | Training Lab programme selection and discipline gating | COMPLETE |
| DG-06 | Technical Blocks workflow | COMPLETE |
| DG-07 | Call & Diagnose workflow (hidden actual → call → confirm → reveal) | COMPLETE |
| DG-08 | Position Transition workflow (setup → ready → sighters → verification → review → transition) | COMPLETE |
| DG-09 | Report and export workflow | COMPLETE |
| DG-10 | Troubleshooting escalation path | COMPLETE |
| DG-11 | Update process | COMPLETE (renders the gap explicitly; the process itself stays WINDOWS RC1 DEPENDENT) |
