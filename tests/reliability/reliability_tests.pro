# M0 tests for the Session Reliability Layer storage foundation.
# QT deliberately restricted to core: compiling at all PROVES the layer has
# no QML/GUI dependency (required M0 test #14).
QT = core
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = reliability_tests

include(../../Reliability.pri)

SOURCES += tst_storagepaths.cpp
