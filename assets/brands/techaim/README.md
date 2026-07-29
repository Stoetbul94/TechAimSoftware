# Tech Aim brand package — APPROVED

The current and only buildable brand. Registered in
`src/app/BrandPackage.cpp` as `makeTechAim()`; the authoritative inventory is
`docs/design/Brand_Asset_Register.md`.

## Identity

| Field | Value | Status |
| --- | --- | --- |
| Product name | Tech Aim Electronic Target Control | approved |
| Short name | Tech Aim | approved |
| Publisher | JAC SHOOTING SOLUTIONS (PTY) LTD | approved |

## Colour

| Token | Value | Status |
| --- | --- | --- |
| `accentPrimary` | `#A80038` | **approved 2026-07-29** — sampled from `techaim_color.png` (276,718 of 710,403 opaque px) |
| `accentHover` | `#C40046` | approved 2026-07-29 |
| `accentPressed` | `#80032A` | approved 2026-07-29 |
| `accentSubtle` | `#2D0A18` | approved 2026-07-29 |
| logo-intrinsic | `#BF1919` | logo artwork only — **not** an application accent |

Superseded: `#e8003d` (live shooting UI) and `#e6003c` (UI-0 concept) are
legacy/concept values and must not be reintroduced.

## Artwork

Served from `images/logo/` through the compiled `qrc` — that is where the
shipping build reads them.

| Asset | File | Status |
| --- | --- | --- |
| Colour logo | `images/logo/techaim_color.png` | approved, 3163 × 973 |
| White logo | `images/logo/techaim_white.png` | in use, dark backgrounds |
| Monochrome | `images/logo/techaim_black.png` | in use, single-ink / print |
| Report logo | `images/logo/techaim_color.png` | in use |
| Windows icon | — | **BRAND APPROVAL REQUIRED** |
| Vector master | — | **BRAND APPROVAL REQUIRED** |

### Missing

- **Windows `.ico`.** `TechAim.rc` declares no `ICON` resource, so the
  executable carries the default Qt/MinGW icon. Deriving one from the raster
  logo is a brand act and has not been done.
- **Vector master.** Only raster exists. This limits print and high-DPI
  quality; no SVG of the mark is in the repository.

## Prohibited

- Recolouring, tinting or re-hueing any file above.
- Using `#BF1919` as a UI accent.
- Reintroducing `images/logo/seta.png` or `tachus*.png` as Tech Aim branding.
