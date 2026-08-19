# ─────────────────────────────────────────────────────────────────────────────
# Tech Aim Range Management System.
#
# A SEPARATE BINARY. It does not include Seta.pro, does not link ModReader and
# has no serial/Modbus dependency: RMS never talks to target hardware, only to
# target NODES, and only by listening. That separation is what keeps range
# management out of the single-target application.
#
# Milestone 1 — read-only observation of whatever nodes are heard.
# Milestone 3 — a configured physical range: persistent lanes, persistent
#               nodeId <-> laneId mapping, automatic reconnection. Still no
#               commands; the only thing RMS writes is its own configuration.
#
#   qmake TechAimRMS.pro && mingw32-make -f Makefile.Release
# ─────────────────────────────────────────────────────────────────────────────

QT += core gui qml quick network
CONFIG += c++17
TARGET = TechAimRMS
TEMPLATE = app

INCLUDEPATH += $$PWD/../src

# The development simulator is compiled in for this milestone because there is
# no live range to point at yet. It is confined to src/rms/dev/, is guarded by
# this define, and prints a standing banner whenever it runs.
DEFINES += TECHAIM_RMS_DEV_SIMULATOR

SOURCES += \
    main.cpp \
    $$PWD/../src/rms/RmsProtocol.cpp \
    $$PWD/../src/rms/TargetNodeRecord.cpp \
    $$PWD/../src/rms/RangeMonitor.cpp \
    $$PWD/../src/rms/RangeListModel.cpp \
    $$PWD/../src/rms/RangeDefinition.cpp \
    $$PWD/../src/rms/RangeStore.cpp \
    $$PWD/../src/rms/RangeConfigurationService.cpp \
    $$PWD/../src/rms/LaneListModel.cpp \
    $$PWD/../src/rms/UnassignedNodeModel.cpp \
    $$PWD/../src/rms/RmsJsonStore.cpp \
    $$PWD/../src/rms/CompetitionState.cpp \
    $$PWD/../src/rms/TargetGeometry.cpp \
    $$PWD/../src/rms/DisplayController.cpp \
    $$PWD/../src/rms/DisplayLaneModel.cpp \
    $$PWD/../src/rms/MatchPlan.cpp \
    $$PWD/../src/rms/AthleteRegistry.cpp \
    $$PWD/../src/rms/MatchPlanService.cpp \
    $$PWD/../src/rms/PlanLaneModel.cpp \
    $$PWD/../src/rms/AthleteListModel.cpp \
    $$PWD/../src/rms/ProgrammeDisplay.cpp \
    $$PWD/../src/rms/RmsUdpObserver.cpp \
    $$PWD/../src/rms/dev/SimulatedRange.cpp

HEADERS += \
    $$PWD/../src/rms/RmsProtocol.h \
    $$PWD/../src/rms/TargetNodeRecord.h \
    $$PWD/../src/rms/RangeMonitor.h \
    $$PWD/../src/rms/RangeListModel.h \
    $$PWD/../src/rms/RangeDefinition.h \
    $$PWD/../src/rms/RangeStore.h \
    $$PWD/../src/rms/RangeConfigurationService.h \
    $$PWD/../src/rms/LaneListModel.h \
    $$PWD/../src/rms/UnassignedNodeModel.h \
    $$PWD/../src/rms/RmsJsonStore.h \
    $$PWD/../src/rms/CompetitionState.h \
    $$PWD/../src/rms/TargetGeometry.h \
    $$PWD/../src/rms/DisplayController.h \
    $$PWD/../src/rms/DisplayLaneModel.h \
    $$PWD/../src/rms/Athlete.h \
    $$PWD/../src/rms/MatchPlan.h \
    $$PWD/../src/rms/AthleteRegistry.h \
    $$PWD/../src/rms/MatchPlanService.h \
    $$PWD/../src/rms/PlanLaneModel.h \
    $$PWD/../src/rms/AthleteListModel.h \
    $$PWD/../src/rms/ProgrammeDisplay.h \
    $$PWD/../src/rms/RmsUdpObserver.h \
    $$PWD/../src/rms/dev/SimulatedRange.h

RESOURCES += rms.qrc
