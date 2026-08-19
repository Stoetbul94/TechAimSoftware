# RMS Milestone 4.5 — Target Display MVP

Status: **implemented** on `feature/rms`.
Scope: the Displays page shows what the target stations already decided. It
adds no authority, no arithmetic and no transmission.

---

## 1. What this milestone is

A range needs a screen. Until now RMS could tell an operator that a station was
answering and what it had scored, but not *where the shots went*. This
milestone draws the impacts.

It draws them from telemetry RMS already receives. It introduces:

- a target face renderer (`RmsTargetView.qml`) shared by every surface,
- an ALL TARGETS overview and a SINGLE TARGET view,
- PREVIOUS / NEXT lane navigation with wrap-around,
- FULL SCREEN presentation for a range monitor, projector or TV,
- AUTO ROTATE through the lanes at a chosen interval,
- honest presentation of what RMS did *not* see.

## 2. The one rule this milestone must not break

**RMS does not score.**

The display maps a shot's `xMm`/`yMm` onto a face so a human can see the group.
It never maps that position back to a value. There is no function anywhere in
the RMS tree that takes a coordinate and returns a score, and none that takes a
score and invents a coordinate.

Concretely:

| Value | Where it comes from |
|---|---|
| Shot position on the face | `xMm`, `yMm` from `shot.accepted`, scaled by the face radius |
| The shot's score | `authoritativeScore` from `shot.accepted`, transported verbatim |
| The lane's running total | `totalScore` from `node.status` — the STATION's total |
| Shots taken | `shotsAccepted` from `node.status` — the STATION's count |

`TargetGeometry` carries a banner saying it is not scoring, and its header
comment says why: ring radii exist to *draw rings*, and the moment a radius is
compared against a shot to produce a number, RMS has become a second scoring
authority and the two will disagree in front of a jury.

The RMS tree was grepped after implementation for score-from-geometry
arithmetic. The only radius/`hypot` uses are: painting rings, deciding whether
a shot is on the face, and holding an off-face shot at the edge.

## 3. Target geometry

`src/rms/TargetGeometry.{h,cpp}` — QtCore only, no GUI, unit-testable.

Ring radii follow `r(k) = tenRingRadiusMm + (10 - k) * ringStepMm`. The
standards table mirrors `CompetitionCatalogue.qml` / `IssfTargetCanvas.qml` in
the foundation so the two products draw the same face:

| `targetStandardId` | 10-ring | step | outermost drawn | black to ring |
|---|---|---|---|---|
| `issf.10m.air-rifle` | 0.25 mm | 2.5 mm | 4 | 4 |
| `issf.10m.air-pistol` | 5.75 mm | 8.0 mm | 4 | 7 |
| `issf.50m.rifle` | 5.2 mm | 8.0 mm | 4 | 5 |
| `issf.50m.pistol` | 5.2 mm | 8.0 mm | 4 | 5 |

Rules the implementation holds to:

- **An unsupported standard is not silently substituted.** `specFor()` returns
  `supported = false` and the view draws a labelled placeholder. Drawing an air
  pistol face for an unknown 300 m standard would be a lie with a picture.
- **The y axis flips exactly once.** Telemetry y is up-positive; screen y is
  down-positive. `normalise()` performs the flip, and every renderer consumes
  normalised coordinates. This matches the foundation's own renderers
  (`py = cy - scale * y`). *Confirmation against real hardware output is still
  outstanding* — see §9.
- **A wild shot is held at the edge, never dropped.** `normaliseClamped()`
  keeps an off-face impact on the rim and flags it, because a shot that
  vanishes reads as "no shot".
- Positions are normalised to a **fraction of the face radius**, so a 120 px
  card and a 900 px full-screen view place the same shot at the same relative
  point. Only pixels differ.

## 4. Display state

`src/rms/DisplayController.{h,cpp}` owns presentation state and nothing else.
Choosing what to look at sends nothing; no station can tell the difference.

- `DisplayMode` — `AllTargets`, `SingleTarget`, `RotateTargets`.
- `LaneFilter` — `AllPhysical` (every configured lane) or `Participating`
  (lanes in the plan). A filter changes what is on screen; it never removes a
  lane from the range. When a plan has lanes, `Participating` is auto-selected
  until the operator chooses for themselves.
- `laneOrder()` is the **single** ordered set. Previous, next and rotation all
  walk it, so they can never disagree about what "next" means. It wraps.
- Rotation stops on any deliberate action — selecting a lane, previous, next,
  ALL TARGETS, or leaving the page. A display that fights the operator is worse
  than one that does not rotate.
- The interval is clamped to 2 s … 10 min.
- `tickRotation(nowMs)` takes the time as an argument and the clock is
  injectable, so rotation is tested deterministically rather than by sleeping.

**`laneOrderList` is a NOTIFYING property, not only an invokable.** A
`Q_INVOKABLE` cannot drive a live QML model: nothing tells the binding to
re-read it, so a lane strip bound to the invokable is filled once — with the
empty pre-configuration order — and never again. That was a real defect found
by looking at a screenshot, and `tst_target_display.cpp` now asserts the
property exists, notifies, and is read-only.

## 5. What is drawn per lane

`src/rms/DisplayLaneModel.{h,cpp}` is the one place a lane's display data is
assembled, so the overview tile and the large view can never drift apart.

- Shot history is bounded at `kVisibleShots = 30`. A 60-shot match therefore
  shows the most recent 30 impacts. This is a display bound, not a record: the
  station's totals are shown unchanged beside it.
- `nodeTotalLabel` is the **station's** total and is what the big number shows.
  `observedTotalLabel` is RMS's sum of what it actually received and appears
  only inside the unseen-shot warning, where it is explicitly labelled.
- Planned and observed athlete/programme are carried separately, with
  `athleteMismatch` / `programmeMismatch` flags. The display states the
  disagreement; it changes neither side.

## 6. Shots RMS did not see

UDP loses datagrams and stations go offline. When
`shotsAccepted > observedShotCount`, the display says so:

- the overview tile footer shows `⚠ N unseen`,
- the single view shows a warning panel: how many impacts are drawn, how many
  the station accepted, and both totals side by side.

RMS does **not** interpolate a missing shot, and does not present its own
observed sum as the score. A face with gaps in it that says so is honest; a
face with invented shots is not.

## 7. Terminal competition states

FINISHED and ELIMINATED are the third axis defined in
`rms-finals-elimination-display.md` — independent of node health and target
health. The display:

- overlays the state on a **solid panel** over a dimmed face. Ring numbers
  reading through the words made the caveat unreadable, and a caveat that
  cannot be read is not a caveat.
- keeps the face visible behind — dimmed, not deleted.
- never dims a terminal lane the way an offline one is dimmed: completing a
  course is not a fault.
- gives FINISHED and ELIMINATED different words **and** different colours.
- prints `SIMULATED STATE — no station reported this` on a filled strip
  whenever the value came from the development injection.

**Protocol v1 is unchanged.** No real station reports competition status, so
every real lane reads UNKNOWN and no overlay appears. The only way this field
moves is a deliberate v2 bump.

## 8. Read-only, still

Nothing on the Displays page transmits. There is no START, STOP, RESET, MATCH,
SIGHTING, POSITION CHANGE, FEED, PAUSE, RESUME or LOAD_MATCH control, and
`tst_readonly.cpp` still fails the build if any authored RMS file gains a
transmit call, a TCP connection, or a reference to the node's inbound control
port.

## 9. Open items — deliberately not closed

- **PHYSICAL TARGET DISPLAY TEST: NOT TESTED.** No real target hardware was
  used. All evidence is from the development simulator.
- **Coordinate convention unconfirmed against hardware.** The y-up convention
  and the millimetre unit follow the foundation's renderers, not a measured
  capture from a live station. Confirm before a range trusts the picture.
- **`issf.50m.rifle` ten-ring radius (5.2 mm)** carries the same open question
  as the foundation: it needs rulebook confirmation or physical calibration.
- **Not implemented, by instruction:** FOLLOW_LEADER, LEADERBOARD, TOP_3,
  FINALS_DIRECTOR, RANGE_STATUS_ROTATION, SMART_TV_CLIENT.
- The development simulator draws a shot's coordinates and its score from
  independent pseudo-random sequences, so in simulated runs a plotted position
  does **not** correspond to the score shown. Correlating them would require a
  coordinate→score rule, which is exactly what this product must not contain.
