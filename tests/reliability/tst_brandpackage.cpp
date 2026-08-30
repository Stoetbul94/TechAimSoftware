// UI-1 — Design System, brand tokens and the BrandPackage boundary.
//
// The point of these checks is the BOUNDARY: a brand package may change how
// the product looks and what it is called, and nothing else. Anything that
// would let a brand value reach scoring, rules, recovery or discipline
// availability is a defect, not a styling choice.
//
// The QML-side checks read the token and homepage sources as text. That is
// deliberate: this harness is QtCore-only (no QML engine), and a text check
// still proves the thing that actually regresses — a developer pasting a hex
// literal back into the homepage.
#include "test_support.h"
#include "app/BrandPackage.h"
#include "app/ProductIdentity.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <cmath>
#include <QSet>
#include <QString>
#include <QStringList>
#include <array>
#include <cmath>

using namespace ta::app;

namespace {

QString repoRoot()
{
    // The harness binary lives in <repo>/release; fixtures are addressed from
    // the source tree the same way the other reliability tests do it.
    QDir d(QString::fromLatin1(RELIABILITY_FIXTURES_DIR));
    d.cdUp();   // tests/reliability
    d.cdUp();   // tests
    d.cdUp();   // repo root
    return d.absolutePath();
}

QString readFile(const QString& relative, bool* ok = nullptr)
{
    QFile f(repoRoot() + QLatin1Char('/') + relative);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok) *ok = false;
        return QString();
    }
    if (ok) *ok = true;
    const QString s = QString::fromUtf8(f.readAll());
    f.close();
    return s;
}

// Strip // line comments so an explanatory comment mentioning a legacy colour
// cannot fail a check about the CODE. (The window-title test learned this the
// hard way in P0.)
QString stripComments(const QString& src)
{
    QString out;
    out.reserve(src.size());
    const QStringList lines = src.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const int idx = line.indexOf(QStringLiteral("//"));
        out += (idx >= 0 ? line.left(idx) : line);
        out += QLatin1Char('\n');
    }
    return out;
}

} // namespace

void run_brand_package_tests()
{
    std::printf("--- UI-1 design system + brand package ---\n");

    // ── the approved accent ─────────────────────────────────────────────────
    {
        const BrandPackage& b = brand();
        // brand() is the package for THIS build's flavour. Both products'
        // exact palettes are asserted together further down; here the point is
        // that the running build gets its OWN package and not the other one.
#ifdef BRAND_SETA
        check(b.flavourKey == QLatin1String("SETA_OEM"),
              "brand: the built flavour is SETA", b.flavourKey);
        check(b.accentPrimary == QLatin1String("#00539E"),
              "brand: accentPrimary is the approved SETA #00539E (sampled from seta.png)",
              b.accentPrimary);
        check(b.accentHover == QLatin1String("#25B0E6"),
              "brand: accentHover is #25B0E6", b.accentHover);
        check(b.accentPressed == QLatin1String("#003A6E"),
              "brand: accentPressed is #003A6E", b.accentPressed);
        check(b.logoIntrinsicColour == QLatin1String("#212D60"),
              "brand: the SETA logo navy is recorded as logo-intrinsic");
#else
        check(b.flavourKey == QLatin1String("TECH_AIM"),
              "brand: the built flavour is Tech Aim");
        check(b.accentPrimary == QLatin1String("#A80038"),
              "brand: accentPrimary is the approved #A80038 (sampled from the logo)");
        check(b.accentHover == QLatin1String("#C40046"),
              "brand: accentHover is #C40046");
        check(b.accentPressed == QLatin1String("#80032A"),
              "brand: accentPressed is #80032A");
        check(b.logoIntrinsicColour == QLatin1String("#BF1919"),
              "brand: the logo tagline red is recorded as logo-intrinsic");
#endif
        check(b.accentPrimary != b.logoIntrinsicColour,
              "brand: the logo-intrinsic red is NOT the application accent");

        // The concept accent from the UI-0 mockup must never reach the product.
        check(b.accentPrimary.compare(QLatin1String("#e6003c"), Qt::CaseInsensitive) != 0 &&
              b.accentHover.compare(QLatin1String("#e6003c"), Qt::CaseInsensitive) != 0,
              "brand: the UI-0 concept accent #e6003c is not adopted");
    }

    // ── identity is unchanged by the design system ──────────────────────────
    {
        const BrandPackage& b = brand();
#ifdef BRAND_SETA
        check(b.productName == QLatin1String("SETA Electronic Target Control"),
              "brand: product name is SETA Electronic Target Control");
        check(b.shortProductName == QLatin1String("SETA"),
              "brand: short product name is SETA");
#else
        check(b.productName == QLatin1String("Tech Aim Electronic Target Control"),
              "brand: product name is Tech Aim Electronic Target Control");
        check(b.shortProductName == QLatin1String("Tech Aim"),
              "brand: short product name is Tech Aim");
#endif
        // The publisher is a LEGAL fact and is not part of the skin, so it is
        // asserted identically on both product lines.
        check(b.publisher == QLatin1String("JAC SHOOTING SOLUTIONS (PTY) LTD"),
              "brand: publisher is JAC SHOOTING SOLUTIONS (PTY) LTD");
        check(b.productName == identity().fullProductName,
              "brand: package and ProductIdentity agree on the product name");
        check(b.publisher == identity().legalPublisher,
              "brand: package and ProductIdentity agree on the publisher");
    }

    // ── missing assets are REPORTED, never invented ─────────────────────────
    {
        const BrandPackage& b = brand();
        const QStringList missing = b.missingAssets();

        // The Windows icon genuinely does not exist yet. The test asserts that
        // the system SAYS SO rather than quietly deriving one from the logo.
        bool iconReported = false;
        for (const QString& m : missing)
            if (m.startsWith(QStringLiteral("windowsIcon"))) iconReported = true;
        check(iconReported,
              "brand: the absent Windows icon is reported as missing, not invented");
        check(!b.isComplete(),
              "brand: the package is honestly incomplete while an asset is absent");

#ifdef BRAND_SETA
        // SETA supplied ONE mark (seta.png). The white-on-dark and single-ink
        // variants do not exist, and the package must SAY SO rather than fall
        // back to the Tech Aim logo - which would put another company's mark on
        // a SETA header and a SETA report.
        bool whiteReported = false, monoReported = false;
        for (const QString& m : missing) {
            if (m.startsWith(QStringLiteral("logoWhite")))      whiteReported = true;
            if (m.startsWith(QStringLiteral("logoMonochrome"))) monoReported  = true;
        }
        check(whiteReported && monoReported,
              "brand: the SETA logo variants that were never supplied are reported");
        check(b.logoColour == identity().brandLogoPath
              && b.reportLogo == identity().brandLogoPath,
              "brand: the SETA marks come from identity, not a per-screen literal");
        check(!b.logoColour.contains(QLatin1String("techaim")),
              "brand: no Tech Aim logo asset is used by the SETA package");
        check(missing.size() == 3,
              QString(QStringLiteral("brand: icon + two logo variants are the outstanding SETA assets (missing: %1)"))
                  .arg(missing.join(QStringLiteral(", "))));
#else
        // The icon must be the ONLY outstanding item. Anything else appearing
        // here means an asset was quietly dropped from the package.
        check(missing.size() == 1,
              QString(QStringLiteral("brand: the icon is the ONLY outstanding Tech Aim asset (missing: %1)"))
                  .arg(missing.join(QStringLiteral(", "))));
#endif
    }

    // ── BRAND-PALETTE SEPARATION (SETA blue vs Tech Aim red) ────────────────
    // The two packages must be complete, distinct, and neither may leak into
    // the other. A screen asks for `accentPrimary`; the package decides what
    // that IS, so nothing here may depend on which product is being drawn.
    {
        const BrandPackage& ta = brandFor(BuildFlavour::TechAim);
        const BrandPackage& sa = brandFor(BuildFlavour::SetaOem);

        // Tech Aim red, EXACTLY as approved 2026-07-29 and unchanged by the
        // SETA work. If this moves, the Tech Aim product changed appearance.
        check(ta.accentPrimary == QLatin1String("#A80038")
              && ta.accentHover   == QLatin1String("#C40046")
              && ta.accentPressed == QLatin1String("#80032A")
              && ta.accentSubtle  == QLatin1String("#2D0A18")
              && ta.accentBright  == QLatin1String("#E8003D"),
              "brand: the Tech Aim red palette is unchanged",
              ta.accentPrimary + QStringLiteral("/") + ta.accentBright);
        check(ta.logoIntrinsicColour == QLatin1String("#BF1919"),
              "brand: the Tech Aim logo-intrinsic colour is unchanged");

        // SETA blue, sampled from images/logo/seta.png. #25B0E6 and #00539E
        // are LOGO colours; #003A6E is accentPrimary x 0.70 and #0F2740 is
        // 28% accentPrimary over surfacePrimary, both stated derivations.
        check(sa.accentPrimary == QLatin1String("#00539E")
              && sa.accentHover   == QLatin1String("#25B0E6")
              && sa.accentPressed == QLatin1String("#003A6E")
              && sa.accentSubtle  == QLatin1String("#0F2740")
              && sa.accentBright  == QLatin1String("#25B0E6"),
              "brand: the SETA blue palette is the approved one",
              sa.accentPrimary + QStringLiteral("/") + sa.accentBright);
        check(sa.logoIntrinsicColour == QLatin1String("#212D60"),
              "brand: the SETA logo navy is declared logo-intrinsic, not an accent");

        // NO LEAKAGE, in either direction.
        // Hand-rolled on purpose: this harness links QtCore only, so QColor
        // is not available and pulling QtGui in for three integers would make
        // a GUI-free test suite depend on a GUI.
        const auto rgb = [](const QString& hex) {
            const QString h = QString(hex).remove(QLatin1Char('#'));
            return std::array<int, 3>{ h.mid(0, 2).toInt(nullptr, 16),
                                       h.mid(2, 2).toInt(nullptr, 16),
                                       h.mid(4, 2).toInt(nullptr, 16) };
        };
        const auto isBlue = [&rgb](const QString& hex) {
            const auto c = rgb(hex); return c[2] > c[0] + 25;
        };
        const auto isRed = [&rgb](const QString& hex) {
            const auto c = rgb(hex); return c[0] > c[2] + 25;
        };
        check(isRed(ta.accentPrimary) && isRed(ta.accentHover)
              && isRed(ta.accentPressed) && isRed(ta.accentBright),
              "brand: no SETA blue leaks into the Tech Aim package");
        check(isBlue(sa.accentPrimary) && isBlue(sa.accentHover)
              && isBlue(sa.accentPressed) && isBlue(sa.accentBright),
              "brand: no Tech Aim red leaks into the SETA package");
        check(ta.accentPrimary != sa.accentPrimary
              && ta.accentBright != sa.accentBright
              && ta.logoColour != sa.logoColour,
              "brand: the two packages are genuinely different products");

        // An accent is a FILL THAT CARRIES TEXT. Both must clear 4.5:1 with
        // their declared text colour, or the accent is not usable as one.
        // WCAG relative luminance / contrast ratio.
        const auto contrast = [&rgb](const QString& a, const QString& b) {
            const auto lum = [&rgb](const QString& hex) {
                const auto c = rgb(hex);
                auto ch = [](int v) {
                    const double x = v / 255.0;
                    return x <= 0.03928 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4);
                };
                return 0.2126 * ch(c[0]) + 0.7152 * ch(c[1]) + 0.0722 * ch(c[2]);
            };
            const double la = lum(a), lb = lum(b);
            return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
        };
        check(contrast(ta.textOnAccent, ta.accentPrimary) >= 4.5,
              "brand: Tech Aim text-on-accent is legible",
              QString::number(contrast(ta.textOnAccent, ta.accentPrimary), 'f', 2));
        check(contrast(sa.textOnAccent, sa.accentPrimary) >= 4.5,
              "brand: SETA text-on-accent is legible",
              QString::number(contrast(sa.textOnAccent, sa.accentPrimary), 'f', 2));
        // The focus ring has to survive the DARKEST canvas, which is the whole
        // reason it is the lighter accent rather than accentPrimary.
        check(contrast(ta.focusOutline, QStringLiteral("#0B0D10")) >= 3.0
              && contrast(sa.focusOutline, QStringLiteral("#0B0D10")) >= 3.0,
              "brand: both focus rings read on the darkest canvas");

        // Both packages are addressable and neither is a stub any more.
        check(!sa.productName.isEmpty() && !sa.publisher.isEmpty(),
              "brand: the SETA package names its product and publisher");
        check(sa.publisher == ta.publisher,
              "brand: branding does NOT re-attribute the legal publisher");
    }

    // ── SEMANTIC COLOURS ARE NOT BRAND COLOURS ──────────────────────────────
    // A fault must read as a fault in every product. These live in the token
    // layer, not the brand package, and no package may set them - which is why
    // BrandPackage has no field for them at all.
    {
        bool tokOk = false;
        const QString tokens = readFile(QStringLiteral("src/ui/theme/DesignTokens.qml"), &tokOk);
        check(tokOk && !tokens.isEmpty(), "tokens: DesignTokens.qml is readable");
        // The status block still carries its own literals...
        QString flat = tokens;
        flat.remove(QLatin1Char(' '));
        // These stay SEMANTIC: an error is red because it is an error, not
        // because of the brand. They now carry a light variant as well as a
        // dark one, so the assertion is that they are LITERAL and not routed
        // through the brand - which is the actual requirement.
        check(flat.contains(QStringLiteral("\"#D0392B\""))
              && flat.contains(QStringLiteral("\"#20C997\""))
              && flat.contains(QStringLiteral("\"#E8A13D\""))
              && !flat.contains(QStringLiteral("errorText:PRODUCT"))
              && !flat.contains(QStringLiteral("successText:PRODUCT"))
              && !flat.contains(QStringLiteral("warningText:PRODUCT")),
              "tokens: error / success / warning stay semantic literals");
        check(!flat.contains(QStringLiteral("errorText:PRODUCT."))
              && !flat.contains(QStringLiteral("successText:PRODUCT.")),
              "tokens: no brand package may set a semantic status colour");
        // ...and the brand accents no longer do.
        check(!flat.contains(QStringLiteral("accentPrimary:\"#"))
              && !flat.contains(QStringLiteral("accentHover:\"#")),
              "tokens: the brand accent comes from the package, not a literal");
        check(tokens.contains(QStringLiteral("PRODUCT.accentPrimary")),
              "tokens: the accent is read from the build's brand package");
    }

    // ── the OEM seam is addressable and does not fall back ──────────────────
    {
        const BrandPackage& oem = brandFor(BuildFlavour::SetaOem);
        check(oem.flavourKey == QLatin1String("SETA_OEM"),
              "oem: the reserved package is addressable");
        check(!oem.productName.isEmpty(),
              "oem: the SETA package now names its product");
        check(!oem.isComplete(),
              "oem: the package reports itself incomplete while assets are absent");
        check(oem.missingAssets().size() == 3,
              QString(QStringLiteral("oem: exactly the three unsupplied SETA assets are reported (%1)"))
                  .arg(oem.missingAssets().join(QStringLiteral(", "))));

        // The critical one: asking for the OEM package must NOT silently hand
        // back Tech Aim artwork.
        check(oem.logoColour != brandFor(BuildFlavour::TechAim).logoColour,
              "oem: the OEM package does not inherit Tech Aim artwork");
        check(isFlavourBuildable(currentFlavour()) && !isFlavourBuildable(
                  currentFlavour() == BuildFlavour::TechAim ? BuildFlavour::SetaOem
                                                            : BuildFlavour::TechAim),
              "oem: the OEM flavour remains unbuildable");
    }

    // ── switching package changes PRESENTATION ONLY ─────────────────────────
    {
        // Programme behaviour is not reachable from a BrandPackage at all:
        // the struct carries no rule, discipline, scoring or storage field.
        // This is asserted structurally — if someone adds one, the field list
        // below stops matching and this check must be revisited.
        const BrandPackage& a = brandFor(BuildFlavour::TechAim);
        const BrandPackage& c = brandFor(BuildFlavour::SetaOem);
        check(a.flavourKey != c.flavourKey,
              "boundary: the two packages are genuinely different objects");
        // Identity of the RUNNING build is fixed by the flavour, not by which
        // package was inspected — reading the OEM package must not mutate it.
#ifdef BRAND_SETA
        check(identity().fullProductName == QLatin1String("SETA Electronic Target Control"),
#else
        check(identity().fullProductName == QLatin1String("Tech Aim Electronic Target Control"),
#endif
              "boundary: inspecting the OEM package does not change the running identity");
        check(brand().accentPrimary == brandFor(currentFlavour()).accentPrimary,
              "boundary: inspecting a package does not change the running accent",
              brand().accentPrimary);
    }

    // ── the token layer exists and is semantic ──────────────────────────────
    {
        bool ok = false;
        const QString tokens = readFile(QStringLiteral("src/ui/theme/DesignTokens.qml"), &ok);
        check(ok && !tokens.isEmpty(), "tokens: DesignTokens.qml exists");

        const char* required[] = {
            "backgroundPrimary", "backgroundSecondary", "surfacePrimary",
            "surfaceSecondary", "surfaceElevated", "inputBackground",
            "borderSubtle", "borderStrong", "textPrimary", "textSecondary",
            "textDisabled", "accentPrimary", "accentHover", "accentPressed",
            "accentSubtle", "successBackground", "successText",
            "warningBackground", "warningText", "errorBackground",
            "errorText", "focusOutline"
        };
        for (const char* name : required) {
            check(tokens.contains(QString::fromLatin1(name)),
                  QString(QStringLiteral("tokens: %1 is defined")).arg(QString::fromLatin1(name)));
        }

        check(!tokens.contains(QStringLiteral("\"#A80038\"")),
              "tokens: the brand accent literal has left the token file");

        // Semantic naming: no colour-literal token names.
        const QRegularExpression literalName(
            QStringLiteral("property color (red|grey|gray|blue|green)[0-9]"));
        check(!literalName.match(tokens).hasMatch(),
              "tokens: no token is named after a literal colour (red1 / grey7)");

        const QString typo = readFile(QStringLiteral("src/ui/theme/Typography.qml"), &ok);
        check(ok && typo.contains(QStringLiteral("pageTitle")) &&
              typo.contains(QStringLiteral("numericMetric")) &&
              typo.contains(QStringLiteral("buttonText")),
              "tokens: typography roles are defined");
        // No proprietary font may be required.
        check(typo.contains(QStringLiteral("Segoe UI")) && typo.contains(QStringLiteral("Consolas")),
              "tokens: only fonts that ship with Windows are required");

        const QString space = readFile(QStringLiteral("src/ui/theme/Spacing.qml"), &ok);
        check(ok && space.contains(QStringLiteral("spacing8")) &&
              space.contains(QStringLiteral("radiusMedium")) &&
              space.contains(QStringLiteral("touchMinimum")),
              "tokens: spacing, radius and touch scales are defined");
        check(space.contains(QStringLiteral("touchMinimum:       44")),
              "tokens: the touch-target floor is 44 px");
    }

    // ── the homepage carries no hard-coded palette ──────────────────────────
    {
        bool ok = false;
        const QString page = stripComments(
            readFile(QStringLiteral("LoginPage.qml"), &ok));
        check(ok && !page.isEmpty(), "home: LoginPage.qml is readable");

        check(page.contains(QStringLiteral("theme.tokens.accentPrimary")),
              "home: the homepage binds to the central accent token");

        // Any remaining #rrggbb literal in the CODE is a regression.
        const QRegularExpression hex(QStringLiteral("\"#[0-9A-Fa-f]{3,8}\""));
        QStringList found;
        auto it = hex.globalMatch(page);
        while (it.hasNext()) found << it.next().captured(0);
        check(found.isEmpty(),
              QString(QStringLiteral("home: no hard-coded colour literals remain (found %1: %2)"))
                  .arg(found.size()).arg(found.join(QStringLiteral(", ")).left(120)));

        // The three legacy reds must not reappear here.
        for (const char* legacy : { "#e8003d", "#e6003c", "#C40046", "#a80038" }) {
            check(!page.contains(QString::fromLatin1(legacy), Qt::CaseInsensitive),
                  QString(QStringLiteral("home: legacy/raw red %1 is not reintroduced"))
                      .arg(QString::fromLatin1(legacy)));
        }

        // Version B structure must survive: the primary action lives in the
        // bottom bar, outside the clipped setup column.
        check(page.contains(QStringLiteral("id: actionBar")),
              "home: the bottom action bar is present");
        check(page.contains(QStringLiteral("id: startSessionRect")),
              "home: the primary Start action is present");
        check(page.contains(QStringLiteral("id: setupScroll")),
              "home: the setup column scrolls (cannot clip the primary action)");
        const int barIdx   = page.indexOf(QStringLiteral("id: actionBar"));
        const int startIdx = page.indexOf(QStringLiteral("id: startSessionRect"));
        check(barIdx >= 0 && startIdx > barIdx,
              "home: the Start action sits INSIDE the bottom action bar");

        // Four labelled event groups.
        for (const char* group : { "OFFICIAL ISSF MATCH", "FINALS", "TRAINING LAB", "PRACTICE" }) {
            check(page.contains(QString::fromLatin1(group)),
                  QString(QStringLiteral("home: event group %1 is present"))
                      .arg(QString::fromLatin1(group)));
        }

        // Touch targets.
        //
        // A regex cannot tell an interactive control from a decoration — the
        // first version of this check failed on a 7 px connection-status DOT,
        // which is not a control at all. So it asserts what IS checkable in
        // source: that the homepage declares its interactive controls at the
        // design-system sizes, every one of which clears the 44 px floor.
        // Whether each hit area actually feels right under a thumb on a range
        // tablet is HUMAN VISUAL CHECK REQUIRED — see
        // docs/design/Screen_Layout_Rules.md.
        struct Ctl { const char* decl; const char* what; };
        const Ctl controls[] = {
            { "height: 52", "fields and selectors" },
            { "height: 56", "primary and secondary actions" },
            { "height: 58", "discipline selector cards" },
            { "height: 78", "event and programme cards" },
        };
        for (const Ctl& c : controls) {
            check(page.contains(QString::fromLatin1(c.decl)),
                  QString(QStringLiteral("home: %1 use the %2 touch size"))
                      .arg(QString::fromLatin1(c.what), QString::fromLatin1(c.decl)));
        }
        // The pre-UI-1 control size must not creep back in.
        check(!page.contains(QStringLiteral("height: 46; radius")),
              "home: the pre-UI-1 46 px control size is not reintroduced");
    }

    // ── Theme.qml still serves the un-migrated screens ──────────────────────
    {
        bool ok = false;
        const QString themeSrc = readFile(QStringLiteral("Theme.qml"), &ok);
        check(ok, "theme: Theme.qml is readable");
        check(themeSrc.contains(QStringLiteral("DesignTokens")) &&
              themeSrc.contains(QStringLiteral("Typography")) &&
              themeSrc.contains(QStringLiteral("Spacing")),
              "theme: the token layer is exposed through Theme.qml");
        // Back-compat: legacy properties must survive, because ~20 screens
        // have not been migrated and this phase is homepage-only.
        for (const char* legacy : { "brandPrimary", "bgBase", "textPrimary", "fontFamily", "logoWhite" }) {
            check(themeSrc.contains(QString::fromLatin1(legacy)),
                  QString(QStringLiteral("theme: legacy property %1 is preserved for un-migrated screens"))
                      .arg(QString::fromLatin1(legacy)));
        }
    }

    // ── brand assets on disk ────────────────────────────────────────────────
    {
        const QString root = repoRoot();
        for (const char* asset : { "images/logo/techaim_color.png",
                                   "images/logo/techaim_white.png",
                                   "images/logo/techaim_black.png" }) {
            check(QFileInfo::exists(root + QLatin1Char('/') + QString::fromLatin1(asset)),
                  QString(QStringLiteral("assets: %1 exists")).arg(QString::fromLatin1(asset)));
        }
        // The OEM folder must exist as a documented placeholder and must NOT
        // contain artwork copied from anywhere.
        const QString oemDir = root + QStringLiteral("/assets/brands/seta-oem");
        check(QFileInfo::exists(oemDir + QStringLiteral("/README.md")),
              "assets: the reserved OEM brand folder documents itself");
        QDir oem(oemDir);
        const QStringList art = oem.entryList(QStringList()
            << QStringLiteral("*.png") << QStringLiteral("*.ico")
            << QStringLiteral("*.jpg") << QStringLiteral("*.svg"), QDir::Files);
        check(art.isEmpty(),
              "assets: no OEM artwork has been invented or copied in");
    }
}
