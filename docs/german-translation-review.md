# German Translation Review — Tech Aim

**Status: German Beta Translation — Native Review Required.**

This document tells a German-speaking reviewer exactly what exists, what does
not, and how to change it without touching application code.

## Current state — SETA German completion pass (product/seta)

**Status: COMPLETE DRAFT — NATIVE TECHNICAL REVIEW PENDING (on-screen UI).**
**Overall: PARTIAL** — the printed report / PDF surface is still English.

| Metric | Value |
|---|---|
| Catalogue entries | 1,095 |
| Translated | 856 |
| Untranslated | 239 |
| **On-screen application surface** | **827 / 827 translated** |
| Printed report / PDF surface | 0 / 223 |
| Vendored QModMaster forms (unreachable UI) | not translated |
| Catalogue | `translations/techaim_de_DE.ts` |
| Compiled | `translations/techaim_de_DE.qm`, embedded via `techaim_translations.qrc` |
| Language code / fallback | `de-DE` / `en` |

### What changed in this pass

The earlier gap was not a thin catalogue. **545 user-visible strings were never
wrapped in `qsTr()` at all**, so no catalogue could reach them however complete
it was. This pass wrapped the on-screen surface and authored German for all of
it: the landing and session screen, the Training Lab setup panels (Technical
Blocks, Call & Diagnose, Position Transition, Wind Map), the shooting screen and
its HUDs, the right panels, the dialog framework, recovery and licence dialogs,
Settings, the Finals and Training Lab cards, and the readiness strip.

**Everything still English on screen is a deliberate technical value**: ` mm`,
`X:`, `, Y:`, `—`, `S`, `#`, `%1`, ` (%1)`, the shot counts 10/15/20/30/40/60,
the score bands 10s/9s/8s/≤7, and `Form` (the vendored QModMaster designer name,
in a window this product never shows).

### What is NOT translated, and why

**The printed report / PDF views (223 strings).** They are documents rather than
controls, laid out on fixed A4 geometry, and German expands: translating them
without measuring those page layouts would risk clipped report pages, which is a
worse defect than an English report. They are a separate, scoped piece of work.

### Gates

`SETA-LANG-004` asserts every on-screen string has German, with the neutral
values listed explicitly, and asserts that the printed-report surface is *still*
outstanding — so this document cannot drift into claiming more than the build
does. `SETA-LANG-005` asserts that switching language leaves `programmeId`,
`rulesetId`, `disciplineId`, `targetStandardId`, distance, shot count, scoring
mode, target family, range, weapon and event index **identical**.

### BETA is not removed

Coverage is a string count. Native technical review is a different question:
whether a German range officer or ISSF-licensed coach would actually say
*Streukreis*, *Probeschießen*, *Kniend/Liegend/Stehend*, *Ansage & Diagnose*.
`de-DE` therefore stays flagged beta in `LanguageService`, and a test asserts it.

## Two limits a reviewer must know about

1. **Not every user-facing string is extractable yet.** Translation only sees
   text wrapped in `qsTr()` (QML) or `tr()` (C++). Large parts of the Training
   Lab HUDs and report views use bare string literals — for example
   `PositionTransitionHud.qml` contains only 8 `qsTr()` calls in a file full of
   visible text. Those strings cannot be translated until they are wrapped.
   Wrapping them is a code change and is **not** part of this review.

2. **`lupdate` coverage was previously wrong** and has been corrected: the
   extractor was pointed at 16 of the 66 product QML files. The 583 figure
   reflects the corrected list.

## How to revise the translation (no code changes)

1. Open `translations/techaim_de_DE.ts` in **Qt Linguist**.
2. Edit translations; mark each entry finished.
3. Compile: `lrelease translations/techaim_de_DE.ts`
4. Rebuild so the `.qm` is re-embedded (it ships inside the executable).
5. Consistency is checked against `docs/german-translation-glossary.md`.

The `.ts` file is the single source; no German strings live in QML or C++.

## Review checklist

- [ ] Range commands are unambiguous to a German range officer.
- [ ] `Probeschüsse` vs `Kontrollschüsse` are not confusable in context —
      the first are never counted, the second always are.
- [ ] `Lage` (shot placement) is never used where `Position`/`Stellung`
      (body position) is meant, and vice versa.
- [ ] The training disclaimer is unmistakably non-official.
- [ ] No German string claims certification, approval or an official result.
- [ ] Long compounds (`Streukreisdurchmesser`) do not overflow their controls;
      report clipping to engineering rather than shortening into an invented
      abbreviation.
- [ ] Umlauts and ß render correctly on screen **and** in exported PDFs.

## Open decisions for the reviewer

1. `Position` vs `Stellung` for the 3P positions — pick one.
2. Whether to abbreviate `Mittlerer Treffpunkt` to `MTP` on compact cards.
3. Whether German range practice prefers `STOP` or `HALT` (the catalogue uses
   `STOP` to match the ISSF English command).

## Not yet verified

German **layout** (clipping, overlap, touch-target size at 1280×720 and
1100×700) and German **PDF export** have not been visually verified. Both are
in `docs/pre-beta-manual-acceptance.md` as human checks.
