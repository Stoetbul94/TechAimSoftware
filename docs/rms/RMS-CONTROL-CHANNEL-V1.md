# RMS control channel v1

The authenticated control and replay plane. Telemetry is unchanged.

---

## 0. A correction to the premise, found before anything was built

Phase R1 reported port 7756 as "reserved". That is true of the *number* and
misleading about the *socket*.

`receiverTachus.cpp:73` does `udpSocket->bind(7756, QUdpSocket::ShareAddress)`.
**UDP 7756 is genuinely occupied** by the target application's historical
inbound control path (`startMatchFromServer`), which predates RMS. The existing
design note is explicit that RMS does not use, extend or remove it, and that
replacing it must be "its own reviewed change".

**This does not block the control channel, and it is not worked around.**
TCP 7756 and UDP 7756 are different sockets at the operating-system level, so
the control listener binds TCP 7756 and the legacy UDP listener is left running
and untouched. Nothing in this milestone alters, gates or removes the legacy
path — that decision stays where the design note put it.

The read-only guard is updated **deliberately** rather than by accident: it
now asserts the distinction (no RMS file sends to **UDP** 7756) instead of
banning the number outright.

---

## 1. Transport and direction

| | |
|---|---|
| **Telemetry** | UDP **7755**, node → range, unchanged, still read-only |
| **Control** | **TCP 7756**, framed, authenticated |
| **Listener** | the **NODE** listens |
| **Initiator** | **RMS** connects, using the address it learned from that node's UDP telemetry |

TCP because control needs what UDP does not give: reliable delivery, ordering,
request/response pairing, and replay batches larger than a datagram.

The node listening is what makes a range practical: nodes need no fixed RMS
address, DHCP stays acceptable, and RMS can follow a node whose address
changes. **The address is connection metadata, never identity** — after
connecting, RMS verifies the authenticated `nodeId` and drops the connection if
it is not the node it intended to reach.

## 2. Framing (§30)

TCP is a byte stream. One `read()` is not one message.

**Length-prefixed JSON.** Each frame is a 4-byte big-endian unsigned length
followed by exactly that many bytes of UTF-8 JSON.

- The length is read first and **validated before a single byte is allocated**.
- A frame longer than the limit for its type is refused and the connection
  closed. An oversized length is never used as an allocation size.
- Partial frames accumulate; two frames in one read are both delivered; a
  disconnect mid-frame discards the partial and reports a clean error.

All four cases are tested.

## 3. Message size limits (§31)

| Message | Max |
|---|---|
| Handshake | 8 KiB |
| Command | 16 KiB |
| Acknowledgement | 16 KiB |
| Replay batch | 256 KiB |
| Absolute frame cap | **256 KiB** |

The cap is checked against the declared length before reading the body, so a
hostile peer cannot make RMS or a node reserve memory by claiming a large
frame.

## 4. Version

`controlProtocolVersion = 1`, carried on every control frame and **independent
of** the telemetry `protocolVersion` and of any application version. A version
the peer does not implement is rejected, never downgraded silently.

## 5. Handshake

```
RMS  ──►  hello        controlProtocolVersion, rmsInstanceId, nonce
NODE ──►  challenge    nodeId, bootId, product, appVersion, commit,
                       capabilities[], nodeNonce
RMS  ──►  auth         hmac( rmsNonce ‖ nodeNonce ‖ nodeId ‖ rmsInstanceId )
NODE ──►  authResult   accepted | rejected(reasonCode), sessionOfRecord
```

RMS rejects, and closes:

- a `controlProtocolVersion` it does not implement
- a `nodeId` other than the one it dialled
- a failed `authResult`

There is no downgrade path and no anonymous mode.

## 6. Capabilities (§11)

The node advertises what it can actually do:
`status`, `eventReplay`, `athleteAssignment`, `sessionPrepare`, `startAt`,
`stop`, `paperFeed`, `reportRequest`, `supportBundle`.

**Command availability is capability-driven, never inferred from the product
name.** A command the node did not advertise is refused with
`UNSUPPORTED_CAPABILITY` rather than attempted.

## 7. Command envelope (§12)

```json
{ "controlProtocolVersion": 1,
  "messageType": "COMMAND",
  "commandId": "<globally unique>",
  "nodeId": "...", "laneId": "...", "sessionId": "...",
  "commandType": "START_AT",
  "issuedAtUtcMs": 0,
  "payload": { }
}
```

## 8. Acknowledgement (§13)

Every command produces exactly one ack.

```json
{ "controlProtocolVersion": 1,
  "messageType": "ACK",
  "commandId": "...", "nodeId": "...",
  "accepted": true,
  "reasonCode": "OK",
  "message": "",
  "resultingState": { },
  "nodeTimestampUtcMs": 0,
  "duplicate": false }
```

**A TCP write that succeeded is not a command that executed.** The ack reports
the state the node is *now in*, and `accepted:false` carries a machine-readable
`reasonCode`.

## 9. Idempotency (§14)

The node remembers a bounded history of recently handled `commandId`s. A repeat
**does not execute again**; it returns the original outcome with
`duplicate: true`.

This is why `commandId` is generated by RMS and is stable across a retry: a
retried `FEED_PAPER` that fed twice would be a physical fault, not a networking
one.

## 10. Commands in v1 (§15)

`PING`, `REQUEST_STATUS`, `REQUEST_REPLAY`, `ASSIGN_ATHLETE`,
`PREPARE_SESSION`, `START_AT`, `STOP`.

`FEED_PAPER` is defined in the grammar and **capability-gated off by default**:
it moves physical hardware, and §43 says it does not become an
operator-visible range-wide command until the node adapter is physically
validated.

## 11. Fan-out (§41, §42)

An operator acts on one lane, a selection, or all lanes. Internally that is
always **N independent commands**, each with its own `commandId` and its own
ack. A 50-lane start where one lane is offline reports **49 accepted, 1 failed,
which lane, and why** — never a global success that hides a lane.

## 12. Control connection state (§32)

Distinct from telemetry, and deliberately so: a healthy UDP heartbeat says
nothing about whether commands can be delivered.

`TelemetryOnline` · `ControlDisconnected` · `ControlConnecting` ·
`ControlAuthenticated` · `ControlError`

## 13. What is NOT in v1

- No TLS. The security boundary is stated honestly in `RMS-SECURITY-V1.md`.
- No command that alters scoring, coordinates or the node's own competition
  rules. The authority boundary is unchanged: the node may always refuse.

---

## 14. R2B — what qualification added and what it exposed

Phase R2B exercised this protocol against simulated nodes that speak it
through the **production** `NodeControlEndpoint`, at 1, 20 and 50 lanes. Full
results: `RMS-R2-QUALIFICATION.md`. Two properties of v1 surfaced that were not
visible when the protocol was tested one message at a time, and both are
recorded here as part of the contract rather than left in a test file.

### 14.1 Command idempotency is DURABLE

> **R2B recorded per-boot idempotency as an accepted limitation. R2C closed it.**
> The text describing it as accepted behaviour has been replaced, not annotated.

The handled-command store is a **journal owned by the node**, not a cache owned
by the endpoint, and a node hands its recovered journal to the new endpoint on
startup. The question a repeated `commandId` asks is therefore *"did this NODE
do it"*, not *"did this PROCESS do it"*.

A `commandId` reused **across a node restart** is recognised and **not applied
again**. The node answers with the ORIGINAL outcome — accepted or refused, with
its reason code and resulting state — and `duplicate: true`. That matters: RMS
retried precisely because it never heard the first answer, so a bare
"already executed" would leave it exactly as ignorant as the lost ack did.

| Command | Journalled |
|---|---|
| `ASSIGN_ATHLETE`, `PREPARE_SESSION`, `START_AT`, `STOP`, `FEED_PAPER` | yes |
| `PING`, `REQUEST_STATUS`, `REQUEST_REPLAY` | no — they read and change nothing |

**Retention.** At most 512 entries; evictable entries older than 48 hours are
pruned. Above both bounds sits one rule: an entry that is durable **and**
belongs to the node's current session is never evicted. Staying inside a budget
by forgetting the running match's `START_AT` would re-arm the exact failure the
journal exists to prevent. When everything left is protected the store stays
over budget and reports that it did, rather than dropping something dangerous.

**What RMS may now rely on.** A retry with the same `commandId` is safe, within
a boot and across one. Retrying after a timeout is the correct action.

**What RMS must still never do.** Mint a NEW `commandId` for a retry of the same
operator intent. The journal recognises ids; a new id is a new command, and
exactly-once is defeated by the RMS side alone.

### 14.2 A boot change RETIRES the channel

> **R2B recorded this protection as living at the wrong end. R2C moved it.**

RMS compares the `bootId` in a node's telemetry against the one it is tracking.
A change means the process was replaced, so whatever authenticated to the
previous incarnation is **void by definition** — not "probably stale", not
"worth a try". The channel is retired at that moment, before anything is sent on
it, and this sequence runs automatically with no operator action:

```
RESTART DETECTED → REAUTHENTICATING → (pending commands retried) → REPLAYING → CURRENT
```

Capabilities are refreshed by construction, because they arrive on the Challenge
of the new handshake — a node that came back running a different build is
believed about what it now advertises.

The node's own refusal remains as a **second** line of defence: even with RMS at
its most wrong, a command is never applied on the strength of an authentication
the current process did not perform. It is simply no longer the *first* line.

**`bootId` is a process incarnation, not an identity.** A boot change creates no
lane, no node, no athlete assignment and no session, and leaves the ledger, the
watermark and the time-sync measurement untouched. First sight of a node is not
a restart, and an unchanged `bootId` is not one either.

**A pending command survives it.** A command whose answer never arrived is held
— and persisted — and retried after reauthentication with its ORIGINAL id.

### 14.3 `START_AT` (§16–§20)

`START_AT` names an **instant**, never an action, and carries the measured
offset with it:

```json
{ "commandType": "START_AT",
  "payload": { "startAtUtcMs": 1700000030000,
               "rmsToNodeOffsetMs": -2500,
               "syncQuality": "GOOD" } }
```

The node converts the instant into its **own** clock using that offset and
schedules. It does **not** start on arrival — starting on arrival would give
every lane its own delivery jitter as a head start.

Rules, all enforced and tested:

- a lane whose measured sync quality is `UNUSABLE` is **refused before the
  command is sent**, with reason `SYNC_UNUSABLE`. A start placed wrongly is
  worse than a start refused;
- a second `START_AT` on a lane that already has one scheduled or running is
  refused with `PRECONDITION_FAILED`, and the existing instant is **unchanged**.
  Silently re-basing a running competition clock would rewrite the elapsed time
  of a live match.

### 14.4 An undelivered command is FAILED

A command that got no answer is reported `UNREACHABLE` and **failed** — never
assumed applied, and never recorded as a refusal the node did not make. This is
the reason acks exist, and it is asserted per lane in the fan-out tests.
