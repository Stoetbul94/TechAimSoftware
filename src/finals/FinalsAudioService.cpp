#include "FinalsAudioService.h"

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
    m_clipsDir = QCoreApplication::applicationDirPath()
                 + QStringLiteral("/audio/finals");
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
        QApplication::beep();
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
