# Tech Aim — German Translation Status

Document version 1.0 (P0-J) · Application commit `3741980`

**GERMAN BETA TRANSLATION — NATIVE TECHNICAL REVIEW REQUIRED**

English is the **controlled master edition**. German is a beta translation of
both the application and the manuals. No professional certification is
claimed.

## Application translation

| Metric | Value |
|---|---|
| Extractable source strings | 583 |
| Translated | 100 |
| Untranslated (render in English) | 483 |
| Coverage | ~17% |
| Catalogue | `translations/techaim_de_DE.ts` → `.qm`, embedded in the binary |
| Language code | `de-DE` |
| Fallback | English (source language) |

**What is translated:** the release-critical core — range commands, the three
shooting positions, sighter/counted-shot vocabulary, primary navigation,
Training Lab programme names, Settings labels and the *"Not an official
competition result"* disclaimer.

**What is not:** most descriptive text, most summary and report body content,
most observations and coaching prompts.

### A German session is currently mixed-language

Untranslated strings fall back to English **by design** — never blank, never a
raw key. A German user therefore sees German controls alongside English
descriptive text. This must be accepted explicitly before any German-language
beta.

### Two coverage limits

1. **Not everything is extractable yet.** Only text wrapped in `qsTr()` (QML)
   or `tr()` (C++) can be translated. Large parts of the Training Lab HUDs and
   report views use bare literals — `PositionTransitionHud.qml` has 8
   `qsTr()` calls in a file full of visible text. **Those strings cannot be
   translated until the code wraps them**, which is a development task, not a
   translation task.
2. **Extractor coverage was previously wrong** — it looked at 16 of 66 product
   QML files. Corrected; the 583 figure reflects the corrected list.

**Consequence:** the true German coverage of the *visible* interface is lower
than 17%, because some visible text is not counted in the 583 at all.

## Manual translation

| Document | Status |
|---|---|
| `TechAim_Quick_Start_DE.md` | German beta — native review required |
| `TechAim_Operator_Manual_DE.md` | German beta — **structural translation**; several parts summarised with an explicit pointer to the English master |
| `TechAim_Troubleshooting_DE.md` | German beta — **key faults translated**; the remainder points to the English master |

The German manuals follow `docs/german-translation-glossary.md` and the
application catalogue. **Manual terminology must match the UI**; where a UI
string is still English, the German manual shows the German term with the
**English UI label in brackets**, so the reader can find the control on
screen.

## Native reviewer scope

1. Verify the range commands against German range practice.
2. Confirm `Probeschüsse` (never counted) and `Kontrollschüsse` (always
   counted) cannot be confused.
3. Confirm `Lage` (shot placement) is never used for `Position`/`Stellung`
   (body position).
4. Confirm no German string claims certification or an official result.
5. Decide `Position` vs `Stellung`, and whether to abbreviate
   `Mittlerer Treffpunkt`.
6. Check German layout for clipping at 1366×768, 1280×720 and 1100×700.
7. Check German PDF output for umlauts, wrapping and overflow.

Reviewers edit `translations/techaim_de_DE.ts` in Qt Linguist — **no
application code changes are needed**. See
`docs/german-translation-review.md`.

## German screenshots

None captured. They must be taken **after** confirming the German UI loads,
and must honestly show the mixed-language state rather than being staged from
translated screens only. See `TechAim_Manual_Screenshot_Register.md`
(SS-28…SS-30).

## Recommendation

**Do not ship a German-language beta as "German support" yet.** At ~17% string
coverage, with unwrapped strings and unverified layout, it is a *preview*.
Either raise coverage substantially and complete native review first, or label
it explicitly as a preview and set expectations accordingly.
