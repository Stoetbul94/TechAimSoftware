# Platform boundary (Android tablet milestone A1/A2) — QtCore-only sources.
#
# The ONE place that may ask which platform the binary targets. Included by
# Seta.pro and by any test .pro that needs the seam, following the same
# sharing pattern as Reliability.pri.
#
# Depends on Reliability.pri for ta::rel::StoragePaths — include that first.

INCLUDEPATH += $$PWD/src

HEADERS += $$PWD/src/platform/PlatformService.h
SOURCES += $$PWD/src/platform/PlatformService.cpp

# QML bridge. Header-only (inline accessors), but it declares Q_OBJECT so it
# must be listed in HEADERS for moc to see it. Kept out of the QtCore-only
# PlatformService so a console test binary can use the seam without QML.
HEADERS += $$PWD/src/platform/PlatformBridge.h
