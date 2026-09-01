# RMS — current state audit

Read-only inventory taken at the start of Phase R1, on `feature/rms` @
`b8fc972` (clean, local == remote).

**The headline: RMS is not a new project.** The brief warned against assuming
it was empty, and that warning was right. RMS is a working application at
milestone **M4.7** with its own `TechAimRMS.pro`, 52 source files, a 28-page
QML operator UI, **1 301 automated checks passing**, a deterministic range
simulator, and a completed physical range test. Most of what the brief's §10
list asks for already exists.

This round therefore did not design a protocol or build a foundation. It
audited what is there, found the one concrete limit blocking the stated
milestone, and lifted it.

---

## 1. State at the start (§1)

| | RMS | NodeTelemetry |
|---|---|---|
| Worktree | `TechAimSoftware-RMS` | `TechAimSoftware-NodeTelemetry` |
| Branch | `feature/rms` | `feature/rms-node-telemetry` |
| HEAD | `b8fc972` | `8d4d87e` |
| Clean? | **YES** | **YES** |
| Local == remote? | **YES** | **YES** |
| Last development commit | `b8fc972` — IPv4 observation bind | `8d4d87e` — wire-contract tests |

## 2. What RMS already has (§2)

| Area | State |
|---|---|
| Application | **`rms/TechAimRMS.pro`** — a separate binary, not a mode of the target app |
| Operator UI | **28 QML pages**: LiveRange, LaneDetail, TargetGrid, LaneCard, Athletes, Displays, MatchReview, NewMatch, RangeSetup, FirstRun, FullScreenDisplay, NavRail, StatusPill… |
| Protocol | **`RmsProtocol.{h,cpp}` — v1, shared contract** |
| Network | `RmsUdpObserver`, `NetworkDiagnostics` |
| Node registry | `RangeMonitor`, `TargetNodeRecord`, `UnassignedNodeModel` |
| Lane model | `LaneListModel`, `PlanLaneModel`, `RangeDefinition`, `RangeListModel` |
| Athletes | `Athlete`, `AthleteRegistry`, `AthleteListModel` |
| Match planning | `MatchPlan`, `MatchPlanService` |
| Persistence | `RmsJsonStore`, `RangeStore` — **versioned JSON, atomic write** |
| Displays | `DisplayController`, `DisplayLaneModel`, `ProgrammeDisplay` |
| Station identity | `StationCode` |
| Geometry | `TargetGeometry` — official face geometry, qualified |
| Field test | `FieldTestService`, `FieldTestRecorder` |
| Simulator | `dev/SimulatedRange`, `dev/TargetShotFixtures` |
| Tests | `tests/rms/` — 12 files, **1 301 checks / 0 failures** |
| Deployment | `tests/release/check_rms_deployment.py`, `dist/` packages |
| Docs | 11 architecture documents |

### Against the brief's §10 v1 list

| | Capability | State |
|---|---|---|
| A | Lane overview | **DONE** |
| B | Target connection / health | **DONE** — `ConnectionState`, offline episodes, stale drops |
| C | Athlete assignment | **DONE** |
| D | Discipline / match assignment | **DONE** — `MatchPlanService` |
| E | Session start / status / complete | **PARTIAL** — observed, not commanded |
| F | Live shot / score display | **DONE** |
| G | Central range commands | **ABSENT BY DESIGN** — see below |
| H | Reconnect status | **DONE** — `nodeRestarts`, `priorBootIds`, `offlineEpisodes` |
| I | Result collection | **DONE** — `ShotLedger` |
| J | Result export | **PARTIAL** |
| K | Logging / support | **PARTIAL** |

## 3. The protocol that already exists (§8)

Read from `RmsProtocol.h` rather than designed here.

| | |
|---|---|
| Transport | **JSON, one object per UDP datagram** |
| Port | **7755** observation (node → range); **7756 reserved** for control and asserted unused |
| Direction | node **broadcasts**; RMS observes. Nodes do not connect to RMS |
| Version | `protocolVersion = 1`, mandatory on every message; unknown fields ignored (forward compatible), unknown version **rejected and counted, never guessed** |
| Messages | `node.announce`, `node.status`, `shot.accepted` — three, and no more |
| Node identity | `nodeId` — stable, persisted; **not** the COM port, **not** the IP, **not** the lane |
| Restart detection | `bootId` changes per process start → distinguishes "node restarted" from "network blinked" |
| Idempotency | `eventId` (globally unique) + `shotSequence` (1-based, monotonic per session) |
| Stale suppression | `statusSeq` monotonic per boot; `priorBootIds` drops stragglers |
| Authority | `authoritativeScore` **computed by the node, never recalculated**; `rawX/rawY` are display and diagnostics only |

**This satisfies §4, §11, §13, §15 and §16 as written.** No re-scoring, no
second acquisition engine, no volatile identity.

## 4. NodeTelemetry (§9)

| | |
|---|---|
| Purpose | the target node's telemetry **publisher** |
| Files | `NodeTelemetryService`, `NodeIdentity`, `UdpTelemetrySink`, `ITelemetrySink` |
| Direction | node → range, one way |
| Where it subscribes | **`SessionStore::eventApplied`** |
| Commands | none — "It decides nothing, scores nothing, and can refuse nothing" |

**It is the correct architecture, not a prototype to retire.** Subscribing at
`eventApplied` means it can only ever describe shots the node has **already
accepted** — structurally incapable of seeing a raw Modbus read, a candidate
shot, a rejected shot or a UI click. It also skips `replayed` events, because
a recovery replay is history being rebuilt and re-announcing it as live would
misstate when shots happened.

That is a stronger guarantee than any amount of validation on the RMS side.

## 5. What is deliberately absent

**Commands and acknowledgements do not exist, and that is a decision, not a
gap.** `RmsProtocol.h` says so explicitly: no command message, no encoder, no
acknowledgement type, with `docs/architecture/rms-command-boundary-design.md`
holding the conceptual design that is *not* implemented. `tst_readonly.cpp`
asserts no RMS source uses port 7756 as a destination.

The brief's §22–§24 therefore describe work that has been consciously deferred
so the read-only path could be proven first. That ordering is right and this
round did not disturb it.

## 6. Gaps against this brief

| Gap | Reality |
|---|---|
| **Scale** | Simulator was clamped to **3–6 lanes**. §34 wants 20, §35 wants 50. **FIXED THIS ROUND** |
| Commands / acks | absent by design; design doc exists |
| **Persistence is JSON, not SQLite** | `RmsJsonStore` is versioned, atomic, and refuses documents from a newer RMS. §27 prefers SQLite; the existing store is not a weakness, and swapping it would be a rewrite with no stated failure behind it |
| **Replay / catch-up** | UDP broadcast has **no replay**. `unobservedShotCount()` tells RMS what it has *not* seen, which is honest, but there is no mechanism to fetch it. Closing this needs the reserved control port |
| Security | **none in v1** — any host on the LAN can send a datagram RMS will parse. Read-only means the exposure is false data, not target control. Must be closed before commands exist |
| `docs/rms/` | did not exist; this document creates it |

## 7. Recommendation

The architecture is **viable and should be kept**. The read-only-first
ordering, the node-authoritative rule, the identity design and the ledger are
all sound, and the parts this brief asks for next — commands, acks, replay,
security — are precisely the ones that need the control port that was
deliberately reserved and left unused.

The right next milestone is **not** SQLite and **not** a new protocol. It is
the control channel: authentication first, then commands with ids and
acknowledgements, then replay over the same channel.
