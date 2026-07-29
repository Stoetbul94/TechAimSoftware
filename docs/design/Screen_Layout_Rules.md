# Tech Aim — Screen Layout Rules

**Version:** 1.0 (UI-1) · Companion to `TechAim_Design_System.md`

Rules for how a screen behaves as the window changes size, and how it survives
German. Written against the Version B homepage; the principles apply to any
screen migrated later.

---

## 1. The three rules that outrank layout tidiness

1. **The primary action is always reachable.** It never lives inside a
   scrolling region, and it is never below the fold.
2. **Nothing clips.** Content that does not fit scrolls, wraps or elides with a
   tooltip. Silently cut-off content is prohibited.
3. **Never shrink text to fit.** Reflow, wrap, or scroll instead. Text that
   shrinks until it fits becomes text nobody can read on a range.

> These exist because both were violated in the shipped build. The UI-0 audit
> found the event panel clipped horizontally at 1536 px — hiding an entire
> discipline — and the Start button pushed out of a clipped anchor chain.

## 2. Supported sizes

| Size | Status | Notes |
|---|---|---|
| 1536 × 960 | primary target | the range laptop in use |
| 1366 × 768 | supported | common projector/tablet resolution |
| 1280 × 720 | supported | lower bound for two columns |
| 1100 × 700 | supported, degraded | below this, single column |

Window chrome consumed by the shell: 40 px `Header` + 20 px margin. A page
therefore has `windowHeight − 60` to work with.

## 3. Panel rules

| Rule | Value |
|---|---:|
| `leftPanelMinimum` | 380 px |
| `rightPanelMinimum` | 460 px |
| `twoColumnMinimum` | 880 px |
| `actionBarHeight` | 88 px |

- The left panel takes **44 %**, the right **56 %**. The event list is the
  denser content and gets the greater width (a Version B principle).
- Below `twoColumnMinimum`, the two-column split stops being viable: the panels
  stack, the event list first. *(Rule defined; single-column stacking is **not
  implemented** in UI-1 — the supported floor of 1100 px stays two-column.)*
- The action bar is **full width and outside both panels**. It is the reason
  the primary action cannot be clipped, and its height is fixed.

## 4. Scroll containers

| Region | Behaviour |
|---|---|
| Left setup column | `Flickable`, `StopAtBounds`, scrollbar only when content overflows |
| Right event list | `ScrollView`, vertical only, horizontal **always off** |
| Action bar | never scrolls |
| Header / footer | never scroll |

Horizontal scrolling is prohibited on the homepage. A horizontal scrollbar is
how the clipped-discipline defect would have hidden itself.

## 5. Card and control minimums

| Element | Minimum |
|---|---:|
| Any interactive target | 44 × 44 |
| Fields, selectors | 52 |
| Primary/secondary buttons | 56 |
| Discipline selector cards | 58 |
| Event/programme cards | 78 |
| Gap between adjacent targets | 8 |

Cards fill the available panel width; they never define their own fixed width.
Titles elide with `ElideRight`; subtitles are limited to one line.

## 6. German text expansion

Allowance: **1.35×** (`Typography.germanExpansionFactor`).

| Element | Rule |
|---|---|
| Micro-labels (`label`) | size to content; never to the English string |
| Buttons | padding-driven width, or elide with a tooltip; never truncate mid-word without one |
| Status chips | may wrap to two lines; must not overlap a neighbour |
| Card titles | elide; the subtitle carries the detail |
| Warning banners | wrap freely; height grows |

Worked examples:

| English | German | Ratio |
|---|---|---:|
| Mode | Betriebsart | 2.75× |
| Share | Netzwerkfreigabe | 3.2× |
| Start session | Sitzung starten | 1.2× |
| Load saved session | Gespeicherte Sitzung laden | 1.4× |
| Connected | Verbunden | 1.0× |

The two worst offenders are micro-labels, which is why they must size to
content. `Betriebsart` and `Netzwerkfreigabe` are the sampled states to check.

**Status: NOT YET VERIFIED IN THE RUNNING APPLICATION.** German catalogue
switching plus visual inspection of the sampled states is
**HUMAN VISUAL CHECK REQUIRED**.

## 7. Verification matrix

| Check | 1536×960 | 1366×768 | 1280×720 | 1100×700 |
|---|---|---|---|---|
| Primary action visible | ✅ by construction¹ | ✅ by construction¹ | ✅ by construction¹ | ✅ by construction¹ |
| Event panel unclipped | ⚠️ | ⚠️ | ⚠️ | ⚠️ |
| No horizontal scrollbar | ⚠️ | ⚠️ | ⚠️ | ⚠️ |
| Setup column scrolls | ✅ by construction² | ✅ by construction² | ✅ by construction² | ✅ by construction² |
| German labels do not overlap | ⚠️ | ⚠️ | ⚠️ | ⚠️ |

¹ The action bar is anchored to the content area's bottom, outside every
scroll container. Asserted structurally in `tst_brandpackage.cpp`
("the Start action sits INSIDE the bottom action bar").
² The setup column is a `Flickable` with `contentHeight` bound to its content.

⚠️ = **HUMAN VISUAL CHECK REQUIRED.** These cannot be proven from source, and
the automated capture route is blocked on this machine (see the UI-0 audit:
endpoint security blocks both input injection and the screenshot helper).

## 8. What is explicitly NOT implemented

- Single-column stacking below 880 px (rule defined, not built).
- Loading/skeleton states (§15 of the design system — **DESIGN REQUIRED**).
- Migration of screens other than the homepage.
