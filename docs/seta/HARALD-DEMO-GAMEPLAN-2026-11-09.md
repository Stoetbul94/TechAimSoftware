# SETA demo with Harald — game plan (Mon 2026-11-09)

Working note for the SETA-only chat. Not a release document.

---

## 0. Where SETA actually stands (gathered from the repo + memory)

| Item | State |
|---|---|
| Code | worktree `C:\Users\User\Downloads\TechAimSoftware-SETA`, branch `feature/seta-dsb-2026-integration` (head `9e3acd5`, 2026-08-31). Line: `product/seta` → this branch. |
| Latest build | **SETA 1.0.0-EVAL4** — one-file installer in `Downloads\SETA-1.0.0-EVAL4-Delivery\SETA-1.0.0-EVAL4-Setup.exe` + tester checklist |
| Status of that build | **Evaluation build.** All automated checks green (6 418, ~186 DSB-specific). **No DSB programme has been fired on a physical target yet.** I found no record of the EVAL4 physical checklist being completed. |
| Branding | SETA blue: accent `#00539E`, light `#25B0E6`, navy `#212D60`, sampled from the SETA logo. Publisher stays JAC SHOOTING SOLUTIONS (legal fact, not a skin). Teiler **shown** in SETA (hidden in Tech Aim). |
| Data | SETA keeps its own data (`%LOCALAPPDATA%\TechAim\TechAimSETA`) — never mixed with Tech Aim. |
| Language | German catalogue ~79 % translated (239 strings outstanding — recorded, not invented). |
| Selection screen | `SetaCompetitionSelector.qml`: **Rule set → Discipline → Programme** (step 3 skipped if only one). |
| Still asked of SETA | white/knockout logo, single-ink print logo (the `.ico` now exists). |

### DSB rules — what is in (Sportordnung, Stand 01.01.2026)

| Kennzahl | Programme | Shots | Scoring | Clock | Status |
|---|---|---|---|---|---|
| 1.10 | LG 10 m | 20 / 40 / 60 | Zehntel | 30 / 50 / **75** min (EST times) | working |
| **1.20** | **LG 3-Stellung 10 m** | 3×10 / 3×20 | ganze Ringe | 15 min prep, then **three separate position clocks** 25/20/30 or 35/30/40 | working — own engine (`Dsb120Controller`), the most important physical test |
| 1.40 | KK-Sportgewehr 50 m 3×20 | 60 | ganze Ringe (DM 2026) | one 105 min clock | working, **no crash-resume** |
| 1.60 | KK-Freigewehr 3×40 | 120 | ganze Ringe | one 165 min clock, changes at 40/80 | working, **no crash-resume** |
| 1.80 | KK-Liegend 50 m | 60 | Zehntel | 50 min | working |
| 2.10 | LP 10 m | 20 / 40 / 60 | ganze Ringe | 30 / 50 / 75 min | working |
| 2.20 | 50 m Pistole | 60 (30) | ganze Ringe | 30-shot time = organiser-set | working |

**Harald's original requests, answered:** 10 m 3×10 ✅, 10 m 3×20 ✅, 50 m 3×20 ✅,
50 m 3×40 ✅. **10 m 3×15 and 50 m 3×10 are not DSB programmes** — closed; could
only come back as a clearly-labelled custom/club profile.
Not built (need different target hardware): 2.16 Mehrschüssige LP, 2.17 LP
Mehrkampf, 2.18 LP Standard.

All DSB targets used (Scheibe Nr. 1, 3, 4, 7) are dimensionally identical to the
ISSF faces → **no scoring change was needed for DSB**.

---

## 1. Layman's explanation for Harald (≈ 1 minute)

### English

> **From the shot to the screen in five steps**
>
> 1. **The target measures.** When the pellet goes through, the sensors in the
>    SETA target work out exactly where it hit — as an X and Y position in
>    tenths of a millimetre from the centre. The target doesn't decide the
>    score; it only reports *where*.
> 2. **The cable carries it.** The laptop asks the target, many times a
>    second, "any new shots?" over the USB/serial cable (the industry-standard
>    Modbus language). When the shot counter goes up, it reads the new X/Y.
> 3. **The software checks it.** Before anything is shown, the shot is checked:
>    is it a real new shot, not a duplicate, not a glitch, nothing missed? Only
>    then is it *accepted* — and the target's paper/band is advanced exactly
>    once.
> 4. **The software scores it.** From the X/Y position and the official ring
>    sizes of that target (ISSF/DSB — identical for these faces), it calculates
>    the ring value, decimal or whole-ring depending on the programme, plus the
>    Teiler. The shot appears on the screen immediately.
> 5. **It's saved and reported.** Every shot is written to a protected log
>    straight away (position, value, time, sighter or match). If the power or
>    cable fails, the match can be resumed without losing or doubling a shot.
>    At the end you get the result sheet and PDF.
>
> **The rules live in the software, not in the target.** Choosing "DSB 1.20
> 3×20" tells the software the shot count, the positions, the clocks and the
> scoring mode straight from the 2026 Sportordnung.

### Deutsch (zum Vorlesen / Ausdrucken)

> **Vom Schuss zum Bildschirm in fünf Schritten**
>
> 1. **Die Scheibe misst.** Die SETA-Scheibe ermittelt, wo der Schuss
>    getroffen hat — als X/Y-Position in Zehntelmillimetern vom Zentrum. Die
>    Scheibe bewertet nicht, sie meldet nur *wo*.
> 2. **Das Kabel überträgt.** Der Laptop fragt die Scheibe über USB/seriell
>    (Standardprotokoll Modbus) mehrmals pro Sekunde nach neuen Schüssen. Steigt
>    der Schusszähler, werden die neuen Koordinaten gelesen.
> 3. **Die Software prüft.** Jeder Schuss wird geprüft — wirklich neu, nicht
>    doppelt, kein Fehlsignal, keiner verloren. Erst dann wird er übernommen,
>    und das Papier-/Gummiband wird genau einmal weitertransportiert.
> 4. **Die Software wertet.** Aus der Position und den offiziellen Ringmaßen
>    der Scheibe (ISSF und DSB sind hier identisch) berechnet sie den Ringwert
>    — Zehntel oder ganze Ringe je nach Wettbewerb — sowie den Teiler.
> 5. **Gespeichert und ausgewertet.** Jeder Schuss wird sofort gesichert.
>    Bei Strom- oder Kabelausfall kann der Wettkampf ohne verlorene oder
>    doppelte Schüsse fortgesetzt werden. Am Ende: Ergebnisliste und PDF.
>
> **Die Regeln stecken in der Software.** Wer „DSB 1.20 3×20" wählt, bekommt
> Schusszahl, Stellungen, Zeiten und Wertungsart direkt aus der Sportordnung
> 2026.

*(Have a native speaker glance at the German before printing.)*

---

## 2. The "athlete edition" — SETA-branded, idiot-proof

**Principle:** same engine, same rules, same scoring — **only fewer buttons.**
Brand stays a build-time fact (blue SETA package); "simple" is a *mode*, not a
second product, so nothing about scores can drift between the two.

### What the athlete sees

```
┌──────────────────────────────────────┐
│ [SETA logo]          ● Target ready  │   ← one status light, green/red
│                                      │
│   Name: [ Max Mustermann        ]    │
│                                      │
│   [ LG 10 m ] [ LG 3-Stellung ]      │   ← big tiles, only the club's
│   [ LP 10 m ] [ KK Liegend    ]      │     enabled programmes
│                                      │
│          [   START   ]               │   ← one button
└──────────────────────────────────────┘
   During match: target + score + shots left + clock. Nothing else.
   After match:  total · [Print] · [PDF] · [New match]
```

### Idiot-proofing list (ranked by "how often will this go wrong")

1. **Auto-connect the target** — find the COM port by itself; the athlete never
   types a port. (Known past failure: stray space in the port field.)
2. **One status light** with plain words: *Target ready / Target not connected —
   check the cable.*
3. **Programme tiles only for what the club enables** (set once by the club
   operator). No rule-set/discipline tree for athletes.
4. **Operator lock (PIN)** around settings, COM port, Training Lab, coach
   analytics, config — hidden from athletes, not deleted.
5. **No dead ends** — every screen has one obvious next action; dangerous
   actions (end match early, discard) need a confirm.
6. **Automatic save + resume** offered with one button after a crash.
7. **German as default** for the SETA athlete build, large type, touch-sized
   buttons.
8. **Live/demo can't be confused** — athlete build always Live; demo only via
   operator.

### What must NOT change (hard rules from the repo)

- No change to scoring, ring geometry, DSB/ISSF rules, timing, acquisition,
  paper feed, recovery. Simple mode only **hides and reorders UI**.
- No `if (SETA) …` branches in screens — the mode is read from one setting /
  build option; screens read it, not the brand.
- Must follow the UI workflow in `CLAUDE.md`: new UI decision entry (simple
  mode supersedes nothing silently), defect register, acceptance checklist,
  screenshots as evidence.

### Suggested build order (after tomorrow, once Harald agrees)

| Step | What | Size |
|---|---|---|
| A | UI decision entry + acceptance checklist for "Athlete mode" | small |
| B | Operator PIN + hide Training Lab / coach / settings | small |
| C | Club programme whitelist → big tiles home screen | medium |
| D | COM-port auto-detect + single status light | medium (needs hardware test) |
| E | Simplified match + end-of-match screens | medium |
| F | Physical test on a SETA target (with the EVAL4 checklist items C, E, H) | range day |

---

## 3. Tomorrow — running order

**Before leaving (tonight / morning)**
- [ ] Install `SETA-1.0.0-EVAL4-Setup.exe` on the demo laptop; launch once; accept EULA.
- [ ] `config.ini` next to the exe: `app_mode=Live`, `is_single_decimal=1` (otherwise shots plot 10× too close to centre). Restart after editing.
- [ ] If no target on site: decide demo vs live, and say so to Harald.
- [ ] Print the German explanation (section 1) and the DSB table (section 0).
- [ ] Charge laptop; USB cable; spare cable.

**In the meeting (~45 min)**
1. **5 min — the explanation** (section 1). Shot → cable → check → score → save.
2. **10 min — DSB selection**: Rule set DSB → Luftgewehr → 1.20 3×20. Show the
   prep clock, the gate before kneeling, Teiler, German labels.
3. **10 min — fire / simulate a few shots**, show report + PDF.
4. **5 min — be straight about status**: EVAL build, DSB not yet fired on a
   physical target, 1.40/1.60 cannot resume after a crash, German 79 %.
5. **10 min — the athlete edition**: show the sketch in section 2, ask the
   questions below.
6. **5 min — agree next step**: who runs the EVAL4 physical checklist, when.

**Questions to get answered by Harald**
- [ ] Who uses it day-to-day — athlete alone, or club operator + athlete?
- [ ] Which programmes should a typical club see? (whitelist for the tiles)
- [ ] Touchscreen or mouse? Screen size?
- [ ] German only, or German + English?
- [ ] Print on site, PDF, or both? Teiler on the athlete screen or report only?
- [ ] Does he still want 10 m 3×15 / 50 m 3×10 as **club** (non-DSB) profiles?
- [ ] White and single-ink SETA logo files.
- [ ] Does the result need to go to a central PC (Zentralrechner, SpO 0.4.3.2)? → links to the RMS work.
- [ ] Who runs the EVAL4 physical checklist, and when?

**Don't promise tomorrow:** official-competition use, 2.16/2.17/2.18, a
delivery date for the athlete edition before the physical test is done.
