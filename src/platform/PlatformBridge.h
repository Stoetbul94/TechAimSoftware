#ifndef TA_PLATFORM_PLATFORMBRIDGE_H
#define TA_PLATFORM_PLATFORMBRIDGE_H

// QML face of the platform boundary. Exposed as the PLATFORM context property.
//
// WHY A CONTEXT PROPERTY AND NOT A THEME FIELD.
// Theme.qml is instantiated once in main.qml and reached by ANCESTOR SCOPE
// (`theme.xxx`). That works for descendants of the root window, but a file
// loaded outside that chain resolves `theme` to undefined and — this is the
// dangerous part — an unqualified QML lookup that fails does so SILENTLY,
// leaving an empty font family and a surprised reader. A context property is
// registered on the engine's root context and is therefore in scope in every
// QML file this engine loads, unconditionally. Fonts are needed in report
// views, floating windows and PDF-grab surfaces alike, so the safe scope is
// the only acceptable one.
//
// Read-only and CONSTANT: these are compile-time platform facts, not user
// settings. Nothing here may become writable without a design decision — a
// runtime-switchable font would invalidate the PDF layout evidence.

#include <QObject>
#include <QString>
#include "PlatformService.h"

class PlatformBridge : public QObject
{
    Q_OBJECT

    // Which shell this binary targets, for diagnostics and About screens.
    Q_PROPERTY(QString shellName READ shellName CONSTANT)
    Q_PROPERTY(bool isAndroid READ isAndroid CONSTANT)

    // True when the app must draw its own desktop window chrome (frameless
    // hint, custom minimise / maximise / close, Maximized visibility).
    // False on Android, where the system owns the window and the content
    // fills the available area.
    Q_PROPERTY(bool desktopWindowChrome READ desktopWindowChrome CONSTANT)

    // True where a native file picker exists. QML uses it to mark export and
    // import actions as unavailable rather than letting them silently no-op.
    Q_PROPERTY(bool desktopFileDialogs READ desktopFileDialogs CONSTANT)

    // ── Font tokens ─────────────────────────────────────────────────────
    // Replaces hardcoded "Segoe UI" / "Consolas" literals across the QML.
    //
    // Windows resolves to exactly the historical families, so the desktop UI
    // and every existing PDF/report layout are pixel-identical to before.
    //
    // Android resolves to families guaranteed present on the platform. NO
    // Microsoft font file is added to the repository or shipped in the APK —
    // "Segoe UI" and "Consolas" are proprietary and are not ours to
    // redistribute. Android substitutes anyway; naming the substitute makes
    // the metrics predictable instead of accidental.
    Q_PROPERTY(QString uiFont READ uiFont CONSTANT)
    Q_PROPERTY(QString monoFont READ monoFont CONSTANT)

    // Minimum comfortable touch target. 0 on desktop (callers keep their own
    // sizing); a real floor on Android. Callers apply it as a lower bound,
    // never as a fixed size, so desktop layouts are untouched.
    Q_PROPERTY(int minTouchTarget READ minTouchTarget CONSTANT)

public:
    explicit PlatformBridge(QObject* parent = nullptr) : QObject(parent) {}

    QString shellName() const
    { return ta::platform::shellName(ta::platform::currentShell()); }

    bool isAndroid() const
    { return ta::platform::currentShell() == ta::platform::Shell::AndroidTablet; }

    bool desktopWindowChrome() const
    { return ta::platform::supportsDesktopWindowChrome(); }

    bool desktopFileDialogs() const
    { return ta::platform::supportsDesktopFileDialogs(); }

    QString uiFont() const
    {
#if defined(Q_OS_ANDROID)
        // Roboto is the Android system UI face and is present on every
        // supported device.
        return QStringLiteral("Roboto");
#else
        return QStringLiteral("Segoe UI");
#endif
    }

    QString monoFont() const
    {
#if defined(Q_OS_ANDROID)
        // "monospace" is the Android family alias; the platform maps it to
        // whichever fixed-pitch face the device actually ships. Naming a
        // concrete face instead risks a silent fallback with different
        // metrics, which is precisely what the fixed-width report columns
        // cannot tolerate.
        return QStringLiteral("monospace");
#else
        return QStringLiteral("Consolas");
#endif
    }

    int minTouchTarget() const
    {
#if defined(Q_OS_ANDROID)
        // 48dp is the Android accessibility floor for an interactive target.
        return 48;
#else
        return 0;
#endif
    }
};

#endif // TA_PLATFORM_PLATFORMBRIDGE_H
