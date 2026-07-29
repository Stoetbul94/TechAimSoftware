# Brand Assets for Tech Aim Documentation

Document version 1.0 (P0-J refinement) · Application commit `84db7a2`

Defines which artwork the manuals may use, and records candidate assets that
are **not yet approved**.

## Approved for use in manuals

| Asset | Path | Use |
|---|---|---|
| Tech Aim wordmark (colour) | `images/logo/techaim_color.png` | manual cover and page headers |
| Tech Aim wordmark (black) | `images/logo/techaim_black.png` | grayscale / print |
| Tech Aim wordmark (white) | `images/logo/techaim_white.png` | dark backgrounds |

These are the assets the **application itself** ships and renders on every
report, so a manual using them matches what the reader sees on screen.

**Brand colour: `#C40046`** (Tech Aim red). Supporting shades in use:
`#A80038` (pressed), `#E8004F` (highlight).

## Candidate assets — NOT APPROVED

Two SVG files were supplied from outside the repository:

| File | Colours | Assessment |
|---|---|---|
| `logo.svg` (120×40 wordmark) | `#1f2937` slate, `#f59e0b` amber | **Does not match the product** |
| `logo-mark.svg` (32×32 square mark) | `#1f2937` slate, `#f59e0b` amber | **Does not match the product** |

**Why they are not adopted:**

1. **Colour conflict.** They use a slate/amber palette. The Tech Aim
   application uses red `#C40046` throughout its interface and on every
   exported report. Putting slate/amber artwork on a manual that documents a
   red application would present a **third** visual identity to the reader.
2. **They are placeholders.** Both are Arial text in a plain rectangle, not
   finished artwork. `logo-mark.svg` is the letters "T" over "A".
3. **They originate outside this repository** (a separate `TECHAIM/public/brand`
   tree, which appears to belong to a web project). Nothing establishes them as
   the approved identity for this product.

**Status: CANDIDATE — BRAND APPROVAL REQUIRED.**

If these are intended to become the Tech Aim identity, that is a **product
branding decision**, and it would need to be applied to the application first
(interface, report headers, theme) and only then to the manuals — not the
other way round. Until that happens the manuals continue to use the shipped
wordmark.

## Windows application icon

**There is no approved `.ico` application icon.** The build does not embed
one, and Windows shows its default icon.

- **Do not invent one.** No icon is to be generated, mocked or implied.
- Screenshot **SS-31** (Windows application icon in Explorer / the taskbar) is
  registered as **PENDING — BLOCKED** and cannot be captured until an approved
  icon exists.
- Manuals must not show or describe an application icon as if one existed.

`logo-mark.svg` is the obvious *shape* candidate for an eventual icon, but per
the section above it is not approved, and its palette does not match the
product.

**Status: WINDOWS RC1 DEPENDENT + BRAND APPROVAL REQUIRED.**

## End-user agreement artwork — do not reproduce

`images/loginPage/End User Agreement SETA-1.png` and `-2.png` are rendered
images of a **SETA-era agreement naming an entity other than JAC SHOOTING
SOLUTIONS (PTY) LTD**.

- **Must not** appear in any manual, screenshot or generated document.
- **Must not** be described as the product's licence terms.
- Any screenshot that would include them is rejected.

**Status: LEGAL REPLACEMENT REQUIRED BEFORE EXTERNAL BETA.**
