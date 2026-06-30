#-------------------------------------------------
#
# Project created by QtCreator 2010-11-24T09:57:26
#
#-------------------------------------------------

QT       += core gui serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#TARGET = qModMaster
#TEMPLATE = app

SOURCES += ModReader/src/mainwindow.cpp \
    ModReader/3rdparty/libmodbus/modbus.c \
    ModReader/forms/about.cpp \
    ModReader/forms/settingsmodbusrtu.cpp \
    ModReader/forms/settingsmodbustcp.cpp \
    ModReader/src/modbusadapter.cpp \
    ModReader/src/eutils.cpp \
    ModReader/src/registersmodel.cpp \
    ModReader/src/rawdatamodel.cpp \
    ModReader/forms/settings.cpp \
    ModReader/forms/busmonitor.cpp \
    ModReader/3rdparty/libmodbus/modbus-data.c \
    ModReader/3rdparty/libmodbus/modbus-tcp.c \
    ModReader/3rdparty/libmodbus/modbus-rtu.c \
    ModReader/src/rawdatadelegate.cpp \
    ModReader/src/registersdatadelegate.cpp \
    ModReader/src/modbuscommsettings.cpp \
    ModReader/3rdparty/QsLog/QsLogDest.cpp \
    ModReader/3rdparty/QsLog/QsLog.cpp \
    ModReader/3rdparty/QsLog/QsLogDestConsole.cpp \
    ModReader/3rdparty/QsLog/QsLogDestFile.cpp \
    ModReader/src/infobar.cpp \
    ModReader/forms/tachuswidget.cpp

HEADERS  += ModReader/src/mainwindow.h \
    ModReader/3rdparty/libmodbus/modbus.h \
    ModReader/forms/about.h \
    ModReader/forms/settingsmodbusrtu.h \
    ModReader/forms/settingsmodbustcp.h \
    ModReader/src/modbusadapter.h \
    ModReader/src/eutils.h \
    ModReader/src/registersmodel.h \
    ModReader/src/rawdatamodel.h \
    ModReader/forms/settings.h \
    ModReader/forms/busmonitor.h \
    ModReader/src/rawdatadelegate.h \
    ModReader/src/registersdatadelegate.h \
    ModReader/src/modbuscommsettings.h \
    ModReader/3rdparty/QsLog/QsLog.h \
    ModReader/3rdparty/QsLog/QsLogDest.h \
    ModReader/3rdparty/QsLog/QsLogDestConsole.h \
    ModReader/3rdparty/QsLog/QsLogLevel.h \
    ModReader/3rdparty/QsLog/QsLogDisableForThisFile.h \
    ModReader/3rdparty/QsLog/QsLogDestFile.h \
    ModReader/src/infobar.h \
    ModReader/forms/tachuswidget.h

INCLUDEPATH += 3rdparty/libmodbus \
    3rdparty/QsLog

TRANSLATIONS += translations/$$TARGET"_zh_CN.ts"
TRANSLATIONS += translations/$$TARGET"_zh_TW.ts"

unix:SOURCES +=

unix:DEFINES += _TTY_POSIX_

win32:SOURCES +=

win32:DEFINES += _TTY_WIN_  WINVER=0x0A00 _WIN32_WINNT=0x0A00

win32:LIBS += -lsetupapi -lwsock32 -lws2_32

QMAKE_CXXFLAGS += -std=gnu++11

DEFINES += QS_LOG_LINE_NUMBERS     # automatically writes the file and line for each log message
#DEFINES += QS_LOG_DISABLE         # logging code is replaced with a no-op
#DEFINES += QS_LOG_SEPARATE_THREAD # messages are queued and written from a separate thread
#DEFINES += LIB_MODBUS_DEBUG_OUTPUT # enable debug output from libmodbus

FORMS    += ModReader/forms/mainwindow.ui \
    ModReader/forms/about.ui \
    ModReader/forms/settingsmodbusrtu.ui \
    ModReader/forms/settingsmodbustcp.ui \
    ModReader/forms/settings.ui \
    ModReader/forms/busmonitor.ui \
    ModReader/forms/tachuswidget.ui

RESOURCES += \
    ModReader/icons/icons.qrc \
    ModReader/translations/translations.qrc


















