#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include "customprint.h"
#include <QTranslator>

//mode reader modification
#include <stdio.h>
#include <stdlib.h>
#include <QDir>
#include <QTranslator>
#include <QScreen>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

#include "ModReader/3rdparty/QsLog/QsLog.h"
#include "ModReader/3rdparty/QsLog/QsLogDest.h"
#include "ModReader/src/mainwindow.h"
#include "ModReader/src/modbusadapter.h"
#include "ModReader/src/modbuscommsettings.h"
#include "ModReader/forms/tachuswidget.h"

#include "defines.h"
#include "appsettings.h"
#include "receiverTachus.h"
#include "src/bridge/coachreportbridge.h"
#include "src/bridge/coachreportfeeder.h"
#include "src/bridge/pdfexporter.h"
#include "src/finals/Finals3PController.h"
#include "src/finals10m/Finals10mController.h"
#include "src/qualification/QualificationController.h"
#include "src/incident/EstIncidentController.h"
#include "src/finals/FinalsAudioService.h"
#include "src/reliability/storage/StoragePaths.h"
#include "logfile.h"
#include <QLockFile>
#include <QDir>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QQuickWindow>
#include <QSGRendererInterface>

QTranslator *Translator;

// Session Reliability Layer (M0): storage-failure dialog. Fires BEFORE the
// QML engine exists, so it cannot use dialogManager — same frameless
// TechAim-styled widget pattern as the single-instance dialog.
// Returns true if the operator chose Retry, false for Exit.
static bool showStorageFailureDialog(const ta::rel::StorageResult& r)
{
    QDialog box(nullptr, Qt::FramelessWindowHint | Qt::Dialog);
    box.setAttribute(Qt::WA_TranslucentBackground);
    QVBoxLayout* outer = new QVBoxLayout(&box);
    outer->setContentsMargins(0, 0, 0, 0);
    QFrame* card = new QFrame(&box);
    card->setObjectName("card");
    card->setStyleSheet(
        "#card { background-color: #1f2026; border: 1px solid #3a3b42;"
        " border-radius: 13px; }");
    outer->addWidget(card);
    QVBoxLayout* lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 20, 24, 18);
    lay->setSpacing(10);
    QLabel* title = new QLabel("Session Storage Unavailable", card);
    title->setStyleSheet("color: #f2f3f5; font-family: 'Segoe UI';"
                         " font-size: 16px; font-weight: bold;"
                         " background: transparent; border: none;");
    QLabel* body = new QLabel(
        r.operatorMessage
        + "\n\nPath: " + (r.affectedPath.isEmpty()
                          ? ta::rel::StoragePaths::applicationDataRoot()
                          : r.affectedPath)
        + "\n\nWithout working session storage, match data cannot be saved.",
        card);
    body->setStyleSheet("color: #b6b9c0; font-family: 'Segoe UI';"
                        " font-size: 12px; background: transparent; border: none;");
    body->setWordWrap(true);
    QPushButton* retry = new QPushButton("Retry", card);
    retry->setCursor(Qt::PointingHandCursor);
    retry->setDefault(true);
    retry->setStyleSheet(
        "QPushButton { background-color: #a80038; color: white;"
        " font-family: 'Segoe UI'; font-size: 12px; font-weight: bold;"
        " border: none; border-radius: 8px; padding: 7px 24px; }"
        "QPushButton:pressed { background-color: #8a002f; }");
    QPushButton* exitBtn = new QPushButton("Exit", card);
    exitBtn->setCursor(Qt::PointingHandCursor);
    exitBtn->setStyleSheet(
        "QPushButton { background-color: #26272c; color: #d7d8dd;"
        " font-family: 'Segoe UI'; font-size: 12px;"
        " border: 1px solid #3a3b42; border-radius: 8px; padding: 7px 24px; }"
        "QPushButton:pressed { background-color: #2a2b30; }");
    QObject::connect(retry,   &QPushButton::clicked, &box, &QDialog::accept);
    QObject::connect(exitBtn, &QPushButton::clicked, &box, &QDialog::reject);
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    btnRow->addWidget(exitBtn);
    btnRow->addWidget(retry);
    lay->addWidget(title);
    lay->addWidget(body);
    lay->addLayout(btnRow);
    return box.exec() == QDialog::Accepted;
}

int main(int argc, char *argv[])
{
    // Qt::AA_EnableHighDpiScaling is now the default in Qt6 — removed
    /////////////////////////////// opengl

    // OPEN_GL block: software-renderer mode for embedded target hardware without GPU.
    // On development machines with a real GPU, this block should be disabled —
    // uncomment to force software rendering on the target device:
//#ifdef OPEN_GL
//    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
//    qputenv("QSG_RENDER_LOOP", "basic");
//#endif

    ///////////////////////////////
    QApplication app(argc, argv);
    // Session Reliability Layer (M0): organization/application identity is
    // what QStandardPaths::AppLocalDataLocation resolves against. Every
    // existing QSettings call passes explicit org strings or filenames, so
    // nothing else shifts.
    QCoreApplication::setOrganizationName(QStringLiteral("TechAim"));
    QCoreApplication::setApplicationName(QStringLiteral("TechAim"));

    // F9B: build identity embedded at COMPILE time (Seta.pro DEFINES + the
    // compiler's __DATE__/__TIME__). The app never runs git; the customer
    // machine needs no Git / repo / Qt Creator. Exposed to QML as BUILDINFO
    // (shown in Settings ▸ About) and logged once at startup so the operator
    // can confirm the release executable matches the committed source.
#ifndef APP_VERSION_STR
#define APP_VERSION_STR "0.0.0"
#endif
#ifndef APP_GIT_SHA
#define APP_GIT_SHA "unknown"
#endif
#ifndef APP_BUILD_CONFIG
#define APP_BUILD_CONFIG "Unknown"
#endif
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION_STR));
    const QString kBuildTimestamp = QStringLiteral(__DATE__ " " __TIME__);
    qInfo().noquote() << "TechAim" << APP_VERSION_STR
                      << APP_BUILD_CONFIG << "build · commit" << APP_GIT_SHA
                      << "· built" << kBuildTimestamp;

    ///////////////////////////////////////////////////////////
    /// single instance app
    ///////////////////////////////////////////////////////////
    QLockFile lockFile(QDir::temp().absoluteFilePath("tachus_seta.lock"));

    /* Trying to close the Lock File, if the attempt is unsuccessful for 100 milliseconds,
         * then there is a Lock File already created by another process.
         / Therefore, we throw a warning and close the program
         * */
    if(!lockFile.tryLock(100)){
        // TechAim dialog framework (C5): this fires BEFORE the QML engine
        // exists, so it cannot use dialogManager — a small frameless
        // TechAim-styled widget dialog replaces the native QMessageBox.
        QDialog box(nullptr, Qt::FramelessWindowHint | Qt::Dialog);
        box.setAttribute(Qt::WA_TranslucentBackground);
        QVBoxLayout* outer = new QVBoxLayout(&box);
        outer->setContentsMargins(0, 0, 0, 0);
        QFrame* card = new QFrame(&box);
        card->setObjectName("card");
        card->setStyleSheet(
            "#card { background-color: #1f2026; border: 1px solid #3a3b42;"
            " border-radius: 13px; }");
        outer->addWidget(card);
        QVBoxLayout* lay = new QVBoxLayout(card);
        lay->setContentsMargins(24, 20, 24, 18);
        lay->setSpacing(10);
        QLabel* title = new QLabel("Already Running", card);
        title->setStyleSheet("color: #f2f3f5; font-family: 'Segoe UI';"
                             " font-size: 16px; font-weight: bold;"
                             " background: transparent; border: none;");
        QLabel* body = new QLabel("Another instance of TechAim is already open.\n"
                                  "Only one instance can run at a time.", card);
        body->setStyleSheet("color: #b6b9c0; font-family: 'Segoe UI';"
                            " font-size: 12px; background: transparent; border: none;");
        QPushButton* ok = new QPushButton("Close", card);
        ok->setCursor(Qt::PointingHandCursor);
        ok->setDefault(true);
        ok->setStyleSheet(
            "QPushButton { background-color: #a80038; color: white;"
            " font-family: 'Segoe UI'; font-size: 12px; font-weight: bold;"
            " border: none; border-radius: 8px; padding: 7px 24px; }"
            "QPushButton:pressed { background-color: #8a002f; }");
        QObject::connect(ok, &QPushButton::clicked, &box, &QDialog::accept);
        QHBoxLayout* btnRow = new QHBoxLayout();
        btnRow->addStretch(1);
        btnRow->addWidget(ok);
        lay->addWidget(title);
        lay->addWidget(body);
        lay->addLayout(btnRow);
        box.exec();
        return 1;
    }
    ///////////////////////////////////////////////////////////


    //// translations
    QTranslator translator;
    //qrc:/images/leftPanel/pistol_box_copy.png
    //    translator.load(QLocale(), "Test", QString(), ":/Translations/Translations");

    QFile file("://translations/french.qm");
    if (!file.open(QIODevice::ReadOnly))
        qDebug() << "Can't find it!";

    QString curDir = QDir::currentPath();
    //    curDir.append("/french.qm");
    //    bool isTrlsFileLoaded = translator.load("C://Work/tachus/Merging_app_modReader/translations/german.qm");
    bool isTrlsFileLoaded = translator.load("german.qm");

    if(!isTrlsFileLoaded) {
        qDebug() << "FILE NOT LOADED " << translator.isEmpty();
    }
    else {
        qDebug() << "FILE LOADED";
        qApp->installTranslator(&translator);
    }
    ////

    // ── Session Reliability Layer (M0): storage initialization ──────────
    // Resolve the AppData root, create the directory tree, probe that
    // session storage is durably writable. Never silent: failure blocks
    // with Retry/Exit (the full degraded-persistence model is M2).
    {
        ta::rel::StorageResult storage = ta::rel::StoragePaths::initialize();
        while (!storage.ok) {
            LogFile::instance().appendToLogFile(
                QString("M0 storage init FAILED: %1 [%2]")
                    .arg(storage.technicalDetail, storage.affectedPath),
                LogType::interfaceLevel);
            if (!showStorageFailureDialog(storage))
                return 1;                       // operator chose Exit
            storage = ta::rel::StoragePaths::initialize();
        }
        LogFile::instance().appendToLogFile(
            QString("M0 storage ready: root=%1 (%2)")
                .arg(ta::rel::StoragePaths::applicationDataRoot(),
                     storage.technicalDetail),
            LogType::interfaceLevel);

        // Preservation-only legacy migration: journals written by pre-M0
        // builds into the process CWD move to Sessions/Archive/Legacy.
        const ta::rel::StorageResult legacy =
            ta::rel::StoragePaths::migrateLegacyJournals(QDir::currentPath());
        LogFile::instance().appendToLogFile(
            QString("M0 legacy journal scan: %1%2")
                .arg(legacy.ok ? QString() : QStringLiteral("FAILED - "),
                     legacy.technicalDetail),
            LogType::interfaceLevel);
    }

    AppSettings *appsettings = new AppSettings("config.ini");
    QScreen *srn = QApplication::screens().at(0);
    qreal dotsPerInch = (qreal)srn->logicalDotsPerInch();

    qDebug() <<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << dotsPerInch;
    ///-------------------------------------------------
    //Modbus Adapter
    ModbusAdapter modbus_adapt(NULL);
    //Program settings
    QString filePath = QString("%1/qModMaster.ini").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    ModbusCommSettings settings(filePath);

    //show main window
    mainWin = new MainWindow(NULL, &modbus_adapt, &settings);
    //connect signals - slots
    QObject::connect(&modbus_adapt, SIGNAL(refreshView()), mainWin, SLOT(refreshView()));
    QObject::connect(mainWin, SIGNAL(resetCounters()), &modbus_adapt, SLOT(resetCounters()));
    //mainWin->show();

    TachusWidget* widget = new TachusWidget(mainWin);
    appsettings->setTachusWidget(widget);
    //widget->show();
    ///-----------------------------------------------------------
    ReceiverTachus receiver;
    receiver.setTachus(widget);
    QQmlApplicationEngine engine;
    //For QML
    CustomPrint  printComponent(widget);
    //    printComponent.printTest();
    engine.rootContext()->setContextProperty("CUSTOMPRINT", &printComponent);
    engine.rootContext()->setContextProperty("MODREADER", widget);
    engine.rootContext()->setContextProperty("APPSETTINGS", appsettings);
    // Coach-report QML bridge (offline analytics engine <-> QML). No analytics
    // logic here; it only marshals CoachReportData to/from QVariant.
    CoachReportBridge coachReport;
    engine.rootContext()->setContextProperty("COACHREPORT", &coachReport);
    // Real-data feeder: reads the TachusWidget game-mode match history and runs
    // the report through the bridge. QML calls COACHFEED.analyzeCurrentMatch(...).
    CoachReportFeeder coachFeed(widget, &coachReport);
    engine.rootContext()->setContextProperty("COACHFEED", &coachFeed);
    // A4 PDF export of the Coach Report Print view (grabbed sections -> QPdfWriter).
    PdfExporter pdfExport;
    engine.rootContext()->setContextProperty("PDFEXPORT", &pdfExport);
    // 3P FINAL — dedicated finals state machine / time authority (fully
    // separate from qualification). QML binds to it; it owns all finals timing.
    Finals3PController finalsController;
    engine.rootContext()->setContextProperty("FINALS3P", &finalsController);
    // D4: deterministic audio — one WAV clip per command cue from
    // <appDir>/audio/finals/<cueId>.wav, system-beep fallback per missing
    // clip. The controller has no audio dependency; the service listens.
    FinalsAudioService finalsAudio;
    QObject::connect(&finalsController, &Finals3PController::commandIssued,
                     &finalsAudio, &FinalsAudioService::onCommandIssued);
    engine.rootContext()->setContextProperty("FINALSAUDIO", &finalsAudio);
    // 10m Air Rifle / Air Pistol FINAL (F1/F2) — single-athlete training course.
    // Separate controller from the 3P Final; shares only the reliability layer.
    // Discipline chosen at start via FINALS10M.configureDiscipline("FINAL_AR10"
    // | "FINAL_AP10"). The audio service is reused (beep fallback until the
    // official 10m command cues exist). See docs/10m-finals-architecture.md.
    Finals10mController finals10mController;
    engine.rootContext()->setContextProperty("FINALS10M", &finals10mController);
    // F9B: read-only build identity for Settings ▸ About (embedded at compile
    // time; no runtime git). A plain QVariantMap context property.
    QVariantMap buildInfo;
    buildInfo[QStringLiteral("version")] = QStringLiteral(APP_VERSION_STR);
    buildInfo[QStringLiteral("config")]  = QStringLiteral(APP_BUILD_CONFIG);
    buildInfo[QStringLiteral("commit")]  = QStringLiteral(APP_GIT_SHA);
    buildInfo[QStringLiteral("built")]   = kBuildTimestamp;
    engine.rootContext()->setContextProperty("BUILDINFO", buildInfo);
    QObject::connect(&finals10mController, &Finals10mController::commandIssued,
                     &finalsAudio, &FinalsAudioService::onCommandIssued);
    // Phase B — shared qualification persistence seam (QUAL). Idle until a
    // qualification discipline drives it (wired per-discipline in B1–B3); like
    // FINALS3P it owns a reliability SessionStore for its match record.
    QualificationController qualController;
    engine.rootContext()->setContextProperty("QUAL", &qualController);
    // Phase E — EST incident workflow service (INCIDENTS). Discipline-agnostic:
    // it submits typed incident/Jury events through whichever session store is
    // ACTIVE (qualification or finals); the reducer record is authoritative.
    EstIncidentController incidentController;
    incidentController.setStoreProvider(
        [&qualController, &finalsController, &finals10mController]() -> ta::rel::SessionStore* {
            if (qualController.store() && qualController.store()->active())
                return qualController.store();
            if (finalsController.store() && finalsController.store()->active())
                return finalsController.store();
            if (finals10mController.store() && finals10mController.store()->active())
                return finals10mController.store();
            return nullptr;
        });
    engine.rootContext()->setContextProperty("INCIDENTS", &incidentController);
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
    //    return -1;
}
