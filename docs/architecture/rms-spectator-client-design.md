# RMS spectator / smart-TV client — architecture note

Status: **design note. Deliberately NOT built.**
Written during Milestone 4.5 so the target display does not accidentally
foreclose it.

---

## 1. Why write this now

The Displays page currently renders inside `TechAimRMS.exe` and drives a
second monitor from the same process. That is right for a range office with a
projector next to it. It does not extend to what a competition actually wants:
screens in the spectator area, a screen behind the firing point, a screen in
the club bar, a phone held by a coach — none of which will run a Qt binary and
none of which should be able to affect the range.

This note records the shape those clients must take, so that nothing built in
Milestone 4.5 has to be undone to reach it.

## 2. The constraint that decides the design

A spectator screen is the **least** trusted thing on the network and the
**most** likely to be seen by people who will act on what it says. Therefore:

- A spectator client is **strictly downstream**. It receives; it never sends.
  Not commands, not corrections, not acknowledgements that gate anything.
- A spectator client is **not** an observer of node telemetry. It must not join
  the node broadcast. If a TV in a bar can decode `shot.accepted`, then the
  broadcast's audience is unbounded and the range has no control over who sees
  what, when.
- Therefore: **RMS is the only consumer of node telemetry, and the only
  producer of spectator data.** One hop, one direction.

```
target nodes  ──(protocol v1, UDP)──▶  RMS  ──(presentation feed)──▶  spectator clients
                                        │
                                        └── operator UI (this application)
```

## 3. What a spectator feed is not

- It is **not** the node protocol re-broadcast. Node telemetry is an
  engineering contract with sequence numbers, boot ids and gap semantics.
  Spectator clients have no business reasoning about any of that.
- It is **not** a live view of RMS's internal models. Those change with every
  milestone; a display client must not break because a role was renamed.
- It is **not** a scoring source. The same rule as everywhere else in this
  product: the value shown is the station's `authoritativeScore`, transported.

A spectator feed is a **presentation document**: already-decided values, already
formatted decisions about what is on screen, with an explicit staleness stamp.

## 4. Shape of the feed

A read-only HTTP endpoint served by RMS, returning a versioned JSON document,
plus a push channel (SSE or WebSocket) carrying the same document on change.
HTTP because every candidate client — smart TV browser, Chromecast, phone,
kiosk PC, a second Qt app — can consume it without a native runtime.

Properties it must have:

- **Versioned**, with the same rule as protocol v1: unknown *fields* are
  ignored, an unknown *version* is rejected. A client one release behind must
  degrade, not lie.
- **Self-dating.** Every document carries the time RMS assembled it and the
  age of the newest telemetry behind it. A frozen screen must be able to say it
  is frozen. A spectator screen showing a stale score with no indication is the
  worst failure mode this system has.
- **Whole-state, not deltas.** A TV that was unplugged for the middle of a
  relay must recover by receiving the next document, with no replay logic and
  no sequence reasoning on the client.
- **Identified by stable ids** — `laneId`, `athleteId`, `programmeId` — with
  display text derived from them, never looked up by label (QML-LANG-001).
- **No node internals.** No `bootId`, no `statusSeq`, no gap counts, no IP
  addresses. Those belong in Lane detail → Diagnostics.

## 5. What the client chooses, and what RMS chooses

The operator decides what the range shows. A spectator client must not be able
to make the range show something else, but it also must not need an operator to
plug a keyboard into a TV.

Split it:

- **RMS decides the content** — which lanes, which athletes, which programme,
  what the competition state is.
- **The client decides the presentation** — screen size, how many tiles fit,
  language, whether it shows a leaderboard or a single lane, its own rotation.

A client requesting "lane 4" is not commanding anything: it is asking RMS's
read-only endpoint for a document it is already entitled to.

## 6. What Milestone 4.5 deliberately got right for this

- Every display value derives in **C++** (`DisplayLaneModel`,
  `TargetGeometry`), and QML only formats. A spectator feed can be assembled
  from the same layer without reimplementing a single decision.
- Shot positions are normalised to a **fraction of the face radius**, so any
  client at any resolution places a shot identically without knowing millimetres
  or ring radii.
- Target standards are identified by `targetStandardId`, so a client draws the
  right face from an id rather than from a picture RMS sends it.
- "Unseen shots" is already a first-class value rather than a silent gap, so a
  spectator screen can be honest about incompleteness instead of implying a
  complete match.

## 7. Not in scope, and why

FOLLOW_LEADER, LEADERBOARD, TOP_3, FINALS_DIRECTOR, RANGE_STATUS_ROTATION and
SMART_TV_CLIENT are all excluded from Milestone 4.5 by instruction. Each of them
needs a ranking decision, and ranking is a competition-authority question, not
a display question — the same reason RMS must never infer elimination. They
land only when the authority for them is settled, in their own milestone.
