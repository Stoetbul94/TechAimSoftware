# SETA Single Target 1.0.0-EVAL2 — evaluation gate

**Verdict: READY FOR SETA EXTERNAL EVALUATION.**

| | |
|---|---|
| **AUTOMATED ACCEPTANCE** | **PASS** |
| **VISUAL PRODUCT ACCEPTANCE** | **PASS** |
| **PHYSICAL SETA ACCEPTANCE** | **PENDING** |

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
| Version | **1.0.0-EVAL2** |

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
| QML | **286 / 0** (+ **13 DSB assertions DEFERRED**) |
| Manuals | **1 402 / 0** |
| Project memory | **216 / 0** |
| Training Lab evidence | **903 / 0** |
| SETA deployment audit | **31 / 0** |
| SETA Windows icon | **7 / 0** |
| **TOTAL** | **6 389 / 0** |

**Runtime, from the packaged copy with no Qt on `PATH`:** 0 missing DLL,
0 TypeError, 0 ReferenceError, 0 QML type failures, 0 acquisition warnings,
7 pre-existing binding warnings (the same seven Tech Aim carries).

### The 13 deferred assertions

Eight test the DSB engine's integration into the shared screens; five assert
that DSB is OFFERED in the selector and that every catalogue entry is reachable
through it. DSB is deferred
and excluded from the build, so they describe a configuration this binary
deliberately is not. They are printed as `DEFER`, counted separately, and
**neither passed nor deleted**.

### Two further defects the portable launch found

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
| Rendered SETA screens | produced offline for the selector, discipline list, left panel, settings and operating-mode pills. They show component layout and palette, **not the running application** — no screenshot of a live session exists |

---

## 8a. Visual product acceptance — and what it found

The packaged build was extracted and run with no Qt on `PATH`, and
representative screens were rendered offline (no synthetic input, no screen
capture).

| Check | Result |
|---|---|
| Packaged `SETA.exe` launches | **PASS** |
| SETA icon, product title, application identity | **PASS** |
| Version and commit correct | **PASS** |
| Missing DLL · TypeError · ReferenceError · QML type failure | **0 / 0 / 0 / 0** |
| Unexpected Tech Aim branding | **NONE** |
| SETA blue palette, no Tech Aim maroon in product components | **PASS** |
| Clipping, missing components, broken alignment | **NONE SEEN** |
| Teiler visible where SETA shows it | **PASS** (brand-gated, asserted in `tests/qml`) |

### Four defects this round found — none in the functional core

1. **The support collector searched one directory level too high.** It looked in
   `%LOCALAPPDATA%\TechAim` — the vendor folder — while Qt puts the data in
   `<organisation>\<application>`. The vendor folder has no `Sessions`
   directory, and `-ErrorAction SilentlyContinue` turned that into an empty
   result, so **every bundle reported 0 journals without saying it had looked in
   the wrong place**. Measured here: **0 journals before, 41 after.**
   *This corrects the Phase A blocker inventory*, which recorded the collector as
   working and blamed the empty RC3F bundles on it never being run. It was also
   broken.
2. **The collector did not ship in the package at all.** `check_deployment.py`
   now fails a package without it.
3. **The bundle named the wrong product.** Run from the deployed package it
   reported `Product: Tech Aim Electronic Target Control` to a SETA operator,
   and `UNKNOWN` for the version and commit — it looked for a manifest filename
   and an executable name that the SETA deployment does not use.
4. **The selector still offered DSB 2026.** Found by the visual check, not by a
   test. An operator could have chosen a rule set the build cannot run. The rule
   set is gated; the catalogue data is untouched.

None of these is in acquisition, scoring, Modbus, reconnect, paper feed,
qualification, 3P, finals, persistence, reports, CRO or last-shot dwell. No
functional core file changed between the tested binary and this one.

### Why EVAL1 is not the shipped package

EVAL1 was built, packaged and inspected but **never released**: it had no
support collector, and the collector itself was broken. SETA's physical test is
the next evidence gate and would have produced a bundle worth nothing.

Rather than replace EVAL1's hashes underneath the same name, this is a new
identity. **EVAL1 is superseded and must not be distributed.**

---

## 9. Package

| | |
|---|---|
| Version | **1.0.0-EVAL2** |
| Branch | `product/seta` |
| Commit | **`26a3b11`** |
| Path | `dist/v1.0.0-eval2/SETA-Single-Target-1.0.0-EVAL2-Windows-x64.zip` |
| Size | 45.53 MB |
| **ZIP SHA-256** | `2B55E4F16EA7A2B6525CE2B872CBA665D65027AAB7323AE26035BF93BC184C96` |
| **EXE SHA-256** | `5573E86E5F1718CD554FF76120FD58082C7BADE12693669B296EBDE411D0C420` |

**Superseded, do not distribute:** 1.0.0-EVAL1, ZIP
`5F609940F15D13C0455926C73496A5CD5AC0EAAC9325E442B26371C719789089`.

Built clean: `release/` deleted, fresh `qmake`, full recompile. The binary was
verified to contain `1.0.0-EVAL2` and commit `26a3b11` before packaging.

Ships with `Collect-Logs.cmd`, `Make-SupportBundle.ps1`, the evaluation
checklist, the DSB deferral record, the carryover manifest and this gate.

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
