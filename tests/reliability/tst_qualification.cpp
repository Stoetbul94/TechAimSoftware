// Phase B0 — qualification persistence seam (QualificationController).
//
// Proves the shared seam turns classified, scored shots into a journal whose
// reducer state is authoritative: sighters excluded from officials/total, first
// official is number 1, decimal precision preserved, duplicate/out-of-phase
// shots refused, unsupported disciplines rejected, and the whole session
// replays into the correct QualificationState. Uses an injected in-memory
// journal + ManualClock so it is deterministic and touches no disk.

#include "qualification/QualificationController.h"
#include "reliability/journal/JournalValidator.h"
#include "reliability/replay/ReplayEngine.h"
#include "test_support.h"

#include <QVariantMap>

using namespace ta::rel;

void run_qualification_tests()
{
    std::printf("--- qualification persistence tests (Phase B0) ---\n");

    // 1) AR10 decimal: a full session journals and replays authoritatively.
    {
        MemoryJournalFile file;
        ManualClock clock;
        QualificationController qc;
        qc.storeForTesting()->setClockForTesting(&clock);
        qc.storeForTesting()->setJournalFileForTesting(&file);

        int accepted = 0;
        QObject::connect(&qc, &QualificationController::shotAccepted,
                         &qc, [&accepted](const QVariantMap&) { ++accepted; });

        const bool started = qc.startSession(
            QStringLiteral("AR10"), QStringLiteral("60"), QStringLiteral("A"),
            60, 4500000, 900000, -1, QStringLiteral("L1"), QStringLiteral("T1"));
        check(started, "AR10 session starts");
        check(qc.active() && qc.persistenceHealth() == 0,
              "controller active and healthy");

        qc.beginPreparation();
        qc.beginSighting();
        check(qc.submitSighter(1.0, 1.0, 10.9, 1001, 45.0, true),
              "sighter 1 accepted");
        check(qc.submitSighter(-2.0, 3.0, 10.2, 1002, 200.0, true),
              "sighter 2 accepted");
        check(qc.officialShotCount() == 0,
              "sighters do not increment official count");

        qc.beginOfficialMatch();
        check(qc.nextOfficialShotNumber() == 1, "first official is number 1");
        check(qc.submitOfficial(0.0, 0.0, 10.9, 2001, 0.0, true),
              "official 1 accepted");
        check(qc.nextOfficialShotNumber() == 2, "next official is number 2");
        check(qc.submitOfficial(1.0, 1.0, 10.3, 2002, 90.0, true),
              "official 2 accepted");
        check(qc.submitOfficial(-1.0, 2.0, 9.8, 2003, 180.0, true),
              "official 3 accepted");

        check(accepted == 5,
              "five shotAccepted emissions (2 sighter + 3 official)",
              QString::number(accepted));
        check(qc.officialShotCount() == 3, "three officials",
              QString::number(qc.officialShotCount()));
        check(qc.sighterCount() == 2, "two sighters");
        // 10.9 + 10.3 + 9.8 = 31.0 → sighters (10.9, 10.2) excluded
        check(qRound(qc.totalDecimal() * 10) == 310,
              "official total excludes sighters (31.0)",
              QString::number(qc.totalDecimal()));

        qc.completeMatch();

        // Replay: the journal is the authoritative record.
        const ReplayResult rr = ReplayEngine::replayBytes(file.data);
        check(rr.ok, "AR10 journal replays", rr.error.technicalDetail);
        check(rr.state.discipline == Discipline::AirRifle10m,
              "replayed discipline is AR10");
        check(std::holds_alternative<QualificationState>(rr.state.disc),
              "replayed disc is QualificationState");
        check(rr.state.officials.size() == 3 && rr.state.sighters.size() == 2,
              "replay counts match live");
        check(!rr.state.officials.isEmpty()
                  && rr.state.officials.first().shot.shotNumber == 1,
              "replay: first official shotNumber is 1");
        check(rr.state.totalTenths == 310, "replay total 31.0 tenths",
              QString::number(rr.state.totalTenths));
        check(rr.state.officials.size() == 3
                  && rr.state.officials.at(1).shot.scoreTenths == 103,
              "decimal precision preserved (10.3 → 103 tenths)");

        qc.closeSession();   // injected file: writes close markers, no archive
        const ValidationReport rep = JournalValidator::validateBytes(file.data);
        check(rep.classification == JournalClassification::Clean,
              "closed AR10 journal validates Clean",
              QLatin1String(journalClassificationName(rep.classification)));
    }

    // 2) Phase guard — an official before OfficialMatchStarted is refused, so
    //    the journal can never contain an out-of-phase official.
    {
        MemoryJournalFile file;
        ManualClock clock;
        QualificationController qc;
        qc.storeForTesting()->setClockForTesting(&clock);
        qc.storeForTesting()->setJournalFileForTesting(&file);
        qc.startSession(QStringLiteral("AR10"), QStringLiteral("60"),
                        QStringLiteral("A"), 60, 4500000, 900000, -1,
                        QString(), QString());
        qc.beginPreparation();
        qc.beginSighting();
        check(!qc.submitOfficial(0, 0, 10.0, 3001, 0, true),
              "official before match start is refused");
    }

    // 3) Duplicate-externalId prevention (the identity guard shared with retry
    //    and replay): the same raw measurement id is never double-counted.
    {
        MemoryJournalFile file;
        ManualClock clock;
        QualificationController qc;
        qc.storeForTesting()->setClockForTesting(&clock);
        qc.storeForTesting()->setJournalFileForTesting(&file);
        qc.startSession(QStringLiteral("AP10"), QStringLiteral("60"),
                        QStringLiteral("A"), 60, 4500000, 900000, -1,
                        QString(), QString());
        qc.beginPreparation();
        qc.beginSighting();
        qc.beginOfficialMatch();
        check(qc.submitOfficial(0, 0, 10, 7777, 0, true),
              "AP10 official accepted");
        check(!qc.submitOfficial(1, 1, 9, 7777, 0, true),
              "duplicate externalId refused (not double-counted)");
        check(qc.officialShotCount() == 1,
              "official count stays 1 after duplicate");
    }

    // 4) The seam refuses disciplines it does not own (no silent accept, no
    //    fallback). Finals and 3P-qual/25m are out of scope here.
    {
        QualificationController qc;
        check(!qc.startSession(QStringLiteral("FINAL3P"), QStringLiteral("x"),
                               QStringLiteral("A"), 35, 0, 0, -1,
                               QString(), QString()),
              "FINAL3P refused by the qualification seam");
        check(!qc.startSession(QStringLiteral("3P50"), QStringLiteral("x"),
                               QStringLiteral("A"), 60, 0, 0, -1,
                               QString(), QString()),
              "3P50 refused (rules pending)");
    }
}
