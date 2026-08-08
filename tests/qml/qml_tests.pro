# QML shot-path smoke test — regression cover for QML-SHOT-001.
#
# A console binary, deliberately separate from the C++ harnesses: those link
# QtCore only and can never execute a line of QML, which is exactly how the
# RC2a defect reached the field with every suite green.
#
#   qmake qml_tests.pro && mingw32-make -f Makefile.Release
#   QT_QPA_PLATFORM=offscreen ./release/qml_tests.exe
QT       += core qml
QT       -= gui
CONFIG   += console c++17
CONFIG   -= app_bundle
TEMPLATE  = app
TARGET    = qml_tests

# The test reads the REAL CenterPane.qml rather than a pasted copy, so it can
# never pass against a stale duplicate while the shipped file is broken.
DEFINES += TECHAIM_SOURCE_DIR=\\\"$$PWD/../..\\\"

SOURCES += tst_qml_shot_path.cpp
