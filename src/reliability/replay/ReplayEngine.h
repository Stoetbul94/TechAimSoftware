#ifndef TA_REL_REPLAYENGINE_H
#define TA_REL_REPLAYENGINE_H

// Session Reliability Layer — replay engine (M3, spec §13/§15).
//
// Reconstructs the authoritative SessionState from a journal EXCLUSIVELY by
// folding events through the pure reducer. This is the only sanctioned way
// to rebuild state after a crash: Journal → Validator → Reducer → State.
// Nothing here deserializes controller internals; the reducer is the sole
// authority (architecture rule, M3).
//
// Snapshot optimization (spec §13): when the valid prefix contains a
// StateSnapshot, replay may start from the snapshot's embedded reducer state
// and fold only the tail (the reducer already guarantees the snapshot equals
// the fold-from-zero, so the result is identical). A drift check folds from
// zero as well and compares, on request.

#include "reliability/journal/JournalValidator.h"
#include "reliability/reducer/SessionState.h"

namespace ta {
namespace rel {

struct ReplayResult {
    bool ok = false;
    SessionState state;         // the reducer-rebuilt state (sole authority)
    quint64 lastAppliedSeq = 0;
    int eventsFolded = 0;       // events folded on top of the start point
    bool usedSnapshot = false;  // started from a StateSnapshot
    ErrorInfo error;            // set when !ok
};

namespace ReplayEngine {

// Fold a validated prefix (JournalValidator::validEnvelopes) through the
// reducer. When useSnapshot is true, starts from the last StateSnapshot in
// the prefix; otherwise folds from zero. A reducer rejection during replay
// means the journal is internally inconsistent — returns !ok with the seq.
ReplayResult replay(const QVector<EventEnvelope>& validPrefix,
                    bool useSnapshot = true);

// Convenience: validate + replay in one call.
ReplayResult replayBytes(const QByteArray& journalBytes,
                         const QString& expectedSessionId = QString(),
                         bool useSnapshot = true);
ReplayResult replayFile(const QString& path,
                        const QString& expectedSessionId = QString(),
                        bool useSnapshot = true);

// Drift detector (spec §13): fold-from-zero must equal snapshot-based
// replay. Returns true when they agree (or there is no snapshot).
bool snapshotsAgreeWithFold(const QVector<EventEnvelope>& validPrefix);

} // namespace ReplayEngine

} // namespace rel
} // namespace ta

#endif // TA_REL_REPLAYENGINE_H
