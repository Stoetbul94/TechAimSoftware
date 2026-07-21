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

> ⚠️ Most rules here are 📋 (project-owner-supplied, not yet diffed against a
> published PDF). **Exception:** the **10m Finals** files
> ([10m-finals-shared.md](10m-finals-shared.md),
> [10m-air-rifle-final.md](10m-air-rifle-final.md),
> [10m-air-pistol-final.md](10m-air-pistol-final.md)) carry the **✅ tag** —
> verified 2026-07-21 against the official ISSF Rule Book **Edition 2025 (Second
> Print 07/2026), Effective 1 July 2026**, rule 6.17.2 (pp. 265–272), read
> section-by-section from the source PDF. The remaining discipline/EST files are
> still 📋; upgrade them to ✅ when diffed against the same edition.

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
| [10m Air Rifle Final](10m-air-rifle-final.md) | ❌ Not implemented (F0 docs) | ❌ No | ❌ No | ❌ No | ✅ **Yes — verified** | 2026-07-21 | Single vs 8-athlete scope; HQ command timings |
| [10m Air Pistol Final](10m-air-pistol-final.md) | ❌ Not implemented (F0 docs) | ❌ No | ❌ No | ❌ No | ✅ **Yes — verified** | 2026-07-21 | Single vs 8-athlete scope; **final is decimal** |
| [Shared: 10m Finals](10m-finals-shared.md) | ❌ Not implemented (F0 docs) | ❌ No | ❌ No | ❌ No | ✅ **Yes — verified (6.17.2)** | 2026-07-21 | See architecture gap report |
| [10m Air Rifle Qualification](10m-air-rifle.md) | ✅ Scoring + persistence + recovery | ✅ Yes (B1) | ✅ Yes (C) | ✅ **Yes (D1)** — EST workflow = Phase E | 📋 Yes | 2026-07-20 | EST boundary at exactly 3:00 / 5:00 |
| [10m Air Pistol Qualification](10m-air-pistol.md) | ✅ Scoring + persistence + recovery | ✅ Yes (B2, integer) | ✅ Yes (C) | ✅ **Yes (D2)** — EST workflow = Phase E | 📋 Yes | 2026-07-20 | EST boundary at exactly 3:00 / 5:00 |
| [50m Rifle Prone](50m-rifle-prone.md) | ✅ Scoring + persistence + recovery | ✅ Yes (B3, decimal) | ✅ Yes (C) | ✅ **Yes (D3)** — EST workflow = Phase E | 📋 Yes | 2026-07-20 | EST boundary at exactly 3:00 / 5:00 |
| [50m Rifle 3 Positions Qualification](50m-rifle-3p-qualification.md) | 🔧 Scoring only | ❌ No | ⚠️ Partial (generic) | ❌ No | ⏳ **INCOMPLETE** | — | Course of fire, timing, position structure |
| [25m Pistol](25m-pistol.md) | ❌ No | ❌ No | ❌ No | ❌ No | ⏳ **INCOMPLETE** | — | Entire discipline unmodelled |
| [Shared: EST malfunctions](est-malfunctions.md) | ✅ Domain + operator workflow (Phase E) | ✅ events+reducer (A+E) | ✅ `EstIncidentRecord` v3 | incident-aware (E4) | 📋 Yes | 2026-07-21 | 3:00 / 5:00 boundary interpretation |

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

**Recovery dispatch (Phase A + D):** on resume, `main.qml`'s `dispatchRecovery()`
selects the discipline restorer from the recovered session's stable
`disciplineId`. `FINAL3P` → the (unchanged) `Finals3PController` restorer;
`AR10`/`AP10`/`PRONE50` → the shared qualification restorer
(`enterQualificationMode(config, startFresh)` + `restoreQualificationSession`,
Phase D1–D3); `3P50` returns a clear "recovery not yet implemented" result and
unknown disciplines fail safe — there is **never** a silent fallback into the
Finals restorer.

---

## Change history

| Date | Change | Source | Components |
|---|---|---|---|
| 2026-07-19 | Created rules reference: README, EST malfunctions, AR10/AP10/Prone50 populated, 3P Final populated, 3P-Qual + 25m Pistol placeholders. | Project owner (📋) | docs/issf-rules/* |
| 2026-07-19 | M3 Phase A landed: recovery dispatcher + generic EST incident domain (4 events, reducer record, state v2). Docs synced. | Implementation (🛠) | main.qml, reliability layer, tests |
| 2026-07-19 | Phase B0: shared qualification persistence seam (`QualificationController`/`QUAL`). B1: 10m Air Rifle live scoring journalled (persistence ✅, recovery restore = Phase D). | Implementation (🛠) | src/qualification, ShootingPage, tests |
| 2026-07-20 | Phase C (replay readiness + timer anchors) and Phase D1–D3 (AR10/AP10/PRONE50 crash recovery via the shared qualification restorer). EST workflow remains Phase E. | Implementation (🛠) | QUAL, ShootingPage/CenterPane, main.qml, RecoveryDialog |
| 2026-07-21 | Phase E: EST incident + Jury workflow (E0–E5) — service, operator UI, allowances, recovery phases, resume gating, incident-aware crash recovery, per-session history. EST rules edition/page still pending (📋). | Implementation (🛠) | src/incident, IncidentWindow, reliability layer |
| 2026-07-21 | Phase F0: 10m Air Rifle & Air Pistol **Final** rules + architecture package (docs only, no code). Rules ✅ **verified** against ISSF Rule Book Edition 2025 (Second Print 07/2026), rule 6.17.2, pp. 265–272 — 8 finalists, 2×5 (250 s) + 14 singles (50 s) = 24 shots, **decimal both AR & AP**, eliminations from shot 12, shoot-off ties. Architecture recommends one `Finals10mController` (rifle/pistol config); single-vs-8-athlete scope is the blocking gap. | Rules ✅ + Architecture (🛠) | docs/issf-rules/10m-*-final.md, docs/10m-finals-architecture.md |
