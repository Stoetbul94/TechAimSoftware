# 50 m Rifle 3 Positions — Qualification

> # ⛔ RULES INCOMPLETE — DO NOT IMPLEMENT DISCIPLINE-SPECIFIC RECOVERY YET
>
> The official current rules for **50 m Rifle 3 Positions Qualification** have
> **not** been supplied. Do not implement persistence, reducer coverage, or a
> recovery restorer for this discipline until the rules below are provided.
> **Do not infer them from the 3P Final or from another discipline.**

See [README.md](README.md) for the legend and authority hierarchy, and
[est-malfunctions.md](est-malfunctions.md) for shared EST rules.

## Document status

- **Implementation status:** 🔧 Some 3P qualification scoring exists in legacy
  QML (gated on `is3PMatch`; see [docs/3p-discipline.md](../3p-discipline.md)),
  but **not** validated against current official rules and **not** journalled /
  recoverable.
- **Rules verification status:** ⏳ **INCOMPLETE — awaiting official rules.**
- **Last reviewed:** —
- **Applicable ISSF rulebook edition:** ⏳ not supplied
- **Supported by TechAim:** Partial scoring only; **not** recoverable.

## ⚠️ Known conflict to resolve before implementation

The current application shows **90 minutes** for 50 m Rifle 3 Pos on EST
(`LoginPage.getMatchTime()`), and legacy 3P material references both **120-shot**
and **60-shot** courses of fire in different places. **Do not rely on old
120-shot or newer 60-shot assumptions.** The supplied-rules requirement is that
the official current course of fire be confirmed. Report any conflict between the
existing application and the supplied rules **before** changing behaviour.

## Missing rules required before implementation

Provide the official current rules for:

1. **Course of fire** — total shots and how distributed across positions.
2. **Official shot count** — (120-shot vs 60-shot must be confirmed, not
   assumed).
3. **Position sequence** — order of Kneeling / Prone / Standing.
4. **Timing** — total match time and/or per-position timing.
5. **Preparation & sighting structure** — combined vs per-position; durations.
6. **Position-change timing** — change-over allowance between positions.
7. **New sighting periods** — whether a new sighting period applies before each
   position.
8. **Interruption behaviour during a position** — resume-from-frozen vs restart.
9. **Interruption behaviour during change-over.**
10. **Scoring method** — decimal vs integer (and any per-position differences).
11. **Completion and ranking structure.**

## Sections deliberately left unfilled

The remaining template sections (Event overview, Scoring rules, Match phases,
Timing, Preparation and sighting, Official shot sequence, EST effects, Recovery
requirements, Journal events, Reducer state, Restorer ownership, tests) are
**intentionally blank** pending the rules above. Do not fill them with
assumptions or outdated rules.

## Open questions

All eleven items above.

## Change history

| Date | Change | Source | Components |
|---|---|---|---|
| 2026-07-19 | Placeholder created; listed the exact missing rules and the 90-min / 120-vs-60-shot conflict to resolve. | Project owner instruction | — |
