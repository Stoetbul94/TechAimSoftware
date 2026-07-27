# Tech Aim Manual — Validation Traceability

Document version 1.1 (P0-J refinement) · Application commit `84db7a2`

Every documented procedure is traced to the screen/component that implements
it, the controller or action behind it, and the check that proves it.

**A procedure is never marked verified because it sounds plausible.**

## Status values

| Code | Status | Meaning |
|---|---|---|
| **CT** | VERIFIED FROM CODE AND TESTS | a passing harness test or direct source inspection covers it |
| **MT** | VERIFIED BY EXISTING MANUAL TEST | a person performed it against a running build and recorded the result |
| **HV** | HUMAN VISUAL CHECK REQUIRED | written from source, **not** seen on screen |
| **RC1** | WINDOWS RC1 DEPENDENT | needs the installer / signing pipeline |
| **PT** | PHYSICAL TARGET DEPENDENT | needs a real electronic target |
| **DE** | GERMAN REVIEW REQUIRED | needs a native German technical reviewer |

**There are currently no MT entries.** No interactive GUI session has been
performed against this build, so nothing is claimed as manually verified.

An installation, Windows-security, PDF-layout or Live-target procedure is
**never** marked complete merely because it is described.

## Traceability table

| # | Procedure | Screen / component | Controller / action | Check | Status |
|---|---|---|---|---|---|
| 1 | Launch shows Tech Aim identity | window title | `main.qml` title ← `PRODUCT.fullProductName` | identity assertions; startup log | **CT** (binding) / **HV** (caption) |
| 2 | Executable is `TechAim.exe` | — | `Seta.pro` `TARGET` | `release/` listing | **CT** |
| 3 | Windows file properties correct | — | `TechAim.rc` | `VersionInfo` read from binary | **CT** |
| 4 | About shows version/commit/publisher | `SettingsPage.qml` ABOUT / BUILD | `BUILDINFO`, `PRODUCT` | — | **HV** |
| 5 | Only one instance runs | startup dialog | `QLockFile` (Tech Aim + legacy) | — | **HV** |
| 6 | Legacy `Seta.exe` cannot run concurrently | — | legacy lock also held | by construction | **CT** (construction) / **HV** (against a real old build) |
| 7 | Select language | Settings LANGUAGE | `LanguageService.selectLanguage` | i18n assertions | **CT** (service) / **HV** (UI) |
| 8 | Language persists across restart | — | config.ini `[App_Settings] ui_language` | i18n persistence test | **CT** |
| 9 | Untranslated text falls back to English | — | Qt source-language fallback | i18n test | **CT** |
| 10 | Unknown language code falls back | — | `applyPersistedLanguage` | i18n test | **CT** |
| 11 | Language does not change brand/theme/exe/data | — | `LanguageService` has no identity access | i18n test | **CT** |
| 12 | German UI renders without clipping | all screens | — | — | **HV** + **DE** |
| 13 | Live rejects simulated input | shooting screen | operating-mode source gate | mode/source tests | **CT** |
| 14 | Demo rejects physical input | shooting screen | operating-mode source gate | mode/source tests | **CT** |
| 15 | Change mode via Restart Now / Later | Settings OPERATING MODE | `OperatingModeService` | — | **HV** |
| 16 | Restart relaunches the same executable | — | `applicationFilePath()` | source inspection | **CT** (inspection) / **HV** (round trip) |
| 17 | Target connects; indicator healthy | header | `MODREADER` | — | **HV** + **PT** |
| 18 | Shot acquisition and scoring | centre pane | `calculateShootingSocre()` | — | **HV** + **PT** |
| 19 | Discipline selection gates Training Lab | Training Lab catalogue | discipline gating | PT availability rules | **CT** (gating) / **HV** (catalogue UI) |
| 20 | Sighters excluded from counted metrics | all Training | `computeBlockMetrics` on counted only | training harness | **CT** |
| 21 | Competition overlays hidden in Training | `CenterPane.qml` | `isTrainingModeAny` gate | source + build | **CT** (gate) / **HV** (visual) |
| 22 | Technical Blocks: START BLOCK → block → review | `TrainingRightPanel` / `TrainingHud` | `TRAINING` | training harness | **CT** (logic) / **HV** (flow) |
| 23 | Shot 0 of N counts only counted shots | right panels | controllers | training harness | **CT** |
| 24 | Block metrics (avg, MPI, group, spread) | Block Review | `computeBlockMetrics` | training harness | **CT** |
| 25 | Cadence = interval between consecutive shots | Block Review / PT | `computeBlockMetrics` timing | cadence tests | **CT** |
| 26 | Visibility modes hide score during block | shooting screen | `TRAINING.showImpacts` | — | **HV** |
| 27 | Call & Diagnose: actual stays hidden until confirm | `CallDiagnoseHud` | `CALLDIAG` two-phase events | C&D harness | **CT** |
| 28 | One unresolved shot at a time | `CallDiagnoseHud` | `CALLDIAG` invariant | C&D harness | **CT** |
| 29 | Call error / bias / typical accuracy | C&D summary | `CallDiagnoseAnalytics` | C&D harness | **CT** |
| 30 | Group Pattern descriptions + confidence | GROUP PATTERN INSIGHTS | `GroupPatternAnalyzer` | pattern tests | **CT** |
| 31 | Pattern needs sufficient sample | GROUP PATTERN INSIGHTS | `analyzeGroup` guard | pattern tests | **CT** |
| 32 | PT: shots during POSITION SETUP ignored | PT workflow | `POSTRANS` phase 1 | PT harness | **CT** |
| 33 | PT: POSITION READY → sighters → verification | `PositionTransitionRightPanel` | `POSTRANS` state machine | PT harness | **CT** (logic) / **HV** (flow) |
| 34 | PT timers each measure what is documented | Position Review | `POSTRANS.positionReview` | PT harness | **CT** |
| 35 | Ready→first shot includes the sighter phase | Position Review | `readyToFirstShotMs` | PT harness | **CT** |
| 36 | Rhythm Steady/Variable/Inconsistent thresholds | PT cards | `ptRhythmLabel` | rhythm classifier tests | **CT** |
| 37 | No rhythm label below 3 shots / missing timing | PT cards | `ptRhythmLabel` guard | rhythm tests | **CT** |
| 38 | Positions kept separate | PT summary | per-position records | PT harness | **CT** |
| 39 | Session highlights ranked correctly | SESSION HIGHLIGHTS | `sessionRankings` | PT harness | **CT** |
| 40 | PT summary layout | POSITION TRANSITION COMPLETE | `PositionTransitionHud` | — | **HV** |
| 41 | Export PDF produces a branded report | report views | `CUSTOMPRINT` | — | **HV** |
| 42 | PDF metadata is Tech Aim | — | `customprint.cpp` setTitle/setCreator | source inspection | **CT** |
| 43 | Report software label is "Tech Aim 0.9.0" | report footers | `PRODUCT.softwareVersionLabel` | identity test | **CT** |
| 44 | Training disclaimer present; absent on competition | report views | report views | — | **HV** |
| 45 | German PDF: umlauts, wrapping, no overflow | exported PDFs | — | — | **HV** + **DE** |
| 46 | Clean Home closes durably and resets UI | summary → Home | `closeCleanly` + UI reset | clean-Home test | **CT** |
| 47 | No stale counters after Home | — | store-active gating | clean-Home test | **CT** |
| 48 | New session gets a new identity | NEW SESSION | controllers | clean-Home test | **CT** |
| 49 | Save and Close / Keep for Recovery | close prompt | `main.qml` | — | **HV** |
| 50 | Interrupted session offered for recovery | Recovery dialog | `RecoveryCoordinator` | recovery tests | **CT** (detection) / **HV** (dialog) |
| 51 | Completed session never offered | — | archive-on-complete | recovery tests | **CT** |
| 52 | Resume restores phase and shot count | Recovery dialog | `loadRecoveredState` | recovery tests | **CT** |
| 53 | Damaged record reported, not silently loaded | — | `JournalValidator` hash chain | reliability tests | **CT** |
| 54 | Incident categories and scopes | `IncidentWindow` | `INCIDENTS` | incident tests | **CT** (model) / **HV** (dialog) |
| 55 | Unresolved incident blocks official shots | shooting screen | incident gate | incident tests | **CT** |
| 56 | Settings controls behave as documented | `SettingsPage` | `APPSETTINGS` | — | **HV** |
| 57 | Data locations as documented | — | `StoragePaths` | storage root logged at startup | **CT** (root) / **RC1** (final paths) |
| 58 | Installation / update / uninstall | — | — | — | **RC1** |
| 59 | Windows security warnings | — | — | — | **RC1** |
| 60 | Screenshots match current identity | — | — | — | **HV** (none captured) |

| 61 | End-user agreement artwork | login screen | `images/loginPage/End User Agreement SETA-*.png` | shows a SETA-era agreement naming another entity — **must not be reproduced or approved in any manual** | **LEGAL REPLACEMENT REQUIRED BEFORE EXTERNAL BETA** |
| 62 | Windows application icon | Explorer / taskbar | none embedded | no approved `.ico` exists; none may be invented | **RC1** + BRAND APPROVAL REQUIRED |
| 63 | Live target acceptance | whole system | — | no physical target exercised | **PT** — the system is **not** Live-hardware certified |
| 64 | Manual PDF layout | generated PDFs | `build-manuals.ps1` | HTML generates; PDF needs a LaTeX engine | **HV** + **RC1** |

## Summary

| Status | Count |
|---|---|
| VERIFIED FROM CODE AND TESTS (fully) | 30 |
| Partly CT, remainder HV | 8 |
| HUMAN VISUAL CHECK REQUIRED | 20 |
| WINDOWS RC1 DEPENDENT | 4 |
| PHYSICAL TARGET DEPENDENT | 2 |
| GERMAN REVIEW REQUIRED | 3 |
| **VERIFIED BY EXISTING MANUAL TEST** | **0** |

**The measured behaviour of the software — scoring inputs aside — is well
covered automatically. What is not covered is everything a person sees:**
screen flow, layout, dialogs, PDF rendering, and anything needing a real
target.

## Documentation freeze rule

Update these manuals whenever any of the following changes: a visible button
or screen name · a workflow · event structure · operating-mode behaviour ·
recovery behaviour · report content · a data path · the executable name ·
language · the installer · a security warning · the target connection
procedure.

**Release checklist item: DOCUMENTATION REVIEW COMPLETE.**
No Windows RC may be approved while known release behaviour contradicts these
manuals.
