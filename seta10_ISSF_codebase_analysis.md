# Seta10 / Tachus Codebase Analysis — ISSF Discipline Optimization

**Source:** `Seta10_Latest_Codes.7z`, repo `raosrinu2004/Merging_app_modReader`, working-tree state as of file timestamps through May 2023.
**Scope of this pass:** architecture mapping + full trace of the live shot-to-score pipeline, with findings checked against the official ISSF 2022/2023 Technical Rules where the question was a verifiable physical/regulatory fact rather than a code-style judgment call.

---

## 1. Architecture overview

This is one Qt application, not two. `Seta.pro` does `include(ModReader/qModMaster.pro)`, so the QModMaster-derived Modbus stack you originally uploaded (`ui_mainwindow.h` etc.) compiles directly into the same binary as the QML scoring frontend — there's no separate process or IPC boundary between them.

**Runtime wiring** (`main.cpp`):

| C++ object | Exposed to QML as | Role |
|---|---|---|
| `ModbusAdapter` + `MainWindow` | — (not exposed directly) | Modbus RTU/TCP transport to target hardware |
| `TachusWidget` | `MODREADER` | **The bridge.** All shot data, scores, settings getters/setters the QML calls through |
| `ReceiverTachus` | — | Wraps `TachusWidget`, listens for incoming shot events |
| `AppSettings` | `APPSETTINGS` | Config, backed by `config.ini` |
| `CustomPrint` | `CUSTOMPRINT` | Match report / PDF generation |

**Data flow for one shot:**

```
Target hardware (acoustic sensor array — confirmed by comment in
tachuswidget.cpp:75-77 referencing microphone Modbus register reads)
        │ Modbus RTU/TCP
        ▼
ModbusAdapter / MainWindow  (ModReader/src/)
        │
        ▼
TachusWidget::uxShoot(xCor, yCor)   [ModReader/forms/tachuswidget.cpp:355]
  → stores raw + range-ratio-adjusted coordinates in m_xCordList/m_yCordList
  → routes into sighter-mode or game-mode coordinate lists separately
        │
        ▼
CenterPane.qml: calculateShootingSocre(xPoint, yPoint, ...)  [line 1601]
  → THIS is where ring geometry → decimal score actually happens
        │
        ▼
MODREADER.setScore(calculatedSccore)   [CenterPane.qml:1894]
  → TachusWidget::setScore()  [tachuswidget.cpp:1046] stores it
```

**Key implication:** the C++ layer (`TachusWidget`) is data plumbing and Modbus I/O. **The actual ISSF-relevant scoring math lives in QML**, specifically in one function in `CenterPane.qml`. That's where almost all of the discipline-optimization work will happen.

---

## 2. The scoring engine: `CenterPane.qml::calculateShootingSocre()` (lines 1601–1895)

### 2.1 Structure

The function computes a radius from target center (`mapedRadius`), then branches on two QML properties — `gameRange` (10 or 50, meters) and `gameMode` (boolean: pistol vs rifle) — to pick a linear-interpolation formula:

```
score = 9 + (totalR - mapedRadius) / ringSpacing
totalR = ringSpacing + radiusOf10Ring + bulletRadius
```

This is structurally the correct approach — ISSF decimal scoring is defined as linear interpolation of the shot's distance from center between ring boundaries, with the score determined by the edge of the bullet hole nearest center (hence adding bullet/pellet radius to the boundary distance). The question is whether the constants are right and whether all four discipline combinations are even reachable.

### 2.2 Dead code you should delete first

**Lines 1626–1710 are unreachable.** `var old = false` at line 1626 gates an entire duplicate set of pistol/rifle/10m/50m formulas that never executes. The live formulas are in the `else` branch starting line 1711 ("config bullet radius"). Before doing any amendment work here, I'd delete 1626–1710 outright — keeping dead branches with the same variable names as the live ones is exactly how a future edit accidentally changes the wrong copy.

### 2.3 Verified against official ISSF target specifications

I checked the four live formula branches against the ISSF 2022/2023 Technical Rules (target dimension tables) rather than relying on memory, since these are exactly the kind of regulated figures worth confirming directly.

| Discipline | Code constants (line) | Official ISSF figures | Match? |
|---|---|---|---|
| **10m Air Rifle** | `r2rDis=2.5, radOf10Ring=0.25` (1750–1751) | Ring spacing 2.5mm radius; 10-ring (white dot) = 0.5mm diameter → 0.25mm radius | ✅ Exact |
| **10m Air Pistol** | `r2rDis=8, radOf10Ring=5.75` (1723–1724) | Ring spacing 8mm radius; 10-ring = 11.5mm diameter → 5.75mm radius | ✅ Exact |
| **50m Pistol** | `r2rDis=25, radOf10Ring=25` (1778–1779) | 10-ring = 50mm diameter → 25mm radius (confirmed) | ✅ 10-ring matches; ring spacing plausible but I could not independently confirm the 25mm figure from the rule text I retrieved — verify against your rulebook copy directly, or empirically against a calibration plate |
| **50m Rifle** | `r2rDis=8, radOf10Ring=5.2` (1804–1805) | Could not independently confirm from retrieved rule text (the figures I found describe the 600mm overall black aiming area, not ring-by-ring spacing) | ⚠️ Needs verification |

Bullet/pellet radius compensation (`APPSETTINGS.bullet_diameter()/2`) is correctly applied in all four branches, and `bullet_diameter()` is properly read from `config.ini` (`appsettings.cpp:39`, default 5.6mm) rather than hardcoded — good practice, since it means caliber compensation is at least architecturally correct even before the constants above are double-checked.

**Bottom line on 2.3:** the 10m formulas (both disciplines) are solid. The 50m formulas are structurally consistent with the same methodology and partially confirmed, but I'd want them validated against a physical calibration target before trusting them in competition — not because the code looks wrong, but because I can only verify what's in the rule text I retrieved, and one of the two 50m branches I couldn't fully confirm that way.

### 2.4 The bug that actually matters most here

`CenterPane.qml:1609`:
```qml
var distance_factor = APPSETTINGS.getMatch_meter() /10 /*10m match*/
```

`appsettings.cpp:828-831`:
```cpp
double AppSettings::getMatch_meter() const
{
    return 10; //m_match_meter;
}
```

**`getMatch_meter()` is hardcoded to always return `10`, ignoring whatever match distance is actually configured.** The real stored value (`m_match_meter`) is commented out. There's a second function, `getMatch_meter_new()` (`appsettings.cpp:863`), that correctly returns the real configured value — but I grepped the entire codebase and **`getMatch_meter_new()` is never called from anywhere.** It's dead code sitting next to the function that's actually live.

**Practical effect:** `distance_factor` is always exactly `10/10 = 1.0`, for every match, regardless of discipline. The entire `modified_xPoint = xPoint/distance_factor` scaling step is currently a no-op — coordinates pass through unscaled no matter what.

For 10m matches this happens to be harmless, since no scaling is needed there anyway. For 50m matches, whether this is actually causing wrong scores depends on a fact I don't have: whether the raw `xCor`/`yCor` your hardware reports over Modbus are already true real-world millimeters at the actual target distance, or whether they're in some sensor-native unit that genuinely needs the distance compensation to land on real mm. I'd treat this as the single highest-priority thing to check empirically — fire a shot at a known, measured position on a 50m setup and see if the displayed score matches hand-calculation. This is also exactly the validation ISSF itself requires for EST approval (their stated accuracy bar is "scoring shots to an accuracy of at least one-half of one decimal scoring ring"), so it's worth having that test rig regardless.

**Fix is small either way:** swap line 1609 to call `getMatch_meter_new()` instead, *and* fix `getMatch_meter()` itself to return `m_match_meter` rather than a hardcoded constant — then audit anywhere else in the codebase that might be relying on the current (broken) hardcoded-10 behavior, since "fixing" it could change 50m behavior that's currently silently relying on the bug.

---

## 3. Discipline coverage gaps

I grepped every `gameRange` comparison across the entire codebase (QML and C++) to map exactly which disciplines are reachable at all.

**Confirmed: only `gameRange == 10` and `gameRange == 50` exist, anywhere.** There is no `25` branch in any file. This means:

- **25m Pistol disciplines are entirely unimplemented** — Rapid Fire Pistol, 25m Pistol (Women), Standard Pistol, Sport Pistol. These are mainstream Olympic events, not edge cases, so if "ISSF disciplines" in your scope includes pistol events beyond 10m/50m, this is the largest single gap, not a tweak.
- If you do add 25m support, note that ISSF's official rule text defines **25m Pistol events as using 5-shot series**, not the 10-shot series used everywhere else. The codebase currently hardcodes 10 shots/series in two places: `SERIES_SHOOT_COUNT 10` (`tachuswidget.cpp:20`) and `m_shotPerSeries = 10` default (`tachuswidget.h:412`). There is a `setShotPerSeries()` setter, so the *mechanism* for a configurable series length already exists — it just isn't currently driven by discipline selection.

**ISSF Finals format is not implemented anywhere.** I searched for countdown/timer/elimination/start-from-zero logic across all QML and found none — the matches that came up for "final" were unrelated variable names (`finalTime`, `finalShootNotificationRect`). Current ISSF finals require: starting from zero (qualification score doesn't carry over), a specific shot sequence (e.g., for 10m air rifle: two 5-shot series at 250 seconds each, then 14 single shots at 50 seconds each, with elimination after the 10th shot and every 2 shots thereafter), and per-shot countdown enforcement. None of that machinery exists in this codebase yet — `m_shot_interval` and `m_series_start_at`/`m_series_end_at` in `tachuswidget.h` look like the closest existing scaffolding, but they're generic interval/range settings, not a finals state machine.

**What's already correctly in place** (worth knowing so you don't rebuild it): sighter shots are properly kept separate from scored shots via parallel coordinate/score lists (`m_xCordList_sighterMode` vs `m_xCordList_gameMode`), the app correctly defaults to sighter mode on startup, and there's existing networked multi-lane architecture (`QTcpServer`, lane naming, server path settings) for aggregating results across firing points — useful infrastructure if a full ISSF-format match needs central results aggregation across many lanes.

---

## 4. Code health findings relevant to making changes safely

### 4.1 Modbus calls are not thread-safe

I traced the full call chain: `TachusWidget` → `MainWindow::modbusWriteSingleRegister/modbusReadRegistry` (`mainwindow.cpp:474-492`) → `ModbusAdapter::directModbusWriteSingleRegister/directModbusReadRegistry` (`modbusadapter.cpp:478-486`) → raw `modbus_write_register()`/`modbus_read_registers()` calls against a single shared `modbus_t*` context. **No `QMutex` or any locking exists anywhere in this chain.**

Meanwhile, three independent execution contexts call into this same unguarded path: the main GUI-thread polling timer (100ms `QTimer` in the `TachusWidget` constructor, `tachuswidget.cpp:36-37`), `MotorThread::run()` (`tachuswidget.h:40-72`, busy-waits in a loop with no timeout calling Modbus read/write), and `WorkerThread::run()` (`tachuswidget.h:121-129`). All three are genuine separate `QThread`s. libmodbus's context object is documented as not safe for concurrent access — interleaved calls can corrupt the RTU/TCP frame being sent or received.

This is the kind of bug that shows up as an intermittent, hard-to-reproduce bad shot read or a hang, and it's worse the more reliable you need the system to be (i.e., worse for an actual sanctioned competition than for casual practice). I'd prioritize this alongside the scoring fixes, not after them — a `QMutex` around the shared Modbus context, locked in `directModbusWriteSingleRegister`/`directModbusReadRegistry`, is a small, contained fix. The motor-status busy-wait loops (`while (!motorStarted)` with no timeout, `tachuswidget.h:56-70` and `:82-96`) are also worth a hard timeout — currently a non-responding motor controller would hang that thread forever.

### 4.2 A recurring "_new" / dead-duplicate pattern

This isn't isolated to `getMatch_meter`. The same shape shows up repeatedly: `m_match_distance_new` alongside an older distance field, `RightPanelORG.qml` / `RightPanel - Copy.qml` next to the live `RightPanel.qml`, `LoginPageOld.qml` next to `LoginPage.qml`, and the dead `if(old)` block in the scoring function itself. It looks like a habit of duplicating a file/function to try a fix rather than editing in place or using git branches properly (which ties into the git findings below — there's a real repo here, but it isn't being used as one).

Practical recommendation: before starting ISSF amendment work, I'd do a pass to identify and delete confirmed-dead duplicates (I can help trace which ones are genuinely unreferenced vs. which "_old"/"ORG" files might still be loaded somewhere). Working in a codebase where every function might have a silently-dead twin makes every future change riskier than it needs to be — exactly what happened with `getMatch_meter`.

### 4.3 Git history is not usable for tracing evolution

You asked me to look at this directly, so to be clear about what's actually there: the repository has **a single commit** ("Initial commit", 2019-01-01, 295 files). Working-tree file timestamps run through May 2023, and `git status` shows **140 modified files and dozens of new untracked files** relative to that one commit — meaning everything built since January 2019, including all the actively-developed files covered above (`tachuswidget.cpp`, `appsettings.cpp`, `CenterPane.qml`, `customprint.cpp`, etc.), was **never committed**. There's no commit-by-commit story to read; the `.git` folder is essentially a snapshot from before most of the real development happened, sitting alongside ~4.5 years of uncommitted work.

What I did instead: diffed the working tree against that single commit to identify which files have seen the most active development (this is how I prioritized what to read — `tachuswidget.cpp/h` alone account for +2,698 lines since 2019, by far the most of any file). That diff is reproducible any time with `git diff --stat HEAD` from the repo root if useful to you.

**Practical recommendation, separate from the ISSF work but relevant to doing it safely:** get this into real version control before making amendments. Right now there's no way to review a change, revert a mistake, or see who changed what when. A first commit of the current working tree (`git add -A && git commit -m "Snapshot before ISSF discipline work"`) costs nothing and gives you a baseline to diff against for everything we do next.

---

## 5. Suggested order of operations

Given everything above, here's how I'd sequence the actual amendment work, roughly in order of risk/value:

1. **Commit the current working tree** — establishes a real baseline before anything changes.
2. **Delete the dead `if(old)` block** in `calculateShootingSocre` (lines 1626–1710) — pure cleanup, zero behavior change, removes a trap.
3. **Fix `getMatch_meter()`** and wire the real value through, then empirically validate 50m scoring against a calibration target before trusting it.
4. **Add Modbus call locking** — contained, removes a real reliability risk.
5. **Decide scope on 25m Pistol and Finals format** — these are genuinely new features, not fixes, and will be the largest pieces of work. Worth scoping deliberately (which 25m events specifically, which finals formats) rather than discovering requirements mid-implementation.
6. **Verify 50m Rifle's `radOf10Ring=5.2` constant** against your own rulebook copy or a calibration plate, since I couldn't independently confirm it.

I'm set up to go deep on any of these next — happy to start with whichever matters most to you, or to read through `appsettings.cpp` and `customprint.cpp` in full if you want the settings layer and report-generation format audited with the same level of detail before deciding priorities.
