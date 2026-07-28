# Tech Aim Electronic Target Control — Schnellstart-Anleitung

> **GERMAN BETA TRANSLATION — NATIVE TECHNICAL REVIEW REQUIRED**
> **DEUTSCHE BETA-ÜBERSETZUNG — FACHLICHE PRÜFUNG DURCH MUTTERSPRACHLER
> ERFORDERLICH**
>
> Die englische Ausgabe (`TechAim_Quick_Start_EN.md`) ist die **verbindliche
> Master-Ausgabe**. Bei Abweichungen gilt die englische Fassung.

Produktversion 0.9.0 · Release-Kanal: Pre-Beta Validation
Dokumentversion 1.1 (P0-J refinement) · Sprache: Deutsch (Beta)
Veröffentlicht 2026-07-27 · Anwendungs-Basis-Commit `21b40db` · Dokumentations-Commit `cc69939`
Herausgeber: JAC SHOOTING SOLUTIONS (PTY) LTD

**Status: Pre-Beta-Dokumentation. Interne Evaluierung — nicht zur
öffentlichen Verbreitung.**

> **Hinweis zur Oberfläche.** Die deutsche Übersetzung der Anwendung ist
> unvollständig (rund 100 von 583 Texten). Nicht übersetzte Texte erscheinen
> **absichtlich auf Englisch**. Eine deutsche Sitzung ist daher derzeit
> **gemischtsprachig**. Englische Bedienelement-Bezeichnungen stehen in
> dieser Anleitung in Klammern, damit Sie sie am Bildschirm wiederfinden.

---

## 1. Willkommen bei Tech Aim

Tech Aim ist eine Software zur elektronischen Trefferauswertung und zum
Training für ISSF-Disziplinen. Sie erfasst und wertet jeden Schuss aus und
liefert Sportlern und Trainern gemessene Rückmeldungen — sowohl für
wettkampfnahe Sitzungen als auch für strukturierte Übungen im **Trainingslabor
(Training Lab)**.

## 2. Sicherheit und Geltungsbereich

- **Die Sicherheitsregeln des Schießstandes haben immer Vorrang.** Tech Aim
  zeigt Kommandos an, steuert den Stand aber nicht.
- Dies ist ein **Pre-Beta-Validierungsstand**. Keine zertifizierte Software,
  weder ISSF- noch SETA-zertifiziert.
- **Ergebnisse im Demo-Modus sind keine physischen Ergebnisse** und dürfen
  niemals als offiziell dargestellt werden.
- Trainingsberichte tragen den Hinweis *„Kein offizielles Wettkampfergebnis"*
  (*Not an official competition result*). Diesen Hinweis nicht entfernen.

## 3. Voraussetzungen

| Element | Hinweis |
|---|---|
| Windows-PC | Windows 10/11 |
| `TechAim.exe` | Die Anwendung |
| Elektronische Scheibe + Verbindung | **Nur im Live-Modus.** Für Demo nicht erforderlich. |
| `config.ini` | Liegt neben der Anwendung; enthält Betriebsmodus und Sprache |

## 4. Tech Aim starten

1. `TechAim.exe` ausführen.
2. Der Fenstertitel lautet **Tech Aim Electronic Target Control**.

**Erwartetes Ergebnis:** Die Startseite erscheint.

**Häufiger Fehler:** Eine alte `Seta.exe` starten. Das Produkt heißt jetzt
`TechAim.exe`. Es kann immer nur **eine** Instanz laufen.

## 5. Sprache wählen

1. **Einstellungen (Settings)** öffnen.
2. Unter **SPRACHE (LANGUAGE)** **English** oder **Deutsch (Beta)** wählen.

**Erwartetes Ergebnis:** Die Oberfläche wechselt die Sprache. Erscheint der
Hinweis **Neustart erforderlich (Restart required)**, starten Sie neu.

## 6. Live- und Demo-Modus

| Modus | Schussquelle | Verwendung |
|---|---|---|
| **Live-Ziel (Live target)** | Physische elektronische Scheibe | Reales Schießen |
| **Demo / Simulation** | Softwareseitig erzeugte Schüsse | Einarbeitung, Vorführung, Test |

Tech Aim erzwingt den Modus: Im Live-Modus werden simulierte Eingaben
abgelehnt, im Demo-Modus Eingaben der physischen Scheibe. So kann eine
Vorführung nicht mit einem echten Ergebnis verwechselt werden.

Der Moduswechsel erfolgt über **Jetzt neu starten (Restart Now)** oder
**Später neu starten (Restart Later)**.

## 7. Scheibe verbinden oder prüfen

**Nur im Live-Modus.** Vor dem Start prüfen, ob die Verbindungsanzeige die
Scheibe als verbunden ausweist.

*[HUMAN VISUAL CHECK REQUIRED / PHYSICAL TARGET DEPENDENT]*

## 8. Sportler auswählen

Vor dem Start auf der Startseite auswählen oder eingeben. Der Name wird in den
Sitzungsdatensatz geschrieben und auf jedem Bericht gedruckt.

## 9. Disziplin wählen

- 10 m Luftgewehr · 10 m Luftpistole · 50 m Gewehr liegend ·
  50 m Gewehr Dreistellungskampf

**Wichtig:** Die Disziplin steuert das weitere Angebot. **Positionswechsel
(Position Transition)** erscheint **nur** bei 50 m Gewehr
Dreistellungskampf.

## 10. Wettkampf oder Trainingsprogramm wählen

| Programm | Verfügbar für |
|---|---|
| Technikblöcke (Technical Blocks) | unterstützte Disziplinen |
| Ansage & Diagnose (Call & Diagnose) | unterstützte Disziplinen |
| Positionswechsel (Position Transition) | **nur 50 m Dreistellungskampf** |

Die Trefferbild-Analyse (**Group Pattern**) ist **kein** eigenes Programm,
sondern erscheint innerhalb der Programme als **GROUP PATTERN INSIGHTS**.

## 11.–12. Sitzung starten und Bildschirm verstehen

| Bereich | Inhalt |
|---|---|
| Mitte | Scheibenbild und Schüsse |
| Rechte Leiste | Status der Sitzung und Hauptaktion |
| Kopfzeile | Sportler, Disziplin, Verbindung |

**Wettkampf-Anzeigen sind im Trainingslabor bewusst ausgeblendet.** Eine
Wettkampf-Countdown-Uhr oder der rote Zähler `000` dürfen **nicht** erscheinen.

## 13. Probeschüsse und gewertete Schüsse

- **Probeschüsse (Sighters)** werden **niemals** in gewertete Ergebnisse oder
  Trefferbild-Kennzahlen einbezogen.
- **Gewertete Schüsse (Counted shots)** werden erfasst und ausgewertet.

Der Fortschritt erscheint als **Shot 0 of N** und zählt nur gewertete Schüsse.

## 14.–15. Sitzung beenden und Ergebnisse ansehen

Die Sitzung vollständig durchlaufen lassen oder über die Programmaktion
beenden (z. B. **End Training**). Die Zusammenfassung erscheint in der
Hauptansicht.

> **Tech Aim berichtet gemessene Muster und Zeiten.
> Die technische Ursache wird dadurch nicht bewiesen.**

## 16. PDF exportieren

**EXPORT PDF** wählen und das Ziel bestätigen.

**Erwartetes Ergebnis:** Ein PDF im Tech-Aim-Layout wird erstellt.

*Hinweis: Die deutsche PDF-Ausgabe (Umlaute, Umbrüche) ist noch **nicht**
visuell geprüft.*

## 17. Korrekt zur Startseite zurückkehren

**Home** in der Zusammenfassung wählen. Die Sitzung wird sauber geschlossen;
beim erneuten Öffnen darf **kein** alter Sitzungszustand erscheinen.

## 18. Unterbrochene Sitzung wiederherstellen

Tech Aim zeichnet den Fortschritt laufend auf, sodass eine unterbrochene
Sitzung normalerweise sicher fortgesetzt werden kann. Nach einem Absturz
erscheint ein Wiederherstellungsdialog: **Resume Match** / **Resume Training**
oder **Discard**.

Eine **sauber abgeschlossene** Sitzung darf **niemals** als unvollständig
angeboten werden.

## 19. Fünf häufige Probleme

| Problem | Zuerst prüfen |
|---|---|
| Kein Schuss empfangen | Live-Modus? Scheibe verbunden? Phase nimmt Schüsse an? |
| Schuss wurde ignoriert | Schüsse während **POSITION SETUP** werden bewusst ignoriert. |
| Probeschüsse in Ergebnissen | Darf nicht sein — bitte melden. |
| PDF-Export schlägt fehl | Ziel beschreibbar? Datei bereits geöffnet? |
| Fehlende deutsche Texte | Nicht übersetzte Texte bleiben absichtlich englisch. |

## 20. Support kontaktieren

Senden Sie: Version und Build (**Einstellungen ▸ ABOUT / BUILD**),
Betriebsmodus, Disziplin und Programm, Beschreibung von Vorgehen/Erwartung/
Ergebnis sowie ggf. das exportierte PDF. **Keine nicht benötigten
personenbezogenen Daten senden.**

---

## Checkliste für die erste Sitzung

```
□ Anwendung gestartet (TechAim.exe)
□ Richtige Sprache gewählt
□ Richtiger Betriebsmodus gewählt (Live / Demo)
□ Richtige Disziplin gewählt
□ Scheibenverbindung geprüft        (nur Live)
□ Sportler ausgewählt
□ Wettkampf oder Trainingsprogramm gewählt
□ Probeschüsse abgeschlossen
□ Gewertete Sitzung abgeschlossen
□ PDF exportiert
□ Sitzung sauber über Home beendet
```

---

**Vollständige Referenz:** `TechAim_Operator_Manual_EN.md` (Englisch, Master)
· `TechAim_Operator_Manual_DE.md` (Deutsch, Beta) ·
`TechAim_German_Translation_Status.md`
