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
#include "src/finals/FinalsAudioService.h"
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
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
    //    return -1;
}
