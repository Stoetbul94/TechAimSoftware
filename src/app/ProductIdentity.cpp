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
    p.displayName        = QStringLiteral("Tech Aim");
    p.fullProductName    = QStringLiteral("Tech Aim Electronic Target Control");
    p.releaseDescription = QStringLiteral("Tech Aim Electronic Target Control\n"
                                          "Windows");

    p.executableBaseName = QStringLiteral("TechAim");
    p.applicationId      = QStringLiteral("za.co.techaim.electronic-target-control");
    p.organisationName   = QStringLiteral("TechAim");
    p.organisationDomain = QStringLiteral("techaim.co.za");

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
    // RC3B-DIAG is INTERNAL. It carries developer_mode=1 and exists to be
    // diagnosed at the range; a banner and a report footer reading "SETA
    // Evaluation" would tell an operator - and anyone reading a printed
    // result - that this is the build that was handed over. It is not.
    //
    // 1.0.0-RC1 opens the v1.0 line. It is a CANDIDATE: every blocker is
    // closed and the full regression is green, but no live target test was
    // performed for the close-out - the acquisition evidence is inherited from
    // RC3F (docs/release/V1.0-PHYSICAL-EVIDENCE-INHERITANCE.md). A candidate
    // that has not itself been fired at may not describe itself as released,
    // so the channel stays an evaluation channel and the field-test notice
    // stays on the screen and in the report footer.
    // 1.0.0 is the PRODUCTION release. The channel says so plainly: a customer
    // reading it, or reading a report footer, must not find a candidate's
    // wording on a result they are about to hand to an athlete.
    //
    // The invariant the harness enforces has not gone away - it now asserts the
    // opposite side of the same rule: a production channel makes no field-test
    // or evaluation claim, and an evaluation build still may not claim to be
    // production. Both are checked.
    p.releaseChannel = QStringLiteral("Production Release");

    // Shown wherever a result could be mistaken for an official one. Short
    // enough to sit in a status strip without covering scores or controls.
    // EMPTY for a production release. isFieldTest() is derived from this being
    // non-empty, so Settings -> About stops showing the notice by the guard it
    // always had - the mechanism stays intact for the next evaluation build,
    // which is the point of deriving it rather than hard-coding a flag.
    p.fieldTestNotice = QString();

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

// The flavour is fixed at compile time. TECHAIM_FLAVOUR_SETA_OEM is
// deliberately NOT settable from any build we ship; see isFlavourBuildable().
BuildFlavour currentFlavour()
{
#ifdef TECHAIM_FLAVOUR_SETA_OEM
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
    // SETA_OEM is reserved: the branding assets, theme and installer identity
    // for it do not exist yet. Accepting it here would let an unbranded or
    // half-branded OEM build ship.
    return f == BuildFlavour::TechAim;
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
