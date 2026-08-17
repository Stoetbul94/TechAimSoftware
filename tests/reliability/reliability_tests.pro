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
INCLUDEPATH += $$PWD/../../src/training
SOURCES += $$PWD/../../src/qualification/QualificationController.cpp
SOURCES += $$PWD/../../src/incident/EstIncidentController.cpp
SOURCES += $$PWD/../../src/mode/OperatingModeService.cpp
# Stage 5: the Wind Map controller. Compiling it in this QT=core harness is
# what proves it carries no QML/GUI dependency.
SOURCES += $$PWD/../../src/training/WindMapController.cpp
SOURCES += $$PWD/../../src/training/WindMapAnalytics.cpp
SOURCES += $$PWD/../../src/training/WindMapVerdict.cpp
HEADERS += $$PWD/../../src/training/WindMapController.h
HEADERS += $$PWD/../../src/training/WindMapAnalytics.h
HEADERS += $$PWD/../../src/training/WindMapVerdict.h
HEADERS += $$PWD/../../src/qualification/QualificationController.h
HEADERS += $$PWD/../../src/incident/EstIncidentController.h
HEADERS += $$PWD/../../src/mode/OperatingMode.h
HEADERS += $$PWD/../../src/mode/OperatingModeService.h

# Committed golden fixtures live next to the sources (byte-exact, -text in
# .gitattributes). The harness reads them from the source tree.
DEFINES += RELIABILITY_FIXTURES_DIR=\\\"$$PWD/fixtures\\\"

HEADERS += \
    test_support.h

SOURCES += \
    main.cpp \
    test_support.cpp \
    tst_storagepaths.cpp \
    tst_capture_profile.cpp \
    ../../src/app/DocumentationCapture.cpp \
    ../../src/app/ProductIdentity.cpp \
    ../../src/app/BrandPackage.cpp \
    tst_brandpackage.cpp \
    tst_homepage_layout.cpp \
    tst_windmap.cpp \
    tst_windmap_recovery.cpp \
    tst_windmap_controller.cpp \
    tst_windmap_analytics.cpp \
    tst_windmap_qml.cpp \
    tst_windmap_perf.cpp \
    tst_windmap_verdict.cpp \
    tst_windmap_dispersion.cpp \
    tst_target_hardware.cpp \
    ../../src/target/TargetDeviceFingerprint.cpp \
    ../../src/target/PaperFeedCoordinator.cpp \
    seed_windmap.cpp \
    tst_training_parity.cpp \
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
    tst_operatingmode.cpp \
    tst_fixtures.cpp

# product/seta: measure the SETA build, not the Tech Aim one.
DEFINES += BRAND_SETA
