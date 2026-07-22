# 10m Air Rifle FINAL

Individual **10m Air Rifle Final**, Men and Women. Governed by ISSF rule
**6.17.2** (jointly with 10m Air Pistol) plus the general finals procedures
**6.17.1** and the rifle technical rules (Section 7).

**The complete rule audit lives in [10m-finals-shared.md](10m-finals-shared.md)**
— that file is authoritative for the course of fire, scoring, eliminations,
ties, commands, malfunctions, EST handling and completion, all of which are
**identical** for Air Rifle and Air Pistol (same rule 6.17.2). This file records
only what is **specific to Air Rifle**.

See [README.md](README.md) for the source-status legend.

---

## Document status

- **Implementation status:** ⏳ Not implemented (F0 rules + architecture only).
- **Rules verification status:** ✅ **Official — verified** against the source
  PDF. Source record: see [10m-finals-shared.md](10m-finals-shared.md#source-record-for-every--official-entry-below).
- **Applicable edition:** ISSF Rule Book **Edition 2025 (Second Print 07/2026),
  Effective 1 July 2026** — rules 6.17.1, 6.17.2, Section 7 (Rifle).
- **Last reviewed:** 2026-07-21.

---

## Verified format (identical to the shared file — repeated for a standalone read)

- **8** finalists; **start from zero** (no qualification carry-over).
- **2 × 5-shot series** at **250 s** each, then **14 single shots** at **50 s**
  each = **24 shots**.
- **Decimal (tenth-ring)** scoring; cumulative total ranks; ties by shoot-off.
- Eliminations from shot 12: 8th(12) 7th(14) 6th(16) 5th(18) 4th(20) 3rd(22,
  bronze) 2nd/1st(24, silver/gold).
- Ties for elimination → tie-break single shots until broken (6.17.2 h).

Full audit and citations: [10m-finals-shared.md](10m-finals-shared.md).

---

## Air-Rifle-SPECIFIC differences (✅ Official)

1. **"TAKE YOUR POSITIONS" hold = 30 seconds.** After all introductions and
   "TAKE YOUR POSITIONS", the CRO waits **30 seconds** (Pistol: 10 s) before
   commanding "FIVE (5) MINUTES PREPARATION AND SIGHTING TIME… START"
   (6.17.2 e, p.271). **This is the only course-of-fire timing difference from
   Air Pistol.** In the implementation it is a single config constant.

2. **Call-to-line dress / `walking` (6.17.1.14.c, p.267).** Rifle finalists
   called from the Preparation Area must walk to the line **fully dressed with
   trousers and jackets closed**; ISSF rule **7.5.2.3** governs `walking`. (No
   effect on scoring or timing; relevant only if the app models presentation.)

3. **National identification (6.17.1.14.v, p.270).** Rifle athletes must display
   the national flag or 3-letter IOC identifier on the jacket pocket facing the
   audience or the lower back. (Presentation/report metadata only.)

4. **Equipment rules — Section 7 (Rifle).** Rifle equipment control, clothing
   stiffness tolerances (adjusted in 2026), and jacket/trouser rules apply.
   These are equipment-check concerns, not scoring/timing; the app does not
   enforce equipment control.

5. **Target geometry.** 10m Air Rifle target and decimal ring geometry are the
   rifle values already used by the qualification scorer
   (`CenterPane.qml::calculateShootingSocre()`); the Final reuses the **same**
   decimal ring math (max 10.9). No new geometry.

---

## Notes for implementation (non-rule)

- **Discipline id (proposed):** `ARF10` (Air Rifle Final 10m) — a new
  `Discipline` enum value; see [../10m-finals-architecture.md](../10m-finals-architecture.md#event-model).
- **Scoring reuse:** decimal, identical to 10m AR qualification scoring; the
  authoritative store already keeps `scoreTenths` as fixed-point integer tenths.
- **Everything else** (commands, windows, eliminations, ties, recovery) is
  shared with Air Pistol — implement once, parameterise the 30 s hold.

---

## Air-Rifle-specific rule gaps

- None beyond the shared gaps in
  [10m-finals-shared.md](10m-finals-shared.md#rule-gaps-summary-full-detail-in-the-architecture-docs-gap-report)
  and the architecture gap report. The 30 s hold, dress, and national-ID rules
  are fully verified.
