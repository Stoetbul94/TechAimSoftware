# SETA — calculations, integrity checks, and what is still to do

Working note, 2026-11-09. Read from the SETA source on
`feature/seta-athlete-mode` (`a5b1bfe`), which carries the Tech Aim 1.0.0 core
unchanged. Every item names the file it comes from.

---

## Part 1 — Calculations in the software

### 1.1 From the target to millimetres

| Step | What happens | Where |
|---|---|---|
| Read | The laptop reads two 16-bit Modbus registers per shot: X and Y | `tachuswidget.cpp` ~L600 |
| Sign | Values ≥ 255 are decoded as **16-bit two's complement** (negative = left/below centre) | `TachusWidget::getRealValue()` |
| Units | Divided by **10** when `is_single_decimal=1` (target reports tenths of mm), else by 100 | same, `decimalDevider` |
| Distance | On a 10 m setup the coordinate is divided by `match distance / game range` (reduced-distance ranges) | same, `ratio` |

### 1.2 Ring score (the core formula)

One formula for every discipline, measured from the **shot centre** with the
**projectile radius added** — so a shot whose edge touches a ring line gets the
higher ring (the ISSF/DSB "outward gauge" principle):

```
d     = √(x² + y²)                         distance of the shot centre, mm
score = 9 + (R10 + S + r − d) / S
```

| Discipline | R10 = 10-ring radius | S = ring spacing | r = projectile radius | DSB face |
|---|---|---|---|---|
| 10 m Air Rifle | 0.25 mm | 2.5 mm | 2.25 mm (4.5 mm) | Scheibe Nr. 1 |
| 10 m Air Pistol | 5.75 mm | 8 mm | 2.25 mm (4.5 mm) | Scheibe Nr. 7 |
| 50 m Rifle | 5.2 mm | 8 mm | 2.8 mm (5.6 mm) | Scheibe Nr. 3 |
| 50 m Pistol | 25 mm | 25 mm | 2.8 mm (5.6 mm) | Scheibe Nr. 4 |

Source: `CenterPane.qml::calculateShootingSocre()`; projectile diameter from
`AppSettings::projectileDiameterMm()` (4.5 mm at 10 m, 5.6 mm at 50 m, unless
`bullet_size` is set in `config.ini`).

All four sets of numbers match the DSB target table (SpO 0.20, S-0.23): ring
10 diameters 0.5 / 11.5 / 10.4 / 50 mm and spacings 2.5 / 8 / 8 / 25 mm. That
also answers the open CLAUDE.md question about **50 m `radOf10Ring = 5.2`**: it
is half of the DSB Nr. 3 ten-ring diameter of 10.4 mm.

### 1.3 Limits and rounding

| Rule | Where |
|---|---|
| `+ 0.00001` added before cutting, so floating-point error can never drop 9.7 to 9.6 | `calculateShootingSocre` |
| Below 1.0 → **0** (a miss) | same |
| 11.0 or more → **10.9** (the maximum decimal score) | same |
| Decimal score is **cut off** to one decimal, never rounded up (9.79 → 9.7) | `TachusWidget::getFormatedSCore()` |
| **Whole-ring** programmes: `floor(decimal)` (9.7 → 9) | `ShootingPage.qml` ~L1080, and per profile `scoringMode` |

Which programme is decimal or whole-ring comes from the DSB/ISSF profile
(`CompetitionCatalogue.qml`), e.g. DSB 1.10 and 1.80 decimal; 1.20, 1.40, 1.60,
2.10, 2.20 whole ring.

### 1.4 Teiler (SETA shows it; Tech Aim hides it)

```
Teiler (one shot) = √(x² + y²) × 100         distance from centre in 1/100 mm
```

Source: `TachusWidget::getTeilerForShoot()`.

> ⚠ **Check before relying on the series Teiler.** `TachusWidget::getTeiler(series)`
> adds the Teilers of the shots **in that series** and then divides by the
> number of shots **in the whole match**, not in the series. The match-wide
> value is a correct average. A per-series value is too small whenever the
> match has more shots than the series. This is shown on the match report
> (`MatchReportInfo.qml`). It also needs Harald to confirm which Teiler DSB
> users expect: an average, or the **best (smallest) Teiler**.
> **Not changed. Needs the rule and a decision first.**

### 1.5 Coach analytics (reports, not scoring)

`src/analytics/CoachAnalyticsEngine.cpp`, a frozen engine. Standard statistics only:

- mean, median, standard deviation, min/max/range, percentiles
- MPI (mean point of impact), horizontal/vertical bias and standard deviation
- group radius, group diameter (largest distance between any two shots)
- trend = least-squares slope of score against shot number; correlation
- rolling 5-shot average and SD (used for the "deterioration" pattern)
- timing between shots (reset time, decision time)

None of these feed back into the score.

---

## Part 2 — Checksums and integrity checks

| What | Method | Protects against | Where |
|---|---|---|---|
| **Target cable traffic** | **CRC-16 (Modbus RTU)** on every message, from libmodbus | corrupted bytes on the USB/serial line | `ModReader/3rdparty/libmodbus/modbus-rtu.c` |
| **Shot counter check** | The target's shot counter must step by exactly the expected amount; a reconnect must prove the counter again before shots are accepted | duplicated, skipped or phantom shots | `src/target/AcquisitionDecision.h` |
| **Invalid-coordinate check** | Sentinel values (e.g. the old `-1` that once scored 10.8) are rejected before scoring | a fake 10.8 from a bad read | `CenterPane.qml` ~L265, `AcquisitionDecision.h` |
| **Session journal hash chain** | Every line of `*.jsonl` stores **SHA-256(previous hash + line)**, kept as the first 128 bits (32 hex). The first line starts from 32 zeros | any edit, deletion, reordering or truncation of the saved shots | `src/reliability/journal/HashChain.cpp` |
| **Journal validation on load** | Checks sequence numbers, that each line links to the previous hash, and recalculates every hash | recovering from a damaged or altered file | `src/reliability/journal/JournalValidator.cpp` |
| **Release files** | **SHA-256** of the `.exe` and the delivery `.zip`, written beside the package | a swapped or damaged installer | `tools/release/build-*-package.ps1` |
| **Support bundle** | Records the running exe's **SHA-256** and compares it with the release manifest | knowing exactly which build a log came from | `tools/release/Make-SupportBundle.ps1` |

**Not a checksum:** the old licence check (`TachusWidget::isValidLicence()`) is
only a date test. It is switched off in the login screen, and its condition can
never be true as written (`dayOfYear() > 2024`). Re-enabling it needs a proper
design (see CLAUDE.md, deferred work).

**`.tch` match files** have **no** checksum. The hash-chained journal is the
protected record.

---

## Part 3 — What is still to do (SETA)

### A. Athlete version (`feature/seta-athlete-mode`)

| # | Item | State |
|---|---|---|
| 1 | Athlete home, operator PIN, Display (theme + language) | **done** (`a5b1bfe`), not pushed |
| 2 | Your visual approval of UI-DEC-016 / 017 after clicking through it | **waiting on you** |
| 3 | Match screen in athlete mode: hide Settings / Training Lab / coach; big score, shots left, clock | not started |
| 4 | Simple end-of-match screen: total, Print, PDF, New match | not started |
| 5 | German text for the new screens (`techaim_de_DE.ts`) | not started |
| 6 | Light theme for the match screen (needs those screens moved to tokens first) | not started |
| 7 | New SETA installer (EVAL5) with athlete mode | not started |

### B. SETA product (from before the athlete work)

| Item | State |
|---|---|
| **EVAL4 physical checklist on a SETA target**: 1.20 clocks, 1.60 change at shot 40, USB disconnect, support bundle | **never done**. DSB has not been fired on a real target |
| DSB 1.40 / 1.60 crash-resume | known gap (shared 50 m 3P engine) |
| German catalogue: 239 strings untranslated, then native review | open |
| SETA white/knockout and single-ink logo files | requested from SETA |
| Series Teiler (1.4 above): rule + fix | **new finding**, needs Harald |
| **EVAL4 cannot commit a DSB programme.** `6409b4a` deleted `ShootingPage.applyCompetitionProfile/applyCompetitionDisplay`; `28a36f0` did not restore them. `main.qml` still calls them, so `applySetaProgramme()` aborts with a TypeError on every DSB pick. Recovery labels are hit by the same missing function | **fixed on `feature/seta-athlete-mode`** (restored verbatim from `6409b4a^`, + 3 reliability checks), not committed. **EVAL4 installer still has the bug: do not hand it out for DSB** |
| Athlete PIN unlock did nothing (`onOperatorUnlocked: operatorUnlocked = true` hit AthleteHome's same-named signal) | **fixed** (`rootItem.` qualified, + 2 checks), not committed |
| **Carry-forward audit vs Tech Aim 1.0.0 (2026-11-09).** ModReader, TachusWidget, `src/target`, `src/finals*`, `src/analytics` and the journal/hash chain are byte-identical; CenterPane scoring unchanged. Two 1.0.0 integrity fixes had NOT been carried: the `.tch` save guard (ACQ-SENTINEL-003, `appsettings.cpp` save + autosave) and the MPI "nan → —" guard (`MatchReportInfo.qml`); the acquisition- and logging-integrity suites existed but were not wired into the SETA runner | **ported verbatim**; reliability 2773/0 (+247 integrity checks). 1.0.0's full QML harness run against SETA: 409/435, every failure a deliberate SETA difference (Teiler, own theme, DSB catalogue, translated text). 1.0.0's finals harness: 369/379, failures are Tech Aim identity only |
| In-match "KEINE SCHEIBE" panel expands over the target face in **Demo** mode | existing, intended for Live; decide whether Demo should keep it compact |
| Athlete screens stay English when Deutsch is chosen | known: German strings for the new screens not yet written |
| 2.16 / 2.17 / 2.18 (falling/turning targets) | not planned; needs different hardware |
| Central results PC (SpO 0.4.3.2 Zentralrechner) | RMS work |

### C. From the Harald meeting
Record his answers here: who uses it, which events per club, touchscreen,
languages, print/PDF, Teiler type, club 3×15 / 50 m 3×10 profiles, logo files,
who runs the EVAL4 test and when.

**Teiler: question for Harald.** Decision 2026-11-09: fix the series Teiler
**after** Harald answers. Do not change the code before then.

> *"Für eine Serie: soll die Software den durchschnittlichen Teiler zeigen,
> den besten (kleinsten) Teiler, oder beides?"*
> ("For a series: should the software show the average Teiler, the best
> (smallest) Teiler, or both?")

| Harald's answer | Fix |
|---|---|
| Average | Divide by the shots **in the series**, not the whole match (`TachusWidget::getTeiler`) |
| Best | Return the smallest Teiler in the series |
| Both | Show both on the match report (`MatchReportInfo.qml`) |

Whichever answer: add a regression check, update this document, and record
the rule source (DSB SpO section, or Harald as the customer requirement).

Answer: ______________________
