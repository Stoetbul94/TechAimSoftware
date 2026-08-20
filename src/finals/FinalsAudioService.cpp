#include "FinalsAudioService.h"

#include "platform/PlatformService.h"

#include <QSoundEffect>
#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QDebug>

FinalsAudioService::FinalsAudioService(QObject* parent)
    : QObject(parent)
{
    // A1/A2: the clips root is platform-resolved.
    //
    // Windows keeps <applicationDirPath>/audio/finals exactly as before —
    // dropping <cueId>.wav files next to the executable still works and no
    // deployed install changes.
    //
    // Android has no writable, populated "application directory" to read
    // assets from, so clips resolve through the Qt Android asset namespace
    // ("assets:/audio/finals") and travel inside the APK. No audio file is
    // duplicated in the repository to make this work — the repository ships
    // no WAV clips at all today, on either platform.
    m_clipsDir = ta::platform::finalsAudioClipsRoot();
}

QString FinalsAudioService::clipPathForCue(const QString& cueId,
                                           const QString& clipsDir)
{
    if (cueId.isEmpty())
        return QString();
    return QDir(clipsDir).filePath(cueId.toLower() + QStringLiteral(".wav"));
}

void FinalsAudioService::setEnabled(bool e)
{
    if (e == m_enabled)
        return;
    m_enabled = e;
    emit enabledChanged();
}

void FinalsAudioService::setVolume(double v)
{
    v = qBound(0.0, v, 1.0);
    if (qFuzzyCompare(v, m_volume))
        return;
    m_volume = v;
    for (QSoundEffect* fx : m_effects)
        fx->setVolume(v);
    emit volumeChanged();
}

void FinalsAudioService::onCommandIssued(const QVariantMap& event)
{
    playCue(event.value(QStringLiteral("audioCueId")).toString());
}

void FinalsAudioService::playCue(const QString& cueId)
{
    if (!m_enabled)
        return;
    m_lastCueId = cueId;
    const QString path = clipPathForCue(cueId, m_clipsDir);
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        m_lastFallback = false;
        effectFor(path)->play();
    } else {
        m_lastFallback = true;
        // A1/A2: QApplication::beep() is a SILENT NO-OP on Android. Calling it
        // there and reporting "fallback played" would claim an audible cue
        // that never happened — for a finals command sequence, that is a
        // safety-relevant lie. So the beep is only attempted where it actually
        // sounds, and the absence is logged rather than papered over.
        //
        // usedFallback is still reported truthfully in the signal either way,
        // so the developer drawer and tests observe the same thing on both
        // platforms: this cue produced no clip.
        if (ta::platform::supportsSystemBeep()) {
            QApplication::beep();
        } else {
            qWarning().noquote()
                << "Finals audio: no clip for cue" << cueId
                << "and no system beep on this platform — command cue was SILENT."
                << "Expected clip:" << path;
        }
    }
    emit cuePlayed(cueId, m_lastFallback);
}

QSoundEffect* FinalsAudioService::effectFor(const QString& path)
{
    QSoundEffect* fx = m_effects.value(path, nullptr);
    if (!fx) {
        fx = new QSoundEffect(this);
        fx->setSource(QUrl::fromLocalFile(path));
        fx->setVolume(m_volume);
        m_effects.insert(path, fx);
    }
    return fx;
}
