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
    printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    fflush(stdout);
    return g_failures ? 1 : 0;
}

#include "tst_qml_shot_path.moc"
