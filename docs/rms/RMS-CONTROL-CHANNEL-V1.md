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
