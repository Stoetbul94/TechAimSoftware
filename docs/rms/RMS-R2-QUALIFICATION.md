# RMS R2 qualification — the control plane, closed out

What phase R2B set out to prove, what it now proves, and — set out just as
plainly — what it still does not.

**Status:** software qualification complete. **No physical qualification has
been performed.** Nothing in this document may be cited as evidence that a real
target node behaved any particular way.

---

## 0. Why this document exists

R2 built an authenticated control and replay protocol and tested it as a
protocol: frames, handshakes, refusals, one node at a time. That left five
things a range actually depends on unproven, and a protocol that is only
correct in isolation has not been qualified — it has been demonstrated.

R2B closed those five:

| # | Gap left by R2 | Closed by |
|---|---|---|
| 1 | Simulated nodes did not speak the control protocol at all | `ControlledNode`, composing the **production** endpoint |
| 2 | Catch-up existed but nothing ran it automatically | `RangeControlCoordinator::reconcileAll` |
| 3 | Reconciliation progress lived only in memory | a versioned, atomic watermark |
| 4 | Nothing recorded which commands had been issued | the command audit |
| 5 | "Start the range" had no defined meaning in time | measured clock offsets and `START_AT` |

and added the two things that make them usable: control at 20 and 50 lanes, and
a window that says what the control channel is actually doing.

---

## 1. The harness

`tests/rms/rms_tests.pro` — QtCore + network only, no GUI platform plugin, so
it cannot block on the "no Qt platform plugin" modal.

```bash
cd tests/rms && qmake rms_tests.pro && mingw32-make -f Makefile.Release && ./release/rms_tests.exe
```

**1542 checks, 0 failures.** (R2 baseline: 1391.) The new work contributes 141
named checks across four suites, and the read-only guard contributes 10 more by
scanning the two new control-plane file pairs:

| Suite | Checks | What it holds |
|---|---:|---|
| `tst_catchup.cpp` | 57 | gap detection, automatic recovery, restart, watermark, audit |
| `tst_timesync.cpp` | 31 | offset measurement, quality grading, scheduled starts |
| `tst_control_scale.cpp` | 32 | full control at 20 and 50 lanes, partial failure |
| `tst_control_status.cpp` | 21 | what the window says, and refuses to say |

### No simulator-only protocol

`src/rms/dev/ControlledNode` **composes** the production
`NodeControlEndpoint` and implements the production `IControlCommandHandler`.
Every ack in every test above came out of the shipping endpoint, through the
shipping framing, after the shipping HMAC handshake. Nothing calls an RMS
internal to manufacture a result.

`src/rms/control/RangeControlCoordinator` is transport-free in the same way and
for the same reason: a caller supplies a function that moves bytes to one node
and back. Production supplies a socket; the tests supply a direct call. Every
refusal is therefore provable without a network and without timing.

---

## 2. Offline catch-up (§6, §7)

**Gap detection is from SEQUENCES only.** Never from score totals — a total can
match while shots are missing, and a total cannot say *which*.

Two kinds of gap, and RMS looks for both:

- a **shortfall** — the node reports more accepted shots than RMS holds, which
  is what a stretch offline looks like;
- a **hole** — RMS holds #17 and #19 but not #18, which is what *one* lost
  datagram looks like.

The second is the one a naive implementation misses. Once a later shot arrives,
the highest sequence already matches the node's count, so a shortfall test alone
declares RMS current while a shot is missing. `tst_catchup.cpp` asserts exactly
that scenario and the recovery of the single middle shot.

Recovery uses **the same ingest live telemetry uses**, so deduplication is
inherited rather than reimplemented, and replayed events keep their **original
`eventId`**. Consequences, all asserted:

- a node that is current is **not asked for a replay at all**;
- catch-up run twice recovers nothing the second time and changes no count;
- a session longer than one 200-event batch is fetched across several batches;
- one `reconcileAll` pass recovers every dark lane on the range, with no
  operator action.

**RMS remains a mirror.** It requests; the node answers; the node's history is
authoritative from beginning to end.

---

## 3. Restart recovery (§10, §11)

Same node, same session, **new `bootId`**. The node recovered its own shots from
its own store; RMS tracks the boot change, counts one restart, and the ledger
continues across it — asserted at 11 shots before and 15 after.

### Two findings recorded rather than smoothed over

**A stale control channel is refused by the node, not by RMS.** After a node
process restarts, RMS's client still believes it is authenticated — nothing told
it otherwise. The *node* refuses the next command with `NOT_AUTHENTICATED`, and
the test asserts both halves: RMS's optimism, and the node's refusal. The
protection is real, but it lives at the node. A future revision should have RMS
notice the boot change and drop the channel itself.

**Command idempotency is per boot.** The handled-command cache lives in the
endpoint, which dies with the process. A command id reused *across* a restart is
applied a **second** time. Within one boot it is still suppressed, which is what
makes an ordinary retry safe.

This is a **known limit of control protocol v1**. It is asserted in
`tst_catchup.cpp` rather than hidden, and it is why the audit persists what was
issued: closing it needs the node to persist handled command ids, which is a
node-side change and its own reviewed work.

---

## 4. Persistence (§12, §13)

Watermarks and the command audit are written through `RmsJsonStore` — the same
versioned, atomic store every other RMS document uses. Temp file, then rename;
`schemaVersion` stamped by the store so no caller can forget; a document from a
newer RMS refused rather than half-read. **Not SQLite, and not a second private
file format.**

**The watermark records what RMS actually holds**, read back from the monitor
*after* ingesting — never what it asked for. Recording the request would claim a
reconciliation that may not have happened, and the shots between the claim and
the truth would never be requested again.

It is written on the *no-gap* path too. "Reconciled to 20, nothing missing" is
the common case and exactly the fact a crash must not lose.

**The audit carries no secret material.** No key, no MAC, no nonce — by
construction, and asserted by scanning the serialised document. `PING` and
`REQUEST_STATUS` are not recorded: they change nothing, and an audit buried
under heartbeats is an audit nobody reads. An undeliverable command is recorded
as `UNREACHABLE` and **failed** — never as an optimistic success.

---

## 5. Time sync and scheduled starts (§16–§20)

"Start all lanes" cannot mean "send START and hope". Fifty commands leave RMS at
fifty different moments; a node that starts on *arrival* starts its competition
clock at its own delivery jitter.

RMS names an **instant**, not an action. Each node converts it into its own
clock using its own measured offset. Delivery jitter then changes only how much
warning a lane gets.

**The measurement.** Four stamps (t0 send, t1/t2 at the node, t3 receive), the
classic offset and round-trip estimate, and **uncertainty reported as half the
round trip** — the strongest statement this exchange can honestly support.

| Bound | Grade |
|---|---|
| ≤ 25 ms | `GOOD` |
| ≤ 250 ms | `DEGRADED` |
| otherwise, or a negative round trip | `UNUSABLE` |

Asserted: a symmetric path recovers the offset exactly; an asymmetric path
biases the estimate by `(d1−d2)/2` and **the reported bound covers that error**;
an impossible measurement (negative round trip) is discarded, not believed; an
unmeasured clock reads `UNUSABLE`, never `GOOD`.

**A lane whose sync is `UNUSABLE` is refused, not started on a guess** — and the
refusal names the lane and the reason. A second `START_AT` is refused with
`PRECONDITION_FAILED` and the already-scheduled instant is unchanged: silently
re-basing a running competition clock would rewrite the elapsed time of a live
match.

Three lanes at −2 500 ms, 0 ms and +7 100 ms schedule at three different numbers
on three different clocks and land on **one** instant. Same assertion at 20 and
at 50 lanes.

### What is not claimed

These thresholds grade a *software* measurement taken in-process on one machine.
**No competition-grade synchronisation claim is made or may be drawn from them.**
What a real range achieves over real Wi-Fi is unmeasured, and measuring it needs
hardware.

---

## 6. Twenty and fifty lanes (§21–§27)

R1 qualified fifty lanes of read-only telemetry. That said nothing about
authenticating fifty channels, addressing fifty nodes individually, starting
them together and reconciling the ones that fell behind.

At both 20 and 50 lanes, all asserted: every channel authenticates; every lane
gets its **own** command id (a set of N distinct ids, not one broadcast);
per-lane athlete assignment; one scheduled start that all lanes place on the
same real instant; every fourth lane goes dark for 40 shots and one automatic
pass recovers all of them; every lane ends with a complete 60-shot record and no
sequence hole; the audit accounts for every state-changing command.

### A fan-out never collapses to a boolean

An operator told "all started" while two lanes sit idle has been misinformed at
the one moment it costs a match. `FanOutResult` reports accepted, failed, and
**which**:

- three lanes with a dead control link → 17 accepted, 3 failed, the three named,
  each `UNREACHABLE` — *not* recorded as a refusal the node never made — and the
  other 17 verified to have genuinely applied the command;
- one lane already started refuses a range-wide start → 19 start, 1 refuses by
  name, the refusing lane keeps the start it already had, and the other 19 are
  unaffected.

### Scope

One machine, in-process links, no network. This qualifies the **protocol, the
addressing and the bookkeeping** at fifty lanes. It says nothing about wire
timing, radio conditions or real hardware, and **no timing claim may be drawn
from it**.

---

## 7. What the window says (§28–§31)

`ControlStatusModel`, exposed to QML as `CONTROL`, drawn as
`RmsControlStatusBar` on the Live Range page.

**Status only. There is no command control in the UI, and none may be added
until a control transport is wired and separately qualified.**

The panel's most important behaviour is refusing to look operable when it is
not. This build has **no control transport wired**, so the bar reads:

> **CONTROL OFF** — CONTROL CHANNEL NOT ENABLED - this build observes only. Lane
> commands are not available.

Every lane reads `NOT CONNECTED` even though the coordinator's in-process
channels authenticate in tests: the panel describes the **range**, not the
software's opinion of itself. `transportAttached` is *declared* by whoever wires
a transport and is never inferred.

The tone is **neutral**, not red. A build without a control transport is
behaving correctly, and a false alarm teaches operators to ignore the colour
that means something is wrong.

With a transport attached the bar names counts rather than colours — "3 of 4
lanes authenticated; 1 lane behind by 4 shots - recovering" — and each lane
carries its own channel state, sync grade, unobserved count and reconciliation
watermark. One lane's state is never derived from another's.

**Evidence:** `docs/rms/evidence/r2b-control-status-bar.png` — a capture of the
real application window, not a mockup.

---

## 8. Boundaries held

| | |
|---|---|
| **Telemetry** | UDP 7755, node → range, read-only, **unchanged** |
| **Legacy UDP 7756** | `receiverTachus.cpp` still binds it. **Untouched.** |
| **Control** | TCP 7756 — a different socket |
| **Crossing** | none. No RMS telemetry over TCP; no RMS commands over UDP |
| **Scoring** | never re-derived. RMS compares sequences and nothing else |
| **Windows / SETA / Android** | **not modified.** Commands are not integrated into the Windows target application in this phase |

The read-only guard (`tst_readonly.cpp`) enforces the first four mechanically
and now scans the two new control-plane file pairs as well: no `QUdpSocket`
anywhere under `src/rms/control/`, and no reference to the telemetry port.

---

## 9. What is still open

1. **Physical qualification. Nothing here has touched hardware.** Every result
   in this document is a software measurement against a simulator. **An
   emulator result is never a physical pass.**
2. **No control transport is wired.** The protocol, the coordinator and the
   panel exist; the socket does not. Until it does, RMS still cannot command a
   target — which is what the window says.
3. **Command idempotency does not survive a node restart** (§3). Closing it
   needs the node to persist handled command ids.
4. **RMS does not drop a channel on a boot change** (§3). The node refuses; RMS
   should also notice.
5. **`FEED_PAPER` stays capability-gated off.** It moves physical hardware and
   does not become an operator-visible range command until a node adapter is
   physically validated.
6. **Real-network timing is unmeasured.** The sync grades are software
   thresholds, not field figures.
