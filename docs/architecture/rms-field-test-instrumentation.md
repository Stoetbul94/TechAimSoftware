# RMS field-test instrumentation — design note

Milestone 4.7. Station identity, lane commissioning, diagnostics, the event
recorder and the evidence bundle.

The whole milestone answers one question: **after a real range day, can
somebody who was not there work out what happened?**

---

## 1. Identity — the rule everything else rests on

**A physical lane is not a device identity.**

| thing | identified by | persisted? |
|---|---|---|
| a target station | `nodeId` | yes, by the station itself |
| a physical lane | `laneId` / lane number | yes, by RMS |
| the relationship | `laneId ↔ nodeId` | yes, by RMS, and only this |

**Never identity, in any circumstance:**

- IP address — DHCP changes it; the lane must not move
- MAC address alone
- COM port
- `bootId` — changes on every process start, by design
- the `laneId` a station reports in legacy telemetry
- **discovery order** — whichever tablet boots fastest is not lane 1

Each of those has a test in `tst_field_test.cpp` proving it cannot move a lane.
They remain useful as *diagnostics* and are shown as such.

`nodeId` is minted once by the station (`NodeIdentity::mintNodeId()`,
`"TA-NODE-"` + twelve hex) and persisted in its own settings. RMS did not
invent a second authoritative identity and must not.

## 2. Station code — a label, never a key

`StationCode` turns `TA-NODE-E368E222403F` into `E222-403F`.

- **Deterministic.** A slice of the id, not a hash and not a counter. An
  operator who wrote a code on masking tape last week finds the same code
  today.
- **Never persisted.** Nothing is stored under it and nothing is looked up by
  it. The full `nodeId` remains the only key.
- **Collision-safe.** `codesFor()` takes the whole set of stations at once. If
  any two collide at the short length, *every* code in the set grows together
  until they are distinct — a range reading two different formats is its own
  kind of confusing. If they still collide at full length they are the same
  station, which cannot happen by construction.

It is a plain function over a string rather than an object, precisely so nobody
can store one by accident.

## 3. Commissioning

Two workflows, and only the first is needed for the first field test.

**Workflow A — one tablet at a time.** Power up one unknown station, assign the
one new station code to its physical lane, repeat. Requires no change on the
tablet and works today. This is the field-test procedure.

**Workflow B — station-code matching.** Read the code off the tablet's own
screen and match it. **The target application does not display its nodeId
anywhere.** See §7.

**The tablet does not own the lane assignment.** The station says what it *is*;
RMS decides where it *stands*. That is what lets the same tablet move to another
lane later without touching the tablet.

Replacement is deliberately two acts: `clearLane()` then `assignNodeToLane()`.
`assignNodeToLane` **refuses** an occupied lane and says by whom. Silently
displacing a station would be the surprising and dangerous behaviour.

A station whose AppData was wiped comes back with a new `nodeId` and is treated
as a **new unassigned station**, never assumed to be the old one.

## 4. The event recorder

`FieldTestRecorder` — an RMS-local, append-only **JSONL** diary.

- **It is not the node's SessionStore**, not a competition record, and not an
  adjudication log. Starting or stopping it does nothing to any target.
- **One JSON object per line.** A single large JSON document becomes
  unparseable the moment the process dies mid-write — which is exactly the run
  whose evidence matters most. With JSONL a crash costs at most the final line,
  and `allEvents()` skips a truncated tail and keeps everything before it.
- **Buffered, flushed on a timer**, never inside the UDP read. A twenty-lane
  range must not wait on a disk.
- **The view is bounded** (`kUiEventCap`), the file is not. A QML list holding
  a whole range day would grow without limit.

### Transitions, not heartbeats

`nodeChanged` fires on every heartbeat. `FieldTestService` keeps a snapshot per
node and records only what *changed*: went offline, came back, restarted,
target connected, a new worst unseen gap.

The unseen counter needed particular care. `shotsAcceptedByNode` rides the two
second status heartbeat while shot events arrive as they happen, so the
difference flaps by one continuously during normal shooting. The recorder logs
only a **new worst gap** or a **full reconciliation**, and the summary says in
words when RMS has seen shots newer than the station's last heartbeat — because
otherwise somebody will file that as a bug.

## 5. The evidence bundle

`summary.txt` · `summary.json` · `range-snapshot.json` · `lane-mappings.json` ·
`node-summary.json` · `diagnostics.json` · `events.jsonl` · `shots.csv` ·
`README.txt`

- **Machine-readable as well as human-readable**, so a later analysis does not
  have to scrape a text report.
- **No source, no repository paths, no test material.** Asserted by test.
- **Unseen shots are counted, never fabricated.** The summary states
  "N individual impacts unavailable to RMS" rather than inventing rows.
- **Scores are transported, never recomputed.** `shots.csv` names the column
  `authoritativeScore`.
- **Mode is stamped everywhere.** A DEMO bundle says `simulated: true` and
  carries `*** SIMULATED / DEMO DATA ***` at the top of the human summary. It
  can never be read as physical qualification evidence.
- **`physicalShotRegistrationVerified` is hard-coded false.** Software cannot
  set it; only the physical checklist can.

## 6. Preflight wording

The verdict is `OBSERVATION PREFLIGHT COMPLETE`, never `RANGE READY`.

RMS cannot command a station and cannot certify one. A phrase implying
competition readiness would be a claim the product is not entitled to make.
States are PASS / WARNING / FAIL / WAITING, and "no stations yet" and "nothing
commissioned yet" are **WAITING** — the normal state of a range five minutes
before a test. A red line there would teach an operator to ignore red lines.

A listener that could not bind is a **FAIL**, stated in the loudest terms the
page has. An operator who cannot tell a dead socket from quiet tablets will
spend the morning blaming the tablets.

## 7. Node-side station identity — NOT IMPLEMENTED

**Audited: the target application does not expose its `nodeId` to the
operator.** `NodeIdentity` mints and persists it; no QML reads it, and no
context property carries it.

**It was deliberately not added, three days before the field test.** The code
would be small — a presentation-only screen. The risk is not the code: it is
building, testing, packaging and deploying a **new target-application build to
every tablet** immediately before the first multi-lane test. That trades a
convenience for the stability of the thing being tested.

Workflow A needs no tablet change and is sufficient.

If it is wanted later, the constraints are:

- a dedicated branch cut from the shared foundation HEAD, never the protected
  foundation directly, and never from an RMS branch
- **presentation only**: it may *show* station id, nodeId, app version, local
  IP, target connection and boot id
- it must **not** change nodeId generation, telemetry, scoring, target control,
  competition state, or protocol v1

Meanwhile the `nodeId` is readable on a tablet without any code change: it is
in the target application's own settings under the key `rms/nodeId`.

## 8. Future — IDENTIFY_STATION, documented and NOT implemented

The obvious next convenience: select a lane in RMS, and the tablet shows
`IDENTIFYING — LANE 4` in large type for a few seconds. It would make
commissioning trivial.

**It is not implemented and must not be improvised.** It is a command, and RMS
has no command channel. Building a one-off UDP message "just for convenience"
would create an unaudited, unacknowledged control path into the target
application — precisely the thing every milestone so far has refused. It waits
for a designed, acknowledged command protocol.

## 9. Future — the Incident Centre

The field-test event recorder is infrastructure a jury/incident timeline could
later build on. Two rules if that happens:

1. **Raw observations and adjudication decisions must stay distinct.** What RMS
   saw is evidence; what a jury decided is a ruling. Merging them into one
   stream destroys the ability to say which is which.
2. **Nothing in this milestone adjudicates.** There is no delete shot, no annul,
   no cross-fire correction, no penalty and no result editing. This milestone is
   diagnostic.

## 10. Still read-only

No START, STOP, RESET, MATCH, SIGHTING, POSITION CHANGE, FEED, PAUSE, RESUME or
LOAD_MATCH. No use of UDP 7756. `tst_readonly` still gates every authored RMS
file, and `tst_field_test` additionally scans the new service's meta-object for
command-shaped method names.
