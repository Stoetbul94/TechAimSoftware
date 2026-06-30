import QtQuick 2.4

// Shared TechAim brand/theme constants.
//
// Colors below are extracted directly from the provided logo SVG files
// (fill="#..." values), not eyeballed from a rendered preview - they are
// exact matches to the brand assets in images/logo/.
//
// Usage: instantiated once in main.qml as `Theme { id: theme }`. Because
// main.qml's root Item already successfully exposes simple properties
// (isDefaultIcon, gameRange, gameMode, etc.) to every descendant page via
// QML's ancestor-scope property lookup with zero import statements anywhere
// - this file follows that same proven pattern instead of introducing a new
// singleton/module mechanism that hasn't been used anywhere else in this
// codebase and can't be compile-tested in this environment.
//
// Every consuming page should reference colors/fonts via `theme.xxx`
// rather than hardcoding hex values again - that duplication (every one of
// the 22 UI files currently defines its own colors independently, with no
// shared source of truth) is exactly what makes brand/style changes
// error-prone today.

QtObject {
    // --- Brand colors (exact, from logo SVGs) ---
    readonly property color brandPrimary: "#a80038"   // deep crimson - target rings, dominant accent
    readonly property color brandAccent:  "#bf1919"   // red - tagline, secondary accent / alerts
    readonly property color brandDark:    "#111111"   // near-black - wordmark
    readonly property color brandLight:   "#fbf9fa"   // off-white - light backgrounds

    // --- Logo assets (pick the variant matching the background you're placing it on) ---
    readonly property string logoWhite: "qrc:/images/logo/techaim_white.png"  // for dark backgrounds
    readonly property string logoColor: "qrc:/images/logo/techaim_color.png" // for light/neutral backgrounds
    readonly property string logoBlack: "qrc:/images/logo/techaim_black.png" // for light backgrounds, monochrome contexts (e.g. print/PDF)

    // --- Dark UI neutral scale (chrome: backgrounds, surfaces, borders) ---
    // Built around brandPrimary as the accent, not copied from any one
    // reference mockup's exact shade.
    readonly property color bgBase:      "#0d0d0f"  // main window/canvas background
    readonly property color bgSurface:   "#18181b"  // cards, panels - one step lighter than bgBase
    readonly property color bgSurfaceAlt: "#1f1f23" // hover/alt surface state
    readonly property color borderColor: "#2a2a2e"  // subtle dividers/card borders

    // --- Text ---
    readonly property color textPrimary:   "#f5f5f5"  // headings, primary content
    readonly property color textSecondary: "#9a9a9e"  // labels, captions, muted content
    readonly property color textOnBrand:   "#ffffff"  // text placed on top of brandPrimary/brandAccent fills

    // --- Status (kept distinct from target scoring-zone colors, which are a
    // separate, function-specific system already configurable via
    // APPSETTINGS - these are for UI chrome only: connection state, etc.) ---
    readonly property color statusConnected:    "#2e9e5b"
    readonly property color statusDisconnected: "#9a9a9e"
    readonly property color statusError:        brandAccent

    // --- Typography ---
    // "Luxi Mono" (used in the current Header.qml) is a dated monospace
    // choice for UI chrome. Segoe UI is the standard Windows system font
    // (this project's .pro file targets Windows SDK paths) with broadly
    // available fallbacks for other platforms.
    readonly property string fontFamily: "Segoe UI"

    // --- Corner radius / spacing conventions, for the card-based look ---
    readonly property int radiusSmall:  4
    readonly property int radiusMedium: 8
    readonly property int radiusLarge:  12
    readonly property int spacingUnit:  8   // base spacing unit; use multiples (2x, 3x) for consistency
}
