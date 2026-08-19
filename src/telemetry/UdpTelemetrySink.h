#ifndef TA_TELEMETRY_UDPTELEMETRYSINK_H
#define TA_TELEMETRY_UDPTELEMETRYSINK_H

// The production sink: one UDP datagram per message, broadcast on the range's
// existing node→range port (7755).
//
// SEND-ONLY, FIRE-AND-FORGET. The socket is never bound for reading, so this
// class cannot receive anything — there is no inbound path here for an RMS
// command to arrive on, and the node's legacy inbound port (7756) is not
// touched. A failed send returns false and is over; nothing waits, nothing
// retries here, and no acknowledgement is expected because protocol v1 has
// none.

#include "ITelemetrySink.h"

#include <QHostAddress>
#include <QObject>

class QUdpSocket;

namespace ta {
namespace telemetry {

class UdpTelemetrySink : public QObject, public ITelemetrySink
{
    Q_OBJECT
public:
    // Defaults to broadcast on the RMS observation port. A unicast address is
    // accepted so a test (or a fixed range controller) can be targeted
    // directly without putting traffic on the whole subnet.
    explicit UdpTelemetrySink(QObject* parent = nullptr);

    void setDestination(const QHostAddress& address, quint16 port);

    bool send(const QByteArray& datagram) override;

    QString lastError() const { return m_lastError; }
    int sentCount() const { return m_sent; }
    int failedCount() const { return m_failed; }

private:
    QUdpSocket* m_socket = nullptr;
    QHostAddress m_address;
    quint16 m_port = 0;
    QString m_lastError;
    int m_sent = 0;
    int m_failed = 0;
};

} // namespace telemetry
} // namespace ta

#endif // TA_TELEMETRY_UDPTELEMETRYSINK_H
