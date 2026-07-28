# Tech Aim Manual — Validation Traceability

Document version 1.2 (P0.1) · Application baseline commit `21b40db` · Documentation source commit `cc69939`

Every documented procedure is traced to the screen/component that implements
it, the controller or action behind it, and the check that proves it.

**A procedure is never marked verified because it sounds plausible.**

## Status values

| Code | Status | Meaning |
|---|---|---|
| **VERIFIED FROM CODE AND TESTS** | VERIFIED FROM CODE AND TESTS | a passing harness test or direct source inspection covers it |
| **VERIFIED BY EXISTING MANUAL TEST** | VERIFIED BY EXISTING MANUAL TEST | a person performed it against a running build and recorded the result |
| **HUMAN VISUAL CHECK REQUIRED** | HUMAN VISUAL CHECK REQUIRED | written from source, **not** seen on screen |
| **WINDOWS RC1 DEPENDENT** | WINDOWS RC1 DEPENDENT | needs the installer / signing pipeline |
| **PHYSICAL TARGET DEPENDENT** | PHYSICAL TARGET DEPENDENT | needs a real electronic target |
| **GERMAN REVIEW REQUIRED** | GERMAN REVIEW REQUIRED | needs a native German technical reviewer |

**There are currently no VERIFIED BY EXISTING MANUAL TEST entries.** No interactive GUI session has been
performed against this build, so nothing is claimed as manually verified.

An installation, Windows-security, PDF-layout or Live-target procedure is
**never** marked complete merely because it is described.

## Traceability table

| # | Procedure | Screen / component | Controller / action | Check | Status |
|---|---|---|---|---|---|
| 1 | Launch shows Tech Aim identity | window title | `main.qml` title ← `PRODUCT.fullProductName` | identity assertions; startup log | **VERIFIED FROM CODE AND TESTS** (binding) / **HUMAN VISUAL CHECK REQUIRED** (caption) |
| 2 | Executable is `TechAim.exe` | — | `Seta.pro` `TARGET` | `release/` listing | **VERIFIED FROM CODE AND TESTS** |
| 3 | Windows file properties correct | — | `TechAim.rc` | `VersionInfo` read from binary | **VERIFIED FROM CODE AND TESTS** |
| 4 | About shows version/commit/publisher | `SettingsPage.qml` ABOUT / BUILD | `BUILDINFO`, `PRODUCT` | — | **HUMAN VISUAL CHECK REQUIRED** |
| 5 | Only one instance runs | startup dialog | `QLockFile` (Tech Aim + legacy) | — | **HUMAN VISUAL CHECK REQUIRED** |
| 6 | Legacy `Seta.exe` cannot run concurrently | — | legacy lock also held | by construction | **VERIFIED FROM CODE AND TESTS** (construction) / **HUMAN VISUAL CHECK REQUIRED** (against a real old build) |
| 7 | Select language | Settings LANGUAGE | `LanguageService.selectLanguage` | i18n assertions | **VERIFIED FROM CODE AND TESTS** (service) / **HUMAN VISUAL CHECK REQUIRED** (UI) |
| 8 | Language persists across restart | — | config.ini `[App_Settings] ui_language` | i18n persistence test | **VERIFIED FROM CODE AND TESTS** |
| 9 | Untranslated text falls back to English | — | Qt source-language fallback | i18n test | **VERIFIED FROM CODE AND TESTS** |
| 10 | Unknown language code falls back | — | `applyPersistedLanguage` | i18n test | **VERIFIED FROM CODE AND TESTS** |
| 11 | Language does not change brand/theme/exe/data | — | `LanguageService` has no identity access | i18n test | **VERIFIED FROM CODE AND TESTS** |
| 12 | German UI renders without clipping | all screens | — | — | **HUMAN VISUAL CHECK REQUIRED** + **GERMAN REVIEW REQUIRED** |
| 13 | Live rejects simulated input | shooting screen | operating-mode source gate | mode/source tests | **VERIFIED FROM CODE AND TESTS** |
| 14 | Demo rejects physical input | shooting screen | operating-mode source gate | mode/source tests | **VERIFIED FROM CODE AND TESTS** |
| 15 | Change mode via Restart Now / Later | Settings OPERATING MODE | `OperatingModeService` | — | **HUMAN VISUAL CHECK REQUIRED** |
| 16 | Restart relaunches the same executable | — | `applicationFilePath()` | source inspection | **VERIFIED FROM CODE AND TESTS** (inspection) / **HUMAN VISUAL CHECK REQUIRED** (round trip) |
| 17 | Target connects; indicator healthy | header | `MODREADER` | — | **HUMAN VISUAL CHECK REQUIRED** + **PHYSICAL TARGET DEPENDENT** |
| 18 | Shot acquisition and scoring | centre pane | `calculateShootingSocre()` | — | **HUMAN VISUAL CHECK REQUIRED** + **PHYSICAL TARGET DEPENDENT** |
| 19 | Discipline selection gates Training Lab | Training Lab catalogue | discipline gating | PT availability rules | **VERIFIED FROM CODE AND TESTS** (gating) / **HUMAN VISUAL CHECK REQUIRED** (catalogue UI) |
| 20 | Sighters excluded from counted metrics | all Training | `computeBlockMetrics` on counted only | training harness | **VERIFIED FROM CODE AND TESTS** |
| 21 | Competition overlays hidden in Training | `CenterPane.qml` | `isTrainingModeAny` gate | source + build | **VERIFIED FROM CODE AND TESTS** (gate) / **HUMAN VISUAL CHECK REQUIRED** (visual) |
| 22 | Technical Blocks: START BLOCK → block → review | `TrainingRightPanel` / `TrainingHud` | `TRAINING` | training harness | **VERIFIED FROM CODE AND TESTS** (logic) / **HUMAN VISUAL CHECK REQUIRED** (flow) |
| 23 | Shot 0 of N counts only counted shots | right panels | controllers | training harness | **VERIFIED FROM CODE AND TESTS** |
| 24 | Block metrics (avg, MPI, group, spread) | Block Review | `computeBlockMetrics` | training harness | **VERIFIED FROM CODE AND TESTS** |
| 25 | Cadence = interval between consecutive shots | Block Review / PT | `computeBlockMetrics` timing | cadence tests | **VERIFIED FROM CODE AND TESTS** |
| 26 | Visibility modes hide score during block | shooting screen | `TRAINING.showImpacts` | — | **HUMAN VISUAL CHECK REQUIRED** |
| 27 | Call & Diagnose: actual stays hidden until confirm | `CallDiagnoseHud` | `CALLDIAG` two-phase events | C&D harness | **VERIFIED FROM CODE AND TESTS** |
| 28 | One unresolved shot at a time | `CallDiagnoseHud` | `CALLDIAG` invariant | C&D harness | **VERIFIED FROM CODE AND TESTS** |
| 29 | Call error / bias / typical accuracy | C&D summary | `CallDiagnoseAnalytics` | C&D harness | **VERIFIED FROM CODE AND TESTS** |
| 30 | Group Pattern descriptions + confidence | GROUP PATTERN INSIGHTS | `GroupPatternAnalyzer` | pattern tests | **VERIFIED FROM CODE AND TESTS** |
| 31 | Pattern needs sufficient sample | GROUP PATTERN INSIGHTS | `analyzeGroup` guard | pattern tests | **VERIFIED FROM CODE AND TESTS** |
| 32 | PT: shots during POSITION SETUP ignored | PT workflow | `POSTRANS` phase 1 | PT harness | **VERIFIED FROM CODE AND TESTS** |
| 33 | PT: POSITION READY → sighters → verification | `PositionTransitionRightPanel` | `POSTRANS` state machine | PT harness | **VERIFIED FROM CODE AND TESTS** (logic) / **HUMAN VISUAL CHECK REQUIRED** (flow) |
| 34 | PT timers each measure what is documented | Position Review | `POSTRANS.positionReview` | PT harness | **VERIFIED FROM CODE AND TESTS** |
| 35 | Ready→first shot includes the sighter phase | Position Review | `readyToFirstShotMs` | PT harness | **VERIFIED FROM CODE AND TESTS** |
| 36 | Rhythm Steady/Variable/Inconsistent thresholds | PT cards | `ptRhythmLabel` | rhythm classifier tests | **VERIFIED FROM CODE AND TESTS** |
| 37 | No rhythm label below 3 shots / missing timing | PT cards | `ptRhythmLabel` guard | rhythm tests | **VERIFIED FROM CODE AND TESTS** |
| 38 | Positions kept separate | PT summary | per-position records | PT harness | **VERIFIED FROM CODE AND TESTS** |
| 39 | Session highlights ranked correctly | SESSION HIGHLIGHTS | `sessionRankings` | PT harness | **VERIFIED FROM CODE AND TESTS** |
| 40 | PT summary layout | POSITION TRANSITION COMPLETE | `PositionTransitionHud` | — | **HUMAN VISUAL CHECK REQUIRED** |
| 41 | Export PDF produces a branded report | report views | `CUSTOMPRINT` | — | **HUMAN VISUAL CHECK REQUIRED** |
| 42 | PDF metadata is Tech Aim | — | `customprint.cpp` setTitle/setCreator | source inspection | **VERIFIED FROM CODE AND TESTS** |
| 43 | Report software label is "Tech Aim 0.9.0" | report footers | `PRODUCT.softwareVersionLabel` | identity test | **VERIFIED FROM CODE AND TESTS** |
| 44 | Training disclaimer present; absent on competition | report views | report views | — | **HUMAN VISUAL CHECK REQUIRED** |
| 45 | German PDF: umlauts, wrapping, no overflow | exported PDFs | — | — | **HUMAN VISUAL CHECK REQUIRED** + **GERMAN REVIEW REQUIRED** |
| 46 | Clean Home closes durably and resets UI | summary → Home | `closeCleanly` + UI reset | clean-Home test | **VERIFIED FROM CODE AND TESTS** |
| 47 | No stale counters after Home | — | store-active gating | clean-Home test | **VERIFIED FROM CODE AND TESTS** |
| 48 | New session gets a new identity | NEW SESSION | controllers | clean-Home test | **VERIFIED FROM CODE AND TESTS** |
| 49 | Save and Close / Keep for Recovery | close prompt | `main.qml` | — | **HUMAN VISUAL CHECK REQUIRED** |
| 50 | Interrupted session offered for recovery | Recovery dialog | `RecoveryCoordinator` | recovery tests | **VERIFIED FROM CODE AND TESTS** (detection) / **HUMAN VISUAL CHECK REQUIRED** (dialog) |
| 51 | Completed session never offered | — | archive-on-complete | recovery tests | **VERIFIED FROM CODE AND TESTS** |
| 52 | Resume restores phase and shot count | Recovery dialog | `loadRecoveredState` | recovery tests | **VERIFIED FROM CODE AND TESTS** |
| 53 | Damaged record reported, not silently loaded | — | `JournalValidator` hash chain | reliability tests | **VERIFIED FROM CODE AND TESTS** |
| 54 | Incident categories and scopes | `IncidentWindow` | `INCIDENTS` | incident tests | **VERIFIED FROM CODE AND TESTS** (model) / **HUMAN VISUAL CHECK REQUIRED** (dialog) |
| 55 | Unresolved incident blocks official shots | shooting screen | incident gate | incident tests | **VERIFIED FROM CODE AND TESTS** |
| 56 | Settings controls behave as documented | `SettingsPage` | `APPSETTINGS` | — | **HUMAN VISUAL CHECK REQUIRED** |
| 57 | Data locations as documented | — | `StoragePaths` | storage root logged at startup | **VERIFIED FROM CODE AND TESTS** (root) / **WINDOWS RC1 DEPENDENT** (final paths) |
| 58 | Installation / update / uninstall | — | — | — | **WINDOWS RC1 DEPENDENT** |
| 59 | Windows security warnings | — | — | — | **WINDOWS RC1 DEPENDENT** |
| 60 | Screenshots match current identity | — | — | — | **HUMAN VISUAL CHECK REQUIRED** (none captured) |

| 61 | End-user agreement artwork | login screen | `images/loginPage/End User Agreement SETA-*.png` | shows a SETA-era agreement naming another entity — **must not be reproduced or approved in any manual** | **LEGAL REPLACEMENT REQUIRED BEFORE EXTERNAL BETA** |
| 62 | Windows application icon | Explorer / taskbar | none embedded | no approved `.ico` exists; none may be invented | **WINDOWS RC1 DEPENDENT** + BRAND APPROVAL REQUIRED |
| 63 | Live target acceptance | whole system | — | no physical target exercised | **PHYSICAL TARGET DEPENDENT** — the system is **not** Live-hardware certified |
| 63b | Instructional diagrams (DG-01..DG-11) | operator manual | `diagrams/make_diagrams.py` | source committed; regeneration byte-identical; all 11 confirmed embedded in the PDF | **VERIFIED FROM CODE AND TESTS** (generated + embedded) / **HUMAN VISUAL CHECK REQUIRED** (legibility in the final PDF) |
| 64 | Manual PDF layout | generated PDFs | `build-manuals.ps1` | HTML generates; PDF needs a LaTeX engine | **HUMAN VISUAL CHECK REQUIRED** + **WINDOWS RC1 DEPENDENT** |

## Summary

| Status | Count |
|---|---|
| VERIFIED FROM CODE AND TESTS (fully) | 30 |
| Partly verified from code and tests, remainder human visual check required | 8 |
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
