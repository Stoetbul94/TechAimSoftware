#ifndef TA_REL_RELIABILITYERROR_H
#define TA_REL_RELIABILITYERROR_H

// Session Reliability Layer — typed error codes (M1, spec §24 extended).
// Machine-readable first: every failure in the reliability core is one of
// these codes; QString appears only as human-facing message/detail fields
// on ErrorInfo (ReliabilityResult.h), never as the error identity.
// Header-only on purpose: core/ carries no translation units of its own.

namespace ta {
namespace rel {

enum class ReliabilityError {
    None = 0,
    // arguments / events
    InvalidArgument,
    InvalidEvent,
    UnsupportedEventType,
    UnsupportedEventVersion,
    // serialization
    SerializationFailed,
    DeserializationFailed,
    // file I/O
    FileOpenFailed,
    FileReadFailed,
    FileWriteFailed,
    FlushFailed,
    SyncFailed,
    // envelope / json structure
    InvalidJson,
    MissingField,
    InvalidFieldType,
    // sequencing
    InvalidSequence,
    DuplicateSequence,
    SequenceGap,
    // integrity
    HashMismatch,
    BrokenHashChain,
    SessionMismatch,
    SchemaTooNew,
    // reduction
    ReducerRejected,
    InvalidStateTransition,
    // whole-journal classification errors
    CorruptJournal,
    TruncatedTail
};

enum class ErrorSeverity {
    Info,       // expected/benign (e.g. TruncatedTail after power loss)
    Warning,    // recoverable degradation
    Error,      // operation failed; caller must handle
    Critical    // data integrity or availability at risk
};

inline const char* reliabilityErrorName(ReliabilityError code)
{
    switch (code) {
    case ReliabilityError::None:                    return "None";
    case ReliabilityError::InvalidArgument:         return "InvalidArgument";
    case ReliabilityError::InvalidEvent:            return "InvalidEvent";
    case ReliabilityError::UnsupportedEventType:    return "UnsupportedEventType";
    case ReliabilityError::UnsupportedEventVersion: return "UnsupportedEventVersion";
    case ReliabilityError::SerializationFailed:     return "SerializationFailed";
    case ReliabilityError::DeserializationFailed:   return "DeserializationFailed";
    case ReliabilityError::FileOpenFailed:          return "FileOpenFailed";
    case ReliabilityError::FileReadFailed:          return "FileReadFailed";
    case ReliabilityError::FileWriteFailed:         return "FileWriteFailed";
    case ReliabilityError::FlushFailed:             return "FlushFailed";
    case ReliabilityError::SyncFailed:              return "SyncFailed";
    case ReliabilityError::InvalidJson:             return "InvalidJson";
    case ReliabilityError::MissingField:            return "MissingField";
    case ReliabilityError::InvalidFieldType:        return "InvalidFieldType";
    case ReliabilityError::InvalidSequence:         return "InvalidSequence";
    case ReliabilityError::DuplicateSequence:       return "DuplicateSequence";
    case ReliabilityError::SequenceGap:             return "SequenceGap";
    case ReliabilityError::HashMismatch:            return "HashMismatch";
    case ReliabilityError::BrokenHashChain:         return "BrokenHashChain";
    case ReliabilityError::SessionMismatch:         return "SessionMismatch";
    case ReliabilityError::SchemaTooNew:            return "SchemaTooNew";
    case ReliabilityError::ReducerRejected:         return "ReducerRejected";
    case ReliabilityError::InvalidStateTransition:  return "InvalidStateTransition";
    case ReliabilityError::CorruptJournal:          return "CorruptJournal";
    case ReliabilityError::TruncatedTail:           return "TruncatedTail";
    }
    return "Unknown";
}

inline ErrorSeverity defaultSeverity(ReliabilityError code)
{
    switch (code) {
    case ReliabilityError::None:
        return ErrorSeverity::Info;
    case ReliabilityError::TruncatedTail:
        // The expected power-loss signature; recovery handles it routinely.
        return ErrorSeverity::Warning;
    case ReliabilityError::SchemaTooNew:
    case ReliabilityError::UnsupportedEventType:
    case ReliabilityError::UnsupportedEventVersion:
        return ErrorSeverity::Warning;
    case ReliabilityError::HashMismatch:
    case ReliabilityError::BrokenHashChain:
    case ReliabilityError::SessionMismatch:
    case ReliabilityError::CorruptJournal:
        return ErrorSeverity::Critical;
    case ReliabilityError::FileOpenFailed:
    case ReliabilityError::FileReadFailed:
    case ReliabilityError::FileWriteFailed:
    case ReliabilityError::FlushFailed:
    case ReliabilityError::SyncFailed:
        return ErrorSeverity::Critical;
    default:
        return ErrorSeverity::Error;
    }
}

inline bool defaultRecoverable(ReliabilityError code)
{
    switch (code) {
    // A torn tail is recoverable by design (truncate + resume, M3).
    case ReliabilityError::None:
    case ReliabilityError::TruncatedTail:
        return true;
    // I/O failures are retryable once the medium recovers (M2 queue).
    case ReliabilityError::FileOpenFailed:
    case ReliabilityError::FileReadFailed:
    case ReliabilityError::FileWriteFailed:
    case ReliabilityError::FlushFailed:
    case ReliabilityError::SyncFailed:
        return true;
    // Newer-schema journals are readable by a newer build, not by this one.
    case ReliabilityError::SchemaTooNew:
    case ReliabilityError::UnsupportedEventType:
    case ReliabilityError::UnsupportedEventVersion:
        return true;
    default:
        return false;
    }
}

} // namespace rel
} // namespace ta

#endif // TA_REL_RELIABILITYERROR_H
