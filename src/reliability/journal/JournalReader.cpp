#include "JournalReader.h"

#include "reliability/events/EventSerializer.h"

#include <QFile>
#include <QIODevice>
#include <QStringDecoder>

namespace ta {
namespace rel {

namespace {

bool isValidUtf8(const QByteArray& bytes)
{
    QStringDecoder decoder(QStringDecoder::Utf8,
                           QStringDecoder::Flag::Stateless);
    const QString decoded = decoder.decode(bytes);
    Q_UNUSED(decoded);
    return !decoder.hasError();
}

} // namespace

JournalReadResult JournalReader::readBytes(const QByteArray& bytes)
{
    JournalReadResult result;
    result.fileOk = true;

    if (bytes.isEmpty())
        return result;

    qint64 lineNumber = 0;
    int start = 0;
    while (start < bytes.size()) {
        const int lf = bytes.indexOf('\n', start);
        const bool hasLf = lf >= 0;
        const int end = hasLf ? lf : bytes.size();
        QByteArray raw = bytes.mid(start, end - start);
        // Tolerate a CR left by CRLF-mangling transfer tools (documented).
        if (raw.endsWith('\r'))
            raw.chop(1);

        ++lineNumber;
        JournalLine line;
        line.lineNumber = lineNumber;
        line.rawBytes = raw;
        line.hasLineFeed = hasLf;
        line.isEmpty = raw.isEmpty();

        if (!hasLf)
            result.finalLineTruncated = true;

        if (!line.isEmpty) {
            if (!isValidUtf8(raw)) {
                line.parsed = false;
                line.error = ReliabilityResult::failure(
                    ReliabilityError::InvalidJson,
                    QStringLiteral("Journal line is not valid UTF-8."),
                    QStringLiteral("invalid UTF-8 byte sequence in line %1")
                        .arg(lineNumber)).error;
            } else {
                const ReliabilityResult pr =
                    EventSerializer::deserializeEnvelope(raw, &line.envelope);
                line.parsed = pr.ok;
                if (!pr.ok)
                    line.error = pr.error;
                else
                    ++result.parsedCount;
            }
        }

        result.lines.append(line);
        if (!hasLf)
            break;
        start = lf + 1;
        if (start == bytes.size())
            break;   // file ended exactly after the final LF — no extra line
    }

    return result;
}

JournalReadResult JournalReader::readDevice(QIODevice& device)
{
    JournalReadResult result;
    if (!device.isOpen() && !device.open(QIODevice::ReadOnly)) {
        result.fileOk = false;
        result.error = ReliabilityResult::failure(ReliabilityError::FileOpenFailed,
            QStringLiteral("The session journal could not be opened."),
            device.errorString()).error;
        return result;
    }
    const QByteArray bytes = device.readAll();
    return readBytes(bytes);
}

JournalReadResult JournalReader::readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        JournalReadResult result;
        result.fileOk = false;
        result.error = ReliabilityResult::failure(ReliabilityError::FileOpenFailed,
            QStringLiteral("The session journal could not be opened."),
            file.errorString(), path).error;
        return result;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        JournalReadResult result;
        result.fileOk = false;
        result.error = ReliabilityResult::failure(ReliabilityError::FileReadFailed,
            QStringLiteral("The session journal could not be read."),
            file.errorString(), path).error;
        return result;
    }
    return readBytes(bytes);
}

} // namespace rel
} // namespace ta
