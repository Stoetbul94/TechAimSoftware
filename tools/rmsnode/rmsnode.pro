# DEVELOPMENT TOOL — a deterministic multi-node telemetry harness.
#
# It drives the REAL production path: the real QualificationController, its
# real SessionStore, the real NodeTelemetryService and the real UDP sink. The
# ONLY thing it replaces is the QML click — it calls exactly the invokables
# ShootingPage.qml calls (startSession / beginPreparation / beginSighting /
# beginOfficialMatch / submitSighter / submitOfficial), with the same
# arguments and the same simulated-source flag.
#
# It is NOT a telemetry-only shot route: every shot it publishes has been
# classified, scored, validated by the reducer and durably journalled first,
# because that is the only way a shot can reach eventApplied.
#
# It exists because milestone 2 §17 needs six distinct node identities on one
# machine and the application takes a single-instance lock.
#
#   qmake rmsnode.pro && mingw32-make -f Makefile.Release
#   ./release/rmsnode.exe --nodes 6 --localhost

QT = core network
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = rmsnode

include(../../Reliability.pri)
include(../../Telemetry.pri)

INCLUDEPATH += $$PWD/../../src
INCLUDEPATH += $$PWD/../../src/qualification

SOURCES += \
    main.cpp \
    $$PWD/../../src/qualification/QualificationController.cpp \
    $$PWD/../../src/mode/OperatingModeService.cpp

HEADERS += \
    $$PWD/../../src/qualification/QualificationController.h \
    $$PWD/../../src/mode/OperatingModeService.h \
    $$PWD/../../src/mode/OperatingMode.h
