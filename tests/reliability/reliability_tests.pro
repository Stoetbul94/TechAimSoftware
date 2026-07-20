# Tests for the Session Reliability Layer (M0 storage + M1 core).
# QT deliberately restricted to core: compiling at all PROVES the layer has
# no QML/GUI dependency (required M0 test #14 / M1 layering rule).
QT = core
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = reliability_tests

include(../../Reliability.pri)

# Phase B0: the qualification write-path seam. QtCore-only — compiling it in
# this GUI-free harness proves it carries no QML/GUI dependency.
INCLUDEPATH += $$PWD/../../src
SOURCES += $$PWD/../../src/qualification/QualificationController.cpp
SOURCES += $$PWD/../../src/incident/EstIncidentController.cpp
HEADERS += $$PWD/../../src/qualification/QualificationController.h
HEADERS += $$PWD/../../src/incident/EstIncidentController.h

# Committed golden fixtures live next to the sources (byte-exact, -text in
# .gitattributes). The harness reads them from the source tree.
DEFINES += RELIABILITY_FIXTURES_DIR=\\\"$$PWD/fixtures\\\"

HEADERS += \
    test_support.h

SOURCES += \
    main.cpp \
    test_support.cpp \
    tst_storagepaths.cpp \
    tst_fixedpoint.cpp \
    tst_events.cpp \
    tst_serializer.cpp \
    tst_hashchain.cpp \
    tst_writer.cpp \
    tst_reader.cpp \
    tst_validator.cpp \
    tst_reducer.cpp \
    tst_incidents.cpp \
    tst_qualification.cpp \
    tst_snapshot.cpp \
    tst_store.cpp \
    tst_recovery.cpp \
    tst_fixtures.cpp
