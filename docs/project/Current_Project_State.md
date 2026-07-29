# Tech Aim — Current Project State

**Updated:** 2026-07-29 · **Reviewed at commit:** `d4674d0`

The single place to look first. Every future UI phase must read this before
starting — see `CLAUDE.md`, *Tech Aim UI project memory*.

---

## 1. Product

| | |
|---|---|
| Product | Tech Aim Electronic Target Control |
| Publisher | JAC SHOOTING SOLUTIONS (PTY) LTD |
| Version | 0.9.0 |
| Release channel | Pre-Beta Validation |
| Repository | `C:\Users\User\Downloads\TechAimSoftware-repo\seta10` |
| Branch | `feature/training-lab` |
| Executable | `TechAim.exe` |

## 2. Commits

| Milestone | Commit |
|---|---|
| Application baseline (pre-UI work) | `a74d3fd` |
| **Version B implementation** | `8022033` |
| Design-system foundation | `37c9e2b` docs · `1bc6b80` tokens + BrandPackage · `f62d289` homepage binding · `15a3492` tests |
| **Homepage defect fix** | `41c09a3` (P0) + `d4674d0` (completion) |
| Project memory | `445665e` |
| Current accepted HEAD | `d4674d0` |

## 3. Phase

| | |
|---|---|
| Latest completed | **UI-2 — Version B homepage · ACCEPTED 2026-07-29** |
| Current | **TRAINING LAB RELEASE 2 — WIND MAP · stages 5.2 + 6.1 (shared Training shell boundary, analysis review) — implemented, human visual check outstanding** |
| Next approved | Stage 6.2 — branded Wind Map PDF report, consuming `analysisModel()` unchanged |

**Version B is the accepted Tech Aim Beta homepage** (UI-DEC-012), approved by
**HUMAN VISUAL APPROVAL — ARNOLD BAILIE** on 2026-07-29 against commit
`d4674d0`. Every defect (UI-HOME-001…010) has a fix, passing automated checks
and an approved rendered result. **Seven are fully closed; three — UI-HOME-002,
003, 004 — are appearance-approved but interaction-unverified**, because
scrolling, event transitions and the folder picker were never driven by hand.

**The homepage is closed to styling change** unless a new defect is found.

## 4. Tests

| Suite | Result |
|---|---|
| Reliability (incl. UI-1 brand, UI-2 layout, Wind Map) | **1953 / 0** |
| Documentation — manuals | **979 / 0** |
| Documentation — project memory | **155 / 0** |
| Training | **567 / 0** |
| Finals 10m | **143 / 0** |
| 3P Finals | **233 / 0** |
| qmllint | clean on all shipped QML |

Reliability grew 902 → 988 (UI-1, +86) → 1041 (UI-2 P0, +53) → 1059 (UI-2
completion, +18) → 1695 (Wind Map stages 3–4.1) → 1815 (Wind Map stage 5:
controller/workflow/resume + QML source guards, +120) → 1911 (stage 6
analytics engine, +96) → 1953 (stages 5.2 + 6.1: shared Training shell
boundary, analysis review model and engine/view equality, +42).

The reliability harness is `QT = core`. `WindMapController.cpp` compiles into
it, which is the proof that the controller carries no QML/GUI dependency.

## 5. Build and runtime

| | |
|---|---|
| Build | **clean** — `qmake` + `mingw32-make -f Makefile.Release` |
| Launch | **verified** under the isolated documentation-capture profile; window title gate exact |
| QML warnings from `LoginPage.qml` | none |
| Pre-existing QML warnings | `CoachDetailedView`, `CoachPrintView`, `IncidentWindow` — unrelated, untouched |

## 6. Visual review and screenshot status

**Reviewed and approved** — HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29,
at 1536 × 960 in English against commit `d4674d0`.

**Not reviewed:** 1366 × 768, 1280 × 720 and 1100 × 700 were never opened; the
German catalogue has never been run on this page; and no interaction was driven
by hand (wheel/touch scrolling, event transitions, the folder picker). Both are recorded as
NOT TESTED in the acceptance checklist. Neither blocks the accepted design.

**No screenshot file exists.** Automated capture remains blocked by endpoint
security, which blocks both synthetic input and the screen-capture helper. No
bypass was attempted. The approval is a reviewer sign-off, not an image.
Screenshots are still wanted for the operator manuals — see §9.

The only screenshot in the repository is
`manual-preview/ui-audit/raw/01-home.png`, captured at baseline `a74d3fd` —
the **pre-Version-B** page. UI-0 evidence only; not Version B evidence.

## 7. Manual status

Operator manuals, Quick Start and Troubleshooting exist (EN + DE beta) and pass
979 documentation checks. **They are out of date with respect to the homepage:**
every Home-screen screenshot and any prose describing the old layout needs
regenerating after visual approval. Deferred until the page is accepted, so the
work is done once.

## 8. Release blockers

| # | Blocker | Owner |
|---|---|---|
| 1 | ~~Homepage not visually approved~~ **CLEARED 2026-07-29** | — |
| 2 | **No Windows icon** — `TechAim.rc` declares no `ICON`, so the executable ships the default Qt/MinGW icon | brand approval |
| 3 | Manuals not regenerated for the accepted homepage | documentation |
| 4 | Licence-expiry check DISABLED; re-enabling needs approval + a test fixture | separate approval |
| 5 | EULA screen: acceptance lives in `HKCU\Software\Seta\shootingApp` — legacy registry path under the old brand | review |

Not blockers, but open: three window sizes and the German catalogue were never
reviewed (§6).

## 9. Next approved phase

# TRAINING LAB RELEASE 2 — WIND MAP

A **separate feature phase**. Wind Map work must not be combined with homepage
commits, and the homepage is closed to styling change (UI-DEC-012).

**Stage 1 complete and APPROVED** — all seven specification questions
answered 2026-07-29 (`docs/training-lab-wind-map-spec-review.md` §7).

**Stage 2 complete and APPROVED** —
`docs/training-lab-wind-map-implementation-spec.md` defines the domain model,
six events, reducer state, controller phases, journal format, recovery
behaviour, analytics formulas, minimum-sample rules, 3P separation, UI
workflow, report structure, a 21-case test plan and ten explicit non-goals.

**Stage 3 (domain + events) ACCEPTED** at `d023d21`; **Stage 4 (recovery
proof)** and **Stage 4.1 (Training snapshot parity)** ACCEPTED at `cbacae6`
(`docs/training-lab-wind-map-recovery-audit.md`,
`docs/training-snapshot-parity-audit.md`).

**Stage 5 implemented** — `docs/training-lab-wind-map-capture.md`. The
programme is now usable end to end: `WindMapController` (`WINDMAP`), the
Training Lab catalogue entry (50 m rifle only), the setup view, the capture
panel with the eight-sector compass ring, manual condition entry, the factual
session review, and recovery dispatch. Stage 5 found that the workflow phase
had to become **durable** — without it a session interrupted after
`finishSighters()` but before the first counted shot resumes into the wrong
phase and misclassifies the next shot — so `WindMapPhaseChanged` was
appended to the event catalogue and `SessionState::wmPhase` snapshot-serialised
(state v5 → v6).

**Stage 5 is NOT accepted yet:** the automated evidence is complete
(reliability 1815/0, training 567/0, finals10m 143/0, 3P finals 233/0, clean
application build, clean launch check) but the visual check is
**MANUAL-ASSISTED VISUAL CHECK REQUIRED** — automatic capture remains
blocked by endpoint security and no workaround was attempted.

**The PDF report is NOT implemented.** Stage 6.2 will consume
`analysisModel()` unchanged — no separate PDF calculation is permitted.

**Stage 5.1 ACCEPTED** — `8a1fe26`. UI-WIND-001 (Wind Map 3P capture screen
rendering Final 35 / Ceremony / timing state) is **closed** with
**HUMAN VISUAL APPROVAL — ARNOLD BAILIE, 2026-07-29** at **1536 × 960
logical**. Other resolutions are recorded as NOT TESTED.

**Stages 5.2 + 6.1 implemented** — `docs/training-lab-wind-map-analysis.md`.
5.2 replaced four drifting per-programme gates with ONE shared boundary
(`isTrainingModeAny`) and one shared `TrainingTopBar` for all four Training Lab
programmes, closing the same defect in Call & Diagnose, Position Transition and
Technical Blocks (UI-TRAIN-001/002/003, registered before the fix). 6.1 added
`WINDMAP.analysisModel()` and the ten-section analysis review, with the
analytics engine as the only calculation authority — asserted by a value-by-
value equality test between the engine and the view model.

**UI-TRAIN-001/002/003 remain OPEN**: the code is fixed and automated evidence
passes, but none of the three sibling capture screens has been reviewed on
screen. **The Wind Map analysis review has not been visually reviewed at any
resolution.**

Approved scope: `Wind Map — Post-Session Review`, **50 m Rifle Prone and 3P
only**, manual wind entry stored as numeric degrees + m/s, a standing condition
that each accepted shot snapshots **immutably**, sighters recorded but excluded
from counted statistics, and **descriptive-only** analytics — correlation
never causation, every figure shown with its sample size.

Required stages, in order (1–5 done; 6–8 outstanding for analytics/report):

1. **Specification review** — read `docs/issf-rules/README.md` and the
   applicable discipline files first. If the rules needed are incomplete,
   ambiguous or marked *Awaiting official rule confirmation*, **stop and
   request the missing official rule** before implementing.
2. **Training-domain events and state** — Wind Map is a Training Lab
   programme, classified and started through the TRAINING owner. It is never a
   qualification or Final session.
3. **Journal and recovery support** — append-only events through
   `SessionStore`, a reducer path, and a restorer registered with the
   discipline recovery dispatcher. Recovery must fail safe on an unknown
   discipline, never silently fall back to Finals.
4. **Neutral analytics** — the analytics engine stays pure C++ and
   presentation-free; every reported value derives in C++, QML only formats.
5. **3P position separation where applicable** — gate anything 3P-specific on
   `is3PMatch` and follow `docs/3p-discipline.md`.
6. **Report / PDF output** — the Tech Aim Report System components on white A4
   pages, via the existing `CUSTOMPRINT` / `PdfExporter` paths.
7. **Automated tests** — extend the training harness; no phase closes on a
   build alone.
8. **Focused commits** — one concern per commit.

### Carried forward, not part of Wind Map

- Regenerate the operator-manual screenshots for the accepted homepage.
- Capture the three unreviewed window sizes and a German pass, if wanted.
- Windows icon (blocker #2) and the EULA registry path (blocker #5).

## 10. Deferred

| Item | Reason |
|---|---|
| Token migration beyond the homepage | UI-DEC-004 — scope decision |
| Component adoption on the homepage | `TaButton`/`TaStatusChip`/`TaWarningBanner` exist but are unused there; swapping verified markup in the same phase that introduced them risks regression |
| Version C stepped workflow | UI-DEC-011 — post-Beta concept |
| SETA OEM appearance | UI-DEC-010 — reserved, unbuildable, no approved assets |
| Single-column layout below 880 px | rule defined, not implemented |
| Loading / skeleton states | undesigned |
| 25 m Pistol disciplines | scope decision outstanding |
| Range Management System | **separate product**, not lane software, and explicitly not the next lane task |

## 11. Required final environment

| | |
|---|---|
| `app_mode` | **Live** ✅ current |
| Language | **English** ✅ current |
| `release/config.ini` | untracked, ignored by `.gitignore:3` ✅ |

## 12. Note on the production data root

The isolated capture profile leaves the production root byte-identical —
verified before and after every launch. Between UI-1 and UI-2, three files
appeared in `%LOCALAPPDATA%\TechAim\TechAim\cache\`: Qt QML and pipeline
**caches**, written by diagnostic runs of `release/TechAim.exe` made *outside*
the capture profile. No session, athlete or result data was created or
modified. Recorded for completeness so the earlier "123 files / 3,151,405
bytes" figure is not treated as a discrepancy.
