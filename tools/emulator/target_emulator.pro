# Tech Aim target emulator — Modbus TCP server speaking the real register map.
#
# Deliberately links the SAME vendored libmodbus the application uses, so the
# protocol behaviour under test is the protocol the application will meet.
#
#   qmake target_emulator.pro && mingw32-make -f Makefile.Release
#   release\target_emulator.exe --scenario B
QT       -= core gui
CONFIG   += console c++17
CONFIG   -= app_bundle qt
TEMPLATE  = app
TARGET    = target_emulator

LIBMODBUS = $$PWD/../../ModReader/3rdparty/libmodbus
# libmodbus sources include "ModReader/3rdparty/libmodbus/config.h" by
# repo-relative path, so the repository ROOT must be on the include path too.
INCLUDEPATH += $$LIBMODBUS $$PWD/../..
DEFINES += _CRT_SECURE_NO_WARNINGS

SOURCES += target_emulator.cpp \
    $$LIBMODBUS/modbus.c \
    $$LIBMODBUS/modbus-data.c \
    $$LIBMODBUS/modbus-tcp.c

win32: LIBS += -lws2_32
