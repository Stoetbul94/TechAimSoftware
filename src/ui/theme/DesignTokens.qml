import QtQuick 2.15

// Tech Aim Design System — semantic colour tokens (UI-1, UI-THEME-001).
//
// This is the ONE place an application colour is defined. Screens reference
// tokens by MEANING (`surfacePrimary`, `textSecondary`), never by appearance
// (`grey7`, `red1`), so a brand package can move a value without every screen
// having to agree about what "red1" was for.
//
// Instantiated by Theme.qml and reached through ancestor scope as
// `theme.tokens.<name>`. That mirrors the pattern Theme.qml already documents
// and that main.qml already relies on — deliberately not a singleton, because
// no singleton/module mechanism exists anywhere else in this codebase.
//
// ── UI-THEME-001: LIGHT / DARK ──────────────────────────────────────────────
// Every token below resolves through `isLight`. Because consumers read
// `theme.tokens.*` as QML bindings, changing `appearance` re-evaluates the
// whole product live — no restart, and no screen has to know a theme exists.
// That is the entire reason the token layer was built; this change uses it
// rather than editing colours screen by screen.
//
// A screen that still hard-codes a hex literal does NOT follow the theme.
// Those are listed in docs/design/current-design-audit.md and are migrated by
// replacing the literal with a token, not by adding a second theme mechanism.
//
// Appearance is PRESENTATION ONLY. Nothing here may influence acquisition,
// scoring, timing, competition rules, session state or reports.
//
// See docs/design/TechAim_Design_System.md for the full rules.

QtObject {

    // ── APPEARANCE ──────────────────────────────────────────────────────────
    // "system" | "light" | "dark". Bound by Theme.qml to APPSETTINGS.
    // Default "dark" so existing operators keep what they have.
    property string appearance: "dark"

    // Qt::ColorScheme — Unknown = 0, Light = 1, Dark = 2. Unknown (a platform
    // that does not report a preference) is treated as dark, which is this
    // product's established appearance.
    readonly property bool systemIsLight:
        Application.styleHints.colorScheme === Qt.ColorScheme.Light

    readonly property bool isLight: appearance === "light"
                                    || (appearance === "system" && systemIsLight)

    // ── BRAND ACCENT ────────────────────────────────────────────────────────
    // APPROVED 2026-07-29. accentPrimary is sampled from the approved logo
    // asset images/logo/techaim_color.png, where #A80038 accounts for 276,718
    // of 710,403 opaque pixels. It also matches Theme.brandPrimary and the
    // entire report/PDF system, which was already the largest consumer.
    //
    // The interaction states are defined HERE rather than by reaching for
    // whichever red a screen happened to have: the product previously shipped
    // three competing brand reds (#a80038 / #e8003d / #C40046).
    //
    // The accent itself does NOT change between themes — it is the brand, and
    // #A80038 carries white text at 8.9:1 on either canvas. Only the tinted
    // selection fill flips, because a near-black tint is invisible on white.
    // The accent is the BRAND, so it is read from the product's brand package
    // rather than written here. A screen asks for a semantic token and gets
    // whichever accent this product uses; a second product line is a package,
    // not a fork. Every package states a textOnAccent contrast-checked against
    // its own accentPrimary.
    readonly property color accentPrimary: PRODUCT.accentPrimary   // rest state
    readonly property color accentHover:   PRODUCT.accentHover     // hover / focus lift
    readonly property color accentPressed: PRODUCT.accentPressed   // active / pressed
    readonly property color accentSubtle:  isLight ? PRODUCT.accentSubtleLight
                                                   : PRODUCT.accentSubtle
    readonly property color textOnAccent:  PRODUCT.textOnAccent    // the only text colour permitted on an accent fill

    // Logo-intrinsic only. This is the tagline red inside the approved
    // artwork. It is NOT an application accent and must not be used as one.
    readonly property color brandLogoSecondary: PRODUCT.brandLogoSecondary

    // ── SURFACES ────────────────────────────────────────────────────────────
    // Light keeps the same three-step depth order as dark (canvas → card →
    // nested), so a screen's structure reads identically in both.
    readonly property color backgroundPrimary:   isLight ? "#F4F6F8" : "#0B0D10"  // the window canvas
    readonly property color backgroundSecondary: isLight ? "#FFFFFF" : "#0C0E12"  // app bar / header strip
    readonly property color surfacePrimary:      isLight ? "#FFFFFF" : "#15171C"  // panels and cards
    readonly property color surfaceSecondary:    isLight ? "#F7F9FB" : "#1B1E24"  // nested cards, popovers
    readonly property color surfaceElevated:     isLight ? "#EDF1F5" : "#1F2026"  // hover surface, raised rows
    readonly property color inputBackground:     isLight ? "#FFFFFF" : "#1D2026"  // text fields and pickers

    // ── BORDERS ─────────────────────────────────────────────────────────────
    // On light these must be dark enough to draw a real edge against white —
    // a border that only just differs from its surface reads as no border.
    readonly property color borderSubtle: isLight ? "#D5DBE2" : "#2A2E36"  // default card / field outline
    readonly property color borderStrong: isLight ? "#AEB8C4" : "#3A404A"  // secondary buttons, dividers that must read

    // ── TEXT ────────────────────────────────────────────────────────────────
    // The audit found five near-identical greys doing this one job
    // (#9aa0aa / #8a8f98 / #9a9ba0 / #8a8a92 / #6b7280, ~86 occurrences).
    // They collapse to two here plus a disabled step.
    //
    // Light contrast against surfacePrimary (#FFFFFF):
    //   textPrimary   #12161B  ~17.0:1
    //   textSecondary #4C5765   ~8.2:1
    //   textDisabled  #6E7885   ~4.6:1  — deliberately still legible; a
    //                                     disabled control must be readable,
    //                                     only clearly not actionable.
    readonly property color textPrimary:   isLight ? "#12161B" : "#F3F6FA"  // headings and primary content
    readonly property color textSecondary: isLight ? "#4C5765" : "#AAB4C0"  // labels, captions, supporting copy
    readonly property color textDisabled:  isLight ? "#6E7885" : "#6F7A86"  // disabled controls, micro-labels

    // ── STATUS ──────────────────────────────────────────────────────────────
    // FUNCTIONAL, not decorative. A brand package may tune the hue but may not
    // swap the meanings or make the three states hard to tell apart: in this
    // product they carry connection state and Live-vs-Demo, and Live/Demo
    // confusion is a result-integrity risk, not a styling preference.
    //
    // The four hues (green / amber / red / blue) and their meanings are
    // IDENTICAL in both themes. Only the lightness flips: on dark a deep tint
    // behind a bright glyph, on light a pale tint behind a deep glyph.
    readonly property color successBackground: isLight ? "#E4F7EF" : "#0D2018"
    readonly property color successText:       isLight ? "#0E7C57" : "#20C997"
    readonly property color warningBackground: isLight ? "#FDF3E2" : "#2A1A05"
    readonly property color warningText:       isLight ? "#8A5A10" : "#E8A13D"
    readonly property color errorBackground:   isLight ? "#FDEAEA" : "#2A0B10"
    readonly property color errorText:         isLight ? "#B3261E" : "#D0392B"
    readonly property color infoBackground:    isLight ? "#E8F0FC" : "#0C1A2E"
    readonly property color infoText:          isLight ? "#1B4F9C" : "#2F6FD0"

    // ── FOCUS ───────────────────────────────────────────────────────────────
    // Keyboard focus must be visible against every surface above. accentHover
    // is used rather than accentPrimary because it is the lighter of the two
    // and stays legible on the darkest canvas; on light it stays legible
    // because it is a saturated red against a near-white field.
    readonly property color focusOutline: PRODUCT.focusOutline
    readonly property int   focusOutlineWidth: 2

    // ── OVERLAY ─────────────────────────────────────────────────────────────
    // Dim behind a modal. Alpha-premultiplied ARGB; deep enough that the
    // dialog is unambiguously the only thing actionable. Lighter on light,
    // where a near-opaque black scrim reads as a fault rather than a dim.
    readonly property color scrim: isLight ? "#66101418" : "#AA000000"

    // ── OPACITY ─────────────────────────────────────────────────────────────
    // Higher on light: the same 0.40 that reads as "dimmed" against a dark
    // canvas reads as "broken" against white.
    readonly property real disabledOpacity: isLight ? 0.55 : 0.40
    readonly property real mutedOpacity:    isLight ? 0.72 : 0.65
}
