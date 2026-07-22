# Training Lab (T1) harness — Technical Blocks domain, gate, visibility,
# lifecycle, analytics, recovery, separation. QtCore-only console binary
# (compiling proves the Training domain carries no QML/GUI dependency).
QT = core
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = training_tests

include(../../Reliability.pri)

INCLUDEPATH += $$PWD/../../src
SOURCES += \
    $$PWD/../../src/training/TrainingProgramController.cpp \
    $$PWD/../../src/training/TrainingBlockMetrics.cpp \
    $$PWD/../../src/analytics/CoachAnalyticsEngine.cpp
HEADERS += \
    $$PWD/../../src/training/TrainingProgramTypes.h \
    $$PWD/../../src/training/TrainingBlockMetrics.h \
    $$PWD/../../src/training/TrainingProgramController.h

SOURCES += tst_training.cpp
