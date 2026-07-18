#include "ReplayEngine.h"

#include "reliability/reducer/SessionReducer.h"

namespace ta {
namespace rel {

namespace {

// Fold `events` in order through the reducer starting from `start`.
ReplayResult foldFrom(const SessionState& start, quint64 startLastSeq,
                      const QVector<EventEnvelope>& events, int firstIndex,
                      bool usedSnapshot)
{
    ReplayResult r;
    r.state = start;
    r.lastAppliedSeq = startLastSeq;
    r.usedSnapshot = usedSnapshot;
    for (int i = firstIndex; i < events.size(); ++i) {
        // Replay tolerates the declared-drop gaps the validator has already
        // vetted (degraded-mode aux drops, §9E) and the jump from a snapshot's
        // embedded lastSeq: align lastSeq so the reducer's dense-sequence
        // DEFENCE accepts this (validated) next event. Live submit still gets
        // the strict check — the store always assigns dense seqs there.
        if (events[i].seq > r.state.lastSeq + 1)
            r.state.lastSeq = events[i].seq - 1;
        const ReduceResult rr = SessionReducer::apply(r.state, events[i]);
        if (!rr.ok) {
            r.ok = false;
            r.error = rr.error;
            r.error.sequence = static_cast<qint64>(events[i].seq);
            return r;
        }
        r.state = rr.state;
        r.lastAppliedSeq = events[i].seq;
        ++r.eventsFolded;
    }
    r.ok = true;
    return r;
}

// Index of the last StateSnapshot in the prefix, or -1.
int lastSnapshotIndex(const QVector<EventEnvelope>& events)
{
    for (int i = events.size() - 1; i >= 0; --i)
        if (std::holds_alternative<StateSnapshot>(events[i].payload))
            return i;
    return -1;
}

} // namespace

ReplayResult ReplayEngine::replay(const QVector<EventEnvelope>& validPrefix,
                                  bool useSnapshot)
{
    if (validPrefix.isEmpty()) {
        ReplayResult r;
        r.ok = true;   // an empty journal replays to the default (empty) state
        return r;
    }

    if (useSnapshot) {
        const int snapIdx = lastSnapshotIndex(validPrefix);
        if (snapIdx >= 0) {
            const auto& snap = std::get<StateSnapshot>(validPrefix[snapIdx].payload);
            SessionState start;
            const ReliabilityResult dr =
                deserializeSessionState(snap.stateJson, &start);
            if (dr.ok) {
                // The snapshot's embedded state equals the fold up to the
                // event BEFORE the snapshot; the snapshot event itself occupies
                // its own seq, so advance lastSeq to it before folding the tail.
                start.lastSeq = validPrefix[snapIdx].seq;
                return foldFrom(start, validPrefix[snapIdx].seq,
                                validPrefix, snapIdx + 1, /*usedSnapshot*/ true);
            }
            // A corrupt snapshot payload — fall through to fold-from-zero.
        }
    }
    return foldFrom(SessionState(), 0, validPrefix, 0, /*usedSnapshot*/ false);
}

ReplayResult ReplayEngine::replayBytes(const QByteArray& journalBytes,
                                       const QString& expectedSessionId,
                                       bool useSnapshot)
{
    const ValidationReport rep =
        JournalValidator::validateBytes(journalBytes, expectedSessionId);
    return replay(rep.validEnvelopes, useSnapshot);
}

ReplayResult ReplayEngine::replayFile(const QString& path,
                                      const QString& expectedSessionId,
                                      bool useSnapshot)
{
    const ValidationReport rep =
        JournalValidator::validateFile(path, expectedSessionId);
    return replay(rep.validEnvelopes, useSnapshot);
}

bool ReplayEngine::snapshotsAgreeWithFold(const QVector<EventEnvelope>& validPrefix)
{
    const ReplayResult viaSnapshot = replay(validPrefix, /*useSnapshot*/ true);
    const ReplayResult viaFold = replay(validPrefix, /*useSnapshot*/ false);
    if (!viaSnapshot.ok || !viaFold.ok)
        return false;
    return viaSnapshot.state == viaFold.state;
}

} // namespace rel
} // namespace ta
