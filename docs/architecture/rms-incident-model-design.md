# Tech Aim RMS — incident handling: design note only

**Nothing in this note is implemented.** There are no incident controls in RMS,
no Jury actions, and no way for RMS to alter a score. It exists so that when
incident handling is built it starts from the right shape, because the wrong
shape here is expensive to undo.

## The rule everything else follows

> **THE RAW OBSERVED SHOT AND THE ADJUDICATED RESULT ARE TWO DIFFERENT THINGS,
> AND THE RAW ONE IS NEVER DESTROYED.**

A shot accepted by a target node is a fact: it happened, at a coordinate, with
a score the node computed. A jury decision is a separate fact about that shot.
Recording the second by overwriting the first destroys the evidence the
decision was based on — and the appeal, the protest and the audit all need it.

So a future incident model must be **additive**. An annulment marks a shot
annulled; it does not delete it. A rescore records a new effective value beside
the original; it does not edit it. The effective result is always a
**derivation** over the raw record plus the adjudications, never a mutation of
the raw record.

This is the same discipline the target node's own reliability layer already
follows — corrections mark records, they never remove them
(`docs/session-reliability-implementation-spec.md` §8). RMS must not be the
place that breaks it.

## Incidents anticipated

`CROSS_FIRE` · `MISSING_SHOT` · `EXTRA_SHOT` · `ANNULLED_SHOT` · `PENALTY` ·
`TIME_CREDIT` · `MALFUNCTION` · `DISTURBANCE` · `PROTEST`

Each will need, at minimum: what it applies to (lane, session, shot sequence),
who raised it, who authorised it, when, and the rule reference. None of that is
built.

## Where authority sits

The node is the authority on what was **shot**. A jury is the authority on what
**counts**. RMS is neither: it will be where an official's decision is recorded
and displayed, and it must never invent one.

**RMS still computes no score.** Today it transports the node's
`authoritativeScore`; a future effective result will be derived from that value
plus recorded adjudications, and RMS will still not be scoring — it will be
applying decisions somebody else made.

## Not now

No incident buttons. No score adjustment. No annulment. RMS's score display in
this build is, and remains, the node's own accepted score.
