# RMS — authority boundary

Who owns what, stated before any control channel is built. §5 asks that
nothing be left ambiguous, so every row below says exactly one owner.

**The governing rule, already enforced in the protocol:** the target node is
authoritative for everything that turns a physical event into a score. RMS
observes, aggregates and (later) commands. RMS never re-derives a score.

---

## The table

| Concern | Owner | Why |
|---|---|---|
| Physical acquisition | **NODE** | The hardened Modbus/serial path lives there and is field-proven. Duplicating it would create a second acquisition engine — the one thing §6 forbids |
| Coordinate validation | **NODE** | `ACQ-SENTINEL-003` and friends run before a shot reaches the store; RMS never sees a candidate coordinate |
| Scoring | **NODE** | `authoritativeScore` is **transported, never recalculated**. RMS carries `rawX/rawY` for display and diagnostics only |
| Paper feed | **NODE** | One accepted shot, one feed, decided at the node. RMS may later *request* a feed; it may never own the policy |
| Target counter / reconciliation | **NODE** | Counter baseline and reset crossing are acquisition concerns |
| Reconnect baseline | **NODE** | RMS losing the network must not touch how a node reconciles its target |
| Shot role, official shot number | **NODE** | Set at acceptance from competition state; RMS receives them, never infers them |
| Competition rules | **NODE** | ISSF/DSB rule engines live in the single-target product |
| **Competition clock** | **NODE today; see below** | |
| Session identity | **NODE** | `sessionId` is minted by the node; `bootId` distinguishes restart from blink |
| Node identity | **NODE** | `nodeId` stable and persisted, independent of COM port, IP and lane |
| **Lane identity** | **RMS** | A lane is a range concept. The node reports the lane it believes it is on; RMS owns the mapping |
| **Athlete assignment** | **RMS** | The operator assigns; the node reports what it was told |
| **Match / discipline plan** | **RMS** | `MatchPlanService` |
| **Results aggregation** | **RMS** | Across lanes — the one thing a node cannot do |
| **Range database** | **RMS** | Ranges, athletes, plans, observed ledgers |
| **Reports** | **SHARED** | Per-athlete report is the node's (it holds the authoritative session); range results are RMS's |
| **Target health display** | **SHARED** | Node reports, RMS aggregates and decides what "stale" means across the range |
| **Operator overrides** | **RMS, once commands exist** | With an id, an acknowledgement and a recorded outcome — never fire-and-forget |

## Offline behaviour is part of the boundary

If RMS disappears, **nothing at the lane changes**. The node keeps scoring,
keeps feeding paper, keeps its session, and keeps its journal. This is not a
resilience feature bolted on; it falls out of the node owning everything in
the top half of the table.

The reverse is also defined: when a node goes quiet, RMS marks the lane
**offline and stale** and keeps it visible. It does not remove the lane and it
does not invent state. `unobservedShotCount()` states what RMS knows it has
missed rather than pretending the totals are complete.

## The competition clock (§25) — recommendation

Today the **node** owns it, and for a single lane that is correct: the clock
is monotonic at the node and survives RMS being absent entirely.

For multi-lane range control the requirement changes — lanes must not start at
whatever moment their datagram happened to arrive. The recommendation is
therefore:

> **RMS distributes an authoritative START EPOCH; each lane computes its own
> remaining time from that epoch using its own monotonic clock.**

Not "RMS owns the clock" (a network hiccup would then stall a live match), and
not "each lane starts when it hears the command" (lanes would drift apart by
their delivery jitter). Distributing an epoch keeps the node's proven monotonic
timing and makes the *start instant* common.

This needs one thing v1 does not have: a shared time reference good enough to
compare epochs across machines. That is a handshake offset, not internet NTP —
and it should be designed with the control channel, not before it.

## What must be true before any command exists

1. **Authentication.** Today any host on the LAN can send a datagram RMS will
   parse. While RMS is read-only the worst case is false data on a dashboard.
   The moment a command can move a target, an unauthenticated channel is a
   safety problem, not a security nicety.
2. **Command id + acknowledgement + outcome.** Socket delivery is not
   execution (§23).
3. **Idempotency**, so a retried command is not a second feed.

Until those three exist, the reserved control port stays unused — and
`tst_readonly.cpp` asserts that it does.
