#ifndef TA_PLATFORM_PLATFORMSERVICE_H
#define TA_PLATFORM_PLATFORMSERVICE_H

// Tech Aim — the platform boundary.
//
// THIS IS THE ONLY PLACE THAT MAY ASK "WHICH PLATFORM AM I ON".
//
// Domain code (scoring, session storage, replay, recovery, qualification,
// finals, training, analytics) must never contain a platform branch. Where
// behaviour genuinely differs between the Windows desktop product and the
// Android tablet product, it is expressed here as a NAMED CAPABILITY and the
// call site asks for the capability, not for the operating system.
//
// Rationale: `#ifdef Q_OS_ANDROID` scattered through call sites is untestable
// and drifts. A named capability can be read, listed, documented and asserted
// in a test on either platform (see tests/platform/).
//
// Windows behaviour is the reference. Every capability below is TRUE on
// Windows, so the desktop product's behaviour is unchanged by construction —
// a capability only ever subtracts on Android.
//
// QtCore only. No GUI, no QML, no Modbus.

#include <QString>

namespace ta {
namespace platform {

// Which shell this binary was built for. Compile-time, not runtime: a build
// targets exactly one.
enum class Shell {
    WindowsDesktop,
    AndroidTablet
};

Shell currentShell();
QString shellName(Shell s);

// ── Named capabilities ──────────────────────────────────────────────────
// Each answers ONE question a call site actually has. All are true on
// Windows Desktop.

// May the process relaunch itself (QProcess::startDetached on its own
// executable)? Android has no supported way to do this; an operating-mode
// change there must ask the operator to restart the app instead.
bool supportsSelfRelaunch();

// Is a cross-process single-instance lock meaningful? Android guarantees a
// single task instance per package, so the QLockFile dance has nothing to
// defend against and its legacy-lock migration is meaningless.
bool supportsSingleInstanceLock();

// Can QWidget/QDialog be used for a pre-QML startup surface? On Android the
// desktop widget dialogs (frameless + translucent + stylesheet) are not a
// reliable startup surface.
bool supportsDesktopStartupDialogs();

// Are the native QFileDialog save/open pickers usable? On Android they are
// not; export must target an app-owned directory instead.
bool supportsDesktopFileDialogs();

// Does the platform draw its own window chrome that the app should replace
// (frameless hint, custom minimise/maximise/close, Maximized visibility)?
bool supportsDesktopWindowChrome();

// Does QApplication::beep() actually produce an audible cue? It is a no-op on
// Android, so the audio service must not report a beep it did not make.
bool supportsSystemBeep();

// Does the platform's INSTALLER place a config.ini beside the application?
//
// Windows: true. The deployed install ships an operator-editable config.ini
//   next to TechAim.exe. Its ABSENCE is meaningful there — AppSettings falls
//   back to its documented in-code defaults — so the application must never
//   manufacture one and silently change that behaviour.
//
// Android: false. An APK ships no such file and has nowhere to put one, so a
//   first run has to seed the minimum itself (see ensureConfigSeeded).
bool shipsConfigFileWithInstall();

// ── Platform-resolved locations ─────────────────────────────────────────

// Absolute path of the active application configuration file (config.ini).
//
// Windows: unchanged — the caller's name is resolved exactly as QSettings
//   always resolved it (relative to the working directory, i.e. the install
//   directory). Deployed installs keep their operator-editable config.ini
//   next to the executable, and NOTHING about the existing field deployment
//   moves.
//
// Android: the working directory is "/" and is not writable, and the
//   application binary directory is not writable either. The file resolves
//   into the app-private settings directory under
//   QStandardPaths::AppLocalDataLocation, via the existing
//   ta::rel::StoragePaths::settingsDirectory().
QString configFilePath(const QString& configFileName);

// Root directory holding the finals command-cue WAV clips.
//
// Windows: <applicationDirPath>/audio/finals — unchanged.
// Android: "assets:/audio/finals", the Qt Android asset namespace. Clips are
//   packaged into the APK rather than living beside a (non-existent)
//   executable directory. No audio file is duplicated in the repository.
QString finalsAudioClipsRoot();

// Absolute path of a saved match record (.tch).
//
// Windows: unchanged — the caller's relative name is returned untouched, so a
//   match still saves beside the executable exactly where operators and the
//   support bundle already look for it.
//
// Android: the working directory is "/" and the application binary directory
//   is not writable, so a relative name is not merely wrong there - the save
//   silently fails and a completed match is lost. It resolves into an
//   app-private Matches directory instead.
QString matchRecordPath(const QString& fileName);

// Absolute path of the remembered user/port details file.
//
// Windows: unchanged — <applicationDirPath>/<fileName>.
// Android: applicationDirPath() is the native-library directory, which can
//   never be written to. Resolves into the app-private settings directory.
QString userDetailsPath(const QString& fileName);

// Ensure a configuration file exists at `path`, seeding the minimum required
// for a first run on a platform that ships no config.ini.
//
// Deliberately minimal: it writes ONLY the keys without which the app cannot
// make a safe first-run decision. Every other setting keeps the in-code
// default that AppSettings already documents, so seeding can never silently
// change scoring, timing or display behaviour.
//
// Returns true if the file already existed or was created successfully.
bool ensureConfigSeeded(const QString& path);

}} // namespace ta::platform

#endif // TA_PLATFORM_PLATFORMSERVICE_H
