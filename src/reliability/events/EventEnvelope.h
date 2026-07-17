#ifndef TA_REL_EVENTENVELOPE_H
#define TA_REL_EVENTENVELOPE_H

// Session Reliability Layer — event envelope (M1, spec §5).
//
// One envelope per journal line. Field short-names and their serialized
// order are FROZEN (EventSerializer):
//   sv  journal schema version (int, = 1)
//   pv  payload version (int)
//   sid session UUID (string)
//   lane lane identity (string, may be empty — always serialized)
//   seq sequence number (u64; 0 = session header = SessionStarted)
//   tw  UTC wall timestamp, ISO-8601 with milliseconds
//   tm  monotonic milliseconds since session start (i64)
//   t   event type identifier (string)
//   p   typed payload (object)
//   av  application version (string, seq 0 only)
//   dev device identity (string, seq 0 only, optional)
//   ph  previous line hash (32 lowercase hex; genesis for seq 0)
//   h   this line's hash (32 lowercase hex)
//
// `replayed` is IN-MEMORY ONLY — never serialized, never authoritative
// (spec §20: replay delivery marker).

#include "DomainEvent.h"

#include <QByteArray>
#include <QString>

namespace ta {
namespace rel {

// Journal schema version this build reads and writes.
inline constexpr qint32 kJournalSchemaVersion = 1;

// Truncated chained SHA-256 (spec decision D8): 128 bits = 32 hex chars.
inline constexpr int kHashHexLength = 32;

// The tw format written by producers (UTC; documented in the M1 doc).
inline constexpr const char* kWallTimestampFormat = "yyyy-MM-ddTHH:mm:ss.zzz";

// 32 lowercase hex characters — the only accepted hash spelling.
inline bool isWellFormedHashHex(const QByteArray& hex)
{
    if (hex.size() != kHashHexLength)
        return false;
    for (char c : hex) {
        const bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        if (!ok)
            return false;
    }
    return true;
}

struct EventEnvelope {
    qint32 schemaVersion = kJournalSchemaVersion;   // sv
    qint32 payloadVersion = 1;                      // pv
    QString sessionId;                              // sid
    QString lane;                                   // lane
    quint64 seq = 0;                                // seq
    QString wallTimestampIso;                       // tw
    qint64 monotonicMs = 0;                         // tm
    QString eventType;                              // t (must match payload)
    DomainEvent payload;                            // p
    QString appVersion;                             // av (seq 0 only)
    QString deviceId;                               // dev (seq 0 only, optional)
    QByteArray previousHash;                        // ph (32 hex)
    QByteArray currentHash;                         // h  (32 hex)

    bool replayed = false;                          // in-memory only

    // Structural validation of the envelope itself (payload validation is
    // separate — validateEvent()). Enforces the type/payload-conflict rule.
    ReliabilityResult validate() const
    {
        if (schemaVersion != kJournalSchemaVersion)
            return ReliabilityResult::failure(
                schemaVersion > kJournalSchemaVersion
                    ? ReliabilityError::SchemaTooNew
                    : ReliabilityError::InvalidEvent,
                QStringLiteral("Unsupported journal schema version."),
                QStringLiteral("sv=%1, supported=%2")
                    .arg(schemaVersion).arg(kJournalSchemaVersion),
                QString(), static_cast<qint64>(seq));
        if (sessionId.isEmpty())
            return ReliabilityResult::failure(ReliabilityError::InvalidEvent,
                QStringLiteral("Envelope has no session id."),
                QStringLiteral("sid empty"), QString(),
                static_cast<qint64>(seq));
        if (wallTimestampIso.isEmpty())
            return ReliabilityResult::failure(ReliabilityError::InvalidEvent,
                QStringLiteral("Envelope has no wall timestamp."),
                QStringLiteral("tw empty"), QString(),
                static_cast<qint64>(seq));
        if (monotonicMs < 0)
            return ReliabilityResult::failure(ReliabilityError::InvalidEvent,
                QStringLiteral("Envelope has a negative monotonic time."),
                QStringLiteral("tm=%1").arg(monotonicMs), QString(),
                static_cast<qint64>(seq));
        if (eventType != QLatin1String(eventTypeId(payload)))
            return ReliabilityResult::failure(ReliabilityError::InvalidEvent,
                QStringLiteral("Envelope event type conflicts with its payload."),
                QStringLiteral("t=%1, payload=%2")
                    .arg(eventType, QLatin1String(eventTypeId(payload))),
                QString(), static_cast<qint64>(seq));
        if (seq == 0 && !std::holds_alternative<SessionStarted>(payload))
            return ReliabilityResult::failure(ReliabilityError::InvalidEvent,
                QStringLiteral("Sequence 0 must be the session header."),
                QStringLiteral("seq 0 carries %1 instead of SessionStarted")
                    .arg(eventType),
                QString(), 0);
        return ReliabilityResult::success();
    }
};

} // namespace rel
} // namespace ta

#endif // TA_REL_EVENTENVELOPE_H
