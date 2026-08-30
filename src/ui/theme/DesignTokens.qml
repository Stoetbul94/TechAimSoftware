import QtQuick 2.15

// Tech Aim Design System — semantic colour tokens (UI-1).
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
// See docs/design/TechAim_Design_System.md for the full rules, and
// docs/design/current-design-audit.md for the measured evidence behind these
// values (240 distinct literals across 66 QML files before this existed).

QtObject {

    // ── BRAND ACCENT ────────────────────────────────────────────────────────
    // The values live in the BUILD's BrandPackage (src/app/BrandPackage.cpp)
    // and arrive through PRODUCT. This file stays the semantic layer: a screen
    // asks for `accentPrimary` and gets whichever accent this product uses,
    // and never has to know which brand it is drawing.
    //
    //   TECH_AIM  #A80038  sampled from images/logo/techaim_color.png
    //   SETA_OEM  #00539E  sampled from images/logo/seta.png
    //
    // Both carry white text at ~7.7:1, so the two are functionally
    // interchangeable and no component needs a per-brand branch. The interaction
    // states are defined ONCE, in the package, rather than by reaching for
    // whichever accent a screen happened to have: the product previously
    // shipped three competing brand reds (#a80038 / #e8003d / #C40046).
    readonly property color accentPrimary: PRODUCT.accentPrimary   // rest state
    readonly property color accentHover:   PRODUCT.accentHover     // hover / focus lift
    readonly property color accentPressed: PRODUCT.accentPressed   // active / pressed
    readonly property color accentSubtle:  PRODUCT.accentSubtle    // tinted fill behind a selected card
    readonly property color textOnAccent:  PRODUCT.textOnAccent    // the only text colour permitted on an accent fill

    // The BRIGHTER brand tone the live shooting UI and the Training Lab HUDs
    // already used (#E8003D for Tech Aim). Naming it here is what lets a brand
    // package recolour those screens without every HUD owning a literal.
    readonly property color accentBright:  PRODUCT.accentBright

    // Logo-intrinsic only. The colour inside the approved artwork that is NOT
    // an application accent and must not be used as one (Tech Aim: the tagline
    // red; SETA: the logo navy).
    readonly property color brandLogoSecondary: PRODUCT.brandLogoSecondary

    // ── SURFACES ────────────────────────────────────────────────────────────
    readonly property color backgroundPrimary:   "#0B0D10"  // the window canvas
    readonly property color backgroundSecondary: "#0C0E12"  // app bar / header strip
    readonly property color surfacePrimary:      "#15171C"  // panels and cards
    readonly property color surfaceSecondary:    "#1B1E24"  // nested cards, popovers
    readonly property color surfaceElevated:     "#1F2026"  // hover surface, raised rows
    readonly property color inputBackground:     "#1D2026"  // text fields and pickers

    // ── BORDERS ─────────────────────────────────────────────────────────────
    readonly property color borderSubtle: "#2A2E36"  // default card / field outline
    readonly property color borderStrong: "#3A404A"  // secondary buttons, dividers that must read

    // ── TEXT ────────────────────────────────────────────────────────────────
    // The audit found five near-identical greys doing this one job
    // (#9aa0aa / #8a8f98 / #9a9ba0 / #8a8a92 / #6b7280, ~86 occurrences).
    // They collapse to two here plus a disabled step.
    readonly property color textPrimary:   "#F3F6FA"  // headings and primary content
    readonly property color textSecondary: "#AAB4C0"  // labels, captions, supporting copy
    readonly property color textDisabled:  "#6F7A86"  // disabled controls, micro-labels

    // ── STATUS ──────────────────────────────────────────────────────────────
    // FUNCTIONAL, not decorative. A brand package may tune the hue but may not
    // swap the meanings or make the three states hard to tell apart: in this
    // product they carry connection state and Live-vs-Demo, and Live/Demo
    // confusion is a result-integrity risk, not a styling preference.
    readonly property color successBackground: "#0D2018"
    readonly property color successText:       "#20C997"
    readonly property color warningBackground: "#2A1A05"
    readonly property color warningText:       "#E8A13D"
    readonly property color errorBackground:   "#2A0B10"
    readonly property color errorText:         "#D0392B"
    readonly property color infoBackground:    "#0C1A2E"
    readonly property color infoText:          "#2F6FD0"

    // ── FOCUS ───────────────────────────────────────────────────────────────
    // Keyboard focus must be visible against every surface above. accentHover
    // is used rather than accentPrimary because it is the lighter of the two
    // and stays legible on the darkest canvas. The package decides which
    // colour that is (Tech Aim #C40046 at 3.18:1; SETA #25B0E6 at 7.81:1).
    readonly property color focusOutline: PRODUCT.focusOutline
    readonly property int   focusOutlineWidth: 2

    // ── OVERLAY ─────────────────────────────────────────────────────────────
    // Dim behind a modal. Alpha-premultiplied ARGB; deep enough that the
    // dialog is unambiguously the only thing actionable.
    readonly property color scrim: "#AA000000"

    // ── OPACITY ─────────────────────────────────────────────────────────────
    readonly property real disabledOpacity: 0.40
    readonly property real mutedOpacity:    0.65
}
