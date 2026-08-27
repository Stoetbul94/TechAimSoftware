# All implemented disciplines — audit against the current rules

**No production code was changed by this audit** other than the separately
authorised 3P Final presentation correction. Everything below is research,
comparison and reporting.

Rule source for every ISSF row: **ISSF Rule Book 2026, Edition 2025 (Second
Print 07/2026), effective 1 July 2026**, read from the official PDF on
2026-08-27 — see [ISSF-50M-3P-FINAL-SOURCE-MANIFEST.md](ISSF-50M-3P-FINAL-SOURCE-MANIFEST.md).

> **Audit coverage is not uniform, and this document says so per row.** Two
> disciplines were audited in depth against the verified text; the rest were
> checked on course-of-fire and scoring mode only. A row marked
> `NOT AUDITED THIS ROUND` is not a pass.

## Implemented disciplines — from the repository, not from memory

Found in `CompetitionCatalogue.qml`, the controllers under `src/`, and the
mode-entry functions in `ShootingPage.qml`:

| Discipline | Software id | Controller | Kind |
|---|---|---|---|
| 10 m Air Rifle | `AR10` | `QualificationController` | official |
| 10 m Air Pistol | `AP10` | `QualificationController` | official |
| 50 m Rifle (3 Positions) | `RIFLE50` / `3P50` | `QualificationController` | official |
| 50 m Rifle Prone | `PRONE50` | `QualificationController` | official |
| 50 m Free Pistol | `FREEPISTOL50` | `QualificationController` | official |
| 10 m Air Rifle Final | `FINAL_AR10` | `Finals10mController` | official |
| 10 m Air Pistol Final | `FINAL_AP10` | `Finals10mController` | official |
| 50 m Rifle 3 Positions Final | `FINAL3P` | `Finals3PController` | official |
| Open Practice / free | — | legacy path | **non-official training** |
| Training Lab: Technical Blocks, Call & Diagnose, Wind Map, Position Transition | `TRAINING`, `CALLDIAG`, `WINDMAP`, `POSTRANS` | training controllers | **non-official training** |

**No DSB / German competition mode is implemented on this branch.** The DSB
1.20 / 1.40 / 1.60 work lives on `product/seta`. It is therefore out of scope
here and is **not** audited — see the portability note at the end.

## Audit table

| DISCIPLINE | AUTHORITY | COMPONENT | RULE ITEM | OFFICIAL | SOFTWARE | STATUS | SEV | CODE CHANGE? |
|---|---|---|---|---|---|---|---|---|
| **50 m 3P Final** | 6.17.3 ✅ | `Finals3PConfig.h` | Total official shots | 35 | 35 | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 a ✅ | `Finals3PConfig.h` | Kneeling / Prone match shots | 10 / 10 | 10 / 10 | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 a/e ✅ | `Finals3PController` | Combined K+P+standing-sight block | 22 min continuous | `stage1Ms` 1320000, shared clock | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 d ✅ | `Finals3PConfig.h` | Preparation and sighting | 5 min, after 30 s hold | 300000 / 30000 | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 d ✅ | `Finals3PController` | Prep warning | `30 SECONDS` at 4:30 | `prepWarnMs` 270000 | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 f ✅ | `Finals3PConfig.h` | Block warnings | 17:00 / 21:30 | 1020000 / 1290000 | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 g ✅ | `Finals3PConfig.h` | Interval, LOAD→START | 30 s, 5 s | 30000 / 5000 | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 g ✅ | `Finals3PConfig.h` | Standing series | 2 × 5 shots, 250 s | `seriesMs` 250000 | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 h ✅ | `Finals3PConfig.h` | Singles | 50 s each, shots 31–35 | `singleMs` 50000 | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 h ✅ | `Finals3PController` | Elimination points | 8th/7th at 30; 6th, 5th, 4th, bronze, gold/silver | conditional notices, no ranking fabricated | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 b ✅ | controller | Scoring | decimal | decimal | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 d–j ✅ | `issueCommand` | CRO command wording | verbatim phrases | verbatim, identical | **PASS** | — | No |
| 50 m 3P Final | 6.17.3 i ✅ | — | Tie-breaking | tie shots + countback | **not implemented** | **NOT IMPLEMENTED** | SEV-2 | **No** — cross-lane, needs Range Management |
| 50 m 3P Final | — | `Finals3PRightPanel.qml` | Interval countdown labelling | operator must not read it as sighting | was unlabelled | **CORRECTED** | SEV-3 | **Yes — done** |
| **10 m AR/AP Final** | 6.17.2 a/c ✅ | `Finals10mConfig.h` | Course | 5 + 5 then **14** singles, **24 total** | 24-shot model | **PASS** | — | No |
| 10 m AR/AP Final | 6.17.2 a ✅ | `Finals10mConfig.h` | Series / single times | 250 s / 50 s | 250 s / 50 s | **PASS** | — | No |
| 10 m AR/AP Final | 6.17.2 b ✅ | controller | Eliminations | begin after shot 12, every two shots | implemented | **PASS** | — | No |
| 10 m AR/AP Final | 6.17.2 b ✅ | controller | Scoring | decimal | decimal | **PASS** | — | No |
| 10 m AR/AP Final | 6.17.2 b ✅ | — | Tie handling | shoot-off scores | not implemented | **NOT IMPLEMENTED** | SEV-2 | No — cross-lane |
| 10 m AR/AP Final | — | — | Report / PDF | n/a | **missing (F6)** | **NOT IMPLEMENTED** | SEV-2 | Not this round |
| **50 m Rifle 3P Qualification** | event table ✅ | `ShootingPage.qml` | Course of fire | **3 × 20 = 60 shots** | `matchShootCount === 60` gates `is3PMatch` | **PASS** | — | No |
| 50 m 3P Qualification | 6.11.1.1 ✅ | `AppSettings` | Preparation and sighting | **15 minutes** | `m_prepTimeMinutes = 15` | **PASS** | — | No |
| 50 m 3P Qualification | — | `QualificationController` | Per-position time, transitions, target-mode changes | ⏳ not extracted this round | position machinery implemented, 3P-gated | **NOT AUDITED THIS ROUND** | — | No |
| **10 m AR / AP Qualification** | event table ✅ | catalogue | Course of fire | 60 shots | 60-shot programmes present | **PASS** | — | No |
| 10 m AR / AP Qualification | — | — | Timing, series, warnings | ⏳ not extracted | — | **NOT AUDITED THIS ROUND** | — | No |
| **50 m Rifle Prone** | event table ✅ | catalogue | Course of fire | 60 shots | 60-shot programme | **PASS** | — | No |
| 50 m Rifle Prone | — | — | Timing | ⏳ not extracted | — | **NOT AUDITED THIS ROUND** | — | No |
| **50 m Free Pistol** | — | catalogue | Everything | ⏳ | 60-shot programmes present | **AUTHORITY NOT VERIFIED** | — | No |
| **Open Practice** | n/a | legacy | — | — | — | **NOT APPLICABLE** — non-official training | — | No |
| **Training Lab (4 programmes)** | n/a | training controllers | — | — | — | **NOT APPLICABLE** — non-official training, correctly not claiming to be a match | — | No |

## Future rule changes to watch

| Discipline | Announcement | Date | Intended effective | Software impact | Action now? |
|---|---|---|---|---|---|
| All | ISSF news "ISSF announces changes with 2026 Rulebook" (<https://www.issf-sports.org/news/4878>) | 2026 | already in the Second Print | none beyond what is audited above | **NO** |
| Rifle/Pistol | LA28 Olympic-format discussion reported in ISSF communications | Aug 2026 | **not stated as effective** | unknown | **NO** — a news announcement does not override a currently effective print |

**Rule: a currently effective rulebook governs. An announcement does not.**

## What this audit did NOT do

- It did not extract per-position shooting times for the qualification
  disciplines, nor the qualification command sequences. Those rows say
  `NOT AUDITED THIS ROUND` rather than `PASS`.
- It did not audit 50 m Free Pistol against any source.
- It did not audit DSB modes; none are implemented on this branch.
- It did not verify equipment or range rules the software does not enforce.

## Portability

Every row above describes **shared core** behaviour (`src/finals`,
`src/finals10m`, `src/qualification`, `Finals3PConfig.h`) except the
presentation correction, which is shared QML. Android (`feature/android-tablet`)
and SETA (`product/seta`) are 33–34 commits behind and carry **none** of the 3P
Final work, so every 3P row is `ANDROID: NO` / `SETA: NO`. See
[../architecture/CROSS-PLATFORM-FIX-REGISTER.md](../architecture/CROSS-PLATFORM-FIX-REGISTER.md).
Nothing was ported in this round.
