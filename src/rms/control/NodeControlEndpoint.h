#ifndef TA_RMS_NODECONTROLENDPOINT_H
#define TA_RMS_NODECONTROLENDPOINT_H

// The NODE side of the control channel.
//
// TRANSPORT-FREE ON PURPOSE. This class takes bytes and returns bytes. It owns
// no socket. That is what makes every case in the brief's security section -
// wrong key, bad MAC, replayed challenge, wrong nodeId, wrong version, unknown
// command, malformed JSON, oversize frame, duplicate commandId, stale command -
// testable deterministically, with no network and no timing. The socket is a
// thin shell around this, and the shell has no decisions in it.
//
// THE NODE DECIDES. Every command may be refused. RMS states an intention; the
// node is the authority on whether its own match may be altered, and the ack
// reports the state the node is NOW in rather than merely acknowledging
// receipt. That rule comes straight from the existing command-boundary design
// and is not softened here.
//
// NO SCORING. This file contains no scoring, no coordinate handling and no
// competition rules. It validates, dispatches to a handler, and answers.

#include "rms/control/ControlProtocol.h"

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QQueue>
#include <QSet>
#include <QString>

namespace ta {
namespace rms {
namespace control {

// What the node can actually do with a command. Deliberately narrow, and with
// NO network types in it: the network layer authenticates and parses, this
// applies semantics, and the separation is what keeps both testable.
class IControlCommandHandler
{
public:
    virtual ~IControlCommandHandler() = default;

    struct Result {
        bool        accepted = false;
        QString     reasonCode;
        QString     message;
        QJsonObject resultingState;
    };

    // Apply ONCE. The endpoint guarantees this is never called twice for the
    // same commandId, so an implementation does not need its own dedup.
    virtual Result apply(const Command& c) = 0;

    // Persisted session events after `afterSequence`, oldest first, at most
    // `maxEvents`. Original event objects - ids and sequences preserved.
    virtual QList<QJsonObject> replayEvents(const QString& sessionId,
                                            int afterSequence,
                                            int maxEvents,
                                            bool* hasMoreOut) = 0;
};

class NodeControlEndpoint
{
public:
    struct Identity {
        QString     nodeId;
        QString     bootId;
        QString     product;
        QString     appVersion;
        QString     commit;
        QStringList capabilities;
    };

    NodeControlEndpoint(Identity id,
                        QByteArray rangeKey,
                        IControlCommandHandler* handler);

    // Feed received bytes; returns bytes to send back. A returned
    // `closeConnection` means the peer must be dropped - the reason is in
    // `lastError()`.
    struct Reaction {
        QByteArray reply;
        bool       closeConnection = false;
    };
    Reaction onBytes(const QByteArray& bytes, qint64 nowUtcMs);

    bool    authenticated() const { return m_authenticated; }
    QString lastError() const { return m_lastError; }
    int     commandsApplied() const { return m_applied; }
    int     duplicatesRefused() const { return m_duplicates; }

    // Commands older than this are refused as stale. Generous enough for a
    // slow range network, short enough that a captured command cannot be
    // replayed hours later.
    static constexpr qint64 kCommandWindowMs = 60 * 1000;

    // Bounded history: a range does not issue unbounded commands, and an
    // unbounded set would be a memory leak an attacker could drive.
    static constexpr int kCommandHistory = 512;

private:
    Ack handleCommand(const Command& c, qint64 nowUtcMs);
    // Emits a REPLAY_BATCH followed by its ack.
    QByteArray buildReplay(const Command& c, qint64 nowUtcMs);
    Ack makeAck(const Command& c, bool accepted, const char* reasonCode,
                const QString& message, qint64 nowUtcMs) const;
    bool hasCapability(const char* c) const;

    Identity   m_id;
    QByteArray m_key;
    IControlCommandHandler* m_handler = nullptr;

    FrameReader m_reader;
    bool    m_authenticated = false;
    QString m_rmsNonce, m_nodeNonce, m_rmsInstanceId;
    QString m_lastError;
    int     m_applied = 0;
    int     m_duplicates = 0;

    // commandId → the ack that was produced the first time. A repeat returns
    // this instead of applying anything again.
    QHash<QString, Ack> m_handled;
    QQueue<QString>     m_handledOrder;
};

} // namespace control
} // namespace rms
} // namespace ta

#endif // TA_RMS_NODECONTROLENDPOINT_H
