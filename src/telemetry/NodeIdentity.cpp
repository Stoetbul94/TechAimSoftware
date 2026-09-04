#include "NodeIdentity.h"

#include <QSettings>
#include <QUuid>

namespace ta {
namespace telemetry {

namespace {

QString mintNodeId()
{
    // "TA-NODE-" + 12 hex. Short enough to read out over a radio during a
    // match, long enough that two stations configured from the same disk
    // image do not collide.
    const QString hex = QUuid::createUuid().toString(QUuid::Id128).left(12);
    return QStringLiteral("TA-NODE-") + hex.toUpper();
}

QString mintBootId()
{
    return QUuid::createUuid().toString(QUuid::Id128).left(12).toLower();
}

} // namespace

NodeIdentity NodeIdentity::build(QSettings& settings)
{
    NodeIdentity id;
    id.m_bootId = mintBootId();

    const QString key = QString::fromLatin1(NodeIdentity::settingsKey());
    const QString stored = settings.value(key).toString().trimmed();
    if (!stored.isEmpty()) {
        id.m_nodeId = stored;
        return id;
    }

    id.m_nodeId = mintNodeId();
    id.m_generated = true;
    settings.setValue(key, id.m_nodeId);
    // sync() here and not at destruction: if the application is killed before
    // it next writes settings, the station must still come back with the SAME
    // identity, or RMS would see a brand new lane appear.
    settings.sync();
    return id;
}

const char* NodeIdentity::settingsKey()
{
    return "rms/nodeId";
}

NodeIdentity NodeIdentity::forApplication()
{
    QSettings settings;   // application namespace, set in main() from ProductIdentity
    return build(settings);
}

NodeIdentity NodeIdentity::forSettingsFile(const QString& iniPath)
{
    QSettings settings(iniPath, QSettings::IniFormat);
    return build(settings);
}

} // namespace telemetry
} // namespace ta
