# DSB 2026 — source register

Every claim in the other three DSB documents traces to a row here. A claim with
no row is not a claim; it is a question.

## Primary authority

| | |
|---|---|
| Document | **Sportordnung des Deutschen Schützenbundes** |
| Edition | **Stand 01.01.2026**, 1. Auflage, ISBN 978-3-96416-121-5 |
| Publisher | Deutscher Schützenbund e.V., Lahnstraße 120, 65195 Wiesbaden |
| Consulted form | Official free online flip catalogue, <https://dsb.de/fileadmin/dsb/sportordnung/> |
| Consulted on | 2026-08-18 |

The DSB states that only its own authorised publications are binding — the
printed rulebook and the online Sportordnung on the DSB site. The flip catalogue
used here **is** that online version. Nothing below comes from a forum, a club
page, Wikipedia or a vendor summary.

**Citation format.** `SpO 2026, <part>, S. <printed page>` plus the catalogue
page in brackets, because the catalogue paginates the whole book continuously
while the rulebook paginates per part. Mapping used:
Teil 0 printed page *n* = catalogue page *n+18*; Teil 1 *n* = *n+112*;
Teil 2 *n* = *n+134*.

**No long passages are reproduced anywhere in this repository.** What is
recorded is rule numbers, titles, and the numeric parameters a scoring
application must implement — facts, not text.

---

## S-0 — General and electronic-target rules (Teil 0)

| ID | Rule | Title (DE) | Location | Supports |
|---|---|---|---|---|
| S-0.1 | 0.4.1 | Wettkampfscheiben | Teil 0, S. 10 [p28] | Only DSB-approved competition targets and electronic targets may be used at championships qualifying for the DM |
| S-0.2 | 0.4.1.1 | Schießfolge | Teil 0, S. 10 [p28] | Numbered targets are shot in ascending order; violation = 2-ring deduction from the first series |
| S-0.3 | 0.4.2 | Probescheiben | Teil 0, S. 10 [p28] | Sighting targets must be clearly marked and must be provided to the shooter |
| S-0.4 | 0.4.3.1 | Elektronische Scheiben — Definition | Teil 0, S. 10 [p28] | EST determines the shot value electronically from a measuring medium; acoustic, light-barrier and paper/scanner systems are all recognised |
| S-0.5 | 0.4.3.2 | Aufbau einer elektronischen Scheibe | Teil 0, S. 12 [p30] | Required parts: Messteil, Rechner + **Schützenmonitor**, Drucker. The computer stores **x/y coordinates, ring value, deviation from centre and time of measurement** for every shot of the competition |
| S-0.6 | 0.4.3.2 | Bedienung / PROBE↔WETTKAMPF | Teil 0, S. 12 [p30] | The shooter may set the monitor display mode (zoom/full) and switch PROBE↔WETTKAMPF; the competition management may restrict the switch to officials; returning to PROBE after switching to WETTKAMPF is allowed **only while no competition shot has been fired** |
| S-0.7 | 0.4.3.2 | Bestätigung des Ergebnisausdrucks | Teil 0, S. 12 [p30] | If the athlete does not accept the result they must tell the range officer when leaving the point; the officer notes the time and a written protest must follow immediately |
| S-0.8 | 0.4.3.2 | Zentralrechner | Teil 0, S. 13 [p31] | Results of all electronic targets converge in a central computer, which produces rankings and drives remote displays/screens during the competition |
| S-0.9 | 0.4.3.2 | Kontrollscheiben / Kontrollblätter | Teil 0, S. 13 [p31] | Control targets behind the EST are required wherever no continuous paper strip records all shots; change only after the series result is fixed |
| S-0.10 | 0.8.1 | Störungen an Waffen und Munition | Teil 0, S. 27 [p45] | Max **15 min** time credit for weapon repair/replacement; extra sighters need the shooting director's approval; credits only where the shooter did not cause the interruption |
| S-0.11 | 0.8.2 | Falsches Kommando am Stand | Teil 0, S. 27 [p45] | A wrong range command must be challenged immediately; not recognised once the shooter has fired after it |
| S-0.12 | 0.8.3.1 | Dokumentation | Teil 0, S. 27 [p45] | **All** interruptions and time credits must be documented in writing by the shooting director, jury and/or referee |
| S-0.13 | 0.8.3 | Unterbrechung | Teil 0, S. 28 [p46] | >3 min interruption without own fault → time credit for the lost time, **+1 min** if it falls in the last 5 minutes; point change or >5 min → credit **+5 min**, and **unlimited sighters** before resuming counted shots |
| S-0.14 | 0.8.4 | Defekte von Scheibenanlagen mit elektronischer Wertung | Teil 0, S. 28 [p46] | Lost shooting time must be recorded; all counted shots of every shooter must be counted and noted; on power failure wait for restoration — shots registered on the target but no longer visible on the monitor are established and counted |
| S-0.15 | 0.8.4.1 | Defekt an einer einzelnen Scheibe | Teil 0, S. 28 [p46] | If the service team cannot fix it, the shooter is moved to a replacement point |
| S-0.16 | 0.8.4.2 | Defekt einer Scheibengruppe oder aller Anlagen | Teil 0, S. 28 [p46] | After repair the remaining competition time is extended by **5 min**; restart announced ≥5 min in advance; shooters then get **5 min preparation**; unlimited sighters before the remaining counted shots |
| S-0.17 | 0.8.5.1 | Einspruch wegen nicht angezeigten Schusses | Teil 0, S. 28–29 [p46–47] | Shooter must inform the nearest official **without firing another shot**; official records the protest time; shooter is then instructed to fire an **extra competition shot**; if all shots register, the **last** shot fired is struck; if the disputed shot is nowhere found, only correctly registered shots (incl. the extra shot) count; if the extra shot also fails and the system is not repaired within **5 min**, a replacement point is assigned with **+5 min** and unlimited sighters |
| S-0.18 | 0.8.5.2 | Proteste gegen die Wertung | Teil 0, S. 29 [p47] | A scoring protest is only admissible **before the next shot** (or within **3 min** for the last counted shot); rejected protest = **2-ring deduction** plus a fee — except against a zero score or a non-registration; an extra shot is fired at the end and counts if the protest succeeds and the disputed value cannot be established |
| S-0.19 | 0.8.5.3 | Beschwerde während des Probeschießens | Teil 0, S. 30 [p48] | Jury may move the shooter to a replacement point with extra sighters and a time credit; if the original target is later shown to have been correct, a 2-ring penalty applies |
| S-0.20 | 0.8.5.4 | Fehlfunktion des Papier-/Gummibandes | Teil 0, S. 30 [p48] | Replacement point, **+5 min**, unlimited sighters, then the remaining shots plus a jury-determined number of repeat shots |
| S-0.21 | 0.8.5.5 | Prozedur nach Protest/Nichtanzeige | Teil 0, S. 30–31 [p48–49] | Evidence a jury member collects includes the **LOG printout**, the **central-computer data**, the black paper band (10 m) / rubber band (50 m), control targets and the range report; **CLEAR LOG only with the classification jury's permission** |
| S-0.22 | 0.9.1 | Olympische Wettbewerbe | Teil 0, S. 31 [p49] | Table of Olympic competitions/classes includes 1.10, 1.40, 2.10 |
| S-0.23 | 0.20 | Anhang — Tabelle der Scheiben | Teil 0, S. 62 [p80] | Target dimensions, all in mm — see the programme matrix |
| S-0.24 | 0.21 | Anhang — Wettbewerbe des DSB | Teil 0, S. 76–79 [p94–97] | Master competition table: Kennzahl, calibre, distance, position, shot counts, target number |

## S-1 — Rifle (Teil 1)

| ID | Rule | Title (DE) | Location | Supports |
|---|---|---|---|---|
| S-1.1 | 1.1.1–1.1.4 | Anschlagarten | Teil 1, S. 1–2 [p113] | Prone, standing, kneeling, sitting definitions |
| S-1.2 | 1.6 | Festlegungen für Dreistellungswettbewerbe | Teil 1, S. 13 [p125] | **Order is kniend → liegend → stehend**; from Herren II / Damen II kneeling may be replaced by **sitzend**; in KK 3×20, KK 3×40 and 300 m Freigewehr the rifle *and* accessories may be changed between positions |
| S-1.3 | — | Wettbewerbstabelle Gewehr | Teil 1, S. 18–19 [p130–131] | Equipment table: 1.10 = 20/40/60 shots standing; **1.20 = 20/20/20**, kn/lg/st |
| S-1.4 | — | Schießzeiten Gewehr | Teil 1, S. 20 [p132] | Timing table — see programme matrix |
| S-1.5 | — | Anmerkung zur Schießzeitentabelle | Teil 1, S. 20 [p132] | **Common preparation time is 15 min including an unlimited number of sighters before the start, and is NOT included in the shooting times.** In three-position events this 15-minute preparation/sighting period is taken **before the kneeling position**; sighting before prone and before standing is **at the shooter's discretion**. Stand-occupation times are set by the organiser |
| S-1.6 | — | Zeitablauf (Beispiel Liegendkampf) | Teil 1, S. 17 [p129] | −15 min call to the points (set up weapon and aids, holding/dry-fire practice, equipment checks); −30 s stop; ±0 start of the **total time (sighting and competition)**; a 5-shot sighting series before the standing position is permitted and **is contained in the competition time** |

## S-2 — Pistol (Teil 2)

| ID | Rule | Title (DE) | Location | Supports |
|---|---|---|---|---|
| S-2.1 | 2.11 / 2.11.2 / 2.11.3 | 10 m Luftpistole (2.10) | Teil 2, S. 12 [p146] | Times per Pistolentabelle; **unlimited sighters during the preparation/sighting time before the competition series**; the shooting director announces shot count and competition time and starts with the command **„START"** |
| S-2.2 | 2.12.1–2.12.4 | 10 m Mehrschüssige Luftpistole (2.16) | Teil 2, S. 12–13 [p146–147] | 30-shot round = **6 series of 10 s**; 60-shot round = **12 series of 10 s**; each series is **5 shots on 5 falling targets (Klappscheiben)**; zeroing on a stationary 10 m pistol target in **150 s** before the competition; **one sighting series before each round**; command sequence **LADEN** → 1 min → **ACHTUNG 3–2–1–START**; with optical signalling the shooting time begins when the lamp goes out after 3 s (±1 s) and ends when it lights again; a target counts as hit only if it falls **within** the shooting time |
| S-2.3 | 2.13.2–2.13.4 | 10 m Pistole Mehrkampf (2.17) | Teil 2, S. 14 [p148] | **One sighting series before the precision part and one before the rapid-fire part**; Part 1 precision = 4 series × 5 shots, 150 s each; Part 2 rapid = 4 series × 5 shots in **3/7 s** mode; conduct **as 25 m Pistole (2.40)** |
| S-2.4 | 2.14.2–2.14.4 | 10 m Pistole Standard (2.18) | Teil 2, S. 14 [p148] | **One sighting series of 5 shots in 150 s** before the competition; Part 1 = 4 × 5 in 150 s; Part 2 = 4 × 5 in 20 s; conduct **as 25 m Standardpistole (2.60)** |
| S-2.5 | — | Pistolentabelle | Teil 2, S. 24 [p158] | Timing table — see programme matrix |
| S-2.6 | — | Anmerkung zur Pistolentabelle | Teil 2, S. 25 [p159] | Common preparation time **15 min incl. unlimited sighters before the start, not contained in the listed shooting times** |

## S-15 — Finals (Teil 15)

| ID | Rule | Location | Supports |
|---|---|---|---|
| S-15.1 | Teil 15 (Finalregeln und Endkampfregeln) | [p485–512] | DSB carries its own finals part, including a **FINALE – DREISTELLUNGSKAMPF 50 M GEWEHR** section and Mix-Team rules. **Not yet read in detail** — see the open questions |

---

## S-DM — DSB Deutsche Meisterschaften 2026 (Ausschreibung)

| | |
|---|---|
| Document | **DSB Ausschreibungen 2026 — Deutsche Meisterschaften — Wettbewerbe** (Schusszahlentabelle) |
| Location | <https://www.dsb.de/schiesssport/ausschreibungen-2026/deutsche-meisterschaften/wettbewerbe> |
| Consulted on | 2026-08-18 |
| Role | **COMPETITION CONTEXT**, not base rule. It settles what the DM actually runs, which the Sportordnung deliberately leaves open |

| ID | Supports |
|---|---|
| S-DM.1 | The table carries an explicit **Zehntel (decimal)** column per competition and class. Scoring mode is therefore **per programme and per competition**, not a property of the discipline |
| S-DM.2 | **1.20** at the DM 2026 is listed for **Schüler 1 / Jugend at 60 shots** |
| S-DM.3 | **1.40** at the DM 2026: all announced classes, 60 shots; a final is listed for Herren I / Jun. I m / Damen I / Jun. I w |
| S-DM.4 | **1.60** at the DM 2026: all announced classes, 120 shots |
| S-DM.5 | **2.10**: Schüler 1 = 20 shots; Herren/Damen I–II and Junioren I–II = 40 (LM) / **60 (DM)**; remaining classes 40. **Zehntel = nein** |
| S-DM.6 | **2.20**: all classes 60 shots. **Zehntel = nein** |
| S-DM.7 | Events read as carrying **Zehntel = ja** include **1.10, 1.11, 1.12, 1.80**, the Auflage variants (1.36, 1.41, 2.10 Auflage) and Para rifle events |

> **Reliability note, recorded deliberately.** Two independent automated reads of
> this page on the same day disagreed about **1.40**: one reported no Zehntel
> marking, the other listed 1.40 among the decimal events. The customer
> determination (S-C.1) says whole ring. **1.40's scoring mode must be confirmed
> by eye against the printed table before it is implemented** — see Q3a. Nothing
> else on the page was contradictory.

## S-C — Customer rule determinations

Supplied by the customer on 2026-08-18 as the answers to the open questions
raised by this research. Recorded as **customer authority**: it is the basis on
which implementation may proceed, and it is distinguishable from the
Sportordnung itself.

| ID | Determination |
|---|---|
| S-C.1 | For the DSB 2026 German Championship, **1.20, 1.40 and 1.60 use whole-ring scoring**, not decimal |
| S-C.2 | **10 m 3×15 is not a DSB 2026 national programme** and must not be implemented as DSB 1.20. It may later become a configurable local/custom profile if authority is supplied |
| S-C.3 | **50 m 3×10 is not an official DSB 1.40 programme.** The 30-shot row is a recommendation and never states a 3 × 10 split; it must not be presented as official DSB |
| S-C.4 | **Class → shot count must not be hardcoded globally.** The DM runs 1.20 at 60 shots for Schüler/Jugend while regional competitions also run Schüler 3 × 10, so the programme variant belongs to the **event configuration** |
| S-C.5 | The ordinary 1.20 rules prescribe **no complete ISSF-style command script**. Range announcements are to be configurable/localised and must **not** be part of the rules engine |
| S-C.6 | For 1.20 the three position clocks are **not automatically chained**. Finish a position → enter **`POSITION_CHANGE`** → the range officer / control software starts the next position. Stand-occupation timing is the organiser's |

---

## Open authority — status after the customer determinations

| Q | Question | Status |
|---|---|---|
| Q1 | Is 10 m 3×15 a DSB programme? | **CLOSED — NO** (S-C.2). Not DSB; may become a custom profile only with its own authority |
| Q2 | What is the 30-shot row under 1.40? | **CLOSED — not an official 3 × 10** (S-C.3). A recommended time for a shortened course, nothing more |
| Q3 | Integer vs decimal per programme | **CLOSED for 1.20 / 1.60** (whole ring, S-C.1) and **for 2.10 / 2.20** (nein, S-DM.5/6). **1.10 and 1.80 are decimal** (S-DM.7) |
| Q3a | **1.40 scoring mode** | **OPEN** — S-C.1 says whole ring; one read of the DM table listed 1.40 as decimal. Confirm visually before implementing 1.40 |
| Q4 | Class → programme variant | **CLOSED as an architecture decision** (S-C.4): variant is event configuration, never a hardcoded class rule |
| Q5 | Rifle range-officer commands | **CLOSED** (S-C.5): configurable and localised, outside the rules engine |
| Q6 | Automatic vs commanded position clocks | **CLOSED — commanded** (S-C.6): explicit `POSITION_CHANGE` state between position clocks |
| Q7 | Finals applicability per programme | **OPEN** — Teil 15 not yet read in detail. S-DM.3 shows the DM runs a **1.40 final** for four classes |
| Q8 | Does the SpO mandate a Zentralrechner interface? | **OPEN** — 0.4.3.2 requires a central computer at the event but specifies no protocol. RMS scope |
