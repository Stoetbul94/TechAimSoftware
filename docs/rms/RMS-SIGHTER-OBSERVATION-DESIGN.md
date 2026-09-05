# Sighter observation — design, not yet implemented

**Status: DESIGN ONLY.** Nothing in this document is built. It exists because
R3B §15 asks for the smallest clean option to be chosen *before* code, and
because the wrong choice here silently corrupts an official score.

---

## What happens today

The 2026-09-05 physical test fired **three sighters**. RMS displayed **none**,
and that is current, deliberate behaviour rather than a fault.

`NodeTelemetryService::onEventApplied` returns early on `SighterAccepted`. The
reason is in the code and it is a protocol constraint, not an oversight:

> A `SighterAccepted` carries `shotNumber 0` by construction, protocol v1
> requires a positive competition sequence, and there is no field that
> classifies a shot as a sighter. Synthesising a sequence for it would put
> sighters into RMS's match ledger — the one thing the classification exists to
> prevent.

The journal confirms it: all three sighters carry `shotNumber: 0`, and the
node's own official count for that session is **5, not 8**.

| | Fired | Transmitted | Received | In ledger | Displayed |
|---|---:|---:|---:|---:|---:|
| Sighters | 3 | 0 | 0 | 0 | 0 |

**Classification: the NODE intentionally does not transmit sighters.** None of
the options offered in the brief fits exactly — it is not RMS filtering (A), not
a UI choice (B), and not incorrect (C). It is a deliberate decision forced by a
v1 field gap.

---

## What the end state must guarantee

A range operator should see sighter impacts. Sighters must **never**:

- increment the official shot count
- change the official score total
- consume an `officialShotNumber`
- participate in ranking

Those four are the whole risk. Any design that can violate one of them under
any ordering, retransmission or replay is the wrong design regardless of how
convenient it looks.

---

## The three options, assessed

### Option A — reuse `shot.accepted`, add `shotRole`

Add an optional `shotRole = SIGHTER | OFFICIAL` to the existing shot message,
plus a separate sighter sequence.

**Backwards compatible on the wire?** Yes — unknown fields are ignored.

**And that is exactly why it is dangerous.** An older RMS ignoring `shotRole`
does not see "a sighter I should skip"; it sees **a `shot.accepted` with a
positive sequence** and puts it straight into the official ledger. The
compatibility that makes the field safe to *add* is what makes this option
unsafe to *deploy*: the failure is silent, it inflates a competition score, and
it appears only when versions are mixed — which is precisely what a range with
one updated laptop and five old tablets looks like.

Giving sighters their own sequence space makes it worse, not better: an old
receiver would then see sequences 1, 2, 3 twice and record sequence conflicts on
a lane that is behaving perfectly.

**Rejected.**

### Option B — a new message type, `shot.sighter`

A separate type carrying `eventId`, `sessionId`, `observationSequence`,
`shotRole`, `x`, `y`, `score`, `timestamp` — and **no `officialShotNumber`**.

**Backwards compatible?** Yes, and safely so. RMS already counts unknown message
types and discards them: `unknownTypeDatagrams()` exists and is asserted. An
older RMS therefore behaves exactly as it does today — it shows no sighters —
while a newer one shows them. There is no version pairing in which a sighter can
reach an official ledger, because the official ledger is keyed off
`shot.accepted` and this is not one.

The four guarantees hold **structurally**, not by remembering to check a flag:

- it has no `officialShotNumber` to consume;
- it cannot increment the official count, which comes from `shotsAccepted` in
  `node.status`;
- it cannot change the official total, which comes from `totalScore` in
  `node.status`;
- it cannot rank, because ranking reads the official ledger.

**Recommended.**

### Option C — protocol v2

Not required. v1's own forward-compatibility rules (unknown types counted and
discarded, unknown fields ignored) are sufficient for Option B, and a version
bump would force every node and every RMS to move together for a feature that
does not need it.

**Rejected as unnecessary.**

---

## Recommendation

**Option B, and not before the ledger separation is proven by tests.**

The order matters. The tests come first because the whole risk is a sighter
reaching an official total, and that must be impossible before anything can
transmit one:

1. a `shot.sighter` datagram does **not** change `observedCount()`,
   `observedScoreSum()`, `highestSequence()` or `missingSequences()`;
2. a sighter and an official shot with the **same** numeric sequence coexist
   without a sequence conflict;
3. replay of a session containing both restores each exactly once, into its own
   ledger;
4. an RMS build that does not know the type counts it as an unknown type and
   changes nothing — the mixed-version case, asserted rather than assumed;
5. the physical 2026-09-05 session, which has three sighters and five official
   shots, still yields official 5 / 27.7 with sighters displayed.

Point 5 is worth stating plainly: that fixture already exists
(`tests/rmsnode/fixtures/`), it already contains three sighters, and it is the
natural regression for this work.

---

## Display behaviour, once transmitted

| | |
|---|---|
| During preparation / sighting | Sighter impacts on the lane face, visually distinct — hollow or outlined rather than filled |
| Counters | `Sighters: 3` shown separately. Official reads `Shots: 0` and `Score: 0.0` — **never** 3 and 23.3 |
| When the match starts | Sighters **fade to a background style and stay**, rather than vanishing. An operator who looks up mid-match should be able to see that the lane was sighted; clearing them destroys the only evidence that preparation happened |
| Ranking and results | Untouched. Sighters appear nowhere in either |

The one rule that survives every layout decision: **a sighter never shares a
visual language with a match shot.** If the two can be confused at a glance, the
display has failed regardless of what the totals say.
