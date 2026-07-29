# Tech Aim — current design audit (UI-1)

**Phase:** UI-1 — Design System and Brand Package foundation
**Audited commit:** `8022033` (branch `feature/training-lab`)
**Scope:** every `.qml` in the application root (66 files), `Theme.qml`,
`src/app/ProductIdentity.*`, `TechAim.rc`, `images/logo/`.
**Method:** mechanical extraction (grep over the QML sources) plus pixel
sampling of the approved logo asset. Counts below are measured, not estimated.

---

## 1. Headline numbers

| Measure | Value |
|---|---:|
| QML files in the application root | 66 |
| Distinct hard-coded hex colour literals | **240** |
| Total hex colour occurrences | **972** |
| Distinct `font.pixelSize` values | **22** |
| Distinct `radius` values | **12** |
| Distinct `border.width` values | 3 |
| Files referencing the shared `Theme.qml` | see §4 |

240 distinct colours across 66 files is the core finding. There is a shared
`Theme.qml`, but it is used for roughly a sixth of colour references; the rest
are re-declared per file. A brand change today means editing 972 literals.

---

## 2. THE PALETTE CONFLICT — three shipping reds

This is the most consequential finding and it **requires Arnold's decision**.

Three different "brand reds" are live in the product simultaneously:

| Hex | Occurrences | Where it lives | Introduced by |
|---|---:|---|---|
| `#a80038` | 48 | `Theme.qml` (`brandPrimary`), the whole **report system** (`ReportHeader`, `ReportFooter`, `MetricCard`, `SectionTitle`, `Report3P`, `Report3PSeries`, `SummaryReportView`, `MatchReportInfo`, `CoachPrintView`, `CoachDashboardView`, `CoachReportWindow`), `FloatingWindow`, `TechAimDialog`, `SettingsPage`, finals report views | the shared Theme |
| `#e8003d` | 28 | the **live shooting UI**: `CenterPane`, `LeftPanel`, `RightPanel`, `ShootingPage`, `Finals10m*`, `TrainingHud`, `PositionTransitionHud`, `RecoveryDialog`, `main.qml` | in-session screens |
| `#C40046` | 10 | **Training Lab + the Version B homepage**: `LoginPage`, `TrainingHud`, `TrainingRightPanel`, `TrainingReportView`, `CallDiagnose*`, `PositionTransition*` | most recent work |

Plus two strays:

- `#c40046` (lowercase) in `SettingsPage.qml` — same colour, inconsistent casing.
- `#E00052` in `LoginPage.qml` — a hover-only lightening of `#C40046`.
- `#e6003c` — the UI-0 **concept** accent. **Confirmed absent from the source.**
  It was never adopted, and per the brief it must not be.

### Evidence from the approved asset

The brief names `images/logo/techaim_color.png` as the approved Tech Aim
report/logo asset, and instructs that an authoritative token be selected only
after confirming the approved design source. That asset was sampled directly
(3163 × 973, 710,403 opaque pixels):

| Colour in the approved logo | Pixel count | Role |
|---|---:|---|
| **`#A80038`** | **276,718** | dominant brand red — target rings / mark |
| `#BF1919` | 38,687 | secondary red — tagline |
| `#A70037`, `#A80037`, `#A70038` | < 800 each | anti-aliasing fringe of `#A80038` |

`techaim_white.png` and `techaim_black.png` contain **zero** red pixels, as
expected for monochrome variants.

**`#C40046` does not occur anywhere in the approved logo.** Neither does
`#e8003d` or `#e6003c`.

This corroborates the comment in `Theme.qml`, which states its colours were
extracted from the brand asset rather than eyeballed — `brandPrimary =
#a80038` matches the sampled asset exactly.

### The conflict, stated plainly

The brief says the *expected* authoritative red is `#C40046`. The *approved
asset* says `#A80038`. These cannot both be right, and the audit cannot settle
it, because "approved" is a brand decision and not a measurable property.

> **BRAND APPROVAL REQUIRED — authoritative accent.**
> `accentPrimary` is defined in exactly one place. Whichever value Arnold
> confirms is a one-line change; nothing else in the system moves.

Recorded consequences of each choice:

- **`#A80038`** — matches the approved logo pixel-for-pixel, matches the
  existing shared `Theme.qml`, and matches the largest existing consumer (the
  entire report/PDF system). Choosing it changes the appearance of the Version B
  homepage, the Training Lab and the live shooting screens.
- **`#C40046`** — matches the brief's stated expectation, the Version B
  homepage as built, and the Training Lab. Choosing it means the application
  accent deliberately differs from the logo, and the report system moves too.

Contrast, both on `#0B0D10` background, white text on the fill:

| Colour | Contrast vs `#0B0D10` | White text on fill |
|---|---:|---:|
| `#A80038` | 3.6 : 1 | 7.0 : 1 |
| `#C40046` | 4.4 : 1 | 5.8 : 1 |

Both are acceptable for large text and UI fills; neither passes 4.5:1 as a
*small-text foreground* on the dark background, so the accent must not be used
for body copy in either case (see the design system's contrast rules).

---

## 3. Colour inventory — the repeated values worth tokenising

Top literals by frequency (excluding the reds above):

| Hex | Count | Evident role | Proposed token |
|---|---:|---|---|
| `#191b1f` | 36 | panel/card surface | `surfacePrimary` |
| `#5b6270` | 32 | muted text / disabled | `textDisabled` |
| `#9aa0aa` | 27 | secondary text | `textSecondary` |
| `#111111` | 23 | near-black (wordmark, print) | `inkBlack` |
| `#c9ced6` | 20 | light body text (reports) | `textPrimaryOnLight` |
| `#8a8f98` | 20 | muted text variant | `textSecondary` (duplicate) |
| `#e6e8ec` | 18 | light surface (reports) | `surfaceOnLight` |
| `#26272c` | 18 | border | `borderSubtle` |
| `#1f2026` | 16 | elevated surface | `surfaceElevated` |
| `#9a9ba0` | 15 | muted text variant | `textSecondary` (duplicate) |
| `#8a8a92` | 15 | muted text variant | `textSecondary` (duplicate) |
| `#3a3b40` | 15 | strong border | `borderStrong` |
| `#2a2b30` | 13 | border variant | `borderSubtle` (duplicate) |
| `#d0392b` | 12 | error / danger | `errorText` |
| `#e8a13d` | 11 | warning / amber | `warningText` |
| `#20C997` | 8 | success / connected | `successText` |
| `#0d2018` | 8 | success fill | `successBackground` |
| `#2f6fd0` | 7 | information / link | `infoText` |
| `#1f8a4c` | 7 | success variant | `successText` (duplicate) |

**Inconsistency pattern.** Secondary text alone has at least five near-identical
values (`#9aa0aa`, `#8a8f98`, `#9a9ba0`, `#8a8a92`, `#6b7280`) totalling ~86
occurrences. None of them are meaningfully different on screen; they are drift,
not design. The same is true of borders (`#26272c` / `#2a2b30` / `#2A2E36`) and
of the dark surfaces (`#191b1f` / `#1f2026` / `#1B1E24` / `#15171C` / `#1c1f26`).

**LoginPage.qml** declares its own 13-colour private palette
(`_bg`, `_surface`, `_surfaceAlt`, `_input`, `_borderSub`, `_borderStr`, `_red`,
`_redHover`, `_redDark`, `_txt`, `_txtSec`, `_txtMut`, `_green`) — a
self-contained copy of what should be central. The brief explicitly requires
this to move out of the homepage file.

---

## 4. Typography

| Finding | Detail |
|---|---|
| Family references | `theme.fontFamily` ×160, literal `"Segoe UI"` ×149, `"Consolas"` ×50, plus local aliases (`pg.fam` ×50, `dash.fam` ×19, `fam` ×16) |
| Distinct pixel sizes | **22** — 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 22, 24, 26, 28, 40 (+ 0) |
| Most used | 10 px (×216), 11 px (×194), 12 px (×151), 9 px (×118) |

Roughly half of family references bypass the theme and hard-code `"Segoe UI"`
— the same value, but unreachable from a brand switch.

`"Consolas"` (×50) is used for two different jobs: genuinely tabular numerics
(series scores, shot values), where fixed width is *functional*, and decorative
"technical" styling on non-tabular values (profile summary, helper text) — the
latter flagged as a defect in the UI-0 audit.

Both `Segoe UI` and `Consolas` ship with Windows, so neither introduces a
distribution dependency. **No proprietary font is required.**

A 22-step size ramp is not a scale; 9/10/11/12 are used interchangeably for
what is visually the same role.

---

## 5. Spacing, radii, borders

| Property | Distinct values | Distribution |
|---|---:|---|
| `radius` | 12 | 8 (×87), 6 (×45), 10 (×34), 3 (×28), 4 (×19), 2 (×17), 12 (×14), 5, 7, 9, 13, 15 |
| `border.width` | 3 | 1 (×171), 2 (×14), 3 (×2) — **already effectively a scale** |
| margins | many | 22 (×37), 10 (×37), 12 (×32), 16 (×29), 14 (×25), 8 (×24), 20 (×19), 6 (×16) |

Radii cluster on 6/8/10 with a long tail of one-offs. Margins cluster on
8/10/12/14/16/20/22 — close to a 4 px grid but not on one. `border.width` is
the one dimension already consistent and needs only naming.

---

## 6. Brand assets

`images/logo/`:

| File | Status |
|---|---|
| `techaim_color.png` | **approved** — the brand asset named in the brief |
| `techaim_white.png` | in use (`theme.logoWhite`) — dark-background variant |
| `techaim_black.png` | in use (`theme.logoBlack`) — light/print variant |
| `seta.png` | **legacy** — SETA-era artwork |
| `tachus.png`, `tachus_logo.png`, `tachus_logo1.png` | **legacy** — Tachus-era artwork |

No vector source exists for the Tech Aim mark (the only SVGs in the repository
are manual diagrams). Raster only, at 3163 × 973.

> **BRAND APPROVAL REQUIRED — missing assets:** no dedicated monochrome mark, no
> Windows `.ico`, and no vector master. `TechAim.rc` declares no `ICON`
> resource, so the executable currently carries the default Qt/MinGW icon.

**Legacy artwork must not be carried into the new brand structure.** The brief
also forbids copying the obsolete SETA-era EULA artwork.

---

## 7. Product identity

Already centralised, and in good shape — this is the foundation to extend, not
replace:

- `src/app/ProductIdentity.{h,cpp}` is the single authority for `displayName`,
  `fullProductName`, `executableBaseName`, `organisationName`, `legalPublisher`
  (`JAC SHOOTING SOLUTIONS (PTY) LTD`) and `copyrightLine`.
- `BuildFlavour { TechAim, SetaOem }` already exists, with
  `isFlavourBuildable()` refusing `SetaOem`. The OEM seam is present and
  correctly closed.
- `ProductIdentityBridge` exposes it to QML as `PRODUCT`.

**Gap:** `ProductIdentity` carries *names*. It carries no *presentation* —
no accent colour, no logo path, no icon. Those live in `Theme.qml` and in 972
scattered literals. A `BrandPackage` is the missing half.

---

## 8. Legacy branding still present

| Term | Files |
|---|---|
| `TACHUS` | `main.qml` |
| `Tachus` | `LoginPage.qml`, `main.qml` |
| `tachus` | `MatchReportView.qml`, `SeriesComponent.qml`, `SummaryReportView.qml`, `main.qml` |
| `SETA` | `LoginPage.qml`, `main.qml` |
| `Seta` | `MatchReportView.qml` |
| `seta` | `main.qml` |
| `Seeds` | none |

**These are mostly not user-visible branding.** Inspection shows they are
predominantly asset paths (`bg_tachus.png`), legacy identifiers, and — critically
— **valid references to SETA the German electronics supplier**, e.g. the
`removeSetaLaneShootDataFile` data path.

> Per the standing instruction, **do not globally replace SETA.** Each
> occurrence must be classified individually as *branding* (replace) or
> *hardware/supplier/data-path* (keep). That classification is deliberately out
> of scope for UI-1 and is not attempted here.

---

## 9. Values that are FUNCTIONAL, not decorative

These must **never** be absorbed into the brand system, because changing them
would change the meaning of what an athlete or jury sees:

1. **ISSF target ring geometry and scoring-zone colours** — configurable via
   `APPSETTINGS` and tied to rule interpretation, not styling.
2. **Shot-marker colours** distinguishing sighters from match shots, and
   current shot from previous shots.
3. **Live vs Demo indication** — green/red here is a result-integrity signal.
   A brand package may not weaken or recolour it into ambiguity.
4. **Success / warning / error semantics** — a brand may tune the exact hue,
   but may not swap their meanings or make them indistinguishable.
5. **`Consolas` on genuinely tabular numerics** — column alignment in series
   tables is functional.
6. `border.width: 2` as the **selected-state** marker — carries state, not decoration.

---

## 10. Summary of what UI-1 must fix

| # | Problem | Evidence |
|---|---|---|
| 1 | Three competing brand reds | §2 — 48 / 28 / 10 occurrences |
| 2 | 240 distinct colours, 972 literals | §1 |
| 3 | Five near-identical secondary-text greys | §3 — ~86 occurrences |
| 4 | Half of font references bypass the theme | §4 — 149 literal `"Segoe UI"` |
| 5 | 22-step font ramp with no roles | §4 |
| 6 | 12 radius values with no scale | §5 |
| 7 | `LoginPage.qml` owns a private 13-colour palette | §3 |
| 8 | Identity has names but no presentation half | §7 |
| 9 | No Windows icon, no monochrome mark, no vector master | §6 |
| 10 | Decorative monospace on non-tabular values | §4 |

---

## 11. Open questions for Arnold

1. **Authoritative accent** — `#A80038` (approved logo, existing Theme, report
   system) or `#C40046` (the brief's stated expectation, Version B homepage,
   Training Lab)? **Blocking for the accent token only.**
2. **Secondary accent** — the logo's `#BF1919` tagline red: adopt as
   `accentSecondary`, or leave logo-only?
3. **Windows icon** — no `.ico` exists. Commission, or derive from
   `techaim_color.png`? (Deriving is a brand act; not done unilaterally.)
4. **Vector master** — is an SVG of the Tech Aim mark available? Raster-only
   limits print and high-DPI quality.
