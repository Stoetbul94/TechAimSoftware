// Offline UI evidence renderer.
//
// Loads a QML scene file, renders it offscreen and saves a PNG, so a layout
// claim can be SHOWN rather than asserted. Built for the UI pass that fixed
// UI-TRAIN-001 (left-pane programme label) and the header/connection overlap.
//
// Usage: uirender <scene.qml> <out.png> <width> <height>
#include <QGuiApplication>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlError>
#include <QTimer>
#include <QImage>
#include <QUrl>
#include <QQmlContext>
#include <QQmlEngine>
#include <cstdio>

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    if (argc < 5) { std::printf("usage: uirender <scene.qml> <out.png> <w> <h>\n"); return 2; }
    const QString scene = QString::fromLocal8Bit(argv[1]);
    const QString out   = QString::fromLocal8Bit(argv[2]);
    const int w = QString::fromLocal8Bit(argv[3]).toInt();
    const int h = QString::fromLocal8Bit(argv[4]).toInt();

    // Minimal stand-ins for the ambient context properties the real
    // application injects. Uppercase names cannot be QML ids, so they have to
    // arrive as context properties exactly as they do in production.
    class Stub : public QObject { public:
        Q_INVOKABLE bool getShowGroupAndMPI() { return true; }
        Q_INVOKABLE double bullet_diameter() { return 4.5; }
        Q_INVOKABLE int getMatch_meter() { return 50; }
    };
    Stub appSettings, modReader, login;
    QObject themeObj; themeObj.setProperty("fontFamily", "Segoe UI");

    QQuickView view;
    view.rootContext()->setContextProperty("APPSETTINGS", &appSettings);
    view.rootContext()->setContextProperty("MODREADER", &modReader);
    view.rootContext()->setContextProperty("loginPage", &login);
    view.rootContext()->setContextProperty("theme", &themeObj);
    view.setSource(QUrl::fromLocalFile(scene));
    if (view.status() != QQuickView::Ready) {
        for (const QQmlError& e : view.errors())
            std::printf("QML ERROR  %s\n", qPrintable(e.toString()));
        return 2;
    }
    view.resize(w, h);
    view.show();
    QTimer::singleShot(1500, &app, &QGuiApplication::quit);
    app.exec();

    const QImage img = view.grabWindow();
    if (img.isNull() || !img.save(out)) { std::printf("FAIL  could not save %s\n", qPrintable(out)); return 1; }
    std::printf("saved %s (%dx%d)\n", qPrintable(out), img.width(), img.height());
    return 0;
}
