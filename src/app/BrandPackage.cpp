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

    // The NAME comes from the one identity source, so the package can never
    // disagree with the product it brands. On a SETA build these become the
    // SETA names; the colour system below stays Tech Aim's approved one,
    // because no SETA palette has been supplied and inventing one is a brand
    // act, not an engineering one.
    b.productName      = identity().fullProductName;
    b.shortProductName = identity().displayName;
    b.publisher        = identity().legalPublisher;

    b.logoColour       = identity().brandLogoPath;
    b.logoWhite        = identity().brandLogoOnDarkPath;
    // No dedicated monochrome mark exists. techaim_black.png is the
    // single-ink variant actually shipped and is what the report/PDF path
    // already uses, so it is declared honestly rather than left empty.
    b.logoMonochrome   = QStringLiteral("qrc:/images/logo/techaim_black.png");
    b.reportLogo       = identity().brandLogoPath;
#ifdef BRAND_SETA
    // SETA supplied ONE mark (seta.png). A white-on-dark and a single-ink
    // variant do not exist, so they are reported as missing rather than
    // silently falling back to the Tech Aim logo, which would put the wrong
    // company's mark on a SETA report.
    b.logoWhite        = QString();
    b.logoMonochrome   = QString();
#endif
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

    b.resourceNamespace = QStringLiteral("qrc:/images/logo");
    b.pdfAttribution    = identity().fullProductName;
    b.manualBrandName   = identity().displayName;
    b.defaultLanguage   = QString();   // the operator's choice wins
    return b;
}

// ── SETA OEM ────────────────────────────────────────────────────────────────
// RESERVED AND DELIBERATELY EMPTY.
//
// This exists so the OEM path is a real, testable configuration seam rather
// than a hypothetical one — a test can ask for this package and assert that it
// reports its missing assets instead of silently falling back to Tech Aim
// artwork. Nothing here is approved, and no SETA appearance is implemented.
//
// Note also: "SETA" is the German electronics supplier as well as the legacy
// OEM brand. Nothing in this file may be used to justify a blanket rename of
// SETA references elsewhere in the source.
BrandPackage makeSetaOem()
{
    BrandPackage b;
    b.flavourKey       = QStringLiteral("SETA_OEM");
    b.productName      = QString();
    b.shortProductName = QString();
    b.publisher        = QString();
    // Every asset intentionally absent — BRAND APPROVAL REQUIRED.
    b.accentPrimary    = QString();
    b.resourceNamespace = QStringLiteral("qrc:/assets/brands/seta-oem");
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
    static const BrandPackage setaOem = makeSetaOem();
    return (f == BuildFlavour::SetaOem) ? setaOem : techAim;
}

const BrandPackage& brand()
{
    return brandFor(currentFlavour());
}

} // namespace app
} // namespace ta
