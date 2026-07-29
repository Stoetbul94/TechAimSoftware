import QtQuick 2.15

// Tech Aim Design System — spacing, radius, border and control-size scale (UI-1).
//
// The audit found margins clustering on 8/10/12/14/16/20/22 — close to a 4 px
// grid but never actually on one — and 12 distinct radius values. The scale
// below keeps the values the application already leans on hardest so the
// homepage refactor is not a visual rewrite, while removing the one-offs.
//
// Reached as `theme.space.<name>`.

QtObject {

    // ── SPACING SCALE (4 px grid) ───────────────────────────────────────────
    readonly property int spacing2:  2    // hairline gaps inside a chip
    readonly property int spacing4:  4    // icon-to-label
    readonly property int spacing8:  8    // between related controls
    readonly property int spacing12: 12   // between fields
    readonly property int spacing16: 16   // between sections
    readonly property int spacing24: 24   // panel padding, major separation
    readonly property int spacing32: 32   // page-level separation

    // Panel inset. 22 px was the single most common margin in the audit (37
    // occurrences) and is kept deliberately so the refactor does not shift
    // every panel by 2 px for the sake of grid purity.
    readonly property int panelPadding: 22

    // ── RADIUS ──────────────────────────────────────────────────────────────
    readonly property int radiusSmall:  6    // chips, inputs, small controls
    readonly property int radiusMedium: 8    // buttons, cards
    readonly property int radiusLarge:  10   // panels
    readonly property int radiusPill:   999  // fully rounded — toggles, badges

    // ── BORDERS ─────────────────────────────────────────────────────────────
    // border.width was already consistent in the audit (1 ×171, 2 ×14) and
    // needed only naming. The 2 px weight CARRIES STATE — it is how a selected
    // card is distinguished — so it must not be used decoratively.
    readonly property int borderThin:     1
    readonly property int borderSelected: 2

    // ── TOUCH TARGETS ───────────────────────────────────────────────────────
    // Tech Aim runs on range tablets. 44 px is the floor; controls that are the
    // primary thing on a screen get more.
    readonly property int touchMinimum:       44   // absolute floor, any interactive element
    readonly property int controlHeight:      52   // fields, selectors, secondary buttons
    readonly property int controlHeightLarge: 56   // primary actions, discipline selectors
    readonly property int cardHeight:         78   // event / programme cards

    // ── LAYOUT ──────────────────────────────────────────────────────────────
    // Below this width the two-column homepage stops being two columns. See
    // docs/design/Screen_Layout_Rules.md.
    readonly property int leftPanelMinimum:  380
    readonly property int rightPanelMinimum: 460
    readonly property int twoColumnMinimum:  880
    readonly property int actionBarHeight:   88
}
