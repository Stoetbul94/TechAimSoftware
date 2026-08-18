#pragma once

// Tech Aim — BrandPackage (UI-1).
//
// ProductIdentity answers "what is this product CALLED"; BrandPackage answers
// "what does it LOOK like". Splitting them is what makes a future OEM edition
// a configuration choice instead of a source fork: the OEM build uses the same
// controllers, the same ISSF scoring, the same reliability layer and the same
// discipline set, and differs only in the values below.
//
// HARD BOUNDARY — a brand package controls PRESENTATION AND IDENTITY ONLY.
// It must never influence:
//   * scoring or ring geometry
//   * competition rules or discipline availability
//   * Training Lab analytics
//   * session recovery or the journal
//   * target communication
//   * interpretation of any recorded data
//
// If a brand value would change what a score IS, it does not belong here.
// Tests assert this boundary: switching the package must change presentation
// and nothing else (see tests/reliability/tst_brandpackage.cpp).
//
// See docs/design/Brand_Flavor_Guide.md for how a new package is introduced.

#include <QString>
#include <QStringList>

namespace ta {
namespace app {

enum class BuildFlavour;   // declared in ProductIdentity.h

struct BrandPackage {
    // ── which flavour this package belongs to ────────────────────────────
    QString flavourKey;             // "TECH_AIM" — matches flavourName()

    // ── naming (mirrors ProductIdentity; kept here so a package is a
    //    complete, reviewable unit for brand approval) ────────────────────
    QString productName;            // "Tech Aim Electronic Target Control"
    QString shortProductName;       // "Tech Aim"
    QString publisher;              // "JAC SHOOTING SOLUTIONS (PTY) LTD"

    // ── artwork. Empty string == NOT SUPPLIED == brand approval required.
    //    An absent asset is reported, never invented or derived. ──────────
    QString logoColour;             // primary mark, for light/neutral backgrounds
    QString logoWhite;              // for dark application surfaces
    QString logoMonochrome;         // single-ink contexts
    QString reportLogo;             // the mark used on printed reports and PDFs
    QString windowsIcon;            // .ico embedded in the executable

    // ── colour. These are the ONLY colours a package may set; everything
    //    else derives from them or stays common across flavours. ──────────
    QString accentPrimary;          // rest state
    QString accentHover;            // hover / focus
    QString accentPressed;          // active
    QString accentSubtle;           // tinted fill behind a selected item
    // The BRIGHT accent the live shooting UI and the Training Lab HUDs use.
    // It exists because the product shipped a second, lighter brand tone
    // alongside accentPrimary; naming it here is what lets a package recolour
    // those screens without every HUD carrying its own literal.
    QString accentBright;
    // The only text colour permitted on an accent fill. A package must set it
    // because it depends on how dark that package's accent is.
    QString textOnAccent;
    // Keyboard-focus ring. Must stay legible on the DARKEST canvas, which is
    // why it is the lighter of the two accents, not accentPrimary.
    QString focusOutline;

    // Colour that is intrinsic to the artwork and must NOT be promoted to a
    // general application accent (Tech Aim: the logo's tagline red).
    QString logoIntrinsicColour;

    // ── attribution + resources ──────────────────────────────────────────
    QString resourceNamespace;      // "qrc:/assets/brands/techaim"
    QString pdfAttribution;         // printed in report footers
    QString manualBrandName;        // used on manual covers

    // Only authorised where a package is genuinely region-locked; otherwise
    // empty and the user's choice wins. Never a silent language override.
    QString defaultLanguage;

    // ── validation ───────────────────────────────────────────────────────
    // Assets this package declares but does not supply. A non-empty list is
    // not a failure at runtime — it is the honest answer to "is this brand
    // ready to ship", and the OEM guide requires it to be empty before an
    // OEM edition may be built.
    QStringList missingAssets() const;

    // True when every declared asset exists and every accent is set.
    bool isComplete() const;
};

// The package for the flavour this binary was built with.
const BrandPackage& brand();

// Look up a package explicitly. Used by tests to prove that swapping the
// package changes presentation without touching programme behaviour.
const BrandPackage& brandFor(BuildFlavour f);

} // namespace app
} // namespace ta
