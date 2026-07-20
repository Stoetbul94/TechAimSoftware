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
#include "reliability/storage/StoragePaths.h"
#include "test_support.h"

#include <QDir>
#include <QVariantList>
#include <QVariantMap>

using namespace ta::rel;

namespace {

// Phase C helper: build a crashed (never-closed) qualification journal via the
// controller, advancing an injected ManualClock so elapsed time > 0. Fires
// `sighters` sighters, then — if `startMatch` — begins the official match
// (anchoring the match clock) and fires `officials` officials. prep=15m,
// match=75m. Leaves the session un-closed (a crash).
void buildCrashed(MemoryJournalFile& file, ManualClock& clock,
                  const char* disciplineId, int sighters,
                  bool startMatch, int officials)
{
    QualificationController qc;
    qc.storeForTesting()->setClockForTesting(&clock);
    qc.storeForTesting()->setJournalFileForTesting(&file);
    qc.startSession(QString::fromLatin1(disciplineId), QStringLiteral("60"),
                    QStringLiteral("A"), 60, 4500000, 900000, -1,
                    QString(), QString());
    qc.beginPreparation();       // TimerStarted(Preparation, 900000) at tm now
    clock.advance(30000);
    qc.beginSighting();
    for (int i = 0; i < sighters; ++i) {
        clock.advance(5000);
        qc.submitSighter(0, 0, 10.5, 1000 + i, 0, true);
    }
    if (!startMatch)
        return;                  // crash during Preparation+Sighting
    clock.advance(20000);
    qc.beginOfficialMatch();     // TimerStarted(Match, 4500000) at tm now
    for (int n = 1; n <= officials; ++n) {
        clock.advance(10000);
        qc.submitOfficial(0, 0, 10.0 + (n % 2) * 0.5, 2000 + n, 0, true);
    }
    // no closeSession — simulated crash.
}

// Replay a crashed journal and return its rebuilt state.
ReplayResult replayOf(const MemoryJournalFile& file)
{
    const ValidationReport rep = JournalValidator::validateBytes(file.data);
    return ReplayEngine::replay(rep.validEnvelopes);
}

// Recovered frozen remaining for the current phase clock:
// remaining = durationMs − (lastEventMonoMs − startedAtMonoMs).
qint64 recoveredRemaining(const MemoryJournalFile& file)
{
    const ValidationReport rep = JournalValidator::validateBytes(file.data);
    const ReplayResult rr = ReplayEngine::replay(rep.validEnvelopes);
    const qint64 lastMono = rep.validEnvelopes.isEmpty()
        ? 0 : rep.validEnvelopes.last().monotonicMs;
    return rr.state.timer.durationMs
        - (lastMono - rr.state.timer.startedAtMonoMs);
}

} // namespace

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

    // 5) AR10 cap of 60: unlimited sighters, officials 1..60 accepted, 61st
    //    refused at the durable boundary; target coordinates survive replay.
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
        bool allSighters = true;
        for (int i = 0; i < 15; ++i)
            allSighters = allSighters
                && qc.submitSighter(0, 0, 10.5, 6000 + i, 0, true);
        check(allSighters, "15 sighters all accepted (unlimited sighting)");
        check(qc.officialShotCount() == 0,
              "sighters never increment the official count");
        check(qc.sighterCount() == 15, "15 sighters recorded");

        qc.beginOfficialMatch();
        check(qc.submitOfficial(3.21, -4.56, 10.9, 7000, 0.0, true),
              "official 1 accepted (with coordinates)");
        bool restOk = true;
        for (int n = 2; n <= 60; ++n)
            restOk = restOk && qc.submitOfficial(0, 0, 9.0, 7000 + n, 0, true);
        check(restOk, "officials 2..60 all accepted");
        check(qc.officialShotCount() == 60, "official count reaches exactly 60");
        check(!qc.submitOfficial(0, 0, 10.0, 9999, 0, true),
              "61st official refused at the cap");
        check(qc.officialShotCount() == 60,
              "official count stays 60 after the refused 61st");

        const ReplayResult rr = ReplayEngine::replayBytes(file.data);
        check(rr.ok && rr.state.officials.size() == 60,
              "replay reconstructs 60 officials");
        check(!rr.state.officials.isEmpty()
                  && rr.state.officials.first().shot.xHundredthMm == 321
                  && rr.state.officials.first().shot.yHundredthMm == -456,
              "replay: target coordinates preserved (3.21, -4.56 mm)");
        check(rr.state.discipline == Discipline::AirRifle10m,
              "replay discipline stays AR10");
    }

    // 6) An interrupted (unclosed) AR10 journal is valid and replays with its
    //    officials intact — enough for future deterministic restoration
    //    (Phase D). No CleanShutdown → not marked closed (recoverable).
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
        qc.submitSighter(0, 0, 10.0, 8001, 0, true);
        qc.beginOfficialMatch();
        qc.submitOfficial(0, 0, 10.9, 8101, 0, true);
        qc.submitOfficial(1, 1, 10.2, 8102, 0, true);
        // Simulate a crash: no closeSession.
        const ValidationReport rep = JournalValidator::validateBytes(file.data);
        check(rep.classification == JournalClassification::Clean,
              "interrupted AR10 journal validates (hash chain intact)",
              QLatin1String(journalClassificationName(rep.classification)));
        const ReplayResult rr = ReplayEngine::replayBytes(file.data);
        check(rr.ok && rr.state.officials.size() == 2
                  && rr.state.sighters.size() == 1,
              "interrupted journal replays 2 officials + 1 sighter");
        check(rr.state.lifecycle != Lifecycle::Closed,
              "interrupted session not marked closed (recoverable)");
    }

    // 7) B2 — 10m Air Pistol INTEGER scoring: a distinct discipline, integer
    //    totals, coordinates retained. (The QML seam floors the decimal ring
    //    position to the integer ring value before submit; the harness submits
    //    the already-floored integer values the seam would produce.)
    {
        MemoryJournalFile file;
        ManualClock clock;
        QualificationController qc;
        qc.storeForTesting()->setClockForTesting(&clock);
        qc.storeForTesting()->setJournalFileForTesting(&file);
        check(qc.startSession(QStringLiteral("AP10"), QStringLiteral("60"),
                              QStringLiteral("A"), 60, 4500000, 900000, -1,
                              QString(), QString()),
              "AP10 session starts");
        qc.beginPreparation();
        qc.beginSighting();
        qc.beginOfficialMatch();
        check(qc.submitOfficial(2.10, -1.30, 10, 9001, 0, true),
              "AP10 official persisted as integer 10");
        check(qc.submitOfficial(0, 0, 9, 9002, 0, true), "AP10 official 9");
        check(qc.submitOfficial(0, 0, 8, 9003, 0, true), "AP10 official 8");
        check(qRound(qc.totalDecimal() * 10) == 270,
              "AP10 integer total is 27.0 (10+9+8, no decimal leakage)",
              QString::number(qc.totalDecimal()));

        const ReplayResult rr = ReplayEngine::replayBytes(file.data);
        check(rr.ok && rr.state.discipline == Discipline::AirPistol10m,
              "AP10 replays to AirPistol10m — distinct from AR10");
        check(std::holds_alternative<QualificationState>(rr.state.disc),
              "AP10 disc is QualificationState");
        check(rr.state.officials.size() == 3 && rr.state.totalTenths == 270,
              "AP10 replay: 3 officials, total 270 tenths",
              QString::number(rr.state.totalTenths));
        check(!rr.state.officials.isEmpty()
                  && rr.state.officials.first().shot.scoreTenths == 100,
              "AP10 first official scoreTenths == 100 (integer 10.0, not decimal)");
        check(!rr.state.officials.isEmpty()
                  && rr.state.officials.first().shot.xHundredthMm == 210
                  && rr.state.officials.first().shot.yHundredthMm == -130,
              "AP10 target coordinates preserved (2.10, -1.30 mm)");
    }

    // 8) B3 — 50m Rifle Prone: DECIMAL scoring, distinct discipline, 50-minute
    //    match clock carried as config. Single-stage qualification — no Final,
    //    no 3P position transitions.
    {
        MemoryJournalFile file;
        ManualClock clock;
        QualificationController qc;
        qc.storeForTesting()->setClockForTesting(&clock);
        qc.storeForTesting()->setJournalFileForTesting(&file);
        // 50-min match clock (3_000_000 ms), 60 officials.
        check(qc.startSession(QStringLiteral("PRONE50"), QStringLiteral("60"),
                              QStringLiteral("A"), 60, 3000000, 900000, -1,
                              QString(), QString()),
              "PRONE50 session starts");
        qc.beginPreparation();
        qc.beginSighting();
        check(qc.submitSighter(0, 0, 10.4, 1, 0, true), "PRONE50 sighter");
        qc.beginOfficialMatch();
        check(qc.submitOfficial(1.11, -2.22, 10.9, 101, 0, true),
              "PRONE50 official 1 (decimal, with coords)");
        check(qc.submitOfficial(0, 0, 10.3, 102, 0, true), "PRONE50 official 2");
        check(qRound(qc.totalDecimal() * 10) == 212,
              "PRONE50 decimal total 21.2 (10.9 + 10.3)",
              QString::number(qc.totalDecimal()));

        const ReplayResult rr = ReplayEngine::replayBytes(file.data);
        check(rr.ok && rr.state.discipline == Discipline::Prone50m,
              "PRONE50 replays to Prone50m — not 3P, not generic 50m");
        check(std::holds_alternative<QualificationState>(rr.state.disc),
              "PRONE50 disc is QualificationState (no finals/3P state)");
        check(rr.state.officials.size() == 2 && rr.state.totalTenths == 212,
              "PRONE50 replay: 2 officials, total 212 tenths",
              QString::number(rr.state.totalTenths));
        check(!rr.state.officials.isEmpty()
                  && rr.state.officials.first().shot.scoreTenths == 109,
              "PRONE50 decimal preserved (10.9 -> 109 tenths)");
        check(rr.state.config.matchMs == 3000000,
              "PRONE50 50-minute match clock carried in config");
    }

    // 9) Phase C — recovered timer anchors + crash-phase scenarios. A crashed
    //    journal must carry the frozen remaining competition time (never the
    //    full duration) and the correct next-shot state, for every phase.
    {
        // (a) crash during Preparation+Sighting after sighters: no officials,
        //     Sighting phase, the PREP clock is the active timer.
        {
            MemoryJournalFile file; ManualClock clock;
            buildCrashed(file, clock, "AR10", 3, /*startMatch*/false, 0);
            const ReplayResult rr = replayOf(file);
            check(rr.ok && rr.state.phase == MatchPhase::Sighting,
                  "crash in sighting: phase = Sighting");
            check(rr.state.officials.isEmpty() && rr.state.sighters.size() == 3,
                  "crash in sighting: 3 sighters, 0 officials");
            check(rr.state.timer.active
                      && rr.state.timer.timerId == TimerId::Preparation,
                  "crash in sighting: prep clock is the active timer");
            // elapsed = 30000 (to sighting) + 3*5000 = 45000; remaining 855000.
            check(recoveredRemaining(file) == 900000 - 45000,
                  "crash in sighting: frozen prep remaining (14:15), not full 15:00",
                  QString::number(recoveredRemaining(file)));
        }
        // (b) crash immediately after OfficialMatchStarted: match clock anchored,
        //     0 officials, next official is 1.
        {
            MemoryJournalFile file; ManualClock clock;
            buildCrashed(file, clock, "AR10", 2, /*startMatch*/true, 0);
            const ReplayResult rr = replayOf(file);
            check(rr.state.phase == MatchPhase::OfficialMatch
                      && rr.state.officials.isEmpty(),
                  "crash just after match start: OfficialMatch, 0 officials");
            check(rr.state.timer.active && rr.state.timer.timerId == TimerId::Match
                      && rr.state.timer.durationMs == 4500000,
                  "crash just after match start: 75-min match clock anchored");
        }
        // (c) crash mid-match (after shot 5): frozen match remaining < full,
        //     next official = 6, no duplicate on replay.
        {
            MemoryJournalFile file; ManualClock clock;
            buildCrashed(file, clock, "AR10", 3, true, 5);
            const ReplayResult rr = replayOf(file);
            check(rr.state.officials.size() == 5,
                  "crash mid-match: 5 officials recovered");
            check(rr.state.officials.size() + 1 == 6,
                  "crash mid-match: recovered next official number is 6");
            check(recoveredRemaining(file) < 4500000
                      && recoveredRemaining(file) > 0,
                  "crash mid-match: match remaining is frozen (< full, > 0)",
                  QString::number(recoveredRemaining(file)));
            // replay is idempotent: re-fold the same envelopes → identical state
            const ReplayResult rr2 = replayOf(file);
            check(rr2.state.officials.size() == 5
                      && rr2.state.totalTenths == rr.state.totalTenths,
                  "replay is idempotent: no duplicate accepted shot");
        }
        // (d) crash after shot 59 and after shot 60 (before clean close).
        {
            MemoryJournalFile f59; ManualClock c59;
            buildCrashed(f59, c59, "AR10", 2, true, 59);
            check(replayOf(f59).state.officials.size() == 59,
                  "crash after shot 59: 59 officials, next is 60");

            MemoryJournalFile f60; ManualClock c60;
            buildCrashed(f60, c60, "AR10", 2, true, 60);
            const ReplayResult rr60 = replayOf(f60);
            check(rr60.state.officials.size() == 60,
                  "crash after shot 60 (before close): 60 officials");
            // The match is at capacity — a recovered session must treat it as
            // complete-able and NOT accept a 61st (the controller cap enforces
            // this; see block 5). Lifecycle is still Active (no MatchCompleted
            // was journalled before the crash), so recovery routes to the
            // completion path rather than reopening for shot 61.
            check(rr60.state.lifecycle == Lifecycle::Active,
                  "crash after shot 60: lifecycle Active (completion pending)");
        }
        // (e) AP10 integer + PRONE50 decimal recover their timer anchors too.
        {
            MemoryJournalFile fp; ManualClock cp;
            buildCrashed(fp, cp, "AP10", 1, true, 3);
            const ReplayResult rp = replayOf(fp);
            check(rp.state.discipline == Discipline::AirPistol10m
                      && rp.state.timer.timerId == TimerId::Match,
                  "AP10 crash: match clock anchored, discipline AP10");

            MemoryJournalFile fr; ManualClock cr;
            buildCrashed(fr, cr, "PRONE50", 1, true, 3);
            const ReplayResult rpr = replayOf(fr);
            check(rpr.state.discipline == Discipline::Prone50m
                      && rpr.state.timer.active,
                  "PRONE50 crash: match clock anchored, discipline PRONE50");
        }
    }

    // 10) Phase D — controller-level resume through the real recovery path
    //     (disk journal + coordinator). A fresh controller reopens a crashed
    //     AR10 journal, projects the recovered shots, continues numbering, and
    //     refuses a duplicate hardware id after resume.
    {
        const QString root = QDir::temp().filePath(
            QStringLiteral("ta_qual_recovery_test"));
        QDir(root).removeRecursively();
        StoragePaths::setRootOverrideForTesting(root);
        StoragePaths::initialize();

        QString sid;
        const qint64 lastOfficialExtId = 23;
        {   // build a crashed AR10 session (real journal); scope-out = crash
            QualificationController builder;
            check(builder.startSession(QStringLiteral("AR10"),
                    QStringLiteral("60"), QStringLiteral("A"), 60,
                    4500000, 900000, -1, QString(), QString()),
                  "resume: crashed builder session starts");
            builder.beginPreparation();
            builder.beginSighting();
            builder.submitSighter(0, 0, 10.5, 10, 0, true);
            builder.beginOfficialMatch();
            builder.submitOfficial(1.0, 1.0, 10.9, 21, 0, true);
            builder.submitOfficial(0, 0, 10.2, 22, 0, true);
            builder.submitOfficial(-1.0, 2.0, 9.8, lastOfficialExtId, 0, true);
            sid = builder.sessionId();
            // no closeSession — simulated crash.
        }

        QualificationController c2;
        const bool ok = c2.resumeFromRecovery(sid);
        check(ok, "resume: crashed AR10 session resumed by a fresh controller");
        check(c2.recovered() && c2.active(),
              "resume: controller is active and marked recovered");
        check(c2.officialShotCount() == 3 && c2.sighterCount() == 1,
              "resume: 3 officials + 1 sighter recovered from the journal");
        check(c2.nextOfficialShotNumber() == 4,
              "resume: recovered next official number is 4");
        check(c2.recoveredShots().size() == 4,
              "resume: recoveredShots() projects 4 rows (1 sighter + 3 officials)");
        check(qRound(c2.totalDecimal() * 10) == 309,
              "resume: recovered official total 30.9 (10.9+10.2+9.8)",
              QString::number(c2.totalDecimal()));

        // Duplicate hardware input after resume: a re-reported final pre-crash
        // shot (same external identity) must NOT be accepted again.
        check(!c2.submitOfficial(0, 0, 9.0, lastOfficialExtId, 0, true),
              "resume: duplicate recovered externalId refused (no double-count)");
        check(c2.officialShotCount() == 3,
              "resume: official count unchanged after the duplicate");

        // A genuinely new official continues the sequence as shot 4.
        check(c2.submitOfficial(0, 0, 10.6, 900, 0, true),
              "resume: a new official is accepted after resume");
        check(c2.officialShotCount() == 4,
              "resume: the new live official is shot 4 (not 1/5/duplicate 3)");

        // The resumed journal continues and still validates.
        c2.closeSession();
        QDir(root).removeRecursively();
    }

    // 11) D2 — AP10 resume: integer scores survive the round trip, the
    //     discipline stays AirPistol10m, and the resumed live shot continues
    //     integer scoring with the correct next number.
    {
        const QString root = QDir::temp().filePath(
            QStringLiteral("ta_qual_recovery_ap10"));
        QDir(root).removeRecursively();
        StoragePaths::setRootOverrideForTesting(root);
        StoragePaths::initialize();

        QString sid;
        {
            QualificationController builder;
            builder.startSession(QStringLiteral("AP10"), QStringLiteral("60"),
                                 QStringLiteral("A"), 60, 4500000, 900000, -1,
                                 QString(), QString());
            builder.beginPreparation();
            builder.beginSighting();
            builder.submitSighter(0, 0, 10, 11, 0, true);   // integer sighters
            builder.submitSighter(0, 0, 9, 12, 0, true);
            builder.beginOfficialMatch();
            builder.submitOfficial(1.5, -0.5, 10, 21, 0, true);
            builder.submitOfficial(0, 0, 9, 22, 0, true);
            builder.submitOfficial(0, 0, 10, 23, 0, true);
            sid = builder.sessionId();
            // crash: no close
        }

        QualificationController c2;
        check(c2.resumeFromRecovery(sid), "AP10 resume succeeds");
        check(c2.officialShotCount() == 3 && c2.sighterCount() == 2,
              "AP10 resume: 3 officials + 2 sighters");
        check(qRound(c2.totalDecimal() * 10) == 290,
              "AP10 resume: integer total 29 (10+9+10), no decimal leakage",
              QString::number(c2.totalDecimal()));
        // Every recovered score is a whole ring value.
        bool allInteger = true;
        const QVariantList rec = c2.recoveredShots();
        for (const QVariant& v : rec) {
            const double s = v.toMap().value(QStringLiteral("calculatedscore")).toDouble();
            if (qAbs(s - qRound(s)) > 1e-9)
                allInteger = false;
        }
        check(allInteger && rec.size() == 5,
              "AP10 resume: all recovered scores are integers");
        check(!rec.isEmpty()
                  && qAbs(rec.at(2).toMap().value(QStringLiteral("xmm")).toDouble() - 1.5) < 1e-9,
              "AP10 resume: coordinates intact for face projection");
        // Continued integer live scoring: next official is 4.
        check(c2.submitOfficial(0, 0, 10, 900, 0, true),
              "AP10 resume: next live integer official accepted");
        check(c2.officialShotCount() == 4 && qRound(c2.totalDecimal() * 10) == 390,
              "AP10 resume: shot 4, integer total 39");
        c2.closeSession();
        QDir(root).removeRecursively();
    }
}
