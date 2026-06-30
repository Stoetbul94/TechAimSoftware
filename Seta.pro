QT += charts qml quick printsupport widgets xml

CONFIG += c++11
#QMAKE_CXXFLAGS += /std:c++17

VERSION = 4.0
QMAKE_TARGET_PRODUCT = "SETA"
#QMAKE_TARGET_PRODUCT = "TACHUS CPU"

SOURCES += main.cpp \
    customprint.cpp \
    appsettings.cpp \
    logfile.cpp \
    receiverTachus.cpp \
    sender.cpp

RESOURCES += qml.qrc \
    images.qrc \
    translations.qrc

DISTFILES += \
    images/loginPage/combo_down.png \
    qml/qmlpolarchart/*

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    customprint.h \
    appsettings.h \
    defines.h \
    logfile.h \
    receiverTachus.h \
    sender.h

#SUBDIRS += \
#    ModReader/qModMaster.pro

include(ModReader/qModMaster.pro)

TRANSLATIONS += \
    translations/german.ts \
    translations/italain.ts \
    translations/french.ts \
    translations/spanish.ts \
    translations/chinese.ts

lupdate_only{
SOURCES = main.qml \
        CenterPane.qml \
        ClosePopupDialog.qml \
        Header.qml \
        LeftPanel.qml \
        LoginPage.qml \
        MatchReport.qml \
        MatchReportInfo.qml \
        ModConnectorDialog.qml \
        Page1.qml \
        Page1Form.ui.qml \
        PdfPage.qml \
        PdfSeriesPage.qml \
        RightPanel.qml \
        SeriesComponent.qml \
        SettingsPage.qml \
        ShootingPage.qml \
        SummaryPage.qml
}

#INCLUDEPATH += "C:/Program Files (x86)/Windows Kits/10/Include/10.0.17763.0/ucrt"
##LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/ucrt/x64"
#LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/ucrt/x86"

