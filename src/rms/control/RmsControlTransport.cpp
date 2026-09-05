#include "rms/control/RmsControlTransport.h"

#include "rms/control/ControlProtocol.h"

#include <QElapsedTimer>
#include <QTcpSocket>

namespace ta {
namespace rms {
namespace control {

RmsControlTransport::RmsControlTransport(NodeAddressBook* book, QObject* parent)
    : QObject(parent), m_book(book)
{
}

RmsControlTransport::~RmsControlTransport()
{
    dropAll();
}

RangeControlCoordinator::Link RmsControlTransport::link()
{
    return [this](const QString& nodeId, const QByteArray& frame) {
        return exchange(nodeId, frame);
    };
}

void RmsControlTransport::dropPeer(const QString& nodeId)
{
    QTcpSocket* s = m_sockets.take(nodeId);
    if (!s)
        return;
    s->abort();
    s->deleteLater();
}

void RmsControlTransport::dropAll()
{
    const QStringList ids = m_sockets.keys();
    for (const QString& id : ids)
        dropPeer(id);
}

QTcpSocket* RmsControlTransport::socketFor(const QString& nodeId)
{
    QTcpSocket* s = m_sockets.value(nodeId, nullptr);
    if (s && s->state() == QAbstractSocket::ConnectedState)
        return s;

    // A socket that exists but is not connected is worse than none: it would
    // silently swallow the frame.
    if (s) {
        m_sockets.remove(nodeId);
        s->abort();
        s->deleteLater();
    }

    if (!m_book || !m_book->has(nodeId)) {
        // RMS has never heard this node's telemetry, so it has nowhere to dial.
        // That is a real state and is reported as one rather than looking like
        // a refused connection.
        m_lastError.insert(nodeId, QStringLiteral("no address known for this node"));
        emit transportProblem(nodeId, m_lastError.value(nodeId));
        return nullptr;
    }

    s = new QTcpSocket(this);
    s->connectToHost(m_book->addressFor(nodeId), kControlPort);
    if (!s->waitForConnected(kConnectTimeoutMs)) {
        ++m_connectFailures;
        m_lastError.insert(nodeId, QStringLiteral("connect to %1:%2 failed: %3")
                                       .arg(m_book->addressFor(nodeId).toString())
                                       .arg(int(kControlPort))
                                       .arg(s->errorString()));
        emit transportProblem(nodeId, m_lastError.value(nodeId));
        s->abort();
        s->deleteLater();
        return nullptr;
    }
    m_sockets.insert(nodeId, s);
    return s;
}

QByteArray RmsControlTransport::exchange(const QString& nodeId,
                                         const QByteArray& frame)
{
    QTcpSocket* s = socketFor(nodeId);
    if (!s)
        return QByteArray();          // unreachable; the coordinator handles it

    if (s->write(frame) != frame.size() || !s->waitForBytesWritten(kReplyTimeoutMs)) {
        m_lastError.insert(nodeId, QStringLiteral("write failed: %1").arg(s->errorString()));
        emit transportProblem(nodeId, m_lastError.value(nodeId));
        dropPeer(nodeId);
        return QByteArray();
    }

    // ONE READ IS NOT ONE FRAME. The reply is length-prefixed, and a replay
    // batch is followed by its ack, so bytes are collected until the framing
    // says a whole frame is present - or the deadline passes. Returning a
    // partial frame would make the qualified reader reject a good answer.
    QByteArray in;
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < kReplyTimeoutMs) {
        if (!s->waitForReadyRead(int(kReplyTimeoutMs - clock.elapsed())))
            break;
        in += s->readAll();
        if (in.size() < 4)
            continue;
        // Peek the declared length of the FIRST frame. More may follow it; the
        // caller's FrameReader handles however many arrive together.
        const quint32 declared = (quint32(quint8(in.at(0))) << 24)
                               | (quint32(quint8(in.at(1))) << 16)
                               | (quint32(quint8(in.at(2))) << 8)
                               |  quint32(quint8(in.at(3)));
        if (declared > quint32(kMaxFrameBytes)) {
            // Refused HERE as well as by the reader: an absurd length must not
            // be used to size a wait, let alone an allocation.
            m_lastError.insert(nodeId, QStringLiteral("oversize frame declared (%1 bytes)")
                                           .arg(declared));
            emit transportProblem(nodeId, m_lastError.value(nodeId));
            dropPeer(nodeId);
            return QByteArray();
        }
        if (quint32(in.size()) >= declared + 4u) {
            // Give a batch-plus-ack a moment to arrive together; one more short
            // read costs nothing and saves a needless second round trip.
            if (s->waitForReadyRead(25))
                in += s->readAll();
            return in;
        }
    }

    if (in.isEmpty()) {
        ++m_replyTimeouts;
        m_lastError.insert(nodeId, QStringLiteral("no answer within %1 ms").arg(kReplyTimeoutMs));
    } else {
        ++m_replyTimeouts;
        m_lastError.insert(nodeId, QStringLiteral("truncated answer (%1 bytes) within %2 ms")
                                       .arg(in.size()).arg(kReplyTimeoutMs));
    }
    emit transportProblem(nodeId, m_lastError.value(nodeId));
    // A half-read stream cannot be resynchronised, so the peer is dropped and
    // the next attempt starts a clean connection.
    dropPeer(nodeId);
    return QByteArray();
}

} // namespace control
} // namespace rms
} // namespace ta
