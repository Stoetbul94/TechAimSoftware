#include "ProductIdentity.h"

namespace ta {
namespace app {

QString ProductIdentity::softwareVersionLabel() const
{
    return displayName + QLatin1Char(' ') + version;
}

namespace {

ProductIdentity makeTechAim()
{
    ProductIdentity p;
    // PRODUCT NAME. Compiled per flavour, not assigned and then overwritten:
    // a SETA binary must not carry the Tech Aim product name as dormant text
    // that a `strings` dump would find. Everything genuinely SHARED - the
    // vendor, the legal publisher, the instance lock - is set unconditionally
    // below, because those are facts about the vendor and the machine rather
    // than about the brand.
#ifdef BRAND_SETA
    p.displayName        = QStringLiteral("SETA");
    p.fullProductName    = QStringLiteral("SETA Electronic Target Control");
    p.releaseDescription = QStringLiteral("SETA Electronic Target Control\n"
                                          "Windows Evaluation Build");
    // The shipped file is SETA.exe. A build sent to SETA must not put another
    // company's name on the icon the operator double-clicks.
    p.executableBaseName = QStringLiteral("SETA");
#else
    p.displayName        = QStringLiteral("Tech Aim");
    p.fullProductName    = QStringLiteral("Tech Aim Electronic Target Control");
    p.releaseDescription = QStringLiteral("Tech Aim Electronic Target Control\n"
                                          "Windows Internal Field Test Build");
    p.executableBaseName = QStringLiteral("TechAim");
#endif
    // Shared by EVERY flavour, deliberately. See the field's declaration.
    p.instanceLockName   = QStringLiteral("TechAim");
    p.applicationId      = QStringLiteral("za.co.techaim.electronic-target-control");
    p.organisationName   = QStringLiteral("TechAim");
    p.organisationDomain = QStringLiteral("techaim.co.za");
    // Defaults to the organisation, which is exactly what M0 wrote, so the
    // existing Tech Aim data root does not move.
    p.applicationStorageName = p.organisationName;
    p.brandSettingsScope     = QString();
    p.brandKey               = QStringLiteral("TECH_AIM");

    p.legalPublisher = QStringLiteral("JAC SHOOTING SOLUTIONS (PTY) LTD");
    p.copyrightLine  = QStringLiteral("Copyright (C) JAC SHOOTING SOLUTIONS (PTY) LTD");

    p.version        = QStringLiteral(
#ifdef APP_VERSION_STR
        APP_VERSION_STR
#else
        "0.9.0"
#endif
    );
    // The channel lives HERE, with the rest of the product identity, not in a
    // build define: it contains spaces, and a spaced -D value does not survive
    // the qmake/make/compiler quoting chain intact. The version does come from
    // the build (APP_VERSION_STR) because it has no spaces and must match the
    // packaged artefact name.
    // The channel names the CANDIDATE, so it legitimately changes between
    // builds - RC2a..RC2d carried " - Diagnostic", RC2e dropped it, RC2g-DIAG
    // reinstated it as "Internal Diagnostic Field Test" because that build
    // exists to observe the acquisition decision. What must NEVER change is
    // the invariant the harness enforces: an internal field-test channel that
    // makes no production, general-release or stable claim.
    p.releaseChannel = QStringLiteral("SETA Evaluation");

    // Shown wherever a result could be mistaken for an official one. Short
    // enough to sit in a status strip without covering scores or controls.
    p.fieldTestNotice = QStringLiteral("Evaluation Build — Not for Official Competition Results");

    // Brand mark, as a qrc path so QML asks identity for it rather than
    // hardcoding a logo per screen.
    p.brandLogoPath       = QStringLiteral("qrc:/images/logo/techaim_color.png");
    p.appIconPath         = QStringLiteral(":/images/logo/techaim.ico");
    p.brandLogoOnDarkPath = QStringLiteral("qrc:/images/logo/techaim_white.png");

#ifdef BRAND_SETA
    // ── SETA product line ────────────────────────────────────────────────
    // PRESENTATION + USER-DATA NAMESPACE. The engine is untouched: scoring,
    // acquisition, Modbus, SessionStore, recovery, the paper feed and the
    // report engine are shared, and the session/journal FILE FORMATS stay
    // compatible. A SETA build is the same application under a different
    // name, keeping its own data - not a fork.
    //
    // seta.png is an existing approved asset - it already brands the printed
    // report - so no logo was invented here.
    p.brandKey         = QStringLiteral("SETA");
    p.brandLogoPath    = QStringLiteral("qrc:/images/logo/seta.png");
    p.appIconPath      = QStringLiteral(":/images/logo/seta.ico");
    // SETA supplied ONE mark. There is no white-on-dark variant, so the header
    // uses the same file rather than the Tech Aim white logo - showing another
    // company's mark in a SETA build would be worse than an imperfect one.
    // BrandPackage reports logoWhite as a MISSING ASSET so this stays visible
    // as an outstanding request, not a silent substitution.
    p.brandLogoOnDarkPath = p.brandLogoPath;
    p.reportAuthor     = p.fullProductName;
    p.reportCreator    = p.fullProductName;

    // SEPARATE MUTABLE USER DATA. Without this a SETA install and a Tech Aim
    // install on one machine would read and write the SAME athletes, unfinished
    // sessions, recovery journals, reports, logs and remembered target
    // fingerprints, because Qt derives all of them from
    // <organisationName>/<applicationName>. The VENDOR folder stays shared -
    // the vendor really is the same - and everything mutable inside it does not.
    // There is deliberately NO automatic copy of Tech Aim data into SETA: a
    // fresh SETA install starts clean, and importing an existing store would be
    // a separate, deliberate feature.
    p.applicationId          = QStringLiteral("za.co.techaim.seta.electronic-target-control");
    p.applicationStorageName = QStringLiteral("TechAimSETA");
    p.brandSettingsScope     = QStringLiteral("TechAimSETA");

    // NOT overridden, on purpose:
    //   organisationName / organisationDomain - the vendor, unchanged.
    // The shipped file is SETA.exe. A build sent to SETA must not put another
    // company's name on the icon the operator double-clicks.
    //
    // instanceLockName is deliberately NOT changed with it, and that is the
    // whole point of separating the two fields: one machine drives one target,
    // so this build and a Tech Aim build must still refuse to run together.
    // They share the lock; they share nothing else.
    p.executableBaseName = QStringLiteral("SETA");

    //   instanceLockName - the safety interlock above.
    //     Sharing the lock is deliberate: one machine drives one target, so a
    //     SETA build and a Tech Aim build must still refuse to run together.
    //   legalPublisher - the publishing entity is a legal fact, not a skin.
    //     BRAND/PRODUCT NAME and LEGAL PUBLISHER are separate fields for this
    //     reason; SETA's publisher has not been supplied, so JAC SHOOTING
    //     SOLUTIONS (PTY) LTD remains correct and stays.
#endif

    p.defaultTheme    = QStringLiteral("techaim-dark");
    p.defaultLanguage = QStringLiteral("en");

    p.supportContact = QStringLiteral("support@techaim.co.za");
    p.reportAuthor   = p.fullProductName;
    p.reportCreator  = p.fullProductName;

    // Recognised for migration only — never shown as the product name.
    // "TechAim" itself is retained in the organisation list because the M0
    // reliability work already writes under it; it is the CURRENT root, not
    // a legacy one, and is listed here only so migration code can treat the
    // set uniformly.
    p.legacyApplicationNames  = { QStringLiteral("Seta"), QStringLiteral("Tachus") };
    p.legacyOrganisationNames = { QStringLiteral("Seta"), QStringLiteral("Tachus") };
    p.legacyLockFileNames     = { QStringLiteral("tachus_seta.lock") };
    return p;
}

} // namespace

// The flavour is fixed at compile time.
//
// RATIONALISED: BRAND_SETA and TECHAIM_FLAVOUR_SETA_OEM used to describe the
// same product through two unrelated mechanisms - one chose the name and mark,
// the other chose the asset package - and a SETA build reported "flavour
// TECH_AIM" while calling itself SETA. There is now ONE authority: BuildFlavour
// selects the BrandPackage, and BRAND_SETA is simply the compile-time switch
// that selects the flavour. The older define is still honoured so nothing that
// sets it silently changes meaning.
BuildFlavour currentFlavour()
{
#if defined(BRAND_SETA) || defined(TECHAIM_FLAVOUR_SETA_OEM)
    return BuildFlavour::SetaOem;
#else
    return BuildFlavour::TechAim;
#endif
}

QString flavourName(BuildFlavour f)
{
    switch (f) {
    case BuildFlavour::TechAim: return QStringLiteral("TECH_AIM");
    case BuildFlavour::SetaOem: return QStringLiteral("SETA_OEM");
    }
    return QStringLiteral("TECH_AIM");
}

bool isFlavourBuildable(BuildFlavour f)
{
    // "Producible by THIS build." SETA_OEM used to be refused outright because
    // no SETA palette existed; one now does, sampled from the approved logo, so
    // the question is no longer "does this brand exist" but "is this the brand
    // this binary was compiled as". Buildability is deliberately NOT tied to
    // isComplete(): Tech Aim ships with an outstanding .ico and is buildable,
    // so tying the two would refuse the shipping product.
    return f == currentFlavour();
}

const ProductIdentity& identity()
{
    static const ProductIdentity techAim = makeTechAim();
    // Only one flavour is buildable; the OEM edition resolves to the Tech Aim
    // identity rather than to an empty/partial one, so a mis-set define can
    // never produce a nameless binary.
    return techAim;
}

} // namespace app
} // namespace ta
