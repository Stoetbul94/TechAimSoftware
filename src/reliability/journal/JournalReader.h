#ifndef TA_REL_JOURNALREADER_H
#define TA_REL_JOURNALREADER_H

// Session Reliability Layer — journal reader (M1, step 11).
//
// Streams a journal line-by-line, preserving the raw bytes of every line
// for diagnostics. STRICTLY NON-MUTATING: it never truncates, repairs or
// rewrites anything (recovery actions are M3, coordinated elsewhere).
//
// Line handling:
//   - LF is the separator; a final line WITHOUT a trailing LF is flagged
//     as the torn-tail signature (finalLineTruncated)
//   - a lone trailing '\r' before LF is stripped tolerantly (a journal
//     mangled into CRLF by transfer tools still parses; validation stays
//     structural, see JournalValidator)
//   - empty lines are flagged isEmpty, distinguished from malformed ones
//   - invalid UTF-8 is detected per line and reported typed

#include "reliability/core/ReliabilityResult.h"
#include "reliability/events/EventEnvelope.h"

#include <QByteArray>
#include <QString>
#include <QVector>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace ta {
namespace rel {

struct JournalLine {
    qint64 lineNumber = 0;         // 1-based
    QByteArray rawBytes;           // exact bytes, LF (and tolerated CR) removed
    bool hasLineFeed = true;
    bool isEmpty = false;
    bool parsed = false;
    EventEnvelope envelope;        // valid only when parsed
    ErrorInfo error;               // set when !parsed && !isEmpty
};

struct JournalReadResult {
    bool fileOk = false;           // the medium was readable end-to-end
    ErrorInfo error;               // set when !fileOk
    QVector<JournalLine> lines;
    bool finalLineTruncated = false;
    int parsedCount = 0;
};

namespace JournalReader {

JournalReadResult readFile(const QString& path);
JournalReadResult readDevice(QIODevice& device);
JournalReadResult readBytes(const QByteArray& bytes);

} // namespace JournalReader

} // namespace rel
} // namespace ta

#endif // TA_REL_JOURNALREADER_H
