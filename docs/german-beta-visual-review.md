# German Beta — Visual Review

Document version 1.0 (P0.1) · Application commit `169eef9`

**GERMAN BETA TRANSLATION — NATIVE TECHNICAL REVIEW REQUIRED**

---

## Status of this review

**The visual review was NOT performed.** Interactive GUI control and screen
capture are unavailable in this environment, so no German screen has been seen
at any resolution.

Recording it as done would be a false verification of exactly the thing most
likely to be broken — long German compounds in fixed-width controls.

What follows is therefore: what is **known from code and tests**, what a
reviewer must check, and an **explicit recommendation** based on measurable
coverage rather than appearance.

## 1. Verified from code and tests

| Property | Status | Evidence |
|---|---|---|
| German catalogue loads | **VERIFIED FROM CODE AND TESTS** | startup log `UI language: de-DE (beta translation)`, no load diagnostic |
| Selection persists across restart | **VERIFIED FROM CODE AND TESTS** | i18n persistence test |
| Untranslated strings fall back to English | **VERIFIED FROM CODE AND TESTS** | Qt source-language fallback; i18n test |
| Unknown language code falls back to English | **VERIFIED FROM CODE AND TESTS** | i18n test |
| Language does not alter brand, theme, executable, AppData | **VERIFIED FROM CODE AND TESTS** | i18n test |
| Language does not alter `app_mode` | **VERIFIED FROM CODE AND TESTS** | `LanguageService` has no mode access |
| Umlauts encode correctly through to PDF | **VERIFIED FROM CODE AND TESTS** | text extraction from the German PDFs |

**No blank string and no raw translation key can be produced by design:** an
absent translation resolves to the English source, which always exists.

## 2. Measurable coverage

| Metric | Value |
|---|---|
| Extractable source strings | 583 |
| Translated | 100 (~17%) |
| Falling back to English | 483 |
| Additionally **not extractable** (not wrapped in `qsTr()`) | substantial, concentrated in the Training Lab HUDs |

**The true German coverage of the visible interface is lower than 17%**,
because some visible text is not in the 583 at all. A German session is
**mixed-language on every screen**.

## 3. Required reviewer checks — NOT PERFORMED

At **1536×960, 1366×768, 1280×720, 1100×700**, and maximised/restored:

| # | Screen | Check |
|---|---|---|
| 3.1 | Settings language selector | German label, "(Beta)" suffix, beta note, restart note |
| 3.2 | Homepage | no clipping; mixed language acceptable? |
| 3.3 | Discipline selection | discipline names |
| 3.4 | Training Lab catalogue | programme names and descriptions |
| 3.5 | Technical Blocks (active + review) | metric labels — **highest clipping risk** |
| 3.6 | Call & Diagnose (call + reveal) | `CALL DIFFERENCE`, `HORIZONTAL`, `VERTICAL` |
| 3.7 | Position Transition (all phases) | `Positionswechsel`, `Kontrollschüsse`, rhythm badges |
| 3.8 | Recovery dialog | field labels and actions |
| 3.9 | Incident dialog | category names — currently English |
| 3.10 | Report export | dialog and messages |
| 3.11 | Exported PDFs | umlauts, wrapping, overflow |

**Highest-risk strings:** `Streukreisdurchmesser` (24 chars vs "Group
diameter" at 14), `Positionswechsel`, `Kontrollschüsse`, `PROBESCHIESSEN`,
`Trefferbild-Analyse`.

For each finding record: screen, resolution, string, and whether it is
clipped / overlapping / merely long.

## 4. Recording template

| Category | Count | Notes |
|---|---|---|
| German strings displayed correctly | — | not measured |
| English fallbacks displayed correctly | — | not measured |
| Blank strings | — | **should be zero by design** — any occurrence is a defect |
| Translation keys exposed | — | **should be zero by design** — any occurrence is a defect |
| Clipped German text | — | not measured |
| Mixed-language screens | — | expected on essentially every screen |
| Terminology inconsistencies | — | not measured |
| Umlaut issues | — | none in PDF text extraction; on-screen not measured |
| Restart / persistence behaviour | OK | verified from tests |

## 5. Recommendation

### **Acceptable as a mixed-language evaluation preview — NOT acceptable as German evaluation.**

Reasoning, from measurable facts rather than impression:

1. **~17% string coverage, and lower in practice.** A reviewer asked to
   evaluate "the German version" would spend most of the session reading
   English. That is not a German evaluation; it is an English evaluation with
   German controls.
2. **The fallback behaviour is sound.** Nothing blanks, nothing shows a raw
   key, and language cannot disturb brand, mode or data. So the build is
   *safe* to show in German — it is simply not *complete* in German.
3. **Layout is entirely unverified.** The longest German compounds sit in the
   Training Lab metric cards, which are the tightest fixed-width controls in
   the product. This is a genuine, unquantified risk.

### Conditions for use as a preview

- Label it explicitly a **preview**, not German support.
- Tell the evaluator that untranslated text stays in English **by design**.
- Ask them to report clipping, since it has not been checked.
- Do **not** publish German PDFs externally until section 3.11 is done.

### To reach "acceptable for German evaluation"

1. Wrap the remaining Training Lab strings in `qsTr()` — a **development**
   task, not a translation task, and the current hard ceiling on coverage.
2. Raise catalogue coverage substantially above 17%.
3. Complete native technical review against the glossary.
4. Complete the layout checks in section 3.

**No broad catalogue expansion was performed in this phase**, per the scope
instruction. No release-blocking single missing string was identified that
would justify a narrow exception.
