QT += charts qml quick printsupport widgets xml

CONFIG += c++17
#QMAKE_CXXFLAGS += /std:c++17

# ── P0 Phase C: product identity for the Windows version resource ────────
# TARGET fixes the executable name to TechAim.exe regardless of the project
# filename. Keep these in step with src/app/ProductIdentity.cpp — that is the
# authoritative source; these are the values qmake bakes into the PE
# VERSIONINFO block, which qmake cannot read from C++.
# The SETA product line builds SETA.exe. BRAND_SETA (below) is the one
# compile-time authority: it selects the identity, the BrandPackage and the
# version resource. There is no runtime brand switch and no Tech Aim product
# identity compiled into a SETA binary.
TARGET = SETA
VERSION = 1.0.0
# The version resource is hand-authored (TechAim.rc) rather than generated
# from QMAKE_TARGET_*: qmake leaves InternalName empty and marks the binary
# VFT_DLL. RC_FILE takes precedence over the QMAKE_TARGET_* variables.
win32: RC_FILE = TechAim.rc

# Release-clean: a previously built Seta.exe sits in the SAME output folder
# and stays launchable after the rename, so an operator (or a stale shortcut)
# could run an outdated binary that looks like the product. Delete known
# legacy executables from the output directory on every successful link.
# Resolve the output directory at qmake time: the generated make DESTDIR
# carries a trailing comment, so $(DESTDIR) would expand to "release/ " and
# delete the directory rather than the file. shell_path() picks the right
# separators for whichever shell qmake configured, and both rm -f and del
# exit 0 when the legacy file is already absent.
CONFIG(release, debug|release): LEGACY_OUT_DIR = $$OUT_PWD/release
else:                           LEGACY_OUT_DIR = $$OUT_PWD/debug
# "Seta.exe" is NO LONGER a legacy name: this product links SETA.exe and
# NTFS is case-insensitive, so deleting "Seta.exe" after a link deletes the
# binary that was just built. A name belonging to a CURRENT product cannot
# also be treated as stale.
LEGACY_EXES = Seeds.exe
for(legacy, LEGACY_EXES) {
    QMAKE_POST_LINK += $$quote($(DEL_FILE) $$shell_quote($$shell_path($$LEGACY_OUT_DIR/$$legacy)))$$escape_expand(\\n\\t)
}

# F9B: build identity embedded at COMPILE time (no runtime git). qmake runs
# git once at build-configuration time; the app never shells out to git and
# the customer machine needs no Git / repo / Qt Creator. Build date/time comes
# from the compiler (__DATE__/__TIME__ in main.cpp).
# 0.9.0-RC2 — INTERNAL FIELD TEST. This is a release candidate for controlled
# live-range testing, not the public 1.0. The channel travels with the version
# so a binary can never be mistaken for a general release.
# 1.0.0-EVAL1: the SETA product carrying the Tech Aim v1.0 proven core. It is
# an EVALUATION build - no SETA-branded binary has ever been fired at a target -
# and the release channel and field-test notice say so on screen.
# 1.0.0-EVAL2. EVAL1 was built, packaged and inspected but never released: the
# packaged build had no support collector, and the collector itself searched one
# directory level above the data root and returned 0 journals every time. Both
# are fixed. No product source changed between the two - the difference is the
# support tooling and the commit baked into the binary - but a package that goes
# out must have ONE identity, so this is a new one rather than EVAL1's hashes
# quietly replaced.
APP_VERSION_STR = 1.0.0-EVAL2
GIT_SHA = $$system(git -C \"$$PWD\" rev-parse --short HEAD)
isEmpty(GIT_SHA): GIT_SHA = unknown
DEFINES += APP_VERSION_STR=\\\"$$APP_VERSION_STR\\\"
DEFINES += APP_GIT_SHA=\\\"$$GIT_SHA\\\"
CONFIG(release, debug|release): DEFINES += APP_BUILD_CONFIG=\\\"Release\\\"
else: DEFINES += APP_BUILD_CONFIG=\\\"Debug\\\"

# ── SETA PRODUCT LINE ────────────────────────────────────────────────────────
# This branch (product/seta) IS the SETA product, so the flavour is enabled
# here rather than passed on a command line: a build of this tree must not be
# able to come out unbranded. It selects the SETA name, the existing approved
# seta.png mark and - importantly - a SEPARATE user-data namespace, so a SETA
# install never reads or writes a Tech Aim install's athletes, sessions,
# recovery journals, reports, logs or target fingerprints. The legal publisher
# is NOT changed by it. See src/app/ProductIdentity.cpp.
#
# The test harnesses define this too (tests/*/*.pro): a gate that measures the
# Tech Aim identity would not be measuring the build this branch ships.
DEFINES += BRAND_SETA
# The resource compiler needs the same flavour switch: the Windows icon and
# the two user-visible Properties strings live in TechAim.rc, and a build
# whose C++ said SETA while its resources said Tech Aim would ship the wrong
# emblem on the executable.
win32: RC_DEFINES += BRAND_SETA
#QMAKE_TARGET_PRODUCT = "TACHUS CPU"

SOURCES += main.cpp \
    customprint.cpp \
    appsettings.cpp \
    logfile.cpp \
    receiverTachus.cpp \
    sender.cpp \
    src/analytics/CoachAnalyticsEngine.cpp \
    src/bridge/tachusshotbuilder.cpp \
    src/bridge/coachreportvariant.cpp \
    src/bridge/coachreportbridge.cpp \
    src/bridge/coachreportfeeder.cpp \
    src/bridge/pdfexporter.cpp \
    src/finals/Finals3PController.cpp \
    src/finals/FinalsReportBuilder.cpp \
    src/finals/FinalsAudioService.cpp \
    src/finals10m/Finals10mController.cpp \
    src/qualification/QualificationController.cpp \
    src/incident/EstIncidentController.cpp \
    src/app/ProductIdentity.cpp \
    src/app/BrandPackage.cpp \
    src/app/LanguageService.cpp \
    src/app/DocumentationCapture.cpp \
    src/mode/OperatingModeService.cpp \
    src/training/TrainingProgramController.cpp \
    src/training/TrainingBlockMetrics.cpp \
    src/training/CallDiagnoseController.cpp \
    src/training/CallDiagnoseAnalytics.cpp \
    src/training/GroupPatternAnalyzer.cpp \
    src/training/PositionTransitionController.cpp \
    src/training/WindMapController.cpp \
    src/training/WindMapAnalytics.cpp \
    src/training/WindMapVerdict.cpp \
    src/target/SerialDeviceProvider.cpp \
    src/target/TargetDeviceFingerprint.cpp \
    src/target/PaperFeedCoordinator.cpp

# Offline coach-analytics module (pure C++, independent from Qt/QML).
HEADERS += \
    src/analytics/ShotAnalyticsTypes.h \
    src/analytics/CoachReportData.h \
    src/analytics/CoachAnalyticsEngine.h
INCLUDEPATH += src/analytics

# QML boundary layer (the ONLY place Qt meets the analytics engine).
# tachusshotbuilder is pure C++ (Qt-free); the rest bridge to Qt/QML.
HEADERS += \
    src/bridge/tachusshotbuilder.h \
    src/bridge/coachreportvariant.h \
    src/bridge/coachreportbridge.h \
    src/bridge/coachreportfeeder.h \
    src/bridge/pdfexporter.h
INCLUDEPATH += src/bridge

# 3P FINAL — dedicated finals domain (ISSF Rule Book 2026 Edition 2025,
# Second Print 07/2026). Separate from qualification; see
# docs/3p-finals-discipline.md.
HEADERS += \
    src/finals/Finals3PTypes.h \
    src/finals/Finals3PConfig.h \
    src/finals/Finals3PController.h \
    src/finals/FinalsReportData.h \
    src/finals/FinalsReportBuilder.h \
    src/finals/FinalsAudioService.h \
    src/qualification/QualificationController.h \
    src/incident/EstIncidentController.h \
    src/app/ProductIdentity.h \
    src/app/BrandPackage.h \
    src/app/ProductIdentityBridge.h \
    src/app/LanguageService.h \
    src/app/DocumentationCapture.h \
    src/mode/OperatingMode.h \
    src/mode/OperatingModeService.h

# 10m Air Rifle / Air Pistol FINAL — single-athlete training course (F1).
# ISSF Rule Book Edition 2025 (Second Print 07/2026), rule 6.17.2. Separate
# from the 3P Final; see docs/issf-rules/10m-finals-shared.md and
# docs/10m-finals-architecture.md.
HEADERS += \
    src/finals10m/Finals10mTypes.h \
    src/finals10m/Finals10mConfig.h \
    src/finals10m/Finals10mController.h

# Training Lab (T1) — separate domain from competition (docs/
# training-lab-architecture.md). Technical Blocks vertical slice.
HEADERS += \
    src/training/TrainingProgramTypes.h \
    src/training/TrainingBlockMetrics.h \
    src/training/TrainingProgramController.h \
    src/training/CallDiagnoseTypes.h \
    src/training/CallDiagnoseAnalytics.h \
    src/training/CallDiagnoseController.h \
    src/training/TargetGeometry.h \
    src/training/GroupPatternAnalyzer.h \
    src/training/PositionTransitionTypes.h \
    src/training/PositionTransitionController.h \
    src/training/WindMapTypes.h \
    src/training/WindMapController.h \
    src/training/WindMapAnalytics.h \
    src/training/WindMapVerdict.h

# RC2 target hardware: adapter identity, candidate filtering and the single
# automatic paper-feed authority. Pure logic, unit-tested without hardware.
HEADERS +=     src/target/SerialDeviceProvider.h     src/target/TargetDeviceFingerprint.h     src/target/PaperFeedCoordinator.h
INCLUDEPATH += src
INCLUDEPATH += src/training
INCLUDEPATH += src/finals
INCLUDEPATH += src/finals10m
INCLUDEPATH += src/qualification
INCLUDEPATH += src/incident
INCLUDEPATH += src/mode

# Session Reliability Layer (M0) - QtCore-only storage foundation.
include(Reliability.pri)
# QSoundEffect for the finals audio cues (FinalsAudioService).
QT += multimedia

# P0 Phase F: compiled .qm catalogues ship INSIDE the binary, so a deployed
# install cannot lose its translations to a missing file on disk.
RESOURCES += qml.qrc \
    images.qrc \
    techaim_translations.qrc

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

# P0 Phase F. English is the SOURCE language and needs no catalogue.
# The legacy german/french/italain/spanish/chinese .ts files were
# Tachus-era stubs covering only the vendored QModMaster forms (every entry
# "unfinished"); they are not product catalogues and are not built.
TRANSLATIONS += \
    translations/techaim_de_DE.ts

lupdate_only{
SOURCES = CallDiagnoseHud.qml 
        CallDiagnoseReportView.qml 
        CallDiagnoseRightPanel.qml 
        CenterPane.qml 
        CoachDashboardView.qml 
        CoachDetailedView.qml 
        CoachPrintView.qml 
        CoachReportCard.qml 
        CoachReportWindow.qml 
        ConnectionError.qml 
        Finals10mCommandPanel.qml 
        Finals10mHud.qml 
        Finals10mRightPanel.qml 
        FinalsAdvanceControl.qml 
        FinalsCommandOverlay.qml 
        FinalsDeveloperDrawer.qml 
        FinalsHud.qml 
        FinalsIncidentToast.qml 
        FinalsPerformanceBlock.qml 
        FinalsProgressIndicator.qml 
        FinalsReportTarget.qml 
        FinalsReportView.qml 
        FinalsTopStrip.qml 
        FloatingWindow.qml 
        FloatingWindowGrip.qml 
        Header.qml 
        HeatMapCanvas.qml 
        IncidentWindow.qml 
        IssfTargetCanvas.qml 
        LeftPanel.qml 
        LoginPage.qml 
        MatchReportInfo.qml 
        MatchReportView.qml 
        MetricCard.qml 
        MetricChip.qml 
        ModConnectorDialog.qml 
        PdfPage.qml 
        PdfSeriesPage.qml 
        PersistenceBanner.qml 
        PositionTransitionHud.qml 
        PositionTransitionReportView.qml 
        PositionTransitionRightPanel.qml 
        RecoveryDialog.qml 
        Report3P.qml 
        Report3PSeries.qml 
        ReportFooter.qml 
        ReportHeader.qml 
        ReportWindow.qml 
        RightPanel.qml 
        SectionTitle.qml 
        SeriesCard.qml 
        SeriesComponent.qml 
        SetaCompetitionSelector.qml 
        CompetitionCatalogue.qml 
        SettingsPage.qml 
        ShootingPage.qml 
        ShotTargetCanvas.qml 
        StatChip.qml 
        SummaryReportView.qml 
        TechAimDialog.qml 
        TechAimDialogManager.qml 
        Theme.qml 
        TrainingHud.qml 
        TrainingReportView.qml 
        TrainingRightPanel.qml 
        VIcon.qml 
        WindowManager.qml 
        main.qml
}

#INCLUDEPATH += "C:/Program Files (x86)/Windows Kits/10/Include/10.0.17763.0/ucrt"
##LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/ucrt/x64"
#LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/ucrt/x86"



# -- DSB 2026 - DEFERRED FOR 1.0.0-EVAL1 (SETA-DSB-PORT-001) ---------------
# The DSB sources are PRESENT and unmodified - src/dsb/Dsb120Controller.{h,cpp},
# Dsb120Hud.qml, SetaCompetitionSelector.qml, the rule documents and
# tst_dsb120.cpp. They are excluded from the BUILD, not deleted.
#
# The reason is honesty, not tidiness: the shared screens they hooked into
# came forward from the Tech Aim v1.0 baseline, so the integration points DSB
# relied on are no longer in ShootingPage/LoginPage. Compiling a controller
# that nothing mounts would put half-wired competition code in an evaluation
# binary. The complete pre-port integration is recoverable from f22637f.
#
# CONFIG+=dsb re-enables the sources for the eventual controlled port. It is
# NOT a supported configuration and EVAL1 must not be built with it.
dsb {
    SOURCES += src/dsb/Dsb120Controller.cpp
    HEADERS += src/dsb/Dsb120Controller.h
}