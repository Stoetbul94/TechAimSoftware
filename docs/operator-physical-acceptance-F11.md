# F11 — Physical Live-Target Acceptance Procedure

**Execution status: NOT EXECUTED.** No physical TechAim/SETA target hardware
was attached at build time (only Bluetooth virtual COM5/COM6 were present; the
target requires a dedicated RTU serial adapter, typically COM7 @ 19200 8N1).
Every test below is written so a range operator can run it against real
hardware and record PASS/FAIL. **Do not mark F11 complete, and do not advance
to version 1.0, until this procedure passes on real hardware.**

Run against `release-package/TechAim-0.9.0/Seta.exe` (the signed build once
signing is done). Confirm Settings ▸ About shows commit `6f42000` first.

## Pre-flight (operator records)
- Target hardware model / electronics board version: ____
- Connection type (RTU serial / TCP): ____   Port/endpoint: ____
- MODBUS/serial settings (baud, parity, stop): ____
- Discipline(s) the target physically supports: ____
- `config.ini` `app_mode` = **Live**

## F11C-1  Live source isolation  (NOT EXECUTED)
1. Launch in Live. Confirm the **LIVE TARGET** badge.
2. Click on the on-screen target with the mouse.
   - EXPECT: **no accepted shot**, no journal shot event (the C++ input-source
     gate rejects Simulated input in Live). Record.
3. Fire one physical shot.
   - EXPECT: shot accepted, tagged Physical, marker + score shown, one journal
     `ShotAccepted`. Record coordinates + score.

## F11C-2  Per-discipline Live runs  (NOT EXECUTED)
Run a short official series in each discipline the target supports:
AR10, AP10, PRONE50, one 10 m Final (FINAL_AR10 or FINAL_AP10), FINAL3P.
For each: shots accepted, correct count, correct total, valid journal on exit.
Record any discipline the single target **cannot** physically support.

## F11C-3  Score / coordinate validation  (NOT EXECUTED)
For each physically tested target, fire and record for: centre, 10-ring
boundary, 9/10 boundary, an outer ring, a clearly left/right shot, a clearly
up/down shot. For every shot capture and compare:

| Source | value |
|---|---|
| Target electronics output (x,y) | |
| TechAim raw coordinate | |
| TechAim calculated score | |
| On-screen marker position | |
| Journal `scoreTenths` | |
| Report score | |

If a calibration difference appears, record expected vs received vs displayed
coordinate, the score delta, and the suspected cause (electronics / scaling /
target geometry / UI). **Do not change scoring constants to make one shot look
right.**

## F11D  Communication-fault matrix  (NOT EXECUTED)
While a Live session runs, perform and record each:
1. Disconnect target cable → reconnect.
2. Power-cycle target electronics.
3. Restart the comms adapter.
4. Remove the COM port → restore it.
5. Delayed message (if simulator supports).
6. Duplicate packet (if simulator supports).
7. Malformed/incomplete data.
8. Disconnect during a shot window / during sighting / during recovery
   sighting / after a completed course.

For every case verify: **no crash; fault is visible; no fabricated official
shot; no partial packet accepted; no duplicate accepted twice; reconnect or a
clear operator action; timer correct; session still recoverable; hash chain
valid** (validate the journal afterwards).

## F11E  Power / crash / recovery matrix  (NOT EXECUTED — physical)
For at least one Live session: hard-kill the app → restart & recover;
power-cycle the target; recover with the target disconnected; reconnect;
continue; complete; validate the journal.

Verify: no duplicate physical shot; no missing pre-crash accepted shot;
correct next official shot number; correct target face; correct timer; correct
total; correct source mode (Live); correct incident state; correct report;
valid hash chain. **A real power test requires actually powering the target
down — do not simulate it in software and call it a power test.**

## F11E  Range-incident workflow (hardware-adjacent)  (NOT EXECUTED)
With qualification / a 10 m Final / a recovered session: open an EST incident;
confirm official shots block; timer pauses where required; decision recorded;
time allowance shown; recovery sighting stays separate from officials;
reassignment durable; resolve/authorise unblocks; **the operating-mode selector
stays blocked** throughout; incident history appears in reports where
supported. Validate the journal.

---
### Automated coverage already proving the logic (software side, PASSED)
The controller-level behaviour behind the above (source-gate rejection with no
accepted/journal event, duplicate/out-of-phase rejection, EST-incident
blocking, crash-recovery with correct next shot number and valid hash chain,
completion invariants) is exercised by the reliability (864/0), finals10m
(143/0) and 3P finals (189/0) harnesses. The **physical** column — real target
electronics, cabling, power, and calibration — is what remains NOT EXECUTED.
