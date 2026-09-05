# Tech Aim RMS-node harness.
#
# QT is deliberately core only: it links the node's telemetry conversion, the
# reliability journal reader AND the RMS observer's ingest, and compiling all
# three here without QtGui or QtNetwork is what proves none of that chain
# depends on a window or a socket.
#
# It exercises the 2026-09-05 physical session end to end against the committed
# journal, so a regression in the conversion, the dedup or the gap logic fails
# against a case that actually happened rather than one someone imagined.
#
#   qmake rmsnode_tests.pro && mingw32-make -f Makefile.Release
#   ./release/rmsnode_tests.exe

QT = core
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = rmsnode_tests

ROOT = $$PWD/../..
INCLUDEPATH += $$ROOT/src

# The fixture is found relative to the SOURCE, not the build directory, so the
# harness can be run from anywhere.
DEFINES += RMSNODE_TEST_ROOT=\\\"$$PWD\\\"

include($$ROOT/Reliability.pri)

SOURCES += \
    main.cpp \
    test_support.cpp \
    tst_physical_replay.cpp \
    $$ROOT/src/rms/RmsProtocol.cpp \
    $$ROOT/src/rms/TargetNodeRecord.cpp \
    $$ROOT/src/rms/RangeMonitor.cpp \
    $$ROOT/src/rms/CompetitionState.cpp

HEADERS += \
    test_support.h \
    $$ROOT/src/rms/RmsProtocol.h \
    $$ROOT/src/rms/TargetNodeRecord.h \
    $$ROOT/src/rms/RangeMonitor.h \
    $$ROOT/src/rms/CompetitionState.h \
    $$ROOT/src/telemetry/ShotTelemetry.h
