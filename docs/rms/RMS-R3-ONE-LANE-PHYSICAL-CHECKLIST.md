# RMS R3 — one-lane physical checklist

**One RMS laptop. One Windows Tech Aim tablet or laptop. One target. One local
network. About 8–15 shots.**

This is deliberately **not** a multi-lane test. Fifty lanes are qualified in
software and worth nothing until one lane works with a real target in front of
it.

---

## Read this first

**No shot has ever been fired through the RMS node build.** The telemetry and
control seams are covered by automated checks and by a software one-node test
against the real application, all with **no target attached**. Everything from
TEST 3 onwards is being exercised for the first time by whoever runs this list.

**RMS cannot start or stop your match**, and for this test it should not try.
Session control ships disarmed. TEST 8 arms it deliberately, last, after the
shooting is done — so that if range control misbehaves it cannot spoil the shot
record you came to collect.

**The target application is authoritative throughout.** If the tablet and RMS
disagree, the tablet is right and RMS is behind. A disagreement is an RMS defect
to chase, never a reason to doubt the shot record.

---

## What you need

| | |
|---|---|
| Node | `C:\Users\User\Downloads\TechAim-RMS-Node-EVAL1\TechAim.exe` — version must read **1.0.0-RMS-EVAL1** |
| RMS | `C:\Users\User\Downloads\TechAimSoftware-RMS\Start-RMS.cmd` |
| Network | Both machines on the **same** LAN or AP. Telemetry is broadcast and does not cross subnets |
| Target | The usual CH340 cable and COM port |
| Ammunition | 8–15 rounds |

**Client isolation is the most common reason nothing appears.** Guest networks
and many access points silently drop broadcast between clients. If RMS stays
empty with everything else correct, put both machines on a phone hotspot and
retry before assuming a software fault.

---

## TEST 1 — the node starts and is itself

- [ ] `TechAim.exe` launches from the EVAL folder
- [ ] About screen reads **1.0.0-RMS-EVAL1**, not 1.0.0
- [ ] The normal Tech Aim installation is still present and still opens
- [ ] `%LOCALAPPDATA%\TechAim\TechAim\Settings\range.key` now exists

## TEST 2 — the lane appears in RMS

Start RMS. Leave the node running.

- [ ] RMS header reads **LIVE** and **OBSERVING UDP 7755**
- [ ] **UNASSIGNED DEVICES** goes to **1** within a few seconds
- [ ] Range Setup → assign the node to **Lane 2**
      *(Lane 1 still holds the stale `diagnostic-probe-01` — leave it alone)*
- [ ] Live Range shows Lane 2 as **ONLINE**

Record the node identity RMS shows: `____________________`

## TEST 3 — target connection is reported separately from the link

Connect the target to the node and pick the COM port.

- [ ] The node reaches **Connected**
- [ ] RMS shows the lane's **target** state change, not just "online"
- [ ] Now unplug the target, leaving the node running:
      RMS must show the lane **still reachable** with its **target** gone.
      A node with an unplugged target and a node that has vanished are two
      different problems and must not look the same
- [ ] Reconnect the target

## TEST 4 — shots reach the range

Start a session on the node and fire **five**.

- [ ] Each shot appears on Lane 2 within a second or two
- [ ] Shot count on RMS matches the node **exactly**
- [ ] Score on RMS matches the node **exactly** — RMS must never show a score
      the node did not compute
- [ ] Coordinates look right against the visible group
- [ ] Open the lane detail — the recent shots are listed

## TEST 5 — the network drops mid-session  ← the one that matters

With the session still open, **disconnect the node's Wi-Fi** (not the target).

- [ ] Fire **three** shots while the node is off the network
- [ ] The node records all three normally — an athlete's record must never
      depend on the network having been up
- [ ] RMS marks the lane offline / behind

Reconnect the Wi-Fi.

- [ ] RMS recovers the three missed shots **by itself**, with no operator action
- [ ] Final count on RMS equals the node's count **exactly**
- [ ] **No duplicates** — the three recovered shots appear once each
- [ ] No gap in the shot sequence

## TEST 6 — the node restarts mid-match

With shots already recorded, **close the node application and reopen it**.

- [ ] The node recovers its session
- [ ] RMS notices the restart by itself and re-establishes the lane
- [ ] The lane is **not** duplicated — still one lane, same identity
- [ ] The athlete and session are **not** reset
- [ ] Shots recorded before the restart are still on RMS afterwards
- [ ] Fire **two** more; both appear, with no duplicate of the last pre-restart shot

## TEST 7 — RMS restarts

Close RMS and reopen it. Leave the node running and the session open.

- [ ] The lane reappears
- [ ] The shot count is correct — RMS catches up rather than starting from zero
- [ ] No duplicate shots

## TEST 8 — range control, armed deliberately, last

**Only after the shooting above is complete.** Close the node, then restart it
with session control armed:

```
set TECHAIM_RMS_ARM_CONTROL=1
TechAim.exe
```

- [ ] With control **unarmed** (before this step), RMS reported the lane as
      present but **not** commandable
- [ ] Armed, the node advertises the session commands
- [ ] Copy `range.key` from the node to the RMS laptop so both share it
- [ ] `rmsnodecheck.exe` passes against the running node

Record whether the node ever accepted a command it did not perform: **YES / NO**
*(it must be NO — an acknowledgement for something that did not happen is the
single worst failure this design guards against)*

---

## Whatever happens

- [ ] Generate the **support bundle** from the node and keep it
- [ ] Note the node identity and both boot ids if it restarted
- [ ] Keep the `.tch` match record

**Return the support bundle pass or fail.** It carries the transport and session
diagnostics that make a failure diagnosable rather than guessable.

---

## What a pass here does and does not mean

A green run proves **one lane** works with a real target on a real network. It
says nothing about twenty or fifty lanes on range Wi-Fi, about contention, or
about a full competition day. Those are the next tests, and this one has to pass
before they are worth running.
