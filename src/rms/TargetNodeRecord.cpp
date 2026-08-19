#include "TargetNodeRecord.h"

namespace ta {
namespace rms {

void ShotLedger::rebase(const QString& sessionId)
{
    m_sessionId = sessionId;
    m_seenEventIds.clear();
    m_bySequence.clear();
    m_latestReceived = AcceptedShot();
    m_highestSequence = 0;
    // Counters deliberately survive a rebase: they are lifetime observation
    // quality metrics for the node, not per-session scoring data.
}

ShotIngest ShotLedger::ingest(const AcceptedShot& shot)
{
    ShotIngest result = ShotIngest::Accepted;

    if (shot.sessionId != m_sessionId) {
        // A new session on the node. The old ledger is not "wrong" — it is
        // simply finished. RMS re-bases and keeps observing.
        rebase(shot.sessionId);
        result = ShotIngest::SessionRestarted;
    }

    // Transport-level duplicate: the same event delivered twice. UDP does
    // this routinely; it must show once.
    if (m_seenEventIds.contains(shot.eventId)) {
        ++m_duplicatesSuppressed;
        return ShotIngest::DuplicateSuppressed;
    }

    auto existing = m_bySequence.constFind(shot.shotSequence);
    if (existing != m_bySequence.constEnd()) {
        // Same slot, different eventId. Either the node re-issued the shot or
        // two nodes are colliding on one identity. Suppress and FLAG — the
        // first observation is kept, because overwriting an accepted shot on
        // the strength of a second datagram is exactly how a display starts
        // disagreeing with the target.
        ++m_duplicatesSuppressed;
        ++m_sequenceConflicts;
        return ShotIngest::SequenceConflict;
    }

    if (shot.shotSequence < m_highestSequence) {
        ++m_outOfOrderAccepted;
        if (result == ShotIngest::Accepted)
            result = ShotIngest::AcceptedOutOfOrder;
    }

    m_seenEventIds.insert(shot.eventId);
    m_bySequence.insert(shot.shotSequence, shot);
    m_latestReceived = shot;
    if (shot.shotSequence > m_highestSequence)
        m_highestSequence = shot.shotSequence;

    return result;
}

QList<int> ShotLedger::missingSequences() const
{
    QList<int> gaps;
    for (int s = 1; s <= m_highestSequence; ++s)
        if (!m_bySequence.contains(s))
            gaps.append(s);
    return gaps;
}

double ShotLedger::observedScoreSum() const
{
    double sum = 0.0;
    for (auto it = m_bySequence.constBegin(); it != m_bySequence.constEnd(); ++it)
        sum += it.value().authoritativeScore;
    return sum;
}

QList<AcceptedShot> ShotLedger::shotsInOrder() const
{
    return m_bySequence.values();   // QMap iterates by ascending key
}

} // namespace rms
} // namespace ta
