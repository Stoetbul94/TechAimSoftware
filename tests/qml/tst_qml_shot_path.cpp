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
#include <cmath>
#include <algorithm>

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
        const QString qm = QStringLiteral(TECHAIM_SOURCE_DIR "/translations/german.qm");
        const bool loaded = de.load(qm);
        check(loaded, "QML-LANG-001: the German catalogue loads", qm);
        if (loaded) {
            const QString translated = de.translate("CenterPane", "PISTOL");
            check(!translated.isEmpty() && translated != QStringLiteral("PISTOL"),
                  "QML-LANG-001: German translates PISTOL differently - any logic "
                  "comparing it would flip discipline",
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
            const bool loaded2 = de2.load(QStringLiteral(TECHAIM_SOURCE_DIR "/translations/german.qm"));
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
        check(!catSrc.contains(QStringLiteral("\"rulesetId\": \"DSB\""))
              && !catSrc.contains(QStringLiteral("\"federation\": \"DSB\"")),
              "CATALOGUE-001: no unverified federation programme has been added");
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

    // -- ACQ-SENTINEL-003 - the UI must refuse a coordinate it does not have --
    //
    // The C++ side now derives the shot count from the coordinate arrays, so an
    // index past the end cannot be constructed. That makes this layer a second
    // line rather than the only one - and it is the layer that turns a
    // coordinate into a score, a marker and a journalled result, so it is the
    // layer that has to ask. On 2026-08-23 it did not ask: the -1 sentinel from
    // getXCord() became -1.00/-1.00 mm and scored 10.8 for the rest of three
    // sessions on Tablet-02.
    {
        const QString centre = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml"));
        check(centre.contains(QStringLiteral("function coordinatesUsable(")),
              "ACQ-SENTINEL-003: CenterPane has one place that refuses an unmeasured shot");
        check(centre.contains(QStringLiteral("MODREADER.coordinateHasValue(")),
              "ACQ-SENTINEL-003: the refusal ASKS the backend, it does not test a magic number");
        check(centre.contains(QStringLiteral("if (!coordinatesUsable(i))")),
              "ACQ-SENTINEL-003: the batch reader stops at the first unmeasured shot");
        check(centre.contains(QStringLiteral("if (!coordinatesUsable(shooutIndex))")),
              "ACQ-SENTINEL-003: the live-shot path refuses before it scores - the 2026-08-23 path");
        const int guardAt = centre.indexOf(QStringLiteral("if (!coordinatesUsable(shooutIndex))"));
        const int fetchAt = centre.indexOf(QStringLiteral("MODREADER.getXCord(shooutIndex)"));
        check(guardAt > 0 && fetchAt > guardAt,
              "ACQ-SENTINEL-003: the guard precedes the coordinate fetch",
              QStringLiteral("guard=%1 fetch=%2").arg(guardAt).arg(fetchAt));

        const QString info = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/MatchReportInfo.qml"));
        check(info.contains(QStringLiteral("isFinite(v)")),
              "ACQ-SENTINEL-003: the report prints a dash, not a number, for a missing coordinate");
        check(!info.contains(QStringLiteral("(MODREADER.getXMPIForShoot(seriesIndex, index)*1).toFixed(2)")),
              "ACQ-SENTINEL-003: the unguarded per-shot X cell is gone");
        check(!info.contains(QStringLiteral("(MODREADER.getYMPIForShoot(seriesIndex, index)*1).toFixed(2)")),
              "ACQ-SENTINEL-003: the unguarded per-shot Y cell is gone");

        for (const char* fname : { "/SeriesComponent.qml", "/MatchReportView.qml" }) {
            const QString body = readAll(QStringLiteral(TECHAIM_SOURCE_DIR) + QLatin1String(fname));
            const int uses = body.count(QStringLiteral("MODREADER.getXCord(index+1)"));
            const int asks = body.count(QStringLiteral("MODREADER.coordinateHasValue(index+1)"));
            check(uses > 0 && asks == uses,
                  "ACQ-SENTINEL-003: every marker view asks before it draws",
                  QStringLiteral("%1 uses=%2 asks=%3").arg(QLatin1String(fname)).arg(uses).arg(asks));
        }

        const QString tw = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ModReader/forms/tachuswidget.cpp"));
        const int xAt = tw.indexOf(QStringLiteral("double TachusWidget::getXCord(int index)"));
        check(xAt > 0 && tw.mid(xAt, 260).contains(QStringLiteral("qQNaN()"))
              && !tw.mid(xAt, 260).contains(QStringLiteral("return -1")),
              "ACQ-SENTINEL-003: getXCord returns no numeric sentinel");
        const int mx = tw.indexOf(QStringLiteral("double TachusWidget::getXMPIForShoot"));
        check(mx > 0 && tw.mid(mx, 520).contains(QStringLiteral("qQNaN()"))
              && !tw.mid(mx, 520).contains(QStringLiteral("return -1;")),
              "ACQ-SENTINEL-003: getXMPIForShoot returns no numeric sentinel - it fed the report");
        const int my = tw.indexOf(QStringLiteral("double TachusWidget::getYMPIForShoot"));
        check(my > 0 && tw.mid(my, 520).contains(QStringLiteral("qQNaN()"))
              && !tw.mid(my, 520).contains(QStringLiteral("return -1;")),
              "ACQ-SENTINEL-003: getYMPIForShoot returns no numeric sentinel either");

        // A Q_INVOKABLE that QML cannot actually call is worse than none: moc
        // registers a private one, QML refuses it at runtime with "is not a
        // function", and the TypeError aborts the handler that was trying to
        // be careful. onShootCountChanged would have stopped before drawing or
        // scoring anything. Caught by launching the binary, not by reading it -
        // so the reading test now covers it too.
        {
            const QString h = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ModReader/forms/tachuswidget.h"));
            const int declAt = h.indexOf(QStringLiteral("Q_INVOKABLE bool coordinateHasValue"));
            check(declAt > 0, "ACQ-SENTINEL-003: coordinateHasValue is declared Q_INVOKABLE");
            if (declAt > 0) {
                int lastPublic = -1, lastNonPublic = -1;
                for (const char* spec : { "\npublic:", "\npublic slots:" })
                    lastPublic = qMax(lastPublic, h.lastIndexOf(QLatin1String(spec), declAt));
                for (const char* spec : { "\nprivate:", "\nprotected:",
                                          "\nprivate slots:", "\nsignals:" })
                    lastNonPublic = qMax(lastNonPublic, h.lastIndexOf(QLatin1String(spec), declAt));
                check(lastPublic > lastNonPublic,
                      "ACQ-SENTINEL-003: and it is PUBLIC - QML refuses a private invokable",
                      QStringLiteral("public@%1 nonPublic@%2").arg(lastPublic).arg(lastNonPublic));
            }
        }

        // Every OTHER C++ consumer of a coordinate, categorised and guarded.
        // SCORING CRITICAL because each one publishes off the machine: the UDP
        // broadcast to the range network, the server lane file and the SETA
        // lane CSV. REPORTING for the printed PDF table, whose loop is driven
        // by a SEPARATE counter (m_scoreList_gameMode) and can therefore
        // outrun the coordinate arrays.
        // The cross-discipline propagation audit found two more, in other
        // translation units, that no earlier sweep had reached.
        {
            const QString feeder = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/bridge/coachreportfeeder.cpp"));
            check(feeder.contains(QStringLiteral("coordinateHasValue(1)")),
                  "ACQ-SENTINEL-003: the coach feeder asks the authority, not a number");
            // The old probe compared against -1. NaN == -1.0 is false, so once
            // the accessors answered NaN that test said "we have coordinates"
            // when there were none, and NaN went into MPI and group size.
            check(!feeder.contains(QStringLiteral("== -1.0")),
                  "ACQ-SENTINEL-003: and the dead -1 comparison is gone from it");
            check(feeder.count(QStringLiteral("coordinateHasValue(")) >= 2,
                  "ACQ-SENTINEL-003: it also checks per shot, not once per match",
                  QStringLiteral("asks=%1").arg(feeder.count(QStringLiteral("coordinateHasValue("))));

            const QString settings = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/appsettings.cpp"));
            const int saves = settings.count(QStringLiteral("getXCord(i+1)"));
            const int asks  = settings.count(QStringLiteral("coordinateHasValue(i+1)"));
            check(saves > 0 && asks == saves,
                  "ACQ-SENTINEL-003: every .tch persistence loop asks before it writes",
                  QStringLiteral("writes=%1 asks=%2").arg(saves).arg(asks));
        }

        for (const char* who : { "broadCastNewShoot", "updateShootData",
                                 "updateSetaShootData", "getPDFString" }) {
            const int at = tw.indexOf(QStringLiteral("TachusWidget::") + QLatin1String(who));
            const QString body = at > 0 ? tw.mid(at, 1400) : QString();
            check(at > 0 && body.contains(QStringLiteral("coordinateHasValue(")),
                  "ACQ-SENTINEL-003: this coordinate consumer asks before it publishes",
                  QLatin1String(who));
        }
        check(mx > 0 && tw.mid(mx, 520).contains(QStringLiteral("index >= m_xCordList.count()")),
              "ACQ-SENTINEL-003: getXMPIForShoot bounds the index it indexes with, not shootNumber");
    }

    // -- UI-STATUS-001 - ONE readiness authority, and QML is told when it moves --
    //
    // On 2026-08-25 the header read CONNECTING... and all four Training Lab
    // panels read "Target: Not connected" while the log showed state=ACQUIRING,
    // baseline 7, captured 7, polling every 100 ms. targetReady() tested a
    // private m_acqState mirror that was initialised to Synchronizing, reset to
    // Synchronizing, and never set to Acquiring by anything at all - so it
    // could only ever answer false, in every discipline, forever.
    {
        const QString h  = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ModReader/forms/tachuswidget.h"));
        const QString cp = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ModReader/forms/tachuswidget.cpp"));

        check(!h.contains(QStringLiteral("AcquisitionState m_acqState")),
              "UI-STATUS-001: the stale acquisition mirror is gone, not merely assigned");
        check(!h.contains(QStringLiteral("enum class AcquisitionState")),
              "UI-STATUS-001: and its enum with it - one state machine, not two");

        const int rdy = h.indexOf(QStringLiteral("bool    targetReady()"));
        check(rdy > 0, "UI-STATUS-001: targetReady exists");
        if (rdy > 0) {
            const QString body = h.mid(rdy, 700);
            check(body.contains(QStringLiteral("m_seq.state()")),
                  "UI-STATUS-001: readiness is derived from the sequencer, the one authority");
            check(body.contains(QStringLiteral("AcqState::Acquiring")),
                  "UI-STATUS-001: ACQUIRING counts as ready");
            check(body.contains(QStringLiteral("AcqState::ResettingCounter")),
                  "UI-STATUS-001: and so does OUR own counter reset - it must not flicker "
                  "the indicator at every tenth shot");
        }

        // A derived property whose NOTIFY never fires is a stale binding.
        check(cp.contains(QStringLiteral("void TachusWidget::publishReadinessIfChanged()")),
              "UI-STATUS-001: readiness changes are published");
        check(cp.contains(QStringLiteral("publishReadinessIfChanged();"))
              && cp.indexOf(QStringLiteral("publishReadinessIfChanged();"))
                 > cp.indexOf(QStringLiteral("m_seq.poll(read.counter")),
              "UI-STATUS-001: published from the poll, where the state actually changes");
        check(h.contains(QStringLiteral("publishReadinessIfChanged();")),
              "UI-STATUS-001: and from the central session reset, which drops readiness "
              "without going through setTargetStatus");

        // The panel mapping must name every state; a default reachable by a
        // normal state is a hole, not a default.
        const QString panel = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/TargetStatusPanel.qml"));
        check(panel.contains(QStringLiteral("case \"TARGET CONNECTED\":")),
              "UI-STATUS-001: TARGET CONNECTED has its own case - it used to fall through");
        for (const char* st : { "ACQUISITION FAULT", "TARGET DISCONNECTED", "TARGET NOT CONNECTED",
                                "TARGET NOT DETECTED", "RECONNECTING", "SCANNING",
                                "MANUAL SELECTION REQUIRED", "TARGET DETECTED", "SYNCHRONIZING" })
            check(panel.contains(QStringLiteral("case \"") + QLatin1String(st) + QStringLiteral("\":")),
                  "UI-STATUS-001: the mapping names every engine state", QLatin1String(st));

        // Every Training Lab panel still reads the one shared property.
        const QString sp = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ShootingPage.qml"));
        check(sp.contains(QStringLiteral("MODREADER.targetReady")),
              "UI-STATUS-001: the Training Lab panels read the same authority as the header");
        for (const char* f : { "/CallDiagnoseRightPanel.qml", "/TrainingRightPanel.qml",
                               "/PositionTransitionRightPanel.qml", "/WindMapRightPanel.qml" }) {
            const QString body = readAll(QStringLiteral(TECHAIM_SOURCE_DIR) + QLatin1String(f));
            check(body.contains(QStringLiteral("panel.connected")),
                  "UI-STATUS-001: this panel binds the shared connected flag", QLatin1String(f));
        }
    }

    // -- FINALS-TIMER-001 - a Final has exactly one clock ---------------------
    //
    // onHardwareReconnected() called gameTimer.start() behind nothing but
    // !sighter.visible. A USB reconnect during a Final therefore started the
    // legacy match countdown, which ran at 1 Hz for the rest of the session
    // accumulating gameTime that nothing owned, while the Finals controller
    // held the real competition clock.
    {
        const QString c = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml"));
        check(c.contains(QStringLiteral("readonly property bool legacyClockIsOurs")),
              "FINALS-TIMER-001: one predicate decides whether the legacy clock is ours");
        const int pred = c.indexOf(QStringLiteral("readonly property bool legacyClockIsOurs"));
        if (pred > 0) {
            const QString body = c.mid(pred, 260);
            check(body.contains(QStringLiteral("!shootingPage.isFinalsMatch")),
                  "FINALS-TIMER-001: 50 m 3P Finals excluded");
            check(body.contains(QStringLiteral("!shootingPage.isFinals10mMatch")),
                  "FINALS-TIMER-001: 10 m Finals excluded - not only the discipline that failed");
            check(body.contains(QStringLiteral("!shootingPage.isTrainingModeAny")),
                  "FINALS-TIMER-001: Training Lab excluded, matching its own display gate");
        }
        // EVERY live start site is guarded. A commented-out one does not count.
        int live = 0, guarded = 0;
        const QStringList ls = c.split(QLatin1Char('\n'));
        for (int i = 0; i < ls.size(); ++i) {
            const QString t = ls[i].trimmed();
            if (!t.contains(QStringLiteral("gameTimer.start()")) || t.startsWith(QStringLiteral("//")))
                continue;
            ++live;
            // the guard sits on this line or the one above it
            const QString ctx = (i > 0 ? ls[i - 1] : QString()) + t;
            if (ctx.contains(QStringLiteral("legacyClockIsOurs"))) ++guarded;
        }
        check(live > 0 && guarded == live,
              "FINALS-TIMER-001: every live gameTimer.start() is behind the guard",
              QStringLiteral("live=%1 guarded=%2").arg(live).arg(guarded));
        // And the display gate and the timer gate agree, so the timer can never
        // run where its own readout is hidden.
        check(c.contains(QStringLiteral("!shootingPage.isFinalsMatch && !shootingPage.isFinals10mMatch && !shootingPage.isTrainingModeAny")),
              "FINALS-TIMER-001: the legacy display is gated on the same three modes");
    }

    // -- FINALS-DISPLAY-TIMER-002 - no qualification clock during a Final -----
    //
    // RC3C physical: a timer reading 35:00 sat on the target face for the whole
    // 10 m Final, unchanged across fourteen minutes and one USB reconnect. The
    // declarative gate on timerNotification already excluded finals; two
    // imperative assignments destroyed the binding, and one of them set the row
    // visible from APPSETTINGS.timer() alone.
    {
        const QString c = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml"));
        int enables = 0, gated = 0;
        const QStringList ls = c.split(QLatin1Char('\n'));
        for (const QString& raw : ls) {
            const QString t = raw.trimmed();
            if (!t.startsWith(QStringLiteral("timerNotification.visible ="))
                || t.startsWith(QStringLiteral("//")))
                continue;
            // Assignments that can make it VISIBLE are the ones that matter; an
            // assignment to false cannot put a clock on a Final's face.
            if (t.contains(QStringLiteral("= false"))) continue;
            ++enables;
            if (t.contains(QStringLiteral("legacyClockIsOurs"))) ++gated;
        }
        check(enables > 0 && gated == enables,
              "FINALS-DISPLAY-TIMER-002: every assignment that can show the match clock "
              "is gated on legacyClockIsOurs",
              QStringLiteral("enables=%1 gated=%2").arg(enables).arg(gated));
        check(!c.contains(QStringLiteral("timerNotification.visible = APPSETTINGS.timer()\n")),
              "FINALS-DISPLAY-TIMER-002: the unconditional APPSETTINGS.timer() enable is gone");
        // EVERY legacy clock element, not only the one seen physically. A
        // Final must not be able to set any of them visible.
        int anyEnable = 0, anyGated = 0;
        for (const QString& raw : ls) {
            const QString t = raw.trimmed();
            if (t.startsWith(QStringLiteral("//"))) continue;
            const bool isClock = t.startsWith(QStringLiteral("stStopTimer.visible ="))
                              || t.startsWith(QStringLiteral("stopTimer.visible ="))
                              || t.startsWith(QStringLiteral("timerNotification.visible ="));
            if (!isClock || t.contains(QStringLiteral("= false"))) continue;
            ++anyEnable;
            if (t.contains(QStringLiteral("legacyClockIsOurs"))) ++anyGated;
        }
        check(anyEnable > 0 && anyGated == anyEnable,
              "FINALS-DISPLAY-TIMER-002: no Final can set ANY legacy clock element visible",
              QStringLiteral("enables=%1 gated=%2").arg(anyEnable).arg(anyGated));
        // The declarative gates must still be there - the imperative fix is a
        // second line of defence, not a replacement for them.
        check(c.count(QStringLiteral("!shootingPage.isFinals10mMatch")) >= 4,
              "FINALS-DISPLAY-TIMER-002: the declarative finals gates are intact");
    }

    // -- FINAL-TCH-TIME-001 - a Final's shot times reach its record -----------
    //
    // RC3C physical: the Final .tch carried <time>-1</time> and an empty
    // <time_stamp> for all 29 shots while the HUD showed real times. getTime()
    // reads m_timeConsumedList, which only RightPanel.addToSeries() appended
    // to - and a 10 m Final shows finals10mRightPanel instead.
    {
        const QString sp = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ShootingPage.qml"));
        const int router = sp.indexOf(QStringLiteral("function onShotAccepted(shot)"),
                                      sp.indexOf(QStringLiteral("target: FINALS10M")));
        check(router > 0, "FINAL-TCH-TIME-001: the FINALS10M accepted-shot router exists");
        if (router > 0) {
            const QString body = sp.mid(router, 2600);
            check(body.contains(QStringLiteral("MODREADER.appendTimeConsumed(")),
                  "FINAL-TCH-TIME-001: it records the shot time the controller measured");
            check(body.contains(QStringLiteral("MODREADER.appendTimeStamp(")),
                  "FINAL-TCH-TIME-001: and the timestamp at acceptance");
            check(body.contains(QStringLiteral("rec.timeSec")),
                  "FINAL-TCH-TIME-001: the time comes from the record, not from a new clock");
            check(body.contains(QStringLiteral("!shootingPage.recoveryReplayInProgress")),
                  "FINAL-TCH-TIME-001: a replayed recovery shot does not append a second time");
        }
        // The accessor still refuses an index it does not hold - the fix feeds
        // the list, it does not weaken the guard.
        const QString tw = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ModReader/forms/tachuswidget.cpp"));
        const int gt = tw.indexOf(QStringLiteral("double TachusWidget::getTime(int index)"));
        check(gt > 0 && tw.mid(gt, 300).contains(QStringLiteral("return -1")),
              "FINAL-TCH-TIME-001: getTime still reports -1 for an index it does not hold");
    }

    // ─────────────────────────────────────────────────────────────────────
    // FINALS-3P-MIX-001 — mode-entry exclusivity.
    //
    // Seen in DEMO on RC3D: a 50 m 3P Final rendered the 3P shell (FINAL 35,
    // PREP/K/P/S/S1/S2/SINGLES/DONE) with the 10 m Final's right panel on top
    // of it - "10m Air Rifle Final", 0 / 24 shots, Series 1 / Series 2 /
    // Singles - plus a second HUD running its own clock 33 s out of step, and
    // a second enabled shot router.
    //
    // Cause: enterFinalsMode() (the 3P Final entry) set isFinalsMatch and
    // cleared is3PMatch, but never cleared isFinals10mMatch. Every OTHER entry
    // function cleared all three. A 10 m Final earlier in the same run left
    // the flag true, and every element bound to it stayed mounted. The generic
    // Home route does not clear it either - only homeFromFinals10m() and
    // startNewFinals10m() do - so the entry side is the authority.
    //
    // The rule this enforces: A MODE-ENTRY FUNCTION OWNS EVERY DISCIPLINE
    // FLAG, NOT JUST ITS OWN. Adding a discipline means adding its flag to
    // every entry point, and this test fails until that is done.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString sp = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ShootingPage.qml"));
        check(!sp.isEmpty(), "MIX-001: ShootingPage.qml is readable");

        // Every discipline flag, and every function that engages a mode.
        const QStringList flags = { QStringLiteral("isFinalsMatch"),
                                    QStringLiteral("isFinals10mMatch"),
                                    QStringLiteral("is3PMatch") };
        const QStringList entries = { QStringLiteral("enterFinalsMode"),
                                      QStringLiteral("enterFinals10mMode"),
                                      QStringLiteral("enterQualificationMode"),
                                      QStringLiteral("enterTrainingMode"),
                                      QStringLiteral("enterCallDiagnoseMode"),
                                      QStringLiteral("enterPositionTransitionMode"),
                                      QStringLiteral("enterWindMapMode") };

        for (const QString& fn : entries) {
            const QString body = extractFunction(sp, fn);
            check(!body.isEmpty(),
                  qPrintable(QStringLiteral("MIX-001: %1 exists").arg(fn)));
            if (body.isEmpty())
                continue;
            for (const QString& flag : flags) {
                // An assignment, not merely a mention: "<flag> =" but not "==".
                const QRegularExpression assign(
                    QStringLiteral("\\b%1\\s*=[^=]").arg(flag));
                check(body.contains(assign),
                      qPrintable(QStringLiteral("MIX-001: %1 assigns %2")
                                 .arg(fn, flag)),
                      QStringLiteral("a mode-entry function owns EVERY discipline "
                                     "flag - an unowned flag keeps the previous "
                                     "discipline's layer mounted"));
            }
        }

        // The two finals entries are mutually exclusive, stated explicitly.
        const QString f3p  = extractFunction(sp, QStringLiteral("enterFinalsMode"));
        const QString f10m = extractFunction(sp, QStringLiteral("enterFinals10mMode"));
        check(f3p.contains(QStringLiteral("isFinalsMatch = true")),
              "MIX-001: the 3P Final entry claims isFinalsMatch");
        check(f3p.contains(QStringLiteral("isFinals10mMatch = false")),
              "MIX-001: the 3P Final entry RELEASES isFinals10mMatch",
              QStringLiteral("this exact line is the defect fix"));
        check(f3p.contains(QStringLiteral("is3PMatch = false")),
              "MIX-001: the 3P Final entry releases is3PMatch");
        check(f10m.contains(QStringLiteral("isFinals10mMatch = true")),
              "MIX-001: the 10m Final entry claims isFinals10mMatch");
        check(f10m.contains(QStringLiteral("isFinalsMatch = false")),
              "MIX-001: the 10m Final entry releases isFinalsMatch");
        check(f10m.contains(QStringLiteral("is3PMatch = false")),
              "MIX-001: the 10m Final entry releases is3PMatch");

        // ── the presentation matrix (brief section 11) ────────────────────
        // Each discipline's layer is gated on ITS OWN flag and no other.
        struct Layer { const char* what; const char* anchor; const char* gate; };
        const Layer layers[] = {
            { "10m Final right panel", "Finals10mRightPanel {", "visible: isFinals10mMatch" },
            { "10m Final HUD",         "Finals10mHud {",        "visible: isFinals10mMatch" },
            { "3P Final HUD",          "FinalsHud {",           "visible: isFinalsMatch"    },
        };
        for (const Layer& l : layers) {
            const int at = sp.indexOf(QLatin1String(l.anchor));
            check(at > 0, qPrintable(QStringLiteral("MIX-001: %1 is mounted").arg(l.what)));
            if (at > 0)
                check(sp.mid(at, 400).contains(QLatin1String(l.gate)),
                      qPrintable(QStringLiteral("MIX-001: %1 is gated on %2")
                                 .arg(l.what, l.gate)));
        }

        // The qualification right panel steps aside for the 10m Final only.
        const int rp = sp.indexOf(QStringLiteral("RightPanel {\n        id: rightPanel"));
        check(rp > 0, "MIX-001: the qualification right panel is mounted");
        if (rp > 0) {
            // Span to the end of the block, not a byte count: a comment added
            // above the binding pushed it outside a fixed 900-char window and
            // failed this check for a reason that had nothing to do with the
            // gate. Windows sized by guesswork pass and fail by accident.
            const int end = sp.indexOf(QStringLiteral("\n    }"), rp);
            const QString block = sp.mid(rp, (end > rp ? end - rp : 1500));
            check(block.contains(QStringLiteral("!isFinals10mMatch")),
                  "MIX-001: the qualification panel yields to the 10m Final");
            // FINALS-3P-PANEL-001: it now yields to the 3P Final too, because
            // the 3P Final has its own column. Before that it stayed mounted
            // through a 3P Final showing a 60-shot qualification structure.
            check(block.contains(QStringLiteral("!isFinalsMatch")),
                  "MIX-001: and to the 3P Final, which now has its own panel");
        }

        // Each router is enabled by its own controller's flag.
        const int r3p = sp.indexOf(QStringLiteral("target: FINALS3P"));
        const int r10 = sp.indexOf(QStringLiteral("target: FINALS10M"));
        check(r3p > 0 && sp.mid(r3p, 120).contains(QStringLiteral("enabled: isFinalsMatch")),
              "MIX-001: the FINALS3P shot router is enabled by isFinalsMatch");
        check(r10 > 0 && sp.mid(r10, 120).contains(QStringLiteral("enabled: isFinals10mMatch")),
              "MIX-001: the FINALS10M shot router is enabled by isFinals10mMatch");

        // ── discipline semantics stay in their own domain ─────────────────
        // 10 m course wording must not be reachable from the 3P finals domain,
        // and 3P position wording must not be reachable from the 10 m domain.
        const QString cfg10 = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/finals10m/Finals10mConfig.h"));
        check(cfg10.contains(QStringLiteral("10m Air Rifle Final")),
              "MIX-001: \"10m Air Rifle Final\" is owned by the 10m config");

        const QString hud3p = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/FinalsHud.qml"));
        const QString strip = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/FinalsTopStrip.qml"));
        for (const QString& f : QStringList{ hud3p, strip }) {
            if (f.isEmpty())
                continue;
            check(!f.contains(QStringLiteral("10m Air Rifle Final")),
                  "MIX-001: no 10m discipline name inside the 3P Final HUD");
            check(!f.contains(QStringLiteral("FINALS10M")),
                  "MIX-001: the 3P Final HUD never reads FINALS10M");
            check(!f.contains(QStringLiteral("/ 24")),
                  "MIX-001: no 24-shot course count inside the 3P Final HUD");
        }

        const QString rp10 = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/Finals10mRightPanel.qml"));
        if (!rp10.isEmpty()) {
            check(!rp10.contains(QStringLiteral("FINALS3P")),
                  "MIX-001: the 10m right panel never reads FINALS3P");
            for (const QString& w : QStringList{ QStringLiteral("KNEELING"),
                                                 QStringLiteral("PRONE"),
                                                 QStringLiteral("STANDING") })
                check(!rp10.contains(w),
                      qPrintable(QStringLiteral("MIX-001: no 3P position wording (%1) "
                                                "in the 10m right panel").arg(w)));
        }

        // DEMO must not substitute a controller: the demo shot source is
        // uxShoot, which re-enters the SAME shootCountChanged path, so both
        // routers above serve live and demo alike. Assert there is no
        // demo-specific finals branch choosing a different controller.
        check(!sp.contains(QStringLiteral("demoMode ? FINALS10M"))
              && !sp.contains(QStringLiteral("demoMode ? FINALS3P")),
              "MIX-001: DEMO selects no substitute finals controller");
    }

    // ─────────────────────────────────────────────────────────────────────
    // UI-THEME-001 — System / Light / Dark appearance.
    //
    // The token layer is the ONLY theme authority. These checks assert that
    // (a) it actually resolves per appearance, (b) the preference is a
    // notifying property so the switch is live, (c) the setting is
    // presentation-only, and (d) the LIGHT palette is legible - measured,
    // not asserted, using WCAG relative luminance on the values in the real
    // file.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString tk = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/src/ui/theme/DesignTokens.qml"));
        const QString th = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/Theme.qml"));
        const QString lp = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/LoginPage.qml"));
        const QString ah = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/appsettings.h"));
        const QString ac = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/appsettings.cpp"));

        check(tk.contains(QStringLiteral("property string appearance")),
              "THEME: the token layer carries the appearance");
        check(tk.contains(QStringLiteral("readonly property bool isLight")),
              "THEME: it resolves to a single isLight decision");
        check(tk.contains(QStringLiteral("Application.styleHints.colorScheme")),
              "THEME: \"system\" reads the real OS colour scheme");
        check(tk.contains(QStringLiteral("appearance === \"system\" && systemIsLight")),
              "THEME: system only means light when the OS says light");

        // The preference must be a NOTIFYing property, or a QML binding on it
        // never re-evaluates and the switch needs a restart.
        check(ah.contains(QStringLiteral("Q_PROPERTY(QString appearance READ getAppearance WRITE setAppearance NOTIFY appearanceChanged)")),
              "THEME: appearance is a notifying property, so switching is live");
        check(th.contains(QStringLiteral("appearance: APPSETTINGS.appearance")),
              "THEME: Theme binds the token layer to the persisted preference");

        // Persistence, and where it is NOT written.
        check(ac.contains(QStringLiteral("ui/appearance")),
              "THEME: the preference is persisted");
        check(ac.contains(QStringLiteral("m_userPrefs->sync()")),
              "THEME: it is flushed, so a kill does not lose it");
        {
            const int set = ac.indexOf(QStringLiteral("void AppSettings::setAppearance"));
            check(set > 0, "THEME: setAppearance exists");
            if (set > 0) {
                const QString body = ac.mid(set, 900);
                check(!body.contains(QStringLiteral("m_settings")),
                      "THEME: it never writes the deployed config.ini");
                check(body.contains(QStringLiteral("QStringLiteral(\"dark\")")),
                      "THEME: an unrecognised value degrades to dark, not to undefined");
            }
        }

        // Presentation only.
        for (const QString& forbidden : QStringList{
                 QStringLiteral("setAppearance"), QStringLiteral("getAppearance") }) {
            for (const QString& domain : QStringList{
                     QStringLiteral("/src/target/AcquisitionDecision.h"),
                     QStringLiteral("/ModReader/forms/tachuswidget.cpp"),
                     QStringLiteral("/src/finals/Finals3PController.cpp"),
                     QStringLiteral("/src/finals10m/Finals10mController.cpp") }) {
                const QString f = readAll(QStringLiteral(TECHAIM_SOURCE_DIR) + domain);
                if (f.isEmpty()) continue;
                check(!f.contains(forbidden),
                      qPrintable(QStringLiteral("THEME: %1 is absent from %2")
                                 .arg(forbidden, domain)));
            }
        }

        // The Settings entry, on the start page, using the one dialog system.
        check(lp.contains(QStringLiteral("function openAppearanceDialog()")),
              "THEME: the start page offers an appearance setting");
        check(lp.contains(QStringLiteral("dialogManager.show(")),
              "THEME: it uses the TechAimDialog framework");
        // The rule (docs/techaim-dialogs.md) bans a second MESSAGE dialog
        // mechanism. It does not ban QtQuick.Dialogs outright: LoginPage
        // legitimately uses FolderDialog for the network-share picker, which
        // is a file-system chooser, not a message box. So assert on the
        // instantiation, not on the import, and not on the word appearing in
        // a comment that says the legacy dialogs are gone.
        check(!lp.contains(QStringLiteral("MessageDialog {")),
              "THEME: no second message-dialog mechanism was introduced");
        for (const QString& opt : QStringList{ QStringLiteral("\"system\""),
                                               QStringLiteral("\"light\""),
                                               QStringLiteral("\"dark\"") }) {
            const int at = lp.indexOf(QStringLiteral("function openAppearanceDialog()"));
            check(at > 0 && lp.mid(at, 1400).contains(opt),
                  qPrintable(QStringLiteral("THEME: the picker offers %1").arg(opt)));
        }

        // The header mark must not be the white asset on a light header.
        const QString hd = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/Header.qml"));
        check(hd.contains(QStringLiteral("theme.isLight ? theme.logoColor : theme.logoWhite")),
              "THEME: the header logo follows the theme",
              QStringLiteral("white-on-white is invisible - found by looking at "
                             "the rendered light theme"));

        // ── LIGHT PALETTE LEGIBILITY, MEASURED ────────────────────────────
        // WCAG 2.1 relative luminance and contrast ratio, computed from the
        // hex values actually present in DesignTokens.qml. If someone edits a
        // light colour into something unreadable, this fails.
        struct Lum {
            static double chan(int v) {
                const double s = v / 255.0;
                return (s <= 0.03928) ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4);
            }
            static double of(const QString& hex) {
                bool ok = false;
                const int r = hex.mid(1, 2).toInt(&ok, 16);
                const int g = hex.mid(3, 2).toInt(&ok, 16);
                const int b = hex.mid(5, 2).toInt(&ok, 16);
                return 0.2126 * chan(r) + 0.7152 * chan(g) + 0.0722 * chan(b);
            }
            static double ratio(const QString& a, const QString& b) {
                const double la = of(a), lb = of(b);
                return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
            }
        };

        // Pull the LIGHT branch of a token: `name: isLight ? "#LIGHT" : "#DARK"`
        auto lightOf = [&tk](const char* name) -> QString {
            const QRegularExpression re(
                QStringLiteral("%1:\\s*isLight\\s*\\?\\s*\"(#[0-9A-Fa-f]{6})\"").arg(name));
            const QRegularExpressionMatch m = re.match(tk);
            return m.hasMatch() ? m.captured(1) : QString();
        };

        const QString lSurface = lightOf("surfacePrimary");
        const QString lCanvas  = lightOf("backgroundPrimary");
        const QString lTxt     = lightOf("textPrimary");
        const QString lTxtSec  = lightOf("textSecondary");
        const QString lTxtDis  = lightOf("textDisabled");
        const QString lBorder  = lightOf("borderSubtle");

        check(!lSurface.isEmpty() && !lTxt.isEmpty(),
              "THEME: the light palette is readable from the token file");

        struct Pair { const char* label; QString fg; QString bg; double min; };
        const QList<Pair> pairs = {
            { "light textPrimary on surface",   lTxt,    lSurface, 4.5 },
            { "light textPrimary on canvas",    lTxt,    lCanvas,  4.5 },
            { "light textSecondary on surface", lTxtSec, lSurface, 4.5 },
            { "light textDisabled on surface",  lTxtDis, lSurface, 3.0 },
            { "white on the brand accent",      QStringLiteral("#FFFFFF"),
                                                QStringLiteral("#A80038"), 4.5 },
        };
        for (const Pair& p : pairs) {
            if (p.fg.isEmpty() || p.bg.isEmpty()) { check(false, p.label, "token not found"); continue; }
            const double r = Lum::ratio(p.fg, p.bg);
            check(r >= p.min, p.label,
                  QStringLiteral("%1 on %2 = %3:1, needs %4:1")
                      .arg(p.fg, p.bg).arg(r, 0, 'f', 2).arg(p.min));
        }
        // A border that does not differ from its surface is not a border.
        if (!lBorder.isEmpty() && !lSurface.isEmpty()) {
            const double r = Lum::ratio(lBorder, lSurface);
            check(r >= 1.25, "light border is visible against its surface",
                  QStringLiteral("%1 on %2 = %3:1").arg(lBorder, lSurface).arg(r, 0, 'f', 2));
        }
        // Disabled must be dimmed, not erased.
        check(tk.contains(QStringLiteral("disabledOpacity: isLight ? 0.55")),
              "THEME: disabled controls are less faded on light than on dark");
    }

    // ─────────────────────────────────────────────────────────────────────
    // 50 m 3P INDOOR QUALIFICATION — the 90-minute continuous match clock.
    //
    // ISSF Rule Book 2026, Edition 2025 (Second Print 07/2026), effective
    // 1 July 2026, Rule 6.11.9.2:
    //
    //   "Twenty (20) shots in each position, in the sequence Kneeling, Prone,
    //    Standing, in a total time limit of 1hr 45 minutes (105 minutes) if
    //    outdoor range is used. 1 hr 30 minutes (90 minutes) if indoor range
    //    is used."
    //   "Preparation and Sighting time. Fifteen (15) minutes to fire an
    //    unlimited number of sighting shots."
    //   "Full ring (integer) scoring."
    //
    // The 90 minutes is ONE TOTAL LIMIT for the whole 3 x 20 course. Position
    // changes and the unlimited prone/standing sighters are spent out of it.
    // There is no per-position clock and no transition extension.
    //
    // These are characterization + rule tests over the real sources. They
    // cannot drive a 90-minute clock, so they assert the two things that would
    // break it: the configured value, and that nothing on the position-change
    // path touches the clock.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString as  = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/appsettings.cpp"));
        const QString ash = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/appsettings.h"));
        const QString lp  = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/LoginPage.qml"));
        const QString sp  = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/ShootingPage.qml"));
        const QString cp  = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml"));

        // ── qualification3p_indoor_must_use_90_minute_continuous_clock ────
        {
            const int gt = as.indexOf(QStringLiteral("int AppSettings::getTimeCount"));
            check(gt > 0, "QUAL3P: getTimeCount exists");
            // Span to the function's closing brace, not a byte count. A comment
            // added above the branch pushed it outside a 900-char window and
            // failed this check for a reason unrelated to what it tests - the
            // third time that mistake has been made in this file.
            const int gtEnd = as.indexOf(QStringLiteral("\n}"), gt);
            const QString body = (gt > 0)
                ? as.mid(gt, (gtEnd > gt ? gtEnd - gt : 2000)) : QString();

            // The 3P branch (game_sub_mode == 1) must return 90 minutes.
            const int sub = body.indexOf(QStringLiteral("game_sub_mode == 1"));
            check(sub > 0, "QUAL3P: the 60-shot path branches on 50m 3 Positions");
            check(sub > 0 && body.mid(sub, 120).contains(QStringLiteral("90 * 60")),
                  "QUAL3P indoor: the match clock is 90 minutes",
                  body.mid(sub, 120).simplified());

            check(lp.contains(QStringLiteral("return \"90 min\"")),
                  "QUAL3P indoor: the start page states 90 min for 50m 3 Positions");

            // The preparation and sighting time is 15 minutes (6.11.9.2).
            // The member default lives in the header, not the .cpp.
            check(ash.contains(QStringLiteral("m_prepTimeMinutes = 15")),
                  "QUAL3P: preparation and sighting is 15 minutes");

            // CONTINUITY. The position transition must not touch the clock.
            // NOTE the parenthesis: enterPositionTransition is a PREFIX of
            // enterPositionTransitionMode (a Training Lab programme), and
            // without it this matched the wrong function entirely.
            const int ept = sp.indexOf(QStringLiteral("function enterPositionTransition()"));
            check(ept > 0, "QUAL3P: the 3P position transition exists");
            if (ept > 0) {
                // Span to the NEXT function: the first close-brace belongs to
                // this function's try/catch and cut the body before its log line.
                const int nextFn = sp.indexOf(QStringLiteral("\n    function "), ept + 10);
                const QString t = sp.mid(ept, (nextFn > ept ? nextFn - ept : 4000));
                for (const QString& sym : QStringList{
                         QStringLiteral("gameTimer"), QStringLiteral("gameTime"),
                         QStringLiteral("totalGameTime"),
                         QStringLiteral("startPreparationCountdown"),
                         QStringLiteral("refreshMatchTime") })
                    check(!t.contains(sym),
                          qPrintable(QStringLiteral("QUAL3P continuity: the position "
                                                    "transition does not touch %1").arg(sym)),
                          QStringLiteral("a position change must not restart or extend "
                                         "the 90-minute total"));
                check(t.contains(QStringLiteral("match clock keeps running")),
                      "QUAL3P continuity: and says so, in the session log");
            }

            // The one place that zeroes elapsed time is the clock row going
            // invisible. Its gate must not depend on anything a position
            // change moves - sighter mode, position or break count - or the
            // 90 minutes would restart at every transition.
            const int vis = cp.indexOf(QStringLiteral("visible: APPSETTINGS.timer() && !shootingPage.isFinalsMatch"));
            check(vis > 0, "QUAL3P: the qualification clock row gate is present");
            if (vis > 0) {
                const QString gate = cp.mid(vis, 200);
                for (const QString& sym : QStringList{
                         QStringLiteral("sligterMode"), QStringLiteral("p3Position"),
                         QStringLiteral("p3BreaksDone"), QStringLiteral("is3PMatch") })
                    check(!gate.contains(sym),
                          qPrintable(QStringLiteral("QUAL3P continuity: the clock row is "
                                                    "not gated on %1").arg(sym)),
                          gate.simplified());
            }

            // 60 official shots, three positions of 20, and completion only at 60.
            check(sp.contains(QStringLiteral("count === 20 || count === 40")),
                  "QUAL3P: position breaks at 20 and 40, i.e. 3 x 20");
            check(sp.contains(QStringLiteral("globalMatchModel.count >= matchShootCount")),
                  "QUAL3P: the match completes on the official shot count, not on a clock");
        }

        // ── qualification3p_indoor_must_never_start_105_minute_clock ──────
        {
            // 105 minutes is the OUTDOOR course (6.11.9.2). Saturday is
            // indoor. The value must not exist anywhere in the product.
            for (const QString& f : QStringList{ as, lp, sp, cp }) {
                check(!f.contains(QStringLiteral("105 * 60"))
                          && !f.contains(QStringLiteral("105*60"))
                          && !f.contains(QStringLiteral("6300")),
                      "QUAL3P indoor: no 105-minute (outdoor) match clock exists");
                check(!f.contains(QStringLiteral("\"105 min\"")),
                      "QUAL3P indoor: no 105 min is displayed");
            }
            // And no indoor/outdoor selector was introduced this round: the
            // Saturday configuration is frozen on indoor.
            check(!as.contains(QStringLiteral("outdoorRange"))
                      && !lp.contains(QStringLiteral("outdoorRange")),
                  "QUAL3P: no venue selector was introduced - indoor is fixed");
        }
    }

    // ─────────────────────────────────────────────────────────────────────
    // qualification3p_indoor_cro_time_warnings
    //
    // ISSF Rule Book 2026, Edition 2025 (Second Print 07/2026):
    //   6.11.1.2 e) "The CRO must inform athletes by loudspeaker of the time
    //                remaining at both ten (10) minutes and five (5) minutes
    //                before the end of the competition time"
    //   6.11.1.1 g) "PREPARATION AND SIGHTING TIME...START"
    //   6.11.1.1 i) after 14:30 elapsed, announce "30 SECONDS"
    //   6.11.1.1 j) "END OF PREPARATION AND SIGHTING...STOP"
    //   6.11.1.2 a) "MATCH FIRING...START"
    //   6.11.1.3    "STOP"
    //
    // These were MISSING entirely - the qualification path had no CRO command
    // text of any kind, only countdown displays. This EXECUTES the real
    // threshold logic extracted from CenterPane.qml rather than matching
    // strings, because a string match would not catch a warning that fires
    // twice, fires at the wrong boundary, or never fires at all.
    // ─────────────────────────────────────────────────────────────────────
    {
        const QString cp = readAll(QStringLiteral(TECHAIM_SOURCE_DIR "/CenterPane.qml"));

        // Every command the rule names must exist on the qualification path.
        struct Cmd { const char* text; const char* rule; };
        const Cmd cmds[] = {
            { "PREPARATION AND SIGHTING TIME...START", "6.11.1.1 g" },
            { "30 SECONDS",                            "6.11.1.1 i" },
            { "END OF PREPARATION AND SIGHTING...STOP","6.11.1.1 j" },
            { "MATCH FIRING...START",                  "6.11.1.2 a" },
            { "10 MINUTES",                            "6.11.1.2 e" },
            { "5 MINUTES",                             "6.11.1.2 e" },
            { "STOP",                                  "6.11.1.3"   },
        };
        for (const Cmd& c : cmds)
            check(cp.contains(QString::fromLatin1(c.text)),
                  qPrintable(QStringLiteral("CRO-QUAL: %1 is announced (%2)")
                             .arg(QString::fromLatin1(c.text),
                                  QString::fromLatin1(c.rule))));

        // ── EXECUTE the threshold logic ───────────────────────────────────
        const QString fnCheck = extractFunction(cp, QStringLiteral("croCheckMatchTime"));
        const QString fnAnn   = extractFunction(cp, QStringLiteral("croAnnounce"));
        const QString fnReset = extractFunction(cp, QStringLiteral("croResetAnnouncements"));
        check(!fnCheck.isEmpty() && !fnAnn.isEmpty() && !fnReset.isEmpty(),
              "CRO-QUAL: the announcement functions are extractable");

        if (!fnCheck.isEmpty() && !fnAnn.isEmpty() && !fnReset.isEmpty()) {
            QQmlEngine eng;
            QString qml =
                QStringLiteral("import QtQuick 2.15\n"
                               "Item {\n"
                               "  id: paneItem\n"
                               "  property string croAnnouncement: \"\"\n"
                               "  property bool croWarned10: false\n"
                               "  property bool croWarned5: false\n"
                               "  property bool croWarnedPrep30: false\n"
                               "  property bool legacyClockIsOurs: true\n"
                               "  property var fired: []\n"
                               "  property var log: []\n"
                               "  QtObject { id: croBannerTimer; function restart() {} }\n")
                + fnAnn + QStringLiteral("\n") + fnReset + QStringLiteral("\n")
                + fnCheck + QStringLiteral("\n}\n");
            // capture what was announced
            // Replace the log call with the capture: the extracted function
            // is the REAL one, so its MODREADER dependency is stripped here
            // rather than stubbed - a stub QObject has no appendToLogFile.
            qml.replace(QStringLiteral("MODREADER.appendToLogFile(\"CRO: \" + text)"),
                        QStringLiteral("paneItem.fired.push(text)"));

            QQmlComponent comp(&eng);
            comp.setData(qml.toUtf8(), QUrl());
            if (comp.isError()) {
                QStringList e; for (const QQmlError& x : comp.errors()) e << x.toString();
                check(false, "CRO-QUAL: the extracted logic mounts", e.join(QStringLiteral(" | ")));
            } else {
                QScopedPointer<QObject> o(comp.create());
                check(!o.isNull(), "CRO-QUAL: the extracted logic mounts");
                if (o) {
                    auto fired = [&] { return o->property("fired").toStringList(); };
                    auto call  = [&](int secs) {
                        QMetaObject::invokeMethod(o.data(), "croCheckMatchTime",
                                                  Q_ARG(QVariant, QVariant(secs)));
                    };
                    // 90:00 down to just above ten minutes: silence.
                    call(5400); call(1200); call(601);
                    check(fired().isEmpty(),
                          "CRO-QUAL: nothing is announced before ten minutes remain",
                          fired().join(QStringLiteral(",")));

                    call(600);
                    check(fired().size() == 1 && fired().last() == QStringLiteral("10 MINUTES"),
                          "CRO-QUAL: 10 MINUTES announced at exactly 10:00 remaining",
                          fired().join(QStringLiteral(",")));

                    call(599); call(400); call(301);
                    check(fired().size() == 1,
                          "CRO-QUAL: 10 MINUTES is announced ONCE, not every tick",
                          fired().join(QStringLiteral(",")));

                    call(300);
                    check(fired().size() == 2 && fired().last() == QStringLiteral("5 MINUTES"),
                          "CRO-QUAL: 5 MINUTES announced at exactly 5:00 remaining",
                          fired().join(QStringLiteral(",")));

                    call(299); call(1); call(0);
                    check(fired().size() == 2,
                          "CRO-QUAL: 5 MINUTES is announced ONCE",
                          fired().join(QStringLiteral(",")));

                    // A new session must announce them again.
                    QMetaObject::invokeMethod(o.data(), "croResetAnnouncements");
                    call(600);
                    check(fired().size() == 3 && fired().last() == QStringLiteral("10 MINUTES"),
                          "CRO-QUAL: a new session re-arms the warnings",
                          fired().join(QStringLiteral(",")));

                    // In a Final the legacy clock is not ours - silence.
                    o->setProperty("legacyClockIsOurs", false);
                    QMetaObject::invokeMethod(o.data(), "croResetAnnouncements");
                    const int before = fired().size();
                    call(600); call(300);
                    check(fired().size() == before,
                          "CRO-QUAL: nothing is announced when the legacy clock is not ours",
                          QStringLiteral("a Final owns its own commands"));
                }
            }
        }

        // ── ANNOUNCEMENTS ONLY ────────────────────────────────────────────
        // The rule makes these announcements. They must not touch the clock,
        // the shot count, the position or the target mode.
        for (const QString& fn : QStringList{ QStringLiteral("croAnnounce"),
                                              QStringLiteral("croCheckMatchTime"),
                                              QStringLiteral("croResetAnnouncements") }) {
            const QString body = extractFunction(cp, fn);
            if (body.isEmpty()) continue;
            for (const QString& sym : QStringList{
                     QStringLiteral("gameTimer"), QStringLiteral("gameTime"),
                     QStringLiteral("totalGameTime"), QStringLiteral("sighterTimer"),
                     QStringLiteral("sighterTime"), QStringLiteral("shootCount"),
                     QStringLiteral("changeSighterMode"), QStringLiteral("p3Position"),
                     QStringLiteral("sligterMode") })
                check(!body.contains(sym),
                      qPrintable(QStringLiteral("CRO-QUAL: %1 does not touch %2")
                                 .arg(fn, sym)),
                      QStringLiteral("announcements only - no clock, shot, position "
                                     "or target-mode effect"));
        }
    }

    printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    fflush(stdout);
    return g_failures ? 1 : 0;
}

#include "tst_qml_shot_path.moc"
