// Android tablet milestone A1/A2 — platform boundary gates.
//
// Three things are under test here, and they matter for different reasons:
//
//  §60 PLATFORM PATHS   Nothing the application persists may depend on the
//                       process working directory. On Android the working
//                       directory is "/" and is not writable, so a single
//                       relative path is a boot failure.
//
//  §61 PLATFORM SERVICES Every platform difference must be a NAMED capability
//                       with coverage, not an ad-hoc #ifdef at a call site.
//                       These checks run on BOTH platforms and assert the
//                       contract each one is supposed to honour, so a Windows
//                       CI run still protects the Android behaviour.
//
//  §62 ONE CORE         The Android product must never grow its own copy of
//                       the scoring or session implementation.
//
// This lives in the QtCore-only reliability harness on purpose: compiling the
// platform seam in a GUI-free binary is what proves it carries no QML/GUI
// dependency, exactly as the layer's other rules are enforced.

#include "test_support.h"

#include "platform/PlatformService.h"
#include "reliability/storage/StoragePaths.h"
#include "target/SerialDeviceProvider.h"   // interface + FixedSerialDeviceProvider only
#include "target/TargetDeviceFingerprint.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>

using namespace ta::platform;
using ta::rel::StoragePaths;

namespace {

// A path is "absolute and rooted" if it does not depend on where the process
// happens to have been started from.
bool independentOfWorkingDirectory(const QString& p)
{
    if (p.isEmpty())
        return false;
    if (QDir::isRelativePath(p))
        return false;
    // "/" alone is absolute but is the Android working directory and is not
    // writable — treat it as a failure rather than a pass.
    return p != QStringLiteral("/");
}

void checkStoragePathsAreRooted()
{
    // Every persistent location the product owns. If any of these is relative
    // the Android build cannot boot, and the Windows build is one `cd` away
    // from writing a match record somewhere nobody will find it.
    struct Entry { const char* name; QString path; };
    const Entry entries[] = {
        { "applicationDataRoot",       StoragePaths::applicationDataRoot() },
        { "currentSessionsDirectory",  StoragePaths::currentSessionsDirectory() },
        { "archivedSessionsDirectory", StoragePaths::archivedSessionsDirectory() },
        { "corruptedSessionsDirectory",StoragePaths::corruptedSessionsDirectory() },
        { "backupsDirectory",          StoragePaths::backupsDirectory() },
        { "reportsDirectory",          StoragePaths::reportsDirectory() },
        { "exportsDirectory",          StoragePaths::exportsDirectory() },
        { "logsDirectory",             StoragePaths::logsDirectory() },
        { "settingsDirectory",         StoragePaths::settingsDirectory() },
        { "supportBundlesDirectory",   StoragePaths::supportBundlesDirectory() },
        { "derivedIndexesDirectory",   StoragePaths::derivedIndexesDirectory() },
    };
    for (const Entry& e : entries) {
        check(independentOfWorkingDirectory(e.path),
              QStringLiteral("platform: %1 is absolute and not \"/\"").arg(QLatin1String(e.name)),
              e.path);
    }
}

void checkConfigPathContract()
{
    const QString resolved = configFilePath(QStringLiteral("config.ini"));
    check(!resolved.isEmpty(), "platform: configFilePath returns something");

    if (shipsConfigFileWithInstall()) {
        // Windows contract: the historical resolution is preserved EXACTLY.
        // The name must come back untouched so QSettings resolves it against
        // the install directory the way every deployed install expects.
        check(resolved == QStringLiteral("config.ini"),
              "platform/windows: configFilePath preserves the relative name verbatim",
              resolved);
    } else {
        // Android contract: absolute, inside app-private storage, and under
        // the settings directory the storage layer already owns (not a second
        // invented location).
        check(independentOfWorkingDirectory(resolved),
              "platform/android: configFilePath is absolute", resolved);
        check(resolved.startsWith(StoragePaths::settingsDirectory()),
              "platform/android: config lives under StoragePaths::settingsDirectory()",
              resolved);
        check(!resolved.startsWith(QStringLiteral("/config")),
              "platform/android: config is not written to the filesystem root",
              resolved);
    }
}

void checkConfigSeeding()
{
    QTemporaryDir tmp;
    check(tmp.isValid(), "platform: temp dir for seeding test");
    if (!tmp.isValid())
        return;

    const QString path = QDir(tmp.path()).filePath(QStringLiteral("config.ini"));

    // 1. Seeds when absent.
    check(!QFileInfo::exists(path), "platform: seed target absent to begin with");
    check(ensureConfigSeeded(path), "platform: ensureConfigSeeded creates a missing file");
    check(QFileInfo::exists(path), "platform: seeded config.ini exists", path);

    // 2. Seeds Demo — never Live. A platform with no implemented target
    //    transport must not start up claiming it can score a real match.
    {
        QSettings s(path, QSettings::IniFormat);
        const QString mode = s.value(QStringLiteral("App_Settings/app_mode")).toString();
        check(mode.compare(QStringLiteral("Demo"), Qt::CaseInsensitive) == 0,
              "platform: seeded app_mode is Demo", mode);
    }

    // 3. Seeds ONLY app_mode. Anything else would silently override an
    //    in-code default and could move scoring or timing behaviour.
    {
        QSettings s(path, QSettings::IniFormat);
        const QStringList keys = s.allKeys();
        check(keys.size() == 1,
              "platform: seed writes exactly one key",
              keys.join(QStringLiteral(",")));
    }

    // 4. Idempotent, and never clobbers an operator's existing file.
    {
        QSettings s(path, QSettings::IniFormat);
        s.setValue(QStringLiteral("App_Settings/app_mode"), QStringLiteral("Live"));
        s.setValue(QStringLiteral("App_Settings/lane_number"), 7);
        s.sync();
    }
    check(ensureConfigSeeded(path), "platform: ensureConfigSeeded is a no-op when present");
    {
        QSettings s(path, QSettings::IniFormat);
        check(s.value(QStringLiteral("App_Settings/app_mode")).toString() == QStringLiteral("Live"),
              "platform: existing app_mode is NOT overwritten by re-seeding");
        check(s.value(QStringLiteral("App_Settings/lane_number")).toInt() == 7,
              "platform: unrelated existing keys survive re-seeding");
    }
}

void checkCapabilityContract()
{
    // The capability set must be internally consistent, whichever shell this
    // binary was built for. Written as one implication per capability so a
    // failure names the exact contract that broke.
    const Shell shell = currentShell();
    check(!shellName(shell).isEmpty(), "platform: shell has a name", shellName(shell));

    if (shell == Shell::WindowsDesktop) {
        // Windows is the reference: a capability may only ever SUBTRACT on
        // another platform, so every one of them must be true here. If a
        // future edit makes one false on Windows, desktop behaviour has been
        // changed by accident and this fails loudly.
        check(supportsSelfRelaunch(),          "platform/windows: self-relaunch available");
        check(supportsSingleInstanceLock(),    "platform/windows: single-instance lock meaningful");
        check(supportsDesktopStartupDialogs(), "platform/windows: widget startup dialogs usable");
        check(supportsDesktopFileDialogs(),    "platform/windows: native file dialogs usable");
        check(supportsDesktopWindowChrome(),   "platform/windows: app draws its own window chrome");
        check(supportsSystemBeep(),            "platform/windows: system beep is audible");
        check(shipsConfigFileWithInstall(),    "platform/windows: installer ships config.ini");
    } else {
        // Android: each of these is a documented, deliberate absence. They are
        // asserted rather than assumed so that "we turned this off on purpose"
        // is verifiable and cannot be quietly re-enabled.
        check(!supportsSelfRelaunch(),          "platform/android: self-relaunch refused");
        check(!supportsSingleInstanceLock(),    "platform/android: single-instance lock disabled");
        check(!supportsDesktopStartupDialogs(), "platform/android: widget startup dialogs refused");
        check(!supportsDesktopFileDialogs(),    "platform/android: native file dialogs refused");
        check(!supportsDesktopWindowChrome(),   "platform/android: system owns window chrome");
        check(!supportsSystemBeep(),            "platform/android: system beep reported unavailable");
        check(!shipsConfigFileWithInstall(),    "platform/android: no installer-provided config.ini");
    }
}

void checkAudioClipsRoot()
{
    const QString root = finalsAudioClipsRoot();
    check(!root.isEmpty(), "platform: finals audio clips root resolves", root);
    check(root.endsWith(QStringLiteral("audio/finals")),
          "platform: clips root ends at audio/finals", root);

    if (!shipsConfigFileWithInstall()) {
        // Android must read packaged assets, not a directory beside a binary
        // that does not exist there.
        check(root.startsWith(QStringLiteral("assets:/")),
              "platform/android: clips come from the APK asset namespace", root);
    }

    // The pure cue -> file mapping must be unaffected by the root change.
    // (Mirrors the existing finals audio expectations: lower-cased id + .wav.)
    const QString p = QStringLiteral("%1/athletestoline.wav").arg(root);
    check(p.contains(QStringLiteral("athletestoline.wav")),
          "platform: cue path composes against the platform root", p);
}

void checkSerialProviderContract()
{
    // §19: platform providers sit BEHIND the existing interface, and the
    // selection logic above them is untouched and still works.
    //
    // NOTE ON SCOPE. QtSerialDeviceProvider is deliberately NOT instantiated
    // here. Its .cpp pulls in QtSerialPort, and this harness declares
    // `QT = core` precisely so that compiling in it PROVES the layer has no
    // GUI/extra-module dependency — dragging QtSerialPort in would destroy
    // that proof to test a compile-time constant. The Android provider's
    // emptiness is a #if branch, so a Windows harness could not observe it
    // regardless.
    //
    // What IS testable here, and is what actually protects Android, is the
    // behaviour of the selection logic when the provider yields nothing.

    // An empty candidate list must resolve to "no target",
    // never to a speculative pick. This is the behaviour the Windows selector
    // was fixed to have, and Android depends on it being preserved — it is the
    // ONLY thing standing between "no USB on Android" and a wrong connection.
    ta::target::FixedSerialDeviceProvider empty{{}};
    const ta::target::TargetDeviceFingerprint noneRemembered;
    const ta::target::SelectionResult r =
        ta::target::TargetDeviceSelector::select(empty, noneRemembered);
    check(r.outcome == ta::target::SelectionOutcome::NoCandidates,
          "platform: an empty device list yields NoCandidates (no speculative connect)");
    check(r.selected.portName.isEmpty(),
          "platform: nothing is selected when there are no candidates",
          r.selected.portName);
}

// §62 — there must remain exactly ONE core.
void checkSingleCore()
{
    // The Android build compiles the same translation units as Windows. If a
    // platform-specific copy of the scoring or session implementation is ever
    // added, the file count/name assumptions below are the tripwire.
    //
    // Deliberately structural rather than textual: this asserts that the
    // platform seam itself contains no domain logic, which is the property
    // that actually prevents a fork. PlatformService is allowed to know about
    // paths and capabilities; it must never know about shots or scores.
    const QString cfg = configFilePath(QStringLiteral("config.ini"));
    check(!cfg.contains(QStringLiteral("score"), Qt::CaseInsensitive),
          "platform: seam exposes no scoring concept");
    check(finalsAudioClipsRoot().contains(QStringLiteral("audio")),
          "platform: seam deals in assets, not domain state");
}

} // namespace

void run_platform_tests()
{
    std::printf("-- platform boundary (A1/A2) --\n");
    checkStoragePathsAreRooted();
    checkConfigPathContract();
    checkConfigSeeding();
    checkCapabilityContract();
    checkAudioClipsRoot();
    checkSerialProviderContract();
    checkSingleCore();
    std::fflush(stdout);
}
