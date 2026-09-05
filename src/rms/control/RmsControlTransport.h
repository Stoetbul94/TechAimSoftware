#ifndef TA_RMS_RMSCONTROLTRANSPORT_H
#define TA_RMS_RMSCONTROLTRANSPORT_H

// THE RMS SIDE OF THE CONTROL SOCKET. R2 built the protocol transport-free and
// R2C proved it without a network; this is the socket that was deliberately
// left unwired, and it is the piece the first physical test was missing.
//
// IT CONTAINS NO DECISIONS. Every refusal - wrong key, bad MAC, wrong node,
// wrong version, stale command, duplicate id - is decided by the qualified
// RmsControlClient and NodeControlEndpoint. This class opens sockets, moves
// bytes and gives up on time. If a policy question ever appears here it is in
// the wrong file.
//
// SYNCHRONOUS, WITH A HARD DEADLINE, AND WHY THAT IS ACCEPTABLE HERE.
// RangeControlCoordinator's Link is request/response: hand it bytes, get bytes.
// Backing that with a socket means waiting. Every wait in this class is bounded
// by a short timeout and a failure is simply "no answer", which the coordinator
// already treats as UNREACHABLE and retries later with the same commandId.
//
// The cost is real and stated rather than hidden: a stalled node blocks this
// call for up to the timeout. At one lane that is invisible. Before a
// multi-lane range runs this, the transport needs to move to a worker thread -
// recorded in the R3B report, not discovered later.

#include "rms/control/NodeAddressBook.h"
#include "rms/control/RangeControlCoordinator.h"

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>

class QTcpSocket;

namespace ta {
namespace rms {
namespace control {

class RmsControlTransport : public QObject
{
    Q_OBJECT
public:
    // Deliberately short. A range LAN answers in milliseconds; anything that
    // does not is a lane with a problem, and waiting longer only delays saying
    // so. Connect gets more room than a reply because TCP setup can include an
    // ARP round trip on a cold link.
    static constexpr int kConnectTimeoutMs = 1500;
    static constexpr int kReplyTimeoutMs   = 1500;

    explicit RmsControlTransport(NodeAddressBook* book, QObject* parent = nullptr);
    ~RmsControlTransport() override;

    // The function handed to RangeControlCoordinator::setLink.
    RangeControlCoordinator::Link link();

    // Drops any socket held for this node, so the NEXT frame opens a fresh
    // connection. Called immediately before a handshake: the node builds one
    // endpoint per connection, so a new handshake must arrive on a new socket
    // or it would be answered by an endpoint that already believes it is
    // authenticated.
    void dropPeer(const QString& nodeId);
    void dropAll();

    int openPeers() const { return m_sockets.size(); }
    QString lastError(const QString& nodeId) const { return m_lastError.value(nodeId); }
    // Counts, for the status panel and for a support bundle.
    int connectFailures() const { return m_connectFailures; }
    int replyTimeouts() const   { return m_replyTimeouts; }

signals:
    // Emitted for anything that stopped a frame reaching a node or an answer
    // coming back. Never swallowed: a control channel that fails quietly is
    // indistinguishable from one that was never tried.
    void transportProblem(const QString& nodeId, const QString& reason);

private:
    QByteArray exchange(const QString& nodeId, const QByteArray& frame);
    QTcpSocket* socketFor(const QString& nodeId);

    NodeAddressBook* m_book = nullptr;
    QHash<QString, QTcpSocket*> m_sockets;
    QHash<QString, QString> m_lastError;
    int m_connectFailures = 0;
    int m_replyTimeouts = 0;
};

} // namespace control
} // namespace rms
} // namespace ta

#endif // TA_RMS_RMSCONTROLTRANSPORT_H
