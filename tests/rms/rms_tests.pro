# Tech Aim RMS test harness.
#
# QT is deliberately restricted to core + network: compiling at all PROVES the
# RMS observer, the range configuration, the view models and the protocol carry
# no QML or GUI dependency. It also means this harness needs no platform
# plugin, so it can never block on the "no Qt platform plugin" modal that has
# cost time on the GUI-linked harnesses.
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
    tst_scale.cpp \
    tst_udp.cpp \
    tst_range_config.cpp \
    tst_match_plan.cpp \
    tst_competition_state.cpp \
    tst_target_display.cpp \
    tst_target_geometry.cpp \
    tst_field_test.cpp \
    tst_control.cpp \
    tst_replay.cpp \
    tst_readonly.cpp \
    $$PWD/../../src/rms/StationCode.cpp \
    $$PWD/../../src/rms/FieldTestRecorder.cpp \
    $$PWD/../../src/rms/NetworkDiagnostics.cpp \
    $$PWD/../../src/rms/FieldTestService.cpp \
    $$PWD/../../src/rms/RmsProtocol.cpp \
    $$PWD/../../src/rms/control/ControlProtocol.cpp \
    $$PWD/../../src/rms/control/ControlAuth.cpp \
    $$PWD/../../src/rms/control/NodeControlEndpoint.cpp \
    $$PWD/../../src/rms/control/RmsControlClient.cpp \
    $$PWD/../../src/rms/TargetNodeRecord.cpp \
    $$PWD/../../src/rms/RangeMonitor.cpp \
    $$PWD/../../src/rms/RangeListModel.cpp \
    $$PWD/../../src/rms/RangeDefinition.cpp \
    $$PWD/../../src/rms/RangeStore.cpp \
    $$PWD/../../src/rms/RangeConfigurationService.cpp \
    $$PWD/../../src/rms/LaneListModel.cpp \
    $$PWD/../../src/rms/UnassignedNodeModel.cpp \
    $$PWD/../../src/rms/RmsJsonStore.cpp \
    $$PWD/../../src/rms/CompetitionState.cpp \
    $$PWD/../../src/rms/TargetGeometry.cpp \
    $$PWD/../../src/rms/DisplayController.cpp \
    $$PWD/../../src/rms/DisplayLaneModel.cpp \
    $$PWD/../../src/rms/MatchPlan.cpp \
    $$PWD/../../src/rms/AthleteRegistry.cpp \
    $$PWD/../../src/rms/MatchPlanService.cpp \
    $$PWD/../../src/rms/PlanLaneModel.cpp \
    $$PWD/../../src/rms/AthleteListModel.cpp \
    $$PWD/../../src/rms/ProgrammeDisplay.cpp \
    $$PWD/../../src/rms/RmsUdpObserver.cpp \
    $$PWD/../../src/rms/dev/SimulatedRange.cpp

HEADERS += \
    test_support.h \
    $$PWD/../../src/rms/StationCode.h \
    $$PWD/../../src/rms/FieldTestRecorder.h \
    $$PWD/../../src/rms/NetworkDiagnostics.h \
    $$PWD/../../src/rms/FieldTestService.h \
    $$PWD/../../src/rms/RmsProtocol.h \
    $$PWD/../../src/rms/TargetNodeRecord.h \
    $$PWD/../../src/rms/RangeMonitor.h \
    $$PWD/../../src/rms/RangeListModel.h \
    $$PWD/../../src/rms/RangeDefinition.h \
    $$PWD/../../src/rms/RangeStore.h \
    $$PWD/../../src/rms/RangeConfigurationService.h \
    $$PWD/../../src/rms/LaneListModel.h \
    $$PWD/../../src/rms/UnassignedNodeModel.h \
    $$PWD/../../src/rms/RmsJsonStore.h \
    $$PWD/../../src/rms/CompetitionState.h \
    $$PWD/../../src/rms/TargetGeometry.h \
    $$PWD/../../src/rms/DisplayController.h \
    $$PWD/../../src/rms/DisplayLaneModel.h \
    $$PWD/../../src/rms/Athlete.h \
    $$PWD/../../src/rms/MatchPlan.h \
    $$PWD/../../src/rms/AthleteRegistry.h \
    $$PWD/../../src/rms/MatchPlanService.h \
    $$PWD/../../src/rms/PlanLaneModel.h \
    $$PWD/../../src/rms/AthleteListModel.h \
    $$PWD/../../src/rms/ProgrammeDisplay.h \
    $$PWD/../../src/rms/RmsUdpObserver.h \
    $$PWD/../../src/rms/dev/SimulatedRange.h
