# Tech Aim — deployment documentation

Prepared on `release/0.9.0-deployment-prep` from commit `4151620`.

> **Status: READY FOR PHYSICAL RETEST — NOT APPROVED FOR FINAL DEPLOYMENT.**
> See [0.9.0-deployment-readiness.md](../release/0.9.0-deployment-readiness.md).

## Start here

| If you are… | Read |
|---|---|
| Running the software at a range | [Operator-Guide.md](Operator-Guide.md) |
| Running the physical retest | [Physical-Qualification-Checklist.md](Physical-Qualification-Checklist.md) |
| About to use this for a real session | [Field-Test-Notice.md](Field-Test-Notice.md) |
| Diagnosing a problem | [Diagnostics-Appendix.md](Diagnostics-Appendix.md) |
| Deciding whether to deploy | [0.9.0-deployment-readiness.md](../release/0.9.0-deployment-readiness.md) |

## The 14 operator documents

| # | Topic | Where |
|---|---|---|
| 1 | Installation / extraction | [Operator-Guide](Operator-Guide.md) §1 |
| 2 | First startup | §2 |
| 3 | Target connection | §3 |
| 4 | Automatic COM detection | §4 |
| 5 | Manual COM fallback | §5 |
| 6 | Paper-feed configuration | §6 |
| 7 | Report export | §7 |
| 8 | Logs and support bundle | §8 |
| 9 | Upgrade | §9 |
| 10 | Rollback | §10 |
| 11 | Uninstall / removal | §11 |
| 12 | Known limitations | [0.9.0-known-limitations.md](../release/0.9.0-known-limitations.md) |
| 13 | Internal field-test notice | [Field-Test-Notice.md](Field-Test-Notice.md) |
| 14 | Physical qualification checklist | [Physical-Qualification-Checklist.md](Physical-Qualification-Checklist.md) |

The operator guide contains **no** VID/PID values, Modbus register numbers or
internal class names. Those are in [Diagnostics-Appendix.md](Diagnostics-Appendix.md).

## Engineering documents

| Document | Contents |
|---|---|
| [Deployment-Audit.md](Deployment-Audit.md) | Packaging, windeployqt, DLLs, plugins, storage, logs, installer, signing, crash dumps. **Findings PKG-001/002, CFG-001, LOG-001, SUP-002/003.** |
| [Configuration-Reference.md](Configuration-Reference.md) | Every setting, defaults, and what must never be packaged |
| [AppData-and-Upgrade-Safety.md](AppData-and-Upgrade-Safety.md) | Storage layout and the upgrade/rollback/removal drill — 35 checks, 0 failures |
| [Clean-Machine-Test.md](Clean-Machine-Test.md) | **BLOCKED.** Why, and the 20-step checklist |
| [Installer-Options.md](Installer-Options.md) | No installer exists; options compared; signing is the blocker |
| [Diagnostics-Appendix.md](Diagnostics-Appendix.md) | Selection scoring, Modbus, feed rules, shot stamps, auto-zoom gate |

## Tools

All in `tools/deployment/`.

Verify a package before deploying it:

```bash
powershell -File tools\deployment\Verify-Deployment.ps1 -PackageDir C:\TechAim\RC2a -ExpectDeveloperMode 0
```

Assemble the deployment-preparation structure:

```bash
powershell -File tools\deployment\New-DeploymentPrep.ps1
```

Assemble the physical retest pack (never rebuilds RC2a):

```bash
powershell -File tools\deployment\New-RC2aRetestPack.ps1
```

Run the AppData / upgrade drill:

```bash
powershell -File tools\deployment\Test-AppDataUpgrade.ps1 -OldPackage <old folder> -NewPackage <new folder>
```

Collect a support bundle (run from the folder holding `TechAim.exe`):

```bash
powershell -File Make-SupportBundle.ps1 -Diagnostic
```

## What is not covered

- **No installer** is built, and no framework is chosen.
- **No code signing.** Every artefact is unsigned.
- **No crash dumps** are captured.
- **The clean-machine test has not been performed.**
- **Only one 10 m Air Rifle Training workflow has ever been physically tested.**
