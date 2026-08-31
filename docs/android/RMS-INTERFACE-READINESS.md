# RMS readiness — read-only note

**No RMS code exists and none was written.** This records which single-target
facts are already stable enough for a future Range Management System to
consume, so that when RMS starts it does not have to reverse-engineer them.

Nothing here is a commitment to an API, and nothing in Android was changed to
produce it.

## What already exists, and where

| Fact | Where it lives today | Stability |
|---|---|---|
| Session identity | `SessionStarted` event; `sessionId` in the journal filename | **stable** — the journal is the record |
| Athlete / shooter | `SessionStarted`, and `.tch` `user_name` | stable |
| Target identity | not modelled per-target; one app instance drives one target | **absent** — RMS will need to supply it |
| Shot coordinates | journal shot events, x/y in mm | **stable**, fixed-point |
| Score | journal shot events | stable |
| Shot role (sighter / match) | explicit on the event, set at acceptance | **stable** — not inferred from order |
| Official shot number | explicit on the event | stable |
| Discipline | `SessionStarted`; rule authority snapshot | stable |
| Position (3P) | carried per shot | stable |
| Competition state | reducer state (`SessionState`) | stable |
| Timestamps | monotonic ms + wall ISO on every event | **stable** |
| Connection state | `AndroidUsbTransport::State` / desktop status | stable per platform, **not yet a shared vocabulary** |

## The three real gaps

1. **Target identity is not modelled.** The single-target product has never
   needed to say *which* target it is; a range does. RMS will have to
   introduce it, and the natural place is the session envelope rather than
   each shot.
2. **Connection state has no cross-platform vocabulary.** Windows and Android
   express it differently. RMS would want one enum.
3. **50 m 3P sessions are not journalled** (recorded in the parity audit), so
   those disciplines currently have no event stream for RMS to read at all.

## What already helps

The node-telemetry seam promoted onto this branch (`NODETELEMETRY`) already
publishes shot events over the network with a versioned wire contract, and its
tests run in the harness. That is the natural carrier; it does not need
Android-specific networking added now, and none was.

**Explicitly not done:** no networking was added to Android for RMS, no RMS
production code, and no architectural change made in anticipation of it.
