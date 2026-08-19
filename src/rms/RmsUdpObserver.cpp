#include "RmsUdpObserver.h"

#include <QUdpSocket>

namespace ta {
namespace rms {

RmsUdpObserver::RmsUdpObserver(QObject* parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
{
    connect(m_socket, &QUdpSocket::readyRead, this, &RmsUdpObserver::readPending);
}

bool RmsUdpObserver::listen(quint16 port)
{
    // ShareAddress: a target application on the same machine already binds
    // this port. RMS observing must never take it away from the node.
    m_listening = m_socket->bind(port, QUdpSocket::ShareAddress);
    m_boundPort = m_listening ? port : 0;
    if (!m_listening)
        m_lastError = m_socket->errorString();
    return m_listening;
}

void RmsUdpObserver::readPending()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray payload;
        payload.resize(int(m_socket->pendingDatagramSize()));
        // The sender address is deliberately discarded. RMS has no reply
        // path, so keeping a destination around would only invite one.
        m_socket->readDatagram(payload.data(), payload.size());
        emit datagramReceived(payload);
    }
}

} // namespace rms
} // namespace ta
