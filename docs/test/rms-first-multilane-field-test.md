# First multi-lane field test — range day procedure

The order to do things in, and what each phase proves. Roughly two hours if
nothing goes wrong, and the phases are arranged so that when something does go
wrong you already know which layer it is in.

**Nothing in this procedure commands a target.** RMS observes and configures.
Every match on every tablet is started and run exactly as it is today.

---

## Before you leave

- [ ] The M4.7 field-test package unzipped on the range PC
- [ ] `Launch-TechAimRMS-Live.cmd` launches (do this at home, not at the range)
- [ ] Tablets charged, target stations working **on their own** — RMS cannot
      fix a station that is not already working
- [ ] A printed copy of
      [`rms-physical-shot-registration-checklist.md`](rms-physical-shot-registration-checklist.md)
- [ ] Something to write station codes on: masking tape and a marker

---

## PHASE 1 — network only, no shooting

**Proves:** the PC can hear the tablets at all.

1. Put the RMS PC and every tablet on the **same** LAN. No internet needed.
2. Start **one** tablet.
3. Launch RMS with `Launch-TechAimRMS-Live.cmd`.
4. Go to **FIELD TEST → NETWORK**.

| what to check | where |
|---|---|
| UDP listener says LISTENING on 7755 | Network tab |
| a sensible IPv4 address is listed | Network tab |
| the header says **LIVE** and **OBSERVING UDP 7755** | top right |

**If the listener says ERROR**, stop here — nothing else will work. The page
names the socket error. Usual causes: another program owns the port, or
Windows Firewall has not been allowed. RMS does not change firewall settings;
that is a decision for a person.

**If the listener is fine but no station appears:** the tablets and the PC are
probably not on the same network, or the access point has **client isolation**
enabled, which stops devices talking to each other even though both have
internet. That single setting has eaten whole afternoons.

5. Go to **FIELD TEST → PREFLIGHT**. Expect `OBSERVATION PREFLIGHT WAITING`
   with "Stations heard: WAITING". That is correct before commissioning.

---

## PHASE 2 — commission the lanes

**Proves:** RMS knows which tablet stands on which physical lane, permanently.

### The rule

**A physical lane is not a device identity.** The mapping RMS keeps is
`laneId ↔ nodeId`, and nothing else.

**Never number lanes from:**

- the IP address — DHCP will change it and the lane must not move
- the discovery order — whichever tablet boots fastest is not lane 1
- the boot id — it changes on every restart
- the order the tablets are lying on the bench

### One tablet at a time — the safe method

This needs no change on the tablet and works today.

1. Have **all** tablets off, or disconnected from the network.
2. Power up the tablet that will stand on **physical lane 1**.
3. In RMS: **RANGE SETUP**. Within a few seconds one new entry appears under
   **UNASSIGNED DEVICES**, showing a station code like `E222-403F`.
4. Write that code on tape and stick it on the tablet. Do this — it makes
   every later problem easier.
5. Press **ASSIGN** on Lane 1, then assign that station.
6. Power up the next tablet. Assign it to Lane 2. Continue.

Because only one unknown station is ever present at a time, there is nothing to
guess. Afterwards the mapping is permanent until you deliberately change it.

7. Bring every tablet online. **RANGE SETUP** should show each lane with its
   station code and `TARGET_CONNECTED`.

### Replacing a tablet

Lane 3's tablet dies and a spare replaces it:

1. **RANGE SETUP** → Lane 3 → **CLEAR**
2. Lane 3 → **ASSIGN** → the new station code

RMS deliberately **refuses** to drop a new station onto an occupied lane in one
step. Replacing a tablet must be two conscious acts, not one ambiguous one.

### If a tablet's data is wiped

It comes back with a **new** nodeId, and RMS treats it as a **new, unassigned
station** — it does not assume it is the old lane 3 device. Reassign it
explicitly. That is the safe behaviour, not a bug.

---

## PHASE 3 — one physical target, before anything else

**Proves:** a shot appears where the shot actually went.

**Do this before multi-lane firing.** If the axes are wrong, finding out on one
lane costs ten minutes; finding out on six costs the day.

Work through
[`rms-physical-shot-registration-checklist.md`](rms-physical-shot-registration-checklist.md)
on **one** lane. In short:

| fire | expect on RMS |
|---|---|
| centre | centre |
| right | right |
| left | left |
| **high** | **high** |
| low | low |

For each, write down the tablet's own x, y and score, and what RMS drew.

**The high shot is the one that matters.** Which way is up in the telemetry has
never been confirmed against a real pellet — it is taken from the target
application's own renderers. If a high shot draws low, stop and report it: the
fix is one line in RMS and nothing on the tablet.

Also check the hole **edge**, not its centre, against the ring its score names.
ISSF scores by outward gauge, so on a 10 m air rifle face a 10.0 has its centre
well outside the ten ring. That is correct.

---

## PHASE 4 — multi-lane observation

**Proves:** the whole range works at once.

1. **FIELD TEST → PREFLIGHT.** Expect `OBSERVATION PREFLIGHT COMPLETE`.
   Warnings are allowed; FAIL is not.
2. Fill in test name, range and operator. Press **START FIELD TEST LOG**.
3. Shoot on three to six lanes.

Check, on **DISPLAYS** and **LIVE RANGE**:

- [ ] every shot lands on the **correct lane**
- [ ] the athlete shown matches the plan
- [ ] the programme matches
- [ ] the shot count matches the tablet
- [ ] the score matches the tablet
- [ ] the target face is the right one for the discipline

Any mismatch is worth stopping for. A shot on the wrong lane is the most
serious thing this test can find.

---

## PHASE 5 — break things on purpose

**Proves:** the range recovers, and RMS tells the truth about what it missed.

This phase is the reason for the whole day. Do not skip it.

### 5a — network loss on one lane

1. Disconnect Lane 3 from the network (pull Wi-Fi, not power).
2. **Keep shooting on Lane 3.** The tablet is fine; only RMS is blind.
3. Watch RMS: Lane 3 goes **OFFLINE**, stays in the list, keeps its last
   known target.
4. Reconnect.

Expect:

- [ ] it returns as **the same Lane 3** — not a new station
- [ ] a warning appears: **N shots not observed**
- [ ] the lane's **TOTAL** is still the station's total, not RMS's sum
- [ ] the missing shots are **not** drawn — RMS never invents an impact

### 5b — restart a target application

1. Close and reopen the target app on Lane 4.

Expect:

- [ ] same nodeId, **new boot id**
- [ ] **same physical lane** — a restart never moves a lane
- [ ] restart count increases by one
- [ ] Lane detail → Diagnostics shows both

### 5c — restart RMS

1. Close RMS entirely. Leave every tablet running and shooting.
2. Reopen RMS.

Expect:

- [ ] stations return on their own lanes — the mapping persisted
- [ ] the reconciliation shows what RMS missed while it was gone

**RMS cannot reconstruct events it never received.** A new field-test log is a
new segment, and it names the previous one. That gap is recorded honestly
rather than smoothed over.

### 5d — DHCP, if you can force it

If a tablet gets a new IP, the lane must not move. IP is a diagnostic, never an
identity.

---

## PHASE 6 — export

**Proves:** the day can be analysed afterwards without relying on memory.

1. **FIELD TEST → STOP FIELD TEST LOG**
2. **EXPORT FIELD TEST**
3. The page shows the bundle's folder. Copy the whole folder off the PC.

The bundle contains `summary.txt`, `summary.json`, `range-snapshot.json`,
`lane-mappings.json`, `node-summary.json`, `diagnostics.json`, `events.jsonl`,
`shots.csv` and a `README.txt`. It contains no source code.

Also collect:

- [ ] the completed physical registration checklist
- [ ] photographs of any screen that looked wrong
- [ ] the tablets' own logs if a lane misbehaved

---

## What this test cannot prove

- **It does not certify the range for competition.** RMS observes; it cannot
  command or verify a target station.
- **It does not test RMS commands** — there are none, by design.
- Shots the stations accepted while RMS was offline are counted, never
  recovered. That is a property of the design, not a defect.

## Afterwards

Send the bundle. It is meant to be readable by someone who was not there:
`summary.txt` first, then `events.jsonl` for the sequence of what happened.
