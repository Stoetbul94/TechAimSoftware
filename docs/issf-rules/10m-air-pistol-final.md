# 10m Air Pistol FINAL

Individual **10m Air Pistol Final**, Men and Women. Governed by ISSF rule
**6.17.2** (jointly with 10m Air Rifle) plus the general finals procedures
**6.17.1** and the pistol technical rules (Section 8).

**The complete rule audit lives in [10m-finals-shared.md](10m-finals-shared.md)**
— authoritative for the course of fire, scoring, eliminations, ties, commands,
malfunctions, EST handling and completion, all **identical** to Air Rifle (same
rule 6.17.2). This file records only what is **specific to Air Pistol**.

See [README.md](README.md) for the source-status legend.

---

## Document status

- **Implementation status:** ⏳ Not implemented (F0 rules + architecture only).
- **Rules verification status:** ✅ **Official — verified** against the source
  PDF. Source record: see [10m-finals-shared.md](10m-finals-shared.md#source-record-for-every--official-entry-below).
- **Applicable edition:** ISSF Rule Book **Edition 2025 (Second Print 07/2026),
  Effective 1 July 2026** — rules 6.17.1, 6.17.2, Section 8 (Pistol).
- **Last reviewed:** 2026-07-21.

---

## ⚠️ CRITICAL: the Air Pistol FINAL is DECIMAL (unlike Air Pistol Qualification)

Air Pistol **Qualification** is scored in **integers** (whole rings) — see
[10m-air-pistol.md](10m-air-pistol.md) and the `AP10` integer path (Phase B2).

The Air Pistol **FINAL** is scored in **DECIMAL (tenth-ring)** — 6.17.2 c:
*"Scoring in Finals is done with tenth-ring (decimal) scoring."* This applies to
the single shared rule that governs **both** rifle and pistol 10m finals.

**Do not** reuse the integer aggregation from AP10 qualification for the AP
final. The final's authoritative score is decimal tenths (max 10.9), identical
in representation to the rifle final. This is the single most likely
implementation error and is called out again in the historic-format warning of
[10m-finals-shared.md](10m-finals-shared.md#historic-format-warning-read-before-implementing).

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

## Air-Pistol-SPECIFIC differences (✅ Official)

1. **"TAKE YOUR POSITIONS" hold = 10 seconds.** After all introductions and
   "TAKE YOUR POSITIONS", the CRO waits **10 seconds** (Rifle: 30 s) before
   commanding "FIVE (5) MINUTES PREPARATION AND SIGHTING TIME… START"
   (6.17.2 e, p.271). **This is the only course-of-fire timing difference from
   Air Rifle.** In the implementation it is a single config constant.

2. **Decimal scoring in the final** (6.17.2 c) — see the CRITICAL note above.
   The **representation** is identical to the rifle final; the difference is
   only versus AP *qualification*.

3. **Equipment rules — Section 8 (Pistol).** Pistol equipment control (grip,
   sights, trigger) applies. Equipment-check concern only; the app does not
   enforce equipment control. No rifle jacket / `walking` rule.

4. **Target geometry.** 10m Air Pistol target and decimal ring geometry are the
   pistol values already used by the qualification scorer
   (`CenterPane.qml::calculateShootingSocre()`), but the **decimal** resolution
   (tenths) must be used for the final rather than the integer aggregation used
   for AP qualification. The ring geometry itself is unchanged.

---

## Notes for implementation (non-rule)

- **Discipline id (proposed):** `APF10` (Air Pistol Final 10m) — a new
  `Discipline` enum value; see [../10m-finals-architecture.md](../10m-finals-architecture.md#event-model).
- **Scoring:** decimal tenths — **not** the AP10 integer path. Authoritative
  store keeps `scoreTenths` as fixed-point integer tenths (same field as rifle).
- **Everything else** (commands, windows, eliminations, ties, recovery) is
  shared with Air Rifle — implement once, parameterise the 10 s hold.

---

## Air-Pistol-specific rule gaps

- None beyond the shared gaps in
  [10m-finals-shared.md](10m-finals-shared.md#rule-gaps-summary-full-detail-in-the-architecture-docs-gap-report)
  and the architecture gap report. The 10 s hold and decimal-scoring rules are
  fully verified. (The AP-qualification-integer trap is a *documentation*
  emphasis, not a rule gap.)
