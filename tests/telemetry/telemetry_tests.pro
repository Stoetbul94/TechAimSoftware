# RMS node telemetry — the network half.
#
# The QtCore-only half (identity, publisher, accepted-shot seam, dedup,
# bounded outbox, node-unaffected guarantees) lives in tests/reliability,
# where compiling it at all proves it needs no socket. THIS harness exists
# for the one piece that genuinely does: the UDP sink, and the node → wire →
# decode path end to end.
#
#   qmake telemetry_tests.pro && mingw32-make -f Makefile.Release
#   ./release/telemetry_tests.exe

QT = core network
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = telemetry_tests

include(../../Reliability.pri)
include(../../Telemetry.pri)

INCLUDEPATH += $$PWD/../../src

SOURCES += \
    main.cpp \
    tst_udp_sink.cpp
