# Tech Aim Design System

**Version:** 1.0 (UI-1 foundation)
**Product:** Tech Aim Electronic Target Control 0.9.0
**Publisher:** JAC SHOOTING SOLUTIONS (PTY) LTD
**Implementation:** `src/ui/theme/DesignTokens.qml`, `Typography.qml`, `Spacing.qml`
**Evidence:** `docs/design/current-design-audit.md`

> **Approval status.** Only the accent decision in §3 is *approved* (by Arnold,
> 2026-07-29). Everything else in this document is the **proposed** system,
> derived from what the application already does. Nothing here is described as
> approved unless it says so.
>
> The rendered appearance of any screen remains
> **HUMAN VISUAL CHECK REQUIRED** until Arnold approves a real screenshot.

---

## 1. Product personality

Tech Aim is **instrumentation**, not a consumer app. An operator is running a
match that produces results someone will be held to. The interface should feel
like a well-made measuring device: precise, quiet, unambiguous, and honest
about what it does not know.

| It should read as | It must never read as |
|---|---|
| Precise, calm, deliberate | Playful, decorative, "gamified" |
| Dense but scannable | Cluttered or shouty |
| Confident about state | Ambiguous about Live vs Demo |
| Professional enough to show a federation | Prototype or unfinished |

## 2. Design principles

1. **State before decoration.** Colour, weight and border width carry meaning
   first. A 2 px border means *selected*; it is not a styling flourish.
2. **One thing is loudest.** Every screen has exactly one primary action. If
   everything is emphasised, nothing is.
3. **Never hide the consequential.** A restart requirement, a Demo-mode
   session or a lost connection outranks visual tidiness.
4. **Nothing clips, ever.** Content that does not fit scrolls. Truncated
   content is the single strongest signal of an unfinished product, and it
   loses controls the operator needs.
5. **Touch is a first-class input.** This runs on range tablets.
6. **The token is the source of truth.** A screen that hard-codes a colour has
   opted out of the brand.

---

## 3. Colour palette

### 3.1 The accent — APPROVED 2026-07-29

Before UI-1 the product shipped **three** competing brand reds. The approved
logo `images/logo/techaim_color.png` was sampled directly (710,403 opaque
pixels) and contains `#A80038` (276,718 px) and `#BF1919` (38,687 px) — and
does **not** contain `#C40046`, `#e8003d` or `#e6003c`.

| Token | Hex | Purpose | Permitted | Prohibited | Contrast |
|---|---|---|---|---|---|
| `accentPrimary` | `#A80038` | brand accent, rest state | selected borders, primary button fill, section accent bars, active indicators | **body text**, large background fills, error signalling | 3.6:1 on `backgroundPrimary` — UI/large text only. White on it: **7.0:1 ✅** |
| `accentHover` | `#C40046` | hover and keyboard focus | hover fill of a primary action, focus outline | rest states (would read as two accents) | 4.4:1 on `backgroundPrimary`. White on it: 5.8:1 ✅ |
| `accentPressed` | `#80032A` | active/pressed | pressed fill only | rest or hover | White on it: 9.0:1 ✅ |
| `accentSubtle` | `#2D0A18` | tinted fill behind a selected card | selected card/row background | text, borders, large areas | a surface, not a foreground |
| `textOnAccent` | `#FFFFFF` | the only text colour on an accent fill | text/icons on accent | anywhere else | ≥5.8:1 on all accents ✅ |
| `brandLogoSecondary` | `#BF1919` | **logo artwork only** | nothing in the UI | **any application use** | n/a |

**Superseded.** `#e8003d` (live shooting UI, 28 occurrences) and `#e6003c`
(UI-0 concept, never shipped) are legacy/concept values. They must not be
reintroduced; migration of the remaining screens is tracked in §25.

### 3.2 Surfaces

| Token | Hex | Purpose | Prohibited |
|---|---|---|---|
| `backgroundPrimary` | `#0B0D10` | window canvas | as a card fill (loses the hierarchy) |
| `backgroundSecondary` | `#0C0E12` | app bar / header strip | body content |
| `surfacePrimary` | `#15171C` | panels and cards | full-screen fill |
| `surfaceSecondary` | `#1B1E24` | nested cards, popovers | same-level siblings of `surfacePrimary` |
| `surfaceElevated` | `#1F2026` | hover surface, raised rows | rest state |
| `inputBackground` | `#1D2026` | text fields, pickers | non-editable content — it *promises* editability |

Elevation reads by lightness: canvas → panel → nested → hover. There are no
shadows; the product is flat by intent, so lightness is the only cue and must
not be spent decoratively.

### 3.3 Borders and text

| Token | Hex | Purpose | Contrast |
|---|---|---|---|
| `borderSubtle` | `#2A2E36` | default card/field outline | structural only |
| `borderStrong` | `#3A404A` | secondary buttons, dividers that must read | structural only |
| `textPrimary` | `#F3F6FA` | headings, primary content | **15.8:1** on `surfacePrimary` ✅ AAA |
| `textSecondary` | `#AAB4C0` | labels, captions, supporting copy | **7.9:1** ✅ AAA |
| `textDisabled` | `#6F7A86` | disabled controls, micro-labels | **3.4:1** — large/secondary only ⚠️ never body copy |

The audit found five near-identical greys doing the `textSecondary` job
(~86 occurrences). They collapse to the two above plus a disabled step.

### 3.4 Status — FUNCTIONAL, not decorative

| Token | Hex | Meaning |
|---|---|---|
| `successBackground` / `successText` | `#0D2018` / `#20C997` | connected, ready, passed |
| `warningBackground` / `warningText` | `#2A1A05` / `#E8A13D` | needs attention, restart required |
| `errorBackground` / `errorText` | `#2A0B10` / `#D0392B` | failed, not connected, blocked |
| `infoBackground` / `infoText` | `#0C1A2E` / `#2F6FD0` | neutral information |
| `focusOutline` | `#C40046` (2 px) | keyboard focus |
| `scrim` | `#AA000000` | dim behind a modal |

> A brand package may tune these hues. It may **not** swap their meanings, or
> make them hard to tell apart. In this product they carry connection state and
> **Live vs Demo**, and Live/Demo confusion is a result-integrity risk, not a
> styling preference.

---

## 4. Typography

`Segoe UI` (UI) and `Consolas` (numeric). Both ship with Windows: **no
proprietary font is distributed**, and nothing falls back to a substitute face.

| Role | Size | Weight | Tracking | Use |
|---|---:|---|---:|---|
| `displayTitle` | 28 | bold | 0 | rare, full-screen moments |
| `pageTitle` | 22 | bold | 0 | the screen's name |
| `sectionTitle` | 16 | bold | 0 | panel headings |
| `cardTitle` | 14 | bold | 0 | event/programme card names |
| `body` | 13 | regular | 0 | default copy |
| `bodyStrong` | 13 | bold | 0 | emphasis within copy |
| `helperText` | 11 | regular | 0 | supporting explanation |
| `label` | 10 | bold | 2.0 | uppercase micro-label above a field |
| `buttonText` | 14 | bold | 0 | all buttons |
| `numericMetric` | 14 | bold | 0 | **Consolas** — tabular figures only |
| `statusText` | 11 | bold | 0.5 | chips |

The audit found **22 distinct pixel sizes**, with 9/10/11/12 used
interchangeably for the same visual role. Eleven roles replace them.

**`Consolas` discipline.** Permitted only where fixed width does a job —
series score columns, shot tables, anything read down a column. Using it for
"technical" flavour on prose or a single value makes the product look like
console output and was flagged as a defect in UI-0.

---

## 5. Spacing scale

4 px grid: `spacing2` `spacing4` `spacing8` `spacing12` `spacing16`
`spacing24` `spacing32`.

`panelPadding = 22` is kept off-grid deliberately: it was the single most
common margin in the audit (37 occurrences), and shifting every panel by 2 px
for grid purity would be a visual change with no user benefit.

## 6. Border and radius scale

| Token | Value | Use |
|---|---:|---|
| `radiusSmall` | 6 | chips, inputs, small controls |
| `radiusMedium` | 8 | buttons, cards |
| `radiusLarge` | 10 | panels |
| `radiusPill` | 999 | toggles, badges |
| `borderThin` | 1 | default outline |
| `borderSelected` | 2 | **carries state** — selected only |

`border.width` was already consistent before UI-1 (1 ×171, 2 ×14) and needed
only naming. Radii had 12 values; four remain.

## 7. Elevation and panel hierarchy

Flat by intent. Depth is lightness, not shadow:

```
backgroundPrimary   canvas
└ surfacePrimary    panel            + borderSubtle
  └ surfaceSecondary  nested card    + borderSubtle
    └ surfaceElevated hover/raised
```

A panel may accent its leading edge with a 3 px `accentPrimary` bar. At most
one such bar per panel.

---

## 8. Buttons

| Variant | Fill | Border | Text | Height |
|---|---|---|---|---|
| Primary | `accentPrimary` → hover `accentHover` → pressed `accentPressed` | none | `textOnAccent` | 56 |
| Secondary | transparent | `borderStrong` 1 px | `textSecondary` | 56 |
| Ghost | transparent | none | `textSecondary` | 44 min |

**One primary per screen.** Disabled: `disabledOpacity 0.40`, no hover, no
pointer cursor. Focus: 2 px `focusOutline`, never focus-by-fill-only.

## 9. Input fields

`inputBackground`, `radiusSmall`, `borderSubtle` 1 px → focused `accentHover`
2 px. Height 52. Every field carries a `label` micro-label above it **and** a
placeholder when empty — an unlabelled empty box was a real UI-0 finding.

## 10. Status chips

`successBackground` + 1 px `successText` border + `statusText`, optional 7 px
dot. Height 28–32 (a chip is a *readout*, not a button; when it is tappable it
becomes a control and takes 44 px).

**A chip must never contradict the field beside it.** "Connected" next to an
empty port field was a UI-0 finding and is prohibited.

## 11. Warnings and errors

Inline banner: tinted background + 1 px matching border + `body` text + icon.
**Never smaller than `helperText` (11 px).**

> A restart requirement rendered at 8 px muted grey was a real UI-0 defect. If
> it changes what the operator must do next, it gets a banner.

## 12. Event / programme cards

78 px, `surfaceSecondary`, `radiusMedium`, `borderSubtle` 1 px. Selected:
`accentSubtle` fill, `accentPrimary` 2 px border, filled radio.
Contents: 38 px badge · title (`cardTitle`) · one subtitle line
(`helperText`) · selection indicator. **At most one secondary line** — the
UI-0 audit found three-line cards unreadable at a glance.

Cards are grouped under `label` headings that state the *kind* of activity:
**Official ISSF Match / Finals / Training Lab / Practice**. An operator must
never have to work out whether they are about to start an official match.

## 13. Selected states

Simultaneously: 2 px `accentPrimary` border, `accentSubtle` fill, bold title,
filled indicator. Four signals, because a single one fails for a colour-blind
operator or on a sunlit range display.

## 14. Disabled states
`disabledOpacity 0.40`, no hover, no pointer cursor, not focusable. Never
communicate disabled by colour alone.

## 15. Loading states
Not yet designed — **DESIGN REQUIRED**. The application is currently
synchronous on the paths that matter. Do not invent a spinner ad hoc.

## 16. Empty states
State the fact, then the action: "No folder selected — click to choose", not
"Tap to set folder…". Desktop wording ("click"), never touch-only wording.

## 17. Touch targets

| Class | Size |
|---|---:|
| Absolute floor | **44 × 44** |
| Fields, selectors, secondary buttons | 52 |
| Primary actions, discipline cards | 56–58 |
| Event/programme cards | 78 |

Adjacent targets keep ≥ 8 px separation.

## 18. Screen-width behaviour

See `Screen_Layout_Rules.md`. Rule: **the primary action and the event list are
never clipped**, at any supported size.

## 19. Accessibility and contrast

- Body text ≥ 4.5:1; `textPrimary` 15.8:1 and `textSecondary` 7.9:1 both clear AAA.
- `textDisabled` is 3.4:1 — permitted for micro-labels and disabled controls
  only, **never body copy**.
- The accent is **not** a text colour on dark surfaces (3.6:1).
- State is never colour alone (§13).
- Keyboard focus is always visible.
- **Not yet verified:** screen-reader labelling, and legibility under range
  lighting. **HUMAN VISUAL CHECK REQUIRED.**

## 20. Logo usage

| Context | Asset |
|---|---|
| Dark application surfaces | `techaim_white.png` |
| Light / neutral | `techaim_color.png` |
| Single-ink / print | `techaim_black.png` |

Clear space ≥ the height of the mark's target ring on all sides. Minimum
height 20 px on screen. **Never** recolour, tint, stretch, rotate, add effects,
or place the colour logo on a busy background. `#BF1919` inside the artwork is
intrinsic and is not a UI colour.

## 21. Report / PDF branding
White A4 pages; `techaim_color.png` in the header; attribution
"Tech Aim Electronic Target Control" and the publisher in the footer.
The report system is the largest existing consumer of `accentPrimary` and is
**already on `#A80038`** — the approved accent needs no report change.

## 22. Windows branding
Window title = `PRODUCT.fullProductName`. Version resource carries the product
name, version and `JAC SHOOTING SOLUTIONS (PTY) LTD`.

> **BRAND APPROVAL REQUIRED — Windows icon.** No `.ico` exists and `TechAim.rc`
> declares no `ICON`, so the executable ships the default Qt/MinGW icon.
> Deriving one from the raster logo is a brand act and has not been done.

## 23. German text expansion

German runs ~35% longer ("Betriebsart" for "Mode", "Netzwerkfreigabe" for
"Share"). `Typography.germanExpansionFactor = 1.35`.

Rules: labels size to content, never to the English string; buttons take
padding not fixed width, or elide with a tooltip; two-line wrapping is
acceptable, clipping is not; **never shrink the font to fit**. Sampled German
states are checked in `Screen_Layout_Rules.md` §6.

## 24. Version B homepage structure

```
┌ Header ─ page title + LIVE/DEMO badge ───────────────────────┐
├ Session setup (44%) ─────────┬ Choose an event (56%) ────────┤
│ scrollable:                  │ weapon → distance → sub-disc. │
│  athlete · connection ·      │ OFFICIAL ISSF MATCH           │
│  operating mode · warning ·  │ FINALS                        │
│  network share ·             │ TRAINING LAB                  │
│  selected-programme summary  │ PRACTICE                      │
├ Action bar (full width, 88) ─────────────────────────────────┤
│ READY TO START recap        [Load saved session] [Start →]   │
└ Footer strip ────────────────────────────────────────────────┘
```

Non-negotiable: the event list gets the greater width; the primary action is
**outside** the scrollable column so it cannot be clipped; the selected event
is unmistakable; secondary actions are visually subordinate.

## 25. Future OEM branding rules

Summarised — full detail in `Brand_Flavor_Guide.md`.

1. Same source, same controllers, same scoring, same reliability layer.
2. A brand differs **only** by `BrandPackage` values.
3. **Never** `if (flavour == X) colour = Y` in a screen. Screens read tokens.
4. Never invent, derive or recolour artwork. Absent assets are reported by
   `BrandPackage::missingAssets()`.
5. A flavour is unbuildable until its package is complete
   (`isFlavourBuildable()` refuses `SetaOem` today).
6. A brand package may not touch scoring, rules, analytics, recovery, target
   communication or discipline availability.

### Migration status

| Area | State |
|---|---|
| `LoginPage.qml` (Version B homepage) | **migrated** — 0 hard-coded colours |
| `Theme.qml` | exposes the token layer; legacy properties preserved |
| Report system | already on `#A80038`; not yet token-bound |
| Live shooting UI, HUDs, dialogs, Settings | **legacy styling** — deferred by scope |

UI-1 was explicitly scoped to the homepage. The remaining ~20 screens keep
their existing styling and are unaffected.
