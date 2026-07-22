# Windows Release & Code Signing (TechAim 0.9.0)

Status: **release candidate, unsigned.** This document prepares the build for
later signing and records how the self-contained package is produced. No
certificate is purchased or applied in this phase.

## 1. What ships

A self-contained folder produced from the verified Release runtime:

```
release-package/TechAim-0.9.0/
  Seta.exe                     ← the application (the only file that is signed)
  Qt6*.dll, lib*.dll, ...      ← Qt + MinGW runtime
  platforms/qwindows.dll       ← required platform plugin
  multimedia/ imageformats/ tls/ sqldrivers/ styles/ ...   ← plugins
  qml/                          ← Qt QML modules (Qt, QtQml, QtQuick, QtCharts)
  config.ini                    ← default configuration template (app_mode=Live)
  release-manifest.json         ← build + runtime + hash manifest
  SHA256SUMS.txt                ← SHA-256 of every file
  README.txt
```

The application's own QML/images are compiled into `Seta.exe` (Qt resources);
the `qml/` folder only carries the Qt framework modules.

### Producing the package
The canonical tool is `windeployqt` (Qt 6.5.3 MinGW). **Known issue on this
machine:** `windeployqt` aborts with `Unable to find the platform plugin` for
this Qt install, deploying nothing. Until that is resolved, the package is
assembled from the already-deployed, launch-verified `release/` runtime with
build artifacts (`*.o`, `*.cpp`, `*.h`, `Makefile*`, `moc_*`, `qrc_*`,
`object_script.*`), saved match files (`*.tch`), test PDFs, `qmlerr*.txt`,
and personal data (`userDetails_seta.txt`, `userHistory.txt`) removed. The
resulting set is a **superset** of the strict dependency list windeployqt
reported (`Qt6Core Qt6Gui Qt6Multimedia Qt6Network Qt6Qml Qt6SerialPort
Qt6Widgets Qt6Xml`) and could be slimmed once windeployqt works.

Self-containment was verified by launching `Seta.exe` from the package with
**Qt removed from PATH** — it started with no missing-DLL or platform-plugin
error. (Same machine; a true clean-VM test is still pending.)

## 2. Signing points (do these in order, later)

1. Freeze the executable. **`Seta.exe` must not be recompiled or modified after
   the signing point** — signing binds to the exact bytes.
2. Sign `Seta.exe` with `signtool sign /fd SHA256 /tr <rfc3161-timestamp-url>
   /td SHA256 /a Seta.exe`. **Timestamping is required** so signatures stay
   valid after the certificate expires.
3. (Optional) sign the installer/zip if one is later produced.
4. Regenerate `SHA256SUMS.txt` and `release-manifest.json` **after** signing,
   because signing changes `Seta.exe`'s bytes and hash.

### Files that MUST be signed
- `Seta.exe` (and any future installer executable).

### Files that must NOT change after the signing point
- `Seta.exe` — no rebuild, no patch, no resource edit after it is signed.
- The bundled Qt/MinGW DLLs are already signed by their vendors; do not modify
  them. Re-run `SHA256SUMS.txt` only if the DLL set itself changes.

## 3. Certificate options

| Option | Use | Notes |
|---|---|---|
| **Internal test certificate** (self-signed, `New-SelfSignedCertificate`) | internal lab machines only | Must be installed in each machine's Trusted Publishers store; does NOT satisfy public SmartScreen. |
| **Public OV code-signing certificate** (RSA, from a CA) | external distribution | Cheaper; SmartScreen reputation must still accumulate over downloads/time. |
| **Public EV code-signing certificate** | external distribution, fastest trust | Hardware token; grants immediate SmartScreen reputation with most CAs. |

## 4. SmartScreen / Defender reality

- **An unsigned build WILL trigger SmartScreen** "unrecognized app" prompts and
  may draw Defender heuristic attention. Do not claim otherwise.
- Even a correctly **signed** OV build starts with **no reputation**; SmartScreen
  warnings can persist until enough clean downloads accumulate. EV certificates
  shortcut this.
- If Defender flags a false positive, submit the sample at
  https://www.microsoft.com/wdsi/filesubmission (Developer submission) with the
  publisher details and expect 1–3 business days.

## 5. Publisher / naming (to finalize before signing)
- Product name: **TechAim Electronic Target Scoring**
- Company/publisher: *TBD — must match the code-signing certificate subject.*
- Version scheme: semantic (`0.9.0` RC). Do **not** advance to `1.0.0` until
  physical target validation, clean-machine deployment and all required
  discipline acceptances pass (see the F11 report).

## 6. Hardening already in place / to keep
- No script launchers, no self-modifying code, no process injection, no bundled
  developer tools in the package.
- Build identity (commit + timestamp) is embedded at compile time and shown in
  Settings ▸ About, giving a signed build a verifiable source origin.
