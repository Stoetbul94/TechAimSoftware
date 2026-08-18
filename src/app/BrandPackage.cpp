#include "app/BrandPackage.h"
#include "app/ProductIdentity.h"

#include <QFile>

namespace ta {
namespace app {

namespace {

// ── Tech Aim ────────────────────────────────────────────────────────────────
// APPROVED 2026-07-29.
//
// The accent is sampled from the approved logo asset
// images/logo/techaim_color.png, in which #A80038 accounts for 276,718 of
// 710,403 opaque pixels. Before this decision the product shipped three
// competing brand reds (#a80038 in the report system, #e8003d in the live
// shooting UI, #C40046 in Training Lab and the homepage); #C40046 and #80032A
// are now the interaction states of the one approved accent rather than
// separate brands. See docs/design/current-design-audit.md §2.
BrandPackage makeTechAim()
{
    BrandPackage b;
    b.flavourKey       = QStringLiteral("TECH_AIM");

    b.productName      = QStringLiteral("Tech Aim Electronic Target Control");
    b.shortProductName = QStringLiteral("Tech Aim");
    b.publisher        = QStringLiteral("JAC SHOOTING SOLUTIONS (PTY) LTD");

    b.logoColour       = QStringLiteral("qrc:/images/logo/techaim_color.png");
    b.logoWhite        = QStringLiteral("qrc:/images/logo/techaim_white.png");
    // No dedicated monochrome mark exists. techaim_black.png is the
    // single-ink variant actually shipped and is what the report/PDF path
    // already uses, so it is declared honestly rather than left empty.
    b.logoMonochrome   = QStringLiteral("qrc:/images/logo/techaim_black.png");
    b.reportLogo       = QStringLiteral("qrc:/images/logo/techaim_color.png");
    // BRAND APPROVAL REQUIRED: no .ico exists and TechAim.rc declares no ICON
    // resource, so the executable carries the default Qt/MinGW icon. Deriving
    // one from the raster logo is a brand act and is deliberately NOT done
    // here. Reported by missingAssets(), never invented.
    b.windowsIcon      = QString();

    b.accentPrimary       = QStringLiteral("#A80038");
    b.accentHover         = QStringLiteral("#C40046");
    b.accentPressed       = QStringLiteral("#80032A");
    b.accentSubtle        = QStringLiteral("#2D0A18");
    b.logoIntrinsicColour = QStringLiteral("#BF1919");   // logo tagline only
    // The lighter brand tone the live shooting UI and the HUDs already used.
    b.accentBright        = QStringLiteral("#E8003D");
    b.textOnAccent        = QStringLiteral("#FFFFFF");   // 7.71:1 on #A80038
    b.focusOutline        = b.accentHover;

    b.resourceNamespace = QStringLiteral("qrc:/images/logo");
    b.pdfAttribution    = QStringLiteral("Tech Aim Electronic Target Control");
    b.manualBrandName   = QStringLiteral("Tech Aim");
    b.defaultLanguage   = QString();   // the operator's choice wins
    return b;
}

// ── SETA ────────────────────────────────────────────────────────────────────
// APPROVED. The SETA product line is a BLUE theme, and the palette below is
// sampled from the approved asset images/logo/seta.png, exactly as the Tech Aim
// accent was sampled from techaim_color.png. That image contains three opaque
// colours and no others:
//
//   #25B0E6   13,307 px  73.28%   the wordmark / swoosh
//   #212D60    3,786 px  20.85%   the deep navy
//   #00539E    1,066 px   5.87%   the saturated brand blue
//
// accentPrimary is #00539E rather than the most numerous colour, because an
// accent is a FILL THAT CARRIES WHITE TEXT and #25B0E6 cannot: white on
// #25B0E6 is 2.49:1, which fails at any size. White on #00539E is 7.69:1 -
// within 0.02 of Tech Aim's own 7.71:1 on #A80038, so the two products'
// accents are functionally interchangeable and no component needs to know
// which brand it is drawing.
//
// #25B0E6 becomes the LIGHTER interaction state and the focus ring, mirroring
// how #C40046 relates to #A80038. It reads 7.81:1 on the darkest canvas
// (Tech Aim's focus ring manages 3.18:1), so SETA's focus visibility is better,
// not merely different.
//
// accentPressed is accentPrimary x 0.70 - the same derivation that produced
// Tech Aim's #80032A from #A80038. accentSubtle is 28% accentPrimary over
// surfacePrimary #15171C, the same role #2D0A18 plays for Tech Aim.
//
// NOTHING HERE IS INVENTED: every hue comes from the supplied artwork, and the
// two derived values are stated with their derivation.
BrandPackage makeSeta()
{
    BrandPackage b;
    b.flavourKey       = QStringLiteral("SETA_OEM");

    b.productName      = QStringLiteral("SETA Electronic Target Control");
    b.shortProductName = QStringLiteral("SETA");
    // The publisher is a LEGAL fact and is NOT part of the skin.
    b.publisher        = QStringLiteral("JAC SHOOTING SOLUTIONS (PTY) LTD");

    b.logoColour       = QStringLiteral("qrc:/images/logo/seta.png");
    b.reportLogo       = QStringLiteral("qrc:/images/logo/seta.png");
    // SETA supplied ONE mark. A white-on-dark variant, a single-ink variant and
    // a Windows icon do not exist, so they are REPORTED as missing rather than
    // falling back to the Tech Aim logo - that would put another company's mark
    // on a SETA header and a SETA report.
    b.logoWhite        = QString();
    b.logoMonochrome   = QString();
    b.windowsIcon      = QString();

    b.accentPrimary       = QStringLiteral("#00539E");   // logo, white-on 7.69:1
    b.accentHover         = QStringLiteral("#25B0E6");   // logo, dominant tone
    b.accentPressed       = QStringLiteral("#003A6E");   // accentPrimary x 0.70
    b.accentSubtle        = QStringLiteral("#0F2740");   // 28% over #15171C
    b.accentBright        = QStringLiteral("#25B0E6");
    b.textOnAccent        = QStringLiteral("#FFFFFF");
    b.focusOutline        = b.accentHover;
    // The logo navy. Intrinsic to the artwork - NOT an application accent.
    b.logoIntrinsicColour = QStringLiteral("#212D60");

    b.resourceNamespace = QStringLiteral("qrc:/images/logo");
    b.pdfAttribution    = QStringLiteral("SETA Electronic Target Control");
    b.manualBrandName   = QStringLiteral("SETA");
    b.defaultLanguage   = QString();   // the operator's choice wins
    return b;
}

} // namespace

QStringList BrandPackage::missingAssets() const
{
    QStringList missing;

    // "Missing" means NOT SUPPLIED — the brand has not been given this asset
    // and approval is outstanding.
    //
    // It deliberately does NOT probe the filesystem for a declared qrc path.
    // A qrc path is already guaranteed by the build: rcc fails the build if a
    // listed file is absent. Probing it here would also give a false negative
    // in any QtCore-only context that does not link the resource bundles — for
    // example the reliability harness, which is deliberately GUI-free. Physical
    // existence of the artwork is asserted on disk by
    // tests/reliability/tst_brandpackage.cpp instead.
    const auto needAsset = [&missing](const QString& path, const char* label) {
        if (path.isEmpty())
            missing << QString::fromLatin1(label) + QStringLiteral(" (not supplied)");
    };

    needAsset(logoColour,     "logoColour");
    needAsset(logoWhite,      "logoWhite");
    needAsset(logoMonochrome, "logoMonochrome");
    needAsset(reportLogo,     "reportLogo");
    needAsset(windowsIcon,    "windowsIcon");

    if (accentPrimary.isEmpty()) missing << QStringLiteral("accentPrimary (not supplied)");
    if (accentHover.isEmpty())   missing << QStringLiteral("accentHover (not supplied)");
    if (accentPressed.isEmpty()) missing << QStringLiteral("accentPressed (not supplied)");
    if (productName.isEmpty())   missing << QStringLiteral("productName (not supplied)");
    if (publisher.isEmpty())     missing << QStringLiteral("publisher (not supplied)");

    return missing;
}

bool BrandPackage::isComplete() const
{
    return missingAssets().isEmpty();
}

const BrandPackage& brandFor(BuildFlavour f)
{
    static const BrandPackage techAim = makeTechAim();
    static const BrandPackage setaOem = makeSeta();
    return (f == BuildFlavour::SetaOem) ? setaOem : techAim;
}

const BrandPackage& brand()
{
    return brandFor(currentFlavour());
}

} // namespace app
} // namespace ta
