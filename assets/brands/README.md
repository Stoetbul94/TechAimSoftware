# Tech Aim — brand asset structure

One folder per approved brand package. A folder is a **complete, reviewable
unit for brand approval**: everything a build needs to present itself lives in
one place, and anything absent is recorded rather than substituted.

```
assets/brands/
  techaim/     the current, approved product brand
  seta-oem/    RESERVED — documented, empty, not implemented
```

## Rules

1. **Never invent artwork.** If an approved asset does not exist, record
   `BRAND APPROVAL REQUIRED` in that brand's `README.md` and leave the field
   empty in `src/app/BrandPackage.cpp`. `BrandPackage::missingAssets()` reports
   it; nothing derives a substitute.
2. **Never recolour an approved logo programmatically.** A tinted or
   re-hued mark is a different mark and needs its own approval.
3. **No legacy artwork.** Tachus-era and SETA-era images
   (`images/logo/seta.png`, `tachus*.png`) are **not** carried into this
   structure, and the obsolete SETA-era EULA artwork must never be copied here.
4. **Presentation only.** A brand package may not affect scoring, ISSF rules,
   Training analytics, recovery, target communication or discipline
   availability. See `src/app/BrandPackage.h` for the enforced boundary.
5. **One source tree.** A new brand is a new folder plus a `BrandPackage`
   entry — never a source branch and never an `if (company == …)` in a screen.

## Current state

Assets are still served from `images/logo/` through the compiled `qrc`, because
that is where the shipping build reads them and moving them is a resource-path
change with no design benefit in this phase. This folder is the **registry and
approval record**; `docs/design/Brand_Asset_Register.md` is the authoritative
inventory.
