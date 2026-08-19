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

private:
    QUdpSocket* m_socket = nullptr;
    bool    m_listening = false;
    quint16 m_boundPort = 0;
    QString m_lastError;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_UDPOBSERVER_H
