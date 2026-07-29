# Tech Aim Pre-Beta Feature Scope (P0 Phase G)

**Release:** Tech Aim Electronic Target Control 0.9.0
**Channel:** Pre-Beta Validation
**Publisher:** JAC SHOOTING SOLUTIONS (PTY) LTD
**Executable:** `TechAim.exe`

Features are **frozen** at this list. Anything not named here is out of scope
for the pre-beta package.

## In scope

### Competition / practice
- Supported qualification workflows (10m Air Rifle, 10m Air Pistol,
  50m Rifle Prone, 50m Rifle 3 Positions)
- 10m Air Rifle Final
- 10m Air Pistol Final
- 50m Rifle 3P Final (single-athlete training mode)
- Open Practice

### Training Lab
- Technical Blocks
- Call & Diagnose
- Group Pattern Coach analysis
- Position Transition

### Foundation
- Live / Demo operating-mode selector
- Append-only session journaling
- Replay
- Crash recovery
- Range Incident (EST malfunction) workflow
- Reports and PDF export
- English / German language selection

## Explicitly excluded

| Excluded | Reason |
|---|---|
| Wind Map | not started |
| First Shot & Re-entry | not started |
| Consistency Chain | not started |
| SCATT integration | not started |
| Shadow Shooting | not started |
| Cloud services | not started |
| Android | separate platform effort |
| Range Management coordination | separate application (see RMS notes) |
| SETA blue OEM theme | reserved flavour; assets do not exist |
| Additional disciplines (25m Pistol events) | scope decision outstanding |

## Known limitations carried into the pre-beta

These are **known and accepted**, not defects to be filed:

1. **German is a partial beta translation.** 100 of 583 extractable strings are
   translated; the rest render in English. Additionally, many Training Lab
   strings are not yet wrapped for translation and so cannot be translated at
   all. See `docs/german-translation-review.md`.
2. **German layout and German PDF output are not visually verified.**
3. **The EULA artwork still shows a SETA-era agreement** naming a different
   entity. This is a legal blocker for any public beta and needs a replacement
   document from the publisher. See `docs/product-identity-audit.md`.
4. **No application icon.** The build has no `.ico`; Windows shows the default.
5. **25m Pistol disciplines are unimplemented.**
6. **50m Rifle `radOf10Ring = 5.2`** awaits rulebook confirmation or physical
   calibration.
7. **Licence-expiry check is disabled.**
8. Pre-existing QML binding warnings in the Coach and Incident views.

## Not claimed

The build makes no claim to be a final production release, certified software,
ISSF-certified or SETA-certified. The Windows version resource is marked
`VS_FF_PRERELEASE` accordingly.
