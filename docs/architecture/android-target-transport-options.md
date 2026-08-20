# Android target transport — options and recommendation

**Status:** decision document for milestone **A3**. Written during **A1/A2**
(Android product branch + first bootable APK) on `feature/android-tablet`.

**Scope.** How a Tech Aim Android tablet acquires shots from a physical
electronic target. Nothing in this document is implemented on
`feature/android-tablet`. The first APK is **DEMO / NO-TARGET** only.

**This document does not make the business decision.** The choice depends on
customer hardware facts that are not established in this repository — see
§6 *Open questions that must be answered by the customer*.

---

## 1. Why the Windows transport does not carry over

The Windows product acquires over **Modbus RTU on a USB serial adapter**
(CH340 in the field, per `docs/release/0.9.0-physical-qualification-2026-08-09.md`).
That path has three layers, and only the top one survives on Android:

| Layer | Windows | Android |
|---|---|---|
| Device enumeration | `QSerialPortInfo::availablePorts()` → `src/target/SerialDeviceProvider.cpp` | Returns nothing usable. Android exposes USB through the Java `UsbManager` API, not `/dev/tty*` nodes. |
| Device selection | `src/target/TargetDeviceFingerprint.cpp` — VID/PID, description, Bluetooth/virtual rejection, scoring, remembered device | **Portable logic.** It ranks `SerialDeviceInfo` structs and does not care where they came from. |
| Byte transport | `modbus_new_rtu()` → `CreateFileA("COMxx:")` on Windows, `termios` + `open("/dev/tty…")` elsewhere | The POSIX branch **compiles** on Android (bionic has `termios.h`) and then fails at `open()`. An unprivileged Android app cannot open a USB-serial device node. |

`libQt6SerialPort_arm64-v8a.so` ships in the Qt 6.5.3 Android kit, so the code
**links**. Linking is not support. See §5.

---

## 2. Option A — Modbus TCP

### What already exists

The TCP path is **present and reachable in the current foundation**, not
hypothetical:

- `ModbusAdapter::modbusConnectTCP()` calls `modbus_new_tcp(ip, port)`
  (`ModReader/src/modbusadapter.cpp:114`). `modbus-tcp.c` is plain BSD
  sockets — fully portable, no `_WIN32` branch in the connect path.
- Mode selection: `MainWindow::isModbusTcpMode()`
  (`ModReader/src/mainwindow.cpp:380`) returns
  `ui->cmbModbusMode->currentIndex() != EUtils::RTU`.
- That combo is initialised from `Session/ModBusMode` in **`qModMaster.ini`**
  (`0` = RTU, non-zero = TCP), loaded by
  `ModbusCommSettings::loadSettings()` (`modbuscommsettings.cpp:332`).
- Host comes from `ModbusCommSettings::slaveIP()`, port from `TCPPort()`,
  timeout from `Var/TimeOut` (default `1` second — `0` means wait-forever and
  freezes the app).
- `TachusWidget::connectToTarget()` already **skips the serial selector**
  in TCP mode and logs `modbus TCP mode - serial selector skipped`. This was
  fixed deliberately while bringing up the target emulator.

### What is transport-independent downstream

Everything. Acquisition reads holding registers; the shot coordinates reach
`CenterPane.qml::calculateShootingSocre()` through `TachusWidget` with no
knowledge of how the bytes arrived. Scoring, `SessionStore`, replay, recovery,
finals and training are untouched by the transport choice.

### Assessment

| Dimension | Assessment |
|---|---|
| Implementation effort | **Low.** The code path exists and is exercised on Windows via the emulator. Android work is a settings surface (host/port) and removing the desktop-only `qModMaster.ini` dependency. Days. |
| Hardware change | **Required.** The target must speak Modbus TCP, or a serial↔TCP gateway must sit on the lane. This is the whole cost of Option A, and it is a customer/hardware cost, not a software one. |
| Latency | Adds network stack + Wi-Fi. A wired gateway is predictable; Wi-Fi introduces jitter that has **not been characterised for this product** and matters because shot timing is recorded. Must be measured before adoption. |
| Reliability | TCP retransmits and hides brief loss, but Wi-Fi association loss is a hard stall. Existing reconnect logic applies. |
| Hotplug | Not applicable — no cable to the tablet. A gateway reboot looks like a connection drop, which the existing reconnect path already models. |
| Permission model | `INTERNET` only. No runtime permission dialog, no user interaction. |
| Deployment complexity | **Moves complexity onto the range.** Every lane needs an addressable gateway and a stable IP/DHCP plan. |
| Maintenance burden | **Low in software.** One transport implementation shared with the existing Windows TCP path. |

---

## 3. Option B — Android USB Host + JNI serial driver

### What it would require

1. `UsbManager` enumeration through JNI, plus an `intent-filter` /
   `requestPermission()` flow — the user physically grants access to the
   device, per connection, in a system dialog.
2. A userspace bridge driver for the actual chip. CH340 is what the field
   target presents; CP210x, FT232 and PL2303 are the plausible OEM
   alternatives. Either `usb-serial-for-android` (Apache-2.0) or a
   hand-written driver.
3. **libmodbus RTU cannot be reused as-is.** `modbus_new_rtu()` owns the file
   descriptor and does its own `read`/`write` and timing. Bytes arriving from
   a Java `UsbDeviceConnection` are not an fd. Either the RTU framing and CRC
   are reimplemented over the Java byte stream, or libmodbus is refactored
   onto a pluggable byte-sink. Both are real surgery on the most
   correctness-sensitive transport code in the product.
4. RTU is timing-sensitive (inter-frame silence). Android gives no scheduling
   guarantees, and the JNI hop adds latency that is not bounded.

### Assessment

| Dimension | Assessment |
|---|---|
| Implementation effort | **High.** New JNI layer, a chip driver, an RTU framing path that does not exist today, plus a permission UX. Weeks, and it lands in the highest-risk part of the stack. |
| Hardware change | **None.** This is its single decisive advantage — the existing target and CH340 cable work unchanged. |
| Latency | USB is inherently lower-latency than Wi-Fi, but the JNI boundary and Android scheduling erode the advantage. Unmeasured either way. |
| Reliability | Depends on driver quality. Bugs here corrupt or drop shot data — the worst possible failure domain. |
| Hotplug | Must be handled explicitly: detach intents, permission revocation, re-grant on reconnect. The tablet also has to power the adapter (OTG), which raises a charging question during a long match. |
| Permission model | Runtime USB permission dialog per device/connection. An athlete or official has to tap through it. |
| Deployment complexity | **Low on the range.** Plug the cable in. |
| Maintenance burden | **High.** Per-chip drivers, per-OEM quirks, and Android version drift on the USB stack. This becomes a permanent maintenance line. |

---

## 4. Recommendation

**Prove Option A (Modbus TCP) first**, and treat Option B as a fallback that is
only justified if the customer's hardware reality forbids A.

Reasons, in order of weight:

1. **A is already written.** It is the same `libmodbus` context, the same
   acquisition path, the same scoring. B requires new code in the one place
   where new code is most dangerous.
2. **A is testable now, without a tablet.** The TCP path can be exercised
   against the existing emulator on Windows, which means the Android risk
   reduces to "does the socket open", not "is our RTU framing correct".
3. **B's maintenance burden is permanent**; A's cost is a one-time hardware
   decision.
4. If A proves viable it also serves RMS, which is already a networked
   architecture.

**However — this recommendation is conditional.** Option A's entire cost sits
in hardware that this repository knows nothing about. If the customer's
installed base cannot be given a TCP path at acceptable cost, that fact alone
decides for Option B, regardless of the software argument above.

---

## 5. What must not be claimed

`libQt6SerialPort_arm64-v8a.so` exists in the Qt Android kit and
`src/target/SerialDeviceProvider.cpp` compiles for Android. **Neither fact
means a USB target works.** `QSerialPortInfo::availablePorts()` will return no
usable device on an unrooted tablet, and `open()` on a `/dev/tty*` node will
fail. Any status report that implies otherwise is wrong.

Current state on `feature/android-tablet`:

```
ANDROID USB TARGET:   NOT IMPLEMENTED
PHYSICAL TARGET:      NOT TESTED
```

---

## 6. Open questions that must be answered by the customer

These are hardware and commercial facts, not engineering choices. **They are
not answered in this repository and must not be guessed.**

1. Can the installed target hardware expose Modbus **TCP** at all, natively?
2. If not, is a serial↔Ethernet/Wi-Fi gateway per lane acceptable —
   commercially, physically, and to the range operator?
3. Is a wired network available at the firing point, or is this Wi-Fi only?
4. What shot-timestamp accuracy does the product actually have to guarantee?
   This decides whether Wi-Fi jitter is tolerable and is the one number that
   could rule Option A out on technical grounds.
5. Is the tablet expected to power the adapter over OTG for a full match, and
   can it charge simultaneously? (Option B only.)

Until (1)–(4) are answered, **A3 should be a measurement exercise, not an
implementation commitment.**

---

## 7. Relationship to the target-transport seam

Both options sit behind the same boundary, described in
`docs/architecture/android-product-architecture.md` §4. The existing
`ta::target::ISerialDeviceProvider` (`src/target/SerialDeviceProvider.h`) is
already the right shape for *device discovery* and must not be rewritten —
`TargetDeviceFingerprint`, the scoring/selection logic and the remembered-target
concept are transport-agnostic and are preserved under either option.

See also `docs/architecture/three-product-architecture.md` §4.1, which corrects
the earlier classification of serial/FTDI discovery as generic shared core.
