# Tech Aim RMS test harness — milestone 1, read-only observer.
#
# QT is deliberately restricted to core + network: compiling at all PROVES the
# RMS observer, its dashboard model and its protocol carry no QML or GUI
# dependency. It also means this harness needs no platform plugin, so it can
# never block on the "no Qt platform plugin" modal that has cost time on the
# GUI-linked harnesses.
#
#   qmake rms_tests.pro && mingw32-make -f Makefile.Release
#   ./release/rms_tests.exe

QT = core network
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = rms_tests

INCLUDEPATH += $$PWD/../../src

# The read-only guard scans the authored RMS sources. It needs to know where
# they are; it is a source-tree test by nature, exactly like the committed
# golden fixtures in the reliability harness.
DEFINES += RMS_SOURCE_ROOT=\\\"$$PWD/../..\\\"

# The simulator is compiled into the harness so the scenario it produces is
# tested, not assumed. It stays confined to src/rms/dev/ and the read-only
# guard excludes that directory by name.
DEFINES += TECHAIM_RMS_DEV_SIMULATOR

SOURCES += \
    main.cpp \
    test_support.cpp \
    tst_protocol.cpp \
    tst_monitor.cpp \
    tst_simulator.cpp \
    tst_udp.cpp \
    tst_readonly.cpp \
    $$PWD/../../src/rms/RmsProtocol.cpp \
    $$PWD/../../src/rms/TargetNodeRecord.cpp \
    $$PWD/../../src/rms/RangeMonitor.cpp \
    $$PWD/../../src/rms/RangeListModel.cpp \
    $$PWD/../../src/rms/ProgrammeDisplay.cpp \
    $$PWD/../../src/rms/RmsUdpObserver.cpp \
    $$PWD/../../src/rms/dev/SimulatedRange.cpp

HEADERS += \
    test_support.h \
    $$PWD/../../src/rms/RmsProtocol.h \
    $$PWD/../../src/rms/TargetNodeRecord.h \
    $$PWD/../../src/rms/RangeMonitor.h \
    $$PWD/../../src/rms/RangeListModel.h \
    $$PWD/../../src/rms/ProgrammeDisplay.h \
    $$PWD/../../src/rms/RmsUdpObserver.h \
    $$PWD/../../src/rms/dev/SimulatedRange.h
