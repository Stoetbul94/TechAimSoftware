# Tech Aim — Component Catalogue

**Version:** 1.0 (UI-1) · Companion to `TechAim_Design_System.md`
**Preview:** `qml tools/designsystem/DesignSystemGallery.qml`

Status is stated honestly per component: **BUILT** (a reusable file exists),
**SPECIFIED** (documented, implemented inline on the homepage, not yet
extracted), or **DESIGN REQUIRED**.

> UI-1 was scoped to the homepage. Components were extracted where they are
> genuinely reusable and low-risk; the rest are specified so the next migration
> has a contract to build against rather than a blank page.

---

## Built

### TaButton — `src/ui/components/TaButton.qml`

One component, three variants, so the three do not drift apart.

| Variant | Fill | Border | Text | Height |
|---|---|---|---|---|
| `Primary` | `accentPrimary` → `accentHover` → `accentPressed` | none | `textOnAccent` | `controlHeightLarge` (56) |
| `Secondary` | transparent | `borderStrong` 1 px | `textSecondary` | 56 |
| `Ghost` | transparent | none | `textSecondary` | `touchMinimum` (44) |

States: default · hover · pressed · disabled (`disabledOpacity`, no hover, no
pointer cursor) · keyboard focus (2 px `focusOutline`, Tab-reachable,
Return/Space activate).

**Rules.** One `Primary` per screen. Disabled is never communicated by colour
alone. Width is content + 44 px padding so German expansion does not truncate.

### TaStatusChip — `src/ui/components/TaStatusChip.qml`

A **readout, not a button**. Kinds: `Neutral` `Success` `Warning` `Error`,
each pulling its own token pair. Height 30, or `touchMinimum` (44) when
`interactive: true`.

**Rules.** A chip must never contradict the control beside it — "Connected"
next to an empty COM-port field was a real UI-0 finding and is prohibited. Set
`interactive` only when tapping actually does something.

### TaWarningBanner — `src/ui/components/TaWarningBanner.qml`

Kinds: `Info` `Warning` `Error`. Tinted background + matching 1 px border +
glyph + wrapping body text at `body` size.

**Rules.** Never smaller than `helperText` (11 px). If it changes what the
operator must do next, it gets a banner — this component exists because
"Changing mode requires an application restart" shipped at ~8 px muted grey.
German wraps freely and the banner grows; it never clips.

---

## Specified — implemented inline on the homepage, not yet extracted

### FormField
`inputBackground` · `radiusSmall` · `borderSubtle` 1 px → focused `accentHover`
2 px · height `controlHeight` (52). Always a `label` micro-label above, and
always a placeholder when empty. *(An unlabelled empty athlete box was a UI-0
finding; the placeholder is now in `LoginPage.qml` and should move here.)*

### SegmentedControl
Two or more equal segments, one selected. Selected: `successBackground` +
`successText` for Live, `errorBackground` + `errorText` for Demo, 2 px border.
Height 52.

**Rule.** Live/Demo is the product's highest-stakes segmented control. Its
colours are **functional** — a brand package may tune the hue but may never
make the two states hard to tell apart.

### EventCard
Height `cardHeight` (78) · `surfaceSecondary` · `radiusMedium` ·
`borderSubtle` 1 px. Selected: `accentSubtle` fill, `accentPrimary` 2 px
border, bold title, filled radio — four simultaneous signals (§13).
Contents: 38 px badge · `cardTitle` · **one** `helperText` subtitle ·
selection indicator.

### SectionPanel
`surfacePrimary` · `radiusLarge` · `borderSubtle` 1 px · `panelPadding` (22).
Optional 3 px `accentPrimary` leading edge, at most one per panel. Owns its own
scroll container when its content can overflow.

### ProgrammeSummary
One card, not a grid of equal-weight tiles. Programme name at `sectionTitle`,
a divider, then a 3-column grid of quiet label/value pairs.

**Rule.** Six equally loud bordered tiles was the UI-0 finding this replaces.
The name must read first.

---

## Design required

### Loading / progress
No pattern exists and none should be invented ad hoc. The application is
currently synchronous on the paths that matter.

### Empty state
Partially specified (§16: state the fact, then the action; desktop wording).
Not yet a component.

---

## Adoption status

| Component | Homepage | Other screens |
|---|---|---|
| `TaButton` | inline equivalent | legacy |
| `TaStatusChip` | inline equivalent | legacy |
| `TaWarningBanner` | inline equivalent | legacy |
| FormField / SegmentedControl / EventCard / SectionPanel / ProgrammeSummary | inline | legacy |

The homepage consumes the **token layer** completely (zero hard-coded colours,
asserted by test). It does not yet consume the extracted **components** —
swapping working, verified markup for new components in the same phase that
introduced them would risk a regression for no user-visible gain. That
migration is the natural first step of the next UI phase.

## Rules for adding a component

1. Take `theme` as a property; never reach for a global.
2. Read tokens only — a hard-coded colour, size or radius is a defect.
3. Support default, hover, pressed, disabled and keyboard focus, plus selected
   where the concept applies.
4. Meet `touchMinimum` (44 px) for anything interactive.
5. Size to content, not to the English string.
6. Add it to the gallery in the same commit — an unpreviewable component
   cannot be reviewed.
7. Never ask which brand is running. Screens read tokens; brands set tokens.
