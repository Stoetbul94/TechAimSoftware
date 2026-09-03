# RMS R2 qualification — the control plane, closed out

What phase R2B set out to prove, what it now proves, and — set out just as
plainly — what it still does not.

**Status:** software qualification complete, through phase R2C. **No physical
qualification has been performed.** Nothing in this document may be cited as
evidence that a real target node behaved any particular way.

**Phase history.** R2 built the protocol. R2B qualified it in use and found two
correctness defects, which it recorded rather than hid. **R2C closed both** —
§3a and §3b — and re-ran every gate. Where this document previously described a
limitation as accepted behaviour, that text has been replaced by the fix and its
evidence, not merely annotated.

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

**1719 checks, 0 failures.** (R2 baseline: 1391; R2B: 1542.) The control-plane
suites:

| Suite | Checks | What it holds |
|---|---:|---|
| `tst_catchup.cpp` | 57 | gap detection, automatic recovery, restart, watermark, audit |
| `tst_timesync.cpp` | 31 | offset measurement, quality grading, scheduled starts |
| `tst_control_scale.cpp` | 32 | full control at 20 and 50 lanes, partial failure |
| `tst_control_status.cpp` | 31 | what the window says, and refuses to say |
| `tst_journal.cpp` | 47 | the durable handled-command store and its retention contract |
| `tst_reauth.cpp` | 109 | exactly-once across restarts, boot invalidation, 20/50-lane restart qualification |

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

### Two findings R2B recorded, both CLOSED in R2C

R2B found and wrote down two real defects. Neither is accepted behaviour any
more; both were fixed, and the sections below are the evidence.

| R2B finding | Status |
|---|---|
| Command idempotency was per boot — a reused id executed again after a restart | **CLOSED** — §3a, durable command journal |
| RMS did not invalidate control authority when `bootId` changed | **CLOSED** — §3b, boot-driven invalidation |

---

## 3a. Command idempotency now survives a restart (R2C §2–§6)

**The failure being closed.** RMS sends a command. The node applies it. The ack
is lost. From where RMS sits, "it never arrived" and "it arrived and I did not
hear" are indistinguishable — so RMS must retry. Then the node restarts. Under
R2B the retry executed a *second* time, because the handled-command cache died
with the process. For a `START_AT` that restarts a running match; for a paper
feed it feeds twice.

### The durable journal

`src/rms/control/CommandJournal` is the node's handled-command store, owned by
the **node** rather than by the endpoint — so it outlives the endpoint the way a
file outlives a process. `NodeControlEndpoint` now asks it, not a private cache,
which turns the question a repeated `commandId` asks from *"did this process do
it"* into *"did this node do it"*.

**What is persisted.** Not merely that an id was seen: `commandId`,
`commandType`, `nodeId`, `sessionId`, the accepted/refused outcome, the reason
code, the message, the node's resulting state, and when it was processed. That
is what lets a duplicate be answered with the **original acknowledgement**
(`duplicate: true`) rather than a bare `ALREADY_EXECUTED` — which would leave
RMS exactly as ignorant as the lost ack did. Refusals are remembered too: a
retried command that was refused is refused again, with the same reason.

**What is never persisted.** No key, no MAC, no nonce, no handshake material —
by construction, and asserted by serialising the document and looking.

### Which commands are protected

| Journalled (durable) | Not journalled |
|---|---|
| `ASSIGN_ATHLETE`, `PREPARE_SESSION`, `START_AT`, `STOP`, `FEED_PAPER` | `PING`, `REQUEST_STATUS`, `REQUEST_REPLAY` |

`FEED_PAPER` is included **before** it is ever enabled, so when a node adapter
turns it on it arrives already protected rather than needing a second change.
The three exclusions read and change nothing, so repeating one is harmless and
journalling them would spend the retention budget on traffic that never needed
protecting.

### The retention contract

Bounded, because an unbounded store is a disk leak a peer could drive. Bounded
**safely**, because the obvious way to stay inside a budget is to forget the
running match's `START_AT` — which would re-arm the exact failure being closed.

1. **Hard rule, above the bounds.** An entry that is durable **and** belongs to
   the **current session** is never evicted, by count or by age.
2. **Count.** At most **512** entries. Over budget, the oldest *evictable*
   entry goes first; protected entries are skipped, not dropped. If everything
   left is protected, nothing is evicted and `retainedOverBudget()` counts it —
   visible, never silent.
3. **Age.** Evictable entries older than **48 hours** are pruned. That outlives
   a competition day and the night after it.

Rule 1 outranks rules 2 and 3. Once a session is finished its entries become
evictable like anything else, so the store can still shrink.

### Evidence

`tst_journal.cpp` and `tst_reauth.cpp`:

- a flood of 712 finished-session entries does **not** evict the live session's
  `START_AT`, and the age bound does not either — but once its session ends, it
  prunes;
- lost ack → node restart → same `commandId` retried, for `START_AT`, `STOP`,
  `ASSIGN_ATHLETE` and `PREPARE_SESSION` in turn: each is applied **exactly
  once**, the retry is answered with the original outcome, and the audit records
  `ACK RECOVERED`;
- **§5, the harder case:** node restarts *and* RMS restarts, both recovering
  only from their own files, then RMS retries the same id — still exactly once.

---

## 3b. A boot change now invalidates control authority (R2C §7–§10)

Under R2B, RMS discovered a restart by being **refused**. The node's own defence
held — a command was never applied on the strength of an authentication the
current process never performed — but learning about it that way meant RMS could
not recover on its own.

**Now the node's telemetry drives it.** `noteBootIdentity` compares the
`bootId` in what the node reports against the one RMS is tracking. A change
means the process was replaced, so whatever authenticated to the previous one is
**void by definition** — not "probably stale", not "worth a try". The channel is
retired at that moment, before anything is sent on it.

`serviceNodes` then runs the whole sequence automatically, with no operator
action: **RESTART DETECTED → REAUTHENTICATING → (retry pending) → REPLAYING →
CURRENT**. Capabilities are refreshed by construction, because they arrive on
the Challenge of the new handshake — a node that came back running a different
build is believed about what it now advertises, not what its predecessor did.

### `bootId` is a process incarnation, not an identity (§8)

A boot change creates **no** new lane, node, athlete assignment or session. The
ledger, the watermark and the time-sync measurement are all left alone. Only the
channel is invalid. Asserted directly: after a restart the node count is still
1, the lane and session are unchanged, and the six shots already held are still
held.

First sight of a node is **not** a restart, and an unchanged `bootId` is not one
either — otherwise every range start would report a restart per lane.

### Pending commands (§9)

A command whose answer never arrived is neither forgotten nor re-invented. It is
held as **pending** and, after reauthentication, retried with the **same
`commandId`** — minting a new id for the same operator intent would make the
node treat it as a new command and defeat exactly-once protection entirely.

Pending commands are **persisted**, so an RMS crash between issuing a command
and hearing its answer does not lose the fact that an answer is still owed.

### A defect this uncovered in the observer

Building the restart-plus-catch-up scenario exposed a real bug in `RangeMonitor`
that no earlier test could have reached: **the stale-boot guard was discarding
replayed pre-restart shots.**

The guard exists to stop an old run's *state* overwriting the current run's — an
old heartbeat dragging the shot count or the phase backwards. But a shot is not
state. It is an immutable historical fact with its own unique `eventId`, and the
ledger deduplicates on that. After a node restart, catch-up replays exactly
those events, because the shots fired before the restart were fired under the
previous boot. Dropping them left a lane permanently short of shots its athlete
had actually fired.

Fixed: a shot from a superseded boot is **ingested**, and touches nothing about
the node's identity, liveness or boot bookkeeping. The drop counter does not
move, because nothing is dropped. The guard is unchanged for announces and
statuses, and the R1 test that pins it still passes.

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
3. **`FEED_PAPER` stays capability-gated off.** It moves physical hardware and
   does not become an operator-visible range command until a node adapter is
   physically validated. It is journalled already, so when it is enabled it
   arrives protected.
4. **Real-network timing is unmeasured.** The sync grades are software
   thresholds, not field figures.
5. **The RMS command audit is not bounded.** The *node's* journal is (§3a);
   RMS's own audit trail grows with the range's activity. That is deliberate
   for now — an audit exists to be complete — but a long-lived installation
   will eventually need a retention policy of its own.
6. **The node's journal is written by the simulator, not yet by Tech Aim
   Windows.** `CommandJournal` is production code and `ControlledNode` drives
   the real one, but wiring it into the target application is R3 work and is
   explicitly out of scope here.

Items 3 and 4 of the R2B list — per-boot idempotency and RMS not noticing a boot
change — are **closed**, not deferred. See §3a and §3b.
