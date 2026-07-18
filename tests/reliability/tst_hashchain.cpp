// M1 — hash chain tests (step 8): genesis, multi-event chains, and the
// full tamper suite (payload edit, seq edit, ph edit, removal, insertion,
// reordering, duplication, wrong session, CRLF, empty journal).

#include "reliability/journal/HashChain.h"
#include "reliability/journal/JournalValidator.h"
#include "test_support.h"

using namespace ta::rel;

namespace {

QByteArray cleanJournal()
{
    MemoryJournalFile file;
    testjournal::writeScript(file, {
        testjournal::sessionStarted(Discipline::Prone50m, QStringLiteral("60"),
                                    QStringLiteral("Athlete")),
        SightingStarted{1},
        SighterAccepted{testjournal::shot(0, 98)},
        OfficialMatchStarted{1},
        ShotAccepted{testjournal::shot(1, 103)},
        ShotAccepted{testjournal::shot(2, 105)},
    });
    return file.data;
}

QList<QByteArray> splitLines(const QByteArray& journal)
{
    QList<QByteArray> lines;
    int start = 0;
    while (start < journal.size()) {
        const int lf = journal.indexOf('\n', start);
        if (lf < 0) {
            lines.append(journal.mid(start));
            break;
        }
        lines.append(journal.mid(start, lf - start));
        start = lf + 1;
    }
    return lines;
}

QByteArray joinLines(const QList<QByteArray>& lines)
{
    QByteArray out;
    for (const QByteArray& l : lines) {
        out += l;
        out += '\n';
    }
    return out;
}

} // namespace

void run_hashchain_tests()
{
    std::printf("--- hash chain tests ---\n");

    // genesis + basic determinism
    {
        check(HashChain::genesisPreviousHash() == QByteArray(32, '0'),
              "genesis previous hash is 32 zeros");
        const QByteArray h1 =
            HashChain::computeLineHash(QByteArray(32, '0'), "payload");
        const QByteArray h2 =
            HashChain::computeLineHash(QByteArray(32, '0'), "payload");
        check(h1 == h2 && h1.size() == 32, "hash is deterministic, 32 hex chars",
              QString::fromLatin1(h1));
        check(isWellFormedHashHex(h1), "hash is lowercase hex");
        const QByteArray h3 =
            HashChain::computeLineHash(QByteArray(32, '1'), "payload");
        check(h1 != h3, "previous hash participates in the digest");
    }

    const QByteArray journal = cleanJournal();
    check(splitLines(journal).size() == 6, "clean journal has 6 lines");

    // valid chain
    {
        const ValidationReport rep = JournalValidator::validateBytes(journal);
        check(rep.classification == JournalClassification::Clean
                  && rep.validPrefixCount == 6,
              "valid multi-event chain classifies Clean",
              QLatin1String(journalClassificationName(rep.classification)));
    }

    // changed payload byte (interior)
    {
        QByteArray tampered = journal;
        tampered.replace("\"scoreTenths\":103", "\"scoreTenths\":104");
        const ValidationReport rep = JournalValidator::validateBytes(tampered);
        check(rep.classification == JournalClassification::CorruptInternal
                  && rep.error.code == ReliabilityError::HashMismatch,
              "payload tamper detected as hash mismatch",
              QLatin1String(reliabilityErrorName(rep.error.code)));
    }

    // changed sequence
    {
        QByteArray tampered = journal;
        tampered.replace("\"seq\":2,", "\"seq\":3,");
        const ValidationReport rep = JournalValidator::validateBytes(tampered);
        check(rep.classification != JournalClassification::Clean
                  && rep.firstInvalidLine == 3,
              "sequence tamper detected at the edited line",
              QString::number(rep.firstInvalidLine));
    }

    // changed previous hash
    {
        QList<QByteArray> lines = splitLines(journal);
        QByteArray& l2 = lines[2];
        const int phPos = l2.indexOf("\"ph\":\"");
        l2[phPos + 7] = l2[phPos + 7] == 'a' ? 'b' : 'a';
        const ValidationReport rep =
            JournalValidator::validateBytes(joinLines(lines));
        check(rep.classification == JournalClassification::CorruptInternal
                  && (rep.error.code == ReliabilityError::BrokenHashChain
                      || rep.error.code == ReliabilityError::HashMismatch),
              "ph tamper breaks the chain",
              QLatin1String(reliabilityErrorName(rep.error.code)));
    }

    // removed interior line
    {
        QList<QByteArray> lines = splitLines(journal);
        lines.removeAt(2);
        const ValidationReport rep =
            JournalValidator::validateBytes(joinLines(lines));
        // A removed interior line manifests as both a forward seq gap and a
        // broken chain; the chain break is checked first (§9D makes the gap
        // provisional pending drop-declaration). Either way: CorruptInternal.
        check(rep.classification == JournalClassification::CorruptInternal
                  && (rep.error.code == ReliabilityError::BrokenHashChain
                      || rep.error.code == ReliabilityError::SequenceGap
                      || rep.error.code == ReliabilityError::CorruptJournal),
              "removed line detected as internal corruption",
              QLatin1String(reliabilityErrorName(rep.error.code)));
    }

    // inserted foreign line (valid-looking, wrong chain)
    {
        QList<QByteArray> lines = splitLines(journal);
        lines.insert(3, lines[2]);   // re-insert a copy at the wrong place
        const ValidationReport rep =
            JournalValidator::validateBytes(joinLines(lines));
        check(rep.classification == JournalClassification::CorruptInternal,
              "inserted line detected",
              QLatin1String(reliabilityErrorName(rep.error.code)));
    }

    // reordered lines
    {
        QList<QByteArray> lines = splitLines(journal);
        lines.swapItemsAt(3, 4);
        const ValidationReport rep =
            JournalValidator::validateBytes(joinLines(lines));
        check(rep.classification == JournalClassification::CorruptInternal,
              "reordered lines detected",
              QLatin1String(reliabilityErrorName(rep.error.code)));
    }

    // duplicated final line (tail anomaly — safe to truncate)
    {
        QList<QByteArray> lines = splitLines(journal);
        lines.append(lines.last());
        const ValidationReport rep =
            JournalValidator::validateBytes(joinLines(lines));
        check(rep.classification == JournalClassification::TailTruncated
                  && rep.validPrefixCount == 6 && rep.tailSafelyTruncatable,
              "duplicated tail line classified TailTruncated",
              QLatin1String(journalClassificationName(rep.classification)));
    }

    // wrong session id on an interior line
    {
        QList<QByteArray> lines = splitLines(journal);
        lines[4].replace("0000000000a1", "0000000000ff");
        const ValidationReport rep =
            JournalValidator::validateBytes(joinLines(lines));
        check(rep.classification == JournalClassification::SessionMismatch,
              "interior sid change classified SessionMismatch",
              QLatin1String(journalClassificationName(rep.classification)));
    }

    // CRLF-mangled journal still validates (structural verification)
    {
        QByteArray crlf = journal;
        crlf.replace("\n", "\r\n");
        const ValidationReport rep = JournalValidator::validateBytes(crlf);
        check(rep.classification == JournalClassification::Clean,
              "CRLF-mangled journal still verifies structurally",
              QLatin1String(journalClassificationName(rep.classification)));
    }

    // empty journal
    {
        const ValidationReport rep = JournalValidator::validateBytes(QByteArray());
        check(rep.classification == JournalClassification::Empty,
              "empty journal classifies Empty");
    }
}
