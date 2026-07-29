# Tech Aim — Brand Asset Register

**Version:** 1.0 (UI-1) · Authoritative inventory of every brand asset.

Rule: **an absent asset is recorded, never invented.** Nothing here is
derived, recoloured or substituted.

---

## 1. Tech Aim — APPROVED

| Asset | File | Size | Status | Used by |
|---|---|---|---|---|
| Colour logo | `images/logo/techaim_color.png` | 3163 × 973 | **approved** | reports, PDFs, light surfaces |
| White logo | `images/logo/techaim_white.png` | 3163 × 973 | in use | dark application surfaces (`theme.logoWhite`) |
| Monochrome | `images/logo/techaim_black.png` | 3163 × 973 | in use | single-ink / print (`theme.logoBlack`) |
| Report logo | `images/logo/techaim_color.png` | — | in use | report headers |
| Windows icon | — | — | ❗ **BRAND APPROVAL REQUIRED** | executable, taskbar, shortcuts |
| Vector master | — | — | ❗ **BRAND APPROVAL REQUIRED** | print, high-DPI |

### Colour verified from the asset

`techaim_color.png` sampled at 710,403 opaque pixels:

| Colour | Pixels | Role |
|---|---:|---|
| `#A80038` | 276,718 | **approved `accentPrimary`** |
| `#BF1919` | 38,687 | tagline — **logo-intrinsic, not a UI colour** |
| `#A70037`, `#A80037`, `#A70038` | < 800 each | anti-aliasing fringe |

`techaim_white.png` and `techaim_black.png` contain zero red pixels, as
expected.

### Outstanding

1. **Windows `.ico`.** `TechAim.rc` declares no `ICON` resource, so the
   executable ships the default Qt/MinGW icon — visible in the taskbar, Alt-Tab
   and shortcuts. Deriving one from the raster logo is a brand act and has not
   been done. Reported by `BrandPackage::missingAssets()`.
2. **Vector master.** Raster only. Limits print and high-DPI quality. The only
   SVGs in the repository are manual diagrams.

## 2. SETA OEM — RESERVED, NOT IMPLEMENTED

Every asset outstanding. See `assets/brands/seta-oem/README.md`.

| Asset | Status |
|---|---|
| All artwork, accents, name, publisher | ❗ **BRAND APPROVAL REQUIRED** |

`BuildFlavour::SetaOem` is refused by `isFlavourBuildable()`, so a
half-configured OEM edition cannot ship by accident.

## 3. Legacy artwork — DO NOT USE

Present in the repository, retained only so historical builds and migration
paths still resolve. **None of it is Tech Aim branding.**

| File | Era |
|---|---|
| `images/logo/seta.png` | SETA-era |
| `images/logo/tachus.png` | Tachus-era |
| `images/logo/tachus_logo.png` | Tachus-era |
| `images/logo/tachus_logo1.png` | Tachus-era |
| `images/loginPage/bg_tachus.png` | Tachus-era background |

Prohibited: presenting any of these as Tech Aim branding, and copying the
obsolete SETA-era EULA artwork into `assets/brands/`.

## 4. Where assets live

Artwork is served from `images/logo/` through the compiled `qrc`, because that
is where the shipping build reads it. `assets/brands/` is the **registry and
approval record**, not a second copy — duplicating binaries would create two
sources of truth for what "the logo" is.

## 5. Usage rules

- Clear space ≥ the height of the mark's target ring on all sides.
- Minimum on-screen height 20 px.
- Pick the variant that matches the background; never place the colour logo on
  a busy or mid-tone background.
- **Never** recolour, tint, stretch, rotate, crop or add effects.
- `#BF1919` inside the artwork is intrinsic and must not become a UI accent.

## 6. Change control

Adding, replacing or removing an approved asset requires:
1. Arnold's approval, recorded here with a date.
2. An update to `src/app/BrandPackage.cpp`.
3. A test update in `tests/reliability/tst_brandpackage.cpp`.
4. A note in the design-system migration table if the appearance changes.

| Date | Change | Approved by |
|---|---|---|
| 2026-07-29 | `accentPrimary` fixed at `#A80038`; `#C40046` becomes hover, `#80032A` pressed; `#BF1919` logo-only | Arnold |
