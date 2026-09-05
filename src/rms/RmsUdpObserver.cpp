#include "RmsUdpObserver.h"

#include <QUdpSocket>

namespace ta {
namespace rms {

RmsUdpObserver::RmsUdpObserver(QObject* parent)
    : QObject(parent)
    , m_socket4(new QUdpSocket(this))
    , m_socket6(new QUdpSocket(this))
{
    connect(m_socket4, &QUdpSocket::readyRead, this, &RmsUdpObserver::readPending);
    connect(m_socket6, &QUdpSocket::readyRead, this, &RmsUdpObserver::readPending);
}

bool RmsUdpObserver::listen(quint16 port)
{
    // ShareAddress: a target application on the same machine already binds
    // this port. RMS observing must never take it away from the node.
    const QUdpSocket::BindMode mode =
        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint;

    // IPv4 FIRST, and explicitly on 0.0.0.0. This is the one that matters:
    // every node broadcasts to 255.255.255.255, and an IPv4 socket is the
    // only binding guaranteed to receive that on Windows.
    m_listening4 = m_socket4->bind(QHostAddress::AnyIPv4, port, mode);

    // IPv6 as well, bound IPv6-ONLY so an IPv4 datagram cannot arrive twice.
    m_listening6 = m_socket6->bind(QHostAddress::AnyIPv6, port, mode);

    // Up if EITHER family bound. A range with no IPv6 stack is normal and must
    // not be treated as a failure, and neither must the reverse.
    m_listening = m_listening4 || m_listening6;
    m_boundPort = m_listening ? port : 0;

    if (!m_listening) {
        m_lastError = QStringLiteral("IPv4: %1 | IPv6: %2")
                          .arg(m_socket4->errorString(), m_socket6->errorString());
    } else if (!m_listening4) {
        // Worth surfacing: IPv6 alone will NOT see ordinary node broadcasts.
        m_lastError = QStringLiteral("IPv4 bind failed (%1) - IPv6 only; "
                                     "node broadcasts will NOT be received")
                          .arg(m_socket4->errorString());
    } else {
        m_lastError.clear();
    }
    return m_listening;
}

void RmsUdpObserver::readPending()
{
    // Whichever socket woke us. Both are drained on every wake-up, which is
    // harmless when one has nothing pending and avoids a starved family.
    QUdpSocket* const sockets[] = { m_socket4, m_socket6 };
    for (QUdpSocket* s : sockets) {
        if (!s)
            continue;
        while (s->hasPendingDatagrams()) {
            QByteArray payload;
            payload.resize(int(s->pendingDatagramSize()));
            // THE SENDER ADDRESS IS CONNECTION METADATA, NEVER IDENTITY.
            //
            // Until R3B this was deliberately discarded, because an observer
            // with no reply path should not keep a destination around. R3B
            // gives RMS a reply path - the authenticated TCP control channel -
            // and it has to dial somewhere, so the address is now carried.
            //
            // What has NOT changed is what it means. A node is its nodeId. The
            // address is only where that node was last heard from: RMS dials it
            // and then VERIFIES the authenticated nodeId is the one it meant to
            // reach, dropping the connection if it is not. Nothing downstream
            // may treat this as identity.
            QHostAddress sender;
            quint16 senderPort = 0;
            s->readDatagram(payload.data(), payload.size(), &sender, &senderPort);
            // IPv4-mapped IPv6 (::ffff:a.b.c.d) is normalised so the same node
            // heard on either socket yields one address, not two.
            bool converted = false;
            const quint32 v4 = sender.toIPv4Address(&converted);
            if (converted)
                sender = QHostAddress(v4);
            emit datagramReceived(payload);
            emit datagramReceivedFrom(payload, sender);
        }
    }
}

} // namespace rms
} // namespace ta
