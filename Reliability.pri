# Session Reliability Layer (M0) — QtCore-only sources.
# Included by Seta.pro and every test .pro that needs the layer (the same
# sharing pattern tests/finals already uses for src/finals). Graduation to a
# real static-library target is deliberately deferred (spec §30 D16).

INCLUDEPATH += $$PWD/src

HEADERS += \
    $$PWD/src/reliability/storage/StoragePaths.h \
    $$PWD/src/reliability/storage/StorageSync.h

SOURCES += \
    $$PWD/src/reliability/storage/StoragePaths.cpp \
    $$PWD/src/reliability/storage/StorageSync.cpp
