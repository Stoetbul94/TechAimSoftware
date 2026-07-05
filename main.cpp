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
#include <QLockFile>
#include <QDir>
#include <QMessageBox>
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
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("The application is already running.\n"
                       "Allowed to run only one instance of the application.");
        msgBox.exec();
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
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
    //    return -1;
}
