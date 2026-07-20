# EST Malfunctions & Competition Interruptions (shared)

**Cross-discipline** rules for Electronic Scoring Target (EST) failures,
range-caused interruptions, and the Jury-authorised recovery actions that
follow. Every discipline file references this document. Discipline files must
still state how these shared rules apply to that specific discipline.

See [README.md](README.md) for the source-status legend and authority
hierarchy.

---

## Document status

- **Implementation status:** ✅ **Workflow implemented (Phase E)** on top of the
  Phase A domain. `EstIncidentController` (INCIDENTS) is the application seam:
  raise (freezes the competition clock via the existing `TimerPaused`),
  Jury decisions (`EstDecisionRecorded` — accepted duration, explicit
  no-allowance tri-state, backup review), single-application `TimeCreditGranted`,
  authorised recovery phases (`RecoveryPhaseEntered`: 5-min preparation,
  unlimited recovery sighting — sighters bracketed by `sightingGrantedSeq`),
  live `TargetReassigned`, official-resume gate (journalled + `TimerResumed`;
  officials are refused at BOTH controllers while unresolved), resolution, and
  the per-session incident history. Operator UI: `IncidentWindow.qml`
  ("Range incident" in the left menu — never on the athlete face). Crash
  recovery is incident-aware: the dialog warns on an unresolved incident and
  resume keeps the clock frozen + officials blocked until authorisation.
  ONE authoritative clock: remaining = duration − elapsedRunning(pause-aware)
  + Σcredits; replay-deterministic; a credit may push remaining past the
  original duration. Score corrections remain outside this workflow (official
  correction architecture only). State v3 records the decision tri-state.
- **Rules verification status:** 📋 User-supplied ISSF rule extract. Rule
  numbers (6.11.2, 6.21.3, 6.21.4) were provided by the project owner. **Edition,
  year, and page were not supplied** and are not verified against a published
  PDF in this repo.
- **Last reviewed:** 2026-07-19
- **Applicable ISSF rulebook edition:** ⏳ not supplied

---

## Scope of these rules — athlete-caused vs range-caused

These rules apply to **range-caused / EST / system** failures (the range, the
target matrix, the scoring computer, the network, the application, the
computer, power). They do **not** grant time for **athlete-caused** malfunctions
(the athlete's own equipment/rifle/pistol), which follow separate ISSF
malfunction rules not covered here.

- 🛠 **Implementation decision:** The incident model records an `incidentType`;
  athlete-caused malfunctions may be recorded for the report but **must not**
  auto-grant range-failure time credit. ⏳ The exact athlete-malfunction
  allowance rules are *awaiting official rule confirmation* and are out of scope
  for this task.

---

## 1. General interruption rule — ISSF Rule 6.11.2 📋

When an EST system or range-caused technical failure interrupts competition:

### Interruption **under 3 minutes**
- **No** additional time credit is granted.
- The competition clock resumes from its **frozen remaining value**.
- The interruption time itself must **not** consume the athlete's match time.

### Interruption **over 3 minutes**
- The athlete or affected relay **may** receive time credit equal to the **exact
  duration of time lost**.
- The credit must be **explicitly authorised by the Jury**.

### Interruption **over 5 minutes** (when authorised)
The affected athlete or relay receives:
- Time credit equal to the exact duration lost, **and**
- **5 minutes** of additional preparation time, **and**
- An additional period for **unlimited sighting shots**, **and**
- Continuation of official match shots **only after** the authorised
  preparation and sighting period is completed.

The **same** preparation + sighting procedure applies when the athlete is moved
to another target matrix.

> ⏳ **BOUNDARY — Awaiting official rule confirmation.**
> The wording gives "under 3", "over 3", "over 5". The behaviour at **exactly
> 3:00** and **exactly 5:00** is not specified by the supplied text.
> This is deliberately **not** resolved in code. Tests must treat the boundary
> as an explicit unknown (see *Automated test requirements* below), and the UI
> must present the calculated duration to the Jury rather than auto-classifying
> a value that lands exactly on a threshold. Do **not** assume `>=` or `>`.

---

## 2. Single target matrix failure — ISSF Rule 6.21.3 📋

For an **individual** target failure:
- The Range Officer and Jury verify the failure.
- Backup logs or independent target data **may** be reviewed.
- The target **may** be repaired.
- The athlete **may** be moved to a reserve target.
- **Only the affected athlete** receives the applicable recovery action.
- Time credit is based on the **officially accepted** time lost.
- If the interruption exceeds 5 minutes, **or** the athlete is moved, the
  5-minute preparation + unlimited-sighting procedure must be available.

The software must **distinguish this from a relay-wide failure** (scope =
`IndividualTarget`).

---

## 3. Central computer / network failure — ISSF Rule 6.21.4 📋

For a **central scoring computer or target-network** failure:
- The Range Officer commands **STOP**.
- The interruption time is **recorded**.
- All affected athletes **or the whole relay** are paused.
- The competition clocks **freeze**.
- The system is repaired or restarted.
- The applicable lost time is authorised **for all affected athletes**.
- Where required, a 5-minute preparation period + unlimited sighting shots occur
  **before** the official match resumes.

The architecture must support incident scope of:
- one affected session,
- selected affected firing points,
- an entire relay.

Do **not** hardcode the incident as a single-athlete event.

---

## 4. Jury authority

The **Jury has ultimate authority** over:
- time credit,
- additional preparation time,
- additional sighting shots,
- movement to a reserve target,
- acceptance of backup target data,
- continuation of the relay.

The application **must support** the Jury's decision and **must not silently
make** the final Jury decision. The UI may **recommend** an action (based on the
calculated duration and the rule thresholds) but every credit/allowance/move
requires an **explicit authorised action**, and that action is **journalled**
with the responsible official.

---

## 5. The five distinct timer/allowance concepts

These must **never** be represented by one ambiguous timer value:

1. **Restored competition time** — the frozen `remainingCompetitionTime`
   recomputed from the valid journal at the point of the last valid event. The
   duration the application was unavailable must **not** count against the
   athlete.
2. **Interruption duration** — start time, service-restored time, official
   accepted duration, scope, reason, target/relay affected. Estimated by the
   system; **confirmed or corrected by the Jury/official**. Must **not** rely
   only on process monotonic time across a reboot — use a **persisted UTC
   wall-clock** timestamp for the outage interval, while retaining monotonic
   timestamps for in-session ordering and elapsed competition timing.
3. **Jury-authorised time credit** — a distinct, journalled action (e.g.
   `TimeCreditGranted { durationMs, authorisedBy, reason, incidentId }`). Never
   auto-applied merely because a threshold was crossed.
4. **Additional preparation time** — the 5-minute allowance, represented as a
   **distinct recovery phase**, only when authorised.
5. **Additional sighting period** — unlimited **post-interruption** sighters,
   a distinct recovery phase.

### Recovery sighters are NOT original sighters
Post-interruption sighting shots must be **identifiable as such** and must
**not**:
- increment the official match-shot count,
- affect the official total,
- affect qualification ranking,
- alter the next official shot number.

They must never be mixed with the athlete's original pre-match sighters.

---

## 6. Conceptual recovery flow (over-5-minute / target-move case)

```
Match interrupted
  → incident verified (Range Officer + Jury)
  → recovery authorised
  → 5-minute preparation phase          (distinct phase; authorised)
  → unlimited sighting phase            (recovery sighters, tagged)
  → official continuation authorised
  → restored match clock resumes        (frozen remaining + authorised credit)
```

For under-3-minute interruptions the flow collapses to: *incident recorded →
resume from frozen remaining, no credit.*

---

## 7. Generic EST incident record

A **single generic incident model**, usable by **every** discipline. It must
contain **no** Air Rifle / Air Pistol / Prone / Finals logic.

Fields (suggested):

- incident ID
- session ID
- discipline
- firing point
- relay identifier (where applicable)
- incident type (see list)
- scope: `IndividualTarget` | `SelectedFiringPoints` | `RelayWide`
- interruption start (UTC wall-clock)
- system-restored time (UTC wall-clock)
- calculated duration
- officially accepted duration
- target moved: yes/no
- original target
- reserve target
- backup score reviewed: yes/no
- time credit granted (ms)
- preparation period granted: yes/no
- sighting period granted: yes/no
- authorised official
- Jury note
- Range Officer note
- incident report reference
- status (open / awaiting-authorisation / resolved / abandoned)

**Incident types** (generic):
`target-not-registering`, `continuous-target-fault`, `scoring-computer-failure`,
`target-network-failure`, `application-crash`, `computer-crash`, `power-failure`,
`communication-failure`, `target-move`, `other-EST-failure`.

---

## 8. Journalled events for the EST workflow

**Implemented (M3 Phase A)** — 4 generic events, discipline-neutral. After a
catalogue audit the proposed ~10 events were consolidated to the minimum
genuinely-missing set (`src/reliability/events/EventTypes.h`):

| Event | Status | Purpose |
|---|---|---|
| `EstIncidentRaised` | ✅ implemented | Opens an incident: type + scope + UTC start + firing point/relay. |
| `TimeCreditGranted` | ✅ implemented | Distinct authorised credit (`durationMs`, `authorisedBy`, `reason`, `incidentId`). Recorded only — never auto-applied to a clock. |
| `RecoveryPhaseEntered` | ✅ implemented | One parameterized event = the 5-min prep, the unlimited recovery-sighting period, **and** the official-resume gate (`phase` enum). |
| `EstIncidentResolved` | ✅ implemented | Closes the incident: officially-accepted duration, status, target-move + reserve target, backup-reviewed, Jury/RO notes, report ref. |

Consolidation decisions (anti-proliferation):
- **`EstIncidentScopeChanged`** — *deferred*. Scope is a field on
  `EstIncidentRaised`; a standalone re-scope event is added only when a real
  need appears.
- **`TargetReassigned`** — *folded* into `EstIncidentResolved`
  (`targetMoved` + `originalTarget` + `reserveTarget`).
- **`RecoveryPreparation*` / `RecoverySighting*` / `OfficialResumeAuthorised`**
  — *merged* into the single `RecoveryPhaseEntered` (phase enum); completion is
  implied by the next phase entered.
- **Recovery sighter shots** — *reuse* the existing `SighterAccepted`
  (`shotNumber == 0`, already excluded from the official total/count/next-shot
  number); bracketed by `RecoveryPhaseEntered{Sighting}`.
- **Athlete-equipment malfunction** — *reuse* the existing
  `EquipmentMalfunctionRecorded`, kept separate from range-caused EST incidents.

Each implemented event has: deterministic serialization + registry entry +
reducer handling + replay (folds generically) + snapshot-compatibility (state
`v1→v2`, backward-tolerant) + tests. State snapshot format bumped to **v2**
(adds `estIncidents`); v1 snapshots still deserialize (empty incidents).

---

## 9. Official reporting support

Every EST failure that results in an adjustment must be journalled with enough
data to support the official **Range Incident Report** and scoring-office
record. At minimum preserve: exact adjustment granted, reason, affected
athlete/relay, approving official, date/time, affected target, interruption
duration, preparation allowance, sighting allowance.

- 🛠 **Implementation decision:** Do **not** attempt to generate an official
  ISSF Form IR — its exact layout is not available (⏳). Store the data for a
  future report-export feature.

---

## 10. Recovery-UI requirements (incident-aware)

The unfinished-match dialog must show discipline, athlete, firing point, phase,
official shots completed, score, remaining competition time, journal health, and
**whether an unresolved EST incident exists / whether Jury action is required
before resuming**.

**Ordinary "Resume Match" must not bypass an unresolved incident** that requires
official confirmation. Normal crash recovery (no incident) stays a one-click
path; the advanced incident workflow is separated from it.

Nothing in the "do-not-auto-award" set may be applied without an authorised,
journalled confirmation: exact lost-time credit, 5-minute prep, unlimited
recovery sighters, target reassignment, backup-score acceptance, relay-wide
application.

---

## Automated test requirements (shared boundaries)

Interruption-boundary tests (must be explicit about the threshold, not hidden):

- under 3 minutes → no credit
- **exactly 3:00** → ⏳ **flagged unknown** (assert the code does not silently
  classify; assert Jury prompt)
- over 3 minutes → exact credit available (Jury-authorised)
- **exactly 5:00** → ⏳ **flagged unknown**
- over 5 minutes → credit + 5-min prep + unlimited sighting available
- scope: individual target / selected points / relay-wide
- no adjustment authorised / exact credit authorised / prep authorised /
  recovery sighters authorised / target move / resolved without move
- **Jury decision survives crash and replay** (the authorised action is a
  journalled event; replaying the journal reproduces it exactly)

---

## Open questions

1. ⏳ Exact interpretation of the **3:00 and 5:00 boundaries** (strictly `>` vs
   `>=`). Blocks final boundary-test assertions.
2. ⏳ ISSF **edition / year / page** for 6.11.2, 6.21.3, 6.21.4 (to upgrade 📋 →
   ✅).
3. ⏳ **Athlete-caused** malfunction allowance rules (explicitly out of scope
   now; recorded so it is not forgotten).
4. ⏳ Whether the 5-minute preparation allowance is itself **interruptible** and
   how it recovers if the app crashes *during* the recovery-prep phase.

## Change history

| Date | Change | Source | Components |
|---|---|---|---|
| 2026-07-19 | Created shared EST malfunction reference from supplied 6.11.2 / 6.21.3 / 6.21.4 extracts. | Project owner (📋) | Incident model, EST events, recovery UI |
| 2026-07-19 | M3 Phase A: implemented the generic EST incident domain — 4 events (`EstIncidentRaised`/`TimeCreditGranted`/`RecoveryPhaseEntered`/`EstIncidentResolved`), reducer-owned `EstIncidentRecord`, state v2, `tst_incidents.cpp`. No auto-allowance. | Implementation (🛠) | events, reducer, SessionState, tests |
| 2026-07-21 | **Phase E: operator workflow implemented** (service + UI + resume gating + incident-aware recovery). 2 new events (`EstDecisionRecorded`, `TargetReassigned`); state v3; boundary cases (exactly 3:00/5:00) surface for manual Jury decision — still no auto-classification. Manual drills: AR10 full workflow incl. crash mid-sighting; AP10 no-allowance; PRONE50 reassignment; FINAL3P regression. | Implementation (🛠) | INCIDENTS service, IncidentWindow, controllers, RecoveryDialog |
