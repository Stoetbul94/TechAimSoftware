# Session Reliability Layer (M0 storage + M1 core) — QtCore-only sources.
# Included by Seta.pro and every test .pro that needs the layer (the same
# sharing pattern tests/finals already uses for src/finals). Graduation to a
# real static-library target is deliberately deferred (spec §30 D16).

INCLUDEPATH += $$PWD/src

HEADERS += \
    $$PWD/src/reliability/storage/StoragePaths.h \
    $$PWD/src/reliability/storage/StorageSync.h \
    $$PWD/src/reliability/core/ReliabilityError.h \
    $$PWD/src/reliability/core/ReliabilityResult.h \
    $$PWD/src/reliability/core/FixedPoint.h \
    $$PWD/src/reliability/events/EventTypes.h \
    $$PWD/src/reliability/events/DomainEvent.h \
    $$PWD/src/reliability/events/EventEnvelope.h \
    $$PWD/src/reliability/events/EventSerializer.h \
    $$PWD/src/reliability/events/EventRegistry.h \
    $$PWD/src/reliability/journal/HashChain.h \
    $$PWD/src/reliability/journal/JournalWriter.h \
    $$PWD/src/reliability/journal/JournalReader.h \
    $$PWD/src/reliability/journal/JournalValidator.h \
    $$PWD/src/reliability/reducer/SessionState.h \
    $$PWD/src/reliability/reducer/SessionReducer.h \
    $$PWD/src/reliability/store/MonotonicClock.h \
    $$PWD/src/reliability/store/JournalManager.h \
    $$PWD/src/reliability/store/PersistenceRetryQueue.h \
    $$PWD/src/reliability/store/SessionStore.h \
    $$PWD/src/reliability/replay/ReplayEngine.h \
    $$PWD/src/reliability/recovery/RecoveryTypes.h \
    $$PWD/src/reliability/recovery/RecoveryCoordinator.h

SOURCES += \
    $$PWD/src/reliability/storage/StoragePaths.cpp \
    $$PWD/src/reliability/storage/StorageSync.cpp \
    $$PWD/src/reliability/events/EventSerializer.cpp \
    $$PWD/src/reliability/events/EventRegistry.cpp \
    $$PWD/src/reliability/journal/HashChain.cpp \
    $$PWD/src/reliability/journal/JournalWriter.cpp \
    $$PWD/src/reliability/journal/JournalReader.cpp \
    $$PWD/src/reliability/journal/JournalValidator.cpp \
    $$PWD/src/reliability/reducer/SessionState.cpp \
    $$PWD/src/reliability/reducer/SessionReducer.cpp \
    $$PWD/src/reliability/store/JournalManager.cpp \
    $$PWD/src/reliability/store/PersistenceRetryQueue.cpp \
    $$PWD/src/reliability/store/SessionStore.cpp \
    $$PWD/src/reliability/replay/ReplayEngine.cpp \
    $$PWD/src/reliability/recovery/RecoveryCoordinator.cpp
