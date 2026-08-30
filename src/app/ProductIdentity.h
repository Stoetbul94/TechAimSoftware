#ifndef PRODUCTIDENTITY_H
#define PRODUCTIDENTITY_H

#include <QString>
#include <QStringList>

// ─────────────────────────────────────────────────────────────────────────
// P0 Phase B — the ONE authoritative source of product identity.
//
// Every user-facing product name, the executable base name, the publisher,
// the AppData/QSettings identity and the report metadata derive from here.
// Do not re-declare these strings in QML or in other C++ translation units:
// QML reads them through the PRODUCT context property, C++ through
// ta::app::identity().
//
// Identity is fixed at BUILD time by the flavour below. It is never a user
// setting — there is deliberately no "choose brand" control, because brand
// selects the executable name, publisher and data root, none of which can
// change safely inside a running session.
//
// LANGUAGE IS NOT BRAND. The selected UI language must never change the
// logo, theme, executable name, publisher or AppData identity. The two
// concepts are kept separate on purpose so that a future OEM edition can
// ship German-by-default without German implying that edition.
// ─────────────────────────────────────────────────────────────────────────

namespace ta {
namespace app {

// BuildFlavour is THE authority for which brand a binary is. It selects the
// BrandPackage (palette, marks, attribution); BRAND_SETA is simply the
// compile-time switch that selects the flavour, and ProductIdentity's own
// BRAND_SETA block supplies the matching name and data namespace. There is no
// longer a second, disagreeing mechanism.
enum class BuildFlavour {
    TechAim = 0,   // dark + Tech Aim red   (#A80038, from techaim_color.png)
    SetaOem = 1,   // dark + SETA blue      (#00539E, from seta.png)
};

struct ProductIdentity {
    // ── user-facing prose ────────────────────────────────────────────────
    // displayName is the brand as written in sentences: "Tech Aim".
    QString displayName;
    // fullProductName is the formal name for titles, version resources and
    // report metadata: "Tech Aim Electronic Target Control".
    QString fullProductName;
    QString releaseDescription;

    // ── machine-facing, no spaces ────────────────────────────────────────
    // executableBaseName produces TechAim.exe / SETA.exe; also used for file
    // prefixes. It is the SHIPPED FILE NAME and nothing more.
    QString executableBaseName;
    // The single-instance lock, which is a SAFETY property and not a brand one:
    // one machine drives one target, so a SETA build and a Tech Aim build must
    // refuse to run together even though they are different products with
    // different names and different data. Every flavour therefore shares ONE
    // lock name, and it must never be derived from the executable name - doing
    // so is exactly what would let two brands of the same application drive one
    // target at once. Empty falls back to executableBaseName.
    QString instanceLockName;
    QString applicationId;          // reverse-DNS, for future packaging
    QString organisationName;       // VENDOR. QSettings + AppLocalDataLocation root
    QString organisationDomain;
    // PRODUCT, within the vendor. Qt resolves AppLocalDataLocation as
    // <LOCALAPPDATA>/<organisationName>/<applicationName> and the default
    // QSettings scope as HKCU\Software\<organisationName>\<applicationName>,
    // so this leaf is what separates one product line's MUTABLE USER DATA -
    // sessions, recovery journals, reports, logs, target fingerprints - from
    // another's. Two products of the same vendor share the vendor folder and
    // nothing inside it. Defaults to organisationName, which is what Tech Aim
    // has always used, so the existing data root does not move.
    QString applicationStorageName;
    // Legacy per-brand QSettings organisation ("Tachus"/"Seta") used by the
    // pre-rebrand EULA + last-folder keys. Empty keeps the legacy behaviour
    // exactly; a non-empty value moves ONLY those keys into this product's
    // own scope. Never used for session, score or recovery data.
    QString brandSettingsScope;

    // ── legal ────────────────────────────────────────────────────────────
    QString legalPublisher;
    QString copyrightLine;

    // ── release ──────────────────────────────────────────────────────────
    QString version;                // 0.9.0-RC1
    QString releaseChannel;         // "Internal Field Test"
    // The shipped BRAND SKIN: "TECH_AIM" or "SETA". Deliberately NOT the same
    // thing as BuildFlavour::SetaOem below — see the note on that enum.
    QString brandKey;
    // Shown where a result could be mistaken for an official one. Empty in a
    // future general release; non-empty means this build is a field-test
    // candidate and must say so.
    QString brandLogoPath;          // qrc path to the product's brand mark
    // The mark to use on a DARK surface (the application header). A separate
    // field because "which file" is a brand fact, not a screen decision - a
    // screen must never pick a logo by asking which product this is.
    QString brandLogoOnDarkPath;
    // The SQUARE application icon (a Windows .ico). Separate from the brand
    // mark because the mark is wide: an OS icon has to be square, and the
    // one Windows draws for the WINDOW is not the one it draws for the FILE.
    QString appIconPath;
    QString fieldTestNotice;        // "FIELD TEST — NOT FOR OFFICIAL COMPETITION RESULTS"

    // ── defaults (NOT user identity — see the language/brand note above) ──
    QString defaultTheme;
    QString defaultLanguage;

    // ── support + report attribution ─────────────────────────────────────
    QString supportContact;
    QString reportAuthor;
    QString reportCreator;

    // ── legacy, for migration + audit only ───────────────────────────────
    // Application names and data locations this build must still RECOGNISE
    // (to migrate settings and to refuse to run beside an old instance),
    // but must never present to the user as the product name.
    QStringList legacyApplicationNames;
    QStringList legacyOrganisationNames;
    QStringList legacyLockFileNames;

    // "Tech Aim 0.9.0" — the software attribution printed on reports,
    // replacing the hardcoded "Seta 4.0" strings.
    QString softwareVersionLabel() const;
};

// The identity for the flavour this binary was built with.
const ProductIdentity& identity();

BuildFlavour currentFlavour();
QString flavourName(BuildFlavour f);

// Validates that a flavour is actually producible by this build. SetaOem is
// a reserved value: it is accepted by the type system and documented, but
// building it is refused so that a half-configured OEM edition can never be
// shipped by accident.
bool isFlavourBuildable(BuildFlavour f);

} // namespace app
} // namespace ta

#endif // PRODUCTIDENTITY_H
