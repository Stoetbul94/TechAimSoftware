# SETA product line — identity boundary and competition selection

Branch `product/seta`. This document records the decisions a later phase
must not silently reverse.

---

## 1. Brand / product name is NOT the legal publisher

Three separate facts, deliberately kept in three separate fields:

| Fact | Field | Tech Aim | SETA |
|---|---|---|---|
| What the product is called | `displayName` / `fullProductName` | Tech Aim | **SETA** |
| Who publishes the software | `legalPublisher` | JAC SHOOTING SOLUTIONS (PTY) LTD | **JAC SHOOTING SOLUTIONS (PTY) LTD — unchanged** |
| Which colour/asset package | `BuildFlavour` | `TECH_AIM` (red) | **`SETA_OEM` (blue)** |

Re-branding a product must never silently re-attribute who published the
software. The publisher is a legal fact, not a skin. It changes only when an
authoritative business or contractual requirement is supplied — none has been.

`BuildFlavour` is the ONE authority for which brand a binary is — it selects the
`BrandPackage` — and `BRAND_SETA` is the compile-time switch that selects it.
`SetaOem` was reserved and unbuildable while no SETA palette existed; §4 supplies
one, sampled from the approved logo, so it is now a real package. The startup log
prints `brand SETA · flavour SETA_OEM`.

### Brand assets

`images/logo/seta.png` already existed in the repository, is already in
`images.qrc` and already brands the printed report. **No logo was invented.**

SETA supplied one mark. There is no white-on-dark and no single-ink variant, so
`BrandPackage::missingAssets()` reports both as outstanding rather than falling
back to the Tech Aim logo — putting another company's mark in a SETA header or
on a SETA report would be worse than an imperfect one.

**SETA BRAND ASSETS REQUESTED:** a white/knockout variant for dark surfaces, a
single-ink variant for print, and a Windows `.ico`.

---

## 2. SETA user data is separate from Tech Aim user data

### The audit

Qt resolves everything mutable from the organisation/application pair set in
`main.cpp`:

```
QStandardPaths::AppLocalDataLocation
  = %LOCALAPPDATA%\<organisationName>\<applicationName>
QSettings()  (default ctor)
  = HKCU\Software\<organisationName>\<applicationName>
```

Before this change both came from `organisationName` alone, so **an installed
Tech Aim build and an installed SETA build would have read and written the same
locations** for all of:

| Data | Where |
|---|---|
| Sessions (current / archive / corrupt) | `…\Sessions\*` — `StoragePaths` |
| Recovery + replay state | same root |
| Reports, exports, backups, derived indexes | `…\Reports`, `…\Exports`, … |
| Logs, support bundles | `…\Logs`, `…\SupportBundles` |
| Athlete name / last session state | session store under the same root |
| Remembered target fingerprints | `QSettings` group `TargetDevice` |
| EULA acceptance, last-used folder | legacy `QSettings("Seta"/"Tachus", "shootingApp")` |
| `config.ini` | next to the executable — already separate per install |

**Answer: YES, they would have shared.** That is undesirable for a product
line, so SETA now takes its own namespace.

### The decision

| | Tech Aim | SETA |
|---|---|---|
| `organisationName` (vendor) | `TechAim` | `TechAim` — **shared, deliberately** |
| `applicationStorageName` (product) | `TechAim` | **`TechAimSETA`** |
| Data root | `%LOCALAPPDATA%\TechAim\TechAim` | `%LOCALAPPDATA%\TechAim\TechAimSETA` |
| Registry scope | `…\TechAim\TechAim` | `…\TechAim\TechAimSETA` |
| `brandSettingsScope` (legacy keys) | *(empty — historical scope kept)* | `TechAimSETA` |
| `applicationId` | `za.co.techaim.electronic-target-control` | `za.co.techaim.seta.electronic-target-control` |
| `executableBaseName` | `TechAim` | `TechAim` — **shared, deliberately** |

Two things are shared on purpose:

- **The vendor folder.** The vendor really is the same. Everything mutable
  *inside* it is not.
- **`executableBaseName`**, because it also names the single-instance lock.
  One machine drives one target, so a SETA build and a Tech Aim build must
  still refuse to run at the same time.

`applicationStorageName` defaults to `organisationName`, so Tech Aim's existing
data root does not move by one byte.

### No silent migration

There is **no** automatic copy of Tech Aim data into the SETA namespace. A
fresh SETA install starts clean. Session and journal **file formats stay
compatible**, so a deliberate import feature remains possible later — but it
would be a feature, with its own approval, not a side effect of branding.

One visible consequence, and it is correct: the first SETA launch shows the
EULA again, because acceptance is recorded in the SETA scope and that scope is
new. Nothing else is re-prompted.

---

## 3. Hierarchical competition selection

`SetaCompetitionSelector.qml` is the SETA production selection path:
**RULE SET → DISCIPLINE → PROGRAMME**, with a breadcrumb and Back at every
level. Step 3 is skipped when a discipline has exactly one programme, so the
ISSF path is no longer than it was.

### One authoritative result

The only committed value is `programmeId`. `CompetitionCatalogue.runtimeConfig()`
turns it into the **existing** `gameRange` / `gameMode` / `gameEvent` state.
There is no second set of match-configuration rules: the event index it returns
is the same index the legacy controls already set.

Nothing is written while browsing. Rule set → discipline → back → programme
touches no shooting configuration; only a commit does.

### Equivalence (SETA-INT-001)

All 48 catalogue programmes are compared. For each one, the new path must
produce the same range, weapon, event index, shot count, target standard and
paper variant as the legacy path — and, decisively, the legacy index
arithmetic applied to the new result must land on **the same ListModel row**,
because that row is what sets the shot count and the displayed programme.

### Paper mode is part of the question

The catalogue holds both the standard and the 15-shot-paper variant of every
preset; the running application uses exactly one. Every level of the hierarchy
is filtered by `APPSETTINGS.getIs15Shoot()`, so the same preset is never
offered twice and nothing is offered that cannot be run.

A real consequence: **in 15-shot paper mode there is no 60-shot entry**, so no
ISSF course exists and the ISSF rule set correctly disappears.

### Legacy path preserved

The weapon / distance / event controls are **not deleted**. They are gated on
`!setaSelection` and remain the rollback and reference path until this has been
through an integrated approval; a later cleanup removes them. Only one selector
is ever on screen.

### Not owned by the hierarchy

3 Positions and the two Finals have **no catalogue entry** and stay on their
existing controls. Inventing entries for them would put programmes in the
hierarchy that the qualification engine cannot run.

### DSB

DSB is not offered, and a test asserts it. The navigation begins showing it
automatically the moment catalogue entries with `rulesetId: "dsb"` exist. No
empty DSB section is displayed. Nothing about German programmes is implemented:
even Rule 1.20 Luftgewehr 3-Stellung has only a reported name, number and
20/20/20 — preparation time, sighting policy, position-change rules, match
timing, scoring mode and applicable classes are unknown.

### Language

`programmeId` is identical under a German translator, and the selector compares
no translated string (QML-LANG-001 preserved).

The selector's strings are now in the shipped German catalogue — see §6.

---

## 4. SETA blue theme

### The palette, and where it came from

`images/logo/seta.png` contains exactly three opaque colours and no others:

| Colour | Pixels | Share | Role in the artwork |
|---|---|---|---|
| `#25B0E6` | 13,307 | 73.28% | wordmark / swoosh |
| `#212D60` | 3,786 | 20.85% | deep navy |
| `#00539E` | 1,066 | 5.87% | saturated brand blue |

| Token | SETA | Tech Aim | Derivation |
|---|---|---|---|
| `accentPrimary` | **#00539E** | #A80038 | logo colour |
| `accentHover` | **#25B0E6** | #C40046 | logo colour (the lighter state) |
| `accentPressed` | **#003A6E** | #80032A | accentPrimary × 0.70 |
| `accentSubtle` | **#0F2740** | #2D0A18 | 28% accentPrimary over `surfacePrimary` |
| `accentBright` | **#25B0E6** | #E8003D | the live-UI / HUD tone |
| `focusOutline` | **#25B0E6** | #C40046 | = accentHover |
| `textOnAccent` | #FFFFFF | #FFFFFF | |
| `brandLogoSecondary` | **#212D60** | #BF1919 | logo-intrinsic, NOT an accent |

**`accentPrimary` is not the most numerous colour, on purpose.** An accent is a
fill that carries white text, and `#25B0E6` cannot: white on it is **2.49:1**,
which fails at any size. White on `#00539E` is **7.69:1** — within 0.02 of Tech
Aim's own **7.71:1** on `#A80038`. The two products' accents are therefore
functionally interchangeable, which is why no component needs to know which
brand it is drawing.

`#25B0E6` becomes the lighter interaction state and the focus ring, mirroring
how `#C40046` relates to `#A80038`. It reads **7.81:1** on the darkest canvas
where Tech Aim's ring manages 3.18:1 — SETA's focus visibility is better, not
merely different. Nothing was invented: every hue is from the artwork, and the
two derived values are stated with their derivation.

### Where the palette lives

```
BrandPackage (C++)      the values, one package per flavour   ← authority
   ↓ PRODUCT.accent*    ProductIdentityBridge
DesignTokens.qml        the SEMANTIC layer screens consume
   ↓ theme.tokens.*
89 call sites           migrated off hard-coded brand literals
```

**BuildFlavour is now the single authority.** `BRAND_SETA` is the compile-time
switch that selects it; `TECHAIM_FLAVOUR_SETA_OEM` is still honoured so nothing
that sets it changes meaning. `SetaOem` is no longer a reserved empty stub — it
carries the blue palette — and `isFlavourBuildable()` now means "producible by
THIS build" rather than "does this brand exist". Buildability is deliberately
not tied to `isComplete()`: Tech Aim ships with an outstanding `.ico`.

### What did NOT become blue

Semantic and scoring colours are not brand decoration, and each was classified
individually rather than swept:

| Kept | Why |
|---|---|
| Demo/Live badge, Demo mode dot | mistaking Demo for Live is a result-integrity risk |
| NO TARGET / connection fault | a fault must read as a fault in every product |
| `errorText` / `successText` / `warningText` | semantic tokens; no package may set them |
| score-band colours (`≤7`), shot-score colours | scoring presentation |
| Aborted / Critical / fail states in reports | status, not brand |
| target display colour swatches (Settings) | they show the TARGET's colours |
| the target face itself | ring geometry, numerals and bull are unchanged |
| position-transition indicator pair | a two-state indicator already using blue |

One site was **re-classified in the other direction**: the left-pane event badge
(`LeftPanel.qml`) had been skipped as a "no programme" alert. It is the active
event badge — brand highlight — so it takes the accent.

Discipline art (`DisciplineArt.qml`) is decorative and took the accent. **No
scoring-authoritative target face was touched**, and the approved firearm
artwork was not redrawn.

## 5. Layout at 1366×724

**The previously reported right-panel clipping was a measurement error.** The
screenshots behind it were taken by a DPI-unaware process on a 1920×1200 display
at 125% scaling, so Windows returned a virtualised 1536×912 view of a 1920-wide
window and the right panel appeared to run off the edge. Captured DPI-aware,
maximised is 1920×1140 — exactly the work area — with correct margins.

The landing screen is anchor-based and proportional (`leftPanel` = 44% of the
content area, `rightPanel` fills the rest), so it was already responsive. At a
true 1366×724 window both panels fit horizontally with margins and both scroll
vertically.

**A real defect WAS found at that size, in the new selector:** its step content
sat in a clipped, unscrollable Item, so at 1366×724 the second row of ISSF
disciplines could not be reached — two of the four were effectively missing. An
option the operator cannot reach is the same as an option that does not exist,
which is precisely what a catalogue-driven selector must never do. Fixed by:

- every step in a `Flickable` with a scrollbar,
- responsive card widths (two per row while the panel allows it, one when not),
- the browsing block taking the full panel height,
- 10 px reserved so the scrollbar never sits on a card.

## 6. German

The shipped catalogue is `translations/techaim_de_DE.qm`
(`techaim_translations.qrc`); `german.qm` is a Tachus-era stub that is not
built into the product, and the earlier "German is untranslated" observation
was made against that stub.

`SetaCompetitionSelector.qml` and `CompetitionCatalogue.qml` were added to the
`lupdate` source list. Catalogue labels arrive as **data**
(`qsTr(modelData.labelKey)`), which `lupdate` cannot see, so the English source
text is listed in `translatableLabels` — an extraction aid that nothing reads
and nothing compares.

| Source | German |
|---|---|
| Rule set / Discipline / Programme | Regelwerk / Disziplin / Programm |
| SELECT RULE SET / DISCIPLINE / PROGRAMME | REGELWERK / DISZIPLIN / PROGRAMM WÄHLEN |
| < Back | < Zurück |
| Official competition rules | Offizielles Wettkampfreglement |
| Practice - no rule authority | Training – keine Regelautorität |
| Official course / Preset | Offizieller Wettkampf / Voreinstellung |
| Practice presets | Trainings-Voreinstellungen |
| 10M AIR RIFLE / PISTOL | 10 M LUFTGEWEHR / LUFTPISTOLE |
| 50 Meter RIFLE / Free PISTOL | 50 Meter GEWEHR / FREIE PISTOLE |
| UN-LIMITED | UNBEGRENZT |

`SETA-LANG-002` extends QML-LANG-001 beyond `programmeId`: the rule set,
discipline, target standard and shot count are compared **identically** in
German across the whole hierarchy. It also asserts the catalogue really
contains these translations — otherwise "German passes" would only mean German
is still English.

The QML-LANG-001 negative control moved with it. It needs a string the shipped
catalogue actually translates; `CenterPane`'s "PISTOL" was that string until the
QML-LANG-001 fix removed the comparison and `lupdate` dropped the source, so the
control now uses the selector's discipline label — which is translated, and is
exactly the text a naive hierarchy would have branched on.

## Outstanding for SETA

- White/knockout logo, single-ink logo, Windows `.ico` (reported by
  `BrandPackage::missingAssets()`, never invented).
- Legal publisher, if SETA is to be named as publisher rather than JAC.
- DSB Sportordnung rule detail before any German programme is added.

## Windows icon

The SETA build ships its own executable icon. `images/logo/seta.ico` is built
from the approved `images/logo/seta.png` by `tools/icon/make_seta_ico.py`,
which crops the emblem the logo already contains - the crosshair mark, found
by the artwork's own alpha gutter rather than by hand-typed coordinates - and
renders it at 16, 24, 32, 48, 64, 128 and 256 px. No symbol was invented and
the wide logo is never squashed into a square.

Two different icons are involved, and both are product-scoped:

| What draws it | Where it comes from |
|---|---|
| Explorer, Properties, shortcuts | the PE resource — `TechAim.rc`, selected by `BRAND_SETA` via `RC_DEFINES` |
| Taskbar, Alt+Tab | the WINDOW icon — `QApplication::setWindowIcon(product.appIconPath)` |

The window icon is separate because a window that carries none gets the
Windows placeholder however well branded the binary is; that is exactly what
the switcher showed before this was set. `techaim.ico` is untouched and never
reaches a SETA build, and `seta.ico` never reaches a Tech Aim build -
`tests/release/check_windows_icon.py` proves both directions by reading the
RT_ICON resources out of the compiled binary.

The executable is still `TechAim.exe`. A product-specific name would be
clearer on a machine with both products installed, but the name is depended on
by the qmake TARGET, the single-instance relaunch, the documentation-capture
script, the deployment check, a manuals gate and 34 manual documents, so it is
its own task rather than a side effect of an icon change.
