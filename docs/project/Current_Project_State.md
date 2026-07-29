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
| Latest completed | UI-1 — Design System and Brand Package foundation |
| Current | UI-2 — homepage defect correction · **code complete** |
| Next approved | **Capture real homepage screenshots and obtain visual approval** |

**All ten homepage defects are fixed with automated evidence. None is closed**,
because closure requires a real screenshot and none exists. UI-2 is code
complete but **not accepted**. See `docs/ui/UI_Defect_Register.md`.

## 4. Tests

| Suite | Result |
|---|---|
| Reliability (incl. UI-1 brand + UI-2 layout) | **1059 / 0** |
| Documentation — manuals | **979 / 0** |
| Documentation — project memory | **155 / 0** |
| Training | not re-run this phase — no training code touched |
| Finals 10m | not re-run this phase — no finals code touched |
| 3P Finals | not re-run this phase — no finals code touched |
| qmllint | clean on all shipped QML |

Reliability grew 902 → 988 (UI-1, +86) → 1041 (UI-2 P0, +53) → 1059 (UI-2 completion, +18).

## 5. Build and runtime

| | |
|---|---|
| Build | **clean** — `qmake` + `mingw32-make -f Makefile.Release` |
| Launch | **verified** under the isolated documentation-capture profile; window title gate exact |
| QML warnings from `LoginPage.qml` | none |
| Pre-existing QML warnings | `CoachDetailedView`, `CoachPrintView`, `IncidentWindow` — unrelated, untouched |

## 6. Screenshot status

**BLOCKED — MANUAL-ASSISTED SCREENSHOT CAPTURE REQUIRED.**

No screenshot of the Version B homepage exists at any resolution. Endpoint
security on this machine blocks synthetic input injection *and* the
screen-capture helper. No bypass was attempted and none should be.

The only real screenshot in the repository is
`manual-preview/ui-audit/raw/01-home.png`, captured at baseline `a74d3fd` —
the **pre-Version-B** page. It is UI-0 evidence only.

## 7. Manual status

Operator manuals, Quick Start and Troubleshooting exist (EN + DE beta) and pass
979 documentation checks. **They are out of date with respect to the homepage:**
every Home-screen screenshot and any prose describing the old layout needs
regenerating after visual approval. Deferred until the page is accepted, so the
work is done once.

## 8. Release blockers

| # | Blocker | Owner |
|---|---|---|
| 1 | No Version B screenshots; homepage not visually approved | Arnold + capture |
| 2 | ~~UI-HOME-004~~ resolved in `d4674d0`; UI-DEC-008 now ACCEPTED | — |
| 3 | ~~UI-HOME-005…009~~ all resolved in `d4674d0` | — |
| 4 | **No Windows icon** — `TechAim.rc` declares no `ICON`, so the executable ships the default Qt/MinGW icon | brand approval |
| 5 | Manuals not regenerated for the new homepage | after #1 |
| 6 | Licence-expiry check DISABLED; re-enabling needs approval + a test fixture | separate approval |
| 7 | EULA screen: acceptance lives in `HKCU\Software\Seta\shootingApp` — legacy registry path under the old brand | review |

## 9. Next approved task

**Capture real homepage screenshots and obtain visual approval.**

1. Capture the homepage at 1536×960, 1366×768, 1280×720 and 1100×700 from the
   isolated capture profile (Demo, English, synthetic athlete), saved as
   `manual-preview/ui-audit/raw/vb-home-<w>x<h>.png`.
2. Register them in `docs/ui/UI_Defect_Register.md` §3.
3. Close the per-resolution tables in `docs/ui/Homepage_Acceptance_Checklist.md`.
4. Obtain Arnold's visual approval, moving the ten defects to
   `RESOLVED — AUTOMATED AND VISUAL EVIDENCE`.
5. Regenerate the manual screenshots.
6. Then Windows RC1 preparation.

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
