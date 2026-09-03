# RMS control security v1

Scope: the authenticated control plane on TCP 7756. Telemetry on UDP 7755 is
unchanged and remains unauthenticated — see "The honest boundary" below.

## Method

**HMAC-SHA256 challenge–response over a pre-shared range key.** No custom
cryptography; `QMessageAuthenticationCode` with `QCryptographicHash::Sha256`.

```
mac = HMAC-SHA256( rangeKey,  rmsNonce ‖ nodeNonce ‖ nodeId ‖ rmsInstanceId )
```

- **The key is never transmitted**, in any form. Only the MAC crosses the wire.
- **Both sides contribute a nonce.** A node nonce alone would let a malicious
  node fix the challenge; an RMS nonce alone would let a recorded exchange be
  replayed at the node.
- **`nodeId` is inside the MAC**, so a valid exchange captured from lane 3
  cannot be replayed at lane 4. That is what binding identity to the handshake
  means, as opposed to merely asserting it afterwards.

## Replay protection

| Vector | Defence |
|---|---|
| Replayed handshake | fresh nonces from **both** sides, every connection |
| Handshake reused at another node | `nodeId` is inside the MAC |
| Handshake reused after a restart | `bootId` changes; the node nonce is per connection |
| Replayed command | `commandId` remembered in a bounded history; a repeat returns the original ack and does **not** re-execute |
| Very old command | `issuedAtUtcMs` outside the acceptance window is refused |

## Key storage

| | |
|---|---|
| RMS | `<RMS app data>/range.key` |
| Node | node app data, alongside its identity |
| Format | 32 random bytes, hex |
| Generation | created on first run if absent; never derived from a name, a serial, or anything guessable |
| In Git | **never** — the path is configuration, the key is not |
| In logs | **never** — only "authenticated" or "rejected(reason)" is logged |
| Rotation | write the new key on RMS and each node, then restart. A node still holding the old key fails authentication **visibly**, rather than degrading silently |
| Tests | a deterministic key exists **only inside the test binary**. It is not a default and no shipped configuration contains it |

## The honest boundary

**This authenticates control. It does not encrypt anything, and it does not
authenticate telemetry.**

What it stops: an arbitrary device on the range Wi-Fi issuing commands to a
target. That was the actual risk — without it, anyone on the LAN could start,
stop or reassign a lane.

What it does not stop:

- **Observation.** Control frames are plaintext JSON. Anyone on the LAN can
  read scores and lane state — but they already can, because telemetry is
  broadcast in clear on UDP 7755. The control channel does not widen that.
- **Forged telemetry.** UDP 7755 is unauthenticated, so a hostile host can
  inject false shots into an RMS dashboard. **The lane's own record is
  unaffected**: the node is authoritative and never learns anything from RMS,
  so the consequence is a wrong display, not a wrong result.

Both are acceptable on a closed range LAN and would not be on an open network.
If RMS is ever exposed beyond a range network, this section is the first thing
that must change — and the answer is TLS with node certificates, not a bigger
MAC.

---

## R2B — the audit, and what it deliberately does not contain

Phase R2B added a **persisted command audit**: every state-changing command RMS
issued, whether it was accepted, and what the node said came of it.

### It carries no secret material

No key. No MAC. No nonce. By construction — an audit exists to answer *who
asked what, and what happened*, and none of those three help with that. This is
not a convention; a test serialises the whole document and asserts that the
strings `mac`, `nonce` and `key`, and the range key's own bytes, do not appear
in it.

Each entry holds: `commandId`, `commandType`, `nodeId`, `laneId`, `sessionId`,
`issuedAtUtcMs`, `accepted`, `reasonCode`, `ackUtcMs`, and the node's reported
`resultingState`.

### Diagnostics are not recorded

`PING` and `REQUEST_STATUS` change nothing and are not written. An audit buried
under heartbeats is an audit nobody reads. `REQUEST_REPLAY` is likewise not
audited: it reads history and alters no state.

### A failure is recorded as a failure

A command that could not be delivered is written with reason `UNREACHABLE` and
`accepted: false` — never as an optimistic success, and never as a refusal the
node did not make.

### Where it is written

Through `RmsJsonStore`, into **RMS's own** application data location, alongside
the reconciliation watermark. It is RMS data about RMS's own actions. Nothing in
it can reach a target node, and nothing in it is transmitted anywhere.

### An unchanged boundary, restated

The audit records what RMS *asked for*. It is **not** a record of what happened
on the range: the node remains authoritative, may refuse any command, and its
own session journal is the record of the match. A disagreement between the two
is resolved in the node's favour, always.

### One security-relevant limit found by qualification

Command idempotency does not survive a node restart — the handled-command cache
lives in the endpoint that died. A replayed `commandId` across a restart **is**
applied again. This is a correctness and operator-trust limit rather than an
authentication weakness (the replay still has to pass the HMAC handshake of the
new boot), but it is the kind of thing that must be written down rather than
discovered later. Details and the closing work: `RMS-R2-QUALIFICATION.md` §3
and §9.
