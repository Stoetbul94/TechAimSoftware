#ifndef TA_RMS_UDPOBSERVER_H
#define TA_RMS_UDPOBSERVER_H

// ─────────────────────────────────────────────────────────────────────────────
// A RECEIVE-ONLY UDP endpoint.
//
// The socket is private and the class exposes no way to transmit: no send
// method, no outbound wrapper, no destination address, no reply-to-sender
// path. That is deliberate and is the mechanism behind the milestone's
// read-only invariant — a future control channel must be a NEW, separately
// reviewed class, not a method quietly added here. tst_readonly.cpp scans
// this file for exactly that, which is why the forbidden call is not even
// named in this comment.
//
// It binds the node→range telemetry port (7755) with ShareAddress, so a
// target application running on the same machine keeps working: RMS is one
// more listener, never an owner of the port.
// ─────────────────────────────────────────────────────────────────────────────

#include <QByteArray>
#include <QObject>

class QUdpSocket;

namespace ta {
namespace rms {

class RmsUdpObserver : public QObject
{
    Q_OBJECT
public:
    explicit RmsUdpObserver(QObject* parent = nullptr);

    bool listen(quint16 port);
    bool isListening() const { return m_listening; }
    quint16 boundPort() const { return m_boundPort; }
    QString lastError() const { return m_lastError; }

signals:
    void datagramReceived(const QByteArray& payload);

private slots:
    void readPending();

public:
    // Diagnostics. Which families actually came up, so a range-day log can
    // say what is listening rather than what was intended.
    bool listeningIPv4() const { return m_listening4; }
    bool listeningIPv6() const { return m_listening6; }

private:
    // TWO SOCKETS, ONE PER FAMILY — deliberately not one dual-stack socket.
    //
    // The nodes broadcast to 255.255.255.255. A single socket bound to
    // QHostAddress::Any comes up as "::" (IPv6) with dual-stack enabled, and
    // whether Windows delivers an IPv4 BROADCAST to that socket is not
    // something this project can rely on: it was observed working for a
    // same-machine broadcast here, but that is a different path from a frame
    // arriving off the Wi-Fi adapter, and range day is not the place to find
    // out. An explicit IPv4 socket removes the question entirely.
    //
    // The IPv6 socket is bound IPv6-ONLY so a v4-mapped datagram cannot also
    // be delivered to it — otherwise one announce could be processed twice
    // and a node would look like it was heart-beating at double rate.
    QUdpSocket* m_socket4 = nullptr;   // 0.0.0.0  — the one nodes actually use
    QUdpSocket* m_socket6 = nullptr;   // ::       — kept for IPv6-only ranges
    bool    m_listening = false;       // true when EITHER family is up
    bool    m_listening4 = false;
    bool    m_listening6 = false;
    quint16 m_boundPort = 0;
    QString m_lastError;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_UDPOBSERVER_H
