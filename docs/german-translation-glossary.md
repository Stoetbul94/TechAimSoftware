# German Translation Glossary — Tech Aim

**Status: German Beta Translation — Native Review Required.**
No professional certification is claimed. A native German-speaking shooting
coach or range operator must review this before any German-language beta.

Catalogue: `translations/techaim_de_DE.ts` (compiled to `.qm`, shipped in the
binary). English is the source language; anything untranslated renders in
English by design.

## Principles

1. **Shooting terminology beats literal translation.** Use the term a German
   range officer or ISSF-licensed coach would actually say, not a word-for-word
   rendering of the English.
2. **Be consistent.** One English term maps to exactly one German term
   throughout the UI and the PDF reports.
3. **Do not translate** athlete names, user notes, file paths, session IDs, Git
   hashes, protocol constants, hardware register names, vendor model names or
   raw diagnostic codes.
4. **Do not translate the product name.** "Tech Aim" and "TechAim" are the
   brand and stay untranslated in every language.
5. **Range commands are safety-critical.** They must be unambiguous and match
   established German range practice.

## Core terms

| English | German | Note |
|---|---|---|
| Live Target | Live-Ziel | operating mode |
| Demo / Simulation | Demo / Simulation | keep both, as in English |
| Sighters | Probeschüsse | never counted |
| Counted shot | Gewerteter Schuss | |
| Block | Block | Training Lab unit |
| Block Review | Blockauswertung | |
| Technical Blocks | Technikblöcke | programme name |
| Call & Diagnose | Ansage & Diagnose | programme name |
| Called position | Angesagte Lage | what the athlete calls |
| Actual position | Tatsächliche Lage | what the target measured |
| Group diameter | Streukreisdurchmesser | extreme spread |
| MPI / mean point of impact | Mittlerer Treffpunkt | abbreviate as MTP if space-critical |
| Horizontal spread | Horizontale Streuung | |
| Vertical spread | Vertikale Streuung | |
| Position Transition | Positionswechsel | programme name |
| Kneeling | Kniend | |
| Prone | Liegend | |
| Standing | Stehend | |
| Position Ready | Position bereit | |
| Verification shots | Kontrollschüsse | the counted block after a transition |
| Range Incident | Standstörung | EST malfunction workflow |
| Resume session | Sitzung fortsetzen | recovery |
| Training Session | Trainingseinheit | |
| Not an official competition result | Kein offizielles Wettkampfergebnis | **must appear on every training report** |

## Range commands (safety-critical)

| English | German |
|---|---|
| LOAD | LADEN |
| START — FIRE | START — FEUER |
| STOP — UNLOAD | STOP — ENTLADEN |
| TAKE YOUR POSITIONS | AUF DIE POSITIONEN |
| DO NOT FIRE — RANGE INCIDENT | NICHT FEUERN — STANDSTÖRUNG |
| SIGHTING | PROBESCHIESSEN |
| MATCH | WETTKAMPF |

The separator is an em dash (U+2014), matching the English source.

### Match-screen command banners (added 2026-11-14 — review first)

| English | German |
|---|---|
| PREPARATION AND SIGHTING TIME...START | VORBEREITUNGS- UND PROBEZEIT...START |
| END OF PREPARATION AND SIGHTING...STOP | ENDE DER VORBEREITUNGS- UND PROBEZEIT...STOP |
| MATCH FIRING...START | WETTKAMPF...START |
| 10 MINUTES / 5 MINUTES / 30 SECONDS | 10 MINUTEN / 5 MINUTEN / 30 SEKUNDEN |
| STOP | STOP |

### Range incident / athlete home terms (added 2026-11-14 — review first)

| English | German | Note |
|---|---|---|
| EST incident | EST-Störung | glossary "Range Incident" = Standstörung |
| Range Officer | Standaufsicht | |
| Jury | Jury | |
| Firing point (FP) | Stand | |
| Relay | Durchgang | |
| Time credit | Zeitgutschrift | |
| No additional allowance | Keine zusätzliche Zeit | |
| Reserve target | Ersatzscheibe | |
| Backup score | Sicherungswertung | |
| Official shots | Wertungsschüsse | |
| Operator (athlete-mode PIN) | Aufsicht | |
| Practice (athlete tab) | Training | |
| MPI (reports) | MTP | space-critical form of Mittlerer Treffpunkt |
| Group (reports) | Streukreis | |
| Inner 10 | Innenzehner | |
| Call (Call & Diagnose) | Ansage | "called shots" = angesagte Schüsse |
| Standard deviation (SD) | Std.-Abw. | column headers and stat lines |
| Average (column) | Ø | space-critical; "Durchschnitt" where room allows |
| Split (time since previous shot) | Zwischenzeit | Finals report |
| Hold | Haltezeit | Finals report |
| Scoring (mode) | Wertungsart | |
| Evidence: insufficient / low / moderate / strong | Evidenz: unzureichend / gering / mittel / stark | group-pattern confidence |
| Repeat (Position Transition) | Wiederholung; row tag R1 → W1 | |
| Setup / transition | Aufbau / Wechsel | |
| Generated (report footer) | Erstellt | |
| Page 1 of 6 | Seite 1 von 6 | |
| Coach Report | Trainerbericht | |
| Position (body) | Stellung | used throughout the reports |

## Reviewer notes

- **`Streukreisdurchmesser` is long.** It is the correct term, but it is 24
  characters against the English "Group diameter" at 14. Where a card label
  overflows, prefer `Streukreis` (with the unit making the meaning clear)
  rather than inventing an abbreviation.
- **`Lage` vs `Position`.** Call & Diagnose uses `Lage` for where a shot sat on
  the target; Position Transition uses `Position`/`Stellung` for the body
  position. Keeping these distinct matters — conflating them makes the Call &
  Diagnose screens read as if they were about kneeling/prone/standing.
- **`STOP`** is deliberately not germanised to `HALT`: `STOP` is understood on
  German ranges and matches the ISSF English command.
- Decide `Stellung` vs `Position` for the 3P positions and apply it
  consistently; this catalogue currently uses `Position`/`Positionswechsel`.
