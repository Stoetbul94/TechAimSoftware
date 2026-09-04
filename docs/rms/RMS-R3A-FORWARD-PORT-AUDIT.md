# R3A — forward-port audit

What came across from the stale node-telemetry branch onto current Tech Aim 1.0,
what had to change on the way, and what was deliberately left behind.

**Base:** `release/techaim-1.0.0` @ `f0b7fcc`
**Branch:** `feature/techaim-rms-node-v1`
**Stale branch merged:** **NO.** `feature/rms-node-telemetry` is 174 commits
behind this base. Not one of its commits was merged; its *files* were audited
individually and carried across.

---

## Why not just merge

`feature/rms-node-telemetry` is 4 commits ahead of the release line and **174
behind** it. Merging it would have produced a Tech Aim missing two months of
work — including the whole 1.0.0 release train — in exchange for a feature that
is 29 additive files. The diff was purely additive (2 773 insertions, 0
deletions outside images), which is what made a file-level port the obviously
correct route.

---

## Classification

### PORT DIRECTLY — unchanged, byte-for-byte

The shared wire contract and the publisher. These depend on QtCore and the
reliability layer only, and every API they touch still exists on the current
base.

| File | |
|---|---|
| `src/rms/RmsProtocol.{h,cpp}` | the v1 wire contract |
| `src/telemetry/ITelemetrySink.h` | |
| `src/telemetry/NodeIdentity.{h,cpp}` | persisted `rms/nodeId`, per-process `bootId` |
| `src/telemetry/NodeTelemetryService.{h,cpp}` | see one adaptation below |
| `src/telemetry/UdpTelemetrySink.{h,cpp}` | write-only, never bound |
| `Telemetry.pri` | |
| `tests/telemetry/*`, `tests/reliability/tst_node_telemetry.cpp` | |

### ADAPT TO CURRENT TECH AIM

| Seam | What drifted | What was done |
|---|---|---|
| `ShootingPage.qml` | The stale branch read a `programmeId` **role off the event ListModels**. That role does not exist on this base. | Rewritten against `competitionCatalogue.programmeIdAt(modelKey, index)`, which is how the current base resolves programme identity — **by index**, with catalogue order load-bearing. Reading the absent role would have published an empty programme identity on every lane, silently. |
| `main.cpp` | Insertion point moved; the surrounding controllers are the same. | Seam re-applied before `engine.load`, using the current `qualController` / `finalsController` / `finals10mController` and the unchanged `TachusWidget::targetStatusChanged` signal. |
| `Seta.pro` | — | `QT += network` and `include(Telemetry.pri)`, plus the new `include(RmsNode.pri)`. |
| `NodeTelemetryService.cpp` | — | The shot→wire conversion was **extracted** to `src/telemetry/ShotTelemetry.h` so the live publisher and the new replay provider share one definition. Two copies of the `eventId` formula would drift, and the day they drifted every catch-up would insert duplicates instead of suppressing them. |

### SIMULATOR / RMS-ONLY — not carried into the application

| | Why |
|---|---|
| `tools/rmsnode/` | A standalone headless node harness. Useful to RMS development; not part of the shooting application, and its job here is done by `tools/rmsnodecheck/`, which drives the **real** application instead of standing in for it. |
| `docs/architecture/rms-milestone-2-node-telemetry.md`, `docs/img/rms-m2-*.png` | RMS milestone documentation and screenshots of an earlier build. Carrying screenshots of a superseded binary into a new branch would be citing stale evidence. |

### OBSOLETE

| | |
|---|---|
| The `programmeId` ListModel role assumption | Superseded by the catalogue's `programmeIdAt()`. Recorded here because it is the one place a silent port would have produced a plausible but wrong result. |

---

## What R3A added beyond the port

The stale branch had telemetry only. The control plane is new here, and is the
**qualified R2 code**, shared rather than forked:

| From the RMS branch, unchanged | New in this branch |
|---|---|
| `ControlProtocol`, `ControlAuth`, `NodeControlEndpoint`, `CommandJournal`, `RmsJsonStore` | `src/rms/node/NodeControlServer.{h,cpp}` — the TCP 7756 socket R2 deliberately left unwired |
| | `src/rms/node/TechAimNodeCommands.{h,cpp}` — the command handler and replay provider bound to this application |
| | `tools/rmsnodecheck/` — the software one-node test |

**The control sources are shared, not copied-and-edited.** A second
hand-maintained copy of a control protocol drifts, and a control protocol that
drifts silently sends the wrong command to the wrong lane.

`NodeControlServer` contains **no decisions**. Every refusal — wrong key, bad
MAC, wrong node, wrong version, unknown command, oversize frame, stale command,
duplicate commandId — is decided by `NodeControlEndpoint`, already qualified
against 1 719 automated checks. The server moves bytes and owns connections.

---

## Two design calls worth stating

**Session control is capability-gated, not silently ignored.** `START_AT` and
`STOP` change a live competition, so they are off unless armed. The wrong way to
express that is to accept the command and do nothing — an ack saying `accepted`
when nothing happened is a plausible but false report. The right way is the
mechanism the protocol already has: unarmed, the node does not **advertise**
those capabilities and the endpoint refuses them as `UNSUPPORTED_CAPABILITY`.
RMS is told the truth in the protocol's own vocabulary.

`FEED_PAPER` is never advertised, armed or not.

**The command journal is written on every handled command, not at shutdown.**
The first implementation persisted at `aboutToQuit`. A force-kill during the
one-node test lost the entire journal — which defeats cross-boot idempotency
completely, because a crash is precisely the event it exists to survive. It now
writes as each command is handled; state-changing commands are rare, and the
cost is nothing against what losing them means. The test now force-kills the
node deliberately and asserts the previous boot's command ids are still
recognised.

---

## Tech Aim core

Unchanged: target acquisition, Modbus, serial, scoring, coordinate validation,
shot acceptance, paper feed policy, competition rules, Finals, Qualification,
reports. The integration reads the session record **downstream of acceptance**
and drives only the qualification controller's **existing public** lifecycle
transitions, which the reducer still validates.

Legacy UDP 7756 in `ModReader` is untouched and still runs alongside TCP 7756.
