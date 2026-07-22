#ifndef TA_MODE_OPERATINGMODESERVICE_H
#define TA_MODE_OPERATINGMODESERVICE_H

// F10 — runtime operating-mode authority + Settings/restart API.
//
// Wraps the pure policy in OperatingMode.h with the runtime concerns: the
// resolved config path, the currently RUNNING mode (fixed for the process
// lifetime — mode only changes via restart), the operator's pending SELECTED
// mode, and the atomic config write. Exposed to QML as the OPMODE context
// property. The physical restart is performed by main.cpp (needs QApplication);
// this service only writes the setting and emits restartRequested().

#include <QObject>
#include <QString>
#include "OperatingMode.h"

class OperatingModeService : public QObject
{
    Q_OBJECT
    // Running mode — what this process launched into. Never changes at runtime.
    Q_PROPERTY(bool live READ isLive CONSTANT)
    Q_PROPERTY(QString runningModeToken READ runningModeToken CONSTANT)
    Q_PROPERTY(QString badgeText READ badgeText CONSTANT)
    Q_PROPERTY(bool configValid READ configValid CONSTANT)
    // Operator selection (Settings). 0 = Live, 1 = Demo.
    Q_PROPERTY(int selectedMode READ selectedMode NOTIFY selectionChanged)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY selectionChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged)

public:
    explicit OperatingModeService(QString configPath,
                                  ta::mode::Mode running,
                                  bool configValid,
                                  QString rawValue,
                                  QObject* parent = nullptr);

    ta::mode::Mode runningMode() const { return m_running; }
    bool isLive() const { return m_running == ta::mode::Mode::Live; }
    QString runningModeToken() const { return ta::mode::modeToConfigString(m_running); }
    QString badgeText() const { return ta::mode::modeBadgeText(m_running); }
    bool configValid() const { return m_configValid; }
    int selectedMode() const { return static_cast<int>(m_selected); }
    bool restartRequired() const { return m_restartRequired; }
    QString lastError() const { return m_lastError; }
    QString configPath() const { return m_configPath; }

    // ── authoritative input-source gate ─────────────────────────────────
    // shotSource: 0 = Physical, 1 = Simulated. True only when the source
    // matches the RUNNING mode. Consulted by controllers before durable
    // acceptance and by QML before it routes a shot into the pipeline.
    Q_INVOKABLE bool sourceAllowed(int shotSource) const;

    // Report/label helpers for QML. Given a session's RECORDED mode token
    // ("Live"/"Demo"/empty), returns the report label — an empty/unknown token
    // yields the Legacy label (a pre-F10 session is NEVER relabelled as Live).
    Q_INVOKABLE QString sessionLabel(const QString& token) const {
        return ta::mode::sessionModeLabel(ta::mode::sessionModeFromToken(token));
    }
    // Short badge word for a recorded session token ("LIVE"/"DEMO"/"LEGACY").
    Q_INVOKABLE QString sessionBadge(const QString& token) const {
        const ta::mode::SessionMode s = ta::mode::sessionModeFromToken(token);
        return s == ta::mode::SessionMode::Live ? QStringLiteral("LIVE")
             : s == ta::mode::SessionMode::Demo ? QStringLiteral("DEMO")
                                                : QStringLiteral("LEGACY");
    }
    Q_INVOKABLE bool tokenIsDemo(const QString& token) const {
        return ta::mode::sessionModeFromToken(token) == ta::mode::SessionMode::Demo;
    }

    // ── Settings selection / apply ──────────────────────────────────────
    // Preview a selection (does not touch config). Updates restartRequired.
    Q_INVOKABLE void selectMode(int mode);
    // Apply the selected mode: refuses (returns false) when sessionBusy, when
    // the selection equals the running mode, or when the atomic write fails.
    // On success writes config.ini atomically and sets restartRequired.
    Q_INVOKABLE bool applyModeChange(bool sessionBusy);
    // Operator chose "Restart Now" after a successful applyModeChange.
    Q_INVOKABLE void requestRestart();

    // Atomic, key-preserving write of [App_Settings]/app_mode. Public + static
    // so the harness can exercise it against an isolated temp file. Returns
    // false and leaves the original file intact on any failure.
    static bool writeModeToConfig(const QString& path, ta::mode::Mode mode, QString* err);

signals:
    void selectionChanged();
    void errorChanged();
    void restartRequested();
    void configWriteFailed(const QString& message);

private:
    QString          m_configPath;
    ta::mode::Mode   m_running;
    ta::mode::Mode   m_selected;
    bool             m_configValid;
    QString          m_rawValue;
    bool             m_restartRequired = false;
    QString          m_lastError;
};

#endif // TA_MODE_OPERATINGMODESERVICE_H
