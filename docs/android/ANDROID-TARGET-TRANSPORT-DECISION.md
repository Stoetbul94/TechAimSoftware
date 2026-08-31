# Android target transport — decision from hardware evidence

Resolves blocker **B1** from `ANDROID-V1.0-PARITY-AUDIT.md`. This document
answers *what the hardware actually is*, not what the software could do.

**Decision: USB Host + CH340, carrying the existing Modbus RTU protocol.**
Modbus TCP is not available on the installed targets and cannot be made
available without adding hardware to every lane.

---

## 1. What the current target physically is (§3)

Evidence, all from field runs against real hardware in this repository:

| Source | Evidence |
|---|---|
| `docs/release/0.9.0-physical-qualification-2026-08-09.md` | *"Target: CH340 on COM4"* — a real 10 m Air Rifle qualification |
| same, line 68 | `candidate COM4 (USB-SERIAL CH340) score 1150 [remembered target]` |
| same, line 189 | *"Serial link is 19200 baud, Even parity, 8N1-E, RTS disabled"* |
| `docs/release/0.9.0-polling-architecture.md` | vendor's own log: `portname COM4 19200 Even 8 1 Disable 1` |
| `docs/release/0.9.0-rc2-hardware-audit.md` | `COM4 · USB-SERIAL CH340 · SELECTED`; transport is `modbus_new_rtu(...)` |

**Answer to §3: option A on that list — a USB-to-serial CH340 carrying Modbus
RTU.** Every physical qualification this product has ever passed was conducted
over that link. There is no field evidence of native RS-485 to the tablet, of
Ethernet on the target, or of Wi-Fi.

---

## 2. Software capability is not hardware capability (§4)

The application **can** speak Modbus TCP. `ModReader/src/modbusadapter.cpp:114`
calls `modbus_new_tcp(...)`, and the acquisition gates in
`docs/release/0.9.0-acquisition-gate-results.md` run over it.

**But every one of those TCP runs is against the emulator, not a target.** No
document in this repository records a target answering Modbus TCP, and none
records a target with an Ethernet or Wi-Fi interface.

The transport-options record already said so plainly, and this audit confirms it
rather than softening it:

> *"Hardware change **Required.** The target must speak Modbus TCP, or a
> serial↔TCP gateway must sit on the lane. This is the whole cost of Option A,
> and it is a customer/hardware cost, not a software one."*

`docs/project/Current_Project_State.md` still carries it as an **open customer
question** — whether the target can speak Modbus TCP and whether a per-lane
gateway is acceptable.

| §33 question | Answer |
|---|---|
| Current target physical interface | **USB-serial CH340 → Modbus RTU** |
| Modbus TCP natively available? | **NO** — no evidence of any target-side TCP; every TCP use here is against the emulator |
| Additional hardware required for TCP? | **YES** — target electronics with Ethernet, or a serial↔TCP gateway per lane |
| USB/CH340 current hardware path? | **YES** — field-proven |

**Modbus TCP is therefore not a "no new hardware" option, and this document
does not present it as one.** Calling it that because `libmodbus` supports TCP
would be exactly the inference §3 forbids.

---

## 3. Tablet side (§5)

The CH340 cable that works on Windows is an ordinary USB-A device. Reaching it
from a tablet needs **USB-C OTG host mode plus a USB-C-to-A adapter** — no
change to the target, no change to the cable.

**Not yet verified on the intended tablet**, and it must be before
implementation starts: not every Android tablet exposes USB host mode, and a
tablet that cannot host USB cannot use this transport at all. This is a
five-minute check on the actual device, and it is the one hardware fact this
decision still rests on.

---

## 4. Decision table (§6)

| Criterion | **USB Host + CH340** | **Modbus TCP** |
|---|---|---|
| Current target hardware change | **NONE** | **REQUIRED** — Ethernet on the target, or a gateway |
| Tablet hardware change | USB-C OTG adapter (cheap, per tablet) | none |
| New range infrastructure | **NONE** | gateway and/or network per lane |
| Latency | direct serial, same as Windows today | adds a network hop; gateway adds buffering |
| Reliability | one cable, one failure point | cable + gateway + network + power for each |
| Offline operation | **complete** — no network at all | needs the lane network up |
| One tablet / one target | **exact fit** — physically point-to-point | works, but the addressing is incidental |
| Future RMS | neutral; the tablet still publishes session events over the network | mildly favourable — the lane is already networked |
| Implementation complexity | **HIGH** — JNI/Java USB host, permission flow, CH340 line coding, framing | **LOW** — the socket path already exists |
| Maintenance | permanent: a chip driver we own | low |
| Field serviceability | swap a cable | diagnose cable, gateway, IP, DHCP, firewall |
| Customer setup | plug in | configure an address per lane |
| Wi-Fi / network dependency | **none** | **total** |
| Security / permissions | one Android USB permission dialog | network exposure of a scoring device |
| **Works with installed targets** | **YES** | **NO, without new hardware** |

---

## 5. Recommendation, and why it inverts the earlier one

**Implement USB Host + CH340.**

The earlier record recommended proving Modbus TCP first, and it was right to,
**because it made that recommendation explicitly conditional** on the customer's
installed base being able to take a TCP path at acceptable cost. This audit
resolves that condition, and it resolves it against TCP: the installed targets
are CH340 serial, and giving them TCP means adding a gateway to every lane.

The deciding row is the last one. A single-target product whose purpose is to
run on targets that already exist cannot ship a transport those targets do not
have. TCP wins on every software criterion and loses the only one that is not
ours to change.

**The honest counter-argument, stated rather than buried:** a serial↔TCP gateway
is a real option and needs almost no application code. If the customer is
already willing to put a small gateway on each lane — or is building new lanes
anyway — Option A becomes the better engineering answer immediately, because it
removes a chip driver we would otherwise own forever. **This decision is a
recommendation about the installed base, not a claim that USB is technically
superior.** If the customer says "we will fit gateways", switch.

---

## 6. Architecture the implementation must follow (§7)

```
Android USB host (Java/JNI)  →  CH340 line discipline  →  Modbus RTU framing
        ↓
   TRANSPORT ADAPTER ONLY
        ↓
current proven acquisition core  →  shot acceptance  →  session / competition / reports
```

The adapter supplies bytes. It does **not** decide whether coordinates are
valid, whether a shot is official, how scoring works, how counter
reconciliation works, or how persistence works. There must be exactly one
acquisition authority and it is the existing one.

The protocol parameters are **19200 baud, Even parity, 8 data bits, 1 stop bit,
RTS disabled** — field-authoritative, and they belong to the RTU layer wherever
it runs. They must never be applied to a TCP socket.

---

## 7. Finite implementation plan (next round, not this one)

1. **Verify USB host mode on the intended tablet.** Blocks everything. One
   device, five minutes.
2. Java: `UsbManager` device enumeration, CH340 VID/PID filter, permission
   request via `PendingIntent`, attach/detach `BroadcastReceiver`.
3. Java: CH340 control transfers — baud divisor, parity, stop bits, RTS — for
   19200/Even/8/1/RTS-disabled.
4. JNI bridge exposing open / read / write / close to C++.
5. A `TargetTransport` implementation behind the existing seam, so the
   acquisition core is unchanged.
6. Permission and lifecycle state machine: NO DEVICE → FOUND → PERMISSION
   REQUESTED → GRANTED/DENIED → CONNECTING → READY → REMOVED → RECONNECTING →
   ERROR. **READY only when acquisition is actually ready**, never because a
   `UsbDevice` object exists.
7. Reconnect against the existing counter-reconciliation rules.
8. Then, and only then, the small physical gate.

Steps 2–5 are the new code. Everything below the adapter already exists and is
field-proven.
