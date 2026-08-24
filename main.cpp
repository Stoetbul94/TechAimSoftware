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
#include "src/mode/OperatingMode.h"
#include "src/mode/OperatingModeService.h"
#include "src/training/TrainingProgramController.h"
#include "src/training/CallDiagnoseController.h"
#include "src/training/PositionTransitionController.h"
#include "src/training/WindMapController.h"
#include "src/reliability/storage/StoragePaths.h"
#include "src/platform/PlatformService.h"
#include "src/platform/PlatformBridge.h"
#include "src/app/ProductIdentity.h"
#include "src/app/ProductIdentityBridge.h"
#include "src/rms/RmsProtocol.h"
#include "src/telemetry/NodeIdentity.h"
#include "src/telemetry/NodeTelemetryService.h"
#include "src/telemetry/UdpTelemetrySink.h"
#include "src/app/LanguageService.h"
#include "src/app/DocumentationCapture.h"
#include "logfile.h"
#include <memory>
#include <QFileInfo>
#include <QLockFile>
#include <QProcess>
#include <QDir>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSysInfo>

QTranslator *Translator;

// Session Reliability Layer (M0): storage-failure dialog. Fires BEFORE the
// QML engine exists, so it cannot use dialogManager — same frameless
// TechAim-styled widget pattern as the single-instance dialog.
// Returns true if the operator chose Retry, false for Exit.
static bool showStorageFailureDialog(const ta::rel::StorageResult& r)
{
    // A1/A2 platform seam. The desktop surface below is a frameless,
    // translucent, stylesheet-driven QDialog shown BEFORE the QML engine
    // exists. That is not a reliable startup surface on Android, and there is
    // no operator sitting at a keyboard to press Retry on a tablet that has
    // just failed to create its own private data directory.
    //
    // On Android the failure is reported to logcat and treated as fatal.
    // This is not a downgrade in safety: app-private storage under
    // AppLocalDataLocation is created by the system for the package, so the
    // desktop failure modes this dialog exists for (a disconnected network
    // share, a read-only install directory, a roaming-profile permission
    // problem) do not arise. If it fails anyway, the device is in a state the
    // application cannot repair by retrying, and continuing without session
    // storage would be far worse than refusing to start.
    //
    // Returning false means "operator chose Exit" to the caller.
    if (!ta::platform::supportsDesktopStartupDialogs()) {
        qCritical().noquote()
            << "FATAL: session storage unavailable —" << r.operatorMessage
            << "| detail:" << r.technicalDetail
            << "| path:" << (r.affectedPath.isEmpty()
                                 ? ta::rel::StoragePaths::applicationDataRoot()
                                 : r.affectedPath);
        return false;
    }

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
    // P0 Phase B: these now derive from the ONE product-identity source
    // (src/app/ProductIdentity.*) instead of being spelled out here. The
    // resolved values are unchanged from M0 ("TechAim"/"TechAim"), so the
    // AppLocalDataLocation root and every existing journal path stay exactly
    // where they are — the rename must not move anyone's session data.
    const ta::app::ProductIdentity& product = ta::app::identity();
    QCoreApplication::setOrganizationName(product.organisationName);
    QCoreApplication::setApplicationName(product.organisationName);
    QCoreApplication::setOrganizationDomain(product.organisationDomain);
    QGuiApplication::setApplicationDisplayName(product.fullProductName);

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
    qInfo().noquote() << product.displayName << APP_VERSION_STR
                      << APP_BUILD_CONFIG << "build · commit" << APP_GIT_SHA
                      << "· built" << kBuildTimestamp
                      << "·" << product.releaseChannel
                      << "· flavour" << ta::app::flavourName(ta::app::currentFlavour());

    ///////////////////////////////////////////////////////////
    /// single instance app
    ///////////////////////////////////////////////////////////
    // P0 Phase E — single-instance migration. The identifier moves to the
    // Tech Aim name, but this build ALSO holds the legacy lock: during the
    // migration release a renamed TechAim.exe and an old Seta.exe must never
    // run concurrently against the same config.ini and session store. Taking
    // both locks makes either one block the other in both directions.
    QLockFile lockFile(QDir::temp().absoluteFilePath(
        product.executableBaseName + QStringLiteral(".lock")));
    std::unique_ptr<QLockFile> legacyLock;
    bool blockedByLegacy = false;
    bool singleInstanceBlocked = false;

    // A1/A2 platform seam. On Android the whole single-instance mechanism is
    // meaningless: the system guarantees one task instance per package, there
    // is no second process to collide with, and there is no legacy Seta.exe
    // to migrate away from. Acquiring a lock file in the temp directory would
    // defend against nothing while adding a startup failure mode.
    //
    // The lockFile object is still CONSTRUCTED on every platform because the
    // operating-mode restart handler below captures it; it simply is not
    // acquired here on Android. Windows behaviour is unchanged.
    if (ta::platform::supportsSingleInstanceLock()) {
        for (const QString& legacyName : product.legacyLockFileNames) {
            auto lk = std::make_unique<QLockFile>(QDir::temp().absoluteFilePath(legacyName));
            if (!lk->tryLock(100)) { blockedByLegacy = true; break; }
            legacyLock = std::move(lk);   // held for the process lifetime
        }

        /* Trying to close the Lock File, if the attempt is unsuccessful for 100 milliseconds,
             * then there is a Lock File already created by another process.
             / Therefore, we throw a warning and close the program
             * */
        singleInstanceBlocked = !lockFile.tryLock(100) || blockedByLegacy;
    }

    if (singleInstanceBlocked) {
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
        QLabel* body = new QLabel(
            QCoreApplication::translate("Startup", "%1 is already running.\n"
                                        "Only one instance can run at a time.")
                .arg(product.displayName), card);
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


    // P0 Phase F — UI language. Replaces the Tachus-era block that probed for
    // a french.qm and then loaded "german.qm" from the process CWD (it never
    // resolved in a deployed install). Catalogues now ship in the binary and
    // the choice persists in config.ini. Installed BEFORE the QML engine
    // loads so the first frame is already in the selected language.
    // The service is created after AppSettings below (it needs the resolved
    // config path); see languageService.

    // ── J.1A: isolated documentation-capture profile ────────────────────
    // Developer/documentation facility only; there is no Settings UI for it.
    // BOTH --documentation-capture and --data-root <absolute> are required.
    // Absent either, nothing below runs and the production root is used
    // exactly as before. On any validation failure the application EXITS —
    // it never silently falls back to production.
    bool documentationCaptureActive = false;
    {
        const ta::app::CaptureRequest req =
            ta::app::parseCaptureArguments(QCoreApplication::arguments());
        if (req.requested) {
            const QString installDir =
                QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
            const ta::app::CaptureResult cap = ta::app::prepareCaptureRoot(
                req.dataRoot,
                ta::rel::StoragePaths::productionDataRoot(),
                installDir,
                QStringLiteral(APP_GIT_SHA),
                QStringLiteral(APP_GIT_SHA));
            if (!cap.ok) {
                qCritical().noquote()
                    << "DOCUMENTATION CAPTURE REFUSED:" << cap.operatorMessage
                    << "|" << cap.technicalDetail;
                return 2;                      // never fall back to production
            }
            ta::rel::StoragePaths::setRootOverrideForTesting(cap.resolvedRoot);
            documentationCaptureActive = true;
            qInfo().noquote() << "DOCUMENTATION CAPTURE PROFILE ACTIVE - isolated data root:"
                              << cap.resolvedRoot;
        } else if (!req.dataRoot.isEmpty()) {
            qCritical().noquote() << "DOCUMENTATION CAPTURE REFUSED: --data-root requires "
                                     "--documentation-capture";
            return 2;
        }
    }

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

    // ── A1/A2: platform-safe configuration path ────────────────────────
    // Historically this was the literal relative name "config.ini", which
    // QSettings resolves against the WORKING DIRECTORY. On Windows that is the
    // install directory holding the operator's config.ini, and that resolution
    // is preserved exactly — configFilePath() returns the name unchanged there,
    // so no deployed install moves and no operator has to find a new file.
    //
    // On Android the working directory is "/" and is not writable, and neither
    // is the application binary directory. The path resolves instead into
    // app-private storage via the existing StoragePaths settings directory.
    const QString configFileName = QStringLiteral("config.ini");
    const QString configPath = ta::platform::configFilePath(configFileName);

    // Seed a first-run config ONLY on a platform whose installer ships none.
    // On Windows a missing config.ini is meaningful (AppSettings falls back to
    // documented in-code defaults) and must never be manufactured.
    if (!ta::platform::shipsConfigFileWithInstall()) {
        if (!ta::platform::ensureConfigSeeded(configPath)) {
            qWarning().noquote()
                << "Could not seed first-run configuration at" << configPath
                << "— continuing with in-code defaults.";
        }
    }

    AppSettings *appsettings = new AppSettings(configPath);

    // Language is persisted alongside app_mode. Applied here, before the QML
    // engine is created. Language selection deliberately touches translations
    // ONLY — never the brand, theme, executable identity or app_mode.
    LanguageService* languageService =
        new LanguageService(appsettings->getConfigFilePath(), appsettings);
    languageService->applyPersistedLanguage();

    // F10: operating-mode authority (Live target vs Demo simulation). Parsed
    // case-consistently from config.ini; an invalid/missing value falls back to
    // Live (matching the product's existing default) WITHOUT enabling Demo
    // input, and logs the fallback. Kept strictly separate from build identity
    // (which source commit) and session type (which discipline).
    const ta::mode::ParsedMode parsedMode =
        ta::mode::parseMode(appsettings->getRawAppModeToken());
    OperatingModeService* opMode =
        new OperatingModeService(appsettings->getConfigFilePath(),
                                 parsedMode.mode, parsedMode.valid,
                                 parsedMode.raw, appsettings);
    if (!parsedMode.valid) {
        qWarning().noquote() << "Operating mode: invalid/missing app_mode value"
                             << (parsedMode.raw.isEmpty()
                                     ? QStringLiteral("(absent)")
                                     : QStringLiteral("'%1'").arg(parsedMode.raw))
                             << "— falling back to Live target (Demo input NOT enabled).";
    }
    qInfo().noquote() << "Operating mode:" << opMode->runningModeToken()
                      << (opMode->isLive() ? "(physical target input)"
                                           : "(simulated input)");

    // J.1A: the capture profile is Demo-ONLY. Refusing Live here means a
    // capture profile can never record physical-target input, and the
    // existing source gate continues to reject physical shots in Demo.
    if (documentationCaptureActive && opMode->isLive()) {
        qCritical().noquote()
            << "DOCUMENTATION CAPTURE REFUSED: the capture profile requires Demo mode,"
            << "but the effective operating mode is Live. Set app_mode=Demo in the"
            << "capture profile's config.ini.";
        return 2;
    }

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
    // A1/A2 §41 — runtime platform diagnostics. Early tablet testing needs to
    // see WHERE the app actually put its data, not where we believe it did;
    // an Android app-private path is not reachable through a file manager, so
    // if it is not on screen it cannot be checked at all. Read-only strings.
    buildInfo[QStringLiteral("platformShell")] =
        ta::platform::shellName(ta::platform::currentShell());
    buildInfo[QStringLiteral("qtVersion")]   = QString::fromLatin1(qVersion());
    buildInfo[QStringLiteral("abi")]         = QSysInfo::buildCpuArchitecture();
    buildInfo[QStringLiteral("osVersion")]   = QSysInfo::prettyProductName();
    buildInfo[QStringLiteral("appDataRoot")] = ta::rel::StoragePaths::applicationDataRoot();
    buildInfo[QStringLiteral("settingsPath")] = configPath;
    buildInfo[QStringLiteral("sessionsPath")] = ta::rel::StoragePaths::currentSessionsDirectory();
    buildInfo[QStringLiteral("exportsPath")]  = ta::rel::StoragePaths::exportsDirectory();
    buildInfo[QStringLiteral("logsPath")]     = ta::rel::StoragePaths::logsDirectory();
    engine.rootContext()->setContextProperty("BUILDINFO", buildInfo);

    // Same facts to the log, so a logcat capture from a tablet in the field is
    // self-describing without anyone having to navigate to a Settings screen.
    qInfo().noquote() << "Platform shell:"
                      << ta::platform::shellName(ta::platform::currentShell())
                      << "| Qt" << qVersion()
                      << "| ABI" << QSysInfo::buildCpuArchitecture();
    qInfo().noquote() << "App data root :" << ta::rel::StoragePaths::applicationDataRoot();
    qInfo().noquote() << "Settings file :" << configPath;
    qInfo().noquote() << "Sessions dir  :" << ta::rel::StoragePaths::currentSessionsDirectory();
    // P0 Phase B: product identity for QML. Read-only build-time facts —
    // QML must take product names from here instead of hardcoding them.
    ProductIdentityBridge* productBridge = new ProductIdentityBridge(&app);
    engine.rootContext()->setContextProperty("PRODUCT", productBridge);
    // A1/A2: platform facts + font tokens for QML. Registered on the ROOT
    // CONTEXT so it is in scope in every QML file this engine loads, including
    // report views and floating windows that sit outside the `theme` ancestor
    // chain (see src/platform/PlatformBridge.h for why that matters).
    PlatformBridge* platformBridge = new PlatformBridge(&app);
    engine.rootContext()->setContextProperty("PLATFORM", platformBridge);
    // P0 Phase F: the engine lets a live language switch re-evaluate every
    // qsTr() binding, so most screens change without a restart.
    languageService->setQmlEngine(&engine);
    engine.rootContext()->setContextProperty("LANGUAGE", languageService);
    // F10: operating-mode authority for QML (badge, Settings selector, gate).
    engine.rootContext()->setContextProperty("OPMODE", opMode);
    // Push the running mode into the finals controllers (declared above) so the
    // authoritative input-source gate rejects a wrong-source shot BEFORE it is
    // durably accepted. The qualification controller is set below, right after
    // it is constructed.
    const int runningModeInt = static_cast<int>(opMode->runningMode());
    finalsController.setOperatingMode(runningModeInt);
    finals10mController.setOperatingMode(runningModeInt);
    // Restart-based switch: the service only writes config + asks; main performs
    // the detached relaunch. Release the single-instance lock FIRST so the new
    // instance can immediately acquire it (avoids the "Already Running" race),
    // then start a detached copy and quit cleanly. applyModeChange() has already
    // ensured no session is active before this can be requested.
    QObject::connect(opMode, &OperatingModeService::restartRequested,
                     qApp, [&lockFile]() {
        // A1/A2 platform seam. Android has no supported way for an app to
        // relaunch itself: there is no equivalent of startDetached on our own
        // package, and the Java tricks that approximate it are brittle and
        // version-dependent. Deliberately NOT emulated (see
        // docs/architecture/android-product-architecture.md §5).
        //
        // The new mode is already written to config.ini by applyModeChange(),
        // and OPMODE.restartRequired stays true, so the Settings screen keeps
        // telling the operator a restart is needed. They close and reopen the
        // app; the next launch reads the new mode. Nothing is lost.
        if (!ta::platform::supportsSelfRelaunch()) {
            LogFile::instance().appendToLogFile(
                QStringLiteral("Operating-mode change staged; self-relaunch is not "
                               "available on this platform — operator must restart "
                               "the application manually."),
                LogType::interfaceLevel);
            qInfo().noquote()
                << "Operating mode changed. Close and reopen Tech Aim to apply it.";
            return;
        }

        const QString exe = QCoreApplication::applicationFilePath();
        const QString cwd = QDir::currentPath();
        lockFile.unlock();
        LogFile::instance().appendToLogFile(
            QStringLiteral("Operating-mode restart: relaunching %1").arg(exe),
            LogType::interfaceLevel);
        QProcess::startDetached(exe, QStringList(), cwd);
        QCoreApplication::quit();
    });
    QObject::connect(&finals10mController, &Finals10mController::commandIssued,
                     &finalsAudio, &FinalsAudioService::onCommandIssued);
    // Phase B — shared qualification persistence seam (QUAL). Idle until a
    // qualification discipline drives it (wired per-discipline in B1–B3); like
    // FINALS3P it owns a reliability SessionStore for its match record.
    QualificationController qualController;
    qualController.setOperatingMode(runningModeInt);   // F10 input-source gate
    engine.rootContext()->setContextProperty("QUAL", &qualController);
    // Training Lab (T1) — Technical Blocks domain controller. Separate from
    // every competition controller; owns ALL Training state and journals
    // Training-specific events (sessionKind=Training). Same F10 source gate.
    TrainingProgramController trainingController;
    trainingController.setOperatingMode(runningModeInt);
    engine.rootContext()->setContextProperty("TRAINING", &trainingController);
    // Call & Diagnose (T2) — second Training Lab programme. Own controller +
    // own SessionStore; sessionKind=Training, programId=call_and_diagnose.
    CallDiagnoseController callDiagnoseController;
    callDiagnoseController.setOperatingMode(runningModeInt);
    engine.rootContext()->setContextProperty("CALLDIAG", &callDiagnoseController);
    // Position Transition (T4) — third Training Lab programme (50m 3P only).
    PositionTransitionController positionTransitionController;
    positionTransitionController.setOperatingMode(runningModeInt);
    engine.rootContext()->setContextProperty("POSTRANS", &positionTransitionController);
    // Wind Map (Training Lab Release 2) — fourth Training Lab programme.
    // 50m Rifle Prone and 50m Rifle 3 Positions ONLY; sessionKind=Training,
    // programId=wind_map. A post-session review programme: it records the
    // conditions the athlete observed alongside the shots they fired and never
    // advises, corrects or scores.
    WindMapController windMapController;
    windMapController.setOperatingMode(runningModeInt);
    engine.rootContext()->setContextProperty("WINDMAP", &windMapController);
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

    // ── A1/A2: application lifecycle → durability pump ──────────────────
    // Android may pause, background or kill this process without warning and
    // without any equivalent of closeEvent. Windows effectively never does.
    //
    // This introduces NO new recovery system. Official events are already
    // written with DurabilityClass::Sync and are durable at write time; the
    // only state that can be pending is the retry queue, which holds events
    // accepted while a write was failing. Draining it is exactly what
    // SessionStore::pumpRetryQueue() exists for — production already drives it
    // from a backoff timer — so backgrounding simply asks for that same drain
    // immediately instead of waiting for the next tick.
    //
    // Idempotent by construction: pumpRetryQueue() returns true when the queue
    // is or becomes empty, so repeated lifecycle notifications (Android emits
    // Inactive then Hidden then Suspended for a single Home press) cost
    // nothing and cannot corrupt state.
    //
    // Every discipline that owns a store is pumped, not just the "current"
    // one — a Training session and a qualification session are separate stores
    // and either could be the live one.
    QObject::connect(&app, &QGuiApplication::applicationStateChanged, qApp,
                     [&qualController, &finalsController, &finals10mController,
                      &trainingController, &callDiagnoseController,
                      &positionTransitionController](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive)
            return;                       // coming back to the foreground

        ta::rel::SessionStore* const stores[] = {
            qualController.store(),        finalsController.store(),
            finals10mController.store(),   trainingController.store(),
            callDiagnoseController.store(), positionTransitionController.store()
        };
        int pumped = 0;
        for (ta::rel::SessionStore* s : stores) {
            if (s && s->active()) { s->pumpRetryQueue(); ++pumped; }
        }
        if (pumped > 0) {
            LogFile::instance().appendToLogFile(
                QStringLiteral("Lifecycle: state %1 — drained retry queue for %2 active store(s)")
                    .arg(static_cast<int>(state)).arg(pumped),
                LogType::interfaceLevel);
        }
    });

    // ── RMS node telemetry ───────────────────────────────────────────────
    // This station describes itself to a Range Management System. It is
    // OBSERVATION ONLY: the sink is a write-only UDP socket, there is no
    // inbound path, and nothing RMS does can reach the match. If no RMS is
    // listening, every datagram simply goes nowhere and this application
    // behaves exactly as it did before.
    //
    // It subscribes to the SAME session stores the incident service consults,
    // at SessionStore::eventApplied — downstream of acceptance, so it can only
    // ever describe shots the node has already made authoritative.
    ta::telemetry::UdpTelemetrySink telemetrySink;
    ta::telemetry::NodeTelemetryService nodeTelemetry(
        ta::telemetry::NodeIdentity::forApplication(), &telemetrySink);
    nodeTelemetry.setAppVersion(QStringLiteral(APP_VERSION_STR));
    nodeTelemetry.setProductIdentity(product.displayName);
    nodeTelemetry.attachStore(qualController.store());
    nodeTelemetry.attachStore(finalsController.store());
    nodeTelemetry.attachStore(finals10mController.store());
    // The target link state comes from the existing target-status signal, not
    // from datagram arrival: a node can be perfectly reachable with its target
    // unplugged, and a range display must not show those as the same thing.
    QObject::connect(widget, &TachusWidget::targetStatusChanged, &nodeTelemetry,
                     [widget, &nodeTelemetry]() {
                         nodeTelemetry.setTargetConnected(widget->targetReady());
                         nodeTelemetry.setDeviceIdentity(widget->targetDevice());
                     });
    nodeTelemetry.setTargetConnected(widget->targetReady());
    nodeTelemetry.setDeviceIdentity(widget->targetDevice());
    engine.rootContext()->setContextProperty("NODETELEMETRY", &nodeTelemetry);
    nodeTelemetry.start();
    LogFile::instance().appendToLogFile(
        QStringLiteral("RMS telemetry: node %1 boot %2 publishing on UDP %3")
            .arg(nodeTelemetry.nodeId(), nodeTelemetry.bootId())
            .arg(int(ta::rms::kObservationPort)),
        LogType::interfaceLevel);

    engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
    //    return -1;
}
