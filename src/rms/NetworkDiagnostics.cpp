#include "rms/NetworkDiagnostics.h"

#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QVariantMap>

namespace ta {
namespace rms {

NetworkDiagnostics::NetworkDiagnostics(QObject* parent)
    : QObject(parent)
{
}

void NetworkDiagnostics::setListenerState(bool listening, int port, const QString& error)
{
    if (m_listening == listening && m_port == port && m_error == error)
        return;
    m_listening = listening;
    m_port = port;
    m_error = error;
    emit changed();
}

void NetworkDiagnostics::setMode(const QString& mode, bool isLive)
{
    if (m_mode == mode && m_live == isLive)
        return;
    m_mode = mode;
    m_live = isLive;
    emit changed();
}

QString NetworkDiagnostics::hostName() const
{
    return QHostInfo::localHostName();
}

QVariantList NetworkDiagnostics::collect(bool onlyPlausible) const
{
    QVariantList out;
    const auto ifaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : ifaces) {
        const auto flags = iface.flags();
        const bool up = flags.testFlag(QNetworkInterface::IsUp)
                        && flags.testFlag(QNetworkInterface::IsRunning);
        const bool loopback = flags.testFlag(QNetworkInterface::IsLoopBack);

        for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
            const QHostAddress addr = entry.ip();
            if (addr.protocol() != QAbstractSocket::IPv4Protocol)
                continue;
            // 169.254.x.x means DHCP never answered — worth showing in the
            // full list, never in the short one, because it is a symptom
            // rather than an address anyone can use.
            const bool linkLocal = addr.isLinkLocal();
            if (onlyPlausible && (!up || loopback || linkLocal))
                continue;

            QVariantMap m;
            m[QStringLiteral("name")] = iface.humanReadableName();
            m[QStringLiteral("address")] = addr.toString();
            m[QStringLiteral("netmask")] = entry.netmask().toString();
            m[QStringLiteral("hardware")] = iface.hardwareAddress();
            m[QStringLiteral("up")] = up;
            m[QStringLiteral("loopback")] = loopback;
            m[QStringLiteral("linkLocal")] = linkLocal;
            const QString n = iface.humanReadableName();
            m[QStringLiteral("isWireless")] =
                iface.type() == QNetworkInterface::Wifi
                || n.contains(QLatin1String("Wi-Fi"), Qt::CaseInsensitive)
                || n.contains(QLatin1String("Wireless"), Qt::CaseInsensitive);
            out.append(m);
        }
    }
    return out;
}

QVariantList NetworkDiagnostics::interfaces() const
{
    return collect(true);
}

QVariantList NetworkDiagnostics::allInterfaces() const
{
    return collect(false);
}

bool NetworkDiagnostics::hasUsableInterface() const
{
    return !collect(true).isEmpty();
}

}  // namespace rms
}  // namespace ta
