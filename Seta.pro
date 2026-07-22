QT += charts qml quick printsupport widgets xml

CONFIG += c++17
#QMAKE_CXXFLAGS += /std:c++17

VERSION = 4.0
QMAKE_TARGET_PRODUCT = "SETA"

# F9B: build identity embedded at COMPILE time (no runtime git). qmake runs
# git once at build-configuration time; the app never shells out to git and
# the customer machine needs no Git / repo / Qt Creator. Build date/time comes
# from the compiler (__DATE__/__TIME__ in main.cpp).
APP_VERSION_STR = 0.9.0
GIT_SHA = $$system(git -C \"$$PWD\" rev-parse --short HEAD)
isEmpty(GIT_SHA): GIT_SHA = unknown
DEFINES += APP_VERSION_STR=\\\"$$APP_VERSION_STR\\\"
DEFINES += APP_GIT_SHA=\\\"$$GIT_SHA\\\"
CONFIG(release, debug|release): DEFINES += APP_BUILD_CONFIG=\\\"Release\\\"
else: DEFINES += APP_BUILD_CONFIG=\\\"Debug\\\"
#QMAKE_TARGET_PRODUCT = "TACHUS CPU"

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
    src/mode/OperatingModeService.cpp \
    src/training/TrainingProgramController.cpp \
    src/training/TrainingBlockMetrics.cpp \
    src/training/CallDiagnoseController.cpp \
    src/training/CallDiagnoseAnalytics.cpp

# Offline coach-analytics module (pure C++, independent from Qt/QML).
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
    src/training/CallDiagnoseController.h
INCLUDEPATH += src/training
INCLUDEPATH += src/finals
INCLUDEPATH += src/finals10m
INCLUDEPATH += src/qualification
INCLUDEPATH += src/incident
INCLUDEPATH += src/mode

# Session Reliability Layer (M0) - QtCore-only storage foundation.
include(Reliability.pri)
# QSoundEffect for the finals audio cues (FinalsAudioService).
QT += multimedia

# translations.qrc removed (S2.3): the file never existed and no root-level
# translation assets do either (QModMaster's live in the vendored module).
RESOURCES += qml.qrc \
    images.qrc

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

TRANSLATIONS += \
    translations/german.ts \
    translations/italain.ts \
    translations/french.ts \
    translations/spanish.ts \
    translations/chinese.ts

lupdate_only{
SOURCES = main.qml \
        CenterPane.qml \
        ClosePopupDialog.qml \
        Header.qml \
        LeftPanel.qml \
        LoginPage.qml \
        MatchReport.qml \
        MatchReportInfo.qml \
        ModConnectorDialog.qml \
        PdfPage.qml \
        PdfSeriesPage.qml \
        RightPanel.qml \
        SeriesComponent.qml \
        SettingsPage.qml \
        ShootingPage.qml \
        SummaryPage.qml
}

#INCLUDEPATH += "C:/Program Files (x86)/Windows Kits/10/Include/10.0.17763.0/ucrt"
##LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/ucrt/x64"
#LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/ucrt/x86"

