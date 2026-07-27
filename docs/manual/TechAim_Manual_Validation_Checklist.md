# Tech Aim Manual — Validation Traceability

Document version 1.0 (P0-J) · Application commit `3741980`

Every documented procedure is traced to the screen/component that implements
it, the controller or action behind it, and the check that proves it.

**A procedure is never marked verified because it sounds plausible.**

## Status values

| Status | Meaning |
|---|---|
| **VA** | VERIFIED AUTOMATICALLY — a passing harness test covers it |
| **VM** | VERIFIED MANUALLY — a person performed it against a running build |
| **MVR** | MANUAL VALIDATION REQUIRED — written from source, **not** performed |
| **RC1** | WINDOWS RC1 DEPENDENT |
| **HW** | PHYSICAL HARDWARE DEPENDENT |
| **DE** | GERMAN REVIEW REQUIRED |

**There are currently no VM entries.** No interactive GUI session was
performed for P0-J.

## Traceability table

| # | Procedure | Screen / component | Controller / action | Check | Status |
|---|---|---|---|---|---|
| 1 | Launch shows Tech Aim identity | window title | `main.qml` title ← `PRODUCT.fullProductName` | identity assertions; startup log | **VA** (binding) / **MVR** (caption) |
| 2 | Executable is `TechAim.exe` | — | `Seta.pro` `TARGET` | `release/` listing | **VA** |
| 3 | Windows file properties correct | — | `TechAim.rc` | `VersionInfo` read from binary | **VA** |
| 4 | About shows version/commit/publisher | `SettingsPage.qml` ABOUT / BUILD | `BUILDINFO`, `PRODUCT` | — | **MVR** |
| 5 | Only one instance runs | startup dialog | `QLockFile` (Tech Aim + legacy) | — | **MVR** |
| 6 | Legacy `Seta.exe` cannot run concurrently | — | legacy lock also held | by construction | **VA** (construction) / **MVR** (against a real old build) |
| 7 | Select language | Settings LANGUAGE | `LanguageService.selectLanguage` | i18n assertions | **VA** (service) / **MVR** (UI) |
| 8 | Language persists across restart | — | config.ini `[App_Settings] ui_language` | i18n persistence test | **VA** |
| 9 | Untranslated text falls back to English | — | Qt source-language fallback | i18n test | **VA** |
| 10 | Unknown language code falls back | — | `applyPersistedLanguage` | i18n test | **VA** |
| 11 | Language does not change brand/theme/exe/data | — | `LanguageService` has no identity access | i18n test | **VA** |
| 12 | German UI renders without clipping | all screens | — | — | **MVR** + **DE** |
| 13 | Live rejects simulated input | shooting screen | operating-mode source gate | mode/source tests | **VA** |
| 14 | Demo rejects physical input | shooting screen | operating-mode source gate | mode/source tests | **VA** |
| 15 | Change mode via Restart Now / Later | Settings OPERATING MODE | `OperatingModeService` | — | **MVR** |
| 16 | Restart relaunches the same executable | — | `applicationFilePath()` | source inspection | **VA** (inspection) / **MVR** (round trip) |
| 17 | Target connects; indicator healthy | header | `MODREADER` | — | **MVR** + **HW** |
| 18 | Shot acquisition and scoring | centre pane | `calculateShootingSocre()` | — | **MVR** + **HW** |
| 19 | Discipline selection gates Training Lab | Training Lab catalogue | discipline gating | PT availability rules | **VA** (gating) / **MVR** (catalogue UI) |
| 20 | Sighters excluded from counted metrics | all Training | `computeBlockMetrics` on counted only | training harness | **VA** |
| 21 | Competition overlays hidden in Training | `CenterPane.qml` | `isTrainingModeAny` gate | source + build | **VA** (gate) / **MVR** (visual) |
| 22 | Technical Blocks: START BLOCK → block → review | `TrainingRightPanel` / `TrainingHud` | `TRAINING` | training harness | **VA** (logic) / **MVR** (flow) |
| 23 | Shot 0 of N counts only counted shots | right panels | controllers | training harness | **VA** |
| 24 | Block metrics (avg, MPI, group, spread) | Block Review | `computeBlockMetrics` | training harness | **VA** |
| 25 | Cadence = interval between consecutive shots | Block Review / PT | `computeBlockMetrics` timing | cadence tests | **VA** |
| 26 | Visibility modes hide score during block | shooting screen | `TRAINING.showImpacts` | — | **MVR** |
| 27 | Call & Diagnose: actual stays hidden until confirm | `CallDiagnoseHud` | `CALLDIAG` two-phase events | C&D harness | **VA** |
| 28 | One unresolved shot at a time | `CallDiagnoseHud` | `CALLDIAG` invariant | C&D harness | **VA** |
| 29 | Call error / bias / typical accuracy | C&D summary | `CallDiagnoseAnalytics` | C&D harness | **VA** |
| 30 | Group Pattern descriptions + confidence | GROUP PATTERN INSIGHTS | `GroupPatternAnalyzer` | pattern tests | **VA** |
| 31 | Pattern needs sufficient sample | GROUP PATTERN INSIGHTS | `analyzeGroup` guard | pattern tests | **VA** |
| 32 | PT: shots during POSITION SETUP ignored | PT workflow | `POSTRANS` phase 1 | PT harness | **VA** |
| 33 | PT: POSITION READY → sighters → verification | `PositionTransitionRightPanel` | `POSTRANS` state machine | PT harness | **VA** (logic) / **MVR** (flow) |
| 34 | PT timers each measure what is documented | Position Review | `POSTRANS.positionReview` | PT harness | **VA** |
| 35 | Ready→first shot includes the sighter phase | Position Review | `readyToFirstShotMs` | PT harness | **VA** |
| 36 | Rhythm Steady/Variable/Inconsistent thresholds | PT cards | `ptRhythmLabel` | rhythm classifier tests | **VA** |
| 37 | No rhythm label below 3 shots / missing timing | PT cards | `ptRhythmLabel` guard | rhythm tests | **VA** |
| 38 | Positions kept separate | PT summary | per-position records | PT harness | **VA** |
| 39 | Session highlights ranked correctly | SESSION HIGHLIGHTS | `sessionRankings` | PT harness | **VA** |
| 40 | PT summary layout | POSITION TRANSITION COMPLETE | `PositionTransitionHud` | — | **MVR** |
| 41 | Export PDF produces a branded report | report views | `CUSTOMPRINT` | — | **MVR** |
| 42 | PDF metadata is Tech Aim | — | `customprint.cpp` setTitle/setCreator | source inspection | **VA** |
| 43 | Report software label is "Tech Aim 0.9.0" | report footers | `PRODUCT.softwareVersionLabel` | identity test | **VA** |
| 44 | Training disclaimer present; absent on competition | report views | report views | — | **MVR** |
| 45 | German PDF: umlauts, wrapping, no overflow | exported PDFs | — | — | **MVR** + **DE** |
| 46 | Clean Home closes durably and resets UI | summary → Home | `closeCleanly` + UI reset | clean-Home test | **VA** |
| 47 | No stale counters after Home | — | store-active gating | clean-Home test | **VA** |
| 48 | New session gets a new identity | NEW SESSION | controllers | clean-Home test | **VA** |
| 49 | Save and Close / Keep for Recovery | close prompt | `main.qml` | — | **MVR** |
| 50 | Interrupted session offered for recovery | Recovery dialog | `RecoveryCoordinator` | recovery tests | **VA** (detection) / **MVR** (dialog) |
| 51 | Completed session never offered | — | archive-on-complete | recovery tests | **VA** |
| 52 | Resume restores phase and shot count | Recovery dialog | `loadRecoveredState` | recovery tests | **VA** |
| 53 | Damaged record reported, not silently loaded | — | `JournalValidator` hash chain | reliability tests | **VA** |
| 54 | Incident categories and scopes | `IncidentWindow` | `INCIDENTS` | incident tests | **VA** (model) / **MVR** (dialog) |
| 55 | Unresolved incident blocks official shots | shooting screen | incident gate | incident tests | **VA** |
| 56 | Settings controls behave as documented | `SettingsPage` | `APPSETTINGS` | — | **MVR** |
| 57 | Data locations as documented | — | `StoragePaths` | storage root logged at startup | **VA** (root) / **RC1** (final paths) |
| 58 | Installation / update / uninstall | — | — | — | **RC1** |
| 59 | Windows security warnings | — | — | — | **RC1** |
| 60 | Screenshots match current identity | — | — | — | **MVR** (none captured) |

## Summary

| Status | Count |
|---|---|
| VERIFIED AUTOMATICALLY (fully) | 30 |
| Partly VA, remainder MVR | 8 |
| MANUAL VALIDATION REQUIRED | 17 |
| WINDOWS RC1 DEPENDENT | 3 |
| PHYSICAL HARDWARE DEPENDENT | 2 |
| GERMAN REVIEW REQUIRED | 3 |
| **VERIFIED MANUALLY** | **0** |

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
