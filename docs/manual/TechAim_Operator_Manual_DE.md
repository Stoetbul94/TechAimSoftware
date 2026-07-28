# Tech Aim Electronic Target Control — Bedienungsanleitung

> **GERMAN BETA TRANSLATION — NATIVE TECHNICAL REVIEW REQUIRED**
> **DEUTSCHE BETA-ÜBERSETZUNG — FACHLICHE PRÜFUNG DURCH MUTTERSPRACHLER
> ERFORDERLICH**
>
> Die englische Ausgabe `TechAim_Operator_Manual_EN.md` ist die **verbindliche
> Master-Ausgabe**. Bei Abweichungen gilt die englische Fassung.
>
> **Umfang dieser Fassung:** Teile 1–4, 7–11 und 13–14 sind vollständig
> übersetzt, da sie den Kern der täglichen Bedienung und des Trainingslabors
> bilden. Die übrigen Teile sind zusammengefasst und verweisen ausdrücklich
> auf die englische Master-Ausgabe. Es wurde **kein** Abschnitt leer gelassen
> und **kein** unvollständiger Status verborgen.

Produktversion 0.9.0 · Release-Kanal: Pre-Beta Validation
Dokumentversion 1.0 (P0-J) · Sprache: Deutsch (Beta)
Veröffentlicht 2026-07-27 · Anwendungs-Basis-Commit `21b40db` · Dokumentations-Commit `cc69939`
Herausgeber: JAC SHOOTING SOLUTIONS (PTY) LTD

**Status: Pre-Beta-Dokumentation. Interne Evaluierung — nicht zur
öffentlichen Verbreitung.**

**Zielgruppen:** 🎯 Sportler · 🧑‍🏫 Trainer · 🛠 Standbetreiber

> **Sprachhinweis.** Englische Bedienelement-Bezeichnungen stehen in Klammern,
> weil die Oberfläche derzeit nur teilweise übersetzt ist.

---

## Teil 1 — Produktübersicht

Tech Aim Electronic Target Control erfasst, wertet und analysiert Schüsse
einer elektronischen Scheibe. Unterstützt werden wettkampfnahe
Qualifikations- und Finalabläufe, freies Training sowie ein
**Trainingslabor (Training Lab)** mit strukturierten Messübungen.

**Unterstützter Umfang**

- Qualifikation: 10 m Luftgewehr, 10 m Luftpistole, 50 m Gewehr liegend,
  50 m Gewehr Dreistellungskampf
- Finale: 10 m Luftgewehr, 10 m Luftpistole, 50 m Gewehr 3-Stellungen
- Freies Training (Open Practice)
- Trainingslabor: Technikblöcke, Ansage & Diagnose, Trefferbild-Analyse,
  Positionswechsel (nur 50 m Dreistellungskampf)
- Grundfunktionen: Live/Demo, Sitzungsaufzeichnung, Wiederherstellung,
  Standstörungen, Berichte und PDF-Export, Sprache Englisch/Deutsch

**Beta-Einschränkungen**

1. Deutsche Übersetzung unvollständig (~100 von 583 Texten) —
   gemischtsprachige Oberfläche.
2. Deutsches Layout und deutsche PDF-Ausgabe **nicht visuell geprüft**.
3. Die Nutzungsvereinbarung zeigt noch ein **SETA-Dokument** eines anderen
   Unternehmens — **rechtliche Sperre** für eine Veröffentlichung.
4. Kein Anwendungssymbol.
5. 25-m-Pistole nicht umgesetzt; 10-Ring-Radius 50 m noch zu bestätigen;
   Lizenzprüfung deaktiviert.

**Software-Identität und SETA-Hardware:** Das **Softwareprodukt** heißt
**Tech Aim** (`TechAim.exe`). **„SETA" bezeichnet zusätzlich den
Elektronik-Lieferanten** und die Bahn-Hardware-Anbindung. Beides darf im
selben System vorkommen.

---

## Teil 2 — Erste Schritte 🛠

1. `TechAim.exe` starten.
2. Fenstertitel prüfen: **Tech Aim Electronic Target Control**.
3. **Einstellungen ▸ ABOUT / BUILD** öffnen und Version, **Commit:** und
   **Built:** mit dem vorgesehenen Stand vergleichen.

Systemvoraussetzungen und Installation: *[WINDOWS RC1 DEPENDENT]* — wird mit
dem Installationsprogramm festgelegt.

---

## Teil 3 — Hauptbildschirm 🎯🧑‍🏫🛠

| Nr. | Bereich | Inhalt |
|---|---|---|
| 1 | Kopfzeile | Sportler, Disziplin, Verbindung |
| 2 | Mitte | Scheibenbild, Treffer, Trefferbild-Overlays, Zusammenfassung |
| 3 | Rechte Leiste | Status und Hauptaktion |

Im Trainingslabor trägt die rechte Leiste die Überschrift **TRAINING LAB** mit
dem Programmnamen darunter und zeigt **Sighters fired** sowie den
Programmstatus.

**Wettkampf-Anzeigen sind im Trainingslabor ausgeblendet.** Weder eine
Wettkampfuhr noch der rote Zähler `000` dürfen erscheinen. Andernfalls liegt
ein Fehler vor.

---

## Teil 4 — Betriebsmodi 🛠

### Live-Ziel (Live target)

**ZWECK** — reale Schüsse der physischen Scheibe auswerten.
**VORAUSSETZUNG** — Scheibe eingeschaltet und verbunden.
**ERGEBNIS** — physische Schüsse werden angenommen, **simulierte abgelehnt**.

### Demo / Simulation

**ZWECK** — Bedienung ohne Scheibe mit erzeugten Schüssen.
**VERWENDUNG** — Einarbeitung, Vorführung, Test, Bildschirmfotos.
**GRENZEN** — **keine physischen Ergebnisse**, niemals als offiziell
darstellen. **Eingaben der physischen Scheibe werden abgelehnt.**

Der Modus wird erzwungen, damit eine Vorführung nicht mit einem echten
Ergebnis verwechselt werden kann. Änderung in **Einstellungen ▸ OPERATING
MODE**, Übernahme über **Restart Now** / **Restart Later**.

---

## Teile 5–6 — Disziplinen, Wettkämpfe und freies Training

Zusammengefasst — **vollständig in der englischen Master-Ausgabe, Teile 5–6.**

Die Disziplinauswahl steuert das weitere Angebot; **Positionswechsel ist nur
bei 50 m Dreistellungskampf verfügbar**. 25-m-Pistole ist nicht umgesetzt.
Der 10-Ring-Radius für 50 m ist noch zu bestätigen — absolute 50-m-Werte
vorläufig behandeln.

---

## Teil 7 — Trainingslabor: Überblick 🎯🧑‍🏫

Strukturierte **Messübungen**, nicht nur ein Schießprogramm.

| Programm | Trainiert | Verfügbar für |
|---|---|---|
| Technikblöcke | ein technisches Element in kurzen Blöcken | unterstützte Disziplinen |
| Ansage & Diagnose | **Schusswahrnehmung** | unterstützte Disziplinen |
| Positionswechsel | Aufbau und Einfinden je Stellung | **nur 50 m 3-Stellungen** |

Die Trefferbild-Analyse ist **kein eigenes Programm**, sondern erscheint als
**GROUP PATTERN INSIGHTS**.

> **Tech Aim berichtet gemessene Muster und Zeiten.
> Die technische Ursache wird dadurch nicht automatisch bewiesen.**

Ein weites Trefferbild ist eine Messung. **Warum** es weit war — Stellung,
Halten, Abzug, Nachhalten, Material, Bedingungen — ist eine trainerische
Beurteilung, die die Software nicht vornimmt.

**Probeschüsse (Sighters) sind in allen Programmen von den gewerteten
Kennzahlen ausgeschlossen.**

---

## Teil 8 — Technikblöcke 🎯🧑‍🏫

**ZWECK** — mehrere kurze Blöcke schießen und sich dabei auf ein technisches
Element konzentrieren. Nach jedem Block zeigt Tech Aim das gemessene
Trefferbild; eine Notiz kann erfasst werden.

**Sichtbarkeitsmodi:** **Full Hidden** (nichts bis zur Auswertung),
**Group Only** (Lage ohne Wertung), **Impact Only** (Treffer ohne Wertung).
Das Ausblenden der Wertung ist beabsichtigt — die Aufmerksamkeit soll beim
Prozess bleiben.

**SCHRITTE**

1. Programm konfigurieren (Blöcke, Schüsse je Block, Fokus, Sichtbarkeit).
2. Ggf. Probeschüsse — bleiben ausgeschlossen.
3. **START BLOCK** in der rechten Leiste wählen.
4. Block schießen; Fortschritt **Shot 0 of N** (nur gewertete Schüsse).
5. **Block Review** öffnet sich.
6. Ergebnis lesen, Notiz unter **ATHLETE NOTE** erfassen, **Save Note**.
7. **CONTINUE TO BLOCK …** oder **End Training**.

**Gemessen werden:** Durchschnittswertung, mittlerer Treffpunkt (MPI),
Streukreisdurchmesser, horizontale und vertikale Streuung, mittleres
Schussintervall und dessen Schwankung.

> **Der Rhythmus wird zwischen den Schüssen gemessen** — als Abstand von einem
> gewerteten Schuss zum nächsten, nicht als Zeitstempel und nicht als Zeit vom
> Startsignal bis zum ersten Schuss.
>
> **Hinweis:** In früheren Ständen war dieser Wert falsch berechnet
> (Mittelung absoluter Zeitstempel). Alte Werte sind **nicht** vergleichbar.

---

## Teil 9 — Ansage & Diagnose 🎯🧑‍🏫

**ZWECK** — **Schusswahrnehmung** trainieren: Kann der Sportler vor dem
Ansehen sagen, wohin der Schuss ging?

**Gemessen wird die Wahrnehmung, nicht die Treffleistung.** Eine gute
Wertung bei schlechter Ansage ist trainerisch das dringendere Problem — das
Ergebnis steht dann noch nicht unter bewusster Kontrolle.

**SCHRITTE**

1. Schuss abgeben.
2. **Der tatsächliche Treffer bleibt verborgen.**
3. Vermutete Lage markieren (**Ansage**).
4. **CONFIRM CALL** wählen.
5. Tech Aim zeigt **CALL** und **ACTUAL** mit der Abweichung.
6. Abweichung prüfen (**Target View** / **Comparison**).
7. **CONTINUE TO NEXT SHOT**.

Es wird **immer nur ein Schuss offen gehalten**; der nächste Schuss wird
abgelehnt, bis die Ansage bestätigt ist. So kann eine Ansage nie dem falschen
Schuss zugeordnet werden.

**Angezeigt:** Ansage-Marker, Ist-Marker, Verbindungsvektor,
**CALL DIFFERENCE** (radiale Abweichung), **HORIZONTAL** / **VERTICAL**,
**EXACT CALL — 0.0 mm**, **OUTSIDE NORMAL TARGET FACE**.

> **Median statt Mittelwert.** Eine einzelne stark abweichende Ansage
> verzerrt den Mittelwert. Der typische (Median-)Wert beschreibt das normale
> Ansageverhalten.

> **Warnung: Eine gerichtete Ansage-Abweichung ist keine Empfehlung zur
> Visierverstellung.** Sie zeigt eine Verschiebung der **Wahrnehmung**. Ein
> Ausgleich über das Visier kann den Wahrnehmungsfehler verfestigen.

---

## Teil 10 — Trefferbild-Analyse 🧑‍🏫

Eine **Analyseebene**, kein eigenes Programm — erscheint als
**GROUP PATTERN INSIGHTS**.

**Unterstützte gemessene Beschreibungen:** enges zentriertes Trefferbild ·
enges versetztes Trefferbild · weites Trefferbild · horizontale Reihe ·
vertikale Reihe · diagonale Reihe · zwei Gruppen · fortschreitende Drift ·
Aufweitung oder Verengung · einzelner Ausreißer.

Jede Beschreibung nennt die **Evidenz** und eine **Konfidenz**. Unterhalb von
etwa fünf gewerteten Schüssen wird „nicht genügend Daten" gemeldet statt
geraten.

**Warum keine eindeutige Ursache genannt wird:** Unterschiedliche Ursachen
erzeugen dasselbe gemessene Muster. Eine vertikale Reihe kann von Atmung,
Halten, Einfinden der Stellung, Nachhalten oder Bedingungen stammen. Tech Aim
nennt daher Muster und Evidenz und überlässt die Ursache dem Trainer
(**Coach discussion:**).

Im Dreistellungskampf werden **kniend, liegend und stehend getrennt**
ausgewertet.

---

## Teil 11 — Positionswechsel 🎯🧑‍🏫

**Nur 50 m Gewehr Dreistellungskampf.**

**ZWECK** — messen, wie gut der Sportler jede Stellung aufbaut und sich
einfindet: Dauer des Aufbaus, Zeit bis zum ersten gewerteten Schuss sowie
frühes Trefferbild und Rhythmus.

| Phase | Ablauf |
|---|---|
| **POSITION SETUP** | Stellung aufbauen. **Schüsse werden ignoriert.** Optionale **SETUP CHECKLIST**. **Setup time** läuft. |
| **POSITION READY** | Stellung als fertig erklären; Zeitmessung bis zum ersten gewerteten Schuss beginnt. |
| Probeschüsse | optional, bleiben ausgeschlossen |
| **START VERIFICATION** | gewerteter Block beginnt |
| Kontrolle | N gewertete Schüsse, **Shot 0 of N** |
| **Position Review** | gemessenes Ergebnis der Stellung |
| **BEGIN TRANSITION TO …** | Wechsel zur nächsten Stellung |
| Zusammenfassung | **POSITION TRANSITION COMPLETE** |

**Schüsse während POSITION SETUP werden bewusst ignoriert** — die Stellung
wird aufgebaut, nicht geschossen. Das ist kein Fehler.

### Die Zeitmessungen

| Zeit | Von → bis | Aussage |
|---|---|---|
| Aufbau-/Wechselzeit | Phasenbeginn → **POSITION READY** | Dauer des Stellungsaufbaus |
| Probeschussdauer | Ready → letzter Probeschuss | Zeit zur Bestätigung |
| Ready → erster gewerteter Schuss | **POSITION READY** → erster gewerteter Schuss | Zeit bis zur Schussabgabe (**einschließlich Probeschussphase**) |
| Kontrolldauer | Ready → letzter gewerteter Schuss | Länge des gewerteten Blocks |
| Mittleres Schussintervall | zwischen gewerteten Schüssen | Arbeitsrhythmus |
| Rhythmusschwankung | Streuung der Intervalle | Gleichmäßigkeit |

> **„Ready → erster gewerteter Schuss" schließt die Probeschussphase ein.**
> Nur mit vergleichbarer Probeschuss-Praxis vergleichen.

### Rhythmus-Einstufung (wie umgesetzt)

| Bezeichnung | Bedeutung |
|---|---|
| **Steady** (gleichmäßig) | nahezu konstante Intervalle (Variationskoeffizient < 0,20) |
| **Variable** (schwankend) | mittlere Schwankung (0,20 – 0,40) |
| **Inconsistent** (unregelmäßig) | starke Schwankung (ab 0,40) |

> **Eine Rhythmus-Einstufung allein beweist keine gute oder schlechte
> Technik.** „Steady" kann kontrolliert **oder** gehetzt bedeuten;
> „Inconsistent" gestört **oder** angemessen geduldig.

Unter drei gewerteten Schüssen oder ohne vollständige Zeitangaben wird
**keine** Einstufung angezeigt.

> **Jede Stellung mit sich selbst über Wiederholungen vergleichen**, nicht mit
> einer anderen Stellung. Ein weiteres Trefferbild im Stehen als im Liegen ist
> zu erwarten und kein Befund.

**Die Zusammenfassung enthält:** Überblick · **SESSION HIGHLIGHTS**
(schnellster/langsamster Aufbau, engstes Trefferbild, beste Durchschnitts-
wertung, gleichmäßigster Rhythmus) · **POSITIONS** (Karte je Stellung mit
Mini-Trefferbild, erstem Schuss, MPI, Kennzahlen, Rhythmus-Kennzeichen,
Vergleichsbalken, Trefferbild-Beschreibung) · **WHAT YOU SHOULD TAKE FROM
THIS SESSION** · **EXPORT PDF**, **NEW SESSION**, **Home**.

**Home** schließt die Sitzung dauerhaft; beim erneuten Öffnen darf kein alter
Zustand erscheinen.

---

## Teil 12 — Berichte und PDF-Export

Zusammengefasst — **vollständig in der englischen Master-Ausgabe, Teil 12.**

**EXPORT PDF** wählen und Ziel bestätigen. Ergebnis: ein A4-PDF im
Tech-Aim-Layout mit vergrößertem Logo, Sitzungsdaten, der Software-Angabe
**Tech Aim 0.9.0**, Seitenzahlen und — bei Trainingsberichten — dem Hinweis
*„Kein offizielles Wettkampfergebnis"*. **Wettkampfberichte tragen diesen
Hinweis nicht.**

*Deutsche PDF-Ausgabe ist noch **nicht** geprüft.* **[GERMAN REVIEW REQUIRED]**

---

## Teil 13 — Sitzungslebenszyklus 🛠

Neu → aktiv → abgeschlossen → geschlossen.

**Home** schließt sauber; **NEW SESSION** startet eine neue Sitzung mit neuer
Kennung. Beim Schließen einer aktiven Sitzung bietet Tech Aim **Save and
Close** oder **Keep for Recovery** (mit **Cancel**).

> **Doppelte Sitzungen vermeiden:** immer über **Home** oder **NEW SESSION**
> beenden. Ein erzwungenes Schließen hinterlässt eine unvollständige Sitzung.

Gespeichert wird der fortlaufende Sitzungsdatensatz (Schüsse, Phasen,
Notizen, Zeiten) — **kein** Video, Audio oder biometrische Daten.

---

## Teil 14 — Wiederherstellung 🛠

> **Tech Aim zeichnet den Fortschritt laufend auf, sodass eine unterbrochene
> Sitzung normalerweise sicher fortgesetzt werden kann.**

Nach Stromausfall, Absturz oder Neustart erscheint beim nächsten Start der
Wiederherstellungsdialog mit Sportler, Disziplin, Modus, Phase, Schusszahl
und Speicherzeitpunkt: **Resume Match** / **Resume Training** oder
**Discard**.

**Immer gültig:** Eine **sauber abgeschlossene** Sitzung wird **nie** als
unvollständig angeboten; eine unvollständige nie als abgeschlossen; Schüsse
werden weder erfunden noch verworfen.

**Integrität, einfach erklärt:** Jeder Schritt ist mit dem vorherigen
verkettet. Wird eine Datei abgeschnitten oder verändert, erkennt Tech Aim, bis
wohin der Datensatz vertrauenswürdig ist, statt eine beschädigte Sitzung
stillschweigend zu laden.

**Sofort melden**, wenn eine abgeschlossene Sitzung zur Wiederherstellung
angeboten wird, eine Schusszahl falsch ist oder die Prüfung fehlschlägt —
**nichts löschen**.

---

## Teile 15–20 — Standstörungen, Einstellungen, Daten, Updates, Fehlersuche, Glossar

Zusammengefasst — **vollständig in der englischen Master-Ausgabe, Teile
15–20.**

- **Standstörungen (Teil 15):** Kategorien wie *Target not registering*,
  *Power failure*, *Communication failure*; Umfang *Individual firing point* /
  *Selected firing points* / *Entire relay / range*. Bei ungelöster Störung
  sind offizielle Schüsse gesperrt (**DO NOT FIRE — RANGE INCIDENT**).
  **Tech Aim erfasst die Störung, entscheidet aber nicht über die
  Wettkampffolge** — das ist Sache der Jury.
- **Einstellungen (Teil 16):** **LANGUAGE**, **OPERATING MODE**,
  **MOTOR FEED (SECONDS)**, **Background Color**, **Pellet Color**,
  **ABOUT / BUILD**. Sprache und Marke sind unabhängig: Deutsch ändert
  **nie** Logo, Farbschema, Anwendungsnamen, Herausgeber, Datenablage oder
  Betriebsmodus.
- **Daten und Datenschutz (Teil 17):** Sitzungsdaten in den lokalen
  Windows-Anwendungsdaten unter **TechAim**; PDFs im gewählten Ordner;
  Einstellungen in `config.ini`. Endgültige Pfade: *[WINDOWS RC1 DEPENDENT]*.
  **Notizen werden wörtlich gespeichert und gedruckt** — keine
  personenbezogenen oder medizinischen Angaben eingeben.
- **Updates (Teil 18):** *[WINDOWS RC1 DEPENDENT]*.
- **Fehlersuche (Teil 19):** `TechAim_Troubleshooting_DE.md` (Beta) bzw.
  `TechAim_Troubleshooting_EN.md` (Master).
- **Glossar (Teil 20):** Begriffe siehe englische Master-Ausgabe und
  `docs/german-translation-glossary.md`.

---

**Verwandte Dokumente:** `TechAim_Operator_Manual_EN.md` (Master) ·
`TechAim_Quick_Start_DE.md` · `TechAim_Troubleshooting_DE.md` ·
`TechAim_German_Translation_Status.md`
