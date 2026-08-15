// CD-REPORT-001 / CD-REPORT-002 offline evidence.
//
// Renders the REAL CallDiagnoseReportView.qml with the REAL archived 50 m
// Call & Diagnose session and reports how many per-shot target diagrams were
// actually instantiated. The defect it guards against printed a heading and a
// legend over an empty page because the Repeater model resolved to undefined,
// so counting scene-graph items - not inspecting the binding - is the test.
//
// Usage: render_cd_report <session.json> <view.qml> <out.png> [shotPageIndex]

#include <QGuiApplication>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlContext>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTimer>
#include <QUrl>
#include <QImage>
#include <QFileInfo>
#include <QQmlError>
#include <cstdio>

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    if (argc < 4) {
        std::printf("usage: render_cd_report <session.json> <view.qml> <out.png> [page]\n");
        return 2;
    }
    const QString jsonPath = QString::fromLocal8Bit(argv[1]);
    const QString viewQml  = QString::fromLocal8Bit(argv[2]);
    const QString outPng   = QString::fromLocal8Bit(argv[3]);
    const int pageIndex    = (argc > 4) ? QString::fromLocal8Bit(argv[4]).toInt() : 0;

    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly)) {
        std::printf("FAIL  cannot read %s\n", qPrintable(jsonPath));
        return 2;
    }
    const QVariantMap session =
        QJsonDocument::fromJson(f.readAll()).object().toVariantMap();
    f.close();

    QQuickView view;
    view.rootContext()->setContextProperty(QStringLiteral("SESSION"), session);
    view.rootContext()->setContextProperty(QStringLiteral("VIEW_URL"),
                                           QUrl::fromLocalFile(viewQml).toString());
    view.setSource(QUrl::fromLocalFile(
        QStringLiteral("%1/RenderHarness.qml").arg(QFileInfo(jsonPath).absolutePath())));
    if (view.status() != QQuickView::Ready) {
        for (const QQmlError& e : view.errors())
            std::printf("QML ERROR  %s\n", qPrintable(e.toString()));
        return 2;
    }
    view.rootObject()->setProperty("shotPageIndex", pageIndex);
    view.resize(794, 1123);
    view.show();

    int rc = 1;
    QObject::connect(view.rootObject(), SIGNAL(ready()), &app, SLOT(quit()));
    QTimer::singleShot(6000, &app, &QGuiApplication::quit);   // never hang
    app.exec();

    const int minis = view.rootObject()->property("miniTargetsFound").toInt();
    const int rows  = view.rootObject()->property("shotRowsFound").toInt();
    const QVariantList shots = session.value(QStringLiteral("shots")).toList();
    // The view paginates 6 per page, so the LAST page carries the remainder.
    const int expect = qBound(0, int(shots.size()) - pageIndex * 6, 6);

    const QImage img = view.grabWindow();
    if (!img.isNull() && img.save(outPng))
        std::printf("saved %s (%dx%d)\n", qPrintable(outPng), img.width(), img.height());

    std::printf("shots in session      : %d\n", int(shots.size()));
    std::printf("expected on page %d    : %d\n", pageIndex, expect);
    std::printf("MiniTarget diagrams   : %d\n", minis);
    std::printf("per-shot text columns : %d\n", rows);

    // The MiniTarget count is the assertion: it is 96x96 by contract and one
    // per shot. The 120-wide count is informational only - it matches both the
    // text column and the wrapped Text inside it, so it is not a 1:1 signal.
    const bool ok = (minis == expect) && (rows >= expect) && expect > 0;
    std::printf("%s  CD-REPORT-001: every shot on the page has a target diagram\n",
                ok ? "PASS" : "FAIL");
    std::printf("=== %d checks, %d failures ===\n", 1, ok ? 0 : 1);
    rc = ok ? 0 : 1;
    return rc;
}
