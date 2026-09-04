# Software one-node test for the Tech Aim RMS node.
#
# Console only, QtCore + QtNetwork. It links the SAME control-plane sources the
# node links, so the handshake it performs is the production handshake rather
# than a re-implementation that could agree with a bug.
#
#   qmake rmsnodecheck.pro && mingw32-make -f Makefile.Release
#   ./release/rmsnodecheck.exe

QT = core network
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = rmsnodecheck

ROOT = $$PWD/../..
INCLUDEPATH += $$ROOT/src

SOURCES += \
    main.cpp \
    $$ROOT/src/rms/RmsProtocol.cpp \
    $$ROOT/src/rms/RmsJsonStore.cpp \
    $$ROOT/src/rms/control/ControlProtocol.cpp \
    $$ROOT/src/rms/control/ControlAuth.cpp \
    $$ROOT/src/rms/control/CommandJournal.cpp \
    $$ROOT/src/rms/control/RmsControlClient.cpp

HEADERS += \
    $$ROOT/src/rms/RmsProtocol.h \
    $$ROOT/src/rms/RmsJsonStore.h \
    $$ROOT/src/rms/control/ControlProtocol.h \
    $$ROOT/src/rms/control/ControlAuth.h \
    $$ROOT/src/rms/control/CommandJournal.h \
    $$ROOT/src/rms/control/RmsControlClient.h
