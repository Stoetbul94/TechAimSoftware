# Phase 2 — DSB & ISSF timing audit (read-only)

**Date:** 2026-11-15 · **Repository:** SETA worktree, branch
`feature/seta-athlete-mode` @ `441460f` · **Mode:** Stages A–C of the
research specification (discovery, ledger, proposed corrections). **No code was
changed by this audit.**

**Inputs**

| Input | Status | SHA-256 (first 16) |
|---|---|---|
| `TechAim_DSB_ISSF_2026_Specification.md` (+ .docx/.pdf) | Research draft, *not authorised for rule changes* (its own wording) | `9c227a99090f5c7d` |
| `TechAim_DSB_ISSF_2026_ResearchProfiles.json` | `normative_use_allowed: false` | `6590d7640215adc3` |
| `docs/rules/dsb-2026-*.md` (in repo) | Built from the **official DSB online Sportordnung, Stand 01.01.2026** (dsb.de flip catalogue), consulted 2026-08-18 | — |
| `docs/issf-rules/*.md` (in repo) | Partly verified against the **official ISSF Second Print 07/2026**; several files still 📋 / ⏳ | — |

**Source hierarchy used.** The repo's DSB register is the stronger DSB source
(official DSB catalogue; the research read a non-DSB mirror). For ISSF, neither
the repo nor the research has the **current Third Print (effective
2026-10-19)**. Per `CLAUDE.md`, discipline timing may only change against an
official rule that the repo records; where that is missing the change is
**blocked**, not guessed.

Scope note: SETA targets are **10 m and 50 m** only. 15 m, 25 m, 100 m and
300 m rows of the research are listed at the end as out of scope for this
product, not as defects.

---

## 1. Ledger — what the code does vs. both sources

Legend: **PASS** code = official repo source = research · **FAIL** code differs
from the rule · **BLOCKED** the official rule is not in the repo yet.

### DSB (all 13 SETA programmes) — profile path, `CompetitionCatalogue.qml`

| Programme | Code (file:line) | Code value | Repo official source | Research | Result |
|---|---|---|---|---|---|
| 1.10 LG 20 | `CompetitionCatalogue.qml:591` | 30 min, prep 15, DECIMAL | 30 (S-1.4) | 30 | **PASS** |
| 1.10 LG 40 | `:609` | 50 min | 50 | 50 | **PASS** |
| 1.10 LG 60 | `:627` | 75 min | 75 | 75 | **PASS** |
| 1.20 3×10 | `:651` | 25 / 20 / 30 per position, INTEGER | 25/20/30 incl. sighting | same | **PASS** |
| 1.20 3×20 | `:671` | 35 / 30 / 40 per position | 35/30/40 | same | **PASS** |
| 1.40 3×20 | `:696` | ONE 105 min clock, prep 15 before kneeling | 105 (S-1.4/1.5) | 105, one clock | **PASS** |
| 1.60 3×40 | `:718` | ONE 165 min clock | 165 | 165 | **PASS** |
| 1.80 prone 60 | `:740` | 50 min, DECIMAL | 50 | 50 | **PASS** |
| 2.10 LP 20 / 40 / 60 | `:761` `:779` `:797` | 30 / 50 / 75, INTEGER | 30/50/75 | same | **PASS** |
| 2.20 P 60 | `:820` | **90 min** | 90 (S-2.5) | 90, "NOT 75" | **PASS** |
| 2.20 P 30 | `:839` | 55 min | 55 *empfohlen* | 55 recommended | **PASS** (time is a recommendation — label it so, see F6) |

Prep/sighting for every DSB row is a separate 15-minute period outside the
match time (1.20: once, before kneeling) — **PASS**, research T12 holds.
Tests pinning these: `tst_rule_authority` (105 / 165 reload), `tst_dsb120`
(position clocks, recovery), DSB-SEAM-001.

### ISSF — legacy path, `AppSettings::getTimeCount()` + `LoginPage.legacyMatchTimeText()`

| Event | Code (file:line) | Code value | Repo official source | Research (Second Print) | Result |
|---|---|---|---|---|---|
| 10 m Air Rifle 60 | `appsettings.cpp:617` | 75 min, prep 15 | 75 📋 | 75 | **PASS** |
| 10 m Air Pistol 60 | `appsettings.cpp:625` | 75 min | 75 📋 | 75 | **PASS** |
| 50 m Rifle Prone 60 | `appsettings.cpp:631` | 50 min | 50 📋 | 50 | **PASS** |
| 50 m Rifle 3P 60 | `appsettings.cpp:629` | **90 min, always** | ⏳ *incomplete* | 90 **indoor** / 105 **outdoor**, must be chosen | **FAIL + BLOCKED** (F2) |
| 50 m Pistol 60 | `appsettings.cpp:625` (`game_mode == 0` → 75 for *any* pistol) | **75 min** | **no document** | **90 min** (6.11.9.8) | **FAIL + BLOCKED** (F1) |
| 3P position change | `ShootingPage.qml` p3Course | one clock, sighters inside, no reset | ⏳ | same | **PASS** (T13) |

### Practice (product policy, not a sport rule)

| Item | Code | Value | Research policy | Result |
|---|---|---|---|---|
| Match-10/20/30/40 presets | `appsettings.cpp:143` (config.ini) | 15 / 30 / 45 / 60 min | "custom countdowns never presented as certified" | **DECISION** (F5) |
| Free practice | `shootCountAndTimeMap[-1]` | **90 min** clock | untimed by default | **DECISION** (F5) |

---

## 2. Findings, classified (Stage C)

| ID | Priority | Class | Finding | Proposed correction (NOT implemented) | Blocked on |
|---|---|---|---|---|---|
| **F1** | **P0** | WRONG_VALUE | ISSF **50 m Pistol** runs 75 min: the legacy table returns 75 for every 60-shot pistol course. Research (ISSF Second Print 6.11.9.8) and DSB 2.20 both say **90**. SETA's own DSB 2.20 is already correct, so only the ISSF tile is wrong. | Give ISSF 50 m Pistol its own 90-min authority (a catalogue profile, like DSB), leaving 10 m AP at 75. Add a test pinning 90 and a negative control at 75. | Official ISSF 6.11.9.8 page recorded in `docs/issf-rules/` (new `50m-pistol.md`) |
| **F2** | **P0** | WRONG_TIMER_SCOPE / missing selector | ISSF **50 m 3P** is fixed at 90 min (indoor). Outdoor EST is 105. Nothing in the code knows the range type; research T06 requires an **unclassified range to be blocked**. | Add a lane setting `range_environment = INDOOR / OUTDOOR` (config.ini, operator-only, no default for official mode); 90 / 105 from it; refuse official 3P while unset; show "EST — INDOOR — 90:00" in the confirmation and report. | Official 6.11.9.2 text recorded in `50m-rifle-3p-qualification.md` (still ⏳) **and** SETA's written answer: is each 50 m range indoor or outdoor? |
| **F3** | P1 | RULE_SOURCE_INCOMPLETE | The repo has nothing from the **ISSF Third Print 10/2026** (current). The research flags a 3P Final change (unexpected zero in the 22-min kneeling/prone phase). The 3P Final is implemented against the Second Print. | Diff the Third Print against `50m-rifle-3p-final.md`; until then mark 3P Final "Second Print 07/2026" in its report and docs. | Third Print PDF |
| **F4** | P1 | UI_LABEL_ONLY | SETA `appsettings.cpp:629` still says "105 min is paper/outdoor". Tech Aim 1.0.0 already corrected it (the split is indoor vs outdoor, not paper vs EST). | Copy Tech Aim's corrected comment. | — (safe now) |
| **F5** | P2 | Product policy | Practice presets carry unannounced countdowns (15–60 min) and free practice a 90-min clock; the research recommends untimed free practice and explicit, non-official labelling. | Decide: (a) keep configurable times but label tiles "Training — not an official time", (b) free practice untimed. Feeds Phase 3 "My practice". | Your decision |
| **F6** | P2 | UI_LABEL_ONLY | DSB 2.20 30-shot (55 min) is a **recommendation**; the tile shows it like a rule time. | Tile/report wording "55 min (empfohlen)". | — (safe now) |
| **F7** | P2 | MISSING_EVENT | DSB 1.42 (50 m KK 30 standing, 40 min) is in the research but not in SETA. | Add only if SETA wants it; source row must first be added to the DSB register from the official catalogue. | Your decision + register row |

**No DSB timing defect was found.** The research's headline DSB items — 2.20 =
90 not 75, 1.40 = one 105 clock, 1.20 per-position times including sighting,
1.60 = 165 — are all already implemented correctly in SETA.

---

## 3. Research test matrix (§14) vs. SETA today

| Test | Status in SETA |
|---|---|
| T01 DSB 2.20 = 90 + separate 15 prep | covered (catalogue + seam tests) |
| T02 ISSF 50 m Pistol = 90 | **fails today** (F1) |
| T03 DSB 1.40 one 105 clock | covered (`tst_rule_authority`, `tst_qual3p`) |
| T04 / T05 ISSF 3P indoor 90 / outdoor 105 | **T04 only** (F2) |
| T06 unclassified 3P range blocked | **missing** (F2) |
| T07 / T08 DSB 1.20 per-position | covered (`tst_dsb120`) |
| T09 DSB 1.60 = 165 | covered |
| T10 / T11 25 m precision 300 / 240 s | out of scope (no 25 m) |
| T12 sighters never count | covered (reducer + DSB 1.20 tests) |
| T13 3P position change keeps clock | covered |
| T14 / T15 / T17 comms loss, restart, sleep | covered **2026-11-15** (RECOVERY-CLOCK-001, DSB120 recovery, double-crash tests) |
| T16 training time labelled non-official | **partial** (F5) |
| T18 RMS vs lane | lane-authoritative today; RMS observer only |
| T19 3P Final Third Print | **open** (F3) |
| T20 DSB Part 9 / Part 15 | not offered — correct |

---

## 4. Out of scope for SETA (no 15/25/100/300 m targets)

DSB 1.30, 1.35, 1.50, 1.59, 1.70, 1.90, 2.16–2.18 (need falling/turning targets),
2.30–2.60; ISSF 25 m Rapid / Pistol / Centre-fire / Standard, all 300 m events,
Mixed Team. Recorded so they are never added by "copying a timer": each needs
its own series/exposure state machine and hardware.

---

## 5. What is needed before Stage D (implementation)

1. **ISSF Rule Book 2026 Third Print (10/2026)** as a PDF — for F1 (6.11.9.8),
   F2 (6.11.9.2) and F3 (3P Final). The repo records edition, page and SHA-256,
   never the text.
2. **SETA's written answer**: for each customer 50 m range, indoor or outdoor
   for ISSF 3P? (F2.)
3. **Your decision** on practice timing and labelling (F5) and on DSB 1.42 (F7).

Safe now, without new sources: F4 and F6 (wording only).
