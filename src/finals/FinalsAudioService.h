#pragma once
// 3P FINAL — deterministic audio service (Phase D4, Option 1 approved).
//
// One pre-recorded WAV clip per command cue: clip resolution is a PURE
// cueId -> file mapping (clipPathForCue) with no state and no randomness; a
// missing clip falls back to the system beep, so every command is audible
// and the behaviour for a given cue never varies between runs. Listens to
// Finals3PController::commandIssued (events carry `audioCueId`); the
// controller itself has no audio dependency.
//
// Shipping voice clips = dropping <cueId>.wav files (lower-case command
// type names: athletestoline.wav, loadseries.wav, stop.wav, ...) into
// <appDir>/audio/finals/. Nothing else to configure.

#include <QObject>
#include <QVariantMap>
#include <QHash>
#include <QString>

class QSoundEffect;

class FinalsAudioService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)

public:
    explicit FinalsAudioService(QObject* parent = nullptr);

    // Pure mapping: <clipsDir>/<cueid>.wav. Static so tests can verify the
    // resolution without an audio backend.
    static QString clipPathForCue(const QString& cueId, const QString& clipsDir);

    QString clipsDir() const { return m_clipsDir; }
    void setClipsDir(const QString& dir) { m_clipsDir = dir; }

    bool enabled() const { return m_enabled; }
    void setEnabled(bool e);
    double volume() const { return m_volume; }
    void setVolume(double v);

    // Diagnostics (developer drawer / tests): what the last cue resolved to.
    Q_INVOKABLE QString lastCueId() const { return m_lastCueId; }
    Q_INVOKABLE bool lastCueUsedFallback() const { return m_lastFallback; }

public slots:
    // Connected to Finals3PController::commandIssued in main.cpp.
    void onCommandIssued(const QVariantMap& event);
    void playCue(const QString& cueId);

signals:
    void enabledChanged();
    void volumeChanged();
    void cuePlayed(const QString& cueId, bool usedFallback);

private:
    QSoundEffect* effectFor(const QString& path);

    QString m_clipsDir;
    bool m_enabled = true;
    double m_volume = 1.0;
    QString m_lastCueId;
    bool m_lastFallback = false;
    QHash<QString, QSoundEffect*> m_effects;   // clip path -> cached effect
};
