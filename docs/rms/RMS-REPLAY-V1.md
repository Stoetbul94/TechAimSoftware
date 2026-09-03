# RMS replay / catch-up v1

Closes the gap R1 identified: RMS could *detect* missed shots through
`unobservedShotCount()` but had no way to fetch them.

## Source

Replay is served from the node's **persisted authoritative session events** —
the same journal that underlies live telemetry. Never from a UI model, never
from an RMS cache, never from a re-read of the target.

**Live telemetry behaviour is unchanged.** `NodeTelemetryService` still
subscribes at `SessionStore::eventApplied` and still skips `replayed` events,
because a recovery replay is history being rebuilt and broadcasting it as live
would misstate when shots happened. `REQUEST_REPLAY` is an *explicit historical
fetch* over an authenticated channel and reads persisted events on purpose. The
two paths stay separate, and the live one is not touched.

## Request

```json
{ "commandType": "REQUEST_REPLAY",
  "payload": { "sessionId": "...", "afterSequence": 20, "maxEvents": 200 } }
```

## Response

```json
{ "messageType": "REPLAY_BATCH",
  "requestId": "...", "sessionId": "...",
  "fromSequence": 21,
  "events": [ ... ],
  "hasMore": true,
  "nextSequence": 221 }
```

`maxEvents` is clamped to **200**, and a batch is capped at 256 KiB. A session
of any length is fetched as successive batches; nothing assumes one response
holds everything.

## Event identity is preserved

Replayed events carry their **original** `eventId`, `sessionId`,
`shotSequence`, timestamp, `authoritativeScore`, raw coordinates and shot-role
fields.

**No new event id is ever minted during replay.** That is not a detail: RMS
deduplicates on `eventId`, so a re-minted id would make every catch-up insert
duplicates — the exact failure replay exists to prevent.

## Idempotency

Replay uses the *same* ingest path and the *same* ledger as live telemetry, so
it inherits the deduplication already proven at fifty lanes:

- RMS holds 1–20, misses 21–25, requests `afterSequence: 20`, receives 21–25 → ledger **25**
- the identical request again → ledger **still 25**, `duplicatesSuppressed` rises
- a request from sequence 0, replaying the whole session → ledger **still 25**

## Ordering

A replay batch may arrive interleaved with live telemetry. Both go through one
ingest and the ledger is keyed by sequence, so a live shot 26 arriving during a
replay of 21–25 lands correctly whichever order they reach RMS.

---

## R2B — who asks, when, and how the gap is found

R2 defined the request and proved the response. It did not define **who runs
it**. R2B did: `RangeControlCoordinator::reconcileAll` catches up every
authenticated node the monitor says is behind, on the dashboard's own tick.
**No operator action is required for an ordinary reconnect.**

### The gap is found from SEQUENCES, never from totals

A score total can match while shots are missing, and a total cannot say
*which*. RMS compares two things and asks about their difference:

| Kind | What it looks like | How it is detected |
|---|---|---|
| **Shortfall** | a stretch offline | `shotsAcceptedByNode > ledger.observedCount()` |
| **Hole** | one lost datagram | `ledger.missingSequences()` is non-empty |

**The second one is the trap.** Once a later shot arrives, the highest sequence
already equals the node's count, so a shortfall test *alone* declares RMS
current while shot #18 is missing. Both are checked, every time. When a hole
exists the request starts **below the first hole**, not at the highest sequence,
so the missing middle is fetched rather than skipped.

Re-fetching shots RMS already holds costs nothing: the ledger deduplicates on
`eventId`, which is exactly why replayed events keep their original ids.

### A current node is not asked at all

If there is no shortfall and no hole, **no request is sent**. A reconciliation
pass that asked every node on every tick would put a fifty-lane range's worth of
pointless traffic on the wire during a live match.

### What counts as recovered

Only events that were genuinely new: `Accepted`, `AcceptedOutOfOrder`, and
`SessionRestarted` — the last being what the *first* shot of a session looks
like when RMS learned of the node from a status message and has never seen one
of its shots. A duplicate suppressed on replay is **not** counted as a recovery.
Counting every replayed event would report a recovery that did not happen.

### The reconciliation watermark

Persisted through `RmsJsonStore` — the same versioned, atomic store every other
RMS document uses (temp file then rename, `schemaVersion` stamped by the store,
a newer document refused rather than half-read). **Not SQLite, and not a second
private file format.**

It records `nodeId`, `sessionId`, `lastBootId` and `highestSequence`, read back
from the monitor **after** ingesting — never from what RMS asked for. Recording
the request would claim a reconciliation that may not have happened, and the
shots between the claim and the truth would never be requested again.

It is written on the **no-gap** path too: "reconciled to 20, nothing missing" is
the common case and precisely the fact a crash must not lose.

On load, a failed or refused read applies **nothing**. A partially-applied
watermark would tell RMS it had reconciled further than it had.

### Proven at

1 lane, 20 lanes and 50 lanes; sessions longer than one 200-event batch; a
single lost middle datagram; a full offline stretch; and a node process restart
mid-session. See `RMS-R2-QUALIFICATION.md`.
