# ISSF Rules Reference — TechAim

This directory is the **permanent, human-readable source of discipline
requirements** for TechAim. It is the authority a developer (or AI assistant)
consults before touching scoring, timing, shot counting, match phases, Finals
commands, EST malfunction handling, or recovery/resume behaviour.

It is written so it can be understood **without any prior conversation
history**.

---

## Source-status legend

Every rule statement in every file in this directory is tagged with exactly
one status. These categories must never be merged so that an implementation
decision reads as an official rule.

| Tag | Meaning |
|-----|---------|
| ✅ **Official ISSF rule verified** | Confirmed against an official ISSF rulebook (edition + section recorded). |
| 📋 **User-supplied ISSF rule extract** | Provided by the project owner as an ISSF rule; not yet independently verified against a published PDF. Treated as authoritative for implementation. |
| 🔧 **Existing TechAim behaviour** | What the current application already does. Descriptive, **not** an authority — may be wrong. |
| 🛠 **Implementation decision** | A TechAim engineering choice that fills a gap the rules do not specify. |
| ⏳ **Awaiting official rule confirmation** | Known-missing or ambiguous; must not be resolved silently in code. |

> ⚠️ As of this writing, **no rule here carries the ✅ tag**. The discipline and
> EST rules were supplied by the project owner (📋) and have not been diffed
> against a published ISSF rulebook edition/page in this repository. When an
> official PDF is provided, upgrade the relevant entries to ✅ and record the
> edition, year, and section.

---

## Authority hierarchy

When two sources disagree, the higher entry wins:

1. **Current official ISSF rules** (general regulations)
2. **Official event-specific rules** (per-discipline)
3. **Jury decisions and incident records** (for a specific match/incident)
4. **TechAim implementation decisions**
5. **Existing legacy application behaviour**

**Existing application behaviour must never override an official rule.** If the
code and an official rule conflict, the rule wins and the code is a bug to be
fixed (after reporting the conflict — never silently).

---

## Status table

| Discipline | Supported in TechAim | Persistence | Reducer | Recovery | Rules complete | Last verified | Unresolved Qs |
|---|---|---|---|---|---|---|---|
| [50m Rifle 3 Positions Final](50m-rifle-3p-final.md) | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes (M3) | 📋 Partial | 2026-07-19 | Finals-specific EST allowance mapping |
| [10m Air Rifle Qualification](10m-air-rifle.md) | 🔧 Scoring only | ❌ No | ⚠️ Partial (generic) | ❌ No | 📋 Yes | 2026-07-19 | EST boundary at exactly 3:00 / 5:00 |
| [10m Air Pistol Qualification](10m-air-pistol.md) | 🔧 Scoring only | ❌ No | ⚠️ Partial (generic) | ❌ No | 📋 Yes | 2026-07-19 | EST boundary at exactly 3:00 / 5:00 |
| [50m Rifle Prone](50m-rifle-prone.md) | 🔧 Scoring only | ❌ No | ⚠️ Partial (generic) | ❌ No | 📋 Yes | 2026-07-19 | EST boundary at exactly 3:00 / 5:00 |
| [50m Rifle 3 Positions Qualification](50m-rifle-3p-qualification.md) | 🔧 Scoring only | ❌ No | ⚠️ Partial (generic) | ❌ No | ⏳ **INCOMPLETE** | — | Course of fire, timing, position structure |
| [25m Pistol](25m-pistol.md) | ❌ No | ❌ No | ❌ No | ❌ No | ⏳ **INCOMPLETE** | — | Entire discipline unmodelled |
| [Shared: EST malfunctions](est-malfunctions.md) | Domain ✅ / workflow ❌ | ✅ events+reducer (Phase A) | ✅ `EstIncidentRecord` | n/a | 📋 Yes | 2026-07-19 | 3:00 / 5:00 boundary interpretation |

**Explicitly excluded:** 25m Rapid Fire Pistol. There is intentionally no rules
file for it. Do not create one during EST/recovery work.

Legend for the table cells: ✅ done · ⚠️ partial · ❌ not started · ⏳ blocked on
rules · 🔧 exists in legacy code only.

---

## How the reliability layer uses these files

The core reliability engine (`SessionStore`, `JournalWriter/Reader/Validator`,
`ReplayEngine`, `RecoveryCoordinator`, `SessionReducer`, `SessionState`) is
**discipline-agnostic** and contains **no** ISSF rule. All discipline rules live
here and are enforced by the per-discipline **restorers** (see each file's
*Controller or restorer ownership* section) and by the shared **EST incident
model**.

See [est-malfunctions.md](est-malfunctions.md) for the cross-discipline
interruption/malfunction rules; each discipline file references it and adds only
its discipline-specific effects.

**Recovery dispatch (M3 Phase A):** on resume, `main.qml`'s `dispatchRecovery()`
selects the discipline restorer from the recovered session's stable
`disciplineId`. `FINAL3P` is wired to the (unchanged) `Finals3PController`
restorer; qualification disciplines return a clear "recovery not yet
implemented" result and unknown disciplines fail safe — there is **never** a
silent fallback into the Finals restorer.

---

## Change history

| Date | Change | Source | Components |
|---|---|---|---|
| 2026-07-19 | Created rules reference: README, EST malfunctions, AR10/AP10/Prone50 populated, 3P Final populated, 3P-Qual + 25m Pistol placeholders. | Project owner (📋) | docs/issf-rules/* |
| 2026-07-19 | M3 Phase A landed: recovery dispatcher + generic EST incident domain (4 events, reducer record, state v2). Docs synced. | Implementation (🛠) | main.qml, reliability layer, tests |
