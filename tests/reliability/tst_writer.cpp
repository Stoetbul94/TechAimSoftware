// M1 — journal writer tests (steps 10, 17): append semantics, durability
// policy, sync boundary, latency metrics, injected write/flush/sync
// failures, partial-write poisoning, real-file round trip.

#include "reliability/journal/JournalWriter.h"
#include "reliability/journal/JournalValidator.h"
#include "test_support.h"

#include <QCoreApplication>
#include <QDir>

using namespace ta::rel;

void run_writer_tests()
{
    std::printf("--- journal writer tests ---\n");

    // append assigns sequence, chains hashes, applies durability policy
    {
        MemoryJournalFile file;
        JournalWriter writer(testjournal::identity(), &file);
        check(writer.nextSequence() == 0
                  && writer.lastHash() == QByteArray(32, '0'),
              "fresh writer starts at seq 0 / genesis hash");

        AppendOutcome h = writer.append(
            testjournal::sessionStarted(Discipline::Prone50m,
                                        QStringLiteral("60"),
                                        QStringLiteral("A")),
            QString::fromLatin1(testjournal::kWall), 0);
        check(h.ok && h.envelope.seq == 0 && h.lineAppended,
              "header append ok at seq 0", h.error.technicalDetail);
        check(file.syncCount == 1,
              "SessionStarted (Sync class) reached the sync boundary");

        AppendOutcome s1 = writer.append(DomainEvent(SightingStarted{1}),
                                         QString::fromLatin1(testjournal::kWall),
                                         1000);
        AppendOutcome sighter = writer.append(
            DomainEvent(SighterAccepted{testjournal::shot(0, 97)}),
            QString::fromLatin1(testjournal::kWall), 2000);
        check(s1.ok && sighter.ok && sighter.envelope.seq == 2,
              "sequence advances per append");
        check(sighter.envelope.previousHash == s1.envelope.currentHash,
              "hash chain links consecutive envelopes");
        const int syncsBeforeSighter = 2;   // header + SightingStarted
        check(file.syncCount == syncsBeforeSighter,
              "sighter (Flush class) does NOT fsync (spec D2)",
              QString::number(file.syncCount));
        check(file.flushCount >= 3, "flush performed for flush-class events");

        AppendOutcome official = writer.append(
            DomainEvent(OfficialMatchStarted{1}),
            QString::fromLatin1(testjournal::kWall), 3000);
        AppendOutcome shot = writer.append(
            DomainEvent(ShotAccepted{testjournal::shot(1, 104)}),
            QString::fromLatin1(testjournal::kWall), 4000);
        check(official.ok && shot.ok, "official flow appends");
        check(file.syncCount == syncsBeforeSighter + 2,
              "official shot fsyncs (Sync class)");

        // metrics populated
        const WriterMetrics& m = writer.metrics();
        check(m.appendCount == 5 && m.failureCount == 0,
              "metrics count appends");
        check(m.lastAppendMicros >= 0 && m.lastSyncMicros >= 0
                  && m.totalAppendMicros >= 0,
              "latency metrics populated");

        // the produced journal validates Clean
        const ValidationReport rep = JournalValidator::validateBytes(file.data);
        check(rep.classification == JournalClassification::Clean
                  && rep.validPrefixCount == 5,
              "writer output validates Clean",
              QLatin1String(journalClassificationName(rep.classification)));
    }

    // header rules
    {
        MemoryJournalFile file;
        JournalWriter writer(testjournal::identity(), &file);
        AppendOutcome bad = writer.append(DomainEvent(SightingStarted{1}),
                                          QString::fromLatin1(testjournal::kWall),
                                          0);
        check(!bad.ok && bad.error.code == ReliabilityError::InvalidEvent,
              "non-header first event rejected", bad.error.technicalDetail);
        SessionStarted wrongSid = testjournal::sessionStarted(
            Discipline::Prone50m, QStringLiteral("60"), QStringLiteral("A"));
        wrongSid.sessionId = QStringLiteral("other-session");
        AppendOutcome mismatch = writer.append(DomainEvent(wrongSid),
            QString::fromLatin1(testjournal::kWall), 0);
        check(!mismatch.ok
                  && mismatch.error.code == ReliabilityError::SessionMismatch,
              "header sid mismatch rejected");
        check(writer.nextSequence() == 0 && file.data.isEmpty(),
              "failed appends leave writer state and file unchanged");
    }

    // clean write failure: nothing stored, retryable, state unchanged
    {
        MemoryJournalFile file;
        JournalWriter writer(testjournal::identity(), &file);
        writer.append(testjournal::sessionStarted(Discipline::Prone50m,
                                                  QStringLiteral("60"),
                                                  QStringLiteral("A")),
                      QString::fromLatin1(testjournal::kWall), 0);
        const QByteArray before = file.data;
        file.failWrite = true;
        AppendOutcome out = writer.append(DomainEvent(SightingStarted{1}),
            QString::fromLatin1(testjournal::kWall), 1000);
        check(!out.ok && !out.lineAppended
                  && out.error.code == ReliabilityError::FileWriteFailed,
              "write failure reported typed, line not appended");
        check(writer.nextSequence() == 1 && file.data == before
                  && !writer.failed(),
              "clean write failure leaves writer retryable");
        file.failWrite = false;
        AppendOutcome retry = writer.append(DomainEvent(SightingStarted{1}),
            QString::fromLatin1(testjournal::kWall), 1000);
        check(retry.ok && retry.envelope.seq == 1,
              "same event succeeds on retry after fault clears");
        check(writer.metrics().failureCount == 1, "failure counted in metrics");
    }

    // partial write poisons the writer (torn tail on disk)
    {
        MemoryJournalFile file;
        JournalWriter writer(testjournal::identity(), &file);
        writer.append(testjournal::sessionStarted(Discipline::Prone50m,
                                                  QStringLiteral("60"),
                                                  QStringLiteral("A")),
                      QString::fromLatin1(testjournal::kWall), 0);
        file.partialWriteLimit = 20;
        AppendOutcome out = writer.append(DomainEvent(SightingStarted{1}),
            QString::fromLatin1(testjournal::kWall), 1000);
        check(!out.ok && out.error.code == ReliabilityError::FileWriteFailed
                  && writer.failed(),
              "partial write poisons the writer");
        AppendOutcome refused = writer.append(DomainEvent(SightingStarted{1}),
            QString::fromLatin1(testjournal::kWall), 2000);
        check(!refused.ok && refused.error.code == ReliabilityError::CorruptJournal,
              "poisoned writer refuses further appends");
        // and the torn file is classified TailTruncated by the validator
        const ValidationReport rep = JournalValidator::validateBytes(file.data);
        check(rep.classification == JournalClassification::TailTruncated
                  && rep.validPrefixCount == 1,
              "torn tail from partial write classified TailTruncated",
              QLatin1String(journalClassificationName(rep.classification)));
    }

    // flush failure: line IS appended, durability not achieved
    {
        MemoryJournalFile file;
        JournalWriter writer(testjournal::identity(), &file);
        writer.append(testjournal::sessionStarted(Discipline::Prone50m,
                                                  QStringLiteral("60"),
                                                  QStringLiteral("A")),
                      QString::fromLatin1(testjournal::kWall), 0);
        file.failFlush = true;
        AppendOutcome out = writer.append(
            DomainEvent(SighterAccepted{testjournal::shot(0, 96)}),
            QString::fromLatin1(testjournal::kWall), 1000);
        check(!out.ok && out.lineAppended
                  && out.error.code == ReliabilityError::FlushFailed,
              "flush failure: appended but not durable, typed");
        check(writer.nextSequence() == 2,
              "flush failure still advances the sequence (bytes exist)");
    }

    // sync failure: same appended-but-not-durable semantics
    {
        MemoryJournalFile file;
        JournalWriter writer(testjournal::identity(), &file);
        file.failSync = true;
        AppendOutcome out = writer.append(
            testjournal::sessionStarted(Discipline::Prone50m,
                                        QStringLiteral("60"),
                                        QStringLiteral("A")),
            QString::fromLatin1(testjournal::kWall), 0);
        check(!out.ok && out.lineAppended
                  && out.error.code == ReliabilityError::SyncFailed,
              "sync failure: appended but not durable, typed");
    }

    // open failure
    {
        MemoryJournalFile file;
        file.failOpen = true;
        JournalWriter writer(testjournal::identity(), &file);
        AppendOutcome out = writer.append(
            testjournal::sessionStarted(Discipline::Prone50m,
                                        QStringLiteral("60"),
                                        QStringLiteral("A")),
            QString::fromLatin1(testjournal::kWall), 0);
        check(!out.ok && out.error.code == ReliabilityError::FileOpenFailed,
              "open failure reported typed");
    }

    // durability override honoured
    {
        MemoryJournalFile file;
        JournalWriter writer(testjournal::identity(), &file);
        writer.append(testjournal::sessionStarted(Discipline::Prone50m,
                                                  QStringLiteral("60"),
                                                  QStringLiteral("A")),
                      QString::fromLatin1(testjournal::kWall), 0,
                      DurabilityClass::Append);
        check(file.syncCount == 0 && file.flushCount == 0,
              "explicit Append override skips flush and sync");
    }

    // real file through FileJournalFile + StorageSync boundary
    {
        const QString dir = QDir::temp().filePath(
            QStringLiteral("techaim_m1_writer_%1")
                .arg(QCoreApplication::applicationPid()));
        QDir().mkpath(dir);
        const QString path = dir + QStringLiteral("/journal.jsonl");
        QFile::remove(path);
        {
            FileJournalFile file(path);
            JournalWriter writer(testjournal::identity(), &file);
            AppendOutcome a = writer.append(
                testjournal::sessionStarted(Discipline::Prone50m,
                                            QStringLiteral("60"),
                                            QStringLiteral("A")),
                QString::fromLatin1(testjournal::kWall), 0);
            AppendOutcome b = writer.append(
                DomainEvent(SighterAccepted{testjournal::shot(0, 95)}),
                QString::fromLatin1(testjournal::kWall), 1000);
            check(a.ok && b.ok, "real-file appends succeed (incl. fsync)",
                  a.error.technicalDetail + b.error.technicalDetail);
            // measured budgets (spec §26): recorded in the M1 doc
            const WriterMetrics& m = writer.metrics();
            std::printf("      real-file latency: serialize %lld us, "
                        "append %lld us, flush %lld us, fsync %lld us\n",
                        static_cast<long long>(m.lastSerializeMicros),
                        static_cast<long long>(m.lastAppendMicros),
                        static_cast<long long>(m.lastFlushMicros),
                        static_cast<long long>(m.maxSyncMicros));
            check(m.maxSyncMicros < 100000,
                  "fsync within the 100 ms graduation ceiling (spec D7)",
                  QString::number(m.maxSyncMicros));
        }
        const ValidationReport rep = JournalValidator::validateFile(path);
        check(rep.classification == JournalClassification::Clean
                  && rep.validPrefixCount == 2,
              "real file validates Clean end-to-end",
              QLatin1String(journalClassificationName(rep.classification)));
        QDir(dir).removeRecursively();
    }
}
