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

enum class BuildFlavour {
    TechAim = 0,   // the current and only buildable edition
    SetaOem = 1,   // RESERVED. Documented, validated, never produced yet:
                   // no blue theme, no SETA logo set, no separate output,
                   // no runtime selector. See docs/product-identity-audit.md.
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
    // executableBaseName produces TechAim.exe; also used for file prefixes.
    QString executableBaseName;
    QString applicationId;          // reverse-DNS, for future packaging
    QString organisationName;       // QSettings + AppLocalDataLocation root
    QString organisationDomain;

    // ── legal ────────────────────────────────────────────────────────────
    QString legalPublisher;
    QString copyrightLine;

    // ── release ──────────────────────────────────────────────────────────
    QString version;                // 0.9.0
    QString releaseChannel;         // "Pre-Beta Validation"

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
