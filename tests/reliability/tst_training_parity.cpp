// Stage 4.1 — Training programme snapshot parity.
//
// Technical Blocks, Call & Diagnose and Position Transition were not
// snapshot-serialised. They survived only because nothing in production emits
// a StateSnapshot, and ReplayEngine folds just the tail after one. These
// matrices place a snapshot IN THE MIDDLE of each programme and prove that
// state recorded before the boundary is still there afterwards.
//
// Every case also compares the snapshot fast path against fold-from-zero. That
// comparison uses SessionState::operator==, which now includes all three
// projections — so a field that is folded but not serialised fails here.
#include "test_support.h"

#include "reliability/events/EventTypes.h"
#include "reliability/replay/ReplayEngine.h"
#include "reliability/reducer/SessionState.h"

#include <QString>

using namespace ta::rel;

namespace {

SessionState foldWithSnapshot(const QVector<DomainEvent>& events, bool* ok)
{
    MemoryJournalFile f;
    testjournal::writeScript(f, events);
    const ReplayResult r = ReplayEngine::replayBytes(
        f.data, QString::fromLatin1(testjournal::kSid), /*useSnapshot*/ true);
    *ok = r.ok;
    return r.state;
}

SessionState foldFromZero(const QVector<DomainEvent>& events, bool* ok)
{
    MemoryJournalFile f;
    testjournal::writeScript(f, events);
    const ReplayResult r = ReplayEngine::replayBytes(
        f.data, QString::fromLatin1(testjournal::kSid), /*useSnapshot*/ false);
    *ok = r.ok;
    return r.state;
}

DomainEvent snapshotOf(const QVector<DomainEvent>& prefix)
{
    bool ok = false;
    return buildStateSnapshot(foldFromZero(prefix, &ok));
}

// Runs `prefix` + snapshot + `tail` and asserts the two replay paths agree.
// That equality IS the parity guarantee: anything folded but not serialised
// diverges here.
SessionState boundaryCase(const QVector<DomainEvent>& prefix,
                          const QVector<DomainEvent>& tail,
                          const char* label)
{
    QVector<DomainEvent> script = prefix;
    script << snapshotOf(prefix);
    for (const DomainEvent& e : tail)
        script << e;

    bool okSnap = false, okZero = false;
    const SessionState withSnap = foldWithSnapshot(script, &okSnap);
    const SessionState fromZero = foldFromZero(script, &okZero);
    check(okSnap && okZero,
          QString(QStringLiteral("%1: both replay paths succeed")).arg(QLatin1String(label)));
    check(withSnap == fromZero,
          QString(QStringLiteral("%1: snapshot path == fold-from-zero")).arg(QLatin1String(label)));
    return withSnap;
}

QVector<DomainEvent> trainingHeader()
{
    QVector<DomainEvent> v;
    v << testjournal::sessionStarted(Discipline::AirRifle10m,
                                     QStringLiteral("Training"),
                                     QStringLiteral("Alex Example"));
    return v;
}

ShotCore sc(qint16 n, qint16 score, qint32 x = 10, qint32 y = -5, qint32 split = 4000)
{
    ShotCore s;
    s.shotNumber = n; s.withinStage = n; s.stageId = n > 0 ? 1 : 0;
    s.xHundredthMm = x; s.yHundredthMm = y; s.scoreTenths = score;
    s.splitMs = split; s.externalId = n; s.simulated = true;
    return s;
}

} // namespace

void run_training_parity_tests()
{
    std::printf("--- Stage 4.1: Training programme snapshot parity ---\n");

    // ══ TECHNICAL BLOCKS ═══════════════════════════════════════════════════
    {
        const DomainEvent start = TrainingSessionStarted{
            QStringLiteral("technical_blocks"), Discipline::AirRifle10m,
            5, 6, 2, QStringLiteral("Trigger"), 0};

        // crash during sighters
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << TrainingSighterPhaseStarted{0, 1}
                << TrainingSighterAccepted{sc(0, 98), 0, 1};
            const auto s = boundaryCase(pre, {TrainingSighterAccepted{sc(0, 95), 0, 1}},
                                        "TB crash during sighters");
            check(s.trainingActive && s.trainingInSighterPhase,
                  "TB sighters: the sighter PHASE survives the boundary");
            check(s.trainingSighters.size() == 2,
                  "TB sighters: pre-snapshot sighter is not lost",
                  QString::number(s.trainingSighters.size()));
            check(s.trainingSighterPos.size() == s.trainingSighters.size(),
                  "TB sighters: the parallel position vector stays in step");
            check(s.trainingBlocks.isEmpty(),
                  "TB sighters: sighters never leak into counted blocks");
        }

        // crash during a block, and immediately after a shot
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << TrainingBlockStarted{1, 0}
                << TrainingShotAccepted{sc(1, 104), 1, 1, 0, 0, 0, false}
                << TrainingShotAccepted{sc(2, 101), 1, 2, 0, 0, 0, false};
            const auto s = boundaryCase(pre,
                {TrainingShotAccepted{sc(3, 99), 1, 3, 0, 0, 0, false}},
                "TB crash during a block");
            check(s.trainingCurrentBlock == 1, "TB block: current block survives");
            check(!s.trainingBlocks.isEmpty() && s.trainingBlocks[0].shots.size() == 3,
                  "TB block: pre-snapshot shots are not lost",
                  s.trainingBlocks.isEmpty() ? QStringLiteral("no blocks")
                                             : QString::number(s.trainingBlocks[0].shots.size()));
            if (!s.trainingBlocks.isEmpty() && s.trainingBlocks[0].shots.size() == 3) {
                // Cadence data is a recorded value, never re-derived.
                check(s.trainingBlocks[0].shots[0].splitMs == 4000
                      && s.trainingBlocks[0].shots[0].scoreTenths == 104,
                      "TB block: shot timing and score survive exactly");
            }
            check(s.trainingBlockCount == 5 && s.trainingShotsPerBlock == 6
                  && s.trainingVisibility == 2
                  && s.trainingFocus == QLatin1String("Trigger"),
                  "TB block: programme configuration survives");
        }

        // snapshot BETWEEN blocks, and across a position change
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << TrainingBlockStarted{1, 0}
                << TrainingShotAccepted{sc(1, 104), 1, 1, 0, 0, 0, false}
                << TrainingBlockCompleted{1, 6};
            const auto s = boundaryCase(pre,
                {TrainingBlockStarted{2, 1},
                 TrainingShotAccepted{sc(2, 100), 2, 1, 1, 0, 0, false}},
                "TB snapshot between blocks / across a position change");
            check(s.trainingBlocks.size() == 2, "TB blocks: both blocks present",
                  QString::number(s.trainingBlocks.size()));
            if (s.trainingBlocks.size() == 2) {
                check(s.trainingBlocks[0].completed, "TB blocks: block 1 stays completed");
                check(s.trainingBlocks[0].position == 0 && s.trainingBlocks[1].position == 1,
                      "TB blocks: per-block position survives — positions never pool");
            }
        }

        // completed session
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << TrainingBlockStarted{1, 0}
                << TrainingShotAccepted{sc(1, 104), 1, 1, 0, 0, 0, false}
                << TrainingBlockCompleted{1, 6};
            const auto s = boundaryCase(pre, {TrainingCompleted{5}},
                                        "TB completed session");
            check(s.trainingCompleted, "TB completed: completion survives the boundary");
        }
    }

    // ══ CALL & DIAGNOSE ════════════════════════════════════════════════════
    {
        const DomainEvent start = CallDiagnoseSessionStarted{
            QStringLiteral("call_and_diagnose"), 20, QStringLiteral("Trigger"), 0, false};

        // crash before any shot, still in the sighter phase
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start;
            const auto s = boundaryCase(pre, {CallDiagnoseStarted{0}},
                                        "CD crash before a shot");
            check(s.cdActive, "CD start: the session survives the boundary");
            check(s.cdShotCount == 20 && s.cdFocus == QLatin1String("Trigger"),
                  "CD start: configuration survives");
        }

        // THE CRITICAL CASE: crash while awaiting the athlete's call.
        // The actual shot is already recorded but must not be revealed or
        // discarded — it must come back still awaiting its call.
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << CallDiagnoseStarted{0}
                << CallDiagnoseShotReceived{sc(1, 104, 120, 34), 1, 0};
            const auto s = boundaryCase(pre, {}, "CD crash while AWAITING A CALL");
            check(s.cdShots.size() == 1, "CD awaiting: the actual shot survives",
                  QString::number(s.cdShots.size()));
            if (s.cdShots.size() == 1) {
                check(!s.cdShots[0].hasCall,
                      "CD awaiting: it recovers STILL AWAITING the call — not revealed");
                check(s.cdShots[0].actual.xHundredthMm == 120
                      && s.cdShots[0].actual.yHundredthMm == 34
                      && s.cdShots[0].actual.scoreTenths == 104,
                      "CD awaiting: the hidden actual shot is preserved exactly, not discarded");
            }
            check(s.cdCallingActive, "CD awaiting: the calling phase survives");
        }

        // crash after call confirmation / during reveal
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << CallDiagnoseStarted{0}
                << CallDiagnoseShotReceived{sc(1, 104, 120, 34), 1, 0}
                << CallDiagnoseCallRecorded{1, 0, 250, -180, 4200};
            const auto s = boundaryCase(pre,
                {CallDiagnoseShotReceived{sc(2, 98, -40, 60), 2, 0}},
                "CD crash after call confirmation");
            check(s.cdShots.size() == 2, "CD confirmed: both records survive",
                  QString::number(s.cdShots.size()));
            if (s.cdShots.size() == 2) {
                check(s.cdShots[0].hasCall
                      && s.cdShots[0].calledXHundredthMm == 250
                      && s.cdShots[0].calledYHundredthMm == -180
                      && s.cdShots[0].callSplitMs == 4200,
                      "CD confirmed: the call and its error inputs survive exactly");
                check(!s.cdShots[1].hasCall,
                      "CD confirmed: the NEW shot is still awaiting its own call");
            }
        }

        // snapshot between multiple records, then completion
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << CallDiagnoseStarted{0}
                << CallDiagnoseShotReceived{sc(1, 104), 1, 0}
                << CallDiagnoseCallRecorded{1, 0, 100, 100, 3000}
                << CallDiagnoseShotReceived{sc(2, 99), 2, 0}
                << CallDiagnoseCallRecorded{2, 0, -50, 20, 3100};
            const auto s = boundaryCase(pre,
                {CallDiagnoseCompleted{20, QStringLiteral("good session")}},
                "CD snapshot between records, then completion");
            check(s.cdShots.size() == 2, "CD multi: both pre-snapshot records survive");
            check(s.cdCompleted, "CD multi: completion survives");
        }
    }

    // ══ POSITION TRANSITION ════════════════════════════════════════════════
    {
        const DomainEvent start = PositionTransitionSessionStarted{
            QStringLiteral("position_transition"), QStringLiteral("K,P,S"),
            5, 2, 0, QStringLiteral("Natural point of aim")};

        // crash during PositionSetup
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << PositionSetupStarted{0, 1};
            const auto s = boundaryCase(pre, {PositionChecklistUpdated{0, 1, 2, 1}},
                                        "PT crash during PositionSetup");
            check(s.ptActive && s.ptInSetup,
                  "PT setup: the SETUP phase survives — not mistaken for verification");
            check(!s.ptVerifying, "PT setup: verification is not active");
            check(s.ptSequence == QLatin1String("K,P,S") && s.ptVerificationShots == 5
                  && s.ptRepeats == 2,
                  "PT setup: programme configuration survives");
        }

        // crash during Sighters, then VerificationActive — the three kinds of
        // shot must stay distinguishable across the boundary
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << PositionSetupStarted{0, 1} << PositionReady{0, 1, 42000}
                << PositionSighterAccepted{sc(0, 95, 40, 8, 5000), 0, 1};
            const auto s = boundaryCase(pre,
                {PositionVerificationStarted{0, 1, 42000},
                 PositionVerificationShotAccepted{sc(1, 104, 12, 34, 46000), 0, 1, 1}},
                "PT crash during Sighters into VerificationActive");
            check(!s.ptRecords.isEmpty(), "PT sighters: a position record exists");
            if (!s.ptRecords.isEmpty()) {
                check(s.ptRecords[0].sighters.size() == 1,
                      "PT sighters: the pre-snapshot SIGHTER survives",
                      QString::number(s.ptRecords[0].sighters.size()));
                check(s.ptRecords[0].verifShots.size() == 1,
                      "PT sighters: the counted verification shot is separate",
                      QString::number(s.ptRecords[0].verifShots.size()));
                check(s.ptRecords[0].setupDurationMs != 0 || s.ptRecords[0].readyMonoMs != 0,
                      "PT sighters: setup timing survives as a recorded value");
            }
            check(s.ptVerifying, "PT sighters: the verification phase is restored");
        }

        // crash across K -> P, and during a repeat
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << PositionSetupStarted{0, 1} << PositionReady{0, 1, 42000}
                << PositionVerificationStarted{0, 1, 42000}
                << PositionVerificationShotAccepted{sc(1, 104), 0, 1, 1}
                << PositionVerificationCompleted{0, 1, 5};
            const auto s = boundaryCase(pre,
                {NextPositionTransitionStarted{0, 1, 1},
                 PositionSetupStarted{1, 1},
                 PositionReady{1, 1, 90000},
                 PositionVerificationStarted{1, 1, 90000},
                 PositionVerificationShotAccepted{sc(2, 101), 1, 1, 1}},
                "PT snapshot across K -> P");
            check(s.ptRecords.size() >= 2, "PT K->P: both position records present",
                  QString::number(s.ptRecords.size()));
            if (s.ptRecords.size() >= 2) {
                check(s.ptRecords[0].position == 0 && s.ptRecords[1].position == 1,
                      "PT K->P: the two positions stay separate");
                check(s.ptRecords[0].completed,
                      "PT K->P: the pre-snapshot position stays completed");
            }
            check(s.ptCurrentPosition == 1, "PT K->P: the current position is restored");
        }

        // crash during PositionReview / after completion
        {
            QVector<DomainEvent> pre = trainingHeader();
            pre << start << PositionSetupStarted{0, 1} << PositionReady{0, 1, 42000}
                << PositionVerificationStarted{0, 1, 42000}
                << PositionVerificationShotAccepted{sc(1, 104), 0, 1, 1}
                << PositionVerificationCompleted{0, 1, 5}
                << PositionNoteSaved{0, 1, QStringLiteral("settled quickly")};
            const auto s = boundaryCase(pre,
                {PositionTransitionCompleted{3, QStringLiteral("good rhythm")}},
                "PT crash during review, then completion");
            check(s.ptCompleted, "PT review: completion survives the boundary");
            check(!s.ptRecords.isEmpty() && s.ptRecords[0].note == QLatin1String("settled quickly"),
                  "PT review: the pre-snapshot athlete note survives");
            check(s.ptSessionNote == QLatin1String("good rhythm"),
                  "PT review: the session note from the tail is folded in");
        }
    }

    // ══ COMPATIBILITY AND SAFETY ═══════════════════════════════════════════

    // A v4 state (Wind Map present, no Training programme keys) must still
    // load — the exact shape of the golden fixture committed in Stage 4.
    {
        SessionState src;
        src.sessionKind = QStringLiteral("Training");
        src.trainingActive = true;
        src.trainingProgramId = QStringLiteral("technical_blocks");
        QByteArray v5 = serializeSessionState(src);

        QByteArray v4 = v5;
        const int at = v4.indexOf("\"sessionKind\":");
        const int end = v4.indexOf("\"windMap\":");
        check(at > 0 && end > at,
              "compat: the v5 payload contains the Training keys ahead of windMap");
        if (at > 0 && end > at) {
            v4.remove(at, end - at);                       // drop every v5 key
            v4.replace("\"stateVersion\":5", "\"stateVersion\":4");
            SessionState back;
            const ReliabilityResult r = deserializeSessionState(v4, &back);
            check(r.ok, "compat: a v4 state with no Training keys still loads",
                  r.error.technicalDetail);
            check(!back.trainingActive && back.trainingBlocks.isEmpty()
                  && !back.cdActive && !back.ptActive && back.sessionKind.isEmpty(),
                  "compat: it restores to 'no programme', not an error");
        }

        // ...and a full v5 round-trip is lossless.
        SessionState round;
        const ReliabilityResult rr = deserializeSessionState(v5, &round);
        check(rr.ok && round.sessionKind == QLatin1String("Training")
              && round.trainingActive
              && round.trainingProgramId == QLatin1String("technical_blocks"),
              "compat: a v5 state round-trips losslessly");
    }

    // A malformed programme object fails safely rather than producing a
    // half-restored session.
    {
        SessionState src;
        QByteArray bad = serializeSessionState(src);
        bad.replace("\"training\":{", "\"training\":[");
        SessionState out;
        const ReliabilityResult r = deserializeSessionState(bad, &out);
        check(!r.ok, "safety: a malformed 'training' value is rejected, not guessed");
    }

    // A journal with no Training events leaves every projection empty.
    {
        bool ok = false;
        const auto s = foldWithSnapshot(
            QVector<DomainEvent>() << testjournal::sessionStarted(
                Discipline::Prone50m, QStringLiteral("60"),
                QStringLiteral("Alex Example")), &ok);
        check(ok && !s.trainingActive && !s.cdActive && !s.ptActive
              && s.trainingBlocks.isEmpty() && s.cdShots.isEmpty() && s.ptRecords.isEmpty(),
              "compat: a competition journal leaves all programme projections empty");
    }
}
