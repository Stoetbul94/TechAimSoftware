# Tech Aim RMS — the future command boundary: design note only

**Nothing here is implemented.** RMS has no command channel, no command
grammar, no transmitting socket and no acknowledgement type. This note records
how the match plan is intended to reach the stations one day, so that the plan
model built in milestone 4 does not have to be rebuilt when it does.

## Legacy UDP 7756 stays outside RMS

The target application still has its historical inbound control path on
**UDP 7756** (`receiverTachus.cpp`, `startMatchFromServer`). It predates RMS and
is unchanged.

- RMS **does not use it.**
- RMS **does not extend it.**
- RMS **does not remove it** — that is the node's decision, not RMS's, and it
  is not this milestone's business.

`tests/rms/tst_readonly.cpp` fails the suite if any authored RMS file so much as
names 7756 as a destination. A future command architecture should replace or
gate that legacy path **deliberately**, as its own reviewed change.

## The intended path

```
MATCH PLAN  (RMS configuration — exists today, transmits nothing)
     │
     ▼  LOAD_MATCH command      commandId · nodeId · planId · laneId ·
     │                          programmeId · athlete · expectedState
     ▼
NODE VALIDATES                  the node decides. It may refuse: wrong
     │                          discipline, session already open, target
     │                          disconnected, operator override in progress
     ▼
ACK / REJECT                    the node acks with the state it is NOW in,
     │                          not merely "received"
     ▼
observed sessionId bound to (planId, laneId)
```

## Properties the plan model already preserves

- **`planId` is not a node `sessionId`.** One plan spans several stations and
  will enclose several node sessions; conflating them would make the binding
  above impossible. They are separate fields today for exactly this reason.
- **Plan lanes are keyed by `laneId`**, which is keyed to `nodeId` — stable
  across a station's restart, re-cabling or new address. A command will address
  a station, not an address.
- **Programme identity is snapshotted by id** in the plan, so a command carries
  the same `programmeId` the operator chose, whatever the catalogue does later.
- **PLANNED and OBSERVED are already separate and already compared.** The
  mismatch RMS reports today is precisely what an acknowledgement will resolve
  tomorrow; the display for it exists before the mechanism does.

## Properties a command channel will need

- **Idempotency** — re-delivering a `commandId` is a no-op.
- **Acknowledgement of applied state**, not of receipt.
- **`expectedState` as a precondition** — a command assuming a phase the node
  has left is refused, not applied.
- **Node veto, always.** The node remains the authority on whether its own match
  may be altered. A range officer's intention does not override a station's
  judgement about its own session.

## Not now

No command type exists in protocol v1. No RMS class can transmit. Adding either
is a new milestone with its own review — and the read-only guard will fail the
moment it appears by accident.
