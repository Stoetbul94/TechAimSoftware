# SETA v1.0 — carryover manifest

The audit trail for bringing `product/seta` forward to Tech Aim v1.0 functional
parity **without a historical merge**.

| | |
|---|---|
| Target | `C:\Users\User\Downloads\TechAimSoftware-SETA`, branch `product/seta` |
| Starting HEAD | **`f22637f`** — 0.9.0-RC3a-SETA, 2026-08-19 |
| Tech Aim reference | **`da03984`** — 1.0.0-RC1, 6 948 checks / 0 failures, RC3F physical |
| Common ancestor | `8d1d243` |

## How the work was scoped

A 54-commit merge was rejected as too conflict-prone. Instead every file was
placed in one of five buckets by asking **two** questions: did Tech Aim change
it since the ancestor, and did SETA?

| | Tech Aim changed | SETA changed | Files | Action |
|---|---|---|---|---|
| **1** | yes | **no** | **73** | **TAKE TECH AIM WHOLESALE** — zero risk; SETA has no opinion on these |
| **2** | yes (new file) | n/a | **44** | **ADD FROM TECH AIM** — files SETA never had |
| **3** | **no** | yes | **20** | **KEEP SETA** — SETA's own additive work; Tech Aim has no newer version |
| **4** | n/a | SETA-only | **22** | **KEEP SETA** — SETA-specific and DSB |
| **5** | yes | yes | **25** | **JUDGE PER FILE** — the only real decisions |

**Only 25 files needed judgement.** That is the whole point of doing it this
way rather than merging.

---

## Bucket 1 — take Tech Aim wholesale (73)

SETA never touched these since the ancestor, so taking the proven version
cannot lose SETA work.

| Area | Files | Brings |
|---|---|---|
| `src/target/` | 2 | `AcquisitionDecision`, paper-feed coordination |
| `src/finals10m/` | 2 | the 10 m Final controller: `shotRole`, `buildReport`, corrected shot-time persistence |
| `src/finals/` | 2 | `Finals3PController`, `FinalsReportBuilder` |
| `ModReader/` | 5 | `TachusWidget` accepted-shot path, Modbus, **serial defaults 19200/Even/8/1** |
| root `.qml` | 7 | qualification, finals and report screens |
| root C++ | 3 | |
| `tests/` | 6 | the harnesses that prove the above |
| `tools/`, `docs/` | 47 | packaging, renderer, rule and field-evidence documents |

## Bucket 2 — add from Tech Aim (44)

Files SETA does not have at all. The competition-critical ones:

| File | Why |
|---|---|
| `src/target/AcquisitionSequencer.h` | the acquisition sequencer — **absent from SETA entirely** |
| `Finals10mReportView.qml` | the 10 m Final report (BLOCKER G) |
| `Finals3PRightPanel.qml` | the 3P Final panel (`FINALS-3P-MIX-001`) |
| `tests/reliability/tst_acquisition_integrity.cpp` | acquisition harness |
| `tests/reliability/tst_logging_integrity.cpp` | logging harness |
| `tests/finals/panel_harness.qml` | 3P panel harness |
| `tools/release/Collect-Logs.cmd`, `build-v1-package.ps1` | support bundle + packaging |
| + rule documents, field evidence, renderer scenes | |

## Bucket 3 — keep SETA, additive, Tech Aim has nothing newer (20)

**Tech Aim changed 0 lines in every one of these.** Keeping SETA's version
loses nothing and preserves real SETA work.

| File(s) | SETA's addition |
|---|---|
| `src/reliability/events/EventTypes.h` (+126) | **`RuleAuthority`** — the adopted competition rule snapshotted into the session. A recovered session reads its rules from the snapshot and never re-resolves them from a catalogue that may have moved. Absent = LEGACY, an explicit valid state. |
| `src/reliability/events/{DomainEvent.h,EventRegistry.cpp,EventSerializer.cpp}` | registration and wire format for it |
| `src/reliability/reducer/{SessionReducer.cpp,SessionState.{h,cpp}}` | folds it into recovered state |
| `src/reliability/recovery/*`, `store/*` | carries it through recovery |
| `src/qualification/QualificationController.{h,cpp}` (+185) | an OPTIONAL profile handed over before `startSession`. **An empty map means no profile — a legacy session, which is valid.** ISSF behaviour is unchanged when no profile is set, which is every EVAL1 session. |
| `src/training/*`, `src/bridge/*`, `src/app/DocumentationCapture.cpp` | minor SETA additions |

**Consequence: the journal schema extension survives**, so DSB's persistence
support is not broken by this port even though DSB itself is deferred.

## Bucket 4 — keep SETA, SETA-specific (22)

| File(s) | Class |
|---|---|
| `src/dsb/Dsb120Controller.{h,cpp}`, `Dsb120Hud.qml`, `SetaCompetitionSelector.qml` | **DSB — DEFER / PRESERVE** |
| `docs/rules/dsb-2026-*.md` (4), `tests/reliability/tst_dsb120.cpp` | **DSB — DEFER / PRESERVE** |
| `tests/reliability/tst_qual3p.cpp`, `tst_rule_authority.cpp` | SETA harnesses for the above |
| `images/logo/seta.ico`, `tools/icon/make_seta_ico.py` | **SETA-SPECIFIC** |
| `scripts/deploy/deploy-seta-release.ps1`, `smoke-test-runtime.ps1` | **SETA-SPECIFIC** |
| `tests/release/check_deployment.py`, `check_windows_icon.py` | **SETA-SPECIFIC** |
| `docs/product/seta-product-line.md`, `docs/deployment/seta-windows-runtime.md` | **SETA-SPECIFIC** |
| `tools/uirender/scene_{seta,dsb}_selector.qml`, `scene_opmode_pills.qml` | SETA render scenes |

## Bucket 5 — the 25 files that needed judgement

| File | SETA delta | Tech Aim delta | Classification | Action |
|---|---|---|---|---|
| `src/app/ProductIdentity.cpp` | +76/-7 | +13/-1 | **SHARED WITH SETA DELTA** | Tech Aim base + **keep the `BRAND_SETA` block**, compile-time (§7) |
| `src/app/ProductIdentity.h` | (bucket 3) | | SETA-SPECIFIC | keep; add Tech Aim's `instanceLockName` |
| `src/app/BrandPackage.{h,cpp}` | +81/-17 | +19/0 | **SHARED WITH SETA DELTA** | keep SETA's `makeSeta()`; add `showTeilerMetric`, `accentBright`, `textOnAccent`, `focusOutline`, `accentSubtleLight` |
| `src/app/ProductIdentityBridge.h` | (bucket 3) | | SHARED WITH DELTA | add the brand-mark properties |
| `Seta.pro` | +22 | +13/-2 | **SHARED WITH SETA DELTA** | Tech Aim base + SETA flavour + **NTFS case-collision fix** |
| `TechAim.rc` | +23/-3 | +4/-4 | **SETA-SPECIFIC** | SETA keeps its resource; version bumped |
| `main.cpp` | — | — | SHARED | Tech Aim, + `applicationStorageName` and the lock |
| `appsettings.{h,cpp}` | +95/-34 | +86/-1 | **SHARED WITH SETA DELTA** | Tech Aim base + SETA's brand settings scope |
| `customprint.cpp` | +18/-13 | +12/-4 | **SHARED WITH SETA DELTA** | Tech Aim base (parameterised finals PDF) + SETA attribution |
| `CenterPane.qml` | +17/-4 | +202/-7 | **SHARED** | **Tech Aim** — CRO order/repeat fixes live here |
| `ShootingPage.qml` | +304/-29 | +147/-16 | **SHARED + DSB** | **Tech Aim**; DSB integration gated out of EVAL1 — see below |
| `LoginPage.qml` | +512/-189 | +90/0 | **SHARED + DSB** | **Tech Aim**; catalogue/DSB selector gated out of EVAL1 |
| `ReportWindow.qml` | +5/-5 | +36/-12 | **SHARED** | **Tech Aim** — 10 m Finals routing |
| `MatchReportInfo.qml` | +1/-1 | +19/-39 | **SHARED WITH SETA DELTA** | Tech Aim + **Teiler restored, brand-gated** |
| `SummaryReportView.qml` | +2/-2 | 0/-2 | **SHARED WITH SETA DELTA** | Tech Aim + **Teiler restored, brand-gated** |
| `Theme.qml`, `src/ui/theme/DesignTokens.qml` | +42/-24 | +95/-38 | **SHARED WITH SETA DELTA** | Tech Aim tokens, resolved through the SETA brand package |
| `qml.qrc`, `images.qrc` | +2 each | +2 each | SHARED | union — both new views and `seta.ico` |
| `tests/*` (7 files) | | | **SHARED WITH SETA DELTA** | Tech Aim harnesses + SETA's brand/Teiler assertions |
| `tools/uirender/main.cpp` | +42/-2 | +51/0 | SHARED WITH DELTA | Tech Aim + SETA render selection |
| `CLAUDE.md`, `.gitignore` | | | SHARED | union |

---

## DSB — the one place this port cannot be purely additive

`ShootingPage.qml` carries **59 DSB references** on `product/seta` and
`LoginPage.qml` carries 11 plus the competition-catalogue selector. Tech Aim's
versions of those two files carry the CRO fixes, the last-shot dwell, the 10 m
Finals routing and the report wiring — all of which SETA must have.

**Approach chosen (§15): gate DSB out of the EVAL1 build; delete nothing.**

- `src/dsb/Dsb120Controller.{h,cpp}`, `Dsb120Hud.qml`,
  `SetaCompetitionSelector.qml`, the DSB rule documents and
  `tst_dsb120.cpp` all **remain on disk and in history, unmodified**.
- The DSB *integration points inside the shared screens* do not survive the
  convergence, and the DSB sources are excluded from the EVAL1 build target so
  nothing half-wired can compile into the evaluation binary.
- The complete pre-port integration remains recoverable from **`f22637f`**.
- **No DSB rule logic is altered to make anything pass.**

Recorded as [SETA-DSB-PORT-001](SETA-DSB-PORT-001.md).

## Not transferred from the experimental branch

`integration/seta-v1.0` (`a9d1b75`, `915b902`, `ef92692`) is reference only —
never merged, never packaged. One thing in it is deliberately **left behind**:
the runtime dual-identity branch, which compiled both product names into one
executable. SETA uses the compile-time `BRAND_SETA` architecture it already
had, so the SETA binary carries no dormant Tech Aim product identity (§7).
