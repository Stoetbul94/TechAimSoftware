# Phase A acceptance tests for the 3P FINAL controller (standalone console app).
# qml: LanguageService uses QQmlEngine::retranslate() for live switching.
QT += core gui widgets multimedia qml
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = finals_tests

INCLUDEPATH += ../../src/finals

# Session Reliability Layer (M0): the controller resolves its journal path
# through StoragePaths.
include(../../Reliability.pri)

SOURCES += \
    tst_finals3p.cpp \
    ../../src/finals/Finals3PController.cpp \
    ../../src/finals/FinalsReportBuilder.cpp \
    ../../src/finals/FinalsAudioService.cpp \
    ../../src/app/ProductIdentity.cpp \
    ../../src/app/LanguageService.cpp

# The compiled German catalogue must be embedded here too, so the harness can
# assert the deployed-install behaviour (catalogue inside the binary).
RESOURCES += ../../techaim_translations.qrc

HEADERS += \
    ../../src/app/ProductIdentity.h \
    ../../src/app/LanguageService.h \
    ../../src/finals/Finals3PController.h \
    ../../src/finals/Finals3PTypes.h \
    ../../src/finals/Finals3PConfig.h \
    ../../src/finals/FinalsReportData.h \
    ../../src/finals/FinalsReportBuilder.h \
    ../../src/finals/FinalsAudioService.h
