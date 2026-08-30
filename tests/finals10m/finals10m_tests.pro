# F1 acceptance tests for the 10m Air Rifle / Air Pistol FINAL controller
# (standalone QtCore console app — proves the domain has no QML/GUI dependency).
QT = core
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = finals10m_tests

include(../../Reliability.pri)

# 23: legacy-session evidence. The golden journals in the reliability
# suite predate shotRole, which is exactly what makes them the right
# evidence that role is derived from the EVENT TYPE and needs no migration.
DEFINES += RELIABILITY_FIXTURES_DIR=\\\"$$PWD/../reliability/fixtures\\\"

INCLUDEPATH += $$PWD/../../src
INCLUDEPATH += $$PWD/../../src/finals10m
INCLUDEPATH += $$PWD/../../src/incident

SOURCES += \
    tst_finals10m.cpp \
    $$PWD/../../src/finals10m/Finals10mController.cpp \
    $$PWD/../../src/incident/EstIncidentController.cpp

HEADERS += \
    $$PWD/../../src/finals10m/Finals10mController.h \
    $$PWD/../../src/finals10m/Finals10mConfig.h \
    $$PWD/../../src/finals10m/Finals10mTypes.h \
    $$PWD/../../src/incident/EstIncidentController.h
