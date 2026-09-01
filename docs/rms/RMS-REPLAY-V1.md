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
