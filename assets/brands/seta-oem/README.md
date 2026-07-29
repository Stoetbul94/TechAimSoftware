# SETA OEM brand package — RESERVED, NOT IMPLEMENTED

**Nothing in this folder is approved, and no OEM appearance exists.**

This folder exists so the OEM path is a *real, testable configuration seam*
rather than a hypothetical one. A test can request
`brandFor(BuildFlavour::SetaOem)` and assert that it reports its missing assets
instead of silently falling back to Tech Aim artwork.

## Status

| Asset | Status |
| --- | --- |
| Colour logo | **BRAND APPROVAL REQUIRED** — not supplied |
| White logo | **BRAND APPROVAL REQUIRED** — not supplied |
| Monochrome logo | **BRAND APPROVAL REQUIRED** — not supplied |
| Report logo | **BRAND APPROVAL REQUIRED** — not supplied |
| Windows icon | **BRAND APPROVAL REQUIRED** — not supplied |
| Accent colours | **BRAND APPROVAL REQUIRED** — not supplied |
| Product name | **BRAND APPROVAL REQUIRED** — not supplied |
| Publisher | **BRAND APPROVAL REQUIRED** — not supplied |

`BuildFlavour::SetaOem` is refused by `isFlavourBuildable()`, so a
half-configured OEM edition cannot be shipped by accident.

## Rules for whoever completes this

1. Do **not** copy Tech Aim artwork here and recolour it. That produces a
   derivative of an approved mark without approval.
2. Do **not** copy the obsolete SETA-era artwork from `images/logo/seta.png`
   or the legacy EULA imagery. It is superseded and explicitly out of scope.
3. Do **not** add `if (flavour == SetaOem)` branches to screens. The whole
   point of `BrandPackage` is that screens read tokens and never ask which
   brand they are running.
4. Fill in `makeSetaOem()` in `src/app/BrandPackage.cpp` and drop the approved
   files here. `missingAssets()` empties out on its own once they exist.
5. A blue OEM appearance has been discussed but is **not approved**. Do not
   assume it.

## Note on the name

"SETA" is also the German electronics supplier whose hardware this product
talks to. Valid supplier and hardware references exist throughout the source
(for example the lane shoot-data file path). **Nothing in this folder
justifies a blanket rename of SETA references elsewhere.**
