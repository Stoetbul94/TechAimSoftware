#include "rms/control/NodeAddressBook.h"

#include "rms/RmsProtocol.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace ta {
namespace rms {
namespace control {

QString NodeAddressBook::observe(const QByteArray& payload,
                                 const QHostAddress& sender,
                                 qint64 nowUtcMs)
{
    if (sender.isNull())
        return QString();

    // Only the nodeId is read. Deliberately NOT the full protocol decode: this
    // class must never become a second ingest path with its own opinion about
    // what a datagram means. RangeMonitor is the one ingress.
    const QJsonObject o = QJsonDocument::fromJson(payload).object();
    if (o.isEmpty())
        return QString();
    // A datagram from a protocol RMS does not speak is not a node it can dial.
    if (o.value(QStringLiteral("protocolVersion")).toInt() != kProtocolVersion)
        return QString();
    const QString nodeId = o.value(QStringLiteral("nodeId")).toString();
    if (nodeId.isEmpty())
        return QString();

    Entry& e = m_book[nodeId];
    if (!e.address.isNull() && e.address != sender) {
        // The SAME node at a NEW address. Not a new node - the nodeId said so.
        ++e.updates;
        ++m_addressChanges;
    }
    e.address = sender;
    e.lastSeenUtcMs = nowUtcMs;
    return nodeId;
}

} // namespace control
} // namespace rms
} // namespace ta
