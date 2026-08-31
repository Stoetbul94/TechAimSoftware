# Tech Aim Android — v1.0 parity audit

Read-only audit, performed before any production change, on
`feature/android-tablet` @ `b60867c` (clean, local == remote).

**Conclusion up front: Android cannot be closed out in this round, and the
reason is not a coding backlog.** The single-target Android application has
**no target acquisition transport at all**, and which transport it should have
is an open hardware question that this repository cannot answer. Every item in
the close-out brief from §9 (USB/CH340) through §15, and the whole §38 physical
gate, describes work on a subsystem that does not exist and whose architecture
is undecided.

Two further blockers are recorded below. None of the three is created by this
audit; all three are measured.

---

## 1. State at the start of the round (§1)

| | |
|---|---|
| Worktree | `C:\Users\User\Downloads\TechAimSoftware-Android` |
| Branch | `feature/android-tablet` |
| HEAD | `b60867c` |
| Worktree clean? | **YES** — 0 changed files |
| Local == remote? | **YES** — `origin/feature/android-tablet` = `b60867c` |
| Android version before this round | **0.9.0-ANDROID-A2.5** (`android: APP_VERSION_STR` in `Seta.pro`) |
| Desktop version on this branch | `0.9.0-RC3a-SETA` — stale, and not Android-facing |
| Existing APKs | `dist/TechAim-Android-A1/` — arm64-v8a 37.99 MB, x86_64 39.53 MB (**A1**, older than the A2.5 source) |
| Last Android development commit | `c29470e` (2026-08-20) — `b60867c` is a merge that promoted RMS node telemetry |

Android carries **15 commits** of its own and is **58 commits behind** the Tech
Aim 1.0.0 reference. Merge base: `f4058fa`, which predates the entire Tech Aim
v1.0 correctness programme.

---

## 2. How complete Android already was (§3)

Genuinely delivered by the A1/A2.5 work, and **not** to be rebuilt:

| Component | State |
|---|---|
| Android build scope, `Seta.pro` android block | **ALREADY CURRENT** |
| `AndroidManifest.xml` — package `za.co.techaim.target`, landscape, minimal permissions | **ALREADY CURRENT** |
| Platform boundary seam (startup, storage, audio, export) | **ALREADY CURRENT** |
| `config.ini` via the seam — was a boot blocker, fixed | **ALREADY CURRENT** |
| Session/journal storage on `AppLocalDataLocation` | **ALREADY CURRENT** |
| Landscape touch-safe application shell | **ALREADY CURRENT** |
| QtCore-only platform-boundary test gates | **ALREADY CURRENT** |
| Repeatable APK build + environment-check scripts | **ALREADY CURRENT** |
| A1 APKs, first-boot emulator evidence | **ALREADY CURRENT** |
| Architecture + transport-options records | **ALREADY CURRENT** |
| **Target acquisition (any transport)** | **MISSING — blocker B1** |
| Shared correctness core | **STALE — blocker B2** |
| Qualification / 3P competition clock under backgrounding | **ANDROID-SPECIFIC, unsafe — blocker B3** |
| `.tch` and `USER_DETAILS` paths | **PARTIAL** — `config.ini` went through the seam; these did not |
| Support-evidence export from a tablet | **MISSING** — no Android path exists |

**The prior work was honest about its own limits.** `AndroidManifest.xml`
states in its own comments that there is no USB host feature and no device
intent-filter, and the architecture record lists *"Android USB target
acquisition — NOT IMPLEMENTED"* and *"Physical target of any kind — NOT
TESTED"*. This audit confirms both in code rather than taking them on trust.

---

## 3. Architecture as it stands (§4)

| | |
|---|---|
| Qt | 6.5.3; Android kits `android_arm64_v8a` and `android_x86_64` both present |
| Android SDK | `%LOCALAPPDATA%\Android\Sdk` |
| NDK | 25.1.8937393 |
| JDK | 21.0.2 LTS |
| `adb` | present; one emulator visible, **offline**; **no physical device connected** |
| Package ID | `za.co.techaim.target` |
| Orientation | `sensorLandscape` |
| Permissions | exactly two — `INTERNET`, `ACCESS_NETWORK_STATE`. No storage, no USB |
| Java / Kotlin | **none exists in the repository** |
| JNI | **none** |
| Storage | `QStandardPaths::AppLocalDataLocation`, app-private |
| Signing | debug only; production signing listed as not delivered |

**The toolchain is complete and an APK has been built before.** Building is not
the problem.

---

## 4. Blocker B1 — no target acquisition transport (§9–§15, §38)

Verified in code, not inferred:

- **No Java or Kotlin source exists anywhere in the repository**, so there is
  no `UsbManager`, no `UsbDevice` enumeration, no permission `BroadcastReceiver`
  and no CH340 driver.
- `AndroidManifest.xml` declares no USB host feature and no device
  intent-filter, by explicit decision recorded in its own comments.
- The only serial code is the shared desktop `SerialDeviceProvider`, which
  compiles for Android but, as the transport record states, will find no usable
  device: `QSerialPortInfo::availablePorts()` returns nothing on an unrooted
  tablet and opening a `/dev/tty*` node fails.

Consequently the following brief sections have **no subject**:

| § | Topic | Status |
|---|---|---|
| 9 | USB discovery, permission, CH340, VID/PID | **NOT IMPLEMENTED** |
| 10 | Serial parameters 19200/Even/8/1 | **NOT APPLICABLE** — no Android serial layer to configure |
| 11 | USB permission state machine | **NOT IMPLEMENTED** |
| 12 | Reconnect | **NOT IMPLEMENTED** |
| 13 | Acquisition authority | shared authority exists; **no Android adapter feeds it** |
| 14 | Paper feed on accepted shot | shared logic present; **unreachable without acquisition** |
| 38 | Small physical hardware gate | **CANNOT BE RUN** |

### The decision that is actually blocking

The transport record sets out two options and does **not** choose between them,
because the choice is not the software's to make:

- **Option A — Modbus TCP.** Already written; same `libmodbus` context, same
  acquisition path, same scoring. Testable today without a tablet. Its entire
  cost is hardware: the installed targets must be reachable over TCP.
- **Option B — Android USB Host + a JNI CH340 driver.** No hardware change, the
  existing target and CH340 cable work unchanged. Requires new native code in
  the most correctness-critical place in the product, and a permanent
  maintenance burden.

The recorded recommendation is **prove Option A first**, explicitly conditional
on the customer's installed base being able to take a TCP path at acceptable
cost. **That question is open**, and the two options produce completely
different work: Option A is configuration and networking; Option B is a new
native subsystem.

**This is why the round stops here rather than guessing.** Building a CH340 JNI
driver that the hardware decision then makes redundant would be the most
expensive possible wrong answer.

---

## 5. Blocker B2 — the shared core is 58 commits stale (§2, §6, §7)

Measured by searching for each fix's own marker in both trees:

| Fix | Android | Tech Aim 1.0.0 | Status |
|---|---|---|---|
| ACQ-FLUSH-001 | 0 | 4 | **MISSING** |
| ACQ-DESYNC-002 | 0 | 4 | **MISSING** |
| ACQ-SENTINEL-003 | 0 | 9 | **MISSING** |
| ACQ-READ-004 | 0 | 2 | **MISSING** |
| SERIAL-DEFAULT-005 | 0 | 1 | **MISSING** |
| THREAD-MODBUS-006 | 0 | 2 | **MISSING** |
| QML-SHOT-001 | 1 | 1 | **ALREADY PRESENT** |
| PAPER-FEED-002 | 2 | 2 | **ALREADY PRESENT** |
| UI-STATUS-001 | 0 | 3 | **MISSING** |
| FINALS-TIMER-001 | 0 | 2 | **MISSING** |
| FINALS-DISPLAY-TIMER-002 | 0 | 2 | **MISSING** |
| FINAL-TCH-TIME-001 | 0 | 1 | **MISSING** |
| FINALS-TCH-SIGHTER-001 | 0 | 2 | **MISSING** |
| FINALS-3P-MIX-001 | 0 | 2 | **MISSING** |
| FINALS3P-FLOW-001 | 0 | 1 | **MISSING** |
| UI-LASTSHOT-DWELL-001 | 0 | 1 | **MISSING** |
| CRO-ORDER-001 | 0 | 1 | **MISSING** |
| CRO-REPEAT-002 | 0 | 1 | **MISSING** |

**16 of 18 are absent.** This is the direct consequence of the merge base
predating the Tech Aim v1.0 programme: the acquisition hardening, the finals
work, CRO sequencing and the last-shot dwell all landed after Android branched.

This convergence is required under **either** transport option, so it is the
one piece of Android work that can proceed without the hardware decision. It is
comparable in scale to the SETA Phase B convergence.

---

## 6. Blocker B3 — the competition clock is a UI tick count (§25)

`CenterPane.qml`:

```qml
Timer { id: gameTimer; interval: 1000; repeat: true
        onTriggered: { gameTime++; ... } }
```

Competition elapsed time for Open Practice, Qualification, 50 m Prone and 50 m
3P Qualification is **counted in one-second UI ticks**. On Windows that is
acceptable — the application owns a dedicated lane PC and stays in the
foreground. On Android, Qt suspends timers when the activity is backgrounded,
so a competition clock **stops accruing** while the athlete's tablet is behind
a USB permission dialog, a notification shade, or a screen-off. The athlete
silently gains competition time.

§25 of the brief instructs that timing which relies only on a UI tick count is
to be classified as a release blocker. **It is.**

Not affected: `Finals3PController` already uses one monotonic `QElapsedTimer`
and recomputes `remaining = duration - monotonicElapsed`, never a per-second
decrement. The 3P Final is safe by construction; everything on the legacy
`gameTimer` is not.

**This is shared code.** Tech Aim Windows 1.0.0 is frozen and must not be
changed for this; the fix belongs behind the platform seam, on Android.

---

## 7. Tech Aim identity on Android (§5, §31)

Clean, and worth stating because the brief asks:

| Check | Result |
|---|---|
| `BRAND_SETA` references | **0** |
| DSB sources (`src/dsb`) | **0** |
| German catalogue | **absent** |
| SETA storage namespace | **absent** |
| Package ID | `za.co.techaim.target` — Tech Aim |
| Teiler in QML | present in 4 files — **inherited shared code, not the SETA policy**; whether Android shows it follows `BrandPackage`, unchanged here |

The desktop `APP_VERSION_STR` on this branch is `0.9.0-RC3a-SETA`, which is
stale, but the android block overrides it and no Android artefact carries it.

---

## 8. Storage (§23) — partly done, honestly

`config.ini` was the Android boot blocker and was fixed through the platform
seam: on Android the working directory is `/`, which is not writable, so the
application could neither read nor persist configuration. Session journals,
reports and logs already resolve through `AppLocalDataLocation`.

**Still relative, and therefore still wrong on Android:**

- `appsettings.cpp` `saveMatch()` — `QFile("Match_<stamp>.tch")`, relative to
  the working directory
- `tachuswidget.cpp` `saveNameAndPort()` — `applicationDirPath()/USER_DETAILS`,
  which on Android is the native-library directory

These are the same two sites that forced the Tech Aim and SETA installers to
install per-user rather than into Program Files. On Android they do not have a
per-user workaround: the paths are simply unwritable. Both must go through the
seam before a session can be saved on a tablet.

---

## 9. Support evidence from a tablet (§32)

`Collect-Logs.cmd` is a Windows batch file and is not applicable. **No Android
support-export path exists** — no in-app bundle, no share intent, no
content-URI export.

Severity: **high for a physical evaluation, not a blocker for the automated
phase.** The §38 gate asks a tester to fire shots and report what happened; with
no export path, the only route to the journals is `adb`, which the brief itself
says should not be required of an ordinary customer.

---

## 10. What this round did NOT do, and why

No production code was changed. Specifically **not** attempted:

- A CH340 JNI driver — the transport decision is open (B1); building it first
  risks a wholly wasted native subsystem.
- The 58-commit shared-core convergence — needed under either option, but large
  enough to be its own round, and it cannot be physically validated while B1
  stands.
- Any change to Tech Aim Windows, SETA, RMS or NodeTelemetry.

---

## 11. What is needed to unblock, in order

1. **Answer the transport question** — can the installed targets be reached over
   TCP at acceptable cost? Option A if yes; Option B (JNI CH340) if no. Nothing
   physical can proceed until this is answered.
2. **Converge the shared core** onto Tech Aim 1.0.0 — required either way,
   restores the 16 missing correctness fixes.
3. **Move the competition clock behind the seam** onto a monotonic source for
   Android (B3).
4. **Route `.tch` and `USER_DETAILS` through the platform seam** (§8 above).
5. **Add an Android support-export path** so a tester can return evidence
   without `adb`.
6. Then build the evaluation APK and run the small §38 physical gate.

Steps 2–5 are ordinary engineering and can start immediately. Step 1 is a
decision, and it is the one that governs whether step 6 is weeks or days away.
