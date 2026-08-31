QT += charts qml quick widgets xml

# PrintSupport is a DESKTOP-ONLY link. Audited during the Android milestone
# (A1/A2): no QPrinter or QPrintDialog is ever constructed anywhere in the
# codebase — customprint.cpp included the headers but every PDF path goes
# through QPdfWriter, which lives in QtGui. Windows linkage is unchanged;
# Android simply does not carry a module it cannot use (Android has no native
# print engine, so QPrinter would silently degrade to PDF output anyway).
!android: QT += printsupport

CONFIG += c++17
#QMAKE_CXXFLAGS += /std:c++17

# ── P0 Phase C: product identity for the Windows version resource ────────
# TARGET fixes the executable name to TechAim.exe regardless of the project
# filename. Keep these in step with src/app/ProductIdentity.cpp — that is the
# authoritative source; these are the values qmake bakes into the PE
# VERSIONINFO block, which qmake cannot read from C++.
TARGET = TechAim
VERSION = 0.9.0
# The version resource is hand-authored (TechAim.rc) rather than generated
# from QMAKE_TARGET_*: qmake leaves InternalName empty and marks the binary
# VFT_DLL. RC_FILE takes precedence over the QMAKE_TARGET_* variables.
win32: RC_FILE = TechAim.rc

# ── Android tablet shell (milestone A1/A2) ───────────────────────────────
# Everything Android-specific in this project file lives in THIS scope. The
# Windows build never evaluates it, so the desktop product is unaffected by
# construction.
android {
    # The Java/manifest/gradle side of the APK. androiddeployqt merges this
    # over the Qt template; anything absent here falls back to Qt's default.
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    # SDK 23 is the floor Qt 6.5 supports; 34 matches the newest platform
    # installed on the build machine. Both are recorded in
    # docs/architecture/android-product-architecture.md.
    ANDROID_MIN_SDK_VERSION = 23
    ANDROID_TARGET_SDK_VERSION = 34

    # ANDROID_VERSION_NAME is set below, once APP_VERSION_STR exists — qmake
    # evaluates top to bottom and this scope runs before that assignment.
    ANDROID_VERSION_CODE = 1
}

# Release-clean: a previously built Seta.exe sits in the SAME output folder
# and stays launchable after the rename, so an operator (or a stale shortcut)
# could run an outdated binary that looks like the product. Delete known
# legacy executables from the output directory on every successful link.
# Resolve the output directory at qmake time: the generated make DESTDIR
# carries a trailing comment, so $(DESTDIR) would expand to "release/ " and
# delete the directory rather than the file. shell_path() picks the right
# separators for whichever shell qmake configured, and both rm -f and del
# exit 0 when the legacy file is already absent.
#
# A1/A2: scoped to the DESKTOP build. An Android build links a .so, never a
# Seta.exe, so there is no stale legacy executable to protect anyone from —
# and running the cleanup there actively breaks the build. With Git's sh.exe
# absent from PATH (which the Android build requires, so that qmake emits
# native paths androiddeployqt can follow) $(DEL_FILE) resolves to cmd's `del`,
# which reports "The system cannot find the file specified" and returns a
# FAILURE exit code for an already-absent file. make then aborts the link step.
# The original comment here assumed del exits 0 in that case; it does not.
# Windows behaviour is unchanged.
!android {
    CONFIG(release, debug|release): LEGACY_OUT_DIR = $$OUT_PWD/release
    else:                           LEGACY_OUT_DIR = $$OUT_PWD/debug
    LEGACY_EXES = Seta.exe Seeds.exe
    for(legacy, LEGACY_EXES) {
        QMAKE_POST_LINK += $$quote($(DEL_FILE) $$shell_quote($$shell_path($$LEGACY_OUT_DIR/$$legacy)))$$escape_expand(\\n\\t)
    }
}

# F9B: build identity embedded at COMPILE time (no runtime git). qmake runs
# git once at build-configuration time; the app never shells out to git and
# the customer machine needs no Git / repo / Qt Creator. Build date/time comes
# from the compiler (__DATE__/__TIME__ in main.cpp).
# 0.9.0-RC2 — INTERNAL FIELD TEST. This is a release candidate for controlled
# live-range testing, not the public 1.0. The channel travels with the version
# so a binary can never be mistaken for a general release.
APP_VERSION_STR = 0.9.0-RC3a-SETA
# Android carries its OWN channel string. The Windows value above is a frozen
# release-candidate identity that was audited and shipped; an APK must never
# be mistaken for it. A1 is a development milestone, not a release, and the
# label says so wherever the version is displayed.
android: APP_VERSION_STR = 0.9.0-ANDROID-A2.5
GIT_SHA = $$system(git -C \"$$PWD\" rev-parse --short HEAD)
isEmpty(GIT_SHA): GIT_SHA = unknown
DEFINES += APP_VERSION_STR=\\\"$$APP_VERSION_STR\\\"
DEFINES += APP_GIT_SHA=\\\"$$GIT_SHA\\\"
CONFIG(release, debug|release): DEFINES += APP_BUILD_CONFIG=\\\"Release\\\"
else: DEFINES += APP_BUILD_CONFIG=\\\"Debug\\\"
# Now that APP_VERSION_STR is resolved, hand it to the APK manifest so the
# version shown in Android's app info matches the version shown in-app.
android: ANDROID_VERSION_NAME = $$APP_VERSION_STR
#QMAKE_TARGET_PRODUCT = "TACHUS CPU"

SOURCES += src/app/CompetitionClock.cpp
SOURCES += main.cpp \
    customprint.cpp \
    appsettings.cpp \
    logfile.cpp \
    receiverTachus.cpp \
    sender.cpp \
    src/analytics/CoachAnalyticsEngine.cpp \
    src/bridge/tachusshotbuilder.cpp \
    src/bridge/coachreportvariant.cpp \
    src/bridge/coachreportbridge.cpp \
    src/bridge/coachreportfeeder.cpp \
    src/bridge/pdfexporter.cpp \
    src/finals/Finals3PController.cpp \
    src/finals/FinalsReportBuilder.cpp \
    src/finals/FinalsAudioService.cpp \
    src/finals10m/Finals10mController.cpp \
    src/qualification/QualificationController.cpp \
    src/incident/EstIncidentController.cpp \
    src/app/ProductIdentity.cpp \
    src/app/BrandPackage.cpp \
    src/app/LanguageService.cpp \
    src/app/DocumentationCapture.cpp \
    src/mode/OperatingModeService.cpp \
    src/training/TrainingProgramController.cpp \
    src/training/TrainingBlockMetrics.cpp \
    src/training/CallDiagnoseController.cpp \
    src/training/CallDiagnoseAnalytics.cpp \
    src/training/GroupPatternAnalyzer.cpp \
    src/training/PositionTransitionController.cpp \
    src/training/WindMapController.cpp \
    src/training/WindMapAnalytics.cpp \
    src/training/WindMapVerdict.cpp \
    src/target/SerialDeviceProvider.cpp \
    src/target/TargetDeviceFingerprint.cpp \
    src/target/PaperFeedCoordinator.cpp

# Offline coach-analytics module (pure C++, independent from Qt/QML).
HEADERS += src/app/CompetitionClock.h
HEADERS += \
    src/analytics/ShotAnalyticsTypes.h \
    src/analytics/CoachReportData.h \
    src/analytics/CoachAnalyticsEngine.h
INCLUDEPATH += src/analytics

# QML boundary layer (the ONLY place Qt meets the analytics engine).
# tachusshotbuilder is pure C++ (Qt-free); the rest bridge to Qt/QML.
HEADERS += \
    src/bridge/tachusshotbuilder.h \
    src/bridge/coachreportvariant.h \
    src/bridge/coachreportbridge.h \
    src/bridge/coachreportfeeder.h \
    src/bridge/pdfexporter.h
INCLUDEPATH += src/bridge

# 3P FINAL — dedicated finals domain (ISSF Rule Book 2026 Edition 2025,
# Second Print 07/2026). Separate from qualification; see
# docs/3p-finals-discipline.md.
HEADERS += \
    src/finals/Finals3PTypes.h \
    src/finals/Finals3PConfig.h \
    src/finals/Finals3PController.h \
    src/finals/FinalsReportData.h \
    src/finals/FinalsReportBuilder.h \
    src/finals/FinalsAudioService.h \
    src/qualification/QualificationController.h \
    src/incident/EstIncidentController.h \
    src/app/ProductIdentity.h \
    src/app/BrandPackage.h \
    src/app/ProductIdentityBridge.h \
    src/app/LanguageService.h \
    src/app/DocumentationCapture.h \
    src/mode/OperatingMode.h \
    src/mode/OperatingModeService.h

# 10m Air Rifle / Air Pistol FINAL — single-athlete training course (F1).
# ISSF Rule Book Edition 2025 (Second Print 07/2026), rule 6.17.2. Separate
# from the 3P Final; see docs/issf-rules/10m-finals-shared.md and
# docs/10m-finals-architecture.md.
HEADERS += \
    src/finals10m/Finals10mTypes.h \
    src/finals10m/Finals10mConfig.h \
    src/finals10m/Finals10mController.h

# Training Lab (T1) — separate domain from competition (docs/
# training-lab-architecture.md). Technical Blocks vertical slice.
HEADERS += \
    src/training/TrainingProgramTypes.h \
    src/training/TrainingBlockMetrics.h \
    src/training/TrainingProgramController.h \
    src/training/CallDiagnoseTypes.h \
    src/training/CallDiagnoseAnalytics.h \
    src/training/CallDiagnoseController.h \
    src/training/TargetGeometry.h \
    src/training/GroupPatternAnalyzer.h \
    src/training/PositionTransitionTypes.h \
    src/training/PositionTransitionController.h \
    src/training/WindMapTypes.h \
    src/training/WindMapController.h \
    src/training/WindMapAnalytics.h \
    src/training/WindMapVerdict.h

# RC2 target hardware: adapter identity, candidate filtering and the single
# automatic paper-feed authority. Pure logic, unit-tested without hardware.
HEADERS +=     src/target/SerialDeviceProvider.h     src/target/TargetDeviceFingerprint.h     src/target/PaperFeedCoordinator.h
INCLUDEPATH += src
INCLUDEPATH += src/training
INCLUDEPATH += src/finals
INCLUDEPATH += src/finals10m
INCLUDEPATH += src/qualification
INCLUDEPATH += src/incident
INCLUDEPATH += src/mode

# Session Reliability Layer (M0) - QtCore-only storage foundation.
include(Reliability.pri)
# Platform boundary (A1/A2). MUST come after Reliability.pri: the platform
# service resolves paths through ta::rel::StoragePaths.
include(Platform.pri)

# RMS node telemetry: the shared protocol contract + this station's publisher.
# Node -> RMS observation only; there is no inbound command path.
#
# SHARED, NOT PLATFORM-SPECIFIC: this include is deliberately unscoped, so the
# Windows, Android and SETA-branded builds all compile the SAME publisher and
# speak the SAME protocol v1. There is no per-platform networking fork.
include(Telemetry.pri)
QT += network
# QSoundEffect for the finals audio cues (FinalsAudioService).
QT += multimedia

# P0 Phase F: compiled .qm catalogues ship INSIDE the binary, so a deployed
# install cannot lose its translations to a missing file on disk.
RESOURCES += qml.qrc \
    images.qrc \
    techaim_translations.qrc

DISTFILES += \
    images/loginPage/combo_down.png \
    qml/qmlpolarchart/*

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    customprint.h \
    appsettings.h \
    defines.h \
    logfile.h \
    receiverTachus.h \
    sender.h

#SUBDIRS += \
#    ModReader/qModMaster.pro

include(ModReader/qModMaster.pro)

# P0 Phase F. English is the SOURCE language and needs no catalogue.
# The legacy german/french/italain/spanish/chinese .ts files were
# Tachus-era stubs covering only the vendored QModMaster forms (every entry
# "unfinished"); they are not product catalogues and are not built.
TRANSLATIONS += \
    translations/techaim_de_DE.ts

lupdate_only{
SOURCES = CallDiagnoseHud.qml 
        CallDiagnoseReportView.qml 
        CallDiagnoseRightPanel.qml 
        CenterPane.qml 
        CoachDashboardView.qml 
        CoachDetailedView.qml 
        CoachPrintView.qml 
        CoachReportCard.qml 
        CoachReportWindow.qml 
        ConnectionError.qml 
        Finals10mCommandPanel.qml 
        Finals10mHud.qml 
        Finals10mRightPanel.qml 
        FinalsAdvanceControl.qml 
        FinalsCommandOverlay.qml 
        FinalsDeveloperDrawer.qml 
        FinalsHud.qml 
        FinalsIncidentToast.qml 
        FinalsPerformanceBlock.qml 
        FinalsProgressIndicator.qml 
        FinalsReportTarget.qml 
        FinalsReportView.qml 
        FinalsTopStrip.qml 
        FloatingWindow.qml 
        FloatingWindowGrip.qml 
        Header.qml 
        HeatMapCanvas.qml 
        IncidentWindow.qml 
        IssfTargetCanvas.qml 
        LeftPanel.qml 
        LoginPage.qml 
        MatchReportInfo.qml 
        MatchReportView.qml 
        MetricCard.qml 
        MetricChip.qml 
        ModConnectorDialog.qml 
        PdfPage.qml 
        PdfSeriesPage.qml 
        PersistenceBanner.qml 
        PositionTransitionHud.qml 
        PositionTransitionReportView.qml 
        PositionTransitionRightPanel.qml 
        RecoveryDialog.qml 
        Report3P.qml 
        Report3PSeries.qml 
        ReportFooter.qml 
        ReportHeader.qml 
        ReportWindow.qml 
        RightPanel.qml 
        SectionTitle.qml 
        SeriesCard.qml 
        SeriesComponent.qml 
        SettingsPage.qml 
        ShootingPage.qml 
        ShotTargetCanvas.qml 
        StatChip.qml 
        SummaryReportView.qml 
        TechAimDialog.qml 
        TechAimDialogManager.qml 
        Theme.qml 
        TrainingHud.qml 
        TrainingReportView.qml 
        TrainingRightPanel.qml 
        VIcon.qml 
        WindowManager.qml 
        main.qml
}

#INCLUDEPATH += "C:/Program Files (x86)/Windows Kits/10/Include/10.0.17763.0/ucrt"
##LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/ucrt/x64"
#LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/ucrt/x86"

