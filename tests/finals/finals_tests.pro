# Phase A acceptance tests for the 3P FINAL controller (standalone console app).
QT += core gui widgets
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = finals_tests

INCLUDEPATH += ../../src/finals

SOURCES += \
    tst_finals3p.cpp \
    ../../src/finals/Finals3PController.cpp

HEADERS += \
    ../../src/finals/Finals3PController.h \
    ../../src/finals/Finals3PTypes.h \
    ../../src/finals/Finals3PConfig.h
