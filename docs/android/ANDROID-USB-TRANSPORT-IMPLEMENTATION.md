# Android USB + CH340 transport — implementation record

Phase C3. Follows `ANDROID-TARGET-TRANSPORT-DECISION.md`, which chose USB Host
+ CH340 over Modbus TCP on hardware evidence.

**§4 required that every reasonable existing path be inspected before any chip
driver was hand-written.** All four were. The findings below are the reason the
implementation looks the way it does.

---

## 1. Path A — can Qt SerialPort drive a CH340 through Android USB host?

**NO.** Inspected in the installed kit, not assumed:

| Check | Result |
|---|---|
| `libQt6SerialPort_arm64-v8a.so` ships in the Android kit | yes |
| Android USB references inside it (`UsbManager`, `UsbDevice`, `openDevice`, `android/hardware/usb`) | **0** |
| `/dev/tty*`, `ttyUSB`, `ttyACM`, `/sys/class` strings | **none** |
| Qt's Android jars (`Qt6Android.jar` and siblings) containing USB classes | **0** |

The library links and compiles for Android, which is exactly the trap the
existing transport record warned about: *"`libQt6SerialPort_arm64-v8a.so` exists
in the Qt Android kit … Neither fact means a USB target works."* It has no
Android USB backend at all, and an unprivileged app cannot open a `/dev/tty*`
node. **Path A is closed by inspection.**

## 2. Path B — does an integration already exist in repository history?

**NO.** `git log --all --diff-filter=A -- '*.java' '*.kt'` returns nothing: no
Java or Kotlin has ever existed on any branch. No vendored USB-serial library,
no JNI, no CH340 code.

## 3. Path C — a mature Android USB serial library

**YES, and it is the chosen path.** Verified from the published POM rather than
from memory:

| | |
|---|---|
| Library | `com.github.mik3y:usb-serial-for-android` |
| Version | **3.7.0** |
| Packaging | `aar` |
| **License** | **MIT License** (declared in the POM, `distribution: repo`) |
| Source | <https://github.com/mik3y/usb-serial-for-android>, inception 2013 |
| Available from | JitPack (`com.github.mik3y`), fetch verified from this machine |
| Dependencies | one — `androidx.annotation:1.5.0` |
| Redistribution in commercial Tech Aim | **permitted** — MIT allows commercial use and redistribution with attribution |
| What it replaces | the CH340 line-coding control transfers and the USB bulk read/write loop — **nothing above the byte stream** |

MIT is the cleanest possible answer to §4's licensing requirement: no copyleft,
no per-distribution obligation beyond retaining the notice.

## 4. Path D — a hand-written CH340 driver

**Not needed, and not written.** §4 asks for this only after A–C fail; C
succeeds. Writing our own CH340 control transfers would put new, untestable
native code in the most correctness-critical place in the product, to
re-implement something an MIT-licensed, twelve-year-old library already does.

---

## 5. The integration boundary — and why there is no second Modbus

The hard question §15 asks is whether the existing Modbus authority can consume
an Android USB byte stream, because a `UsbDeviceConnection` is not a file
descriptor and `modbus_new_rtu()` opens a device path.

**It can, and cleanly.** The vendored libmodbus is built around a backend
vtable, `modbus_backend_t` in `modbus-private.h`:

```c
typedef struct _modbus_backend {
    unsigned int backend_type, header_length, checksum_length, max_adu_length;
    int  (*set_slave)(...);
    int  (*build_request_basis)(...);
    int  (*build_response_basis)(...);
    int  (*prepare_response_tid)(...);
    int  (*send_msg_pre)(...);
    ssize_t (*send)(modbus_t*, const uint8_t*, int);
    int  (*receive)(modbus_t*, uint8_t*);
    ssize_t (*recv)(modbus_t*, uint8_t*, int);
    int  (*check_integrity)(...);
    int  (*pre_check_confirmation)(...);
    int  (*connect)(modbus_t*);
    void (*close)(modbus_t*);
    int  (*flush)(modbus_t*);
    int  (*select)(...);
    void (*free)(modbus_t*);
} modbus_backend_t;
```

Everything that defines **the protocol** — request framing, response framing,
CRC (`check_integrity`), slave addressing — belongs to the RTU backend and is
reused verbatim. Only the six members that move **bytes** are replaced:
`connect`, `close`, `flush`, `send`, `recv`, `select`.

That is the whole adapter. There is no second Modbus implementation, no
re-implemented CRC, and no framing in Java. A framing bug cannot be introduced
by this work because no framing code is written by this work.

---

## 6. Architecture

```
Android UsbManager  ──►  usb-serial-for-android (CH340 line coding, bulk I/O)
                                    │  Java
                          ── JNI bridge ──
                                    │  C++
                    AndroidUsbTransport   (state machine, byte queue)
                                    │
              libmodbus Android USB backend (send/recv/connect/close/flush/select)
                                    │
        libmodbus RTU framing + CRC  ── UNCHANGED, reused
                                    │
          existing hardened acquisition authority  ── UNCHANGED
                                    │
        accepted shot ──► competition / session / reporting
```

The transport answers only: is a device present, is permission granted, is the
port open, did these bytes move, has the connection dropped. It decides nothing
about coordinate validity, shot acceptance, scoring, shot role, counter
reconciliation, duplicate suppression or competition state.

---

## 7. Serial configuration

Field-authoritative, from the Windows physical qualification and the vendor's
own log line `portname COM4 19200 Even 8 1 Disable 1`:

| Parameter | Value |
|---|---|
| Baud | **19200** |
| Parity | **EVEN** |
| Data bits | **8** |
| Stop bits | **1** |
| RTS | **DISABLED** |

**DTR is not specified by that evidence and is therefore not invented here.**
The Windows log records RTS as `Disable` and says nothing about DTR. See the
open question in §9 below.

---

## 8. CH340 identification (§8)

**The VID/PID of the actual field device could not be read, because no Android
tablet and no CH340 are connected to this machine** — `adb devices` shows one
emulator, offline. §8 asks for the VID/PID *from the intended physical device*
and that evidence does not exist here.

What the repository does establish is the device's identity string:
`USB-SERIAL CH340`, selected on COM4 during the physical qualification. The
implementation therefore matches on the CH340 vendor/product pairs the chosen
library already recognises for that chip family, defined in **one place** so a
field device that reports a different pair is a one-line addition rather than a
hunt.

**It does not accept an arbitrary USB serial device.** A device is a candidate
only if it matches a known CH340 pair; exposing bulk endpoints is not
sufficient, exactly as §8 requires.

**Open item for the physical gate:** record the VID/PID the real target
reports, and confirm it is in the list.

## 9. DTR — deliberately not invented (§14)

§14 says to verify DTR from field evidence and not to invent semantics. The
evidence is that **there is none to invent from**:

- `DTR` appears nowhere in `modbuscommsettings.cpp`, `modbusadapter.cpp`, or
  the vendored libmodbus RTU backend.
- Only RTS is configured, defaulting to `Disable`.

So the Windows behaviour is not "DTR off" — it is **"the application never
touches DTR"**, leaving whatever the Windows CH340 driver defaults to.

That is not automatically reproducible on Android, and the difference is worth
stating because it is the kind that bites in the field: **some Android CH340
drivers assert DTR and RTS on open**, and on boards that wire DTR to a reset
line an asserted DTR can reset the target at connect time. The implementation
therefore sets **RTS false explicitly** (matching the field setting) and
**leaves DTR at the library default**, because inventing a value would be
guessing at hardware this repository has never observed.

**Physical-gate item:** on first connection, confirm the target does not reset
when the tablet opens the port. If it does, DTR is the first thing to test.

## 10. What is NOT verified by this round

Stated plainly because the distinction decides whether anyone should trust it:

- **No Android device is connected to this machine.** The Java and JNI layers
  have therefore **never executed**. They compile into the APK — which proves
  syntax and API usage against the real SDK — and nothing more.
- **No CH340 has been opened from Android.** Every USB state transition below
  the JNI boundary is unexercised.
- **No target has been contacted over this transport.**
- The C++ transport state machine, the backend seam and the configuration
  values **are** covered by deterministic tests, because §28 requires those to
  run without hardware.

The first real execution of the USB path will be on the tester's tablet. That
is the purpose of the physical gate, and no part of this document should be
read as evidence that it works.
