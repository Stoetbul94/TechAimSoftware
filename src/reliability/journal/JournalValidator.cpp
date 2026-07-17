#include "JournalValidator.h"

#include "HashChain.h"
#include "reliability/events/EventSerializer.h"

namespace ta {
namespace rel {

namespace {

struct Failure {
    bool failed = false;
    JournalClassification classification = JournalClassification::Clean;
    ErrorInfo error;
    qint64 line = -1;
    qint64 seq = -1;
};

// Position-sensitive classification: a problem confined to the FINAL line
// is the torn-tail signature (safe to truncate); interior problems are
// internal corruption; version/session codes force their own class.
JournalClassification classify(ReliabilityError code, bool isLastLine)
{
    switch (code) {
    case ReliabilityError::SchemaTooNew:
    case ReliabilityError::UnsupportedEventType:
    case ReliabilityError::UnsupportedEventVersion:
        return JournalClassification::UnsupportedVersion;
    case ReliabilityError::SessionMismatch:
        return JournalClassification::SessionMismatch;
    default:
        return isLastLine ? JournalClassification::TailTruncated
                          : JournalClassification::CorruptInternal;
    }
}

} // namespace

ValidationReport JournalValidator::validate(const JournalReadResult& read,
                                            const QString& expectedSessionId)
{
    ValidationReport report;
    report.totalLines = read.lines.size();

    if (!read.fileOk) {
        report.classification = JournalClassification::CorruptInternal;
        report.corruptionInternal = true;
        report.error = read.error;
        return report;
    }
    if (read.lines.isEmpty()) {
        report.classification = JournalClassification::Empty;
        return report;
    }

    QByteArray prevHash = HashChain::genesisPreviousHash();
    QString sid = expectedSessionId;
    quint64 expectedSeq = 0;
    Failure failure;

    auto failAt = [&](const JournalLine& line, ReliabilityError code,
                      const QString& detail, qint64 seq) {
        failure.failed = true;
        failure.line = line.lineNumber;
        failure.seq = seq;
        const bool isLast = line.lineNumber == read.lines.size();
        failure.classification = classify(code, isLast);
        // A torn tail is reported under its own typed code; the underlying
        // cause is preserved in the technical detail.
        if (failure.classification == JournalClassification::TailTruncated) {
            failure.error = ReliabilityResult::failure(
                ReliabilityError::TruncatedTail,
                QStringLiteral("The final journal line is incomplete "
                               "(interrupted write)."),
                QStringLiteral("line %1: [%2] %3")
                    .arg(line.lineNumber)
                    .arg(QLatin1String(reliabilityErrorName(code)))
                    .arg(detail),
                QString(), seq).error;
        } else {
            failure.error = ReliabilityResult::failure(code,
                QStringLiteral("The session journal failed validation."),
                QStringLiteral("line %1: %2").arg(line.lineNumber).arg(detail),
                QString(), seq).error;
        }
    };

    for (const JournalLine& line : read.lines) {
        // (1) blank lines never occur in a valid journal
        if (line.isEmpty) {
            failAt(line, ReliabilityError::CorruptJournal,
                   QStringLiteral("blank line"), -1);
            break;
        }
        // (2) unparseable line — typed cause from the reader
        if (!line.parsed) {
            failAt(line, line.error.code, line.error.technicalDetail,
                   line.error.sequence);
            break;
        }

        const EventEnvelope& env = line.envelope;
        const qint64 seqAsSigned = static_cast<qint64>(env.seq);

        // (3) session identity
        if (expectedSeq == 0) {
            if (!sid.isEmpty() && env.sessionId != sid) {
                failAt(line, ReliabilityError::SessionMismatch,
                       QStringLiteral("header sid '%1' != expected '%2'")
                           .arg(env.sessionId, sid), seqAsSigned);
                break;
            }
            sid = env.sessionId;
            // The header payload must agree with its own envelope.
            if (const auto* started = std::get_if<SessionStarted>(&env.payload)) {
                if (started->sessionId != env.sessionId) {
                    failAt(line, ReliabilityError::SessionMismatch,
                           QStringLiteral("payload sessionId '%1' != sid '%2'")
                               .arg(started->sessionId, env.sessionId),
                           seqAsSigned);
                    break;
                }
            }
        } else if (env.sessionId != sid) {
            failAt(line, ReliabilityError::SessionMismatch,
                   QStringLiteral("sid '%1' != session '%2'")
                       .arg(env.sessionId, sid), seqAsSigned);
            break;
        }

        // (4) sequence discipline: strictly seq == line index (M1 — the
        // degraded-gap allowance arrives with M2's markers)
        if (env.seq != expectedSeq) {
            if (expectedSeq > 0 && env.seq == expectedSeq - 1) {
                failAt(line, ReliabilityError::DuplicateSequence,
                       QStringLiteral("seq %1 repeated").arg(env.seq),
                       seqAsSigned);
            } else if (env.seq > expectedSeq) {
                failAt(line, ReliabilityError::SequenceGap,
                       QStringLiteral("seq %1, expected %2 (gap)")
                           .arg(env.seq).arg(expectedSeq), seqAsSigned);
            } else {
                failAt(line, ReliabilityError::InvalidSequence,
                       QStringLiteral("seq %1, expected %2")
                           .arg(env.seq).arg(expectedSeq), seqAsSigned);
            }
            break;
        }

        // (5) hash chain: ph must equal the previous line's h
        if (env.previousHash != prevHash) {
            failAt(line, ReliabilityError::BrokenHashChain,
                   QStringLiteral("ph '%1' != previous h '%2'")
                       .arg(QString::fromLatin1(env.previousHash),
                            QString::fromLatin1(prevHash)), seqAsSigned);
            break;
        }

        // (6) current hash: deterministic re-serialization + recompute
        QByteArray core;
        const ReliabilityResult sr =
            EventSerializer::serializeCoreWithoutCurrentHash(env, &core);
        if (!sr.ok) {
            failAt(line, sr.error.code, sr.error.technicalDetail, seqAsSigned);
            break;
        }
        const QByteArray recomputed =
            HashChain::computeLineHash(env.previousHash, core);
        if (recomputed != env.currentHash) {
            failAt(line, ReliabilityError::HashMismatch,
                   QStringLiteral("stored h '%1' != recomputed '%2'")
                       .arg(QString::fromLatin1(env.currentHash),
                            QString::fromLatin1(recomputed)), seqAsSigned);
            break;
        }

        // (7) snapshot sequencing: the captured state must equal the fold
        // of everything before this envelope
        if (const auto* snap = std::get_if<StateSnapshot>(&env.payload)) {
            if (snap->lastAppliedSeq != env.seq - 1) {
                failAt(line, ReliabilityError::InvalidEvent,
                       QStringLiteral("snapshot lastAppliedSeq %1 != seq-1 (%2)")
                           .arg(snap->lastAppliedSeq).arg(env.seq - 1),
                       seqAsSigned);
                break;
            }
        }

        // valid line — advance
        if (std::holds_alternative<MatchCompleted>(env.payload))
            report.sawMatchCompleted = true;
        if (std::holds_alternative<SessionClosed>(env.payload))
            report.sawSessionClosed = true;
        report.validEnvelopes.append(env);
        report.lastValidSeq = env.seq;
        ++report.validPrefixCount;
        prevHash = env.currentHash;
        ++expectedSeq;
    }

    report.sessionId = sid;
    if (!failure.failed) {
        report.classification = JournalClassification::Clean;
        return report;
    }

    report.classification = failure.classification;
    report.error = failure.error;
    report.firstInvalidLine = failure.line;
    report.firstInvalidSeq = failure.seq;
    report.tailSafelyTruncatable =
        failure.classification == JournalClassification::TailTruncated;
    report.corruptionInternal =
        failure.classification == JournalClassification::CorruptInternal;
    return report;
}

ValidationReport JournalValidator::validateBytes(const QByteArray& journalBytes,
                                                 const QString& expectedSessionId)
{
    return validate(JournalReader::readBytes(journalBytes), expectedSessionId);
}

ValidationReport JournalValidator::validateFile(const QString& path,
                                                const QString& expectedSessionId)
{
    return validate(JournalReader::readFile(path), expectedSessionId);
}

} // namespace rel
} // namespace ta
