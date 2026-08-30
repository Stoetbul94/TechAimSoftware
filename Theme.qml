import QtQuick 2.4
import "qrc:/src/ui/theme"

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

    // ══ UI-1 DESIGN SYSTEM ══════════════════════════════════════════════════
    // The semantic token layer. New and refactored screens read
    // `theme.tokens.*`, `theme.type.*` and `theme.space.*`; the legacy
    // properties further down are retained unchanged so the ~20 screens that
    // have not been migrated yet keep working exactly as before.
    //
    // Full rules: docs/design/TechAim_Design_System.md
    readonly property DesignTokens tokens: DesignTokens { }
    readonly property Typography   type:   Typography   { }
    readonly property Spacing      space:  Spacing      { }

    // --- Brand colors --------------------------------------------------------
    // These are the LEGACY aliases the ~20 unmigrated screens still use. They
    // now resolve to the token layer, so a brand package moves them too and a
    // screen cannot end up half-red / half-blue depending on which era of the
    // codebase it was written in.
    readonly property color brandPrimary: tokens.accentPrimary
    readonly property color brandAccent:  tokens.brandLogoSecondary
    readonly property color brandDark:    "#111111"   // near-black - wordmark
    readonly property color brandLight:   "#fbf9fa"   // off-white - light backgrounds

    // --- Logo assets (pick the variant matching the background you're placing it on) ---
    readonly property string logoWhite: PRODUCT.brandLogoOnDarkPath // for dark backgrounds
    readonly property string logoColor: PRODUCT.brandLogoPath // the product brand mark; identity owns it
    readonly property string logoBlack: "qrc:/images/logo/techaim_black.png" // Tech Aim single-ink mark; SETA has no monochrome variant (reported by BrandPackage)

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
    // SEMANTIC, not brand: a fault must read as a fault in every product, so
    // this is the error token and NOT the brand accent it used to alias.
    readonly property color statusError:        tokens.errorText

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
