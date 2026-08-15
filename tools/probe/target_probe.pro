QT       -= core gui
CONFIG   += console c++17
CONFIG   -= app_bundle qt
TEMPLATE  = app
TARGET    = target_probe
LIBMODBUS = $$PWD/../../ModReader/3rdparty/libmodbus
INCLUDEPATH += $$LIBMODBUS $$PWD/../..
SOURCES += target_probe.cpp \
    $$LIBMODBUS/modbus.c \
    $$LIBMODBUS/modbus-data.c \
    $$LIBMODBUS/modbus-rtu.c
win32: LIBS += -lws2_32 -lsetupapi
