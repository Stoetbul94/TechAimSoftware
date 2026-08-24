# RC3B-DIAG — physical qualification plan

**STATUS: NOT RUN.** This is the checklist for the test, not a record of one.
No line below may be marked until a tablet has produced it.

RC3B-DIAG is **internal**. It is not a SETA evaluation package and must not be
sent as one (see §24 of the engineering brief). `developer_mode=1` is deliberate
— this build exists to be diagnosed.

## Before the range

- One tablet first. Not four. A shared engine is proven by one clean run and
  then repeated, never by four partial ones.
- Same binary on every tablet afterwards. No tablet-specific builds.
- `config.ini`: `app_mode=Live`, `developer_mode=1`.
- Record the tachus log path (`%TEMP%\tachus_log*.log`) before starting; collect
  it after **every** test, pass or fail.
- Confirm the serial parameters actually used are in the log: the connect line
  now prints port, baud, parity, data bits, stop bits and why that port was
  chosen. Expect **19200 / Even / 8 / 1**. If it says 9600/None, stop.

## Optional bench rehearsal (no target hardware)

Two emulator scenarios reproduce the defects without a range. Run these first if
a tablet is available before the range slot.

```
tools\emulator\release\target_emulator.exe --scenario F --port 1502 --fire-every 6 --reset-latency-ms 2600
```
Point the application at Modbus TCP `127.0.0.1:1502` (`%TEMP%\qModMaster.ini`,
`Session/ModBusMode=1`, `TCP/TCPPort=1502`), open a match and leave it. The
target fires every 6 s and honours the counter reset only after 2 600 ms — the
exact latency that stopped acquisition 12 times out of 12 on the tablets.
Expected: shots 1–10, boundary, 11–20, boundary, … with no acquisition fault.

```
tools\emulator\release\target_emulator.exe --scenario G --port 1502
```
The target answers a reconnect with counter 1 while ten shots sit in its slots —
Tablet-02's answer after the USB was replugged. Expected: the next shot is
scored from its real coordinates, or acquisition explicitly refuses. **A 10.8 is
a failure**, whatever else the screen says.

Four more scenarios cover the cases that are not about the boundary:

| | |
|---|---|
| `--scenario H` | the counter leaps forward with no coordinates behind it — **real** lost shots. The guard must still fire; if it does not, the boundary fix was bought by going deaf. |
| `--scenario J` | every coordinate read is refused while the counter advances. Expected: no shot accepted, no score, no feed, an explicit acquisition error. |
| `--scenario K` | one coordinate read in three fails. Partial acquisition is worse than none — some shots look fine. |
| `--scenario L` | the client is dropped mid-session and the counter is kept. Expected: resume, nothing replayed, nothing renumbered, and **no** "shots were missed" claim, because none were. |

## PHYSICAL TEST 1 — Training, 30–60 shots

Must cross the 10, 20, 30 … boundaries several times.

- [ ] every physical shot registers exactly once
- [ ] acquisition never stops
- [ ] no USB reconnect needed at any point
- [ ] no phantom shot
- [ ] no duplicate shot
- [ ] paper feeds exactly once per shot
- [ ] shot numbering is continuous and correct
- [ ] log contains **zero** `ACQUISITION_FAULT`
- [ ] log contains zero `ACQ_COORD_INDEX_INVALID`, `ACQ_COORD_READ_FAILED`,
      `ACQ_COORD_REFUSED_BY_UI`
- [ ] each boundary shows the reset state machine completing: reset requested,
      hardware counter read back at 0, baseline adopted — and no poll judging a
      delta while the reset was outstanding

"The session completed" is not a pass. The log has to be clean as well.

## PHYSICAL TEST 2 — Reconnect

Mid-session, unplug the USB. Reconnect.

- [ ] the disconnect is reported, not absorbed
- [ ] no historical shot replayed
- [ ] no false score
- [ ] no `-1 / -1` coordinate anywhere
- [ ] no repeated 10.8
- [ ] the next real shot is scored from its own real coordinates
- [ ] if the target counted shots while offline, the application says so plainly
      and does not absorb them
- [ ] session indexing stays consistent afterwards

## PHYSICAL TEST 3 — 50 m 3P Final

- [ ] sighters accepted
- [ ] counted shots accepted
- [ ] position and phase transitions clean
- [ ] incident raised, resolved, resumed
- [ ] acquisition unaffected by the incident, before, during and after
- [ ] no repeated 10.8
- [ ] coordinates remain real throughout

## PHYSICAL TEST 4 — Restart and recovery

Restart the application on a known session.

- [ ] session restored correctly
- [ ] no historical shot emitted as new
- [ ] no unsolicited paper feed
- [ ] the first post-recovery physical shot is correct

## Then, and only then

Repeat a shorter run on the remaining valid tablets — **Tablet-01, Tablet-02 and
Tablet-04**. Tablet-03's evidence is July bench material with no target attached
and is not a measure of these fixes.

Only after all four physical tests are clean on the first tablet, and the
shorter runs are clean on the others, may a SETA evaluation build be produced,
with `developer_mode=0` and its own documentation. The external release number
is not decided here.
