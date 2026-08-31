# SETA 1.0.0-EVAL4 — physical evaluation checklist

For the SETA range. Everything below is **software-qualified and physically
untested**. No DSB programme has been fired on a physical target; Tech Aim's
field evidence is Tech Aim's and is not claimed for DSB conduct.

The list is deliberately short. It exercises the **distinct behaviour
families**, not every shot-count variant — firing 20, 40 and 60 of the same
programme tests the same code three times, while 1.20 and 1.60 test things
nothing else does.

**Run `Collect-Logs.cmd` after every session below, pass or fail.** Some logs
live in `%TEMP%` and Windows deletes them; once gone they cannot be recovered.

---

## A — 10 m rifle, standard course (DSB 1.10)

Any shot count. Check: shot recognition, paper feed, score, report.

- [ ] every shot detected, none doubled, none missed
- [ ] one accepted shot → **exactly one** paper feed
- [ ] scores match the visible group
- [ ] report opens, totals correct, PDF saves
- [ ] German wording throughout; **Teiler shown**

## B — 10 m pistol, standard course (DSB 2.10)

- [ ] as A, on the pistol face

## C — DSB 1.20 Luftgewehr 3-Stellung — **the most important test**

This is the only programme with **three independent position clocks**, and
nothing else in the product behaves this way.

- [ ] preparation runs **15 min**, sighters unlimited
- [ ] preparation ends **at a gate** — kneeling does **not** start by itself
- [ ] kneeling starts only when started, on its **own full clock** (25 min for 3×10, 35 min for 3×20) — **not** reduced by the preparation before it
- [ ] kneeling opens **in match** (its sighting was the shared preparation)
- [ ] the position ends at its own shot count; an extra match shot is refused
- [ ] ending a position **arms** the next and starts **nothing** — confirm by waiting a minute at the gate and checking no clock moved
- [ ] prone and standing open **in sighting on a clock already running**; entering match does **not** restart it
- [ ] the first match shot of a position closes its sighting
- [ ] report shows three positions with correct per-position subtotals

## D — DSB 1.40 KK-Sportgewehr 3×20, 50 m

- [ ] 50 m acquisition, feed and scoring behave as at 10 m
- [ ] **one** 105-minute master clock — it does **not** restart at a position change
- [ ] transitions at shots **20** and **40**

## E — DSB 1.60 KK-Freigewehr 3×40, 50 m — **the boundary test**

The specific thing to watch, because it is what a hardcoded course would get
wrong:

- [ ] kneeling **does NOT transition at shot 20** — it continues to 40
- [ ] it **does** transition at shot **40**, and again at **80**
- [ ] one 165-minute master clock throughout

## F — DSB 1.80 KK-Liegendkampf, 50 m prone

- [ ] 60 shots, decimal scoring, report correct

## G — DSB 2.20 50 m Pistole

- [ ] 60 shots, whole-ring scoring, report correct

---

## H — one real USB disconnect / reconnect

Mid-match, in **any** programme (1.20 is the most informative):

- [ ] the status strip shows the disconnection
- [ ] on reconnect no shot is duplicated and none is lost
- [ ] the shot count does **not** reset
- [ ] the clock does **not** restart and no second clock appears
- [ ] the position does **not** restart

## I — one support bundle after a session

- [ ] `Collect-Logs.cmd` produces `SETAElectronicTargetControl-Support-*.zip`
- [ ] it contains that session's journal
- [ ] `WHAT-WAS-COLLECTED.txt` says **`OK - scoped to this product only (TechAimSETA)`**
- [ ] it contains **no** Tech Aim sessions

---

## Known limitation — read before testing recovery

**DSB 1.40 and 1.60 have no journal recovery. An interrupted 50 m
three-position course cannot be resumed.**

This is a limitation of the shared **50 m three-position engine**, not of DSB:
ISSF 3×20 runs on the same engine and has the same gap. It closes for both when
that engine migrates to the qualification seam.

What *does* survive: the adopted rule authority is written into the saved
`.tch`, so a recovered 1.40 is still identifiably DSB 1.40 and never a generic
60-shot 50 m match.

**Do not test crash recovery in C's 50 m equivalents (D and E) and report it as
a defect — it is a known, recorded gap.** Crash recovery **is** implemented and
worth testing in **1.20 (C)**, and in 1.10, 1.80, 2.10 and 2.20.

---

## What SETA is not being asked to do

Reproduce the automated suite. 6 378 automated checks pass, ~186 of them DSB
specific, including all ten DSB 1.20 recovery scenarios. Those cover what a
machine can check. This list covers what only a physical target can.
