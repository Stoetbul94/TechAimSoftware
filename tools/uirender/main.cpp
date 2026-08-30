// Offline UI evidence renderer.
//
// Loads a QML scene file, renders it offscreen and saves a PNG, so a layout
// claim can be SHOWN rather than asserted. Built for the UI pass that fixed
// UI-TRAIN-001 (left-pane programme label) and the header/connection overlap.
//
// Usage: uirender <scene.qml> <out.png> <width> <height>
//
// Scenes live in this directory and pull the application's components in with
// a relative directory import (`import "../.."`), so they render in place —
// nothing has to be copied to the repository root first. See README.md.
#include <QGuiApplication>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlError>
#include <QTimer>
#include <QImage>
#include <QUrl>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QStandardPaths>
#include <QVariantMap>
#include <utility>
#include <cstdio>
#include <QJsonDocument>
#include <QJsonObject>

// Serves a report DTO to a report view, so a report page can be rendered
// offline. The map is NOT written here: it is loaded from a JSON file produced
// by the real controller (finals10m_tests --emit-report), because a rendered
// page is only evidence if what it renders is the product's own report.
//
// This is a proper QObject on purpose. The local Stub below gets away without
// a metaobject because nothing depends on its return values; a report view
// that received `undefined` here would render an empty page and prove nothing.
class ReportStub : public QObject
{
    Q_OBJECT
public:
    QVariantMap data;

    Q_INVOKABLE QVariantMap buildReport(const QVariantMap& meta = QVariantMap()) const
    {
        QVariantMap r = data;
        // The caller's meta fills only what the report did not already carry -
        // the same precedence the real controller applies.
        for (auto it = meta.constBegin(); it != meta.constEnd(); ++it)
            if (!r.contains(it.key()) || r.value(it.key()).toString().isEmpty())
                r.insert(it.key(), it.value());
        return r;
    }
};

// The offscreen platform plugin carries no font database of its own — Qt no
// longer ships fonts, so QFontDatabase::families() comes back EMPTY and every
// glyph renders as an empty box. Register real font files by hand and return
// the family to draw with, or an empty string if none could be found.
static QString bootstrapFont()
{
    // Regular + bold, so font.bold in a scene resolves to a real face rather
    // than a synthesised one. Brand face first, then a plain Latin fallback.
    static const char* const kCandidates[] = {
        "segoeui.ttf", "segoeuib.ttf",      // Segoe UI  — the Tech Aim face
        "arial.ttf",   "arialbd.ttf",       // Arial     — Windows fallback
        "DejaVuSans.ttf", "DejaVuSans-Bold.ttf"
    };

    QString family;
    const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::FontsLocation);
    for (const QString& dirPath : dirs) {
        const QDir dir(dirPath);
        for (const char* const candidate : kCandidates) {
            const QString path = dir.filePath(QString::fromLatin1(candidate));
            if (!QFile::exists(path)) continue;
            const int id = QFontDatabase::addApplicationFont(path);
            if (id < 0) continue;
            const QStringList families = QFontDatabase::applicationFontFamilies(id);
            if (family.isEmpty() && !families.isEmpty()) family = families.first();
        }
        if (!family.isEmpty()) break;
    }
    return family;
}

int main(int argc, char** argv)
{
    // This tool only ever writes a PNG, so it never needs a real display.
    // Left to itself in a headless session the show()/grabWindow() pair below
    // crashes, and QT_QUICK_BACKEND=software applies the desktop's DPI scaling
    // so the saved image is not the requested size. Default to the offscreen
    // platform, which does neither; an explicit setting still wins.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");
    if (qEnvironmentVariableIsEmpty("QT_ENABLE_HIGHDPI_SCALING"))
        qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");

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
        // SettingsPage reads these three directly.
        Q_INVOKABLE double getMotor_movement_time() { return 1.0; }
        Q_INVOKABLE double getMotor_movement_time_sighter() { return 1.0; }
        Q_INVOKABLE bool getAppMode() { return false; }   // false = Demo
        Q_INVOKABLE void saveMotorTimes(double, double) {}
        Q_INVOKABLE void selectLanguage(const QString&) {}
        Q_INVOKABLE void selectMode(int) {}
    };
    Stub appSettings, modReader, login, language, opMode;

    // Text draws as empty boxes unless a real font file is registered first,
    // and the family named here has to be one that actually resolved.
    const QString family = bootstrapFont();
    if (family.isEmpty())
        std::printf("WARN  no usable font found — text will render as empty boxes\n");
    else
        QGuiApplication::setFont(QFont(family));

    QObject themeObj; themeObj.setProperty("fontFamily", family);

    // Enough of PRODUCT / BUILDINFO / LANGUAGE / OPMODE for SettingsPage to
    // lay itself out. Values are placeholders - this renders LAYOUT, not build
    // identity, and nothing here is evidence about the real binary.
    QObject product, buildInfo;
    product.setProperty("fullProductName", "Tech Aim Electronic Target Control");
    product.setProperty("displayName", "Tech Aim");
    product.setProperty("version", "0.9.0");
    product.setProperty("releaseChannel", "RC2");
    product.setProperty("legalPublisher", "SETA Electronic Targets");
    product.setProperty("qtVersion", "6.5.3");
    product.setProperty("architecture", "x86_64");
    product.setProperty("windowsVersion", "Windows 11 Home Single Language 10.0.26200");
    product.setProperty("analyticsVersion", "13");
    product.setProperty("gitCommit", "0000000");
    product.setProperty("softwareVersionLabel", "Tech Aim 0.9.0");
    product.setProperty("isFieldTest", false);
    product.setProperty("fieldTestNotice", "");
    buildInfo.setProperty("config", "Release");
    buildInfo.setProperty("commit", "0000000");
    buildInfo.setProperty("built", "2026-08-11");

    QVariantList langs;
    for (auto pair : { std::make_pair("en", "English"), std::make_pair("de", "Deutsch (beta)") }) {
        QVariantMap m; m["code"] = pair.first; m["label"] = pair.second; langs << m;
    }
    language.setProperty("availableLanguages", langs);
    language.setProperty("languageCode", "en");
    language.setProperty("isBetaTranslation", true);
    language.setProperty("restartRequired", false);
    opMode.setProperty("live", false);
    opMode.setProperty("restartRequired", false);

    QQuickView view;
    view.rootContext()->setContextProperty("APPSETTINGS", &appSettings);
    view.rootContext()->setContextProperty("MODREADER", &modReader);
    view.rootContext()->setContextProperty("loginPage", &login);
    view.rootContext()->setContextProperty("theme", &themeObj);
    view.rootContext()->setContextProperty("PRODUCT", &product);
    view.rootContext()->setContextProperty("BUILDINFO", &buildInfo);
    view.rootContext()->setContextProperty("LANGUAGE", &language);
    view.rootContext()->setContextProperty("OPMODE", &opMode);
    view.rootContext()->setContextProperty("gameRange", 10);

    // A report scene names its DTO through TECHAIM_REPORT_JSON. Absent, the
    // stub serves an empty map and the page renders its own "no data" state -
    // which is honest, and immediately visible in the PNG.
    ReportStub finals10m;
    const QByteArray reportPath = qgetenv("TECHAIM_REPORT_JSON");
    if (!reportPath.isEmpty()) {
        QFile rf(QString::fromLocal8Bit(reportPath));
        if (rf.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(rf.readAll());
            rf.close();
            finals10m.data = doc.object().toVariantMap();
            std::printf("report DTO: %s (%d keys)\n", reportPath.constData(),
                        int(finals10m.data.size()));
        } else {
            std::printf("WARN  could not read %s\n", reportPath.constData());
        }
    }
    view.rootContext()->setContextProperty("FINALS10M", &finals10m);
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
    // A scaled image is still saved, but it is not the evidence that was asked
    // for — say so rather than let the caller assume the size was honoured.
    if (img.width() != w || img.height() != h) {
        std::printf("WARN  requested %dx%d — the platform applied scaling; unset "
                    "QT_SCALE_FACTOR / QT_ENABLE_HIGHDPI_SCALING / QT_QPA_PLATFORM "
                    "to get the exact size\n", w, h);
        return 1;
    }
    return 0;
}

#include "main.moc"
