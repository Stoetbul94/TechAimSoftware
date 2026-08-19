# Windows Application Icon — Specification and Integration Path

Document version 1.0 (P0.1) · Application commit `169eef9`

**STATUS: APPROVED — ARNOLD BAILIE, 2026-08-15 (RC3a).**

The icon is **not invented**: it is the target emblem taken from the approved
brand mark `images/logo/techaim_color.png`, cropped to its own bounding box and
padded onto a square canvas. No new logo was drawn.

`images/logo/techaim.ico` carries the seven sizes Windows requests —
16, 24, 32, 48, 64, 128 and 256 px — and is embedded in `TechAim.exe` as the
first (lowest-numbered) `ICON` resource, which is the one Explorer, the taskbar
and shortcuts use. The Inno Setup script uses the same file for the installer
and assigns it explicitly to the Start Menu and Desktop shortcuts, so neither
falls back to the generic executable icon.

The wordmark itself was rejected for this purpose: at 3.25:1 it is illegible
below about 64 px.

---

## 1. Asset audit

| Asset | Dimensions | Suitable as an icon? |
|---|---|---|
| `images/logo/techaim_color.png` | wordmark (wide) | **No** — a wide wordmark is illegible squashed into a square |
| `images/logo/techaim_black.png` | wordmark (wide) | No — same reason |
| `images/logo/techaim_white.png` | wordmark (wide) | No — same reason |
| `logo.svg` (supplied externally) | 120×40 wordmark | No — wide, and the palette does not match the product |
| `logo-mark.svg` (supplied externally) | 32×32 square mark | **Shape is right, but NOT APPROVED** — slate/amber palette conflicts with the product's red `#C40046`; it is Arial letters "T" over "A", not finished artwork |

**Conclusion: no approved square mark exists.** The three shipped assets are
all wide wordmarks. See `_shared/brand-assets.md`.

## 2. Why a wordmark cannot simply be reused

At 16×16 — the size Windows uses in Explorer lists, the taskbar and window
title bars — a wordmark reduces to an unreadable smear. This is exactly the
"stretched wordmark text at tiny sizes" the requirements forbid. A **distinct
square mark** is required.

## 3. Required specification

### Sizes (all in one multi-resolution `.ico`)

| Size | Purpose |
|---|---|
| 16×16 | Explorer details view, window title bar, taskbar small |
| 20×20 | some Explorer/list scalings |
| 24×24 | Explorer medium |
| 32×32 | desktop shortcut, Alt-Tab |
| 40×40 | some DPI scalings |
| 48×48 | Explorer large icons |
| 64×64 | Explorer extra-large |
| 128×128 | thumbnails |
| 256×256 | PNG-compressed inside the `.ico`; Explorer jumbo, installer |

32-bit colour with an 8-bit alpha channel.

### Design requirements

1. **Recognisable at 16×16.** This is the binding constraint. Design at 16×16
   first and scale *up*, not the reverse.
2. **Transparent background** where appropriate, so it sits correctly on light
   and dark taskbars.
3. **No wordmark text at small sizes.** A single strong glyph or mark.
   Multi-size `.ico` files may carry a more detailed variant at 128/256.
4. **Tech Aim branding** — red `#C40046` is the product colour.
5. Must read correctly on **both** light and dark Windows themes.
6. Distinguishable at a glance from other range/scoring software on a taskbar.

### Where it must appear

Explorer · taskbar · window title bar · desktop and Start shortcuts ·
Alt-Tab · installer and Add/Remove Programs (RC1).

## 4. Integration path — prepared, not applied

When an approved `.ico` exists:

1. Commit it as `images/logo/techaim.ico`.
2. Add **one line** to `TechAim.rc`, above the `VS_VERSION_INFO` block:

   ```
   IDI_ICON1 ICON "images/logo/techaim.ico"
   ```

   The lowest-numbered icon resource becomes the application icon, so it must
   be first.
3. Rebuild — `TechAim.rc` is already wired via `win32: RC_FILE = TechAim.rc`,
   so **no `.pro` change is required**.
4. Verify with:

   ```powershell
   [System.Drawing.Icon]::ExtractAssociatedIcon("release\TechAim.exe").Size
   ```

   and by eye in Explorer at every view size.
5. Capture screenshot **SS-31**, currently PENDING — BLOCKED.
6. Update this document and the validation checklist to VERIFIED.

**No `TechAim.rc` change is made now**, per the requirement not to update it
until an approved icon exists.

## 5. Required user action

Supply **one** of:

- an approved square Tech Aim mark (SVG or ≥256×256 PNG with transparency),
  from which the multi-size `.ico` can be produced; **or**
- a finished multi-resolution `.ico` meeting section 3; **or**
- approval of a specific design direction, with the palette corrected to the
  product's red.

Until then this remains a **release blocker for a professional-looking
installer** at Windows RC1.
