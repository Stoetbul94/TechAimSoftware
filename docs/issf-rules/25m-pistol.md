# 25 m Pistol

> # ⛔ RULES INCOMPLETE — DO NOT IMPLEMENT DISCIPLINE-SPECIFIC RECOVERY YET
>
> **25 m Pistol is not yet sufficiently modelled** in TechAim. There is no
> `Discipline` enum entry, no event coverage, no reducer state, and no
> implementation. Do not implement persistence, reducer, recovery, UI, or tests
> for it until the rules below are supplied.
>
> **This file refers to 25 m Pistol only.** Do **not** implement or document the
> separate **25 m Rapid Fire Pistol** discipline — it is explicitly excluded from
> this and all related work.

See [README.md](README.md) for the legend and authority hierarchy, and
[est-malfunctions.md](est-malfunctions.md) for shared EST rules.

## Document status

- **Implementation status:** ❌ Not modelled (no enum entry, no events, no
  reducer, no UI).
- **Rules verification status:** ⏳ **INCOMPLETE — awaiting official rules.**
- **Last reviewed:** —
- **Applicable ISSF rulebook edition:** ⏳ not supplied
- **Supported by TechAim:** ❌ No.

## Missing rules required before implementation

Provide the official rules for **25 m Pistol** (not Rapid Fire):

1. **Exact event format** — full course of fire.
2. **Precision-stage structure** — shots, series, timing.
3. **Timed/rapid-stage structure for this event** — series, exposure timing.
4. **Series count** and **shots per series**.
5. **Command sequence** — the range commands and their order.
6. **Target exposure timing** — exposure/turn durations.
7. **Scoring** — integer vs decimal; how the stages aggregate.
8. **Preparation & sighting rules.**
9. **Allowable malfunction and EST interruption procedures** (how the shared EST
   rules and any discipline-specific malfunction/refire rules apply).
10. **Interrupted-series continuation or refire rules** — critical: whether an
    interrupted timed series is continued, re-fired, or scored as-is.

## Prerequisite modelling work (blocked)

Before any recovery work, this discipline needs (pending rules):
- a `Discipline::Pistol25m` enum entry + id mapping,
- an event/timing model for its stage/series/exposure structure,
- reducer state for stage/series progression,
- a discipline restorer.

None of this may be started until the rules above are supplied.

## Sections deliberately left unfilled

All remaining template sections are **intentionally blank** pending the rules
above. Do not fill them with assumptions.

## Open questions

All ten items above, plus: does 25 m Pistol require a dedicated C++ controller
(like Finals) or can it run through the ShootingPage restorer? (Decidable only
once the command/exposure timing model is known.)

## Change history

| Date | Change | Source | Components |
|---|---|---|---|
| 2026-07-19 | Placeholder created; listed the exact missing rules. Excludes 25m Rapid Fire Pistol. | Project owner instruction | — |
