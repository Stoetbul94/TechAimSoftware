# German Translation Review — Tech Aim

**Status: German Beta Translation — Native Review Required.**

This document tells a German-speaking reviewer exactly what exists, what does
not, and how to change it without touching application code.

## Current state (P0)

| Metric | Value |
|---|---|
| Extractable source strings | 583 |
| Translated | 100 |
| Untranslated (render in English) | 483 |
| Catalogue | `translations/techaim_de_DE.ts` |
| Compiled | `translations/techaim_de_DE.qm`, embedded via `techaim_translations.qrc` |
| Language code | `de-DE` |
| Source/fallback language | `en` |

**The 100 translated strings are the release-critical core**: range commands,
the three shooting positions, sighter/counted-shot vocabulary, primary
navigation, Training Lab programme names and the "not an official competition
result" disclaimer.

**The 483 untranslated strings render in English.** This is by design, not a
defect: Qt falls back to the source string, so the UI is never blank and never
shows a raw key. It does mean a German session is currently **mixed-language**.

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
