# Tech Aim — Brand Flavour Guide

**Version:** 1.0 (UI-1)
**Implementation:** `src/app/BrandPackage.{h,cpp}`, `src/app/ProductIdentity.{h,cpp}`

How a future company/OEM edition works, and the boundary it must not cross.

> **No OEM appearance is implemented.** `BuildFlavour::SetaOem` is reserved,
> empty, and refused by `isFlavourBuildable()`. A blue OEM look has been
> discussed and is **not approved**.

---

## 1. The split

| Concern | Owner |
|---|---|
| What the product is **called** | `ProductIdentity` |
| What the product **looks like** | `BrandPackage` |
| What the product **does** | controllers, scoring, reliability — *neither* |

That third row is the point. An OEM edition is the **same binary logic** with
different presentation.

## 2. The hard boundary

A `BrandPackage` may set: product name, short name, publisher, five artwork
paths, four accent colours, the logo-intrinsic colour, resource namespace, PDF
attribution, manual brand name, and (only where genuinely region-locked) a
default language.

It may **never** influence:

- scoring, ring geometry or score interpretation
- ISSF rules, match phases, timing
- discipline availability
- Training Lab analytics
- session recovery, the journal, or the hash chain
- target communication

**Test:** if a brand value could change what a score *is*, it does not belong
in a brand package. `tests/reliability/tst_brandpackage.cpp` asserts that
inspecting a different package changes neither the running identity nor the
running accent, and that the reserved package does not inherit Tech Aim
artwork.

## 3. Adding a brand — the whole procedure

1. **Get approval.** Name, publisher, artwork, accent. Nothing starts before
   this; there is no "provisional" brand.
2. **Add the flavour** to `BuildFlavour` in `ProductIdentity.h`.
3. **Add a `makeX()`** in `BrandPackage.cpp` with the approved values.
4. **Drop the approved files** into `assets/brands/<flavour>/`.
5. **Add a `ProductIdentity`** entry for the naming.
6. **Enable the flavour** in `isFlavourBuildable()` — only once
   `BrandPackage::isComplete()` returns true.
7. **Add a test** asserting the package's values and that programme behaviour
   is unchanged.
8. **Build with the flavour selected at build-configuration time.**

There is **no runtime brand selector**. Identity is fixed when the binary is
produced, so a build cannot present itself as two products.

## 4. What is forbidden

| Forbidden | Why |
|---|---|
| `if (flavour == SetaOem) colour = blue` | Screens must read tokens and never ask which brand they are. One conditional becomes fifty. |
| A long-lived branding branch | Divergence. One source tree, configuration only. |
| Inventing or deriving artwork | A recoloured mark is a different mark and needs its own approval. |
| Recolouring an approved logo programmatically | Same. |
| Copying legacy SETA/Tachus artwork into a new brand | Superseded and explicitly out of scope. |
| Silently defaulting an unset asset to Tech Aim's | Hides an incomplete brand. `missingAssets()` reports instead. |
| A brand-driven default language override | Language is the operator's choice unless a package is genuinely region-locked. |

## 5. User data and identity

`organisationName` drives the `QSettings` root and the AppData location. A
separate OEM data identity is a **deliberate configuration choice**, not an
automatic consequence of branding:

- **Shared identity** (default) — an OEM build reads the same session archive.
  Correct when it is the same product wearing a different badge.
- **Separate identity** — set a distinct `organisationName`. Correct when the
  editions are sold as different products. Requires a migration decision:
  existing sessions do **not** move on their own.

Do not change this casually. It decides whether an operator's history survives
an edition switch.

## 6. Testability

Every flavour must be independently testable without being built:
`brandFor(BuildFlavour::X)` returns a package for inspection regardless of
which flavour the binary is. That is how the reserved OEM package is tested
today — it is asserted to be empty, incomplete, and non-inheriting.

## 7. A note on the name "SETA"

**SETA is also the German electronics supplier** whose hardware this product
talks to. Valid supplier, hardware and data-path references exist throughout
the source (for example `removeSetaLaneShootDataFile`).

Nothing in this guide, and nothing in `assets/brands/seta-oem/`, justifies a
blanket rename of SETA references. Each occurrence is classified individually
as *branding* (replace) or *hardware/supplier* (keep).

## 8. Current state

| Flavour | Buildable | Package | Notes |
|---|---|---|---|
| `TechAim` | ✅ yes | complete except the Windows icon | approved accent `#A80038` |
| `SetaOem` | ❌ refused | empty, every asset outstanding | reserved seam only |
