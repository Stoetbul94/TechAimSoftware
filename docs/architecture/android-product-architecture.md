# Tech Aim Android tablet — product architecture

**Branch:** `feature/android-tablet`
**Foundation:** `feature/rc2e-latency-and-reset` @ `f4058fa`
**Status:** milestone **A1/A2** — Android product branch and first bootable APK.

This document defines the platform boundary for the Android tablet product. It
is an architecture document, not a feature list. Sections marked
**NOT IMPLEMENTED** are deliberate scope exclusions for A1/A2.

---

## 1. The rule

**There is one core. Platforms differ only at named seams.**

The Android product is not a fork. It compiles the same scoring, session,
recovery, qualification, finals, training and analytics code as Windows. Any
Android behaviour that differs from Windows must sit behind an explicit,
named seam — never behind a scattered `#ifdef` in domain logic, and never
behind an `if (android)` in QML.

A change that would require copying a domain file into an Android-specific
version is a design failure and must be rejected.

---

## 2. Shared core — identical on both platforms

These are compiled from the same sources, with no platform branching. 118 files
under `src/`, of which only a handful touch GUI at all.

| Area | Location | Notes |
|---|---|---|
| **Scoring** | `CenterPane.qml::calculateShootingSocre()` | Single authority. Never duplicated, never platform-branched. |
| Target geometry / projectile | `AppSettings::projectileDiameterMm`, `src/training/TargetGeometry.h` | SCORING-CAL-001 |
| Session storage | `src/reliability/store/`, `journal/`, `events/` | QtCore-only |
| Reducer / replay / recovery | `src/reliability/reducer/`, `replay/`, `recovery/` | QtCore-only |
| Hash chain | `src/reliability/journal/HashChain.cpp` | QtCore-only |
| Qualification | `src/qualification/` | |
| Finals (3P, 10 m) | `src/finals/`, `src/finals10m/` | |
| Training Lab | `src/training/` | |
| Coach analytics | `src/analytics/` | Frozen, Qt-free |
| EST incidents | `src/incident/` | |
| Operating mode | `src/mode/` | |
| Competition catalogue | `CompetitionCatalogue.qml` | |
| **Target device selection** | `src/target/TargetDeviceFingerprint.*`, `PaperFeedCoordinator.*` | Ranking/selection logic is transport-agnostic — see §4 |

`StorageSync` (`src/reliability/storage/StorageSync.cpp`) already carried a
`Q_OS_UNIX` `fsync()` branch before this branch existed; Android uses it
unchanged. `StoragePaths` already resolves through
`QStandardPaths::AppLocalDataLocation` and needed no change.

---

## 3. Windows shell

Behaviour that is correct on Windows and must be preserved there.

| Concern | Implementation | Android equivalent |
|---|---|---|
| Serial/RTU transport | `libmodbus` RTU → `CreateFileA("COMxx:")` | None. See `android-target-transport-options.md`. |
| Serial enumeration | `QtSerialDeviceProvider` (`QSerialPortInfo`) | Null provider — §4 |
| Desktop window chrome | `Qt.FramelessWindowHint`, custom min/max/close, `visibility: "Maximized"` | Suppressed — full content area, landscape |
| Single instance | `QLockFile` in temp + legacy lock | Not applicable — §5 |
| Operating-mode restart | `QProcess::startDetached(applicationFilePath())` | Not available — §5 |
| Pre-QML startup dialogs | `QDialog` + stylesheet cards in `main.cpp` | QML-native surface — §5 |
| Export / file picking | `QFileDialog::getSaveFileName` | App-owned export dir — §6 |
| Printing | `QT += printsupport` (no `QPrinter` is ever constructed) | Omitted from the Android build |

---

## 4. Target transport seam

**The generic core owns the target *connection interface*. It does not own
Windows FTDI enumeration.**

The existing `ta::target::ISerialDeviceProvider`
(`src/target/SerialDeviceProvider.h`) is already the correct seam and is
**not rewritten**. It has two production-relevant implementations:

```
ISerialDeviceProvider                     (existing, unchanged)
├── QtSerialDeviceProvider                Windows — QSerialPortInfo
├── FixedSerialDeviceProvider             tests — deterministic list
└── NullSerialDeviceProvider              Android — always empty, and says why
```

Everything layered on top of it is preserved verbatim on both platforms:

- `TargetDeviceFingerprint` — VID/PID, manufacturer/description matching,
  Bluetooth and virtual-port rejection, fingerprint strength
- candidate scoring and selection
- the remembered-target concept
- reconnect behaviour

On Android the provider returns an empty list, so selection correctly concludes
"no confident device" and does **not** speculatively connect — which is exactly
the behaviour the Windows code was already fixed to have.

Future transports attach at the same boundary and are described in
`android-target-transport-options.md`:

- `ModbusTcpTransport` — exists today in `ModReader/src/modbusadapter.cpp`
- `AndroidUsbRtuTransport` — **NOT IMPLEMENTED**, deliberately out of scope

---

## 5. Android shell — lifecycle, startup, storage

### Startup surface

`main.cpp` shows two `QDialog`s before the QML engine exists. On Android these
are replaced by a platform-safe path; the Windows safeguards are **not removed**.

### Single instance

Android guarantees a single task instance per package by design, and there is
no second process to collide with. `QLockFile` acquisition, the legacy-lock
migration and the "Already Running" dialog are **disabled on Android** and
remain fully active on Windows.

### Restart / relaunch

`QProcess::startDetached(applicationFilePath())` cannot relaunch an Android
app. Operating-mode changes on Android therefore surface an explicit
"restart the application" instruction instead of self-relaunching. No Java
relaunch hack is introduced to emulate desktop behaviour.

### Storage

All persistent data resolves through `QStandardPaths::AppLocalDataLocation`,
which on Android is app-private (`/data/data/<package>/files`) and requires no
storage permission. `config.ini` moves off the working directory — the working
directory on Android is `/`, which is not writable.

Consequence to be aware of: app-private storage is **removed on uninstall**,
and is not reachable by the operator through a file manager. The support-bundle
and "read the journal" workflows need an Android answer later; they are not
solved in A1/A2.

### Lifecycle

Android can pause, background or kill the process without warning, and there is
no `closeEvent`. `QGuiApplication::applicationStateChanged` drives a flush of
the active session through the **existing** `SessionStore` / `JournalWriter` /
`StorageSync` path. No new recovery system is introduced, and repeated
notifications must be safe and idempotent.

The append-only journal + hash chain + replay design already models
unannounced termination, so Android exercises an existing capability rather
than requiring a new one.

---

## 6. Android UI shell

Target device for A1/A2: **10–13" Android tablet, landscape**. Phones and
portrait shooting are explicitly out of scope.

| Concern | A1/A2 position |
|---|---|
| Orientation | Landscape preferred/locked; the three-pane shooting UI is designed around it |
| Window chrome | Desktop frameless chrome and custom min/max/close suppressed |
| Touch targets | Obvious blockers fixed (tiny hit areas, pointer-precision grips). No wholesale redesign. |
| Hover | 45 `hoverEnabled` sites audited; no action may be reachable *only* via hover. Desktop hover effects preserved. |
| Floating windows | `FloatingWindow` is a QML `Item`, not a top-level `Window` — it ports. Must open/close safely; full redesign deferred. |
| Virtual keyboard | Text fields must not be permanently covered; dismissal must work |
| Fonts | Hardcoded `Segoe UI` / `Consolas` replaced by cross-platform tokens. **No Microsoft font files are added to the repository.** |
| PDF | `QPdfWriter` retained; must not crash. Export UI marked pending where not Android-ready. |

---

## 7. What A1/A2 does not deliver

- **Android USB target acquisition** — NOT IMPLEMENTED
- **Physical target of any kind** — NOT TESTED
- RMS node telemetry — not present in the shared foundation (see
  `three-product-architecture.md`); optional and not required for A1/A2
- Portrait layout, phone form factors
- Storage Access Framework export
- System printing
- Production signing

---

## 7.1 RMS node telemetry — not in the foundation, and what promotion needs

**Finding: the shared foundation does NOT contain RMS node telemetry.**
Verified against `feature/rc2e-latency-and-reset` @ `f4058fa`: there is no
`src/telemetry/`, no `src/rms/`, and no `Telemetry.pri`. A1/A2 therefore
excludes RMS telemetry, exactly as the milestone allows — the bootable APK
comes first.

The work exists on `feature/rms-node-telemetry` (4 commits ahead of the
foundation). Note that `2fa7407`'s subject line says "promote the RMS wire
contract to the shared foundation", but it has **not** been merged into the
foundation — the commit message describes intent, not state. Nothing was
cherry-picked for this milestone.

If it is promoted later, this is the exact surface:

| Kind | Path |
|---|---|
| New | `src/rms/RmsProtocol.{h,cpp}` |
| New | `src/telemetry/ITelemetrySink.h`, `NodeIdentity.{h,cpp}`, `NodeTelemetryService.{h,cpp}`, `UdpTelemetrySink.{h,cpp}` |
| New | `Telemetry.pri`, `tests/telemetry/`, `tools/rmsnode/`, `tests/reliability/tst_node_telemetry.cpp` |
| Modified | `Seta.pro`, `main.cpp`, `ShootingPage.qml`, `CLAUDE.md`, `.gitignore` |

Commits, oldest first: `2fa7407`, `4a658b3`, `7797ed4`, `8d4d87e`.

**Android suitability, if promoted (§25/§26).** The transport is favourable:

- `UdpTelemetrySink` is **send-only and fire-and-forget** — one datagram per
  message, broadcast to the observation port, and the socket is *never bound
  for reading*.
- Therefore **no Android `MulticastLock` is required.** A MulticastLock is
  needed to *receive* broadcast/multicast frames that the Wi-Fi chip would
  otherwise filter. It has no bearing on transmission. A Java `WifiManager`
  bridge must **not** be added for node → RMS traffic.
- A MulticastLock would only become relevant if the direction were reversed —
  an RMS → node discovery or command channel. Protocol v1 has none, and the
  node's legacy inbound port is untouched.
- `INTERNET` permission is already declared in the manifest, which is all a
  UDP send needs.

The remaining Android caveat is lifecycle, not transport: background delivery
must not be relied upon. The target application is expected to stay in the
foreground for the duration of a match, which is consistent with the
landscape, full-screen, single-activity shell defined in §6.

## 8. Relationship to the three-product architecture

This is a **fourth shell on the shared foundation**, not a fourth product line.
`docs/architecture/three-product-architecture.md` describes Tech Aim, SETA/OEM
and RMS; the Android tablet is the Tech Aim single-target product compiled for
a different platform. It must never acquire SETA branding, the DSB catalogue or
a SETA AppData namespace — the first Android line is generic Tech Aim.
