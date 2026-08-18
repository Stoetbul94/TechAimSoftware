// ─────────────────────────────────────────────────────────────────────────────
// QML shot-path smoke test — regression cover for QML-SHOT-001.
//
// WHY THIS EXISTS
// RC2a shipped a Severity 1 defect that every C++ harness passed over: a
// diagnostic stamp inside CenterPane.qml's triggerAutoZoom() referenced
// `newShootCount`, a local var of two unrelated functions. It threw a
// ReferenceError on the function's first statement, and the exception
// propagated into the shot handler and skipped backEndShootCount,
// updateSeriesScore and saveMatch. 2293 reliability + 568 training + 235 3P +
// 143 10m finals checks all passed, because NONE of them executes QML.
//
// WHAT THIS TEST DOES
// It EXECUTES the real functions. It reads CenterPane.qml from the repository,
// extracts triggerAutoZoom() and traceStage() verbatim, mounts them in a
// minimal Item with a stub MODREADER, and calls them through the QML engine
// with a QtMsgHandler installed. Any ReferenceError, TypeError or uncaught QML
// exception fails the test.
//
// Extracting from the real file (rather than pasting a copy here) is
// deliberate: a pasted copy would drift and would have passed while the
// shipped code was broken.
//
// WHAT IT DOES NOT DO
// It does not instantiate the whole CenterPane - that needs the full
// application context (MODREADER, APPSETTINGS, TRAINING, shootingPage, theme,
// QtCharts series...). It covers the shot-stamp path and the operational
// ordering around it, which is where the defect was. Full-component coverage
// would need the application running, which is what the physical test is for.
// ─────────────────────────────────────────────────────────────────────────────

#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QSet>
#include <QTranslator>
#include <QScopedPointer>
#include <cstdio>

static int  g_checks = 0;
static int  g_failures = 0;
static QStringList g_qmlMessages;

static void check(bool ok, const char* name, const QString& detail = QString())
{
    ++g_checks;
    if (ok) {
        printf("PASS  %s\n", name);
    } else {
        ++g_failures;
        printf("FAIL  %s  %s\n", name, qPrintable(detail));
    }
    fflush(stdout);
}

// Every QML warning/error is captured. A ReferenceError arrives here as a
// warning, which is exactly how RC2a's defect was invisible: nothing crashed.
static void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)
        g_qmlMessages.append(msg);
}

// Stub MODREADER. Records the stamps it is given so ordering can be asserted.
class StubModReader : public QObject
{
    Q_OBJECT
public:
    QStringList stamps;
    QStringList logLines;
    int shootCount = 0;

    Q_INVOKABLE void traceShotStageFromQml(const QString& stage, const QVariant& seq)
    {
        stamps.append(QStringLiteral("%1/%2").arg(stage, seq.toString()));
    }
    Q_INVOKABLE int getShootCount() const { return shootCount; }
    Q_INVOKABLE void appendToLogFile(const QString& s) { logLines.append(s); }
};

// Pulls a named function's full text out of the real QML file by brace
// matching, so the test always runs the SHIPPED implementation.
static QString readAll(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(f.readAll());
}

static QString extractFunction(const QString& source, const QString& name)
{
    const int start = source.indexOf(QRegularExpression(
        QStringLiteral("function\\s+%1\\s*\\(").arg(name)));
    if (start < 0)
        return QString();
    int i = source.indexOf(QLatin1Char('{'), start);
    if (i < 0)
        return QString();
    int depth = 0;
    for (int j = i; j < source.size(); ++j) {
        const QChar c = source.at(j);
        if (c == QLatin1Char('{')) ++depth;
        else if (c == QLatin1Char('}')) {
            --depth;
            if (depth == 0)
                return source.mid(start, j - start + 1);
        }
    }
    return QString();
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    printf("=== QML shot-path tests (QML-SHOT-001 regression) ===\n");
    fflush(stdout);

    const QString qmlPath = QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml");
    QFile f(qmlPath);
    check(f.open(QIODevice::ReadOnly | QIODevice::Text),
          "CenterPane.qml can be read", qmlPath);
    if (!f.isOpen()) {
        // ─────────────────────────────────────────────────────────────────────
    // SETA-BRAND-001 — the blue theme is a PACKAGE, not a sweep.
    //
    // Two separate risks: that a brand recolour reaches a SEMANTIC colour (a
    // fault stops reading as a fault), and that it reaches SCORING or TARGET
    // presentation (the picture of the shot changes because of branding).
    // Both are asserted against the shipped source, not against prose.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString tokens = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/ui/theme/DesignTokens.qml"));
        QString flatTokens = tokens;
        flatTokens.remove(QLatin1Char(' '));
        check(!tokens.isEmpty(), "SETA-BRAND-001: DesignTokens.qml is readable");
        check(flatTokens.contains(QStringLiteral("accentPrimary:PRODUCT.accentPrimary"))
              && flatTokens.contains(QStringLiteral("accentBright:PRODUCT.accentBright"))
              && flatTokens.contains(QStringLiteral("focusOutline:PRODUCT.focusOutline")),
              "SETA-BRAND-001: the accent comes from the build's brand package");
        check(flatTokens.contains(QStringLiteral("errorText:\"#D0392B\""))
              && flatTokens.contains(QStringLiteral("successText:\"#20C997\""))
              && flatTokens.contains(QStringLiteral("warningText:\"#E8A13D\"")),
              "SETA-BRAND-001: fault / healthy / warning are NOT brand-driven");

        // Semantic and scoring sites deliberately kept literal. Each one means
        // something; recolouring them for branding would change what the
        // operator is being told.
        struct Site { const char* file; const char* needle; const char* why; };
        const Site kept[] = {
            { "/main.qml", "opLive ? \"#2f7d4f\" : \"#e8003d\"",
              "Demo/Live badge - mistaking Demo for Live is a result-integrity risk" },
            { "/RightPanel.qml", "c: \"#e8003d\"",
              "score band <=7 - scoring presentation, not brand" },
            { "/FinalsReportTarget.qml", "\"#a80038\"",
              "shot-score colour" },
            { "/Report3PSeries.qml", "\"#a80038\"",
              "shot-score colour" },
            { "/SettingsPage.qml", "gameRange === 10 ? \"#f2c200\" : \"#e8003d\"",
              "target display colour swatch - shows the TARGET's colours" },
        };
        for (const Site& site : kept) {
            const QString src = readAll(QStringLiteral(TECHAIM_SOURCE_DIR) + QLatin1String(site.file));
            check(src.contains(QLatin1String(site.needle)),
                  "SETA-BRAND-001: semantic/scoring colour kept literal",
                  QLatin1String(site.file) + QStringLiteral(" - ")
                      + QLatin1String(site.why));
        }

        // The scoring authority itself must carry no brand colour at all.
        const QString centre = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml"));
        check(!centre.contains(QStringLiteral("PRODUCT.accentPrimary"))
              || centre.contains(QStringLiteral("calculateShootingSocre")),
              "SETA-BRAND-001: CenterPane still owns the scoring function");
        check(!centre.contains(QStringLiteral("radOf10Ring: PRODUCT"))
              && !centre.contains(QStringLiteral("radOf10Ring: theme")),
              "SETA-BRAND-001: ring geometry is not reachable from the brand");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-002 — language changes LABELS and nothing else.
    // Extends QML-LANG-001 from programmeId to every stable identity the
    // hierarchy exposes: rule set, discipline and target standard.
    // ─────────────────────────────────────────────────────────────────────
    {
        const auto snapshot = [](QQmlEngine& eng) {
            QQmlComponent comp(&eng, QUrl::fromLocalFile(
                QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
            QObject* cat = comp.create();
            QStringList ids;
            if (cat) {
                QVariant rs;
                QMetaObject::invokeMethod(cat, "ruleSets", Q_RETURN_ARG(QVariant, rs),
                                          Q_ARG(QVariant, QVariant()));
                for (const QVariant& r : rs.toList()) {
                    const QString rid = r.toMap().value(QStringLiteral("rulesetId")).toString();
                    ids << QStringLiteral("R:") + rid;
                    QVariant dv;
                    QMetaObject::invokeMethod(cat, "disciplines", Q_RETURN_ARG(QVariant, dv),
                                              Q_ARG(QVariant, rid), Q_ARG(QVariant, QVariant()));
                    for (const QVariant& d : dv.toList()) {
                        const QString did = d.toMap().value(QStringLiteral("disciplineId")).toString();
                        ids << QStringLiteral("D:") + rid + QLatin1Char('/') + did
                               + QStringLiteral(" T:")
                               + d.toMap().value(QStringLiteral("targetStandardId")).toString();
                        QVariant pv;
                        QMetaObject::invokeMethod(cat, "programmes", Q_RETURN_ARG(QVariant, pv),
                                                  Q_ARG(QVariant, rid), Q_ARG(QVariant, did),
                                                  Q_ARG(QVariant, QVariant()));
                        for (const QVariant& p : pv.toList()) {
                            const QVariantMap m = p.toMap();
                            ids << QStringLiteral("P:")
                                   + m.value(QStringLiteral("programmeId")).toString()
                                   + QStringLiteral(" T:")
                                   + m.value(QStringLiteral("targetStandardId")).toString()
                                   + QStringLiteral(" N:")
                                   + m.value(QStringLiteral("shotCount")).toString();
                        }
                    }
                }
                delete cat;
            }
            return ids;
        };

        QQmlEngine enEngine;
        const QStringList en = snapshot(enEngine);
        check(en.size() > 50, "SETA-LANG-002: the English snapshot is non-trivial",
              QString::number(en.size()));

        QTranslator deShip;
        const bool loaded = deShip.load(
            QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.qm"));
        check(loaded, "SETA-LANG-002: the SHIPPED German catalogue loads");
        if (loaded) QCoreApplication::installTranslator(&deShip);
        QQmlEngine deEngine;
        deEngine.retranslate();
        const QStringList de = snapshot(deEngine);
        if (loaded) QCoreApplication::removeTranslator(&deShip);

        check(en == de,
              "SETA-LANG-002: rule set, discipline, target standard, programme id "
              "and shot count are IDENTICAL in German",
              en.size() == de.size() ? QStringLiteral("same size, different content")
                                     : QStringLiteral("size %1 vs %2")
                                           .arg(en.size()).arg(de.size()));

        // The German catalogue must actually contain the selector's strings -
        // otherwise "German passes" only because German is still English.
        const QString ts = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.ts"));
        const char* mustTranslate[] = {
            "SELECT RULE SET", "SELECT DISCIPLINE", "SELECT PROGRAMME",
            "Rule set", "Discipline", "Programme", "&lt; Back",
            "Official competition rules", "Practice - no rule authority",
            "Official course", "Preset"
        };
        // Scoped to the SELECTOR's own context: the same English word can
        // appear in several contexts, and the first match in the file is not
        // necessarily the one this screen uses.
        const int ctxAt  = ts.indexOf(QStringLiteral("<name>SetaCompetitionSelector</name>"));
        const int ctxEnd = ts.indexOf(QStringLiteral("</context>"), ctxAt);
        const QString ctx = (ctxAt >= 0) ? ts.mid(ctxAt, ctxEnd - ctxAt) : QString();
        check(!ctx.isEmpty(), "SETA-LANG-002: the selector has its own translation context");
        int translated = 0;
        for (const char* src : mustTranslate) {
            const int at = ctx.indexOf(QStringLiteral("<source>%1</source>").arg(
                                           QLatin1String(src)));
            if (at < 0) continue;
            const int end = ctx.indexOf(QStringLiteral("</message>"), at);
            const QString msg = ctx.mid(at, end - at);
            if (!msg.contains(QStringLiteral("type=\"unfinished\""))) ++translated;
        }
        check(translated == int(sizeof(mustTranslate) / sizeof(mustTranslate[0])),
              "SETA-LANG-002: every selector string has a German translation",
              QString::number(translated) + QStringLiteral("/")
                  + QString::number(int(sizeof(mustTranslate) / sizeof(mustTranslate[0]))));

        // And the selector still compares no translated string.
        const QString sel = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/SetaCompetitionSelector.qml"));
        check(!sel.contains(QStringLiteral("=== qsTr(")) && !sel.contains(QStringLiteral("== qsTr(")),
              "SETA-LANG-002: QML-LANG-001 still holds in the selector");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-ID-001 — no user-visible Tech Aim identity survives in a SETA build.
    //
    // Scans STRING LITERALS, with comment lines stripped first: a gate that can
    // pass or fail on a comment is not a gate. Component type names
    // (TechAimDialog) and qrc asset paths are deliberately NOT flagged - they
    // are source identifiers and files, not words an operator ever reads.
    // ─────────────────────────────────────────────────────────────────────
    {
        QDir idRoot(QStringLiteral(TECHAIM_SOURCE_DIR));
        const QStringList files =
            idRoot.entryList(QStringList() << QStringLiteral("*.qml"), QDir::Files);
        QStringList offenders;
        for (const QString& fname : files) {
            const QString body = readAll(idRoot.filePath(fname));
            const QStringList lines = body.split(QChar(10));
            for (int i = 0; i < lines.size(); ++i) {
                QString line = lines.at(i);
                const int comment = line.indexOf(QStringLiteral("//"));
                if (comment >= 0) line = line.left(comment);
                // Only what sits INSIDE a double-quoted literal can be shown.
                int from = 0;
                while (true) {
                    const int a = line.indexOf(QLatin1Char('"'), from);
                    if (a < 0) break;
                    const int b = line.indexOf(QLatin1Char('"'), a + 1);
                    if (b < 0) break;
                    const QString lit = line.mid(a + 1, b - a - 1);
                    from = b + 1;
                    if (lit.startsWith(QStringLiteral("qrc:"))) continue;
                    if (lit.contains(QStringLiteral("Tech Aim"))
                        || lit.contains(QStringLiteral("TechAim")))
                        offenders << (fname + QStringLiteral(":")
                                      + QString::number(i + 1) + QStringLiteral(" \"")
                                      + lit.left(48) + QStringLiteral("\""));
                }
            }
        }
        check(offenders.isEmpty(),
              "SETA-ID-001: no user-visible Tech Aim product identity remains in QML",
              offenders.join(QStringLiteral(" | ")));

        // The footer is the one that was actually shipping the leak, so it is
        // named explicitly rather than left to the sweep above.
        const QString page = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/LoginPage.qml"));
        check(page.contains(QStringLiteral("PRODUCT.displayName + \"  ·  \" + qsTr(\"Electronic target control\")")),
              "SETA-ID-001: the footer takes its product name from ProductIdentity");
        check(!page.contains(QStringLiteral("\"TechAim  ·  Electronic target control\"")),
              "SETA-ID-001: the hardcoded Tech Aim footer literal is gone");

        // Exported PDF file names are user-visible identity too.
        const QString shooting = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ShootingPage.qml"));
        check(shooting.contains(QStringLiteral("function productFilePrefix()"))
              && !shooting.contains(QStringLiteral("\"TechAim_")),
              "SETA-ID-001: exported PDF names derive from the product, not a literal");

        // The C++ side: PDF document metadata and the operator messages that
        // named the product.
        const QString print = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/customprint.cpp"));
        check(!print.contains(QStringLiteral("QStringLiteral(\"Tech Aim "))
              && print.contains(QStringLiteral("ta::app::identity()")),
              "SETA-ID-001: PDF metadata is composed from identity");

        // The LEGAL publisher is a different fact and must NOT have moved.
        const QString ident = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/app/ProductIdentity.cpp"));
        check(ident.contains(QStringLiteral("JAC SHOOTING SOLUTIONS (PTY) LTD")),
              "SETA-ID-001: the legal publisher is unchanged by the identity sweep");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-ID-002 — the Demo pill's SEMANTIC colour is independent of the brand.
    // Colour states WHAT MODE IT IS; the accent states only WHICH IS SELECTED.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString page = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/LoginPage.qml"));
        const int at = page.indexOf(QStringLiteral("// Demo pill"));
        check(at > 0, "SETA-ID-002: the Demo pill is locatable");
        const QString pill = page.mid(at, 1800);
        check(pill.contains(QStringLiteral("border.color: !opModeRow.opLive ? theme.tokens.errorText")),
              "SETA-ID-002: the Demo border is the ERROR token, not the brand accent");
        check(pill.contains(QStringLiteral("color: !opModeRow.opLive ? theme.tokens.errorText")),
              "SETA-ID-002: the Demo label is the ERROR token, not the brand accent");
        check(pill.contains(QStringLiteral("width: 4; radius: 2")),
              "SETA-ID-002: selection is one restrained accent edge strip");
        check(pill.contains(QStringLiteral("color: _errBg")) || pill.contains(QStringLiteral("? _errBg :")),
              "SETA-ID-002: the Demo fill stays the error background");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-003 — the landing screen's German is REAL, and still changes
    // nothing but labels.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString ts = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.ts"));
        struct Want { const char* ctx; const char* src; };
        const Want wanted[] = {
            { "LoginPage", "Start session" },
            { "LoginPage", "Session setup" },
            { "LoginPage", "ATHLETE" },
            { "LoginPage", "Athlete name" },
            { "LoginPage", "OPERATING MODE" },
            { "LoginPage", "Choose an event" },
            { "LoginPage", "Load saved session" },
            { "LoginPage", "Contact us" },
            { "LoginPage", "READY TO START" },
            { "LoginPage", "No athlete entered" },
            { "LoginPage", "Electronic target control" },
            { "TargetStatusPanel", "NO TARGET" },
            { "TargetStatusPanel", "Connect the target USB cable." },
            { "TargetStatusPanel", "Target Connection" },
            { "Header", "ELECTRONIC TARGET" },
        };
        int have = 0;
        QStringList absent;
        for (const Want& w : wanted) {
            const int c = ts.indexOf(QStringLiteral("<name>%1</name>").arg(QLatin1String(w.ctx)));
            if (c < 0) { absent << QLatin1String(w.ctx); continue; }
            const int cEnd = ts.indexOf(QStringLiteral("</context>"), c);
            const QString ctx = ts.mid(c, cEnd - c);
            const int at = ctx.indexOf(QStringLiteral("<source>%1</source>").arg(QLatin1String(w.src)));
            if (at < 0) { absent << QLatin1String(w.src); continue; }
            const int end = ctx.indexOf(QStringLiteral("</message>"), at);
            if (!ctx.mid(at, end - at).contains(QStringLiteral("type=\"unfinished\"")))
                ++have;
            else
                absent << QLatin1String(w.src);
        }
        check(have == int(sizeof(wanted) / sizeof(wanted[0])),
              "SETA-LANG-003: the landing screen's core strings have German",
              absent.join(QStringLiteral(", ")));

        // German is BETA and the surface is NOT fully translated. Assert the
        // honest state rather than a claim of completeness: the language option
        // must still be marked beta.
        const QString lang = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/app/LanguageService.cpp"));
        check(lang.contains(QStringLiteral("QStringLiteral(\"de-DE\"), QStringLiteral(\"Deutsch\"), true")),
              "SETA-LANG-003: German is still declared BETA, not complete");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-004 — German coverage of the ON-SCREEN surface, and the state
    // that must not move with it.
    //
    // Coverage is asserted against the catalogue, not against prose: every
    // remaining unfinished entry in an on-screen context must be one of the
    // deliberately language-neutral values listed here. Anything else is a real
    // gap and fails, which is what stops "German is complete" being a claim
    // somebody can make by editing a document.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString ts = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.ts"));
        check(!ts.isEmpty(), "SETA-LANG-004: the German catalogue is readable");

        // Printed report / PDF views are a SEPARATE surface: fixed A4 geometry,
        // documents rather than controls. They are excluded here and reported
        // as the remaining area rather than silently counted as done.
        const QStringList reportCtx = {
            QStringLiteral("FinalsReportView"), QStringLiteral("CoachPrintView"),
            QStringLiteral("CallDiagnoseReportView"), QStringLiteral("TrainingReportView"),
            QStringLiteral("SummaryReportView"), QStringLiteral("CoachDashboardView"),
            QStringLiteral("CoachDetailedView"), QStringLiteral("PositionTransitionReportView"),
            QStringLiteral("Report3P"), QStringLiteral("Report3PSeries"),
            QStringLiteral("MatchReportInfo"), QStringLiteral("MatchReportView"),
            QStringLiteral("ReportHeader"), QStringLiteral("ReportFooter"),
            QStringLiteral("PdfSeriesPage"), QStringLiteral("SectionTitle"),
            QStringLiteral("FinalsReportTarget"), QStringLiteral("About"),
            QStringLiteral("BusMonitor"), QStringLiteral("MainWindow"),
            QStringLiteral("Settings"), QStringLiteral("SettingsModbusRTU"),
            QStringLiteral("SettingsModbusTCP"), QStringLiteral("ModbusAdapter"),
            QStringLiteral("SerialModbusAdapter"), QStringLiteral("QObject"),
            QStringLiteral("ConnectionError"), QStringLiteral("SeriesComponent"),
            QStringLiteral("IncidentWindow")
        };
        // Units, axis labels, score bands, shot counts, format fragments and the
        // vendored form's designer name. Translating any of these would change a
        // technical value, not a label.
        const QStringList neutral = {
            QStringLiteral(" mm"), QStringLiteral("X:"), QStringLiteral(", Y:"),
            QStringLiteral("—"), QStringLiteral("S"), QStringLiteral("#"),
            QStringLiteral("%1"), QStringLiteral(" (%1)"), QStringLiteral("Form"),
            QStringLiteral("10"), QStringLiteral("15"), QStringLiteral("20"),
            QStringLiteral("30"), QStringLiteral("40"), QStringLiteral("60"),
            QStringLiteral("10s"), QStringLiteral("9s"), QStringLiteral("8s"),
            QStringLiteral("≤7"), QStringLiteral("\n")
        };

        int screenTotal = 0, screenDone = 0, reportGap = 0;
        QStringList realGaps;
        int at = 0;
        while (true) {
            const int c = ts.indexOf(QStringLiteral("<context>"), at);
            if (c < 0) break;
            const int cEnd = ts.indexOf(QStringLiteral("</context>"), c);
            const QString ctx = ts.mid(c, cEnd - c);
            at = cEnd + 1;
            const int n0 = ctx.indexOf(QStringLiteral("<name>")) + 6;
            const QString name = ctx.mid(n0, ctx.indexOf(QStringLiteral("</name>")) - n0);
            const bool isReport = reportCtx.contains(name);

            int m = 0;
            while (true) {
                const int a = ctx.indexOf(QStringLiteral("<message>"), m);
                if (a < 0) break;
                const int b = ctx.indexOf(QStringLiteral("</message>"), a);
                const QString msg = ctx.mid(a, b - a);
                m = b + 1;
                const int s0 = msg.indexOf(QStringLiteral("<source>")) + 8;
                const int s1 = msg.indexOf(QStringLiteral("</source>"));
                if (s0 < 8 || s1 < 0) continue;
                const QString src = msg.mid(s0, s1 - s0);
                const bool unfinished = msg.contains(QStringLiteral("type=\"unfinished\""));
                if (isReport) { if (unfinished) ++reportGap; continue; }
                if (neutral.contains(src)) continue;
                ++screenTotal;
                if (!unfinished) ++screenDone;
                else if (realGaps.size() < 8) realGaps << src.left(40);
            }
        }
        check(screenTotal > 700,
              "SETA-LANG-004: the on-screen surface is large enough to be meaningful",
              QString::number(screenTotal));
        check(realGaps.isEmpty(),
              "SETA-LANG-004: every on-screen string has German",
              QString(QStringLiteral("%1/%2 translated%3"))
                  .arg(screenDone).arg(screenTotal)
                  .arg(realGaps.isEmpty() ? QString()
                                          : QStringLiteral(" - gaps: ")
                                                + realGaps.join(QStringLiteral(" | "))));
        // The printed-report surface is NOT claimed. Asserting it is still
        // outstanding keeps the honest status honest: if someone translates it
        // later this check tells them to update the status document too.
        check(reportGap > 0,
              "SETA-LANG-004: the printed report surface is still outstanding, "
              "so German remains PARTIAL overall",
              QString::number(reportGap) + QStringLiteral(" strings"));

        // German stays BETA regardless of coverage: native technical review is
        // a different question from string count.
        const QString lang = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/app/LanguageService.cpp"));
        check(lang.contains(QStringLiteral("QStringLiteral(\"de-DE\"), QStringLiteral(\"Deutsch\"), true")),
              "SETA-LANG-004: German is still declared BETA");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-005 — switching language moves NOTHING but labels.
    // ─────────────────────────────────────────────────────────────────────
    {
        const auto stateOf = [](QQmlEngine& eng) {
            QQmlComponent comp(&eng, QUrl::fromLocalFile(
                QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
            QObject* cat = comp.create();
            QStringList out;
            if (cat) {
                QVariant all;
                QMetaObject::invokeMethod(cat, "allEntries", Q_RETURN_ARG(QVariant, all));
                for (const QVariant& e : all.toList()) {
                    const QVariantMap m = e.toMap();
                    QVariant cfg;
                    QMetaObject::invokeMethod(cat, "runtimeConfig", Q_RETURN_ARG(QVariant, cfg),
                        Q_ARG(QVariant, m.value(QStringLiteral("programmeId"))));
                    const QVariantMap c = cfg.toMap();
                    out << m.value(QStringLiteral("programmeId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("rulesetId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("disciplineId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("targetStandardId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("distanceM")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("shotCount")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("scoringMode")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("targetFamily")).toString()
                           + QStringLiteral("|") + c.value(QStringLiteral("gameRange")).toString()
                           + QStringLiteral("|") + c.value(QStringLiteral("gameMode")).toString()
                           + QStringLiteral("|") + c.value(QStringLiteral("gameEvent")).toString();
                }
                delete cat;
            }
            return out;
        };

        QQmlEngine e1;
        const QStringList en = stateOf(e1);
        check(en.size() == 61, "SETA-LANG-005: the English state snapshot covers all 61 programmes",
              QString::number(en.size()));

        QTranslator de;
        const bool ok = de.load(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.qm"));
        check(ok, "SETA-LANG-005: the shipped German catalogue loads");
        if (ok) QCoreApplication::installTranslator(&de);
        QQmlEngine e2;
        e2.retranslate();
        const QStringList deState = stateOf(e2);
        if (ok) QCoreApplication::removeTranslator(&de);

        check(en == deState,
              "SETA-LANG-005: programmeId, rulesetId, disciplineId, targetStandardId, "
              "distance, shot count, scoring mode, target family, range, weapon and "
              "event index are ALL identical in German");
    }

    // ─────────────────────────────────────────────────────────────────────
    // DSB-CAT-001 — the DSB 2026 ruleset, as a first-class competition set.
    //
    // Authority: docs/rules/dsb-2026-*.md, Sportordnung 01.01.2026.
    // Every number checked here is a rule value with a page reference; this
    // gate exists so a later edit cannot quietly change a competition.
    // ─────────────────────────────────────────────────────────────────────
    {
        QQmlEngine eng;
        QQmlComponent comp(&eng, QUrl::fromLocalFile(
            QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
        QScopedPointer<QObject> cat(comp.create());
        check(!cat.isNull(), "DSB-CAT-001: catalogue loads", comp.errorString());
        if (cat.isNull()) { /* nothing further to assert */ }
        else {
        const auto def = [&cat](const char* id) {
            QVariant v;
            QMetaObject::invokeMethod(cat.data(), "competitionDefinition",
                                      Q_RETURN_ARG(QVariant, v),
                                      Q_ARG(QVariant, QString::fromLatin1(id)));
            return v.toMap();
        };
        const auto timing = [&cat](const char* id) {
            QVariant v;
            QMetaObject::invokeMethod(cat.data(), "timingFor", Q_RETURN_ARG(QVariant, v),
                                      Q_ARG(QVariant, QString::fromLatin1(id)));
            return v.toMap();
        };
        const auto mins = [](const QVariantMap& t, const char* key) {
            return int(t.value(QLatin1String(key)).toLongLong() / 60000);
        };

        // ── the ruleset exists and is versioned ──────────────────────────
        QVariant all;
        QMetaObject::invokeMethod(cat.data(), "dsbEntries", Q_RETURN_ARG(QVariant, all));
        check(all.toList().size() == 13,
              "DSB-CAT-001: 13 DSB programmes are defined",
              QString::number(all.toList().size()));
        int versioned = 0, contexted = 0, official = 0;
        for (const QVariant& e : all.toList()) {
            const QVariantMap m = e.toMap();
            if (m.value(QStringLiteral("rulesetVersion")).toString() == QLatin1String("2026-01-01")) ++versioned;
            if (!m.value(QStringLiteral("competitionContext")).toString().isEmpty()) ++contexted;
            if (m.value(QStringLiteral("programmeType")).toString() == QLatin1String("OFFICIAL")) ++official;
        }
        check(versioned == 13 && contexted == 13 && official == 13,
              "DSB-CAT-001: every DSB programme is versioned, context-bound and official",
              QString::number(versioned) + QStringLiteral("/")
                  + QString::number(contexted) + QStringLiteral("/")
                  + QString::number(official));

        // ── 1.10 Luftgewehr: shot count -> EST minutes, decimal ──────────
        struct Simple { const char* id; const char* rule; int shots; int match; int prep;
                        const char* scoring; };
        const Simple simple[] = {
            { "dsb.10m.air-rifle.lg20",   "1.10", 20,  30, 15, "DECIMAL" },
            { "dsb.10m.air-rifle.lg40",   "1.10", 40,  50, 15, "DECIMAL" },
            { "dsb.10m.air-rifle.lg60",   "1.10", 60,  75, 15, "DECIMAL" },
            { "dsb.50m.rifle.prone60",    "1.80", 60,  50, 15, "DECIMAL" },
            { "dsb.10m.air-pistol.lp20",  "2.10", 20,  30, 15, "INTEGER" },
            { "dsb.10m.air-pistol.lp40",  "2.10", 40,  50, 15, "INTEGER" },
            { "dsb.10m.air-pistol.lp60",  "2.10", 60,  75, 15, "INTEGER" },
            { "dsb.50m.pistol.p60",       "2.20", 60,  90, 15, "INTEGER" },
            { "dsb.50m.pistol.p30",       "2.20", 30,  55, 15, "INTEGER" },
            { "dsb.50m.rifle.3x20",       "1.40", 60, 105, 15, "INTEGER" },
            { "dsb.50m.rifle.3x40",       "1.60", 120,165, 15, "INTEGER" },
        };
        int ok = 0;
        QStringList wrong;
        for (const Simple& p : simple) {
            const QVariantMap d = def(p.id), t = timing(p.id);
            const bool good =
                d.value(QStringLiteral("ruleNumber")).toString() == QLatin1String(p.rule)
                && d.value(QStringLiteral("shotCount")).toInt() == p.shots
                && d.value(QStringLiteral("scoringMode")).toString() == QLatin1String(p.scoring)
                && t.value(QStringLiteral("timingModel")).toString()
                       == QLatin1String("SINGLE_MATCH_CLOCK")
                && mins(t, "matchMs") == p.match
                && mins(t, "preparationMs") == p.prep
                && t.value(QStringLiteral("preparationPolicy")).toString()
                       == QLatin1String("OUTSIDE_MATCH_TIME")
                && t.value(QStringLiteral("sighterPolicy")).toString()
                       == QLatin1String("UNLIMITED_IN_PREPARATION")
                && t.value(QStringLiteral("positionMs")).toList().isEmpty();
            if (good) ++ok; else wrong << QLatin1String(p.id);
        }
        check(ok == int(sizeof(simple) / sizeof(simple[0])),
              "DSB-CAT-001: every single-master-clock programme carries its rule "
              "number, shot count, EST time, 15 min outside preparation, unlimited "
              "sighters and scoring mode",
              wrong.join(QStringLiteral(", ")));

        // ── 2.20 30-shot time is a RECOMMENDATION, 60-shot is a rule ─────
        check(timing("dsb.50m.pistol.p30").value(QStringLiteral("matchTimeAuthority")).toString()
                  == QLatin1String("RECOMMENDED")
              && timing("dsb.50m.pistol.p60").value(QStringLiteral("matchTimeAuthority")).toString()
                  == QLatin1String("RULE"),
              "DSB-CAT-001: the 30-shot 50 m pistol time is recorded as a recommendation, "
              "the 60-shot time as a rule");

        // ── 1.20: INDEPENDENT position clocks, gated, sighting inside ────
        struct ThreePos { const char* id; const char* variant; int shots;
                          int k; int p; int st; };
        const ThreePos tp[] = {
            { "dsb.10m.air-rifle.3x10", "3x10", 30, 25, 20, 30 },
            { "dsb.10m.air-rifle.3x20", "3x20", 60, 35, 30, 40 },
        };
        int tpOk = 0;
        QStringList tpWrong;
        for (const ThreePos& p : tp) {
            const QVariantMap d = def(p.id), t = timing(p.id);
            const QVariantList pos = t.value(QStringLiteral("positionMs")).toList();
            const QVariantList seq = d.value(QStringLiteral("positions")).toList();
            const QVariantList spp = d.value(QStringLiteral("shotsPerPosition")).toList();
            const bool good =
                d.value(QStringLiteral("ruleNumber")).toString() == QLatin1String("1.20")
                && d.value(QStringLiteral("programmeVariant")).toString() == QLatin1String(p.variant)
                && d.value(QStringLiteral("shotCount")).toInt() == p.shots
                && d.value(QStringLiteral("scoringMode")).toString() == QLatin1String("INTEGER")
                && t.value(QStringLiteral("timingModel")).toString()
                       == QLatin1String("INDEPENDENT_POSITION_CLOCKS")
                && t.value(QStringLiteral("matchMs")).toLongLong() == 0
                && pos.size() == 3
                && pos.at(0).toLongLong() == qint64(p.k)  * 60000
                && pos.at(1).toLongLong() == qint64(p.p)  * 60000
                && pos.at(2).toLongLong() == qint64(p.st) * 60000
                && mins(t, "preparationMs") == 15
                && t.value(QStringLiteral("preparationPolicy")).toString()
                       == QLatin1String("OUTSIDE_MATCH_TIME")
                && t.value(QStringLiteral("sighterPolicy")).toString()
                       == QLatin1String("INSIDE_POSITION_CLOCK")
                && t.value(QStringLiteral("positionTransitionPolicy")).toString()
                       == QLatin1String("GATED_BY_MATCH_CONTROL")
                && seq.size() == 3
                && seq.at(0).toString() == QLatin1String("KNEELING")
                && seq.at(1).toString() == QLatin1String("PRONE")
                && seq.at(2).toString() == QLatin1String("STANDING")
                && spp.size() == 3
                && spp.at(0).toInt() * 3 == p.shots;
            if (good) ++tpOk; else tpWrong << QLatin1String(p.id);
        }
        check(tpOk == 2,
              "DSB-CAT-001: 1.20 3x10 and 3x20 carry independent position clocks "
              "(25/20/30 and 35/30/40), kneeling-prone-standing, a 15 min outside "
              "preparation, sighting INSIDE the position clock and a gated transition",
              tpWrong.join(QStringLiteral(", ")));

        // ── 1.20 has NO master clock; 1.40/1.60 have NO position clocks ──
        check(timing("dsb.10m.air-rifle.3x20").value(QStringLiteral("matchMs")).toLongLong() == 0,
              "DSB-CAT-001: 1.20 exposes no master clock - a caller reading the "
              "wrong field gets 0, never a plausible wrong number");
        check(timing("dsb.50m.rifle.3x20").value(QStringLiteral("positionMs")).toList().isEmpty()
              && timing("dsb.50m.rifle.3x40").value(QStringLiteral("positionMs")).toList().isEmpty(),
              "DSB-CAT-001: 1.40 and 1.60 expose no position clocks");

        // ── target geometry is REUSED, never duplicated ──────────────────
        struct Face { const char* id; const char* std; int dsbNumber; };
        const Face faces[] = {
            { "dsb.10m.air-rifle.lg60",  "issf.10m.air-rifle", 1 },
            { "dsb.10m.air-rifle.3x20",  "issf.10m.air-rifle", 1 },
            { "dsb.50m.rifle.3x20",      "issf.50m.rifle",     3 },
            { "dsb.50m.rifle.3x40",      "issf.50m.rifle",     3 },
            { "dsb.50m.rifle.prone60",   "issf.50m.rifle",     3 },
            { "dsb.50m.pistol.p60",      "issf.50m.pistol",    4 },
            { "dsb.10m.air-pistol.lp60", "issf.10m.air-pistol",7 },
        };
        int faceOk = 0;
        for (const Face& f : faces) {
            const QVariantMap d = def(f.id);
            if (d.value(QStringLiteral("targetStandardId")).toString() == QLatin1String(f.std)
                && d.value(QStringLiteral("dsbTargetNumber")).toInt() == f.dsbNumber)
                ++faceOk;
        }
        check(faceOk == int(sizeof(faces) / sizeof(faces[0])),
              "DSB-CAT-001: every DSB programme SELECTS an existing ISSF target "
              "standard - Scheibe 1/3/4/7 - and defines no new geometry",
              QString::number(faceOk));

        // ── scoring is NOT a property of the ruleset ─────────────────────
        check(def("dsb.10m.air-rifle.lg60").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("DECIMAL")
              && def("dsb.10m.air-rifle.3x20").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("INTEGER"),
              "DSB-CAT-001: DSB is NOT uniformly integer - 1.10 is decimal while "
              "1.20 is whole ring, from the same ruleset");
        check(def("dsb.50m.rifle.prone60").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("DECIMAL")
              && def("dsb.50m.rifle.3x20").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("INTEGER"),
              "DSB-CAT-001: 1.80 is decimal and 1.40 is integer on the same target face");

        // ── the four layers stay separable ───────────────────────────────
        const QVariantMap meta = [&cat]() {
            QVariant v;
            QMetaObject::invokeMethod(cat.data(), "rulesetMetadata", Q_RETURN_ARG(QVariant, v),
                Q_ARG(QVariant, QStringLiteral("dsb.50m.rifle.3x20")));
            return v.toMap();
        }();
        check(meta.value(QStringLiteral("ruleset")).toString() == QLatin1String("dsb")
              && meta.value(QStringLiteral("rulesetVersion")).toString() == QLatin1String("2026-01-01")
              && meta.value(QStringLiteral("ruleNumber")).toString() == QLatin1String("1.40")
              && meta.value(QStringLiteral("programmeVariant")).toString() == QLatin1String("3x20")
              && meta.value(QStringLiteral("competitionContext")).toString() == QLatin1String("DM_2026")
              && meta.value(QStringLiteral("scoringMode")).toString() == QLatin1String("INTEGER"),
              "DSB-CAT-001: ruleset, version, rule number, variant, context and "
              "scoring mode are separately recoverable for the journal and report");

        // ── programmes that are NOT established must not exist ───────────
        QStringList forbidden;
        for (const QVariant& e : all.toList()) {
            const QVariantMap m = e.toMap();
            const QString v = m.value(QStringLiteral("programmeVariant")).toString();
            const QString r = m.value(QStringLiteral("ruleNumber")).toString();
            if (v.contains(QStringLiteral("3x15")))
                forbidden << m.value(QStringLiteral("programmeId")).toString();
            if (r == QLatin1String("1.40") && v == QLatin1String("3x10"))
                forbidden << m.value(QStringLiteral("programmeId")).toString();
        }
        check(forbidden.isEmpty(),
              "DSB-CAT-001: no 10 m 3x15 and no 50 m 1.40 3x10 - neither is "
              "established by the Sportordnung (S-C.2, S-C.3)",
              forbidden.join(QStringLiteral(", ")));

        // ── the hardware-blocked programmes are absent, not faked ────────
        QStringList faked;
        for (const QVariant& e : all.toList()) {
            const QString r = e.toMap().value(QStringLiteral("ruleNumber")).toString();
            if (r == QLatin1String("2.16") || r == QLatin1String("2.17")
                || r == QLatin1String("2.18"))
                faked << r;
        }
        check(faked.isEmpty(),
              "DSB-CAT-001: 2.16 / 2.17 / 2.18 are NOT present - they need falling "
              "targets, a second face and series exposure, and must not be faked "
              "by changing timers only",
              faked.join(QStringLiteral(", ")));

        // ── the legacy ISSF rows are untouched ───────────────────────────
        QVariant legacy;
        QMetaObject::invokeMethod(cat.data(), "entriesFor", Q_RETURN_ARG(QVariant, legacy),
                                  Q_ARG(QVariant, QStringLiteral("game10RangeEventModel")));
        check(legacy.toList().size() == 12
              && legacy.toList().at(5).toMap().value(QStringLiteral("programmeId")).toString()
                     == QLatin1String("issf.10m.air-rifle.qualification60"),
              "DSB-CAT-001: entriesFor() still returns ONLY the index-locked legacy "
              "rows, so no ISSF row moved");
        }
    }

    // ─────────────────────────────────────────────────────────────────────
    // DSB-TIMING-001 — the critical cross-rule test (requirement 28).
    //
    // Three three-position competitions, three different timing structures.
    // If this ever passes by accident, competition behaviour has been inferred
    // from the words "three position" somewhere, which is the exact defect this
    // whole design exists to prevent.
    // ─────────────────────────────────────────────────────────────────────
    {
        QQmlEngine eng;
        QQmlComponent comp(&eng, QUrl::fromLocalFile(
            QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
        QScopedPointer<QObject> cat(comp.create());
        if (!cat.isNull()) {
            const auto timing = [&cat](const char* id) {
                QVariant v;
                QMetaObject::invokeMethod(cat.data(), "timingFor", Q_RETURN_ARG(QVariant, v),
                                          Q_ARG(QVariant, QString::fromLatin1(id)));
                return v.toMap();
            };
            const QVariantMap dsb120 = timing("dsb.10m.air-rifle.3x20");
            const QVariantMap dsb140 = timing("dsb.50m.rifle.3x20");
            const QVariantMap dsb160 = timing("dsb.50m.rifle.3x40");
            const QVariantMap issf3p = timing("issf.50m.rifle.qualification60");

            check(dsb120.value(QStringLiteral("timingModel")).toString()
                      == QLatin1String("INDEPENDENT_POSITION_CLOCKS")
                  && dsb140.value(QStringLiteral("timingModel")).toString()
                      == QLatin1String("SINGLE_MATCH_CLOCK")
                  && dsb160.value(QStringLiteral("timingModel")).toString()
                      == QLatin1String("SINGLE_MATCH_CLOCK"),
                  "DSB-TIMING-001: DSB 1.20 uses independent position clocks while "
                  "DSB 1.40 and 1.60 use one master clock - all three are "
                  "three-position rifle competitions in the same ruleset");

            check(dsb140.value(QStringLiteral("matchMs")).toLongLong() == qint64(105) * 60000
                  && dsb160.value(QStringLiteral("matchMs")).toLongLong() == qint64(165) * 60000,
                  "DSB-TIMING-001: the two master clocks are 105 and 165 minutes");

            // DSB 1.20's three clocks are not one clock split three ways.
            const QVariantList pos = dsb120.value(QStringLiteral("positionMs")).toList();
            qint64 sum = 0;
            for (const QVariant& p : pos) sum += p.toLongLong();
            check(sum == qint64(105) * 60000
                  && dsb120.value(QStringLiteral("matchMs")).toLongLong() == 0,
                  "DSB-TIMING-001: 1.20's position clocks happen to total 105 min, "
                  "and it still exposes NO master clock - the total is a coincidence, "
                  "not a course time");

            // ISSF must be untouched: it declares no DSB timing at all and keeps
            // getting its durations from where it always has.
            check(issf3p.value(QStringLiteral("matchMs")).toLongLong() == 0
                  && issf3p.value(QStringLiteral("preparationMs")).toLongLong() == 0
                  && issf3p.value(QStringLiteral("positionMs")).toList().isEmpty()
                  && issf3p.value(QStringLiteral("preparationPolicy")).toString().isEmpty(),
                  "DSB-TIMING-001: the ISSF 50 m 3P profile declares NO timing here, "
                  "so adding DSB cannot have changed how ISSF is conducted");
        }
    }

    printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
        return 1;
    }
    const QString source = QString::fromUtf8(f.readAll());

    const QString fnTrace = extractFunction(source, QStringLiteral("traceStage"));
    const QString fnZoom  = extractFunction(source, QStringLiteral("triggerAutoZoom"));
    check(!fnTrace.isEmpty(), "traceStage() was found in the real CenterPane.qml");
    check(!fnZoom.isEmpty(),  "triggerAutoZoom() was found in the real CenterPane.qml");

    // ── static guard: the exact RC2a mistake, stated as a rule ────────────
    // triggerAutoZoom must not read a bare `newShootCount`. That identifier is
    // a local var of readDataFromBAckEnd()/onShootCountChanged() and is not in
    // this function's scope.
    check(!fnZoom.contains(QRegularExpression(QStringLiteral("\\bnewShootCount\\b"))),
          "triggerAutoZoom does not reference newShootCount (the RC2a defect)",
          QStringLiteral("it is a local var of two OTHER functions"));
    check(fnZoom.contains(QStringLiteral("shotSeq")),
          "triggerAutoZoom takes the shot sequence as an explicit parameter");

    // Auto-zoom is a display concern; the operational statements must not sit
    // behind it unguarded ever again.
    check(source.contains(QRegularExpression(
              QStringLiteral("try\\s*\\{[^}]*triggerAutoZoom"))),
          "the triggerAutoZoom call site is exception-guarded",
          QStringLiteral("state updates must survive a display failure"));

    // ── executing test: mount the REAL functions and run them ─────────────
    StubModReader stub;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("MODREADER"), &stub);

    const QString harness = QStringLiteral(
        "import QtQml\n"
        "QtObject {\n"
        "    id: paneItem\n"
        "    property bool autoZoomOn: true\n"
        "    property real autoZoomTarget: 2.4\n"
        "    property real autoZoomFactor: 1\n"
        "    property real autoZoomOriginX: 0\n"
        "    property real autoZoomOriginY: 0\n"
        "    property int  autoZoomHold: 0\n"
        "    %1\n"
        "    %2\n"
        "}\n").arg(fnTrace, fnZoom);

    qInstallMessageHandler(messageHandler);
    QQmlComponent comp(&engine);
    comp.setData(harness.toUtf8(), QUrl(QStringLiteral("qrc:/tst_harness.qml")));
    QObject* obj = comp.create();
    qInstallMessageHandler(nullptr);

    check(comp.isReady() && obj != nullptr,
          "the harness carrying the real functions instantiates",
          comp.errorString().trimmed());
    if (!obj) {
        // ─────────────────────────────────────────────────────────────────────
    // SETA-BRAND-001 — the blue theme is a PACKAGE, not a sweep.
    //
    // Two separate risks: that a brand recolour reaches a SEMANTIC colour (a
    // fault stops reading as a fault), and that it reaches SCORING or TARGET
    // presentation (the picture of the shot changes because of branding).
    // Both are asserted against the shipped source, not against prose.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString tokens = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/ui/theme/DesignTokens.qml"));
        QString flatTokens = tokens;
        flatTokens.remove(QLatin1Char(' '));
        check(!tokens.isEmpty(), "SETA-BRAND-001: DesignTokens.qml is readable");
        check(flatTokens.contains(QStringLiteral("accentPrimary:PRODUCT.accentPrimary"))
              && flatTokens.contains(QStringLiteral("accentBright:PRODUCT.accentBright"))
              && flatTokens.contains(QStringLiteral("focusOutline:PRODUCT.focusOutline")),
              "SETA-BRAND-001: the accent comes from the build's brand package");
        check(flatTokens.contains(QStringLiteral("errorText:\"#D0392B\""))
              && flatTokens.contains(QStringLiteral("successText:\"#20C997\""))
              && flatTokens.contains(QStringLiteral("warningText:\"#E8A13D\"")),
              "SETA-BRAND-001: fault / healthy / warning are NOT brand-driven");

        // Semantic and scoring sites deliberately kept literal. Each one means
        // something; recolouring them for branding would change what the
        // operator is being told.
        struct Site { const char* file; const char* needle; const char* why; };
        const Site kept[] = {
            { "/main.qml", "opLive ? \"#2f7d4f\" : \"#e8003d\"",
              "Demo/Live badge - mistaking Demo for Live is a result-integrity risk" },
            { "/RightPanel.qml", "c: \"#e8003d\"",
              "score band <=7 - scoring presentation, not brand" },
            { "/FinalsReportTarget.qml", "\"#a80038\"",
              "shot-score colour" },
            { "/Report3PSeries.qml", "\"#a80038\"",
              "shot-score colour" },
            { "/SettingsPage.qml", "gameRange === 10 ? \"#f2c200\" : \"#e8003d\"",
              "target display colour swatch - shows the TARGET's colours" },
        };
        for (const Site& site : kept) {
            const QString src = readAll(QStringLiteral(TECHAIM_SOURCE_DIR) + QLatin1String(site.file));
            check(src.contains(QLatin1String(site.needle)),
                  "SETA-BRAND-001: semantic/scoring colour kept literal",
                  QLatin1String(site.file) + QStringLiteral(" - ")
                      + QLatin1String(site.why));
        }

        // The scoring authority itself must carry no brand colour at all.
        const QString centre = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml"));
        check(!centre.contains(QStringLiteral("PRODUCT.accentPrimary"))
              || centre.contains(QStringLiteral("calculateShootingSocre")),
              "SETA-BRAND-001: CenterPane still owns the scoring function");
        check(!centre.contains(QStringLiteral("radOf10Ring: PRODUCT"))
              && !centre.contains(QStringLiteral("radOf10Ring: theme")),
              "SETA-BRAND-001: ring geometry is not reachable from the brand");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-002 — language changes LABELS and nothing else.
    // Extends QML-LANG-001 from programmeId to every stable identity the
    // hierarchy exposes: rule set, discipline and target standard.
    // ─────────────────────────────────────────────────────────────────────
    {
        const auto snapshot = [](QQmlEngine& eng) {
            QQmlComponent comp(&eng, QUrl::fromLocalFile(
                QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
            QObject* cat = comp.create();
            QStringList ids;
            if (cat) {
                QVariant rs;
                QMetaObject::invokeMethod(cat, "ruleSets", Q_RETURN_ARG(QVariant, rs),
                                          Q_ARG(QVariant, QVariant()));
                for (const QVariant& r : rs.toList()) {
                    const QString rid = r.toMap().value(QStringLiteral("rulesetId")).toString();
                    ids << QStringLiteral("R:") + rid;
                    QVariant dv;
                    QMetaObject::invokeMethod(cat, "disciplines", Q_RETURN_ARG(QVariant, dv),
                                              Q_ARG(QVariant, rid), Q_ARG(QVariant, QVariant()));
                    for (const QVariant& d : dv.toList()) {
                        const QString did = d.toMap().value(QStringLiteral("disciplineId")).toString();
                        ids << QStringLiteral("D:") + rid + QLatin1Char('/') + did
                               + QStringLiteral(" T:")
                               + d.toMap().value(QStringLiteral("targetStandardId")).toString();
                        QVariant pv;
                        QMetaObject::invokeMethod(cat, "programmes", Q_RETURN_ARG(QVariant, pv),
                                                  Q_ARG(QVariant, rid), Q_ARG(QVariant, did),
                                                  Q_ARG(QVariant, QVariant()));
                        for (const QVariant& p : pv.toList()) {
                            const QVariantMap m = p.toMap();
                            ids << QStringLiteral("P:")
                                   + m.value(QStringLiteral("programmeId")).toString()
                                   + QStringLiteral(" T:")
                                   + m.value(QStringLiteral("targetStandardId")).toString()
                                   + QStringLiteral(" N:")
                                   + m.value(QStringLiteral("shotCount")).toString();
                        }
                    }
                }
                delete cat;
            }
            return ids;
        };

        QQmlEngine enEngine;
        const QStringList en = snapshot(enEngine);
        check(en.size() > 50, "SETA-LANG-002: the English snapshot is non-trivial",
              QString::number(en.size()));

        QTranslator deShip;
        const bool loaded = deShip.load(
            QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.qm"));
        check(loaded, "SETA-LANG-002: the SHIPPED German catalogue loads");
        if (loaded) QCoreApplication::installTranslator(&deShip);
        QQmlEngine deEngine;
        deEngine.retranslate();
        const QStringList de = snapshot(deEngine);
        if (loaded) QCoreApplication::removeTranslator(&deShip);

        check(en == de,
              "SETA-LANG-002: rule set, discipline, target standard, programme id "
              "and shot count are IDENTICAL in German",
              en.size() == de.size() ? QStringLiteral("same size, different content")
                                     : QStringLiteral("size %1 vs %2")
                                           .arg(en.size()).arg(de.size()));

        // The German catalogue must actually contain the selector's strings -
        // otherwise "German passes" only because German is still English.
        const QString ts = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.ts"));
        const char* mustTranslate[] = {
            "SELECT RULE SET", "SELECT DISCIPLINE", "SELECT PROGRAMME",
            "Rule set", "Discipline", "Programme", "&lt; Back",
            "Official competition rules", "Practice - no rule authority",
            "Official course", "Preset"
        };
        // Scoped to the SELECTOR's own context: the same English word can
        // appear in several contexts, and the first match in the file is not
        // necessarily the one this screen uses.
        const int ctxAt  = ts.indexOf(QStringLiteral("<name>SetaCompetitionSelector</name>"));
        const int ctxEnd = ts.indexOf(QStringLiteral("</context>"), ctxAt);
        const QString ctx = (ctxAt >= 0) ? ts.mid(ctxAt, ctxEnd - ctxAt) : QString();
        check(!ctx.isEmpty(), "SETA-LANG-002: the selector has its own translation context");
        int translated = 0;
        for (const char* src : mustTranslate) {
            const int at = ctx.indexOf(QStringLiteral("<source>%1</source>").arg(
                                           QLatin1String(src)));
            if (at < 0) continue;
            const int end = ctx.indexOf(QStringLiteral("</message>"), at);
            const QString msg = ctx.mid(at, end - at);
            if (!msg.contains(QStringLiteral("type=\"unfinished\""))) ++translated;
        }
        check(translated == int(sizeof(mustTranslate) / sizeof(mustTranslate[0])),
              "SETA-LANG-002: every selector string has a German translation",
              QString::number(translated) + QStringLiteral("/")
                  + QString::number(int(sizeof(mustTranslate) / sizeof(mustTranslate[0]))));

        // And the selector still compares no translated string.
        const QString sel = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/SetaCompetitionSelector.qml"));
        check(!sel.contains(QStringLiteral("=== qsTr(")) && !sel.contains(QStringLiteral("== qsTr(")),
              "SETA-LANG-002: QML-LANG-001 still holds in the selector");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-ID-001 — no user-visible Tech Aim identity survives in a SETA build.
    //
    // Scans STRING LITERALS, with comment lines stripped first: a gate that can
    // pass or fail on a comment is not a gate. Component type names
    // (TechAimDialog) and qrc asset paths are deliberately NOT flagged - they
    // are source identifiers and files, not words an operator ever reads.
    // ─────────────────────────────────────────────────────────────────────
    {
        QDir idRoot(QStringLiteral(TECHAIM_SOURCE_DIR));
        const QStringList files =
            idRoot.entryList(QStringList() << QStringLiteral("*.qml"), QDir::Files);
        QStringList offenders;
        for (const QString& fname : files) {
            const QString body = readAll(idRoot.filePath(fname));
            const QStringList lines = body.split(QChar(10));
            for (int i = 0; i < lines.size(); ++i) {
                QString line = lines.at(i);
                const int comment = line.indexOf(QStringLiteral("//"));
                if (comment >= 0) line = line.left(comment);
                // Only what sits INSIDE a double-quoted literal can be shown.
                int from = 0;
                while (true) {
                    const int a = line.indexOf(QLatin1Char('"'), from);
                    if (a < 0) break;
                    const int b = line.indexOf(QLatin1Char('"'), a + 1);
                    if (b < 0) break;
                    const QString lit = line.mid(a + 1, b - a - 1);
                    from = b + 1;
                    if (lit.startsWith(QStringLiteral("qrc:"))) continue;
                    if (lit.contains(QStringLiteral("Tech Aim"))
                        || lit.contains(QStringLiteral("TechAim")))
                        offenders << (fname + QStringLiteral(":")
                                      + QString::number(i + 1) + QStringLiteral(" \"")
                                      + lit.left(48) + QStringLiteral("\""));
                }
            }
        }
        check(offenders.isEmpty(),
              "SETA-ID-001: no user-visible Tech Aim product identity remains in QML",
              offenders.join(QStringLiteral(" | ")));

        // The footer is the one that was actually shipping the leak, so it is
        // named explicitly rather than left to the sweep above.
        const QString page = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/LoginPage.qml"));
        check(page.contains(QStringLiteral("PRODUCT.displayName + \"  ·  \" + qsTr(\"Electronic target control\")")),
              "SETA-ID-001: the footer takes its product name from ProductIdentity");
        check(!page.contains(QStringLiteral("\"TechAim  ·  Electronic target control\"")),
              "SETA-ID-001: the hardcoded Tech Aim footer literal is gone");

        // Exported PDF file names are user-visible identity too.
        const QString shooting = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ShootingPage.qml"));
        check(shooting.contains(QStringLiteral("function productFilePrefix()"))
              && !shooting.contains(QStringLiteral("\"TechAim_")),
              "SETA-ID-001: exported PDF names derive from the product, not a literal");

        // The C++ side: PDF document metadata and the operator messages that
        // named the product.
        const QString print = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/customprint.cpp"));
        check(!print.contains(QStringLiteral("QStringLiteral(\"Tech Aim "))
              && print.contains(QStringLiteral("ta::app::identity()")),
              "SETA-ID-001: PDF metadata is composed from identity");

        // The LEGAL publisher is a different fact and must NOT have moved.
        const QString ident = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/app/ProductIdentity.cpp"));
        check(ident.contains(QStringLiteral("JAC SHOOTING SOLUTIONS (PTY) LTD")),
              "SETA-ID-001: the legal publisher is unchanged by the identity sweep");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-ID-002 — the Demo pill's SEMANTIC colour is independent of the brand.
    // Colour states WHAT MODE IT IS; the accent states only WHICH IS SELECTED.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString page = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/LoginPage.qml"));
        const int at = page.indexOf(QStringLiteral("// Demo pill"));
        check(at > 0, "SETA-ID-002: the Demo pill is locatable");
        const QString pill = page.mid(at, 1800);
        check(pill.contains(QStringLiteral("border.color: !opModeRow.opLive ? theme.tokens.errorText")),
              "SETA-ID-002: the Demo border is the ERROR token, not the brand accent");
        check(pill.contains(QStringLiteral("color: !opModeRow.opLive ? theme.tokens.errorText")),
              "SETA-ID-002: the Demo label is the ERROR token, not the brand accent");
        check(pill.contains(QStringLiteral("width: 4; radius: 2")),
              "SETA-ID-002: selection is one restrained accent edge strip");
        check(pill.contains(QStringLiteral("color: _errBg")) || pill.contains(QStringLiteral("? _errBg :")),
              "SETA-ID-002: the Demo fill stays the error background");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-003 — the landing screen's German is REAL, and still changes
    // nothing but labels.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString ts = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.ts"));
        struct Want { const char* ctx; const char* src; };
        const Want wanted[] = {
            { "LoginPage", "Start session" },
            { "LoginPage", "Session setup" },
            { "LoginPage", "ATHLETE" },
            { "LoginPage", "Athlete name" },
            { "LoginPage", "OPERATING MODE" },
            { "LoginPage", "Choose an event" },
            { "LoginPage", "Load saved session" },
            { "LoginPage", "Contact us" },
            { "LoginPage", "READY TO START" },
            { "LoginPage", "No athlete entered" },
            { "LoginPage", "Electronic target control" },
            { "TargetStatusPanel", "NO TARGET" },
            { "TargetStatusPanel", "Connect the target USB cable." },
            { "TargetStatusPanel", "Target Connection" },
            { "Header", "ELECTRONIC TARGET" },
        };
        int have = 0;
        QStringList absent;
        for (const Want& w : wanted) {
            const int c = ts.indexOf(QStringLiteral("<name>%1</name>").arg(QLatin1String(w.ctx)));
            if (c < 0) { absent << QLatin1String(w.ctx); continue; }
            const int cEnd = ts.indexOf(QStringLiteral("</context>"), c);
            const QString ctx = ts.mid(c, cEnd - c);
            const int at = ctx.indexOf(QStringLiteral("<source>%1</source>").arg(QLatin1String(w.src)));
            if (at < 0) { absent << QLatin1String(w.src); continue; }
            const int end = ctx.indexOf(QStringLiteral("</message>"), at);
            if (!ctx.mid(at, end - at).contains(QStringLiteral("type=\"unfinished\"")))
                ++have;
            else
                absent << QLatin1String(w.src);
        }
        check(have == int(sizeof(wanted) / sizeof(wanted[0])),
              "SETA-LANG-003: the landing screen's core strings have German",
              absent.join(QStringLiteral(", ")));

        // German is BETA and the surface is NOT fully translated. Assert the
        // honest state rather than a claim of completeness: the language option
        // must still be marked beta.
        const QString lang = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/app/LanguageService.cpp"));
        check(lang.contains(QStringLiteral("QStringLiteral(\"de-DE\"), QStringLiteral(\"Deutsch\"), true")),
              "SETA-LANG-003: German is still declared BETA, not complete");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-004 — German coverage of the ON-SCREEN surface, and the state
    // that must not move with it.
    //
    // Coverage is asserted against the catalogue, not against prose: every
    // remaining unfinished entry in an on-screen context must be one of the
    // deliberately language-neutral values listed here. Anything else is a real
    // gap and fails, which is what stops "German is complete" being a claim
    // somebody can make by editing a document.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString ts = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.ts"));
        check(!ts.isEmpty(), "SETA-LANG-004: the German catalogue is readable");

        // Printed report / PDF views are a SEPARATE surface: fixed A4 geometry,
        // documents rather than controls. They are excluded here and reported
        // as the remaining area rather than silently counted as done.
        const QStringList reportCtx = {
            QStringLiteral("FinalsReportView"), QStringLiteral("CoachPrintView"),
            QStringLiteral("CallDiagnoseReportView"), QStringLiteral("TrainingReportView"),
            QStringLiteral("SummaryReportView"), QStringLiteral("CoachDashboardView"),
            QStringLiteral("CoachDetailedView"), QStringLiteral("PositionTransitionReportView"),
            QStringLiteral("Report3P"), QStringLiteral("Report3PSeries"),
            QStringLiteral("MatchReportInfo"), QStringLiteral("MatchReportView"),
            QStringLiteral("ReportHeader"), QStringLiteral("ReportFooter"),
            QStringLiteral("PdfSeriesPage"), QStringLiteral("SectionTitle"),
            QStringLiteral("FinalsReportTarget"), QStringLiteral("About"),
            QStringLiteral("BusMonitor"), QStringLiteral("MainWindow"),
            QStringLiteral("Settings"), QStringLiteral("SettingsModbusRTU"),
            QStringLiteral("SettingsModbusTCP"), QStringLiteral("ModbusAdapter"),
            QStringLiteral("SerialModbusAdapter"), QStringLiteral("QObject"),
            QStringLiteral("ConnectionError"), QStringLiteral("SeriesComponent"),
            QStringLiteral("IncidentWindow")
        };
        // Units, axis labels, score bands, shot counts, format fragments and the
        // vendored form's designer name. Translating any of these would change a
        // technical value, not a label.
        const QStringList neutral = {
            QStringLiteral(" mm"), QStringLiteral("X:"), QStringLiteral(", Y:"),
            QStringLiteral("—"), QStringLiteral("S"), QStringLiteral("#"),
            QStringLiteral("%1"), QStringLiteral(" (%1)"), QStringLiteral("Form"),
            QStringLiteral("10"), QStringLiteral("15"), QStringLiteral("20"),
            QStringLiteral("30"), QStringLiteral("40"), QStringLiteral("60"),
            QStringLiteral("10s"), QStringLiteral("9s"), QStringLiteral("8s"),
            QStringLiteral("≤7"), QStringLiteral("\n")
        };

        int screenTotal = 0, screenDone = 0, reportGap = 0;
        QStringList realGaps;
        int at = 0;
        while (true) {
            const int c = ts.indexOf(QStringLiteral("<context>"), at);
            if (c < 0) break;
            const int cEnd = ts.indexOf(QStringLiteral("</context>"), c);
            const QString ctx = ts.mid(c, cEnd - c);
            at = cEnd + 1;
            const int n0 = ctx.indexOf(QStringLiteral("<name>")) + 6;
            const QString name = ctx.mid(n0, ctx.indexOf(QStringLiteral("</name>")) - n0);
            const bool isReport = reportCtx.contains(name);

            int m = 0;
            while (true) {
                const int a = ctx.indexOf(QStringLiteral("<message>"), m);
                if (a < 0) break;
                const int b = ctx.indexOf(QStringLiteral("</message>"), a);
                const QString msg = ctx.mid(a, b - a);
                m = b + 1;
                const int s0 = msg.indexOf(QStringLiteral("<source>")) + 8;
                const int s1 = msg.indexOf(QStringLiteral("</source>"));
                if (s0 < 8 || s1 < 0) continue;
                const QString src = msg.mid(s0, s1 - s0);
                const bool unfinished = msg.contains(QStringLiteral("type=\"unfinished\""));
                if (isReport) { if (unfinished) ++reportGap; continue; }
                if (neutral.contains(src)) continue;
                ++screenTotal;
                if (!unfinished) ++screenDone;
                else if (realGaps.size() < 8) realGaps << src.left(40);
            }
        }
        check(screenTotal > 700,
              "SETA-LANG-004: the on-screen surface is large enough to be meaningful",
              QString::number(screenTotal));
        check(realGaps.isEmpty(),
              "SETA-LANG-004: every on-screen string has German",
              QString(QStringLiteral("%1/%2 translated%3"))
                  .arg(screenDone).arg(screenTotal)
                  .arg(realGaps.isEmpty() ? QString()
                                          : QStringLiteral(" - gaps: ")
                                                + realGaps.join(QStringLiteral(" | "))));
        // The printed-report surface is NOT claimed. Asserting it is still
        // outstanding keeps the honest status honest: if someone translates it
        // later this check tells them to update the status document too.
        check(reportGap > 0,
              "SETA-LANG-004: the printed report surface is still outstanding, "
              "so German remains PARTIAL overall",
              QString::number(reportGap) + QStringLiteral(" strings"));

        // German stays BETA regardless of coverage: native technical review is
        // a different question from string count.
        const QString lang = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/app/LanguageService.cpp"));
        check(lang.contains(QStringLiteral("QStringLiteral(\"de-DE\"), QStringLiteral(\"Deutsch\"), true")),
              "SETA-LANG-004: German is still declared BETA");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-005 — switching language moves NOTHING but labels.
    // ─────────────────────────────────────────────────────────────────────
    {
        const auto stateOf = [](QQmlEngine& eng) {
            QQmlComponent comp(&eng, QUrl::fromLocalFile(
                QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
            QObject* cat = comp.create();
            QStringList out;
            if (cat) {
                QVariant all;
                QMetaObject::invokeMethod(cat, "allEntries", Q_RETURN_ARG(QVariant, all));
                for (const QVariant& e : all.toList()) {
                    const QVariantMap m = e.toMap();
                    QVariant cfg;
                    QMetaObject::invokeMethod(cat, "runtimeConfig", Q_RETURN_ARG(QVariant, cfg),
                        Q_ARG(QVariant, m.value(QStringLiteral("programmeId"))));
                    const QVariantMap c = cfg.toMap();
                    out << m.value(QStringLiteral("programmeId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("rulesetId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("disciplineId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("targetStandardId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("distanceM")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("shotCount")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("scoringMode")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("targetFamily")).toString()
                           + QStringLiteral("|") + c.value(QStringLiteral("gameRange")).toString()
                           + QStringLiteral("|") + c.value(QStringLiteral("gameMode")).toString()
                           + QStringLiteral("|") + c.value(QStringLiteral("gameEvent")).toString();
                }
                delete cat;
            }
            return out;
        };

        QQmlEngine e1;
        const QStringList en = stateOf(e1);
        check(en.size() == 61, "SETA-LANG-005: the English state snapshot covers all 61 programmes",
              QString::number(en.size()));

        QTranslator de;
        const bool ok = de.load(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.qm"));
        check(ok, "SETA-LANG-005: the shipped German catalogue loads");
        if (ok) QCoreApplication::installTranslator(&de);
        QQmlEngine e2;
        e2.retranslate();
        const QStringList deState = stateOf(e2);
        if (ok) QCoreApplication::removeTranslator(&de);

        check(en == deState,
              "SETA-LANG-005: programmeId, rulesetId, disciplineId, targetStandardId, "
              "distance, shot count, scoring mode, target family, range, weapon and "
              "event index are ALL identical in German");
    }

    // ─────────────────────────────────────────────────────────────────────
    // DSB-CAT-001 — the DSB 2026 ruleset, as a first-class competition set.
    //
    // Authority: docs/rules/dsb-2026-*.md, Sportordnung 01.01.2026.
    // Every number checked here is a rule value with a page reference; this
    // gate exists so a later edit cannot quietly change a competition.
    // ─────────────────────────────────────────────────────────────────────
    {
        QQmlEngine eng;
        QQmlComponent comp(&eng, QUrl::fromLocalFile(
            QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
        QScopedPointer<QObject> cat(comp.create());
        check(!cat.isNull(), "DSB-CAT-001: catalogue loads", comp.errorString());
        if (cat.isNull()) { /* nothing further to assert */ }
        else {
        const auto def = [&cat](const char* id) {
            QVariant v;
            QMetaObject::invokeMethod(cat.data(), "competitionDefinition",
                                      Q_RETURN_ARG(QVariant, v),
                                      Q_ARG(QVariant, QString::fromLatin1(id)));
            return v.toMap();
        };
        const auto timing = [&cat](const char* id) {
            QVariant v;
            QMetaObject::invokeMethod(cat.data(), "timingFor", Q_RETURN_ARG(QVariant, v),
                                      Q_ARG(QVariant, QString::fromLatin1(id)));
            return v.toMap();
        };
        const auto mins = [](const QVariantMap& t, const char* key) {
            return int(t.value(QLatin1String(key)).toLongLong() / 60000);
        };

        // ── the ruleset exists and is versioned ──────────────────────────
        QVariant all;
        QMetaObject::invokeMethod(cat.data(), "dsbEntries", Q_RETURN_ARG(QVariant, all));
        check(all.toList().size() == 13,
              "DSB-CAT-001: 13 DSB programmes are defined",
              QString::number(all.toList().size()));
        int versioned = 0, contexted = 0, official = 0;
        for (const QVariant& e : all.toList()) {
            const QVariantMap m = e.toMap();
            if (m.value(QStringLiteral("rulesetVersion")).toString() == QLatin1String("2026-01-01")) ++versioned;
            if (!m.value(QStringLiteral("competitionContext")).toString().isEmpty()) ++contexted;
            if (m.value(QStringLiteral("programmeType")).toString() == QLatin1String("OFFICIAL")) ++official;
        }
        check(versioned == 13 && contexted == 13 && official == 13,
              "DSB-CAT-001: every DSB programme is versioned, context-bound and official",
              QString::number(versioned) + QStringLiteral("/")
                  + QString::number(contexted) + QStringLiteral("/")
                  + QString::number(official));

        // ── 1.10 Luftgewehr: shot count -> EST minutes, decimal ──────────
        struct Simple { const char* id; const char* rule; int shots; int match; int prep;
                        const char* scoring; };
        const Simple simple[] = {
            { "dsb.10m.air-rifle.lg20",   "1.10", 20,  30, 15, "DECIMAL" },
            { "dsb.10m.air-rifle.lg40",   "1.10", 40,  50, 15, "DECIMAL" },
            { "dsb.10m.air-rifle.lg60",   "1.10", 60,  75, 15, "DECIMAL" },
            { "dsb.50m.rifle.prone60",    "1.80", 60,  50, 15, "DECIMAL" },
            { "dsb.10m.air-pistol.lp20",  "2.10", 20,  30, 15, "INTEGER" },
            { "dsb.10m.air-pistol.lp40",  "2.10", 40,  50, 15, "INTEGER" },
            { "dsb.10m.air-pistol.lp60",  "2.10", 60,  75, 15, "INTEGER" },
            { "dsb.50m.pistol.p60",       "2.20", 60,  90, 15, "INTEGER" },
            { "dsb.50m.pistol.p30",       "2.20", 30,  55, 15, "INTEGER" },
            { "dsb.50m.rifle.3x20",       "1.40", 60, 105, 15, "INTEGER" },
            { "dsb.50m.rifle.3x40",       "1.60", 120,165, 15, "INTEGER" },
        };
        int ok = 0;
        QStringList wrong;
        for (const Simple& p : simple) {
            const QVariantMap d = def(p.id), t = timing(p.id);
            const bool good =
                d.value(QStringLiteral("ruleNumber")).toString() == QLatin1String(p.rule)
                && d.value(QStringLiteral("shotCount")).toInt() == p.shots
                && d.value(QStringLiteral("scoringMode")).toString() == QLatin1String(p.scoring)
                && t.value(QStringLiteral("timingModel")).toString()
                       == QLatin1String("SINGLE_MATCH_CLOCK")
                && mins(t, "matchMs") == p.match
                && mins(t, "preparationMs") == p.prep
                && t.value(QStringLiteral("preparationPolicy")).toString()
                       == QLatin1String("OUTSIDE_MATCH_TIME")
                && t.value(QStringLiteral("sighterPolicy")).toString()
                       == QLatin1String("UNLIMITED_IN_PREPARATION")
                && t.value(QStringLiteral("positionMs")).toList().isEmpty();
            if (good) ++ok; else wrong << QLatin1String(p.id);
        }
        check(ok == int(sizeof(simple) / sizeof(simple[0])),
              "DSB-CAT-001: every single-master-clock programme carries its rule "
              "number, shot count, EST time, 15 min outside preparation, unlimited "
              "sighters and scoring mode",
              wrong.join(QStringLiteral(", ")));

        // ── 2.20 30-shot time is a RECOMMENDATION, 60-shot is a rule ─────
        check(timing("dsb.50m.pistol.p30").value(QStringLiteral("matchTimeAuthority")).toString()
                  == QLatin1String("RECOMMENDED")
              && timing("dsb.50m.pistol.p60").value(QStringLiteral("matchTimeAuthority")).toString()
                  == QLatin1String("RULE"),
              "DSB-CAT-001: the 30-shot 50 m pistol time is recorded as a recommendation, "
              "the 60-shot time as a rule");

        // ── 1.20: INDEPENDENT position clocks, gated, sighting inside ────
        struct ThreePos { const char* id; const char* variant; int shots;
                          int k; int p; int st; };
        const ThreePos tp[] = {
            { "dsb.10m.air-rifle.3x10", "3x10", 30, 25, 20, 30 },
            { "dsb.10m.air-rifle.3x20", "3x20", 60, 35, 30, 40 },
        };
        int tpOk = 0;
        QStringList tpWrong;
        for (const ThreePos& p : tp) {
            const QVariantMap d = def(p.id), t = timing(p.id);
            const QVariantList pos = t.value(QStringLiteral("positionMs")).toList();
            const QVariantList seq = d.value(QStringLiteral("positions")).toList();
            const QVariantList spp = d.value(QStringLiteral("shotsPerPosition")).toList();
            const bool good =
                d.value(QStringLiteral("ruleNumber")).toString() == QLatin1String("1.20")
                && d.value(QStringLiteral("programmeVariant")).toString() == QLatin1String(p.variant)
                && d.value(QStringLiteral("shotCount")).toInt() == p.shots
                && d.value(QStringLiteral("scoringMode")).toString() == QLatin1String("INTEGER")
                && t.value(QStringLiteral("timingModel")).toString()
                       == QLatin1String("INDEPENDENT_POSITION_CLOCKS")
                && t.value(QStringLiteral("matchMs")).toLongLong() == 0
                && pos.size() == 3
                && pos.at(0).toLongLong() == qint64(p.k)  * 60000
                && pos.at(1).toLongLong() == qint64(p.p)  * 60000
                && pos.at(2).toLongLong() == qint64(p.st) * 60000
                && mins(t, "preparationMs") == 15
                && t.value(QStringLiteral("preparationPolicy")).toString()
                       == QLatin1String("OUTSIDE_MATCH_TIME")
                && t.value(QStringLiteral("sighterPolicy")).toString()
                       == QLatin1String("INSIDE_POSITION_CLOCK")
                && t.value(QStringLiteral("positionTransitionPolicy")).toString()
                       == QLatin1String("GATED_BY_MATCH_CONTROL")
                && seq.size() == 3
                && seq.at(0).toString() == QLatin1String("KNEELING")
                && seq.at(1).toString() == QLatin1String("PRONE")
                && seq.at(2).toString() == QLatin1String("STANDING")
                && spp.size() == 3
                && spp.at(0).toInt() * 3 == p.shots;
            if (good) ++tpOk; else tpWrong << QLatin1String(p.id);
        }
        check(tpOk == 2,
              "DSB-CAT-001: 1.20 3x10 and 3x20 carry independent position clocks "
              "(25/20/30 and 35/30/40), kneeling-prone-standing, a 15 min outside "
              "preparation, sighting INSIDE the position clock and a gated transition",
              tpWrong.join(QStringLiteral(", ")));

        // ── 1.20 has NO master clock; 1.40/1.60 have NO position clocks ──
        check(timing("dsb.10m.air-rifle.3x20").value(QStringLiteral("matchMs")).toLongLong() == 0,
              "DSB-CAT-001: 1.20 exposes no master clock - a caller reading the "
              "wrong field gets 0, never a plausible wrong number");
        check(timing("dsb.50m.rifle.3x20").value(QStringLiteral("positionMs")).toList().isEmpty()
              && timing("dsb.50m.rifle.3x40").value(QStringLiteral("positionMs")).toList().isEmpty(),
              "DSB-CAT-001: 1.40 and 1.60 expose no position clocks");

        // ── target geometry is REUSED, never duplicated ──────────────────
        struct Face { const char* id; const char* std; int dsbNumber; };
        const Face faces[] = {
            { "dsb.10m.air-rifle.lg60",  "issf.10m.air-rifle", 1 },
            { "dsb.10m.air-rifle.3x20",  "issf.10m.air-rifle", 1 },
            { "dsb.50m.rifle.3x20",      "issf.50m.rifle",     3 },
            { "dsb.50m.rifle.3x40",      "issf.50m.rifle",     3 },
            { "dsb.50m.rifle.prone60",   "issf.50m.rifle",     3 },
            { "dsb.50m.pistol.p60",      "issf.50m.pistol",    4 },
            { "dsb.10m.air-pistol.lp60", "issf.10m.air-pistol",7 },
        };
        int faceOk = 0;
        for (const Face& f : faces) {
            const QVariantMap d = def(f.id);
            if (d.value(QStringLiteral("targetStandardId")).toString() == QLatin1String(f.std)
                && d.value(QStringLiteral("dsbTargetNumber")).toInt() == f.dsbNumber)
                ++faceOk;
        }
        check(faceOk == int(sizeof(faces) / sizeof(faces[0])),
              "DSB-CAT-001: every DSB programme SELECTS an existing ISSF target "
              "standard - Scheibe 1/3/4/7 - and defines no new geometry",
              QString::number(faceOk));

        // ── scoring is NOT a property of the ruleset ─────────────────────
        check(def("dsb.10m.air-rifle.lg60").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("DECIMAL")
              && def("dsb.10m.air-rifle.3x20").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("INTEGER"),
              "DSB-CAT-001: DSB is NOT uniformly integer - 1.10 is decimal while "
              "1.20 is whole ring, from the same ruleset");
        check(def("dsb.50m.rifle.prone60").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("DECIMAL")
              && def("dsb.50m.rifle.3x20").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("INTEGER"),
              "DSB-CAT-001: 1.80 is decimal and 1.40 is integer on the same target face");

        // ── the four layers stay separable ───────────────────────────────
        const QVariantMap meta = [&cat]() {
            QVariant v;
            QMetaObject::invokeMethod(cat.data(), "rulesetMetadata", Q_RETURN_ARG(QVariant, v),
                Q_ARG(QVariant, QStringLiteral("dsb.50m.rifle.3x20")));
            return v.toMap();
        }();
        check(meta.value(QStringLiteral("ruleset")).toString() == QLatin1String("dsb")
              && meta.value(QStringLiteral("rulesetVersion")).toString() == QLatin1String("2026-01-01")
              && meta.value(QStringLiteral("ruleNumber")).toString() == QLatin1String("1.40")
              && meta.value(QStringLiteral("programmeVariant")).toString() == QLatin1String("3x20")
              && meta.value(QStringLiteral("competitionContext")).toString() == QLatin1String("DM_2026")
              && meta.value(QStringLiteral("scoringMode")).toString() == QLatin1String("INTEGER"),
              "DSB-CAT-001: ruleset, version, rule number, variant, context and "
              "scoring mode are separately recoverable for the journal and report");

        // ── programmes that are NOT established must not exist ───────────
        QStringList forbidden;
        for (const QVariant& e : all.toList()) {
            const QVariantMap m = e.toMap();
            const QString v = m.value(QStringLiteral("programmeVariant")).toString();
            const QString r = m.value(QStringLiteral("ruleNumber")).toString();
            if (v.contains(QStringLiteral("3x15")))
                forbidden << m.value(QStringLiteral("programmeId")).toString();
            if (r == QLatin1String("1.40") && v == QLatin1String("3x10"))
                forbidden << m.value(QStringLiteral("programmeId")).toString();
        }
        check(forbidden.isEmpty(),
              "DSB-CAT-001: no 10 m 3x15 and no 50 m 1.40 3x10 - neither is "
              "established by the Sportordnung (S-C.2, S-C.3)",
              forbidden.join(QStringLiteral(", ")));

        // ── the hardware-blocked programmes are absent, not faked ────────
        QStringList faked;
        for (const QVariant& e : all.toList()) {
            const QString r = e.toMap().value(QStringLiteral("ruleNumber")).toString();
            if (r == QLatin1String("2.16") || r == QLatin1String("2.17")
                || r == QLatin1String("2.18"))
                faked << r;
        }
        check(faked.isEmpty(),
              "DSB-CAT-001: 2.16 / 2.17 / 2.18 are NOT present - they need falling "
              "targets, a second face and series exposure, and must not be faked "
              "by changing timers only",
              faked.join(QStringLiteral(", ")));

        // ── the legacy ISSF rows are untouched ───────────────────────────
        QVariant legacy;
        QMetaObject::invokeMethod(cat.data(), "entriesFor", Q_RETURN_ARG(QVariant, legacy),
                                  Q_ARG(QVariant, QStringLiteral("game10RangeEventModel")));
        check(legacy.toList().size() == 12
              && legacy.toList().at(5).toMap().value(QStringLiteral("programmeId")).toString()
                     == QLatin1String("issf.10m.air-rifle.qualification60"),
              "DSB-CAT-001: entriesFor() still returns ONLY the index-locked legacy "
              "rows, so no ISSF row moved");
        }
    }

    // ─────────────────────────────────────────────────────────────────────
    // DSB-TIMING-001 — the critical cross-rule test (requirement 28).
    //
    // Three three-position competitions, three different timing structures.
    // If this ever passes by accident, competition behaviour has been inferred
    // from the words "three position" somewhere, which is the exact defect this
    // whole design exists to prevent.
    // ─────────────────────────────────────────────────────────────────────
    {
        QQmlEngine eng;
        QQmlComponent comp(&eng, QUrl::fromLocalFile(
            QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
        QScopedPointer<QObject> cat(comp.create());
        if (!cat.isNull()) {
            const auto timing = [&cat](const char* id) {
                QVariant v;
                QMetaObject::invokeMethod(cat.data(), "timingFor", Q_RETURN_ARG(QVariant, v),
                                          Q_ARG(QVariant, QString::fromLatin1(id)));
                return v.toMap();
            };
            const QVariantMap dsb120 = timing("dsb.10m.air-rifle.3x20");
            const QVariantMap dsb140 = timing("dsb.50m.rifle.3x20");
            const QVariantMap dsb160 = timing("dsb.50m.rifle.3x40");
            const QVariantMap issf3p = timing("issf.50m.rifle.qualification60");

            check(dsb120.value(QStringLiteral("timingModel")).toString()
                      == QLatin1String("INDEPENDENT_POSITION_CLOCKS")
                  && dsb140.value(QStringLiteral("timingModel")).toString()
                      == QLatin1String("SINGLE_MATCH_CLOCK")
                  && dsb160.value(QStringLiteral("timingModel")).toString()
                      == QLatin1String("SINGLE_MATCH_CLOCK"),
                  "DSB-TIMING-001: DSB 1.20 uses independent position clocks while "
                  "DSB 1.40 and 1.60 use one master clock - all three are "
                  "three-position rifle competitions in the same ruleset");

            check(dsb140.value(QStringLiteral("matchMs")).toLongLong() == qint64(105) * 60000
                  && dsb160.value(QStringLiteral("matchMs")).toLongLong() == qint64(165) * 60000,
                  "DSB-TIMING-001: the two master clocks are 105 and 165 minutes");

            // DSB 1.20's three clocks are not one clock split three ways.
            const QVariantList pos = dsb120.value(QStringLiteral("positionMs")).toList();
            qint64 sum = 0;
            for (const QVariant& p : pos) sum += p.toLongLong();
            check(sum == qint64(105) * 60000
                  && dsb120.value(QStringLiteral("matchMs")).toLongLong() == 0,
                  "DSB-TIMING-001: 1.20's position clocks happen to total 105 min, "
                  "and it still exposes NO master clock - the total is a coincidence, "
                  "not a course time");

            // ISSF must be untouched: it declares no DSB timing at all and keeps
            // getting its durations from where it always has.
            check(issf3p.value(QStringLiteral("matchMs")).toLongLong() == 0
                  && issf3p.value(QStringLiteral("preparationMs")).toLongLong() == 0
                  && issf3p.value(QStringLiteral("positionMs")).toList().isEmpty()
                  && issf3p.value(QStringLiteral("preparationPolicy")).toString().isEmpty(),
                  "DSB-TIMING-001: the ISSF 50 m 3P profile declares NO timing here, "
                  "so adding DSB cannot have changed how ISSF is conducted");
        }
    }

    printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
        return 1;
    }

    // ── the call that failed in the field ────────────────────────────────
    g_qmlMessages.clear();
    stub.stamps.clear();
    stub.shootCount = 1;

    qInstallMessageHandler(messageHandler);
    const bool invoked = QMetaObject::invokeMethod(
        obj, "triggerAutoZoom",
        Q_ARG(QVariant, QVariant(12.5)),
        Q_ARG(QVariant, QVariant(-7.25)),
        Q_ARG(QVariant, QVariant(1)));
    qInstallMessageHandler(nullptr);

    check(invoked, "triggerAutoZoom(px, py, shotSeq) is invocable");

    const QString joined = g_qmlMessages.join(QLatin1Char('\n'));
    check(!joined.contains(QStringLiteral("ReferenceError")),
          "triggerAutoZoom raises no ReferenceError", joined);
    check(!joined.contains(QStringLiteral("TypeError")),
          "triggerAutoZoom raises no TypeError", joined);
    check(!joined.contains(QStringLiteral("is not defined")),
          "triggerAutoZoom references nothing undefined", joined);
    check(g_qmlMessages.isEmpty(),
          "triggerAutoZoom raises no uncaught QML exception at all", joined);

    // ── it actually zoomed, rather than merely not crashing ──────────────
    check(qFuzzyCompare(obj->property("autoZoomFactor").toReal(), 2.4),
          "auto-zoom executes: autoZoomFactor reaches the target",
          QStringLiteral("got %1").arg(obj->property("autoZoomFactor").toReal()));
    check(obj->property("autoZoomHold").toInt() == 4,
          "auto-zoom executes: the hold timer is armed");
    check(qFuzzyCompare(obj->property("autoZoomOriginX").toReal(), 12.5) &&
          qFuzzyCompare(obj->property("autoZoomOriginY").toReal(), -7.25),
          "auto-zoom centres on the shot coordinates");

    // ── both stamps emitted, correlated to the right shot ────────────────
    check(stub.stamps.contains(QStringLiteral("zoom-requested/1")),
          "zoom-requested is stamped with the correct shot sequence",
          stub.stamps.join(QStringLiteral(", ")));
    check(stub.stamps.contains(QStringLiteral("zoom-started/1")),
          "zoom-started is stamped with the correct shot sequence",
          stub.stamps.join(QStringLiteral(", ")));

    // ── tracing is a bystander: a failing stamp must not abort the zoom ──
    // traceStage swallows its own errors, so even a hostile MODREADER cannot
    // stop autoZoomFactor being assigned.
    g_qmlMessages.clear();
    obj->setProperty("autoZoomFactor", 1.0);
    obj->setProperty("autoZoomHold", 0);
    engine.rootContext()->setContextProperty(QStringLiteral("MODREADER"), nullptr);
    qInstallMessageHandler(messageHandler);
    QMetaObject::invokeMethod(obj, "triggerAutoZoom",
                              Q_ARG(QVariant, QVariant(1.0)),
                              Q_ARG(QVariant, QVariant(2.0)),
                              Q_ARG(QVariant, QVariant(9)));
    qInstallMessageHandler(nullptr);
    check(qFuzzyCompare(obj->property("autoZoomFactor").toReal(), 2.4),
          "a BROKEN tracing bridge does not prevent the zoom",
          QStringLiteral("instrumentation must never abort operational work"));

    engine.rootContext()->setContextProperty(QStringLiteral("MODREADER"), &stub);

    // ── the shot handler's operational ordering ──────────────────────────
    // The field defect was not the zoom itself but what the exception SKIPPED.
    // This models the handler: a display step that throws must not stop
    // backEndShootCount advancing, the series updating, or the match saving.
    const QString ordering = QStringLiteral(
        "import QtQml\n"
        "QtObject {\n"
        "    property int backEndShootCount: 0\n"
        "    property bool seriesUpdated: false\n"
        "    property bool matchSaved: false\n"
        "    function displayStep() { throw new ReferenceError('simulated display failure') }\n"
        "    function handleShot(newShootCount) {\n"
        "        try { displayStep() } catch (e) { }\n"
        "        backEndShootCount = newShootCount\n"
        "        seriesUpdated = true\n"
        "        matchSaved = true\n"
        "    }\n"
        "}\n");
    QQmlComponent ordComp(&engine);
    ordComp.setData(ordering.toUtf8(), QUrl(QStringLiteral("qrc:/tst_ordering.qml")));
    QObject* ord = ordComp.create();
    check(ord != nullptr, "the ordering harness instantiates", ordComp.errorString().trimmed());
    if (ord) {
        QMetaObject::invokeMethod(ord, "handleShot", Q_ARG(QVariant, QVariant(1)));
        check(ord->property("backEndShootCount").toInt() == 1,
              "backEndShootCount advances despite a failing display step",
              QStringLiteral("this is what RC2a skipped, causing re-processing"));
        check(ord->property("seriesUpdated").toBool(),
              "the series update runs despite a failing display step");
        check(ord->property("matchSaved").toBool(),
              "the match-save path runs despite a failing display step");

        // No duplicate processing: with backEndShootCount advanced, the next
        // poll must start at 2, not re-run shot 1.
        QMetaObject::invokeMethod(ord, "handleShot", Q_ARG(QVariant, QVariant(2)));
        check(ord->property("backEndShootCount").toInt() == 2,
              "a second shot advances the counter again - no duplicate processing");
        delete ord;
    }

    delete obj;

    // ── SCORING GEOMETRY ─────────────────────────────────────────────────
    // calculateShootingSocre() is the most correctness-critical function in
    // the codebase and had no coverage at all. These checks assert INTERNAL
    // CONSISTENCY of the ring geometry - they do NOT assert that a constant
    // matches the ISSF rulebook, because docs/issf-rules does not document
    // target-face geometry for any discipline. That gap is recorded in
    // docs/release/scoring-geometry-verification.md and needs the official
    // rule, not a guess in a test.
    {
        printf("\n--- scoring geometry ---\n");
        const QString fnScore =
            extractFunction(source, QStringLiteral("calculateShootingSocre"));
        check(!fnScore.isEmpty(),
              "calculateShootingSocre() was found in the real CenterPane.qml");

        // The text assertions below search the WHOLE file, not the extracted
        // function. extractFunction() brace-matches without stripping
        // comments, and this function contains commented-out closing braces
        // ("//    }"), so the extract terminates early. Each string searched
        // for below occurs exactly once in CenterPane.qml, so the file-wide
        // search is unambiguous. (The extractor is fine for traceStage and
        // triggerAutoZoom, which carry no such comments.)

        // Each discipline's constants, read OUT of the real source so a silent
        // edit to a ring size shows up here.
        struct Geo { const char* name; const char* r2r; const char* r10; };
        const Geo geo[] = {
            { "10 m Air Pistol", "8",   "5.75" },
            { "10 m Air Rifle",  "2.5", "0.25" },
            { "50 m Pistol",     "25",  "25"   },
            { "50 m Rifle",      "8",   "5.2"  },
        };
        for (const Geo& g : geo) {
            const QString pat = QStringLiteral("var r2rDis = %1").arg(g.r2r);
            const QString pr  = QStringLiteral("var radOf10Ring = %1").arg(g.r10);
            check(source.contains(pat) && source.contains(pr),
                  qUtf8Printable(QStringLiteral("%1: ring spacing %2 mm, 10-ring radius %3 mm unchanged").arg(g.name, g.r2r, g.r10)));
        }

        // The shared formula, stated once:
        //     score = 9 + (spacing + r10 + rPellet - radius) / spacing
        // Consequences that must hold for EVERY discipline, whatever the
        // constants are. This is the part a rulebook cannot change.
        auto scoreAt = [](double spacing, double r10, double rPellet, double r) {
            return 9.0 + ((spacing + r10 + rPellet - r) / spacing);
        };
        struct Num { const char* name; double spacing, r10, rPellet; };
        const Num nums[] = {
            { "10 m Air Pistol", 8.0,  5.75, 2.25 },
            { "10 m Air Rifle",  2.5,  0.25, 2.25 },
            { "50 m Pistol",    25.0, 25.0,  2.8  },
            { "50 m Rifle",      8.0,  5.2,  2.8  },
        };
        for (const Num& n : nums) {
            // A pellet edge just touching the 10-ring scores exactly 10.0 -
            // the ISSF "touching counts" convention, and the hinge the whole
            // formula turns on.
            const double atTen = scoreAt(n.spacing, n.r10, n.rPellet,
                                         n.r10 + n.rPellet);
            check(qAbs(atTen - 10.0) < 1e-9,
                  qUtf8Printable(QStringLiteral("%1: a pellet touching the 10-ring scores "
                                 "exactly 10.0").arg(n.name)),
                  QString::number(atTen, 'f', 6));

            // One ring further out is exactly one point lower. If this ever
            // fails, ring width and score step have diverged.
            const double atNine = scoreAt(n.spacing, n.r10, n.rPellet,
                                          n.r10 + n.rPellet + n.spacing);
            check(qAbs(atNine - 9.0) < 1e-9,
                  qUtf8Printable(QStringLiteral(
                      "%1: one ring further out is exactly 9.0").arg(n.name)),
                  QString::number(atNine, 'f', 6));

            // Strictly decreasing with radius - no plateau, no inversion.
            bool monotonic = true;
            double prev = 1e9;
            for (double r = 0.0; r <= 60.0; r += 0.05) {
                const double s = scoreAt(n.spacing, n.r10, n.rPellet, r);
                if (s >= prev) { monotonic = false; break; }
                prev = s;
            }
            check(monotonic,
                  qUtf8Printable(QStringLiteral(
                      "%1: score decreases strictly as the shot moves out - "
                      "never rewards a worse shot").arg(n.name)));

            // Dead centre exceeds the ISSF decimal maximum before clamping,
            // for every discipline. The clamp is therefore load-bearing, not
            // defensive decoration.
            check(scoreAt(n.spacing, n.r10, n.rPellet, 0.0) >= 11.0,
                  qUtf8Printable(QStringLiteral("%1: a centred shot computes >= 11.0 and "
                                 "REQUIRES the clamp").arg(n.name)),
                  QString::number(scoreAt(n.spacing, n.r10, n.rPellet, 0.0),
                                  'f', 4));
        }

        // The clamp itself, in the real source. ISSF decimal maximum is 10.9.
        check(source.contains(QStringLiteral("calculatedSccore = 10.9")),
              "the ISSF decimal maximum 10.9 is enforced in the real function");
        check(source.contains(QRegularExpression(
                  QStringLiteral("calculatedSccore\\s*>=\\s*11"))),
              "the clamp triggers at >= 11, which every discipline reaches at "
              "dead centre");
    }

    // ─────────────────────────────────────────────────────────────────────
    // LANGUAGE INVARIANCE — QML-LANG-001
    //
    // Selecting Deutsch (Beta) made 10 m Air Pistol render and SCORE as a
    // rifle. CenterPane derived `gameMode` by comparing a stored display
    // string against qsTr("PISTOL"). The stored string is captured once out
    // of a ListModel and never retranslates; the qsTr() binding does. In
    // German ("PISTOLE") the two diverged, gameMode fell to false, and
    // calculateShootingSocre() took the rifle branch - wrong ring geometry,
    // wrong bullet radius, wrong SCORE.
    //
    // These checks assert the invariant at the AUTHORITATIVE level, not on
    // pixels: shooting logic derives from the stable discipline enum, and no
    // QML anywhere keys logic off translated text.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString centre   = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml"));
        const QString leftPane = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/LeftPanel.qml"));
        check(!centre.isEmpty() && !leftPane.isEmpty(),
              "QML-LANG-001: CenterPane.qml and LeftPanel.qml can be read");

        // Plain string extraction on purpose: a regex literal here is one
        // stray escape away from matching nothing and passing vacuously.
        const int declAt = centre.indexOf(QStringLiteral("property bool gameMode"));
        check(declAt >= 0, "QML-LANG-001: CenterPane declares a gameMode property");
        QString gameModeExpr;
        if (declAt >= 0) {
            const int colon = centre.indexOf(QLatin1Char(':'), declAt);
            const int eol   = centre.indexOf(QChar(10), declAt);
            if (colon > 0 && eol > colon)
                gameModeExpr = centre.mid(colon + 1, eol - colon - 1).trimmed();
        }
        check(!gameModeExpr.isEmpty(),
              "QML-LANG-001: the gameMode expression was extracted", gameModeExpr);

        check(!gameModeExpr.contains(QStringLiteral("qsTr")),
              "QML-LANG-001: gameMode is NOT derived from a translated string",
              gameModeExpr);
        check(gameModeExpr.contains(QStringLiteral("loginPage.gameMode")),
              "QML-LANG-001: gameMode is derived from the stable discipline enum",
              gameModeExpr);
        check(!gameModeExpr.contains(QStringLiteral("currentGameDisplay")),
              "QML-LANG-001: gameMode does not read a display string",
              gameModeExpr);

        check(!leftPane.contains(QStringLiteral("gameDisplayText2.text ===")),
              "QML-LANG-001: the discipline key is not decided by displayed text");

        QDir root(QStringLiteral(TECHAIM_SOURCE_DIR));
        const QStringList qmlFiles =
            root.entryList(QStringList() << QStringLiteral("*.qml"), QDir::Files);
        QStringList offenders;
        for (const QString& fname : qmlFiles) {
            const QString body = readAll(root.filePath(fname));
            if (body.contains(QStringLiteral("=== qsTr("))
                || body.contains(QStringLiteral("== qsTr("))
                || body.contains(QStringLiteral("!= qsTr(")))
                offenders << fname;
        }
        check(offenders.isEmpty(),
              "QML-LANG-001: no QML compares against a translated string",
              offenders.join(QStringLiteral(", ")));

        QTranslator de;
        const QString qm = QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.qm");
        const bool loaded = de.load(qm);
        check(loaded, "QML-LANG-001: the German catalogue loads", qm);
        if (loaded) {
            // NEGATIVE CONTROL. It must run against a string the SHIPPED
            // catalogue really translates, or it proves nothing. CenterPane's
            // "PISTOL" was that string until QML-LANG-001 removed the compare
            // and lupdate dropped the source with it - so the control moved to
            // the selector's discipline label, which IS translated and IS the
            // text a naive hierarchy would have branched on.
            const QString translated =
                de.translate("SetaCompetitionSelector", "10M AIR RIFLE");
            check(!translated.isEmpty() && translated != QStringLiteral("10M AIR RIFLE"),
                  "QML-LANG-001: German translates the discipline label differently - "
                  "any logic comparing it would flip discipline",
                  translated);
        }

        for (int mode = 0; mode <= 1; ++mode) {
            bool results[2] = { false, false };
            for (int pass = 0; pass < 2; ++pass) {
                if (pass == 1 && loaded) QCoreApplication::installTranslator(&de);
                QQmlEngine langEngine;
                langEngine.retranslate();
                const QString qmlSrc = QStringLiteral(
                    "import QtQuick 2.15\n"
                    "QtObject {\n"
                    "    property QtObject loginPage: QtObject { property int gameMode: %1 }\n"
                    "    property bool gameMode: %2\n"
                    "}\n").arg(mode).arg(gameModeExpr);
                QQmlComponent comp(&langEngine);
                comp.setData(qmlSrc.toUtf8(), QUrl());
                QScopedPointer<QObject> obj(comp.create());
                check(!obj.isNull(),
                      "QML-LANG-001: the real gameMode expression evaluates",
                      comp.errorString());
                if (!obj.isNull())
                    results[pass] = obj->property("gameMode").toBool();
                if (pass == 1 && loaded) QCoreApplication::removeTranslator(&de);
            }
            check(results[0] == results[1],
                  mode == 0 ? "QML-LANG-001: pistol stays pistol in German"
                            : "QML-LANG-001: rifle stays rifle in German");
            check(results[0] == (mode == 0),
                  mode == 0 ? "QML-LANG-001: gameMode is TRUE for the pistol enum"
                            : "QML-LANG-001: gameMode is FALSE for the rifle enum");
        }
    }

    // ─────────────────────────────────────────────────────────────────────
    // SCORING BOUNDARY TRIPLES — ISSF Rule Book 2026 (Edition 2025, Second
    // Print 07/2026, effective 1 July 2026), sections 6.3.3, 6.3.4.2,
    // 6.3.4.3, 6.3.4.6, 7.4, 8.4.
    //
    // The official ring dimensions are now confirmed, so the ring hinges can
    // be asserted against the rulebook rather than against the formula's own
    // internal consistency. Each transition is probed just inside, exactly on,
    // and just outside, with the OFFICIAL per-discipline projectile radius.
    // ─────────────────────────────────────────────────────────────────────
    {
        // Same expression the application evaluates, kept local to this block.
        auto scoreAt = [](double spacing, double r10, double rPellet, double r) {
            return 9.0 + ((spacing + r10 + rPellet - r) / spacing);
        };
        struct Off { const char* name; double spacing, r10, rPellet; };
        const Off offs[] = {
            { "10 m Air Rifle",  2.5, 0.25, 2.25 },   // 0.5 mm 10-ring, 4.5 mm
            { "10 m Air Pistol", 8.0, 5.75, 2.25 },   // 11.5 mm 10-ring, 4.5 mm
            { "50 m Rifle",      8.0, 5.20, 2.80 },   // 10.4 mm 10-ring, 5.6 mm
        };
        const double eps = 0.01;
        for (const Off& o : offs) {
            for (int ring = 10; ring >= 9; --ring) {
                const double d = o.r10 + o.rPellet + (10 - ring) * o.spacing;
                const double inside  = scoreAt(o.spacing, o.r10, o.rPellet, d - eps);
                const double onLine  = scoreAt(o.spacing, o.r10, o.rPellet, d);
                const double outside = scoreAt(o.spacing, o.r10, o.rPellet, d + eps);
                check(qAbs(onLine - ring) < 1e-9,
                      qUtf8Printable(QStringLiteral("%1: the %2-ring boundary scores exactly %2.0 "
                                     "at the official dimension").arg(o.name).arg(ring)),
                      QString::number(onLine, 'f', 6));
                check(inside > onLine,
                      qUtf8Printable(QStringLiteral("%1: just INSIDE the %2-ring scores higher")
                                     .arg(o.name).arg(ring)),
                      QString::number(inside, 'f', 6));
                check(outside < onLine,
                      qUtf8Printable(QStringLiteral("%1: just OUTSIDE the %2-ring scores lower")
                                     .arg(o.name).arg(ring)),
                      QString::number(outside, 'f', 6));
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────
    // SCORING-CAL-001 — the projectile diameter must come from the DISCIPLINE
    //
    // The boundary triples above hardcode the correct per-discipline radii, so
    // they passed while the shipped build scored every 10 m shot with a 5.6 mm
    // projectile: radOfPallet read APPSETTINGS.bullet_diameter(), one
    // process-wide config value defaulting to 5.6 that is never written per
    // discipline, and the deployed config.ini does not define the key at all.
    // These checks bind the PRODUCTION SELECTION PATH, not a copy of it.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString centre  = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml"));
        const QString appSet  = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/appsettings.cpp"));
        check(!centre.isEmpty() && !appSet.isEmpty(),
              "SCORING-CAL-001: CenterPane.qml and appsettings.cpp can be read");

        // The stale process-wide accessor must not appear in the scoring pane
        // at all - not in scoring, not in the marker scale, not in group size.
        check(!centre.contains(QStringLiteral("APPSETTINGS.bullet_diameter()")),
              "SCORING-CAL-001: CenterPane no longer reads the process-wide "
              "bullet_diameter()");
        check(centre.contains(QStringLiteral("APPSETTINGS.projectileDiameterMm(gameRange)")),
              "SCORING-CAL-001: CenterPane selects the projectile by range");

        // Every scoring branch derives its pellet radius from that selector.
        int radAt = 0, radCount = 0, radFromSelector = 0;
        while ((radAt = centre.indexOf(QStringLiteral("var radOfPallet"), radAt)) >= 0) {
            ++radCount;
            const int eol = centre.indexOf(QChar(10), radAt);
            const QString line = centre.mid(radAt, eol - radAt);
            if (line.contains(QStringLiteral("projectileDiameterMm"))) ++radFromSelector;
            radAt = eol;
        }
        check(radCount > 0 && radCount == radFromSelector,
              "SCORING-CAL-001: every radOfPallet comes from the range selector",
              QStringLiteral("%1 of %2").arg(radFromSelector).arg(radCount));

        // The authority itself, in the real C++: 10 m -> 4.5, otherwise 5.6.
        check(appSet.contains(QStringLiteral("projectileDiameterMm")),
              "SCORING-CAL-001: AppSettings exposes projectileDiameterMm()");
        check(appSet.contains(QStringLiteral("rangeMeters == 10 ? 4.5 : 5.6")),
              "SCORING-CAL-001: 10 m resolves to 4.5 mm and 50 m to 5.6 mm");
        // A deliberately configured calibre is still honoured.
        check(appSet.contains(QStringLiteral("m_bulletSizeOverridden")),
              "SCORING-CAL-001: an explicit config bullet_size still overrides");

        // Language cannot reach this decision: the selector takes gameRange,
        // an int, and gameRange is never derived from a translated string.
        check(!centre.contains(QStringLiteral("projectileDiameterMm(qsTr")),
              "SCORING-CAL-001: the projectile selector is not language-derived");

        // Boundary behaviour with the PRODUCTION mapping, plus the negative
        // control: the old 5.6 mm at 10 m must NOT satisfy the 10.0 hinge.
        auto scoreAt = [](double spacing, double r10, double rPellet, double r) {
            return 9.0 + ((spacing + r10 + rPellet - r) / spacing);
        };
        auto pellet = [](int rangeMeters) { return rangeMeters == 10 ? 4.5 : 5.6; };
        struct Disc { const char* name; int range; double spacing, r10; };
        const Disc discs[] = {
            { "10 m Air Rifle",  10, 2.5, 0.25 },
            { "10 m Air Pistol", 10, 8.0, 5.75 },
            { "50 m Rifle",      50, 8.0, 5.20 },
        };
        for (const Disc& d : discs) {
            const double rP = pellet(d.range) / 2.0;
            for (int ring = 10; ring >= 9; --ring) {
                const double at = d.r10 + rP + (10 - ring) * d.spacing;
                check(qAbs(scoreAt(d.spacing, d.r10, rP, at) - ring) < 1e-9,
                      qUtf8Printable(QStringLiteral("SCORING-CAL-001: %1 %2-ring hinge with the "
                                     "production projectile").arg(d.name).arg(ring)),
                      QString::number(rP, 'f', 3));
                check(scoreAt(d.spacing, d.r10, rP, at - 0.01) > ring,
                      qUtf8Printable(QStringLiteral("SCORING-CAL-001: %1 just inside %2")
                                     .arg(d.name).arg(ring)));
                check(scoreAt(d.spacing, d.r10, rP, at + 0.01) < ring,
                      qUtf8Printable(QStringLiteral("SCORING-CAL-001: %1 just outside %2")
                                     .arg(d.name).arg(ring)));
            }
            if (d.range == 10) {
                // Negative control: the shipped 5.6 mm default at 10 m puts the
                // 10.0 hinge in the wrong place. This is the defect, asserted.
                const double wrong = scoreAt(d.spacing, d.r10, 2.8, d.r10 + rP);
                check(wrong > 10.0 + 1e-9,
                      qUtf8Printable(QStringLiteral("SCORING-CAL-001: %1 would OVER-score with "
                                     "the old 5.6 mm default").arg(d.name)),
                      QString::number(wrong, 'f', 6));
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────
    // QML-PARSE-001 — every application QML file must PARSE
    //
    // RC3 shipped an installer whose application exited immediately with -1:
    // engine.load("qrc:/main.qml") produced no root objects because
    // LoginPage.qml had a syntax error - a "//" comment inserted mid-line
    // swallowed the rest of a single-line Image{...} block, including its
    // closing brace. Nothing caught it: rcc embeds QML as bytes without
    // parsing it, the compiler never sees it, and no suite loaded the file.
    //
    // This parses every root .qml through the QML engine and fails on
    // PARSE-level errors only. Type and context errors ("X is not a type",
    // "MODREADER is not defined") are expected here - these files are not
    // being instantiated with the application's context - so they are
    // deliberately ignored. A missing brace is not.
    // ─────────────────────────────────────────────────────────────────────
    {
        QDir root(QStringLiteral(TECHAIM_SOURCE_DIR));
        const QStringList files = root.entryList(QStringList() << QStringLiteral("*.qml"),
                                                 QDir::Files, QDir::Name);
        check(files.size() > 20, "QML-PARSE-001: application QML files found",
              QString::number(files.size()));

        QQmlEngine parseEngine;
        QStringList broken;
        for (const QString& f : files) {
            QQmlComponent c(&parseEngine, QUrl::fromLocalFile(root.filePath(f)));
            for (const QQmlError& e : c.errors()) {
                const QString d = e.description();
                // Parser diagnostics, not resolution diagnostics.
                if (d.contains(QStringLiteral("Expected token"))
                    || d.contains(QStringLiteral("Expected a qualified name id"))
                    || d.contains(QStringLiteral("Unexpected token"))
                    || d.contains(QStringLiteral("Syntax error"))
                    || d.contains(QStringLiteral("Unterminated"))
                    || d.contains(QStringLiteral("Imported file"))) {
                    broken << QStringLiteral("%1:%2 %3").arg(f).arg(e.line()).arg(d);
                }
            }
        }
        check(broken.isEmpty(),
              "QML-PARSE-001: every application QML file parses",
              broken.join(QStringLiteral(" | ")));
    }

    // ─────────────────────────────────────────────────────────────────────
    // CATALOGUE-001 — the competition catalogue seam must change NOTHING
    //
    // Programme definitions moved out of 48 hardcoded ListElements in
    // main.qml into CompetitionCatalogue.qml. main.qml now builds the same
    // four ListModels from it, in the same order, because ShootingPage
    // resolves the user's choice by INDEX.
    //
    // The table below is the inventory as it stood BEFORE the migration.
    // These checks are the before/after proof: same models, same order, same
    // shot counts, same rifle/pistol split.
    // ─────────────────────────────────────────────────────────────────────
    {
        struct Expect { const char* model; int n; const char* counts; const char* pistol; };
        const Expect expected[] = {
            { "game10RangeEventModel",    12, "-1,10,20,30,40,60,-1,10,20,30,40,60", "000000111111" },
            { "game10RangeEventModel_15", 12, "-1,10,15,20,30,40,-1,10,15,20,30,40", "000000111111" },
            { "game50RangeEventModel",    12, "-1,10,20,30,40,60,-1,10,20,30,40,60", "000000111111" },
            { "game50RangeEventModel_15", 12, "-1,10,15,20,30,40,-1,10,15,20,30,40", "000000111111" },
        };

        QQmlEngine catEngine;
        QQmlComponent comp(&catEngine,
                           QUrl::fromLocalFile(QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
        QScopedPointer<QObject> cat(comp.create());
        check(!cat.isNull(), "CATALOGUE-001: CompetitionCatalogue.qml loads", comp.errorString());

        if (!cat.isNull()) {
            QVariant total;
            QMetaObject::invokeMethod(cat.data(), "count", Q_RETURN_ARG(QVariant, total));
            check(total.toInt() == 48, "CATALOGUE-001: 48 programmes, as before the migration",
                  QString::number(total.toInt()));

            QSet<QString> ids;
            for (const Expect& e : expected) {
                QVariant v;
                QMetaObject::invokeMethod(cat.data(), "entriesFor", Q_RETURN_ARG(QVariant, v),
                                          Q_ARG(QVariant, QString::fromLatin1(e.model)));
                const QVariantList rows = v.toList();
                check(rows.size() == e.n,
                      qUtf8Printable(QStringLiteral("CATALOGUE-001: %1 has %2 entries")
                                     .arg(e.model).arg(e.n)),
                      QString::number(rows.size()));

                const QStringList wantCounts = QString::fromLatin1(e.counts).split(QLatin1Char(','));
                const QString wantPistol = QString::fromLatin1(e.pistol);
                bool okCounts = rows.size() == wantCounts.size();
                bool okPistol = rows.size() == wantPistol.size();
                for (int i = 0; i < rows.size(); ++i) {
                    const QVariantMap r = rows.at(i).toMap();
                    if (i < wantCounts.size()
                        && r.value(QStringLiteral("shotCount")).toInt() != wantCounts.at(i).toInt())
                        okCounts = false;
                    if (i < wantPistol.size()
                        && r.value(QStringLiteral("isPistol")).toBool()
                           != (wantPistol.at(i) == QLatin1Char('1')))
                        okPistol = false;
                    ids.insert(r.value(QStringLiteral("programmeId")).toString());
                }
                check(okCounts, qUtf8Printable(QStringLiteral(
                          "CATALOGUE-001: %1 shot counts and ORDER unchanged").arg(e.model)));
                check(okPistol, qUtf8Printable(QStringLiteral(
                          "CATALOGUE-001: %1 rifle/pistol split unchanged").arg(e.model)));
            }

            check(ids.size() == 48, "CATALOGUE-001: every programmeId is unique",
                  QString::number(ids.size()));
            QStringList badIds;
            for (const QString& id : ids) {
                if (id != id.toLower() || id.contains(QLatin1Char(' '))
                    || !(id.startsWith(QStringLiteral("issf."))
                         || id.startsWith(QStringLiteral("techaim."))))
                    badIds << id;
            }
            check(badIds.isEmpty(),
                  "CATALOGUE-001: ids are lowercase, unspaced and authority-scoped (issf.* or "
                  "techaim.*) - machine identity, safe in a session file or an RMS message", badIds.join(QStringLiteral(", ")));

            // Legacy sessions carry no programmeId; identity is recovered from
            // stable numeric state, never from a stored display string.
            QVariant legacy;
            QMetaObject::invokeMethod(cat.data(), "legacyProgrammeId", Q_RETURN_ARG(QVariant, legacy),
                                      Q_ARG(QVariant, 50), Q_ARG(QVariant, false),
                                      Q_ARG(QVariant, 60), Q_ARG(QVariant, false));
            check(legacy.toString() == QStringLiteral("issf.50m.rifle.qualification60"),
                  "CATALOGUE-001: a legacy session maps to a programmeId from numeric state alone",
                  legacy.toString());


            // ── CATALOGUE-002: authority semantics ───────────────────────
            // A preset that shoots on an ISSF target is NOT an ISSF event.
            // Only the 60-shot courses have rule authority; FREE/10/15/20/
            // 30/40 are Tech Aim presets and must not claim otherwise.
            int official = 0, presets = 0, wrongAuthority = 0, missingStandard = 0;
            for (const Expect& e : expected) {
                QVariant v;
                QMetaObject::invokeMethod(cat.data(), "entriesFor", Q_RETURN_ARG(QVariant, v),
                                          Q_ARG(QVariant, QString::fromLatin1(e.model)));
                for (const QVariant& row : v.toList()) {
                    const QVariantMap r = row.toMap();
                    const QString id   = r.value(QStringLiteral("programmeId")).toString();
                    const QString rs   = r.value(QStringLiteral("rulesetId")).toString();
                    const QString fed  = r.value(QStringLiteral("federation")).toString();
                    const QString type = r.value(QStringLiteral("programmeType")).toString();
                    const QString std  = r.value(QStringLiteral("targetStandardId")).toString();
                    const int shots    = r.value(QStringLiteral("shotCount")).toInt();

                    if (!std.startsWith(QStringLiteral("issf."))) ++missingStandard;

                    if (shots == 60) {
                        ++official;
                        if (rs != QStringLiteral("issf") || fed != QStringLiteral("ISSF")
                            || type != QStringLiteral("OFFICIAL")
                            || !id.startsWith(QStringLiteral("issf.")))
                            ++wrongAuthority;
                    } else {
                        ++presets;
                        if (rs != QStringLiteral("techaim") || !fed.isEmpty()
                            || type != QStringLiteral("PRESET")
                            || !id.startsWith(QStringLiteral("techaim.")))
                            ++wrongAuthority;
                    }
                }
            }
            check(official == 4, "CATALOGUE-002: exactly 4 official ISSF 60-shot courses",
                  QString::number(official));
            check(presets == 44, "CATALOGUE-002: the other 44 entries are Tech Aim presets",
                  QString::number(presets));
            check(wrongAuthority == 0,
                  "CATALOGUE-002: no preset claims ISSF authority and no official course "
                  "understates it", QString::number(wrongAuthority));
            check(missingStandard == 0,
                  "CATALOGUE-002: official courses AND presets select the same ISSF target "
                  "standard", QString::number(missingStandard));

            // Language independence, proved by running under a translator.
            QVariant idEn;
            QMetaObject::invokeMethod(cat.data(), "programmeIdAt", Q_RETURN_ARG(QVariant, idEn),
                                      Q_ARG(QVariant, QStringLiteral("game10RangeEventModel")),
                                      Q_ARG(QVariant, 11));
            QTranslator de2;
            const bool loaded2 = de2.load(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.qm"));
            if (loaded2) QCoreApplication::installTranslator(&de2);
            QString idDe;
            {
                QQmlEngine deEngine;
                deEngine.retranslate();
                QQmlComponent c2(&deEngine, QUrl::fromLocalFile(
                                     QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
                QScopedPointer<QObject> cat2(c2.create());
                if (!cat2.isNull()) {
                    QVariant v2;
                    QMetaObject::invokeMethod(cat2.data(), "programmeIdAt", Q_RETURN_ARG(QVariant, v2),
                                              Q_ARG(QVariant, QStringLiteral("game10RangeEventModel")),
                                              Q_ARG(QVariant, 11));
                    idDe = v2.toString();
                }
            }
            if (loaded2) QCoreApplication::removeTranslator(&de2);
            check(!idEn.toString().isEmpty() && idEn.toString() == idDe,
                  "CATALOGUE-001: programmeId is identical in English and German",
                  idEn.toString() + QStringLiteral(" / ") + idDe);
        }

        const QString catSrc = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml"));
        // Only ISSF programmes that already existed. Checked on the DATA -
        // scanning the raw file also matches this file's own explanatory
        // prose, which is exactly the kind of false signal a gate must not
        // carry. Every id is already asserted to start "issf." above; this
        // adds the federation field itself.
        // SUPERSEDED. The original gate asserted that NO federation programme
        // existed, which was right while none was researched. DSB 2026 now
        // does, so the gate asserts the thing that actually protects an
        // athlete: a federation programme may exist ONLY if it carries its
        // authority - rule number, ruleset version and competition context.
        // A federation entry missing any of those is an unverified programme.
        QVariant fedAll;
        QMetaObject::invokeMethod(cat.data(), "allEntries", Q_RETURN_ARG(QVariant, fedAll));
        QStringList unverified;
        for (const QVariant& e : fedAll.toList()) {
            const QVariantMap m = e.toMap();
            if (m.value(QStringLiteral("federation")).toString().isEmpty()) continue;
            if (m.value(QStringLiteral("federation")).toString() == QLatin1String("ISSF")) continue;
            if (m.value(QStringLiteral("ruleNumber")).toString().isEmpty()
                || m.value(QStringLiteral("rulesetVersion")).toString().isEmpty()
                || m.value(QStringLiteral("competitionContext")).toString().isEmpty())
                unverified << m.value(QStringLiteral("programmeId")).toString();
        }
        check(unverified.isEmpty(),
              "CATALOGUE-001: every federation programme carries its rule number, "
              "ruleset version and competition context - no unverified programme",
              unverified.join(QStringLiteral(", ")));
        // The catalogue SELECTS behaviour - it must never carry scoring.
        check(!catSrc.contains(QStringLiteral("radOf10Ring"))
              && !catSrc.contains(QStringLiteral("r2rDis"))
              && !catSrc.contains(QStringLiteral("calculatedSccore")),
              "CATALOGUE-001: the catalogue carries no ring geometry or scoring formula");
        // QML-LANG-001 again: identity must not be derived from display text.
        check(!catSrc.contains(QStringLiteral("=== qsTr(")) && !catSrc.contains(QStringLiteral("== qsTr(")),
              "CATALOGUE-001: no logic in the catalogue compares a translated string");
        // main.qml must no longer own the programme definitions.
        const QString mainSrc = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/main.qml"));
        check(mainSrc.contains(QStringLiteral("competitionCatalogue.fill")),
              "CATALOGUE-001: main.qml builds its models from the catalogue");
        check(!mainSrc.contains(QStringLiteral("10M AIR RIFLE 60")),
              "CATALOGUE-001: the hardcoded programme literals are gone from main.qml");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-SEL-001 — hierarchical selector and SETA identity (product/seta)
    //
    // The selector must be a VIEW of the catalogue, never a second list of
    // programmes. If it could offer something the engine does not know, or
    // lose something the engine does, it would be a divergent source of
    // truth - which is the defect the catalogue seam exists to prevent.
    // ─────────────────────────────────────────────────────────────────────
    {
        QQmlEngine selEngine;
        QQmlComponent selComp(&selEngine,
                              QUrl::fromLocalFile(QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
        QScopedPointer<QObject> sel(selComp.create());
        check(!sel.isNull(), "SETA-SEL-001: catalogue loads for the selector", selComp.errorString());

        if (!sel.isNull()) {
            QVariant v;
            QMetaObject::invokeMethod(sel.data(), "ruleSets", Q_RETURN_ARG(QVariant, v),
                                      Q_ARG(QVariant, QVariant()));   // unfiltered
            const QVariantList sets = v.toList();
            check(sets.size() == 3,
                  "SETA-SEL-001: three rule sets - ISSF, DSB 2026 and the practice presets",
                  QString::number(sets.size()));
            check(!sets.isEmpty() && sets.first().toMap()
                      .value(QStringLiteral("rulesetId")).toString() == QStringLiteral("issf"),
                  "SETA-SEL-001: ISSF is listed first");

            // No DSB rule set may appear until a confirmed profile exists.
            bool sawDsb = false;
            for (const QVariant& r : sets)
                if (r.toMap().value(QStringLiteral("rulesetId")).toString() == QStringLiteral("dsb"))
                    sawDsb = true;
            // SUPERSEDED: DSB profiles now exist and are confirmed against the
            // Sportordnung 01.01.2026, so the rule set MUST be offered. What
            // still may not exist is a programme without authority - asserted
            // by DSB-CAT-001 below.
            check(sawDsb,
                  "SETA-SEL-001: the DSB rule set is offered now that confirmed profiles exist");

            // Every catalogue entry must be reachable through the hierarchy,
            // and the hierarchy must invent nothing.
            int reachable = 0;
            QSet<QString> reachableIds;
            for (const QVariant& rs : sets) {
                const QString rid = rs.toMap().value(QStringLiteral("rulesetId")).toString();
                QVariant dv;
                QMetaObject::invokeMethod(sel.data(), "disciplines", Q_RETURN_ARG(QVariant, dv),
                                          Q_ARG(QVariant, rid), Q_ARG(QVariant, QVariant()));
                for (const QVariant& d : dv.toList()) {
                    const QString did = d.toMap().value(QStringLiteral("disciplineId")).toString();
                    QVariant pv;
                    QMetaObject::invokeMethod(sel.data(), "programmes", Q_RETURN_ARG(QVariant, pv),
                                              Q_ARG(QVariant, rid), Q_ARG(QVariant, did),
                                              Q_ARG(QVariant, QVariant()));
                    for (const QVariant& p : pv.toList()) {
                        ++reachable;
                        reachableIds.insert(p.toMap().value(QStringLiteral("programmeId")).toString());
                    }
                }
            }
            // 48 ISSF/practice entries + 13 DSB 2026 programmes. Stated as
            // two numbers on purpose: if either side moves, this fails.
            check(reachable == 61,
                  "SETA-SEL-001: all 61 catalogue entries are reachable - no programme lost",
                  QString::number(reachable));
            check(reachableIds.size() == 61,
                  "SETA-SEL-001: the hierarchy invents no programme",
                  QString::number(reachableIds.size()));

            // ISSF exposes exactly the four official 60-shot courses.
            QVariant dv;
            QMetaObject::invokeMethod(sel.data(), "disciplines", Q_RETURN_ARG(QVariant, dv),
                                      Q_ARG(QVariant, QStringLiteral("issf")),
                                      Q_ARG(QVariant, QVariant()));
            check(dv.toList().size() == 4,
                  "SETA-SEL-001: ISSF offers four disciplines",
                  QString::number(dv.toList().size()));
            QVariant pv;
            QMetaObject::invokeMethod(sel.data(), "programmes", Q_RETURN_ARG(QVariant, pv),
                                      Q_ARG(QVariant, QStringLiteral("issf")),
                                      Q_ARG(QVariant, QStringLiteral("AR10")),
                                      Q_ARG(QVariant, QVariant()));
            check(pv.toList().size() == 1,
                  "SETA-SEL-001: ISSF 10 m Air Rifle has one programme, so step 3 is skipped",
                  QString::number(pv.toList().size()));
            check(!pv.toList().isEmpty() && pv.toList().first().toMap()
                      .value(QStringLiteral("programmeId")).toString()
                          == QStringLiteral("issf.10m.air-rifle.qualification60"),
                  "SETA-SEL-001: it resolves to the official programmeId");

            // Presets remain selectable - the old shot-count choices are not
            // lost, they moved one level down.
            QMetaObject::invokeMethod(sel.data(), "programmes", Q_RETURN_ARG(QVariant, pv),
                                      Q_ARG(QVariant, QStringLiteral("techaim")),
                                      Q_ARG(QVariant, QStringLiteral("AR10")),
                                      Q_ARG(QVariant, QVariant()));
            check(pv.toList().size() >= 5,
                  "SETA-SEL-001: the Tech Aim presets are still reachable",
                  QString::number(pv.toList().size()));

            // Language independence of the hierarchy itself.
            QTranslator deSel;
            const bool okDe = deSel.load(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.qm"));
            QString idEnSel = pv.toList().isEmpty() ? QString()
                              : pv.toList().first().toMap()
                                    .value(QStringLiteral("programmeId")).toString();
            if (okDe) QCoreApplication::installTranslator(&deSel);
            QString idDeSel;
            {
                QQmlEngine deEng; deEng.retranslate();
                QQmlComponent c3(&deEng, QUrl::fromLocalFile(
                                     QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
                QScopedPointer<QObject> cat3(c3.create());
                if (!cat3.isNull()) {
                    QVariant pv3;
                    QMetaObject::invokeMethod(cat3.data(), "programmes", Q_RETURN_ARG(QVariant, pv3),
                                              Q_ARG(QVariant, QStringLiteral("techaim")),
                                              Q_ARG(QVariant, QStringLiteral("AR10")),
                                              Q_ARG(QVariant, QVariant()));
                    if (!pv3.toList().isEmpty())
                        idDeSel = pv3.toList().first().toMap()
                                      .value(QStringLiteral("programmeId")).toString();
                }
            }
            if (okDe) QCoreApplication::removeTranslator(&deSel);
            check(!idEnSel.isEmpty() && idEnSel == idDeSel,
                  "SETA-SEL-001: the hierarchy resolves the same programmeId in German",
                  idEnSel + QStringLiteral(" / ") + idDeSel);
        }

        // The selector component itself: a view, with no logic on display text.
        const QString selSrc = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/SetaCompetitionSelector.qml"));
        check(!selSrc.isEmpty(), "SETA-SEL-001: SetaCompetitionSelector.qml exists");
        check(!selSrc.contains(QStringLiteral("=== qsTr(")) && !selSrc.contains(QStringLiteral("== qsTr(")),
              "SETA-SEL-001: the selector never compares a translated string");
        check(selSrc.contains(QStringLiteral("catalogue.ruleSets"))
              && selSrc.contains(QStringLiteral("catalogue.disciplines"))
              && selSrc.contains(QStringLiteral("catalogue.programmes")),
              "SETA-SEL-001: every level is driven by the catalogue");
        check(selSrc.contains(QStringLiteral("programmeCommitted")),
              "SETA-SEL-001: selection is committed as a programmeId, once, at the end");
        check(!selSrc.contains(QStringLiteral("radOf10Ring"))
              && !selSrc.contains(QStringLiteral("calculatedSccore")),
              "SETA-SEL-001: the selector carries no scoring");

        // SETA identity: presentation only, behind a build flavour.
        const QString idSrc = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/app/ProductIdentity.cpp"));
        check(idSrc.contains(QStringLiteral("#ifdef BRAND_SETA")),
              "SETA-SEL-001: SETA identity is a build flavour, not a fork");
        check(idSrc.contains(QStringLiteral("qrc:/images/logo/seta.png")),
              "SETA-SEL-001: it uses the existing approved SETA asset");
        const int setaBlock = idSrc.indexOf(QStringLiteral("#ifdef BRAND_SETA"));
        const int setaEnd   = idSrc.indexOf(QStringLiteral("#endif"), setaBlock);
        // Strip comment lines first: the block explains WHICH fields it
        // deliberately leaves alone, and a gate that matches its own prose
        // is not a gate. Assert on the code.
        QString setaBody;
        for (const QString& ln : idSrc.mid(setaBlock, setaEnd - setaBlock)
                                     .split(QChar(10))) {
            const QString t = ln.trimmed();
            if (!t.startsWith(QStringLiteral("//"))) setaBody += ln + QChar(10);
        }
        // SUPERSEDES the earlier "presentation only" rule. That rule was right
        // for a name-and-logo skin and WRONG for a product line: with the
        // storage identity shared, an installed SETA build and an installed
        // Tech Aim build read and write the same athletes, unfinished sessions,
        // recovery journals, reports, logs and remembered target fingerprints.
        // The flavour now also owns the user-data namespace. What it still must
        // NOT touch is the vendor root and the binary/lock identity.
        check(setaBody.contains(QStringLiteral("applicationStorageName")),
              "SETA-SEL-001: the SETA build takes its OWN user-data namespace");
        check(!setaBody.contains(QStringLiteral("organisationName"))
              && !setaBody.contains(QStringLiteral("executableBaseName")),
              "SETA-SEL-001: the vendor root and the single-instance lock stay shared");
        check(!setaBody.contains(QStringLiteral("legalPublisher")),
              "SETA-SEL-001: branding never re-attributes the legal publisher");
        // No silent migration: a fresh SETA install starts clean.
        check(!idSrc.contains(QStringLiteral("copyLegacyData"))
              && !idSrc.contains(QStringLiteral("migrateFromTechAim")),
              "SETA-SEL-001: no automatic copy of Tech Aim data into SETA");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-INT-001 — the hierarchy is INTEGRATED, and changes navigation only.
    //
    // The claim under test is behavioural equivalence: for every catalogue
    // programme, choosing it through RULE SET -> DISCIPLINE -> PROGRAMME must
    // land on the SAME runtime configuration the legacy weapon/distance/event
    // controls produced - and, decisively, must make ShootingPage read the SAME
    // ListModel row, because that row is what sets the shot count and the
    // displayed programme. A navigation change that moved a single row would
    // change what gets shot.
    // ─────────────────────────────────────────────────────────────────────
    {
        QQmlEngine intEngine;
        QQmlComponent intComp(&intEngine,
                              QUrl::fromLocalFile(QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
        QScopedPointer<QObject> cat(intComp.create());
        check(!cat.isNull(), "SETA-INT-001: catalogue loads", intComp.errorString());

        if (!cat.isNull()) {
            // The LEGACY mapping, written out as the reference implementation:
            // main.qml::updateGameType() turns (gameMode, gameEvent) into the
            // ListModel index ShootingPage::setCurrentGameType() then reads.
            //   rifle : events 0..4 -> rows 1..5, anything else -> row 0
            //   pistol: events 0..4 -> rows 7..11, anything else -> row 6
            const auto legacyRow = [](int gameMode, int gameEvent) {
                const int base = (gameMode == 0) ? 6 : 0;
                return (gameEvent >= 0 && gameEvent <= 4) ? base + 1 + gameEvent : base;
            };

            int compared = 0, mismatched = 0;
            QString firstMismatch;
            for (int variant = 0; variant < 2; ++variant) {
                const bool fifteen = (variant == 1);
                for (int range = 10; range <= 50; range += 40) {
                    const QString key = QStringLiteral("game%1RangeEventModel%2")
                                            .arg(range).arg(fifteen ? QStringLiteral("_15")
                                                                    : QString());
                    QVariant ev;
                    QMetaObject::invokeMethod(cat.data(), "entriesFor",
                                              Q_RETURN_ARG(QVariant, ev),
                                              Q_ARG(QVariant, key));
                    const QVariantList rows = ev.toList();
                    for (int row = 0; row < rows.size(); ++row) {
                        const QVariantMap e = rows.at(row).toMap();
                        const QString pid = e.value(QStringLiteral("programmeId")).toString();

                        QVariant cv;
                        QMetaObject::invokeMethod(cat.data(), "runtimeConfig",
                                                  Q_RETURN_ARG(QVariant, cv),
                                                  Q_ARG(QVariant, pid));
                        const QVariantMap cfg = cv.toMap();
                        ++compared;

                        const int gm = cfg.value(QStringLiteral("gameMode")).toInt();
                        const int ge = cfg.value(QStringLiteral("gameEvent")).toInt();
                        const bool ok =
                            cfg.value(QStringLiteral("gameRange")).toInt() == range
                            && gm == (e.value(QStringLiteral("isPistol")).toBool() ? 0 : 1)
                            && cfg.value(QStringLiteral("fifteen")).toBool() == fifteen
                            && cfg.value(QStringLiteral("shotCount")).toInt()
                                   == e.value(QStringLiteral("shotCount")).toInt()
                            && cfg.value(QStringLiteral("targetStandardId")).toString()
                                   == e.value(QStringLiteral("targetStandardId")).toString()
                            // THE decisive one: the legacy index arithmetic on
                            // the new path lands on this very row.
                            && legacyRow(gm, ge) == row;
                        if (!ok) {
                            ++mismatched;
                            if (firstMismatch.isEmpty())
                                firstMismatch = pid + QStringLiteral(" -> range=")
                                    + cfg.value(QStringLiteral("gameRange")).toString()
                                    + QStringLiteral(" mode=") + QString::number(gm)
                                    + QStringLiteral(" event=") + QString::number(ge)
                                    + QStringLiteral(" row=") + QString::number(legacyRow(gm, ge))
                                    + QStringLiteral(" expected row=") + QString::number(row);
                        }
                    }
                }
            }
            check(compared == 48, "SETA-INT-001: all 48 programmes were compared",
                  QString::number(compared));
            check(mismatched == 0,
                  "SETA-INT-001: every programme resolves to the SAME runtime "
                  "configuration and the SAME ShootingPage row as the legacy path",
                  firstMismatch);

            // Paper mode partitions the catalogue: the hierarchy offers the
            // variant this installation can actually run, and never both.
            int standard = 0, fifteenCount = 0;
            for (int variant = 0; variant < 2; ++variant) {
                QVariant v;
                QMetaObject::invokeMethod(cat.data(), "entriesIn", Q_RETURN_ARG(QVariant, v),
                                          Q_ARG(QVariant, variant == 1));
                (variant == 1 ? fifteenCount : standard) = v.toList().size();
            }
            // 24 standard-paper presets + 13 DSB (none of which is a 15-shot
            // paper variant) = 37, against 24 in 15-shot mode.
            check(standard == 37 && fifteenCount == 24 && standard + fifteenCount == 61,
                  "SETA-INT-001: the two paper modes partition the catalogue exactly",
                  QString::number(standard) + QStringLiteral("/")
                      + QString::number(fifteenCount));

            // 15-shot paper has no 60-shot entry, so no ISSF course exists and
            // the ISSF rule set correctly disappears rather than offering a
            // course the installation cannot run.
            QVariant rs;
            QMetaObject::invokeMethod(cat.data(), "ruleSets", Q_RETURN_ARG(QVariant, rs),
                                      Q_ARG(QVariant, true));
            bool issfIn15 = false;
            for (const QVariant& r : rs.toList())
                if (r.toMap().value(QStringLiteral("rulesetId")).toString()
                        == QStringLiteral("issf")) issfIn15 = true;
            check(!issfIn15,
                  "SETA-INT-001: 15-shot paper offers no ISSF course, because none exists");

            QMetaObject::invokeMethod(cat.data(), "ruleSets", Q_RETURN_ARG(QVariant, rs),
                                      Q_ARG(QVariant, false));
            check(rs.toList().size() == 3,
                  "SETA-INT-001: standard paper offers ISSF, DSB and the practice presets",
                  QString::number(rs.toList().size()));
        }

        // The page keeps the legacy controls as the rollback path, and shows
        // exactly one selector at a time.
        const QString page = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/LoginPage.qml"));
        check(page.contains(QStringLiteral("SetaCompetitionSelector")),
              "SETA-INT-001: the selector is wired into LoginPage");
        check(page.contains(QStringLiteral("property bool setaSelection")),
              "SETA-INT-001: the SETA path is a single switch, not a scattered edit");
        check(page.contains(QStringLiteral("visible: !setaSelection"))
              && page.contains(QStringLiteral("id: weaponRow")),
              "SETA-INT-001: the legacy controls are PRESERVED and hidden, not deleted");
        check(page.contains(QStringLiteral("runtimeConfig")),
              "SETA-INT-001: the commit path resolves through the catalogue, so there "
              "is no second set of match-configuration rules");
        check(!page.contains(QStringLiteral("applySetaProgramme(\"")),
              "SETA-INT-001: nothing applies a hardcoded programme id");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-BRAND-001 — the blue theme is a PACKAGE, not a sweep.
    //
    // Two separate risks: that a brand recolour reaches a SEMANTIC colour (a
    // fault stops reading as a fault), and that it reaches SCORING or TARGET
    // presentation (the picture of the shot changes because of branding).
    // Both are asserted against the shipped source, not against prose.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString tokens = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/ui/theme/DesignTokens.qml"));
        QString flatTokens = tokens;
        flatTokens.remove(QLatin1Char(' '));
        check(!tokens.isEmpty(), "SETA-BRAND-001: DesignTokens.qml is readable");
        check(flatTokens.contains(QStringLiteral("accentPrimary:PRODUCT.accentPrimary"))
              && flatTokens.contains(QStringLiteral("accentBright:PRODUCT.accentBright"))
              && flatTokens.contains(QStringLiteral("focusOutline:PRODUCT.focusOutline")),
              "SETA-BRAND-001: the accent comes from the build's brand package");
        check(flatTokens.contains(QStringLiteral("errorText:\"#D0392B\""))
              && flatTokens.contains(QStringLiteral("successText:\"#20C997\""))
              && flatTokens.contains(QStringLiteral("warningText:\"#E8A13D\"")),
              "SETA-BRAND-001: fault / healthy / warning are NOT brand-driven");

        // Semantic and scoring sites deliberately kept literal. Each one means
        // something; recolouring them for branding would change what the
        // operator is being told.
        struct Site { const char* file; const char* needle; const char* why; };
        const Site kept[] = {
            { "/main.qml", "opLive ? \"#2f7d4f\" : \"#e8003d\"",
              "Demo/Live badge - mistaking Demo for Live is a result-integrity risk" },
            { "/RightPanel.qml", "c: \"#e8003d\"",
              "score band <=7 - scoring presentation, not brand" },
            { "/FinalsReportTarget.qml", "\"#a80038\"",
              "shot-score colour" },
            { "/Report3PSeries.qml", "\"#a80038\"",
              "shot-score colour" },
            { "/SettingsPage.qml", "gameRange === 10 ? \"#f2c200\" : \"#e8003d\"",
              "target display colour swatch - shows the TARGET's colours" },
        };
        for (const Site& site : kept) {
            const QString src = readAll(QStringLiteral(TECHAIM_SOURCE_DIR) + QLatin1String(site.file));
            check(src.contains(QLatin1String(site.needle)),
                  "SETA-BRAND-001: semantic/scoring colour kept literal",
                  QLatin1String(site.file) + QStringLiteral(" - ")
                      + QLatin1String(site.why));
        }

        // The scoring authority itself must carry no brand colour at all.
        const QString centre = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml"));
        check(!centre.contains(QStringLiteral("PRODUCT.accentPrimary"))
              || centre.contains(QStringLiteral("calculateShootingSocre")),
              "SETA-BRAND-001: CenterPane still owns the scoring function");
        check(!centre.contains(QStringLiteral("radOf10Ring: PRODUCT"))
              && !centre.contains(QStringLiteral("radOf10Ring: theme")),
              "SETA-BRAND-001: ring geometry is not reachable from the brand");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-002 — language changes LABELS and nothing else.
    // Extends QML-LANG-001 from programmeId to every stable identity the
    // hierarchy exposes: rule set, discipline and target standard.
    // ─────────────────────────────────────────────────────────────────────
    {
        const auto snapshot = [](QQmlEngine& eng) {
            QQmlComponent comp(&eng, QUrl::fromLocalFile(
                QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
            QObject* cat = comp.create();
            QStringList ids;
            if (cat) {
                QVariant rs;
                QMetaObject::invokeMethod(cat, "ruleSets", Q_RETURN_ARG(QVariant, rs),
                                          Q_ARG(QVariant, QVariant()));
                for (const QVariant& r : rs.toList()) {
                    const QString rid = r.toMap().value(QStringLiteral("rulesetId")).toString();
                    ids << QStringLiteral("R:") + rid;
                    QVariant dv;
                    QMetaObject::invokeMethod(cat, "disciplines", Q_RETURN_ARG(QVariant, dv),
                                              Q_ARG(QVariant, rid), Q_ARG(QVariant, QVariant()));
                    for (const QVariant& d : dv.toList()) {
                        const QString did = d.toMap().value(QStringLiteral("disciplineId")).toString();
                        ids << QStringLiteral("D:") + rid + QLatin1Char('/') + did
                               + QStringLiteral(" T:")
                               + d.toMap().value(QStringLiteral("targetStandardId")).toString();
                        QVariant pv;
                        QMetaObject::invokeMethod(cat, "programmes", Q_RETURN_ARG(QVariant, pv),
                                                  Q_ARG(QVariant, rid), Q_ARG(QVariant, did),
                                                  Q_ARG(QVariant, QVariant()));
                        for (const QVariant& p : pv.toList()) {
                            const QVariantMap m = p.toMap();
                            ids << QStringLiteral("P:")
                                   + m.value(QStringLiteral("programmeId")).toString()
                                   + QStringLiteral(" T:")
                                   + m.value(QStringLiteral("targetStandardId")).toString()
                                   + QStringLiteral(" N:")
                                   + m.value(QStringLiteral("shotCount")).toString();
                        }
                    }
                }
                delete cat;
            }
            return ids;
        };

        QQmlEngine enEngine;
        const QStringList en = snapshot(enEngine);
        check(en.size() > 50, "SETA-LANG-002: the English snapshot is non-trivial",
              QString::number(en.size()));

        QTranslator deShip;
        const bool loaded = deShip.load(
            QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.qm"));
        check(loaded, "SETA-LANG-002: the SHIPPED German catalogue loads");
        if (loaded) QCoreApplication::installTranslator(&deShip);
        QQmlEngine deEngine;
        deEngine.retranslate();
        const QStringList de = snapshot(deEngine);
        if (loaded) QCoreApplication::removeTranslator(&deShip);

        check(en == de,
              "SETA-LANG-002: rule set, discipline, target standard, programme id "
              "and shot count are IDENTICAL in German",
              en.size() == de.size() ? QStringLiteral("same size, different content")
                                     : QStringLiteral("size %1 vs %2")
                                           .arg(en.size()).arg(de.size()));

        // The German catalogue must actually contain the selector's strings -
        // otherwise "German passes" only because German is still English.
        const QString ts = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.ts"));
        const char* mustTranslate[] = {
            "SELECT RULE SET", "SELECT DISCIPLINE", "SELECT PROGRAMME",
            "Rule set", "Discipline", "Programme", "&lt; Back",
            "Official competition rules", "Practice - no rule authority",
            "Official course", "Preset"
        };
        // Scoped to the SELECTOR's own context: the same English word can
        // appear in several contexts, and the first match in the file is not
        // necessarily the one this screen uses.
        const int ctxAt  = ts.indexOf(QStringLiteral("<name>SetaCompetitionSelector</name>"));
        const int ctxEnd = ts.indexOf(QStringLiteral("</context>"), ctxAt);
        const QString ctx = (ctxAt >= 0) ? ts.mid(ctxAt, ctxEnd - ctxAt) : QString();
        check(!ctx.isEmpty(), "SETA-LANG-002: the selector has its own translation context");
        int translated = 0;
        for (const char* src : mustTranslate) {
            const int at = ctx.indexOf(QStringLiteral("<source>%1</source>").arg(
                                           QLatin1String(src)));
            if (at < 0) continue;
            const int end = ctx.indexOf(QStringLiteral("</message>"), at);
            const QString msg = ctx.mid(at, end - at);
            if (!msg.contains(QStringLiteral("type=\"unfinished\""))) ++translated;
        }
        check(translated == int(sizeof(mustTranslate) / sizeof(mustTranslate[0])),
              "SETA-LANG-002: every selector string has a German translation",
              QString::number(translated) + QStringLiteral("/")
                  + QString::number(int(sizeof(mustTranslate) / sizeof(mustTranslate[0]))));

        // And the selector still compares no translated string.
        const QString sel = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/SetaCompetitionSelector.qml"));
        check(!sel.contains(QStringLiteral("=== qsTr(")) && !sel.contains(QStringLiteral("== qsTr(")),
              "SETA-LANG-002: QML-LANG-001 still holds in the selector");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-ID-001 — no user-visible Tech Aim identity survives in a SETA build.
    //
    // Scans STRING LITERALS, with comment lines stripped first: a gate that can
    // pass or fail on a comment is not a gate. Component type names
    // (TechAimDialog) and qrc asset paths are deliberately NOT flagged - they
    // are source identifiers and files, not words an operator ever reads.
    // ─────────────────────────────────────────────────────────────────────
    {
        QDir idRoot(QStringLiteral(TECHAIM_SOURCE_DIR));
        const QStringList files =
            idRoot.entryList(QStringList() << QStringLiteral("*.qml"), QDir::Files);
        QStringList offenders;
        for (const QString& fname : files) {
            const QString body = readAll(idRoot.filePath(fname));
            const QStringList lines = body.split(QChar(10));
            for (int i = 0; i < lines.size(); ++i) {
                QString line = lines.at(i);
                const int comment = line.indexOf(QStringLiteral("//"));
                if (comment >= 0) line = line.left(comment);
                // Only what sits INSIDE a double-quoted literal can be shown.
                int from = 0;
                while (true) {
                    const int a = line.indexOf(QLatin1Char('"'), from);
                    if (a < 0) break;
                    const int b = line.indexOf(QLatin1Char('"'), a + 1);
                    if (b < 0) break;
                    const QString lit = line.mid(a + 1, b - a - 1);
                    from = b + 1;
                    if (lit.startsWith(QStringLiteral("qrc:"))) continue;
                    if (lit.contains(QStringLiteral("Tech Aim"))
                        || lit.contains(QStringLiteral("TechAim")))
                        offenders << (fname + QStringLiteral(":")
                                      + QString::number(i + 1) + QStringLiteral(" \"")
                                      + lit.left(48) + QStringLiteral("\""));
                }
            }
        }
        check(offenders.isEmpty(),
              "SETA-ID-001: no user-visible Tech Aim product identity remains in QML",
              offenders.join(QStringLiteral(" | ")));

        // The footer is the one that was actually shipping the leak, so it is
        // named explicitly rather than left to the sweep above.
        const QString page = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/LoginPage.qml"));
        check(page.contains(QStringLiteral("PRODUCT.displayName + \"  ·  \" + qsTr(\"Electronic target control\")")),
              "SETA-ID-001: the footer takes its product name from ProductIdentity");
        check(!page.contains(QStringLiteral("\"TechAim  ·  Electronic target control\"")),
              "SETA-ID-001: the hardcoded Tech Aim footer literal is gone");

        // Exported PDF file names are user-visible identity too.
        const QString shooting = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ShootingPage.qml"));
        check(shooting.contains(QStringLiteral("function productFilePrefix()"))
              && !shooting.contains(QStringLiteral("\"TechAim_")),
              "SETA-ID-001: exported PDF names derive from the product, not a literal");

        // The C++ side: PDF document metadata and the operator messages that
        // named the product.
        const QString print = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/customprint.cpp"));
        check(!print.contains(QStringLiteral("QStringLiteral(\"Tech Aim "))
              && print.contains(QStringLiteral("ta::app::identity()")),
              "SETA-ID-001: PDF metadata is composed from identity");

        // The LEGAL publisher is a different fact and must NOT have moved.
        const QString ident = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/app/ProductIdentity.cpp"));
        check(ident.contains(QStringLiteral("JAC SHOOTING SOLUTIONS (PTY) LTD")),
              "SETA-ID-001: the legal publisher is unchanged by the identity sweep");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-ID-002 — the Demo pill's SEMANTIC colour is independent of the brand.
    // Colour states WHAT MODE IT IS; the accent states only WHICH IS SELECTED.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString page = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/LoginPage.qml"));
        const int at = page.indexOf(QStringLiteral("// Demo pill"));
        check(at > 0, "SETA-ID-002: the Demo pill is locatable");
        const QString pill = page.mid(at, 1800);
        check(pill.contains(QStringLiteral("border.color: !opModeRow.opLive ? theme.tokens.errorText")),
              "SETA-ID-002: the Demo border is the ERROR token, not the brand accent");
        check(pill.contains(QStringLiteral("color: !opModeRow.opLive ? theme.tokens.errorText")),
              "SETA-ID-002: the Demo label is the ERROR token, not the brand accent");
        check(pill.contains(QStringLiteral("width: 4; radius: 2")),
              "SETA-ID-002: selection is one restrained accent edge strip");
        check(pill.contains(QStringLiteral("color: _errBg")) || pill.contains(QStringLiteral("? _errBg :")),
              "SETA-ID-002: the Demo fill stays the error background");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-003 — the landing screen's German is REAL, and still changes
    // nothing but labels.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString ts = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.ts"));
        struct Want { const char* ctx; const char* src; };
        const Want wanted[] = {
            { "LoginPage", "Start session" },
            { "LoginPage", "Session setup" },
            { "LoginPage", "ATHLETE" },
            { "LoginPage", "Athlete name" },
            { "LoginPage", "OPERATING MODE" },
            { "LoginPage", "Choose an event" },
            { "LoginPage", "Load saved session" },
            { "LoginPage", "Contact us" },
            { "LoginPage", "READY TO START" },
            { "LoginPage", "No athlete entered" },
            { "LoginPage", "Electronic target control" },
            { "TargetStatusPanel", "NO TARGET" },
            { "TargetStatusPanel", "Connect the target USB cable." },
            { "TargetStatusPanel", "Target Connection" },
            { "Header", "ELECTRONIC TARGET" },
        };
        int have = 0;
        QStringList absent;
        for (const Want& w : wanted) {
            const int c = ts.indexOf(QStringLiteral("<name>%1</name>").arg(QLatin1String(w.ctx)));
            if (c < 0) { absent << QLatin1String(w.ctx); continue; }
            const int cEnd = ts.indexOf(QStringLiteral("</context>"), c);
            const QString ctx = ts.mid(c, cEnd - c);
            const int at = ctx.indexOf(QStringLiteral("<source>%1</source>").arg(QLatin1String(w.src)));
            if (at < 0) { absent << QLatin1String(w.src); continue; }
            const int end = ctx.indexOf(QStringLiteral("</message>"), at);
            if (!ctx.mid(at, end - at).contains(QStringLiteral("type=\"unfinished\"")))
                ++have;
            else
                absent << QLatin1String(w.src);
        }
        check(have == int(sizeof(wanted) / sizeof(wanted[0])),
              "SETA-LANG-003: the landing screen's core strings have German",
              absent.join(QStringLiteral(", ")));

        // German is BETA and the surface is NOT fully translated. Assert the
        // honest state rather than a claim of completeness: the language option
        // must still be marked beta.
        const QString lang = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/app/LanguageService.cpp"));
        check(lang.contains(QStringLiteral("QStringLiteral(\"de-DE\"), QStringLiteral(\"Deutsch\"), true")),
              "SETA-LANG-003: German is still declared BETA, not complete");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-004 — German coverage of the ON-SCREEN surface, and the state
    // that must not move with it.
    //
    // Coverage is asserted against the catalogue, not against prose: every
    // remaining unfinished entry in an on-screen context must be one of the
    // deliberately language-neutral values listed here. Anything else is a real
    // gap and fails, which is what stops "German is complete" being a claim
    // somebody can make by editing a document.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString ts = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.ts"));
        check(!ts.isEmpty(), "SETA-LANG-004: the German catalogue is readable");

        // Printed report / PDF views are a SEPARATE surface: fixed A4 geometry,
        // documents rather than controls. They are excluded here and reported
        // as the remaining area rather than silently counted as done.
        const QStringList reportCtx = {
            QStringLiteral("FinalsReportView"), QStringLiteral("CoachPrintView"),
            QStringLiteral("CallDiagnoseReportView"), QStringLiteral("TrainingReportView"),
            QStringLiteral("SummaryReportView"), QStringLiteral("CoachDashboardView"),
            QStringLiteral("CoachDetailedView"), QStringLiteral("PositionTransitionReportView"),
            QStringLiteral("Report3P"), QStringLiteral("Report3PSeries"),
            QStringLiteral("MatchReportInfo"), QStringLiteral("MatchReportView"),
            QStringLiteral("ReportHeader"), QStringLiteral("ReportFooter"),
            QStringLiteral("PdfSeriesPage"), QStringLiteral("SectionTitle"),
            QStringLiteral("FinalsReportTarget"), QStringLiteral("About"),
            QStringLiteral("BusMonitor"), QStringLiteral("MainWindow"),
            QStringLiteral("Settings"), QStringLiteral("SettingsModbusRTU"),
            QStringLiteral("SettingsModbusTCP"), QStringLiteral("ModbusAdapter"),
            QStringLiteral("SerialModbusAdapter"), QStringLiteral("QObject"),
            QStringLiteral("ConnectionError"), QStringLiteral("SeriesComponent"),
            QStringLiteral("IncidentWindow")
        };
        // Units, axis labels, score bands, shot counts, format fragments and the
        // vendored form's designer name. Translating any of these would change a
        // technical value, not a label.
        const QStringList neutral = {
            QStringLiteral(" mm"), QStringLiteral("X:"), QStringLiteral(", Y:"),
            QStringLiteral("—"), QStringLiteral("S"), QStringLiteral("#"),
            QStringLiteral("%1"), QStringLiteral(" (%1)"), QStringLiteral("Form"),
            QStringLiteral("10"), QStringLiteral("15"), QStringLiteral("20"),
            QStringLiteral("30"), QStringLiteral("40"), QStringLiteral("60"),
            QStringLiteral("10s"), QStringLiteral("9s"), QStringLiteral("8s"),
            QStringLiteral("≤7"), QStringLiteral("\n")
        };

        int screenTotal = 0, screenDone = 0, reportGap = 0;
        QStringList realGaps;
        int at = 0;
        while (true) {
            const int c = ts.indexOf(QStringLiteral("<context>"), at);
            if (c < 0) break;
            const int cEnd = ts.indexOf(QStringLiteral("</context>"), c);
            const QString ctx = ts.mid(c, cEnd - c);
            at = cEnd + 1;
            const int n0 = ctx.indexOf(QStringLiteral("<name>")) + 6;
            const QString name = ctx.mid(n0, ctx.indexOf(QStringLiteral("</name>")) - n0);
            const bool isReport = reportCtx.contains(name);

            int m = 0;
            while (true) {
                const int a = ctx.indexOf(QStringLiteral("<message>"), m);
                if (a < 0) break;
                const int b = ctx.indexOf(QStringLiteral("</message>"), a);
                const QString msg = ctx.mid(a, b - a);
                m = b + 1;
                const int s0 = msg.indexOf(QStringLiteral("<source>")) + 8;
                const int s1 = msg.indexOf(QStringLiteral("</source>"));
                if (s0 < 8 || s1 < 0) continue;
                const QString src = msg.mid(s0, s1 - s0);
                const bool unfinished = msg.contains(QStringLiteral("type=\"unfinished\""));
                if (isReport) { if (unfinished) ++reportGap; continue; }
                if (neutral.contains(src)) continue;
                ++screenTotal;
                if (!unfinished) ++screenDone;
                else if (realGaps.size() < 8) realGaps << src.left(40);
            }
        }
        check(screenTotal > 700,
              "SETA-LANG-004: the on-screen surface is large enough to be meaningful",
              QString::number(screenTotal));
        check(realGaps.isEmpty(),
              "SETA-LANG-004: every on-screen string has German",
              QString(QStringLiteral("%1/%2 translated%3"))
                  .arg(screenDone).arg(screenTotal)
                  .arg(realGaps.isEmpty() ? QString()
                                          : QStringLiteral(" - gaps: ")
                                                + realGaps.join(QStringLiteral(" | "))));
        // The printed-report surface is NOT claimed. Asserting it is still
        // outstanding keeps the honest status honest: if someone translates it
        // later this check tells them to update the status document too.
        check(reportGap > 0,
              "SETA-LANG-004: the printed report surface is still outstanding, "
              "so German remains PARTIAL overall",
              QString::number(reportGap) + QStringLiteral(" strings"));

        // German stays BETA regardless of coverage: native technical review is
        // a different question from string count.
        const QString lang = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/app/LanguageService.cpp"));
        check(lang.contains(QStringLiteral("QStringLiteral(\"de-DE\"), QStringLiteral(\"Deutsch\"), true")),
              "SETA-LANG-004: German is still declared BETA");
    }

    // ─────────────────────────────────────────────────────────────────────
    // SETA-LANG-005 — switching language moves NOTHING but labels.
    // ─────────────────────────────────────────────────────────────────────
    {
        const auto stateOf = [](QQmlEngine& eng) {
            QQmlComponent comp(&eng, QUrl::fromLocalFile(
                QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
            QObject* cat = comp.create();
            QStringList out;
            if (cat) {
                QVariant all;
                QMetaObject::invokeMethod(cat, "allEntries", Q_RETURN_ARG(QVariant, all));
                for (const QVariant& e : all.toList()) {
                    const QVariantMap m = e.toMap();
                    QVariant cfg;
                    QMetaObject::invokeMethod(cat, "runtimeConfig", Q_RETURN_ARG(QVariant, cfg),
                        Q_ARG(QVariant, m.value(QStringLiteral("programmeId"))));
                    const QVariantMap c = cfg.toMap();
                    out << m.value(QStringLiteral("programmeId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("rulesetId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("disciplineId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("targetStandardId")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("distanceM")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("shotCount")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("scoringMode")).toString()
                           + QStringLiteral("|") + m.value(QStringLiteral("targetFamily")).toString()
                           + QStringLiteral("|") + c.value(QStringLiteral("gameRange")).toString()
                           + QStringLiteral("|") + c.value(QStringLiteral("gameMode")).toString()
                           + QStringLiteral("|") + c.value(QStringLiteral("gameEvent")).toString();
                }
                delete cat;
            }
            return out;
        };

        QQmlEngine e1;
        const QStringList en = stateOf(e1);
        check(en.size() == 61, "SETA-LANG-005: the English state snapshot covers all 61 programmes",
              QString::number(en.size()));

        QTranslator de;
        const bool ok = de.load(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/techaim_de_DE.qm"));
        check(ok, "SETA-LANG-005: the shipped German catalogue loads");
        if (ok) QCoreApplication::installTranslator(&de);
        QQmlEngine e2;
        e2.retranslate();
        const QStringList deState = stateOf(e2);
        if (ok) QCoreApplication::removeTranslator(&de);

        check(en == deState,
              "SETA-LANG-005: programmeId, rulesetId, disciplineId, targetStandardId, "
              "distance, shot count, scoring mode, target family, range, weapon and "
              "event index are ALL identical in German");
    }

    // ─────────────────────────────────────────────────────────────────────
    // DSB-CAT-001 — the DSB 2026 ruleset, as a first-class competition set.
    //
    // Authority: docs/rules/dsb-2026-*.md, Sportordnung 01.01.2026.
    // Every number checked here is a rule value with a page reference; this
    // gate exists so a later edit cannot quietly change a competition.
    // ─────────────────────────────────────────────────────────────────────
    {
        QQmlEngine eng;
        QQmlComponent comp(&eng, QUrl::fromLocalFile(
            QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
        QScopedPointer<QObject> cat(comp.create());
        check(!cat.isNull(), "DSB-CAT-001: catalogue loads", comp.errorString());
        if (cat.isNull()) { /* nothing further to assert */ }
        else {
        const auto def = [&cat](const char* id) {
            QVariant v;
            QMetaObject::invokeMethod(cat.data(), "competitionDefinition",
                                      Q_RETURN_ARG(QVariant, v),
                                      Q_ARG(QVariant, QString::fromLatin1(id)));
            return v.toMap();
        };
        const auto timing = [&cat](const char* id) {
            QVariant v;
            QMetaObject::invokeMethod(cat.data(), "timingFor", Q_RETURN_ARG(QVariant, v),
                                      Q_ARG(QVariant, QString::fromLatin1(id)));
            return v.toMap();
        };
        const auto mins = [](const QVariantMap& t, const char* key) {
            return int(t.value(QLatin1String(key)).toLongLong() / 60000);
        };

        // ── the ruleset exists and is versioned ──────────────────────────
        QVariant all;
        QMetaObject::invokeMethod(cat.data(), "dsbEntries", Q_RETURN_ARG(QVariant, all));
        check(all.toList().size() == 13,
              "DSB-CAT-001: 13 DSB programmes are defined",
              QString::number(all.toList().size()));
        int versioned = 0, contexted = 0, official = 0;
        for (const QVariant& e : all.toList()) {
            const QVariantMap m = e.toMap();
            if (m.value(QStringLiteral("rulesetVersion")).toString() == QLatin1String("2026-01-01")) ++versioned;
            if (!m.value(QStringLiteral("competitionContext")).toString().isEmpty()) ++contexted;
            if (m.value(QStringLiteral("programmeType")).toString() == QLatin1String("OFFICIAL")) ++official;
        }
        check(versioned == 13 && contexted == 13 && official == 13,
              "DSB-CAT-001: every DSB programme is versioned, context-bound and official",
              QString::number(versioned) + QStringLiteral("/")
                  + QString::number(contexted) + QStringLiteral("/")
                  + QString::number(official));

        // ── 1.10 Luftgewehr: shot count -> EST minutes, decimal ──────────
        struct Simple { const char* id; const char* rule; int shots; int match; int prep;
                        const char* scoring; };
        const Simple simple[] = {
            { "dsb.10m.air-rifle.lg20",   "1.10", 20,  30, 15, "DECIMAL" },
            { "dsb.10m.air-rifle.lg40",   "1.10", 40,  50, 15, "DECIMAL" },
            { "dsb.10m.air-rifle.lg60",   "1.10", 60,  75, 15, "DECIMAL" },
            { "dsb.50m.rifle.prone60",    "1.80", 60,  50, 15, "DECIMAL" },
            { "dsb.10m.air-pistol.lp20",  "2.10", 20,  30, 15, "INTEGER" },
            { "dsb.10m.air-pistol.lp40",  "2.10", 40,  50, 15, "INTEGER" },
            { "dsb.10m.air-pistol.lp60",  "2.10", 60,  75, 15, "INTEGER" },
            { "dsb.50m.pistol.p60",       "2.20", 60,  90, 15, "INTEGER" },
            { "dsb.50m.pistol.p30",       "2.20", 30,  55, 15, "INTEGER" },
            { "dsb.50m.rifle.3x20",       "1.40", 60, 105, 15, "INTEGER" },
            { "dsb.50m.rifle.3x40",       "1.60", 120,165, 15, "INTEGER" },
        };
        int ok = 0;
        QStringList wrong;
        for (const Simple& p : simple) {
            const QVariantMap d = def(p.id), t = timing(p.id);
            const bool good =
                d.value(QStringLiteral("ruleNumber")).toString() == QLatin1String(p.rule)
                && d.value(QStringLiteral("shotCount")).toInt() == p.shots
                && d.value(QStringLiteral("scoringMode")).toString() == QLatin1String(p.scoring)
                && t.value(QStringLiteral("timingModel")).toString()
                       == QLatin1String("SINGLE_MATCH_CLOCK")
                && mins(t, "matchMs") == p.match
                && mins(t, "preparationMs") == p.prep
                && t.value(QStringLiteral("preparationPolicy")).toString()
                       == QLatin1String("OUTSIDE_MATCH_TIME")
                && t.value(QStringLiteral("sighterPolicy")).toString()
                       == QLatin1String("UNLIMITED_IN_PREPARATION")
                && t.value(QStringLiteral("positionMs")).toList().isEmpty();
            if (good) ++ok; else wrong << QLatin1String(p.id);
        }
        check(ok == int(sizeof(simple) / sizeof(simple[0])),
              "DSB-CAT-001: every single-master-clock programme carries its rule "
              "number, shot count, EST time, 15 min outside preparation, unlimited "
              "sighters and scoring mode",
              wrong.join(QStringLiteral(", ")));

        // ── 2.20 30-shot time is a RECOMMENDATION, 60-shot is a rule ─────
        check(timing("dsb.50m.pistol.p30").value(QStringLiteral("matchTimeAuthority")).toString()
                  == QLatin1String("RECOMMENDED")
              && timing("dsb.50m.pistol.p60").value(QStringLiteral("matchTimeAuthority")).toString()
                  == QLatin1String("RULE"),
              "DSB-CAT-001: the 30-shot 50 m pistol time is recorded as a recommendation, "
              "the 60-shot time as a rule");

        // ── 1.20: INDEPENDENT position clocks, gated, sighting inside ────
        struct ThreePos { const char* id; const char* variant; int shots;
                          int k; int p; int st; };
        const ThreePos tp[] = {
            { "dsb.10m.air-rifle.3x10", "3x10", 30, 25, 20, 30 },
            { "dsb.10m.air-rifle.3x20", "3x20", 60, 35, 30, 40 },
        };
        int tpOk = 0;
        QStringList tpWrong;
        for (const ThreePos& p : tp) {
            const QVariantMap d = def(p.id), t = timing(p.id);
            const QVariantList pos = t.value(QStringLiteral("positionMs")).toList();
            const QVariantList seq = d.value(QStringLiteral("positions")).toList();
            const QVariantList spp = d.value(QStringLiteral("shotsPerPosition")).toList();
            const bool good =
                d.value(QStringLiteral("ruleNumber")).toString() == QLatin1String("1.20")
                && d.value(QStringLiteral("programmeVariant")).toString() == QLatin1String(p.variant)
                && d.value(QStringLiteral("shotCount")).toInt() == p.shots
                && d.value(QStringLiteral("scoringMode")).toString() == QLatin1String("INTEGER")
                && t.value(QStringLiteral("timingModel")).toString()
                       == QLatin1String("INDEPENDENT_POSITION_CLOCKS")
                && t.value(QStringLiteral("matchMs")).toLongLong() == 0
                && pos.size() == 3
                && pos.at(0).toLongLong() == qint64(p.k)  * 60000
                && pos.at(1).toLongLong() == qint64(p.p)  * 60000
                && pos.at(2).toLongLong() == qint64(p.st) * 60000
                && mins(t, "preparationMs") == 15
                && t.value(QStringLiteral("preparationPolicy")).toString()
                       == QLatin1String("OUTSIDE_MATCH_TIME")
                && t.value(QStringLiteral("sighterPolicy")).toString()
                       == QLatin1String("INSIDE_POSITION_CLOCK")
                && t.value(QStringLiteral("positionTransitionPolicy")).toString()
                       == QLatin1String("GATED_BY_MATCH_CONTROL")
                && seq.size() == 3
                && seq.at(0).toString() == QLatin1String("KNEELING")
                && seq.at(1).toString() == QLatin1String("PRONE")
                && seq.at(2).toString() == QLatin1String("STANDING")
                && spp.size() == 3
                && spp.at(0).toInt() * 3 == p.shots;
            if (good) ++tpOk; else tpWrong << QLatin1String(p.id);
        }
        check(tpOk == 2,
              "DSB-CAT-001: 1.20 3x10 and 3x20 carry independent position clocks "
              "(25/20/30 and 35/30/40), kneeling-prone-standing, a 15 min outside "
              "preparation, sighting INSIDE the position clock and a gated transition",
              tpWrong.join(QStringLiteral(", ")));

        // ── 1.20 has NO master clock; 1.40/1.60 have NO position clocks ──
        check(timing("dsb.10m.air-rifle.3x20").value(QStringLiteral("matchMs")).toLongLong() == 0,
              "DSB-CAT-001: 1.20 exposes no master clock - a caller reading the "
              "wrong field gets 0, never a plausible wrong number");
        check(timing("dsb.50m.rifle.3x20").value(QStringLiteral("positionMs")).toList().isEmpty()
              && timing("dsb.50m.rifle.3x40").value(QStringLiteral("positionMs")).toList().isEmpty(),
              "DSB-CAT-001: 1.40 and 1.60 expose no position clocks");

        // ── target geometry is REUSED, never duplicated ──────────────────
        struct Face { const char* id; const char* std; int dsbNumber; };
        const Face faces[] = {
            { "dsb.10m.air-rifle.lg60",  "issf.10m.air-rifle", 1 },
            { "dsb.10m.air-rifle.3x20",  "issf.10m.air-rifle", 1 },
            { "dsb.50m.rifle.3x20",      "issf.50m.rifle",     3 },
            { "dsb.50m.rifle.3x40",      "issf.50m.rifle",     3 },
            { "dsb.50m.rifle.prone60",   "issf.50m.rifle",     3 },
            { "dsb.50m.pistol.p60",      "issf.50m.pistol",    4 },
            { "dsb.10m.air-pistol.lp60", "issf.10m.air-pistol",7 },
        };
        int faceOk = 0;
        for (const Face& f : faces) {
            const QVariantMap d = def(f.id);
            if (d.value(QStringLiteral("targetStandardId")).toString() == QLatin1String(f.std)
                && d.value(QStringLiteral("dsbTargetNumber")).toInt() == f.dsbNumber)
                ++faceOk;
        }
        check(faceOk == int(sizeof(faces) / sizeof(faces[0])),
              "DSB-CAT-001: every DSB programme SELECTS an existing ISSF target "
              "standard - Scheibe 1/3/4/7 - and defines no new geometry",
              QString::number(faceOk));

        // ── scoring is NOT a property of the ruleset ─────────────────────
        check(def("dsb.10m.air-rifle.lg60").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("DECIMAL")
              && def("dsb.10m.air-rifle.3x20").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("INTEGER"),
              "DSB-CAT-001: DSB is NOT uniformly integer - 1.10 is decimal while "
              "1.20 is whole ring, from the same ruleset");
        check(def("dsb.50m.rifle.prone60").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("DECIMAL")
              && def("dsb.50m.rifle.3x20").value(QStringLiteral("scoringMode")).toString()
                  == QLatin1String("INTEGER"),
              "DSB-CAT-001: 1.80 is decimal and 1.40 is integer on the same target face");

        // ── the four layers stay separable ───────────────────────────────
        const QVariantMap meta = [&cat]() {
            QVariant v;
            QMetaObject::invokeMethod(cat.data(), "rulesetMetadata", Q_RETURN_ARG(QVariant, v),
                Q_ARG(QVariant, QStringLiteral("dsb.50m.rifle.3x20")));
            return v.toMap();
        }();
        check(meta.value(QStringLiteral("ruleset")).toString() == QLatin1String("dsb")
              && meta.value(QStringLiteral("rulesetVersion")).toString() == QLatin1String("2026-01-01")
              && meta.value(QStringLiteral("ruleNumber")).toString() == QLatin1String("1.40")
              && meta.value(QStringLiteral("programmeVariant")).toString() == QLatin1String("3x20")
              && meta.value(QStringLiteral("competitionContext")).toString() == QLatin1String("DM_2026")
              && meta.value(QStringLiteral("scoringMode")).toString() == QLatin1String("INTEGER"),
              "DSB-CAT-001: ruleset, version, rule number, variant, context and "
              "scoring mode are separately recoverable for the journal and report");

        // ── programmes that are NOT established must not exist ───────────
        QStringList forbidden;
        for (const QVariant& e : all.toList()) {
            const QVariantMap m = e.toMap();
            const QString v = m.value(QStringLiteral("programmeVariant")).toString();
            const QString r = m.value(QStringLiteral("ruleNumber")).toString();
            if (v.contains(QStringLiteral("3x15")))
                forbidden << m.value(QStringLiteral("programmeId")).toString();
            if (r == QLatin1String("1.40") && v == QLatin1String("3x10"))
                forbidden << m.value(QStringLiteral("programmeId")).toString();
        }
        check(forbidden.isEmpty(),
              "DSB-CAT-001: no 10 m 3x15 and no 50 m 1.40 3x10 - neither is "
              "established by the Sportordnung (S-C.2, S-C.3)",
              forbidden.join(QStringLiteral(", ")));

        // ── the hardware-blocked programmes are absent, not faked ────────
        QStringList faked;
        for (const QVariant& e : all.toList()) {
            const QString r = e.toMap().value(QStringLiteral("ruleNumber")).toString();
            if (r == QLatin1String("2.16") || r == QLatin1String("2.17")
                || r == QLatin1String("2.18"))
                faked << r;
        }
        check(faked.isEmpty(),
              "DSB-CAT-001: 2.16 / 2.17 / 2.18 are NOT present - they need falling "
              "targets, a second face and series exposure, and must not be faked "
              "by changing timers only",
              faked.join(QStringLiteral(", ")));

        // ── the legacy ISSF rows are untouched ───────────────────────────
        QVariant legacy;
        QMetaObject::invokeMethod(cat.data(), "entriesFor", Q_RETURN_ARG(QVariant, legacy),
                                  Q_ARG(QVariant, QStringLiteral("game10RangeEventModel")));
        check(legacy.toList().size() == 12
              && legacy.toList().at(5).toMap().value(QStringLiteral("programmeId")).toString()
                     == QLatin1String("issf.10m.air-rifle.qualification60"),
              "DSB-CAT-001: entriesFor() still returns ONLY the index-locked legacy "
              "rows, so no ISSF row moved");
        }
    }

    // ─────────────────────────────────────────────────────────────────────
    // DSB-TIMING-001 — the critical cross-rule test (requirement 28).
    //
    // Three three-position competitions, three different timing structures.
    // If this ever passes by accident, competition behaviour has been inferred
    // from the words "three position" somewhere, which is the exact defect this
    // whole design exists to prevent.
    // ─────────────────────────────────────────────────────────────────────
    {
        QQmlEngine eng;
        QQmlComponent comp(&eng, QUrl::fromLocalFile(
            QStringLiteral(TECHAIM_SOURCE_DIR "/CompetitionCatalogue.qml")));
        QScopedPointer<QObject> cat(comp.create());
        if (!cat.isNull()) {
            const auto timing = [&cat](const char* id) {
                QVariant v;
                QMetaObject::invokeMethod(cat.data(), "timingFor", Q_RETURN_ARG(QVariant, v),
                                          Q_ARG(QVariant, QString::fromLatin1(id)));
                return v.toMap();
            };
            const QVariantMap dsb120 = timing("dsb.10m.air-rifle.3x20");
            const QVariantMap dsb140 = timing("dsb.50m.rifle.3x20");
            const QVariantMap dsb160 = timing("dsb.50m.rifle.3x40");
            const QVariantMap issf3p = timing("issf.50m.rifle.qualification60");

            check(dsb120.value(QStringLiteral("timingModel")).toString()
                      == QLatin1String("INDEPENDENT_POSITION_CLOCKS")
                  && dsb140.value(QStringLiteral("timingModel")).toString()
                      == QLatin1String("SINGLE_MATCH_CLOCK")
                  && dsb160.value(QStringLiteral("timingModel")).toString()
                      == QLatin1String("SINGLE_MATCH_CLOCK"),
                  "DSB-TIMING-001: DSB 1.20 uses independent position clocks while "
                  "DSB 1.40 and 1.60 use one master clock - all three are "
                  "three-position rifle competitions in the same ruleset");

            check(dsb140.value(QStringLiteral("matchMs")).toLongLong() == qint64(105) * 60000
                  && dsb160.value(QStringLiteral("matchMs")).toLongLong() == qint64(165) * 60000,
                  "DSB-TIMING-001: the two master clocks are 105 and 165 minutes");

            // DSB 1.20's three clocks are not one clock split three ways.
            const QVariantList pos = dsb120.value(QStringLiteral("positionMs")).toList();
            qint64 sum = 0;
            for (const QVariant& p : pos) sum += p.toLongLong();
            check(sum == qint64(105) * 60000
                  && dsb120.value(QStringLiteral("matchMs")).toLongLong() == 0,
                  "DSB-TIMING-001: 1.20's position clocks happen to total 105 min, "
                  "and it still exposes NO master clock - the total is a coincidence, "
                  "not a course time");

            // ISSF must be untouched: it declares no DSB timing at all and keeps
            // getting its durations from where it always has.
            check(issf3p.value(QStringLiteral("matchMs")).toLongLong() == 0
                  && issf3p.value(QStringLiteral("preparationMs")).toLongLong() == 0
                  && issf3p.value(QStringLiteral("positionMs")).toList().isEmpty()
                  && issf3p.value(QStringLiteral("preparationPolicy")).toString().isEmpty(),
                  "DSB-TIMING-001: the ISSF 50 m 3P profile declares NO timing here, "
                  "so adding DSB cannot have changed how ISSF is conducted");
        }
    }

    printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    fflush(stdout);
    return g_failures ? 1 : 0;
}

#include "tst_qml_shot_path.moc"
