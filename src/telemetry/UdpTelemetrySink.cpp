#include "UdpTelemetrySink.h"

#include "rms/RmsProtocol.h"

#include <QUdpSocket>

namespace ta {
namespace telemetry {

UdpTelemetrySink::UdpTelemetrySink(QObject* parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_address(QHostAddress::Broadcast)
    , m_port(ta::rms::kObservationPort)
{
    // Never bound. A socket that only ever writes cannot deliver an inbound
    // command, which is the property this milestone has to keep.
}

void UdpTelemetrySink::setDestination(const QHostAddress& address, quint16 port)
{
    m_address = address;
    m_port = port;
}

bool UdpTelemetrySink::send(const QByteArray& datagram)
{
    const qint64 written = m_socket->writeDatagram(datagram, m_address, m_port);
    if (written == datagram.size()) {
        ++m_sent;
        return true;
    }
    // A partial write on a datagram socket is a failed datagram, not a partial
    // one — there is nothing to resume.
    ++m_failed;
    m_lastError = m_socket->errorString();
    return false;
}

} // namespace telemetry
} // namespace ta
