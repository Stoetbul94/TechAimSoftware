#ifndef TA_REL_JOURNALVALIDATOR_H
#define TA_REL_JOURNALVALIDATOR_H

// Session Reliability Layer — journal validator (M1, step 12).
//
// Classifies a whole journal without mutating it (truncation/repair is the
// RecoveryCoordinator's call, M3). Verification is STRUCTURED: each line
// is deserialized, its core is deterministically re-serialized, and the
// chained hash is recomputed — never string-splitting on ',"h":"'.
//
// Classification rules (position-sensitive, documented in the M1 doc):
//   Empty              no lines at all
//   Clean              every line valid, chain intact
//   TailTruncated      the ONLY problem is on the FINAL line (torn/
//                      malformed/failed-hash/duplicate tail) — safely
//                      truncatable, everything before it valid
//   UnsupportedVersion a line carries a newer schema (sv), newer payload
//                      version (pv) or an unknown event type — written by
//                      a newer build; never guessed at
//   SessionMismatch    a line's sid differs from the header's (or the
//                      caller-expected) session id
//   CorruptInternal    any other problem on a NON-final line
//
// M1 note: the degraded-mode seq-gap allowance (spec §9D) does not exist
// yet — every gap is invalid until M2 introduces the Degraded markers.

#include "JournalReader.h"

#include <QString>
#include <QVector>

namespace ta {
namespace rel {

enum class JournalClassification {
    Empty = 0,
    Clean,
    TailTruncated,
    CorruptInternal,
    UnsupportedVersion,
    SessionMismatch
};

inline const char* journalClassificationName(JournalClassification c)
{
    switch (c) {
    case JournalClassification::Empty:              return "Empty";
    case JournalClassification::Clean:              return "Clean";
    case JournalClassification::TailTruncated:      return "TailTruncated";
    case JournalClassification::CorruptInternal:    return "CorruptInternal";
    case JournalClassification::UnsupportedVersion: return "UnsupportedVersion";
    case JournalClassification::SessionMismatch:    return "SessionMismatch";
    }
    return "Unknown";
}

struct ValidationReport {
    JournalClassification classification = JournalClassification::Empty;
    int totalLines = 0;
    int validPrefixCount = 0;          // leading fully-valid envelopes
    qint64 firstInvalidLine = -1;      // 1-based; -1 = none
    qint64 firstInvalidSeq = -1;       // seq at the first invalid line, if known
    ErrorInfo error;                   // the first failure, typed
    bool tailSafelyTruncatable = false;
    bool corruptionInternal = false;
    QString sessionId;                 // from the header, when readable
    quint64 lastValidSeq = 0;          // seq of the last valid envelope
    bool sawMatchCompleted = false;    // completion state, where determinable
    bool sawSessionClosed = false;
    QVector<EventEnvelope> validEnvelopes;   // the valid prefix (replay input, M3)
};

namespace JournalValidator {

// expectedSessionId: empty = adopt the header's sid.
ValidationReport validate(const JournalReadResult& read,
                          const QString& expectedSessionId = QString());
ValidationReport validateBytes(const QByteArray& journalBytes,
                               const QString& expectedSessionId = QString());
ValidationReport validateFile(const QString& path,
                              const QString& expectedSessionId = QString());

} // namespace JournalValidator

} // namespace rel
} // namespace ta

#endif // TA_REL_JOURNALVALIDATOR_H
