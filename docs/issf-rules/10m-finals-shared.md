# 10m Air Rifle & 10m Air Pistol FINAL — shared rules

**Individual** Finals for 10m Air Rifle and 10m Air Pistol, Men and Women.
The ISSF governs **both** finals with the **same rule (6.17.2)** and the same
general finals procedures (6.17.1). This file holds everything the two finals
share; the discipline files
([10m-air-rifle-final.md](10m-air-rifle-final.md),
[10m-air-pistol-final.md](10m-air-pistol-final.md)) hold only the few
verified differences.

See [README.md](README.md) for the source-status legend and authority
hierarchy, and [est-malfunctions.md](est-malfunctions.md) for the
cross-discipline EST/interruption rules that the Finals reference.

> This is a **Finals** discipline (medal competition), distinct from the
> 10m Air Rifle / Air Pistol **Qualification** files
> ([10m-air-rifle.md](10m-air-rifle.md), [10m-air-pistol.md](10m-air-pistol.md)).
> A different rule set, a different course of fire, and — importantly —
> **decimal scoring for both** (Air Pistol Qualification is integer; the Air
> Pistol **Final** is decimal). Do not carry qualification behaviour into the
> Final.

---

## Document status

- **Implementation status:** ⏳ Not implemented. This is the F0 rules +
  architecture package only; no controller, QML, scoring or persistence code
  exists for a 10m Final. Architecture: [../10m-finals-architecture.md](../10m-finals-architecture.md).
- **Rules verification status:** ✅ **Official — verified against the source PDF.**
  Extracted directly from the official ISSF rulebook PDF (text-extracted and
  read section-by-section, not from search snippets).
- **Applicable ISSF rulebook edition:** **ISSF Official Statutes, Rules and
  Regulations — EDITION 2025 (Second Print 07/2026), Effective 1 July 2026.**
- **Last reviewed:** 2026-07-21

### Source record (for every ✅ Official entry below)

| Field | Value |
|---|---|
| Document title | ISSF Official Statutes, Rules and Regulations (General Technical Rules, Section 6) |
| Edition / year | Edition 2025, **Second Print 07/2026** |
| Effective date | **1 July 2026** (stated on every page footer and title page) |
| Rules | 6.17.1 (general finals procedures), 6.17.1.1–6.17.1.14, **6.17.2** (10m AR & AP course of fire); supporting: 6.13.2 (allowable malfunctions), 6.15.5 (qualification ties), 6.4 (decimal scoring definition) |
| Pages | 265–272 (6.17.1 → 6.17.2); malfunctions 259; decimal-scoring definition in Section 6.4 |
| Subject | Individual Finals in Olympic Rifle and Pistol events |
| Access date | 2026-07-21 |
| Source | ISSF website → Rules → Rulebook download → official backoffice PDF `ISSF-Rule-Book-2026-Edition-2025-Second-Print-07-2026-Effective-1-July-2026.pdf` |

> **Edition currency note (important).** The 2026 rulebook is the current
> authority as of this writing. A **First Print (12/2025)** existed briefly;
> the **Second Print (07/2026)** replaced it three weeks before this review and
> is what is cited here. The ISSF also publishes a separate **"Commands and
> Announcements for Finals"** document (referenced by the NOTE at the head of
> 6.17.2) that carries the *authoritative detailed command timings*; the
> rulebook states its own timings are **"guidelines."** That HQ document was
> **not** obtained during F0 — see the Rule Gap report.

---

## HISTORIC-FORMAT WARNING (read before implementing)

ISSF individual final formats have changed repeatedly. The **current verified**
format (2026) and older formats that must **not** be used:

| Format | Finalists | Course of fire | Scoring | Status |
|---|---|---|---|---|
| **2026 (current, verified)** | **8** | **2×5-shot series (250 s each) + 14 single shots (50 s each) = 24 shots; eliminations from shot 12** | **Decimal (both AR & AP)** | ✅ Use this |
| 2013–2017 era | 8 | 2 series of 5 (decimal) then single shots; elimination final introduced 2013 | Decimal | ❌ Obsolete — similar but do not assume timings/counts |
| Pre-2013 | 8 | Qualification score **carried into** the final + 10 final shots | AR decimal / **AP integer** | ❌ Obsolete — carry-over and AP-integer are both wrong now |
| "10-shot final added to qualification" | 8 | 10 shots, cumulative with qualification | mixed | ❌ Obsolete |

**Traps this creates for implementation:**
- **Do not** carry the qualification score into the final (pre-2013 behaviour).
  2026: start from **zero** (6.17.1.5).
- **Do not** score the Air Pistol final in integers because Air Pistol
  *qualification* is integer. 2026: the AP **final** is **decimal** (6.17.2).
- **Do not** reuse the 50m 3P Final's stage/elimination structure. The 3P Final
  in this repo is a **single-athlete, 35-shot, position-based** training mode
  with **no eliminations** — a different discipline entirely (6.17.3, not
  6.17.2). Its `Finals3PController` must not be a template for the course of
  fire, timings, or shot counts.
- **Repository check:** no existing repo code or doc encodes an *obsolete* 10m
  final format (there is no 10m-final code at all). The only nearby doc,
  [50m-rifle-3p-final.md](50m-rifle-3p-final.md), is a different final and is
  not obsolete.

---

## The 9-point rule audit (shared 6.17.1 / 6.17.2 — applies to BOTH AR and AP)

Everything here is **✅ Official** (2026 Second Print, rules/pages as noted)
unless tagged otherwise.

### 1. Eligibility
- **Finalists:** the **eight (8)** highest-ranking athletes in Qualification
  advance (6.17.1.1, p.265).
- **Carry-over:** none. Qualification scores entitle a place but **do not carry
  forward**; the Final **starts from zero** (6.17.1.5, p.265).
- **Start order / bib:** start positions assigned by a **random draw** done
  automatically by the computer when the Finals Start List is produced
  (6.17.1.2, p.265).
- **Firing-point assignment:** 10m firing points must be labelled
  **`R1-A-B-C-D-E-F-G-H-R2`**; reserve points are **R1** and **R2** (6.17.1.2,
  p.265).

### 2. Reporting and setup
- **Reporting time:** athletes must report to the Finals Range Preparation Area
  **at least 30 minutes before** the Start Time (6.17.1.3, p.265). The Start
  Time is defined as **when the CRO begins the commands for the first MATCH
  shot/series** (6.17.1.3).
- **Equipment set-up:** guns/equipment may be placed on the firing point **not
  less than 20:00 min before** the Start Time; aiming exercises **not** permitted
  then (6.17.1.14.b, p.267; 6.17.2 e, p.271).
- **Call to the line:** an NTO directs athletes to line up **10 minutes before**
  the published Start Time; CRO announces **"ATHLETES TO THE LINE"**
  (6.17.1.14.c, p.267).
- **Preparation + sighting:** a **combined 5-minute "PREPARATION AND SIGHTING
  TIME"** (6.17.2 e, p.271).
- **Sighting shots:** **unlimited** during the Preparation and Sighting Time
  (6.17.2 e, p.271). No score announcements during sighters.
- **Presentation:** after "ATHLETES TO THE LINE", athletes enter **one at a
  time** and are introduced by the Announcer; the Announcer also introduces the
  CRO and Jury Member-in-Charge (6.17.1.12, p.266; 6.17.2 e).

### 3. Commands (shared wording; the exact sequence)
Verified wording from 6.17.2 (p.271–272) and 6.17.1.14 (p.267–270):

1. **"ATHLETES TO THE LINE"** — call to line (~10 min before start).
2. *(introductions / presentation)*
3. **"TAKE YOUR POSITIONS"** — after all introductions.
4. *(hold: **30 s for Rifle, 10 s for Pistol** — the one course-of-fire
   difference; see discipline files.)*
5. **"FIVE (5) MINUTES PREPARATION AND SIGHTING TIME… START"** — unlimited
   sighters; loading is authorised by START (no separate LOAD here,
   6.17.1.14.f).
6. **"30 SECONDS"** — 30 s before the prep/sighting end.
7. **"STOP… UNLOAD"** — end of prep/sighting. Targets/scoreboard cleared.
8. *(60 s gap.)*
9. **"FOR THE FIRST COMPETITION SERIES… LOAD"** → after **5 s** → **"START"**
   → **250 s** to fire 5 shots.
10. **"STOP"** at 250 s or when all have fired 5 (announcer 15–20 s ranking).
11. **"FOR THE NEXT COMPETITION SERIES, LOAD"** → 5 s → **"START"** → 250 s.
12. **"STOP"** (announcer; explains single shots + elimination begins).
13. **Single shots** (×14): **"FOR THE NEXT COMPETITION SHOT, LOAD"** → 5 s →
    **"START"** → **50 s** → **"STOP"** (announcer). Repeat.
14. After the 24th shot: **"STOP… UNLOAD"**; RO verifies actions open, safety
    flags inserted.
15. **"RESULTS ARE FINAL"** (6.17.2 i) → medallist presentation (6.17.1.14.q).

- **When loading is permitted:** only after **"LOAD"** or **"START"** (early
  loading forbidden, 6.17.1.14.f). For prep/sighting, **START** authorises
  loading.
- **When the athlete may fire:** after **"START"**. Firing after **LOAD** but
  before **START** in a 10m final → the shot is **scored as a miss**
  (6.17.1.14.k, p.269).
- **STOP / UNLOAD:** ends each window; final "STOP… UNLOAD" ends the Final.
- **Command timing:** rulebook timings are **guidelines**; authoritative
  detailed timings are in the separate ISSF HQ "Commands and Announcements for
  Finals" document (6.17.2 NOTE, p.271) — **not obtained (gap)**.

### 4. Competition structure (6.17.2 a/b/c/g, p.271–272)
- **Opening series:** **two (2) series of five (5) MATCH shots** each.
- **Shots per series:** 5.
- **Time per series:** **250 seconds** each.
- **Transition:** after the two series (10 shots), single shots begin.
- **Single shots:** **fourteen (14)** single MATCH shots.
- **Time per single shot:** **50 seconds** each, on command.
- **Elimination points:** eliminations of the lowest-ranked begin **after the
  12th shot** and continue **after every two shots**:
  - after 12 → **8th place** eliminated
  - after 14 → 7th; after 16 → 6th; after 18 → 5th; after 20 → 4th
  - after 22 → **3rd (bronze decided)**
  - after 24 → **2nd and 1st (silver and gold decided)**
- **Maximum shots:** **24** (2×5 + 14), plus any tie-break shots (6.17.2 c/h).

### 5. Scoring (6.17.2 b/c, p.271; 6.4 decimal definition)
- **Decimal or integer:** **decimal (tenth-ring)** for **both** AR and AP
  (6.17.2 c: *"Scoring in Finals is done with tenth-ring (decimal) scoring."*).
- **Precision:** tenths of a ring; a ring is divided into ten equal decimal
  rings; **maximum 10.9** (6.4 decimal-scoring definition).
- **Total:** **cumulative** decimal total determines the ranking (6.17.2 b).
- **Valid-shot handling:** each commanded shot/series counts within its window.
- **Late shot (after LOAD, before START):** scored as a **miss** (6.17.1.14.k).
- **Shot before prep/sighting START, or before the sighting-series LOAD:**
  **disqualification** (6.17.1.14.h).
- **Shot after STOP, before next START:** **not counted** as a MATCH shot; a
  **2-point penalty** applied to the first MATCH shot (6.17.1.14.i).
- **Extra shot in a series/single window:** **nullified**; a **2-point penalty**
  on the last correct shot/series (6.17.1.14.l).
- **Inadvertent shot** by an athlete not in a shoot-off/malfunction completion:
  **nullified, no penalty** (6.17.1.14.m).
- **Crossfires:** not addressed as a distinct 10m-final rule; general EST
  complaint / unexpected-zero procedure (6.17.1.8) governs miss-vs-malfunction.
- **Missing shot (unexpected zero):** Jury procedure (6.17.1.8.b, p.265–266) —
  the Jury determines miss vs malfunction; unless credible evidence of a miss,
  the athlete fires another competition shot whose value counts in lieu.
- **Protested shots:** **score protests (value/number of shots) are NOT
  permitted** in Finals (6.17.1.7, p.265).
- **No score below zero** is recorded after deductions (6.17.1.5).

### 6. Ranking and elimination (6.17.2 b/g/h, p.271–272)
- **When ranking is calculated:** continuously from cumulative decimal totals;
  it becomes decisive at each elimination point (after shots 12, 14, 16, 18, 20,
  22, 24).
- **Elimination order:** lowest-ranked athlete removed at each point, 8th → …
  → bronze after 22 → silver/gold after 24 (list in §4 above).
- **Ties at an elimination boundary:** if there is a tie for the lowest-ranked
  athlete to be eliminated, the tied athletes fire **additional tie-breaking
  single shot(s) until the tie is broken** (6.17.2 h).
- **Shoot-off procedure:** CRO **immediately announces the family names** of the
  tied athletes and commands them to fire the tie-breaking shot(s) with the
  **normal firing procedure** (6.17.2 h). Repeated single shots until broken.
- **Timing of tie-break shots:** same single-shot procedure (LOAD → START →
  50 s → STOP), fired immediately at the boundary before the next elimination.
- **Medal ties:** the same shoot-off procedure decides ties for medal places
  (6.17.2 b: *"ties broken according to shoot-off scores"*).
- **When the next elimination occurs:** after every subsequent 2 shots, once the
  current boundary (including any shoot-off) is resolved.

### 7. Malfunctions and interruptions
- **Athlete/equipment malfunction (6.17.1.6, p.265; 6.13.2, p.259):** only
  **ALLOWABLE** malfunctions (cartridge fails to fire; pellet lodged in barrel;
  gun fails with trigger released) qualify. **One allowable malfunction per
  athlete per Final** (6.17.1.6 / 6.17.1.14.r).
  - *Single shot:* max **1 min** to repair/replace, then **repeat the shot**.
  - *Series:* if repaired within 1 min, shots already fired **count**; the
    athlete completes the series in the time remaining **plus** the repair time
    (**≤ 1 min**).
- **EST malfunction (6.17.1.8, p.265–266):** the Final has its **own** detailed
  EST-complaint procedure:
  - *Sighting target fails to register* → fire another shot; if it fails / strip
    won't advance → CRO **"STOP… UNLOAD"** for all, move the athlete to a
    **reserve target**, then **2 min** preparation and **restart the whole
    Preparation and Sighting Time** for the Final (6.17.1.8.a).
  - *Unexpected zero in a MATCH shot/series* → Jury (Jury-Member-in-Charge +
    second Jury member + one RTS) determines miss vs malfunction; unless
    credible miss evidence, direct another competition shot whose value counts
    in lieu (6.17.1.8.b).
  - *If the re-fire on that target doesn't register* → move to a **reserve
    target**, give **2 min** Prep+Sighting, fire the missing shot on command
    before others continue (6.17.1.8.c).
  - *During delay:* others may dry-fire; if the total delay to resolve exceeds
    **5 minutes**, all athletes get **2 min** sighting before resuming
    (6.17.1.8.d).
- **Interrupted series / single-shot stage:** covered by the incorrect-command
  rule (6.17.1.14.p, p.269): shots already fired count; CRO resets the clock —
  for a multi-shot series the Jury-Member-in-Charge determines remaining time
  and **adds 60 s**, then restarts; accidental extra shots are nullified,
  no penalty.
- **Additional preparation / sighting:** only the specific 2-min allowances in
  6.17.1.8 (a/c/d). No open-ended Jury allowance is defined here for 10m Finals.
- **Target reassignment:** to **reserve target R1/R2** under 6.17.1.8 (a/c),
  Jury-directed.
- **Jury authority:** the Finals Protest Jury decides protests immediately, no
  appeal (6.17.1.10.d, 6.17.1.13).
- **Generic Phase-E incident workflow sufficiency:** ⚠️ **Partially.** The
  existing generic EST workflow ([est-malfunctions.md](est-malfunctions.md))
  models raise → freeze → Jury decision → recovery phases (prep/sighting) →
  resume gating → reassignment — which **structurally covers** the 10m-final
  EST procedure (reserve target, 2-min prep/sighting restart, resume gate). But
  the **finals-specific specifics** (the fixed 2-min values, the
  restart-whole-prep-and-sighting rule, the one-malfunction-per-final limit, the
  repeat-shot-in-lieu semantics, the +60 s incorrect-command rule) are **not**
  encoded and are ⏳ **awaiting** an implementation decision on whether to
  special-case them in the finals controller or extend the generic workflow.
  See the Rule Gap report.

### 8. Penalties and disqualification (6.17.1.14, p.267–270)
- **Late reporting:** **2-point** deduction from the first MATCH shot/series
  (6.17.1.3).
- **Late arrival (>10 min after reporting time):** may not start; recorded as
  the **first eliminated** athlete, shown **DNS**; if a finalist does not report
  at all, the first elimination begins with **7th place** (6.17.1.4).
- **Dry firing** outside permitted periods: **1-point** deduction (10m)
  (6.17.1.14.e).
- **Firing after LOAD before START:** shot scored as a **miss** (6.17.1.14.k).
- **Firing after STOP before next START:** shot not counted + **2-point**
  penalty on first MATCH shot (6.17.1.14.i).
- **Extra shot:** nullified + **2-point** penalty (6.17.1.14.l).
- **Firing before prep/sighting START (or before sighting-series LOAD):**
  **disqualification** (6.17.1.14.h).
- **Protest not upheld:** **2-point** penalty (6.17.1.13.d).
- **Zero-valued shots:** no score below zero recorded (6.17.1.5).
- **Unsafe handling / refusal to comply:** governed by general ISSF rules
  (6.17.1.14.u) — not a distinct finals clause.

### 9. Completion (6.17.2 i, p.272; 6.17.1.14.q, p.269)
- **Completion condition:** after the two remaining finalists fire their **24th
  shots**, if there are no ties or protests, CRO commands **"STOP… UNLOAD"** and
  declares **"RESULTS ARE FINAL."**
- **Medal positions:** decided after shot 22 (bronze) and shot 24 (silver/gold).
- **Result confirmation:** CRO declares "RESULTS ARE FINAL"; Jury assembles the
  three medallists; Announcer recognises bronze → silver → gold (6.17.1.14.q).
- **Protest timing:** protests in a Final are **verbal and immediate**
  (6.17.1.13.a); none after "RESULTS ARE FINAL."
- **Session closure:** the app's own clean-shutdown / archive (reliability
  layer) closes the journalled session.

---

## Comparison matrix — Air Rifle vs Air Pistol (verified)

Legend: **=/V** identical and officially verified · **≠** different · **~?**
apparently shared but not independently verified · **?** unknown.

| Rule / Behaviour | Air Rifle | Air Pistol | Verdict |
|---|---|---|---|
| Finalists | 8 | 8 | **=/V** (6.17.1.1) |
| Preparation | 5:00 combined prep+sighting | 5:00 combined prep+sighting | **=/V** (6.17.2 e) |
| Sighting | unlimited during prep+sighting | unlimited during prep+sighting | **=/V** (6.17.2 e) |
| Opening series | 2 × 5 shots | 2 × 5 shots | **=/V** (6.17.2 a) |
| Series time | 250 s | 250 s | **=/V** (6.17.2 a) |
| Singles | 14 × 1 | 14 × 1 | **=/V** (6.17.2 a) |
| Single-shot time | 50 s | 50 s | **=/V** (6.17.2 a) |
| Eliminations | from shot 12, every 2 shots | from shot 12, every 2 shots | **=/V** (6.17.2 g) |
| Decimal scoring | decimal | **decimal** | **=/V** (6.17.2 c) |
| Tie handling | tie-break single shots until broken | same | **=/V** (6.17.2 h) |
| Commands (course of fire) | same sequence… | …same sequence | **=/V** (6.17.2) |
| **"TAKE YOUR POSITIONS" hold** | **30 s** | **10 s** | **≠** (6.17.2 e) |
| Malfunction handling | 6.17.1.6 / 6.13.2 | 6.17.1.6 / 6.13.2 | **=/V** |
| EST complaint handling | 6.17.1.8 | 6.17.1.8 | **=/V** |
| Completion | 6.17.2 i | 6.17.2 i | **=/V** |
| Equipment / dress code | rifle (Section 7, jacket, national ID on jacket, `walking` 7.5.2.3) | pistol (Section 8) | **≠** (metadata only, not course of fire) |

**Conclusion:** the **course of fire, scoring, eliminations, ties, commands,
malfunction and EST handling, and completion are genuinely identical** and
governed by the same rule (6.17.2). The **only** verified differences are
(1) the post-"TAKE YOUR POSITIONS" hold (**30 s rifle / 10 s pistol**) and
(2) equipment/dress-code metadata (rifle vs pistol technical rules). A shared
implementation is therefore **rule-justified**, not a file-count convenience —
see [../10m-finals-architecture.md](../10m-finals-architecture.md).

---

## Rule gaps (summary; full detail in the architecture doc's gap report)

1. **Single-athlete vs 8-athlete scope** (product decision, **blocking**) — the
   rules describe an 8-athlete elimination final; this app is single-lane and
   the existing 3P Final is a single-athlete training mode. Eliminations, ties,
   shoot-offs and live ranking are only meaningful with 8 athletes' data.
2. **Detailed command timings** — the authoritative "Commands and Announcements
   for Finals" HQ document was not obtained; rulebook timings are "guidelines."
3. **Finals-specific EST specifics vs generic Phase-E workflow** — whether to
   special-case the 2-min prep/sighting restart, one-malfunction-per-final, and
   +60 s incorrect-command rules, or extend the generic workflow.
4. **Live aiming (2026 change)** — the 2026 rulebook introduces "live aiming"
   for rifle/pistol; its exact finals impact was noted in the change
   announcement but its procedural detail was not isolated in 6.17.2 and needs
   confirmation.
