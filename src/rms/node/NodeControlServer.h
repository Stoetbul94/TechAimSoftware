#ifndef TA_RMS_NODE_NODECONTROLSERVER_H
#define TA_RMS_NODE_NODECONTROLSERVER_H

// THE CONTROL SOCKET. R2 built and qualified the protocol transport-free and
// deliberately left the socket unwired; this is that socket, and nothing more.
//
// IT CONTAINS NO DECISIONS. Every refusal - wrong key, bad MAC, wrong node,
// wrong version, unknown command, oversize frame, stale command, duplicate
// commandId - is decided by NodeControlEndpoint, which is already qualified
// against 1719 automated checks. This class moves bytes and owns connections.
// That separation is the reason the interesting cases were provable without a
// network, and it must not erode: if a policy question ever appears in this
// file, it is in the wrong place.
//
// TCP 7756, AND THE LEGACY UDP LISTENER IS UNTOUCHED. UDP 7756 is genuinely
// occupied by the target application's historical startMatchFromServer path.
// TCP 7756 and UDP 7756 are different sockets at the operating-system level,
// so both run side by side and neither is aware of the other. Retiring the
// legacy path is its own reviewed change and is not this one.
//
// TELEMETRY IS NOT AFFECTED. UDP 7755 remains a write-only sink.

#include "rms/control/CommandJournal.h"
#include "rms/control/NodeControlEndpoint.h"

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <functional>

class QTcpServer;
class QTcpSocket;

namespace ta {
namespace rms {
namespace node {

class NodeControlServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool listening READ listening NOTIFY stateChanged)
    Q_PROPERTY(int connectedPeers READ connectedPeers NOTIFY stateChanged)
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY stateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY stateChanged)

public:
    NodeControlServer(ta::rms::control::NodeControlEndpoint::Identity identity,
                      QByteArray rangeKey,
                      ta::rms::control::IControlCommandHandler* handler,
                      ta::rms::control::CommandJournal* journal,
                      QObject* parent = nullptr);
    ~NodeControlServer() override;

    // Starts listening. Returns false and records why on any failure - a
    // control port that could not be bound is a FAILURE the operator must see,
    // never a silently absent feature.
    bool start(quint16 port = ta::rms::control::kControlPort);
    void stop();

    // Called immediately after any exchange that added to the command journal.
    //
    // WRITTEN ON EVERY HANDLED COMMAND, not at shutdown. A journal flushed only
    // on a clean exit is lost to exactly the events it exists to survive - a
    // crash, a force-kill, a power cut at the range - and the next boot would
    // then re-execute a command it had already performed. State-changing
    // commands are rare (a handful per match), so the cost of writing each one
    // is nothing against what losing them means.
    void setPersistHook(std::function<void()> persist) { m_persist = std::move(persist); }

    bool listening() const;
    int  connectedPeers() const { return m_endpoints.size(); }
    bool authenticated() const;
    QString lastError() const { return m_lastError; }

    // Capabilities can change while running - arming session control adds them.
    // A peer already connected keeps the set it authenticated against; the new
    // set applies to the next connection, because capabilities are negotiated
    // at handshake and rewriting them mid-connection would make an ack mean
    // something different from what the peer agreed to.
    void setIdentity(const ta::rms::control::NodeControlEndpoint::Identity& id)
    { m_identity = id; }

signals:
    void stateChanged();

private slots:
    void onNewConnection();

private:
    void closePeer(QTcpSocket* s);

    ta::rms::control::NodeControlEndpoint::Identity m_identity;
    QByteArray m_key;
    ta::rms::control::IControlCommandHandler* m_handler = nullptr;
    ta::rms::control::CommandJournal* m_journal = nullptr;

    QTcpServer* m_server = nullptr;
    // One endpoint per connection. The endpoint holds the handshake state, so
    // it cannot be shared: two peers would otherwise inherit each other's
    // authentication.
    QHash<QTcpSocket*, ta::rms::control::NodeControlEndpoint*> m_endpoints;
    std::function<void()> m_persist;
    QString m_lastError;
};

} // namespace node
} // namespace rms
} // namespace ta

#endif // TA_RMS_NODE_NODECONTROLSERVER_H
