// M1 — journal validator tests (step 12): every classification, sequence
// rules, session mismatch, version mismatch, valid-prefix metadata,
// completion detection, snapshot sequencing, no repair/truncation.

#include "reliability/journal/JournalValidator.h"
#include "reliability/reducer/SessionReducer.h"
#include "test_support.h"

using namespace ta::rel;

namespace {

QByteArray finalsJournal(QVector<EventEnvelope>* envelopesOut = nullptr)
{
    MemoryJournalFile file;
    SessionState state;   // folded alongside to build a coherent snapshot
    QVector<DomainEvent> script = {
        testjournal::sessionStarted(Discipline::Finals3P,
                                    QStringLiteral("FINAL 35"),
                                    QStringLiteral("Athlete")),
        PreparationStarted{0},
        SightingStarted{0},
        SighterAccepted{testjournal::shot(0, 101)},
        OfficialMatchStarted{1},
        ShotAccepted{testjournal::shot(1, 104)},
        ShotAccepted{testjournal::shot(2, 98)},
    };
    JournalWriter writer(testjournal::identity(), &file);
    qint64 tm = 0;
    QVector<EventEnvelope> envelopes;
    for (const DomainEvent& e : script) {
        const AppendOutcome out =
            writer.append(e, QString::fromLatin1(testjournal::kWall), tm);
        check(out.ok, "validator fixture append", out.error.technicalDetail);
        const ReduceResult rr = SessionReducer::apply(state, out.envelope);
        check(rr.ok, "validator fixture reduce", rr.error.technicalDetail);
        state = rr.state;
        envelopes.append(out.envelope);
        tm += 1000;
    }
    // a coherent snapshot, then completion
    const AppendOutcome snap = writer.append(
        DomainEvent(buildStateSnapshot(state)),
        QString::fromLatin1(testjournal::kWall), tm);
    check(snap.ok, "validator fixture snapshot append",
          snap.error.technicalDetail);
    ReduceResult rs = SessionReducer::apply(state, snap.envelope);
    check(rs.ok, "validator fixture snapshot reduce", rs.error.technicalDetail);
    state = rs.state;
    envelopes.append(snap.envelope);
    tm += 1000;
    MatchCompleted done;
    done.totalTenths = state.totalTenths;
    done.officialCount = static_cast<qint16>(state.officials.size());
    const AppendOutcome fin = writer.append(DomainEvent(done),
        QString::fromLatin1(testjournal::kWall), tm);
    check(fin.ok, "validator fixture completion append",
          fin.error.technicalDetail);
    envelopes.append(fin.envelope);
    if (envelopesOut)
        *envelopesOut = envelopes;
    return file.data;
}

} // namespace

void run_validator_tests()
{
    std::printf("--- journal validator tests ---\n");

    const QByteArray journal = finalsJournal();

    // Clean + report metadata + completion detection
    {
        const ValidationReport rep = JournalValidator::validateBytes(journal);
        check(rep.classification == JournalClassification::Clean,
              "coherent journal classifies Clean",
              rep.error.technicalDetail);
        check(rep.totalLines == 9 && rep.validPrefixCount == 9
                  && rep.lastValidSeq == 8,
              "prefix metadata exact",
              QStringLiteral("%1/%2/%3").arg(rep.totalLines)
                  .arg(rep.validPrefixCount).arg(rep.lastValidSeq));
        check(rep.firstInvalidLine == -1 && !rep.corruptionInternal
                  && !rep.tailSafelyTruncatable,
              "clean report has no failure coordinates");
        check(rep.sessionId == QLatin1String(testjournal::kSid),
              "session id adopted from the header");
        check(rep.sawMatchCompleted && !rep.sawSessionClosed,
              "completion state determined from events");
        check(rep.validEnvelopes.size() == 9,
              "valid prefix envelopes returned for replay");
    }

    // expected-session mismatch
    {
        const ValidationReport rep = JournalValidator::validateBytes(
            journal, QStringLiteral("some-other-session"));
        check(rep.classification == JournalClassification::SessionMismatch
                  && rep.validPrefixCount == 0,
              "expected-sid mismatch classified SessionMismatch",
              QLatin1String(journalClassificationName(rep.classification)));
    }

    // byte-sweep over the FINAL line: every truncation point classifies
    // TailTruncated (or Clean at the exact LF boundary), never internal
    {
        const int lastLf = journal.lastIndexOf('\n');
        const int prevLf = journal.lastIndexOf('\n', lastLf - 1);
        bool allSafe = true;
        QString detail;
        for (int cut = prevLf + 1; cut < journal.size() - 1; ++cut) {
            const ValidationReport rep =
                JournalValidator::validateBytes(journal.left(cut));
            const bool ok =
                (rep.classification == JournalClassification::TailTruncated
                 || (cut == prevLf + 1
                     && rep.classification == JournalClassification::Clean))
                && !rep.corruptionInternal
                && rep.validPrefixCount >= 8;
            if (!ok) {
                allSafe = false;
                detail = QStringLiteral("cut=%1 -> %2").arg(cut)
                    .arg(QLatin1String(
                        journalClassificationName(rep.classification)));
                break;
            }
        }
        check(allSafe,
              "power-loss byte sweep over the final line: always tail-safe",
              detail);
    }

    // snapshot sequencing rule: lastAppliedSeq must equal seq-1
    {
        QByteArray tampered = journal;
        tampered.replace("\"lastAppliedSeq\":6", "\"lastAppliedSeq\":5");
        const ValidationReport rep = JournalValidator::validateBytes(tampered);
        check(rep.classification == JournalClassification::CorruptInternal,
              "snapshot with wrong lastAppliedSeq rejected",
              QLatin1String(journalClassificationName(rep.classification)));
    }

    // schema-too-new (sv) forces UnsupportedVersion
    {
        QByteArray newer = journal;
        newer.replace("\"sv\":1,\"pv\":1,\"sid\"", "\"sv\":9,\"pv\":1,\"sid\"");
        const ValidationReport rep = JournalValidator::validateBytes(newer);
        check(rep.classification == JournalClassification::UnsupportedVersion,
              "newer sv classified UnsupportedVersion",
              QLatin1String(journalClassificationName(rep.classification)));
    }

    // interior blank line = internal corruption
    {
        QByteArray blank = journal;
        blank.insert(journal.indexOf('\n') + 1, "\n");
        const ValidationReport rep = JournalValidator::validateBytes(blank);
        check(rep.classification == JournalClassification::CorruptInternal
                  && rep.firstInvalidLine == 2,
              "interior blank line classified CorruptInternal",
              QString::number(rep.firstInvalidLine));
    }

    // validator never mutates its input (bytes in, report out)
    {
        QByteArray copy = journal;
        JournalValidator::validateBytes(copy);
        check(copy == journal, "validator does not mutate input bytes");
    }

    // unreadable file reported typed
    {
        const ValidationReport rep = JournalValidator::validateFile(
            QStringLiteral("Z:/no/such/techaim/journal.jsonl"));
        check(rep.classification != JournalClassification::Clean
                  && rep.error.code == ReliabilityError::FileOpenFailed,
              "unreadable file reported typed",
              QLatin1String(reliabilityErrorName(rep.error.code)));
    }

    // §9D/§9E degraded-gap reconciliation, exercised directly with explicit-
    // seq writes so the physical seqs are controlled precisely.
    {
        // Build a journal with a real gap at seq 3 (skipped) using a valid
        // chain, WITHOUT declaring it -> unexplained gap -> CorruptInternal.
        MemoryJournalFile file;
        JournalWriter writer(testjournal::identity(), &file);
        const QString wall = QString::fromLatin1(testjournal::kWall);
        writer.appendSeq(0, testjournal::sessionStarted(Discipline::Prone50m,
                             QStringLiteral("60"), QStringLiteral("A")),
                         wall, 0, DurabilityClass::Sync);
        writer.appendSeq(1, DomainEvent(OfficialMatchStarted{1}), wall, 1,
                         DurabilityClass::Sync);
        writer.appendSeq(2, DomainEvent(ShotAccepted{testjournal::shot(1, 104)}),
                         wall, 2, DurabilityClass::Sync);
        // seq 3 skipped, seq 4 next — a forward gap with an intact chain
        writer.appendSeq(4, DomainEvent(ShotAccepted{testjournal::shot(2, 99)}),
                         wall, 4, DurabilityClass::Sync);
        const ValidationReport undeclared = JournalValidator::validateBytes(file.data);
        check(undeclared.classification == JournalClassification::CorruptInternal
                  && undeclared.error.code == ReliabilityError::CorruptJournal,
              "undeclared seq gap rejected (never-silent reconciliation)",
              QLatin1String(journalClassificationName(undeclared.classification)));

        // Now the SAME gap, but declared by an AuxEventsDropped{3,3,1} -> Clean.
        MemoryJournalFile file2;
        JournalWriter w2(testjournal::identity(), &file2);
        w2.appendSeq(0, testjournal::sessionStarted(Discipline::Prone50m,
                         QStringLiteral("60"), QStringLiteral("A")),
                     wall, 0, DurabilityClass::Sync);
        w2.appendSeq(1, DomainEvent(OfficialMatchStarted{1}), wall, 1,
                     DurabilityClass::Sync);
        w2.appendSeq(2, DomainEvent(ShotAccepted{testjournal::shot(1, 104)}),
                     wall, 2, DurabilityClass::Sync);
        w2.appendSeq(4, DomainEvent(ShotAccepted{testjournal::shot(2, 99)}),
                     wall, 4, DurabilityClass::Sync);
        w2.appendSeq(5, DomainEvent(AuxEventsDropped{3, 3, 1}), wall, 5,
                     DurabilityClass::Sync);
        const ValidationReport declared = JournalValidator::validateBytes(file2.data);
        check(declared.classification == JournalClassification::Clean,
              "declared drop reconciles the gap -> Clean",
              QStringLiteral("%1: %2")
                  .arg(QLatin1String(journalClassificationName(declared.classification)))
                  .arg(declared.error.technicalDetail));

        // A declared drop with NO corresponding gap is also rejected (phantom).
        MemoryJournalFile file3;
        JournalWriter w3(testjournal::identity(), &file3);
        w3.appendSeq(0, testjournal::sessionStarted(Discipline::Prone50m,
                         QStringLiteral("60"), QStringLiteral("A")),
                     wall, 0, DurabilityClass::Sync);
        w3.appendSeq(1, DomainEvent(AuxEventsDropped{99, 99, 1}), wall, 1,
                     DurabilityClass::Sync);
        const ValidationReport phantom = JournalValidator::validateBytes(file3.data);
        check(phantom.classification == JournalClassification::CorruptInternal,
              "phantom drop declaration (no gap) rejected",
              QLatin1String(journalClassificationName(phantom.classification)));
    }
}
