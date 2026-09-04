#ifndef TA_RMS_NODE_TECHAIMNODECOMMANDS_H
#define TA_RMS_NODE_TECHAIMNODECOMMANDS_H

// WHAT THIS STATION WILL LET A RANGE DO, AND WHAT IT WILL NOT.
//
// This is the node side of the R2 control plane bound to the actual Tech Aim
// application. It implements the qualified IControlCommandHandler contract, so
// authentication, framing, command idempotency and the handled-command journal
// are all inherited from code that is already qualified - none of it is
// reimplemented here.
//
// THE ARMING RULE, AND WHY IT IS A CAPABILITY AND NOT A FLAG.
//
// Two of these commands change a live competition: START_AT begins an official
// match, STOP ends one. In an evaluation build that has never run on a range,
// those must not be reachable by default.
//
// The wrong way to express that is to accept the command and quietly do
// nothing - an ack saying `accepted` when nothing happened is a plausible but
// false report, and this project does not ship those. The right way is the
// mechanism the protocol already has: when session control is not armed, the
// node DOES NOT ADVERTISE those capabilities, and the endpoint refuses them
// with UNSUPPORTED_CAPABILITY. RMS is told the truth, in the protocol's own
// vocabulary, and an operator sees a lane that cannot be started rather than
// one that claims it started.
//
// Read-only commands - REQUEST_STATUS and REQUEST_REPLAY - are always
// available. They change nothing, so there is nothing to arm.
//
// FEED_PAPER IS NEVER ADVERTISED. It moves physical hardware and stays off
// until a node adapter is physically validated.
//
// NO SCORING, NO ACQUISITION. Replay reads the node's own journal - the same
// authoritative record the live publisher reads - and republishes it through
// the SAME conversion, so a replayed shot is byte-identical to the live one.
// Nothing here recomputes a score, a coordinate or an acceptance.

#include "reliability/store/SessionStore.h"
#include "rms/control/ControlProtocol.h"
#include "rms/control/NodeControlEndpoint.h"

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

class QualificationController;

namespace ta {
namespace rms {
namespace node {

class TechAimNodeCommands : public QObject,
                            public ta::rms::control::IControlCommandHandler
{
    Q_OBJECT
    // What a range has asked this lane to be. Exposed so the operator screen
    // can show an assignment that arrived over the network rather than leaving
    // it invisible until the match starts.
    Q_PROPERTY(QString assignedAthlete READ assignedAthlete NOTIFY intentChanged)
    Q_PROPERTY(QString assignedLane READ assignedLane NOTIFY intentChanged)
    Q_PROPERTY(bool sessionControlArmed READ sessionControlArmed
                   WRITE setSessionControlArmed NOTIFY armedChanged)
    Q_PROPERTY(qint64 scheduledStartUtcMs READ scheduledStartUtcMs NOTIFY intentChanged)

public:
    // Supplies whichever session store is ACTIVE, exactly as the incident
    // service does. The node never holds a store pointer of its own.
    using StoreProvider = std::function<ta::rel::SessionStore*()>;

    explicit TechAimNodeCommands(QObject* parent = nullptr);

    void setStoreProvider(StoreProvider p) { m_store = std::move(p); }
    // The qualification lifecycle this node will drive when session control is
    // armed. Its EXISTING public transitions are used - nothing new is added to
    // it, and the reducer still refuses any transition that is not legal.
    void setQualificationController(QualificationController* c) { m_qual = c; }

    // Identity, so replayed events carry the same publisher fields as live ones.
    void setIdentity(const QString& nodeId, const QString& bootId)
    { m_nodeId = nodeId; m_bootId = bootId; }
    void setProgrammeId(const QString& p) { m_programmeId = p; }
    void setLaneHint(const QString& l) { m_laneHint = l; }

    // Where this node's session journals live. Replay reads from here.
    void setJournalDirectory(const QString& dir) { m_journalDir = dir; }

    QString assignedAthlete() const { return m_athlete; }
    QString assignedLane() const { return m_laneHint; }
    qint64  scheduledStartUtcMs() const { return m_scheduledStartUtcMs; }

    bool sessionControlArmed() const { return m_armed; }
    void setSessionControlArmed(bool armed);

    // Exactly what this node advertises, given how it is armed. The endpoint
    // gates every command against this list, so an unarmed node cannot be
    // started even by a peer that ignores it.
    QStringList capabilities() const;

    // ── IControlCommandHandler ────────────────────────────────────────────
    Result apply(const ta::rms::control::Command& c) override;
    QList<QJsonObject> replayEvents(const QString& sessionId, int afterSequence,
                                    int maxEvents, bool* hasMoreOut) override;

signals:
    void intentChanged();
    void armedChanged();
    // A scheduled start reached its instant. Emitted whether or not session
    // control is armed to DRIVE it, so the screen can show that a range asked.
    void startInstantReached();

private:
    Result applyAssignAthlete(const ta::rms::control::Command& c);
    Result applyPrepareSession(const ta::rms::control::Command& c);
    Result applyStartAt(const ta::rms::control::Command& c);
    Result applyStop(const ta::rms::control::Command& c);
    Result statusResult() const;

    StoreProvider m_store;
    QualificationController* m_qual = nullptr;
    QString m_nodeId, m_bootId, m_programmeId, m_laneHint, m_journalDir;
    QString m_athlete;
    bool    m_armed = false;
    qint64  m_scheduledStartUtcMs = -1;
    qint64  m_rmsToNodeOffsetMs = 0;
};

} // namespace node
} // namespace rms
} // namespace ta

#endif // TA_RMS_NODE_TECHAIMNODECOMMANDS_H
