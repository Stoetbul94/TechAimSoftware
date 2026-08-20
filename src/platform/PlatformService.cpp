#include "PlatformService.h"

#include "reliability/storage/StoragePaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace ta {
namespace platform {

Shell currentShell()
{
#if defined(Q_OS_ANDROID)
    return Shell::AndroidTablet;
#else
    return Shell::WindowsDesktop;
#endif
}

QString shellName(Shell s)
{
    switch (s) {
    case Shell::WindowsDesktop: return QStringLiteral("Windows Desktop");
    case Shell::AndroidTablet:  return QStringLiteral("Android Tablet");
    }
    return QStringLiteral("Unknown");
}

// ── Capabilities ────────────────────────────────────────────────────────
// Each is a single compile-time answer. They are written as separate
// functions rather than one bitfield so a call site names the reason it
// cares, and so a test can assert each independently.

bool supportsSelfRelaunch()
{
#if defined(Q_OS_ANDROID)
    return false;
#else
    return true;
#endif
}

bool supportsSingleInstanceLock()
{
#if defined(Q_OS_ANDROID)
    return false;
#else
    return true;
#endif
}

bool supportsDesktopStartupDialogs()
{
#if defined(Q_OS_ANDROID)
    return false;
#else
    return true;
#endif
}

bool supportsDesktopFileDialogs()
{
#if defined(Q_OS_ANDROID)
    return false;
#else
    return true;
#endif
}

bool supportsDesktopWindowChrome()
{
#if defined(Q_OS_ANDROID)
    return false;
#else
    return true;
#endif
}

bool supportsSystemBeep()
{
#if defined(Q_OS_ANDROID)
    return false;
#else
    return true;
#endif
}

bool shipsConfigFileWithInstall()
{
#if defined(Q_OS_ANDROID)
    return false;
#else
    return true;
#endif
}

// ── Locations ───────────────────────────────────────────────────────────

QString configFilePath(const QString& configFileName)
{
#if defined(Q_OS_ANDROID)
    // The working directory is "/" and the binary directory is not writable.
    // Reuse the storage layer that already owns app-private data rather than
    // introducing a second notion of "where our files live".
    const QString dir = ta::rel::StoragePaths::settingsDirectory();
    return QDir(dir).filePath(configFileName);
#else
    // Windows: preserve the historical resolution EXACTLY. QSettings resolves
    // a relative name against the working directory, which for a deployed
    // install is the directory holding the executable and the operator's
    // config.ini. Returning the name unchanged keeps that behaviour bit for
    // bit; an absolute name passed in is likewise returned untouched.
    return configFileName;
#endif
}

QString finalsAudioClipsRoot()
{
#if defined(Q_OS_ANDROID)
    // Qt's Android asset namespace. Files packaged under the APK's assets/
    // are reachable here; QFileInfo::exists() and QUrl both understand it.
    return QStringLiteral("assets:/audio/finals");
#else
    return QCoreApplication::applicationDirPath()
           + QStringLiteral("/audio/finals");
#endif
}

bool ensureConfigSeeded(const QString& path)
{
    if (path.isEmpty())
        return false;
    if (QFileInfo::exists(path))
        return true;                    // never touch an existing config

    const QString dirPath = QFileInfo(path).absolutePath();
    if (!dirPath.isEmpty() && !QDir().mkpath(dirPath))
        return false;

    // MINIMUM VIABLE SEED.
    //
    // app_mode is the only key seeded, and it is seeded to Demo. The reason is
    // safety, not convenience: a platform with no implemented target transport
    // must never start in Live and present itself as able to score a real
    // match. AppSettings already falls back to Live when app_mode is
    // missing/invalid, which is the correct desktop default and the wrong
    // Android one — so the value is written explicitly rather than left to
    // that fallback.
    //
    // Nothing else is written. Every other setting keeps the in-code default
    // AppSettings documents, so this seed cannot alter scoring, projectile
    // size, timing, ranges or display behaviour.
    QSettings seed(path, QSettings::IniFormat);
    seed.beginGroup(QStringLiteral("App_Settings"));
    seed.setValue(QStringLiteral("app_mode"), QStringLiteral("Demo"));
    seed.endGroup();
    seed.sync();

    return seed.status() == QSettings::NoError && QFileInfo::exists(path);
}

}} // namespace ta::platform
