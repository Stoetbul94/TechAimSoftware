# SETA Single Target 1.0.0-EVAL1 — evaluation gate

**Verdict: READY FOR SETA EXTERNAL EVALUATION. Physical SETA test PENDING.**

Assembled 2026-08-30 at the close of Phase B.

---

## 1. What this build is

**The existing SETA product, brought forward to Tech Aim v1.0 functional
parity.** Not a renamed Tech Aim binary, and not a new SETA written from
scratch.

| | |
|---|---|
| Workspace | `C:\Users\User\Downloads\TechAimSoftware-SETA` |
| Branch | `product/seta` |
| Starting HEAD | `f22637f` — 0.9.0-RC3a-SETA |
| Tech Aim reference | `da03984` — 1.0.0-RC1, 6 948 checks / 0 failures, RC3F physical |
| Version | **1.0.0-EVAL1** |

---

## 2. Shared baseline — what came forward

The existing SETA derivative was **54 commits behind** and missing every
correctness fix checked: `AcquisitionSequencer` did not exist on it, the
sentinel coordinate guard was absent, and the serial default was still
QModMaster's 9600/None rather than the field-proven 19200/Even/8/1.

The port converged the shared tree on Tech Aim v1.0 rather than merging 54
commits. Full file-by-file reasoning:
[SETA-V1.0-CARRYOVER-MANIFEST.md](SETA-V1.0-CARRYOVER-MANIFEST.md).

| Area | Status |
|---|---|
| Acquisition (`AcquisitionDecision`, `AcquisitionSequencer`, sentinel, checked reads) | **PARITY** |
| Serial defaults — 19200 / Even / 8 / 1 | **PARITY** |
| Modbus serialisation, locking, reconnect / desync | **PARITY** |
| Counter / baseline reconciliation, reset handling | **PARITY** |
| Paper feed (`PaperFeedCoordinator`) | **PARITY** |
| Shot acceptance, coordinate validation, scoring | **PARITY** |
| Qualification, 50 m 3P, 10 m Finals, 3P Finals state machines | **PARITY** |
| Explicit `shotRole` persistence, shot-time persistence correction | **PARITY** |
| Reports, 10 m Final report + PDF, report routing | **PARITY** |
| CRO order / repeat, last-shot dwell | **PARITY** |
| Support bundle, `Collect-Logs`, journals, build identity | **PARITY** |

---

## 3. What stayed SETA

| | |
|---|---|
| Product identity, name, release channel | **PRESERVED** |
| SETA mark and `seta.ico` | **PRESERVED** |
| SETA blue palette (#00539E, sampled from the artwork) | **PRESERVED** |
| User-data namespace `TechAimSETA` | **PRESERVED** |
| German catalogue — **892 translated entries** | **PRESERVED**, untouched |
| **Teiler — visible**, as the SETA product has always shown it | **PRESERVED** |
| The competition selector and catalogue (ISSF *and* DSB entries) | **PRESERVED** |
| `RuleAuthority` journal schema extension | **PRESERVED** — Tech Aim changed 0 lines there |
| SETA deployment scripts and their auditor | **PRESERVED**, parameterised for the new executable name |

---

## 4. SETA executable identity

| | |
|---|---|
| Executable | **`SETA.exe`** (was `TechAim.exe`) |
| Windows `ProductName` / `FileDescription` | SETA Electronic Target Control |
| `InternalName` / `OriginalFilename` | SETA / SETA.exe |
| Embedded icon | the SETA mark |
| **Dormant Tech Aim product identity in the binary** | **NONE** — verified by byte search |
| Legal publisher | JAC SHOOTING SOLUTIONS (PTY) LTD — unchanged, a legal fact |

Achieved with the **compile-time `BRAND_SETA`** architecture SETA already had.
The experimental runtime dual-identity branch was deliberately **not**
transferred: it compiled both product names into one executable.

**Instance lock:** `instanceLockName` is now its own field, shared by every
flavour. `SETA.exe` and `TechAim.exe` still refuse to run together — one machine
drives one target — even though the file was renamed. Asserted by
`tests/finals` and `tests/qml`.

---

## 5. Automated gate

| Suite | Result |
|---|---|
| Reliability | **2 507 / 0** |
| Training Lab | **568 / 0** |
| 50 m 3P Finals | **240 / 0** |
| 10 m Finals | **229 / 0** |
| QML | **291 / 0** (+ **8 DSB assertions DEFERRED**) |
| Manuals | **1 388 / 0** |
| Project memory | **216 / 0** |
| Training Lab evidence | **903 / 0** |
| SETA deployment audit | **29 / 0** |
| SETA Windows icon | **7 / 0** |
| **TOTAL** | **6 878 / 0** |

**Runtime, from the packaged copy with no Qt on `PATH`:** 0 missing DLL,
0 TypeError, 0 ReferenceError, 0 QML type failures, 0 acquisition warnings,
7 pre-existing binding warnings (the same seven Tech Aim carries).

### The 8 deferred assertions

They test the DSB engine's integration into the shared screens. DSB is deferred
and excluded from the build, so they describe a configuration this binary
deliberately is not. They are printed as `DEFER`, counted separately, and
**neither passed nor deleted**.

### Two defects the portable launch found

1. `Finals10mReportView` and `Finals3PRightPanel` were carried across as files
   but never registered in `qml.qrc` — the application failed at ShootingPage.
   My earlier launch checks grepped only for TypeError/ReferenceError and a QML
   type failure says neither. The check is wider now.
2. The deployment lacked `qml/QtMultimedia` and `Qt6MultimediaQuick.dll`. Finals
   audio loads Qt Multimedia at runtime, so `windeployqt` never saw the import.
   The deployment auditor caught the second by walking the plugin's import table.

---

## 6. DSB 2026 — deferred

**Not included, not advertised, not deleted.** `SETA-DSB-PORT-001`.

`src/dsb/Dsb120Controller.{h,cpp}`, `Dsb120Hud.qml`, the DSB rule documents and
`tst_dsb120.cpp` all remain on `product/seta` unmodified. The sources are gated
out of the build behind `CONFIG+=dsb`, and `Dsb120Hud.qml` is out of the
resource bundle, so **no DSB mode can be selected in this build**. No DSB rule
logic was altered to make anything compile or pass.

The competition selector and catalogue are **not** DSB-only — they serve ISSF
too — so they stayed.

---

## 7. Physical status

| | |
|---|---|
| Shared core functional basis | **derived from the Tech Aim v1.0 proven baseline** — RC3F: 3 tablets, 3 athletes, 385 accepted live shots, 0 acquisition faults, one real reconnect recovered |
| SETA binary automated validation | **PASS** |
| **SETA binary physical validation** | **PENDING** |

**This build has never been fired at.** RC3F used a Tech Aim-branded binary.
The SETA-branded executable, its icon, its data namespace and its German
surface have not run on a range. That is what the evaluation is for.

Must not be written anywhere:
- ~~"SETA is physically qualified."~~
- ~~"DSB 2026 is supported."~~

---

## 8. Known limitations

| | |
|---|---|
| DSB 2026 | deferred — `SETA-DSB-PORT-001` |
| German coverage | 892 translated, 239 unfinished. Newer screens carried from Tech Aim v1.0 introduce English strings with no German yet. **No German was invented.** |
| 7 binding warnings | pre-existing in coach/incident views, unchanged since RC3B, in no acquisition, scoring, timing or competition path |
| SETA hardware assumptions | the build assumes the same electronics, Modbus protocol, feed mechanism and serial device RC3F proved. **An assumption, not a verified fact** — the evaluation settles it |
| Rendered SETA screens | not produced this round. Branding is verified by the deployment auditor (version resource, embedded icon, product name) and by `tests/qml`, not by screenshots |

---

## 9. Package

| | |
|---|---|
| Version | **1.0.0-EVAL1** |
| Branch | `product/seta` |
| Commit | **`47c5645`** |
| Path | `dist/v1.0.0-eval1/SETA-Single-Target-1.0.0-EVAL1-Windows-x64.zip` |
| Size | 45.52 MB |
| **ZIP SHA-256** | `5F609940F15D13C0455926C73496A5CD5AC0EAAC9325E442B26371C719789089` |
| **EXE SHA-256** | `16D0C029EB26DAF6EA666B109B01870C81B3D4351C757374EDE3C28CFECB7636` |

Built clean: `release/` deleted, fresh `qmake`, full recompile. The binary was
verified to contain `1.0.0-EVAL1` and commit `47c5645` before packaging.

Ships with the evaluation checklist, the DSB deferral record and the carryover
manifest.

---

## 10. Tech Aim was not modified

`da03984`, `feature/rc2e-latency-and-reset` and the frozen
`TechAim-1.0.0-RC1-Windows-x64.zip` are untouched. All Phase B work is on
`product/seta` in the SETA workspace. Android and RMS were mapped only — not
modified.
