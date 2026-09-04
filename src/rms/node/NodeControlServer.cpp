#include "rms/node/NodeControlServer.h"

#include <QDateTime>
#include <QTcpServer>
#include <QTcpSocket>

namespace ta {
namespace rms {
namespace node {

using namespace ta::rms::control;

NodeControlServer::NodeControlServer(NodeControlEndpoint::Identity identity,
                                     QByteArray rangeKey,
                                     IControlCommandHandler* handler,
                                     CommandJournal* journal,
                                     QObject* parent)
    : QObject(parent)
    , m_identity(std::move(identity))
    , m_key(std::move(rangeKey))
    , m_handler(handler)
    , m_journal(journal)
{
}

NodeControlServer::~NodeControlServer()
{
    stop();
}

bool NodeControlServer::start(quint16 port)
{
    stop();

    // A CONTROL CHANNEL WITHOUT A KEY DOES NOT OPEN. Falling back to an
    // unauthenticated port would turn a missing file into an open door onto
    // the range - the one failure mode this whole plane exists to prevent.
    if (m_key.size() < 32) {
        m_lastError = QStringLiteral(
            "range key missing or too short - control channel NOT started");
        emit stateChanged();
        return false;
    }

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection,
            this, &NodeControlServer::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, port)) {
        m_lastError = QStringLiteral("could not bind TCP %1: %2")
                          .arg(port).arg(m_server->errorString());
        delete m_server;
        m_server = nullptr;
        emit stateChanged();
        return false;
    }
    m_lastError.clear();
    emit stateChanged();
    return true;
}

void NodeControlServer::stop()
{
    for (auto it = m_endpoints.begin(); it != m_endpoints.end(); ++it) {
        delete it.value();
        it.key()->abort();
        it.key()->deleteLater();
    }
    m_endpoints.clear();
    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
    emit stateChanged();
}

bool NodeControlServer::listening() const
{
    return m_server && m_server->isListening();
}

bool NodeControlServer::authenticated() const
{
    for (auto it = m_endpoints.constBegin(); it != m_endpoints.constEnd(); ++it)
        if (it.value()->authenticated())
            return true;
    return false;
}

void NodeControlServer::onNewConnection()
{
    while (m_server && m_server->hasPendingConnections()) {
        QTcpSocket* s = m_server->nextPendingConnection();
        if (!s)
            continue;

        // A FRESH ENDPOINT PER CONNECTION. The endpoint owns the handshake
        // state, so sharing one would let a second peer inherit the first
        // peer's authentication - which is the whole game.
        //
        // The JOURNAL is shared on purpose: it is the node's memory of what it
        // has already done, and it must not reset because a peer reconnected.
        // That is what makes a retried commandId idempotent across a dropped
        // connection as well as across a restart.
        auto* ep = new NodeControlEndpoint(m_identity, m_key, m_handler, m_journal);
        m_endpoints.insert(s, ep);

        connect(s, &QTcpSocket::readyRead, this, [this, s]() {
            auto it = m_endpoints.find(s);
            if (it == m_endpoints.end())
                return;
            const QByteArray in = s->readAll();
            const int journalBefore = m_journal ? m_journal->size() : 0;
            const NodeControlEndpoint::Reaction r =
                it.value()->onBytes(in, QDateTime::currentMSecsSinceEpoch());
            if (!r.reply.isEmpty())
                s->write(r.reply);
            // PERSIST BEFORE THE NEXT COMMAND, not at shutdown. If this process
            // dies between handling a command and writing it down, the next boot
            // would perform it a second time - which is the failure the journal
            // exists to prevent, so the window is closed here rather than left
            // open for the length of a match.
            if (m_persist && m_journal && m_journal->size() != journalBefore)
                m_persist();
            if (r.closeConnection) {
                // The endpoint decided this peer must go. The reply is flushed
                // first so a refusal reaches the peer rather than being lost
                // with the socket - being told NO is more useful than silence.
                s->flush();
                m_lastError = it.value()->lastError();
                closePeer(s);
            }
            emit stateChanged();
        });

        connect(s, &QTcpSocket::disconnected, this, [this, s]() { closePeer(s); });

        emit stateChanged();
    }
}

void NodeControlServer::closePeer(QTcpSocket* s)
{
    auto it = m_endpoints.find(s);
    if (it == m_endpoints.end())
        return;
    delete it.value();
    m_endpoints.erase(it);
    s->close();
    s->deleteLater();
    emit stateChanged();
}

} // namespace node
} // namespace rms
} // namespace ta
