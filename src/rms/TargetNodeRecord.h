#ifndef TA_RMS_TARGETNODERECORD_H
#define TA_RMS_TARGETNODERECORD_H

// ─────────────────────────────────────────────────────────────────────────────
// What RMS has OBSERVED about one target node. This is a mirror, not a master
// copy: every field here was reported by the node, and nothing in this file
// may ever be written back to it.
//
// The distinction that runs through the whole class:
//   shotsAcceptedByNode   what the NODE says it has accepted — authoritative
//   observedShotCount()   what RMS actually received — may be lower after a
//                         dropped datagram or an RMS restart
// The dashboard shows the node's number. The difference is surfaced as
// "unobserved", never hidden, because a management overview that quietly
// under-reports a live match is worse than one that admits a gap.
// ─────────────────────────────────────────────────────────────────────────────

#include "RmsProtocol.h"

#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>

namespace ta {
namespace rms {

// Result of offering one accepted-shot event to a node's ledger.
enum class ShotIngest {
    Accepted,             // new shot, arrived in order
    AcceptedOutOfOrder,   // new shot, sequence lower than one already held
    DuplicateSuppressed,  // same eventId, or same sequence — displayed once
    SequenceConflict,     // same sequence, DIFFERENT eventId — suppressed+flagged
    SessionRestarted      // shot belongs to a new sessionId; ledger re-based
};

// Per-session shot ledger. Ordered by shotSequence, so out-of-order arrival
// cannot corrupt the displayed order — #18 arriving before #17 lands in the
// right slot and the gap is visible until #17 turns up.
class ShotLedger
{
public:
    void rebase(const QString& sessionId);

    ShotIngest ingest(const AcceptedShot& shot);

    QString sessionId() const { return m_sessionId; }
    int  observedCount() const { return m_bySequence.size(); }
    int  highestSequence() const { return m_highestSequence; }
    int  duplicatesSuppressed() const { return m_duplicatesSuppressed; }
    int  outOfOrderAccepted() const { return m_outOfOrderAccepted; }
    int  sequenceConflicts() const { return m_sequenceConflicts; }

    // Sequence numbers between 1 and highestSequence that RMS has not seen.
    QList<int> missingSequences() const;

    // Sum of the NODE-COMPUTED scores RMS has actually observed. This is a
    // display convenience for the shot list only; the authoritative running
    // total is NodeStatus::totalScore, reported by the node.
    double observedScoreSum() const;

    // Shots in sequence order, oldest first.
    QList<AcceptedShot> shotsInOrder() const;
    // The most recently RECEIVED shot (arrival order, not sequence order).
    AcceptedShot latestReceived() const { return m_latestReceived; }
    bool hasShots() const { return !m_bySequence.isEmpty(); }

private:
    QString m_sessionId;
    QSet<QString> m_seenEventIds;
    QMap<int, AcceptedShot> m_bySequence;
    AcceptedShot m_latestReceived;
    int m_highestSequence = 0;
    int m_duplicatesSuppressed = 0;
    int m_outOfOrderAccepted = 0;
    int m_sequenceConflicts = 0;
};

class TargetNodeRecord
{
public:
    // Identity
    QString nodeId;
    QString bootId;
    QString laneId;
    QString deviceIdentity;
    QString appVersion;
    QString productIdentity;

    // Latest reported state
    QString sessionId;
    QString programmeId;
    QString rulesetId;
    QString targetStandardId;
    QString athleteName;
    QString position;
    ConnectionState connection = ConnectionState::Unknown;
    MatchPhase      phase      = MatchPhase::Unknown;
    int     shotsAcceptedByNode = 0;
    int     shotsExpected       = -1;
    double  totalScoreByNode    = 0.0;
    QString health;
    quint64 lastStatusSeq = 0;

    // Boot identities already superseded. A datagram from one of these is a
    // straggler from a previous run of the node's application — it must be
    // dropped, not mistaken for a second restart, which would reset the
    // stale-status guard and let old state overwrite current state.
    QStringList priorBootIds;

    // Observation bookkeeping
    qint64 firstSeenUtcMs = 0;
    qint64 lastSeenUtcMs  = 0;
    int    nodeRestarts   = 0;   // bootId changes observed
    int    offlineEpisodes = 0;  // transitions live → Offline
    int    staleStatusDropped = 0;
    int    staleBootDropped   = 0;

    ShotLedger ledger;

    bool isOffline() const { return connection == ConnectionState::Offline; }

    // What RMS knows it has NOT seen: the node says N accepted, RMS holds M.
    int unobservedShotCount() const
    {
        const int diff = shotsAcceptedByNode - ledger.observedCount();
        return diff > 0 ? diff : 0;
    }
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_TARGETNODERECORD_H
