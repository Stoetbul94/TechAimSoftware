#ifndef TA_REL_RECOVERYTYPES_H
#define TA_REL_RECOVERYTYPES_H

// Session Reliability Layer — recovery value types (M3).
//
// RecoveryClass is the operator-facing verdict derived from the validator's
// classification + the length of the recoverable prefix. RecoveredMatchState
// is the reducer-rebuilt state a controller consumes to resume — it carries
// ONLY the reducer's SessionState (the sole authority) plus recovery
// metadata; never any serialized controller internals.

#include "reliability/reducer/SessionState.h"

#include <QString>

namespace ta {
namespace rel {

// Operator-facing recovery verdict (spec Task 3).
enum class RecoveryClass {
    Clean,        // fully valid — resume directly
    Recoverable,  // valid prefix + a torn/incomplete tail — resume up to it
    Corrupt,      // interior damage; only a partial prefix survives (wizard)
    Fatal         // newer schema / session mismatch / no usable header
};

inline const char* recoveryClassName(RecoveryClass c)
{
    switch (c) {
    case RecoveryClass::Clean:       return "Clean";
    case RecoveryClass::Recoverable: return "Recoverable";
    case RecoveryClass::Corrupt:     return "Corrupt";
    case RecoveryClass::Fatal:       return "Fatal";
    }
    return "Unknown";
}

// Metadata for the recovery dialog — everything the operator needs to decide,
// derived from the replayed reducer state + the file.
struct RecoveryCandidate {
    QString sessionId;
    QString journalPath;
    QString athlete;
    Discipline discipline = Discipline::None;
    QString disciplineLabel;
    QString matchType;
    QString operatingMode;        // F10: mode the session STARTED in (empty = Unknown/Legacy)
    QString startedAtIso;         // SessionStarted.createdAtIso
    QString lastEventWallIso;     // tw of the last valid line
    QString lastModifiedIso;      // file mtime
    qint16 currentStageId = -1;
    qint32 officialShots = 0;     // recovered so far
    qint32 expectedShots = 0;     // from config
    qint32 totalTenths = 0;
    qint8 phaseId = 0;            // reducer MatchPhase at the crash
    qint64 remainingMs = -1;      // frozen phase-clock remaining (-1 = none)
    // Phase E: unresolved-incident projection (generic reducer values).
    bool openIncident = false;    // an incident still requires authorisation
    qint8 incidentTypeId = 0;
    qint8 incidentScopeId = 0;
    qint8 incidentCreditDecision = 0;   // 0 pending | 1 none | 2 granted
    qint64 incidentCreditMs = 0;
    quint64 lastValidSeq = 0;
    RecoveryClass recoveryClass = RecoveryClass::Fatal;
    QString validationDetail;     // human-facing reason when not Clean
    bool complete = false;        // saw MatchCompleted
    bool resumable = false;       // Resume allowed (Clean/Recoverable)
};

// The reducer-rebuilt state handed to a controller for resume. It is the
// SessionState and nothing else authoritative.
struct RecoveredMatchState {
    SessionState state;           // rebuilt EXCLUSIVELY via the reducer
    QString sessionId;
    QString journalPath;
    quint64 lastValidSeq = 0;
    QString lastEventWallIso;
    QByteArray lastLineHash;      // ph seed for resumed appends
    qint64 validByteLength = -1;  // byte length of the valid prefix; the file
                                  // is truncated here on resume if it has a
                                  // torn tail (-1 = unknown / do not truncate)
    // Replay-derived timing anchors (spec §16) — the monotonic-ms values the
    // controller needs to REBASE its clock so the countdown continues from
    // where it was at the crash (interruption time does not count). These are
    // read from the journal `tm` fields; the reducer stays authoritative for
    // everything else.
    qint64 lastEventMonoMs = 0;        // tm of the last valid event
    qint64 stageClockStartMonoMs = 0;  // tm the current firing window/clock began
    RecoveryClass recoveryClass = RecoveryClass::Clean;
};

} // namespace rel
} // namespace ta

#endif // TA_REL_RECOVERYTYPES_H
