// M1 — journal reader tests (step 11): clean file, empty file, truncated
// tail, malformed internal line, invalid UTF-8, empty-vs-malformed lines,
// raw byte preservation, QByteArray/QIODevice sources, non-mutation.

#include "reliability/journal/JournalReader.h"
#include "test_support.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

using namespace ta::rel;

namespace {

QByteArray smallJournal()
{
    MemoryJournalFile file;
    testjournal::writeScript(file, {
        testjournal::sessionStarted(Discipline::AirRifle10m,
                                    QStringLiteral("60"),
                                    QStringLiteral("Athlete")),
        SightingStarted{1},
        SighterAccepted{testjournal::shot(0, 92)},
    });
    return file.data;
}

} // namespace

void run_reader_tests()
{
    std::printf("--- journal reader tests ---\n");

    const QByteArray journal = smallJournal();

    // clean bytes
    {
        const JournalReadResult r = JournalReader::readBytes(journal);
        check(r.fileOk && r.lines.size() == 3 && r.parsedCount == 3,
              "clean journal reads 3 parsed lines");
        check(!r.finalLineTruncated, "trailing LF means no truncation flag");
        check(r.lines[0].lineNumber == 1 && r.lines[2].lineNumber == 3,
              "line numbers are 1-based");
        // raw bytes preserved exactly (reader never mutates)
        QByteArray reassembled;
        for (const JournalLine& l : r.lines) {
            reassembled += l.rawBytes;
            reassembled += '\n';
        }
        check(reassembled == journal, "raw line bytes preserved exactly");
    }

    // empty file
    {
        const JournalReadResult r = JournalReader::readBytes(QByteArray());
        check(r.fileOk && r.lines.isEmpty() && !r.finalLineTruncated,
              "empty journal reads as zero lines");
    }

    // truncated tail (no trailing LF)
    {
        QByteArray torn = journal;
        torn.chop(25);
        const JournalReadResult r = JournalReader::readBytes(torn);
        check(r.finalLineTruncated, "missing final LF sets finalLineTruncated");
        check(r.lines.size() == 3 && !r.lines[2].hasLineFeed
                  && !r.lines[2].parsed,
              "torn final line kept unparsed with its raw bytes");
        const int lastLf = torn.lastIndexOf('\n');
        check(r.lines[2].rawBytes == torn.mid(lastLf + 1),
              "torn fragment bytes preserved");
    }

    // malformed internal line vs empty line are distinguished
    {
        QByteArray mixed = journal;
        const int firstLf = mixed.indexOf('\n');
        mixed.insert(firstLf + 1, "not json at all\n");
        const JournalReadResult r = JournalReader::readBytes(mixed);
        check(r.lines.size() == 4 && !r.lines[1].parsed && !r.lines[1].isEmpty
                  && r.lines[1].error.code == ReliabilityError::InvalidJson,
              "malformed interior line reported typed, not empty");

        QByteArray withBlank = journal;
        withBlank.insert(firstLf + 1, "\n");
        const JournalReadResult rb = JournalReader::readBytes(withBlank);
        check(rb.lines.size() == 4 && rb.lines[1].isEmpty
                  && !rb.lines[1].parsed
                  && rb.lines[1].error.code == ReliabilityError::None,
              "blank line flagged isEmpty with no error");
    }

    // invalid UTF-8 detected per line
    {
        QByteArray bad = journal;
        const int firstLf = bad.indexOf('\n');
        QByteArray junk = "{\"sv\":1,\"x\":\"\xFF\xFE\xC0\"}\n";
        bad.insert(firstLf + 1, junk);
        const JournalReadResult r = JournalReader::readBytes(bad);
        check(r.lines.size() == 4 && !r.lines[1].parsed
                  && r.lines[1].error.technicalDetail.contains(
                         QStringLiteral("UTF-8")),
              "invalid UTF-8 line reported as such",
              r.lines[1].error.technicalDetail);
    }

    // QIODevice source
    {
        QByteArray copy = journal;
        QBuffer buffer(&copy);
        const JournalReadResult r = JournalReader::readDevice(buffer);
        check(r.fileOk && r.parsedCount == 3, "QIODevice source reads identically");
    }

    // file source + non-mutation guarantee
    {
        const QString dir = QDir::temp().filePath(
            QStringLiteral("techaim_m1_reader_%1")
                .arg(QCoreApplication::applicationPid()));
        QDir().mkpath(dir);
        const QString path = dir + QStringLiteral("/read.jsonl");
        {
            QFile f(path);
            f.open(QIODevice::WriteOnly);
            f.write(journal);
        }
        const JournalReadResult r = JournalReader::readFile(path);
        check(r.fileOk && r.parsedCount == 3, "file source reads identically");
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        check(f.readAll() == journal, "reader never mutates the file");
        f.close();
        const JournalReadResult missing =
            JournalReader::readFile(dir + QStringLiteral("/absent.jsonl"));
        check(!missing.fileOk
                  && missing.error.code == ReliabilityError::FileOpenFailed,
              "missing file reported typed");
        QDir(dir).removeRecursively();
    }
}
