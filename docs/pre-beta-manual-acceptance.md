# Tech Aim Pre-Beta Manual Acceptance (P0 Phase I)

**Build:** Tech Aim Electronic Target Control 0.9.0 · Pre-Beta Validation
**Executable:** `release/TechAim.exe`

Two kinds of entry appear below:

- **[VERIFIED]** — checked automatically in this environment; the evidence is
  named. No human repeat needed unless something changed.
- **[HUMAN]** — requires a person at the screen or at the range. **Not
  performed.** Interactive GUI control and physical target hardware are not
  available here, so no visual acceptance is claimed for these.

---

## 1. Identity

| # | Check | Status |
|---|---|---|
| 1.1 | Executable is `TechAim.exe` | **[VERIFIED]** `release/` contains only `TechAim.exe` |
| 1.2 | Stale `Seta.exe` removed from output | **[VERIFIED]** post-link clean; directory listing confirms |
| 1.3 | Windows version resource correct | **[VERIFIED]** `Get-Item .VersionInfo` — company, description, InternalName `TechAim`, OriginalFilename `TechAim.exe`, product, 0.9.0.0, "Windows Pre-Beta Validation Build" |
| 1.4 | Startup log identity | **[VERIFIED]** `Tech Aim 0.9.0 Release build · commit <sha> · Pre-Beta Validation · flavour TECH_AIM` |
| 1.5 | Session data root unchanged | **[VERIFIED]** `%LOCALAPPDATA%\TechAim\TechAim` |
| 1.6 | No user-facing Seta/Seeds/Tachus product identity | **[VERIFIED]** identity assertions in the 3P finals harness |
| 1.7 | SETA hardware/supplier references retained | **[VERIFIED]** audit doc, class C — lane server + shot-data file share untouched |
| 1.8 | Window caption and taskbar read "Tech Aim Electronic Target Control" | **[HUMAN]** — binding verified to load without error, but the rendered caption was not seen |
| 1.9 | Settings ▸ About shows product, version, channel, publisher | **[HUMAN]** |
| 1.10 | Application icon | **[HUMAN]** — **known gap: no `.ico` exists**, Windows shows the default |

## 2. English flows

All **[HUMAN]**. Confirm each screen opens, is readable, and no control is
clipped or unreachable.

- [ ] Homepage / athlete selection / discipline selection
- [ ] Open Practice
- [ ] Qualification (10m AR, 10m AP, 50m Prone, 50m 3P)
- [ ] 10m Air Rifle Final, 10m Air Pistol Final, 50m 3P Final
- [ ] Training Lab catalogue
- [ ] Technical Blocks — block, Block Review, summary
- [ ] Call & Diagnose — call, reveal, summary
- [ ] Position Transition — K→P→S, review, summary
- [ ] Reports and PDF export
- [ ] Recovery dialog after a forced kill mid-session
- [ ] Range Incident workflow
- [ ] Settings
- [ ] Home from each flow leaves no stale state

## 3. German

| # | Check | Status |
|---|---|---|
| 3.1 | German catalogue loads | **[VERIFIED]** `UI language: de-DE (beta translation)`, no load diagnostic |
| 3.2 | Language persists across restart | **[VERIFIED]** `[App_Settings] ui_language` read back on next launch |
| 3.3 | Unknown language code falls back to English + logs | **[VERIFIED]** code path + startup log |
| 3.4 | Untranslated strings render English, never blank/key | **[VERIFIED]** by design — Qt source-language fallback; 483 strings currently rely on it |
| 3.5 | Language does not change brand, theme, executable or app_mode | **[VERIFIED]** LanguageService has no access to ProductIdentity or mode state |
| 3.6 | Selector shows "Deutsch (Beta)" and the beta note | **[HUMAN]** |
| 3.7 | Live switch retranslates without a restart | **[HUMAN]** |
| 3.8 | Umlauts and ß render on screen | **[HUMAN]** |
| 3.9 | **Mixed-language screens are expected** — confirm acceptable for beta | **[HUMAN]** |

### German layout — all [HUMAN]

Check at **1536×960, 1366×768, 1280×720, 1100×700, maximised, restored**:

- [ ] No clipped text, no overlapping buttons, no horizontal overflow
- [ ] Touch targets stay full size
- [ ] Wrapping looks intentional; scroll/flick still available
- [ ] English layout unchanged
- [ ] Watch `Streukreisdurchmesser`, `Positionswechsel`, `Kontrollschüsse`,
      `PROBESCHIESSEN` — the longest compounds, most likely to overflow

## 4. Training Lab regression

| # | Check | Status |
|---|---|---|
| 4.1 | Technical Blocks / Call & Diagnose / Group Pattern / Position Transition logic | **[VERIFIED]** training harness 567/0 |
| 4.2 | Position Transition shot cadence is the shot-to-shot interval | **[VERIFIED]** cadence tests |
| 4.3 | No stray match timer in training modes | **[VERIFIED]** gate change; **[HUMAN]** visual confirm |
| 4.4 | No ghost "000" counter | **[VERIFIED]** gate change; **[HUMAN]** visual confirm |
| 4.5 | No stale state after Home | **[VERIFIED]** clean-Home lifecycle test |
| 4.6 | **Technical Blocks "Avg shot time" now reads differently** (it was averaging absolute timestamps) — sanity-check against a real session | **[HUMAN]** |

## 5. Reports and PDFs — all [HUMAN]

- [ ] Enlarged Tech Aim logo correct on every report type
- [ ] Software attribution reads "Tech Aim 0.9.0" (was "Seta 4.0")
- [ ] PDF metadata author/creator is Tech Aim
- [ ] German PDFs: umlauts, no missing glyphs, no overflow, page numbers visible
- [ ] Training disclaimer present on training reports and **absent** from
      competition reports
- [ ] Athlete notes unchanged; Demo/Live status accurate
- [ ] No development paths leak into output
- [ ] Generated filenames contain no problematic filesystem characters

## 6. Recovery — all [HUMAN]

- [ ] Kill mid-session → recovery offers the session; resume is correct
- [ ] Clean close → no candidate offered
- [ ] Completed session → never offered as unfinished
- [ ] Completed session never becomes recoverable, unfinished never becomes
      completed

## 7. Restart / mode switch

| # | Check | Status |
|---|---|---|
| 7.1 | Restart relaunches `TechAim.exe`, not a legacy name | **[VERIFIED]** by inspection — restart uses `applicationFilePath()`, so it re-launches itself by resolved path; no code references `Seta.exe` |
| 7.2 | Live → Demo and Demo → Live restart | **[HUMAN]** |
| 7.3 | Path containing spaces; installed path | **[HUMAN]** |
| 7.4 | Second instance blocked with "Tech Aim is already running" | **[HUMAN]** |
| 7.5 | A legacy `Seta.exe` and `TechAim.exe` cannot run concurrently | **[VERIFIED]** by construction — this build also holds the legacy lock; **[HUMAN]** to confirm against a real old binary |

## 8. Physical range — all [HUMAN]

Nothing below can be exercised here; all of it needs real hardware.

- [ ] Live target over Modbus RTU (COM7, 19200)
- [ ] Real shot acquisition and scoring against known impacts
- [ ] Coordinate units and y-orientation
- [ ] Full match on live hardware without dropped shots
- [ ] Recovery after a real power interruption mid-match

---

## Summary

Automated verification covers product identity, the executable rename and
version resource, storage-root stability, language loading/persistence/
fallback, and all four test harnesses. **Everything visual — English and
German layout, PDF rendering, dialogs — and everything involving physical
target hardware remains unverified and needs a human.**
