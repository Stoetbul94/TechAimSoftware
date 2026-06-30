# ISSF Rules Reference — TechAim Electronic Target Software

**Source:** ISSF Rule Book 2026 Edition (effective 1 January 2026) + Wikipedia/ISSF official event pages  
**Last verified:** 2026-07-01  
**Purpose:** Reference for UI event display, metadata, and future scoring/timing implementation

---

## EVENTS IMPLEMENTED IN CURRENT BACKEND

The TechAim backend (`TachusWidget` + `AppSettings`) currently supports two axes:

| Property    | Values               | Meaning                        |
|-------------|----------------------|-------------------------------|
| `gameMode`  | 0 = Pistol, 1 = Rifle | Weapon class                  |
| `gameRange` | 10 or 50             | Distance (metres)              |

This gives four discipline combinations: **10m Air Pistol**, **10m Air Rifle**, **50m Pistol**, **50m Rifle Prone**.

> ⚠️ **3-Position (50m Rifle 3×40) is NOT implemented** — would require position-switching
> logic in the Modbus protocol layer. Flagged as a pending decision (see CLAUDE.md).
>
> ⚠️ **25m events (25m Pistol, 25m Rapid Fire Pistol) are NOT implemented.**
>
> ⚠️ **Finals format (elimination scoring, countdown timer) is NOT implemented.**

---

## ISSF QUALIFICATION EVENT SPECIFICATIONS

### 10m Air Pistol (AP)
| Parameter             | Value                                          |
|-----------------------|------------------------------------------------|
| Qualification shots   | 60                                             |
| Sighting shots        | Unlimited, 15 min prep                         |
| Time limit            | 75 min (90 min if no electronic targets)       |
| Scoring               | Integer (max 10 per shot, max qual = 600 pts)  |
| Target diameter       | 170 × 170 mm                                   |
| 10-ring diameter      | 11.5 mm                                        |
| Bullet calibre        | 4.5 mm (.177)                                  |
| Final format (2026)   | 8 athletes, 24 shots, 150s per 5-shot series   |
| Notes                 | Decimal scoring adopted in some leagues/levels |

**Standard shot packs (as implemented in code):**

| gameEvent | Shots | Code match time | ISSF proportional time |
|-----------|-------|-----------------|------------------------|
| 0         | 10    | 10 min          | ~12.5 min              |
| 1         | 20    | 20 min          | ~25 min                |
| 2         | 30    | 30 min          | ~37.5 min              |
| 3         | 40    | 40 min          | ~50 min                |
| 4         | 60    | 50 min ⚠️       | 75 min (ISSF standard) |
| 5         | Free  | no limit        | no limit               |

> ⚠️ The code's 60-shot timer (50 min) differs from ISSF standard (75 min). Needs decision: adjust to ISSF standard or keep as-is.

---

### 10m Air Rifle (AR)
| Parameter             | Value                                          |
|-----------------------|------------------------------------------------|
| Qualification shots   | 60 (was 40 for women pre-2018, now equal)      |
| Sighting shots        | Unlimited, 15 min prep                         |
| Time limit            | 75 min                                         |
| Scoring               | Decimal (max 10.9 per shot, max qual = 654.0)  |
| Target total diameter | 45.5 mm                                        |
| 10-ring diameter      | 0.5 mm                                         |
| Bullet calibre        | 4.5 mm (.177)                                  |
| Final format (2026)   | 8 athletes, 24 shots, 250s per 5-shot series   |
| Notes                 | Decimal scoring standard since 2013            |

**Shot pack timing same as Air Pistol table above.**

> ⚠️ The `radOf10Ring = 5.2` constant in `CenterPane.qml::calculateShootingSocre()` for
> 50m Rifle needs verification against physical calibration. See CLAUDE.md pending items.

---

### 50m Rifle Prone (RP)
| Parameter             | Value                                          |
|-----------------------|------------------------------------------------|
| Qualification shots   | 60                                             |
| Sighting shots        | Unlimited, 15 min prep                         |
| Time limit            | **50 min** (electronic targets) / 60 min paper |
| Scoring               | Decimal (max 10.9 per shot, max qual = 654.0)  |
| Target total diameter | 154.4 mm                                       |
| 10-ring diameter      | 10.4 mm                                        |
| Bullet calibre        | 5.6 mm (.22 LR)                                |
| Final format (2026)   | 8 athletes, 24 shots, 150s per 5-shot series   |

> ✅ The code's 60-shot time of 50 min **matches** ISSF standard for electronic targets.

**Shot pack timing:**

| gameEvent | Shots | Code match time | ISSF approx     |
|-----------|-------|-----------------|-----------------|
| 0         | 10    | 10 min          | ~8 min          |
| 1         | 20    | 20 min          | ~17 min         |
| 2         | 30    | 30 min          | ~25 min         |
| 3         | 40    | 40 min          | ~33 min         |
| 4         | 60    | 50 min          | 50 min ✅       |
| 5         | Free  | no limit        | no limit        |

---

### 50m Pistol / Free Pistol (FP)
| Parameter             | Value                                          |
|-----------------------|------------------------------------------------|
| Qualification shots   | 60                                             |
| Sighting shots        | Unlimited, 15 min prep                         |
| Time limit            | 120 min                                        |
| Scoring               | Integer (max 10 per shot, max = 600)           |
| Target diameter       | 500 mm (same as 50m Rifle target diameter)     |
| 10-ring diameter      | 50 mm                                          |
| Bullet calibre        | 5.6 mm (.22 LR)                                |
| Notes                 | Not currently an Olympic event (removed 2018)  |

> ⚠️ 50m Pistol time limit in code (50 min for 60 shots) differs significantly from ISSF
> standard (120 min). Decision needed on whether to show ISSF time or code time.

---

## EVENTS NOT IN BACKEND (future scope)

### 50m Rifle 3 Positions (R3P) — NOT IMPLEMENTED
| Parameter             | Value                                          |
|-----------------------|------------------------------------------------|
| Positions             | Kneeling → Prone → Standing (in order)        |
| Shots per position    | 40 (3×40 = 120 total)                          |
| Time limit            | 5h 45 min total                                |
| Scoring               | Decimal                                        |
| Target                | Same as 50m Prone (154.4 mm total diameter)    |
| 10-ring               | 10.4 mm                                        |
| Final (2026)          | Standing position only, 8 athletes, 45 shots   |
| Notes                 | **Requires position-mode switching in backend**|

### 25m Rapid Fire Pistol (RFP) — NOT IMPLEMENTED
| Parameter             | Value                                          |
|-----------------------|------------------------------------------------|
| Shots                 | 30 (precision stage) + 30 (rapid fire)         |
| Time per rapid series | 8, 6, 4 seconds per 5-shot string              |
| Scoring               | Integer (max 150+150 = 300)                    |
| Notes                 | **Requires timed-string hardware control**     |

### 25m Pistol Women — NOT IMPLEMENTED
| Shots                 | 30 (precision) + 30 (rapid fire)               |
| Notes                 | **Requires timed-string hardware control**     |

---

## NETWORK SHARE FEATURE

**Status:** Fully implemented in C++ backend, was hidden in old UI.

**How it works:**
1. `MODREADER.getNetworkPath()` reads a saved UNC/share path from `USER_DETAILS` config file
2. `MODREADER.setIsServerNetworkEnabled(bool)` toggles the feature on/off
3. `APPSETTINGS.setSetaSettingsFilePathFromQML(path)` sets the settings file location on the share
4. `MatchReport.qml::printImageInNetworkPath()` saves PDF match reports to the share
5. `TachusWidget::updateSetaShootSummaryData()` writes live score data to the share when enabled

**Use case:** Multi-lane range — each target PC saves results to a shared network folder on a
range management laptop, which aggregates scores across all lanes.

**UI requirement:** The network path needs to be configurable in the UI (previously was configured
via a settings dialog or direct file edit). The `networkSwitch` toggle and `netowrk_path_text`
hidden elements in LoginPage.qml need to be exposed in the redesign.

---

## PREP TIME STANDARDS

All events: **15 minutes unlimited sighting shots** before competition starts.

---

## SOURCES

- [ISSF 2026 Rulebook changes announcement](https://www.issf-sports.org/news/4878)
- [ISSF Rules & Regulations page](https://www.issf-sports.org/rules)
- [ISSF 10m Air Rifle (Wikipedia)](https://en.wikipedia.org/wiki/ISSF_10_meter_air_rifle)
- [ISSF 10m Air Pistol (Wikipedia)](https://en.wikipedia.org/wiki/ISSF_10_meter_air_pistol)
- [ISSF 50m Rifle Prone (Wikipedia)](https://en.wikipedia.org/wiki/ISSF_50_meter_rifle_prone)
- [ISSF 50m Rifle Three Positions (Wikipedia)](https://en.wikipedia.org/wiki/ISSF_50_meter_rifle_three_positions)
