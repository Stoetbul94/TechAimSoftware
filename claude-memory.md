# Claude working memory — TechAim seta10

> Handoff notes from the AI-assisted sessions of 2026-07-01 → 2026-07-03.
> Read together with `CLAUDE.md` (repo intro) and `WORKFLOW_NOTES.md` (feature docs).
> Newest state is on branch `feature/techaim-rebrand`.

---

## 1. Build & run

- Qt 6.5.3 MinGW 64-bit. Build: add `C:\Qt\Tools\mingw1120_64\bin` and
  `C:\Qt\6.5.3\mingw_64\bin` to PATH, then `mingw32-make -f Makefile.Release -j6`
  from the repo root. Re-run `qmake Seta.pro -spec win32-g++` after adding files
  to a `.qrc` (Makefile deps are generated at qmake time).
- QML lives in `qml.qrc` → **every QML change needs a rebuild** (rcc + relink).
- The exe locks during linking — kill `Seta.exe` before rebuilding.
- `release/config.ini` (untracked) — **required contents proven at the range
  2026-07-04** (first successful live-fire session):

  ```ini
  [shot_count_and_timer]
  timer=yes

  [App_Settings]
  app_mode=Live
  is_single_decimal=1
  motor_movement_time=1
  motor_movement_time_sighter=1
  ```

  - `app_mode=Live` is the ONLY way to leave demo mode — the LIVE/DEMO chip on
    the login page just displays it, nothing in the UI sets it. Without it the
    app silently ignores all hardware shots ("shots not registering").
  - `is_single_decimal=1`: this hardware (COM7, RTU 19200 Even 8/1) reports
    shot coordinates in **tenths of a millimetre**; the default (0 → ÷100)
    plots every shot 10× too close to centre (all shots look like 10.7+).
    Value mirrored from the known-good old install at
    `Desktop/Tech Aim Software/seta10/config.ini`.
  - Both are read once at startup — config changes need an app restart.
- Demo-mode test drill: launch → click green **Connected** (→ Demo/Offline) →
  Share toggle OFF (or configure a folder) → pick discipline → Start session →
  play button starts the match → **click the target to fire demo shots**.

## 2. ISSF implementation status (all live-verified in demo)

| Feature | State |
|---|---|
| Discipline selector: Pistol / Rifle → 10m/50m → Prone/3P | done |
| Match times: 10m 75min · Prone 50min · 3P **90min (EST)** | done (105 is paper/outdoor only) |
| Preparation/sighting phase (15 min) before every match | done — play button or timer expiry starts match |
| 3P state machine K→P→S (20/40 boundaries, auto sighting break) | done — match clock runs through changeovers per ISSF |
| START PRONE / START STANDING resume button (CenterPane, own signal) | done |
| Phase label: SIGHTING/MATCH · position · MATCH COMPLETE | done |
| Auto match-finish at final shot (all disciplines, clock stops) | done |
| 3P match report PDF (Report3P.qml) | done — see §4 |
| Shots carry `position` (0/1/2/-1) and `xmm/ymm` roles | done |

Key commits: `d63b02a` selector · `2c67b74` timer plumbing · `8ea95fe` prep phase ·
`8b1591a` 3P machine · `1dfd1e7` auto-finish · `7ae5e42` Home-crash fix ·
`ee06756` share guard · `59636fe` 3P report.

## 3. Hard-won technical gotchas (do not rediscover)

1. **Unqualified root-member lookups fail silently from nested handlers**
   (`Timer.onTriggered`, `MouseArea.onClicked`) under the compiled-QML cache.
   Always qualify: `shootingPage.fn()`, `paneItem.signal()`. Writes to
   ancestor-scope properties from other documents are also silent no-ops — use
   signals (`LoginPage.rangeSelected` → `main.qml`).
2. **`TachusWidget::changeSighterMode` is not thread-safe**: its QList swaps race
   the 100 ms polling worker; with stored shots it heap-corrupts and crashes.
   3P transitions deliberately skip it (QML `sligterMode` handles routing).
   Consequence: backend save/CSV files tag mid-match position sighters as match
   shots — proper fix is a mutex in C++ (open item).
3. **ListModel role sets lock at first append**: `globalMatchModel`,
   `globalSlighterModel`, `globalModelOfData` must keep identical append shapes
   (`position`, `xmm`, `ymm` included) or cross-model copies throw silently.
4. **`RightPanel.updateTotal()/updateListModel()` must not run while hidden** —
   they now early-return when `!visible`; removing that guard reintroduces a
   100%-reproducible native crash on Home with shots present.
5. **LogFile drops entries under thread contention** (`m_previousData` is never
   flushed). For debugging QML, append markers to a visible property (e.g.
   `phaseText`) instead of trusting the log.
6. **Timer arming from the shot pipeline fails** (`restart()` reports running but
   never fires). Use declaratively-armed repeating timers (`running:` binding) —
   see `positionWatch` / `matchCompleteWatch` in `ShootingPage.qml`.
7. Watch for **stray `gameEvent` indices**: Prone and 3P share `gameEvent=4`;
   the discipline is `APPSETTINGS.getGameSubMode()` (+`get10or50mRange()`), both
   persisted from LoginPage. `is3PMatch` is fixed at `beginPreparationPhase()`.
8. **A leading space in the port text field silently killed live mode at the
   range**: Start-session blindly called `connectedModbus(" COM7")`, which tore
   down the already-working COM7 connection and failed to reopen (`" COM7"` is
   an invalid device name) → "Com port not connected". Fixed in
   `connectedModbus`: trims the name and returns early (keeping the live
   connection) when already connected to the requested/last port.
9. **C++ getters called from delegate bindings must reject the whole negative
   range, not just `-1`.** Pressing play with sighter shots crashed the app
   natively (2026-07-04): entering match mode empties the score list, so
   `updateListModel` computed `currentPageIndex = floor(-1/10) = -1`; the three
   not-yet-destroyed sighter delegates re-evaluated
   `getTeilerForShootOfMatch(currentPageIndex*10 + index)` with −10/−9/−8, and
   the old guard (`shootNumber == -1` only) let `m_xCordList.at(-10)` run →
   heap crash. Fixed both layers: `shootNumber < 0` guard in C++, `Math.max(0,…)`
   clamp in QML. Delegate destruction is deferred — assume any index a binding
   computes can be stale/negative during model churn.
10. **Automation/testing quirk**: the app window drifts ±15 px between states;
   synthetic clicks must be based on a fresh screenshot or they miss silently.
   Bitdefender: `release\Seta.exe` needs an **Advanced Threat Defense** exception
   (separate from the Antivirus folder exclusion) or file writes freeze the UI.

## 4. 3P match report (`Report3P.qml`)

- Page 1 of the match report when `is3PMatch` (others keep the legacy page):
  TechAim colour logo, meta line, position table (Series A/B, subtotal dec(int),
  inner 10s, group mm, MPI mm, s/shot), grand total, series trend strip
  (best green / worst red), three per-position target plots, score
  distribution, best/worst shot, RO signature footer. Sighters excluded.
- Stats computed in `refresh()` from `globalMatchModel` (mm from `xmm/ymm`;
  50 m face constant 154.4 mm; pellet `APPSETTINGS.bullet_diameter()`).
- `MatchReport.qml` grabs `report3p` instead of `print_region` when 3P;
  `CUSTOMPRINT.createPdf()` opens a native Save dialog (known quirk: typing a
  name may append to "untitled.pdf" in automation).

## 5b. ShootingPage redesign — DONE & verified across disciplines (2026-07-04)

The shooting UI is ONE shared page (`ShootingPage`/`LeftPanel`/`CenterPane`/
`RightPanel`) used by every discipline; the redesign therefore applies to all.
Verified live in demo: 10m Air Pistol, 10m Air Rifle, 50m Rifle 3 Pos (50m
Prone shares the 3P page). Delivered:
- Header status strip with phase stepper (SIGHT·MATCH, or SIGHT·KNEEL·PRONE·
  STAND for 3P); redundant on-target phase pills + phase word removed.
- Bottom action bar: one context-aware full-width button + Feed paper.
- Left panel: full-height labelled nav (Stats/Report/Coach report/Group·MPI/
  Settings/Home), compact discipline header, crisp vector icons (`VIcon.qml`,
  QtQuick.Shapes) incl. realistic pistol/rifle silhouettes.
- Right panel cards: LAST SHOT (score+dir+X/Y mm), dark shot log with a
  **Time (s)** column (was the German "Teiler"; now per-shot split seconds via
  the timeComsumed role), TOTAL card (total+inner10s+time) with a colour-coded
  horizontal distribution bar chart (10/9/8/≤7), aligned S1-S6 series cards.
  Full-width dark bg covers the legacy total_score_block PNG chrome.
Fragility preserved: all hidden data-holder Texts + onTextChanged backend
pushes (setTotalScoreWD/updateSeriesScore/…) kept alive at opacity 0.

Parked design polish (user OK'd for later): colour-code shot-log scores;
tint/tag sighter shots; highlight the active series card.

## 5a. Range list from first live day (2026-07-04) — do AFTER the full redesign

User's backlog, in his priority order (design first, then these):
1. **Redesign must reach every discipline** — Prone, 10m Air Rifle, 10m Air
   Pistol, etc. each get the same layout adapted to its own discipline
   (its phases, series count, times).
2. **Verify match/prep times per discipline** against ISSF rules.
3. **Auto-zoom on shot** (like SIUS Ascor): after a shot lands, the view
   zooms in on the shot briefly/persistently. Series + last shot are the
   most important info to keep visible.
4. **Coach report** — third report type next to Stats/Report (button added
   in redesign; content TBD with user).
5. **Motor feed time settings reachable from the login/home page** too, not
   only SettingsPage (rarely visited). Keep manual "Feed paper" button on
   the shooting page as a jam-recovery fallback (motor is otherwise fully
   automatic after each shot).
6. Report bug seen in kn.pdf: an empty series still prints ("SERIES: 3
   Group: 0 mm MPI: NaN, NaN") — skip empty series.
Design principles he stated: not too complicated, professional, functional,
user-friendly; the series and the last shot are the key data.

## 5. Open items / next steps

1. **User to confirm**: the "phantom dot" on the 2nd shot of a phase is most
   likely the **MPI marker** (visible when count > 1, drawn at the mean point).
   If confirmed, restyle as crosshair or hide during sighting.
2. Backend sighter tagging for 3P (thread-safe `changeSighterMode`, §3.2).
3. "Match time used" on the report uses shot-timer accumulation, not wall time.
4. Match Performance count cell in RightPanel shows an unrelated number (pre-existing).
5. Continue rebrand: ShootingPage, SettingsPage, RightPanel, MatchReport, SummaryPage.
6. Awaiting user decisions: 25m Pistol scope; ISSF Finals format.
7. Nice-to-haves parked: load-session dialog default dir; per-position report
   pages for non-3P; MPI/group per series on page 1.

## 6. Verification evidence (2026-07-03)

Full 60-shot 3P demo match: K 210.3 / P 214.4 / S 201.9 → total 626.6 (595),
inner-10s 49; sighting 15:00 countdowns; match clock 90:00 → ran through both
changeovers; MATCH COMPLETE froze clock at 86:31; PDF result sheet rendered
correctly with standing group visibly widest (as shot).

## 7. ShootingPage redesign plan (agreed 2026-07-03, in progress)

Direction: mockup 2/3 style (dark + #e8003d red, Theme.qml tokens, like LoginPage)
with mockup 1's phase stepper (Prep → Sighting → K → P → S) and score-distribution
bars. Phased, each phase verified live then committed:
1. Top status strip: event, shooter, phase chip (phaseText, amber/red/green),
   shots counter (globalMatchModel.count/matchShootCount), DEMO chip. Resume
   button may move here later.
2. Right panel: dark cards — LAST SHOT (big score + X/Y mm from xmm/ymm roles),
   shot log, series totals S1-S6, match total + 10.x/9.x/8.x distribution bars.
3. Left panel: labeled nav buttons, discipline card, theme colors.
4. Phase stepper above target (3P-aware).
Constraint: do NOT restructure CenterPane internals (chart/shot pipeline is
fragile); style around them. Report design polish also parked (user note).
