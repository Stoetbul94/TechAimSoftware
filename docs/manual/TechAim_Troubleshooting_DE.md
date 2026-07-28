# Tech Aim Electronic Target Control — Fehlersuche

> **GERMAN BETA TRANSLATION — NATIVE TECHNICAL REVIEW REQUIRED**
> **DEUTSCHE BETA-ÜBERSETZUNG — FACHLICHE PRÜFUNG DURCH MUTTERSPRACHLER
> ERFORDERLICH**
>
> Die englische Ausgabe `TechAim_Troubleshooting_EN.md` ist die **verbindliche
> Master-Ausgabe** und enthält **alle** Störungsbilder. Diese Fassung deckt
> die häufigsten Fälle sowie die Entscheidungsbäume ab.

Produktversion 0.9.0 · Dokumentversion 1.0 (P0-J) · Sprache: Deutsch (Beta)
Anwendungs-Basis-Commit `21b40db` · Dokumentations-Commit `cc69939` · Herausgeber: JAC SHOOTING SOLUTIONS (PTY) LTD

**Status: Pre-Beta-Dokumentation. Interne Evaluierung.**

> **Microsoft Defender, SmartScreen oder die Windows-Firewall niemals
> deaktivieren**, um ein Problem zu umgehen. Keine Anweisung in dieser
> Anleitung verlangt das.

**Bei jeder Meldung mitsenden:** Version, **Commit:** und **Built:**
(**Einstellungen ▸ ABOUT / BUILD**), Betriebsmodus, Disziplin und Programm,
Beschreibung von Vorgehen/Erwartung/Ergebnis sowie ggf. das exportierte PDF.
**Keine nicht benötigten personenbezogenen Daten senden.**

---

## 1. Anwendung

### 1.0 Anwendung startet nicht

**PRÜFEN** — Liegt `TechAim.exe` vollständig mit allen Qt-Laufzeitdateien und
dem Ordner `platforms` vor? Läuft bereits eine Instanz?

**NICHT TUN** — die `.exe` allein auf einen anderen Rechner kopieren;
Sicherheitssoftware deaktivieren.

Weitere Ursachen: *[WINDOWS RC1 DEPENDENT]*.

### 1.1 „Tech Aim is already running"

**BEDEUTUNG** — Einzelinstanz-Schutz. Nur eine Instanz darf die Sitzungsdaten
verwenden.

**MASSNAHMEN** — zur laufenden Instanz wechseln; ist keine sichtbar, den
verbliebenen Prozess beenden und neu starten.

**HINWEIS** — Dieser Stand blockiert **absichtlich** auch gegen eine alte
`Seta.exe`: Zwei Programme, die denselben Sitzungsspeicher beschreiben, würden
ihn beschädigen.

**NICHT TUN** — zwei Kopien auf denselben Daten betreiben.

### 1.2 Falsche Sprache / englische Texte im deutschen Modus

**BEDEUTUNG** — meist **kein** Fehler. Die deutsche Übersetzung ist
unvollständig; nicht übersetzte Texte fallen absichtlich auf Englisch zurück.

**MASSNAHMEN** — Sprache erneut wählen; bei Hinweis **Restart required** neu
starten. Ist die Oberfläche nach Wahl von Deutsch **vollständig** englisch,
konnte der Katalog nicht geladen werden — mit dem Startprotokoll melden.

**NICHT TUN** — `config.ini` auf einen nicht angebotenen Sprachcode setzen;
unbekannte Codes fallen auf Englisch zurück.

### 1.3 Abgeschnittene oder überlappende Texte

**URSACHE** — lange deutsche Komposita; kleines Fenster.

**MASSNAHMEN** — Fenster vergrößern; Bildschirm, Sprache und Fenstergröße
notieren und melden.

**Bekanntes Risiko:** Das deutsche Layout ist **nicht** visuell geprüft.
**[GERMAN REVIEW REQUIRED]**

### 1.4 Alte Sitzungsdaten bleiben nach „Home" bestehen

**BEDEUTUNG** — Fehler. Ein sauberes „Home" muss Zähler und Sitzungszustand
zurücksetzen.

**MASSNAHMEN** — Anwendung neu starten, prüfen ob die Sitzung geschlossen ist,
mit Programmnamen und Schritten melden.

---

## 2. Scheibenverbindung

**[PHYSISCHE HARDWARE ERFORDERLICH]**

### 2.1 Scheibe offline / Anzeige wird rot

**URSACHEN** — Stromversorgung, Kabel, falscher COM-Anschluss, Anschluss durch
anderes Programm belegt.

**MASSNAHMEN** — Anschluss im Windows-Geräte-Manager prüfen; andere Programme
schließen; neu verbinden; sonst Scheibe aus- und einschalten und Anwendung neu
starten.

### 2.2 Kein Schuss empfangen

Siehe [Entscheidungsbaum A](#a--kein-schuss-empfangen).

**Häufigste Nicht-Fehler**
- **Demo-Modus gewählt** — physische Schüsse werden abgelehnt.
- **Falsche Phase** — Schüsse während **POSITION SETUP** werden bewusst
  ignoriert.
- **Sitzung bereits abgeschlossen** — es werden keine Schüsse mehr angenommen.

### 2.3 Modus lehnt die Eingabe ab

**BEDEUTUNG** — beabsichtigt. Tech Aim lehnt Schüsse ab, deren Quelle nicht
zum Betriebsmodus passt, damit eine Vorführung nicht als echtes Ergebnis
erscheinen kann.

---

## 3. Trainingslabor

### 3.1 Wettkampfuhr erscheint im Training

**BEDEUTUNG** — Fehler. Wettkampf-Countdown-Anzeigen sind in allen
Trainingsprogrammen ausgeblendet. Mit Programm und Bildschirm melden.

### 3.2 Roter Zähler `000` erscheint im Training

**BEDEUTUNG** — Fehler; wirkt wie ein „Geister-Schuss". Melden.

### 3.3 Trainingsprogramm nicht verfügbar

**URSACHE** — Disziplin-Steuerung. **Positionswechsel gibt es nur bei 50 m
Dreistellungskampf.**

### 3.4 START BLOCK / POSITION READY / START VERIFICATION nicht verfügbar

**BEDEUTUNG** — der Ablauf ist nicht in der Phase, die diese Aktion anbietet.
Reihenfolge: **POSITION SETUP → POSITION READY → (Probeschüsse) → START
VERIFICATION**.

### 3.5 Ansage kann nicht bestätigt werden / Ist-Treffer erscheint zu früh

**BEABSICHTIGT** — es bleibt nur **ein** Schuss offen; der nächste wird
abgelehnt, bis die Ansage bestätigt ist.

**Erscheint der Ist-Treffer VOR der Bestätigung**, ist das ein Fehler, der die
Übung entwertet — sofort mit Schussnummer melden.

### 3.6 Kein Rhythmus angezeigt

**BEABSICHTIGT** — es sind mindestens **drei** gewertete Schüsse mit
vollständigen Zeitangaben nötig. Tech Aim zeigt lieber nichts als einen
unzuverlässigen Wert.

### 3.7 Trefferbild-Analyse meldet „nicht genügend Daten"

**BEABSICHTIGT** — es werden etwa fünf gewertete Schüsse benötigt.

### 3.8 Probeschüsse erscheinen in den Kennzahlen

**BEDEUTUNG** — Fehler. Bericht als Nachweis exportieren und melden.

---

## 4. Berichte

### 4.1 PDF-Export ohne Wirkung

Siehe [Entscheidungsbaum C](#c--pdf-export-schlägt-fehl).

**ZUERST PRÜFEN** — Ist das Ziel beschreibbar? Ist eine gleichnamige Datei in
einem PDF-Betrachter geöffnet?

### 4.2 Fehlende Umlaute / falsche Berichtssprache

**Bekanntes Risiko:** Die deutsche PDF-Ausgabe ist **nicht** geprüft.
**[GERMAN REVIEW REQUIRED]** — PDF und Bildschirmfoto beifügen.

### 4.3 Bericht unerwartet als „Demo" gekennzeichnet

**BEDEUTUNG** — Die Sitzung lief im **Demo**-Modus. Die Kennzeichnung ist
korrekt. **Kennzeichnung oder den Hinweis „Kein offizielles Wettkampfergebnis"
niemals aus einem PDF entfernen.**

---

## 5. Wiederherstellung

### 5.1 Unvollständige Sitzung nicht gefunden

Siehe [Entscheidungsbaum B](#b--sitzung-lässt-sich-nicht-fortsetzen).

### 5.2 Falsche Sitzung angeboten

Sportler, Disziplin, Modus, Phase, Schusszahl und Speicherzeit im Dialog
prüfen. Bei Abweichung **nicht** fortsetzen, sondern melden.

### 5.3 Prüfung schlägt fehl

**BEDEUTUNG** — Der Datensatz konnte nicht als unversehrt bestätigt werden
(z. B. Abbruch durch Stromausfall). Tech Aim meldet das, statt eine
beschädigte Sitzung zu laden.

**NICHTS LÖSCHEN** — zur Untersuchung melden.

### 5.4 Abgeschlossene Sitzung wird zur Wiederherstellung angeboten

**BEDEUTUNG** — Fehler. Vor dem Verwerfen melden.

---

## 6. Windows und Sicherheit

**[WINDOWS RC1 DEPENDENT] — dieser Abschnitt ist bewusst unvollständig.**

Meldungen von Defender/SmartScreen, „Herausgeber konnte nicht verifiziert
werden", Firewall-Abfragen, fehlende DLLs sowie Installations- und
Update-Fehler hängen vom Installationsprogramm und der Signatur ab, die noch
nicht existieren.

---

## 7. Entscheidungsbäume

### A — Kein Schuss empfangen

```
1. Live oder Demo?
   Demo  -> physische Schüsse werden abgelehnt. Auf Live umstellen, neu starten.
   Live  -> weiter.

2. Verbindungsanzeige in Ordnung?
   Nein  -> Abschnitt 2.1 (Strom, Kabel, COM-Anschluss).
   Ja    -> weiter.

3. Richtige Disziplin gewählt?
   Nein  -> Home, neu wählen, Sitzung neu starten.
   Ja    -> weiter.

4. Nimmt die aktuelle Phase Schüsse an?
   POSITION SETUP        -> Schüsse werden BEWUSST ignoriert. POSITION READY,
                            dann START VERIFICATION wählen.
   Sitzung abgeschlossen -> keine Annahme mehr. Neue Sitzung starten.
   Ansage offen (A&D)    -> zuerst die Ansage bestätigen.
   Störung ungelöst      -> offizielle Schüsse gesperrt. Störung klären.
   sonst                 -> weiter.

5. Scheibe eingeschaltet und hat sie den Schuss selbst registriert?
   Nein  -> Anlagen-/Gerätefehler, kein Softwarefehler.
   Ja    -> weiter.

6. COM-/Netzwerkeinstellung korrekt und Anschluss frei?
   Nein  -> korrigieren, neu verbinden.
   Ja    -> weiter.

7. Stellt ein Neuverbinden (oder Aus-/Einschalten + Neustart) den Betrieb her?
   Ja    -> fortsetzen; Unterbrechung melden.
   Nein  -> ESKALIEREN.

8. Erfassen: Version + Commit, Betriebsmodus, Disziplin, Phase,
   COM-/Netzwerkeinstellungen, Protokoll, Zeitpunkt des fehlenden Schusses.
```

### B — Sitzung lässt sich nicht fortsetzen

```
1. Wurde die Sitzung sauber abgeschlossen (Home aus der Zusammenfassung)?
   Ja  -> KORREKTES Verhalten. Abgeschlossene Sitzungen werden nie angeboten.
   Nein -> weiter.

2. Wurde mit "Keep for Recovery" geschlossen?
   Nein / erzwungen -> Wiederherstellung sollte dennoch greifen; weiter.

3. Wird beim Start ein Wiederherstellungseintrag angezeigt?
   Ja  -> Sportler, Disziplin, Modus, Phase, Schusszahl und Zeit prüfen,
          dann Resume Match / Resume Training.
   Nein -> weiter.

4. Ist der Datensatz gültig?
   Als fehlerhaft gemeldet -> NICHTS löschen. ESKALIEREN.
   Kein Eintrag vorhanden  -> weiter.

5. Gleiches Windows-Benutzerkonto und gleicher Rechner?
   Nein -> Sitzungsdaten sind benutzer- und rechnerbezogen.
   Ja   -> weiter.

6. Verwendet die Anwendung die erwartete Datenablage?
   Speicherort im Startprotokoll prüfen.

7. Für den Support sichern: Startprotokoll, Speicherort,
   Sportler/Disziplin/ungefähre Zeit, ob bereits exportiert wurde.
```

### C — PDF-Export schlägt fehl

```
1. Ist der Zielordner für diesen Benutzer beschreibbar?
   Nein -> stattdessen nach "Dokumente" exportieren.
   Ja   -> weiter.

2. Ist eine gleichnamige Datei in einem PDF-Betrachter geöffnet?
   Ja   -> schließen und erneut versuchen (Windows sperrt offene Dateien).
   Nein -> weiter.

3. Enthält der Dateiname \ / : * ? " < > | ?
   Ja   -> umbenennen und erneut versuchen.
   Nein -> weiter.

4. Ist der Bericht am Bildschirm vollständig?
   Nein -> Sitzung ggf. zuerst abschließen.
   Ja   -> weiter.

5. Genügend freier Speicherplatz?
   Nein -> Speicher freigeben.
   Ja   -> weiter.

6. Lässt sich ein ANDERER Bericht exportieren?
   Ja   -> Problem betrifft nur diesen Berichtstyp — welchen, bitte melden.
   Nein -> Problem betrifft den Export allgemein — bitte so melden.

7. Erfassen: Berichtstyp, Zielpfad, genauer Fehlertext,
   Version + Commit, Betriebsmodus, Protokoll.
```

### D — Anwendung startet nicht

**[WINDOWS RC1 DEPENDENT] — nach der Paketierung zu vervollständigen.**

---

**Verwandte Dokumente:** `TechAim_Troubleshooting_EN.md` (Master) ·
`TechAim_Quick_Start_DE.md` · `TechAim_Operator_Manual_DE.md`
