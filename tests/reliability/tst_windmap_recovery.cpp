// Wind Map — snapshot / replay recovery matrix (stage 4).
//
// The question this file exists to answer: can Wind Map state recorded BEFORE
// a StateSnapshot survive recovery?
//
// ReplayEngine::replay defaults to useSnapshot=true and folds only the tail
// after the last StateSnapshot. Anything absent from the serialized snapshot
// is therefore lost at that boundary. Wind Map is snapshot-serialised (state
// v4) for exactly that reason, and the pre/post-boundary cases below are the
// proof — several of them fail outright if the windMap keys are removed.
#include "test_support.h"

#include "training/WindMapTypes.h"
#include "reliability/events/EventTypes.h"
#include "reliability/replay/ReplayEngine.h"
#include "reliability/reducer/SessionState.h"

#include <QString>

using namespace ta::rel;
using namespace ta::training;

namespace {

WindSnapshotFields wmMeasured(qint16 deg, qint32 hundredths, qint64 ms,
                              const QString& note = QString())
{
    WindSnapshotFields w;
    w.windValid = true; w.windCalm = false;
    w.windDirectionDegrees = deg; w.windSpeedHundredthMs = hundredths;
    w.windSource = 0; w.windRecordedMs = ms; w.windNote = note;
    return w;
}
WindSnapshotFields wmCalm(qint64 ms)
{
    WindSnapshotFields w;
    w.windValid = true; w.windCalm = true; w.windRecordedMs = ms;
    return w;
}
WindSnapshotFields wmNone() { return WindSnapshotFields{}; }

WindMapShotAccepted counted(qint32 id, qint8 pos, const WindSnapshotFields& w,
                            qint32 x = 0, qint32 y = 0)
{
    WindMapShotAccepted e;
    static_cast<WindSnapshotFields&>(e) = w;
    e.shot = testjournal::shot(static_cast<qint16>(id), 102, x, y);
    e.shotId = id; e.position = pos;
    return e;
}
WindMapSighterAccepted sighter(qint32 id, qint8 pos, const WindSnapshotFields& w)
{
    WindMapSighterAccepted e;
    static_cast<WindSnapshotFields&>(e) = w;
    e.shot = testjournal::shot(0, 95, 5, 5);
    e.shotId = id; e.position = pos;
    return e;
}
WindConditionChanged condition(const WindSnapshotFields& w)
{
    WindConditionChanged e;
    static_cast<WindSnapshotFields&>(e) = w;
    return e;
}

DomainEvent windStart(bool is3P)
{
    WindMapSessionStarted e;
    e.disciplineId = is3P ? QStringLiteral("3P50") : QStringLiteral("PRONE50");
    e.is3P = is3P;
    e.positionSequence = is3P ? QStringLiteral("K-P-S") : QString();
    return e;
}

// Writes a script and replays it through the REAL path, snapshot fast-path
// enabled (the default). Returns the folded state.
SessionState replayScript(const QVector<DomainEvent>& events, bool* ok)
{
    MemoryJournalFile file;
    testjournal::writeScript(file, events);
    const ReplayResult r = ReplayEngine::replayBytes(
        file.data, QString::fromLatin1(testjournal::kSid), /*useSnapshot*/ true);
    *ok = r.ok;
    return r.state;
}

// The header every script needs before a Wind Map session can start.
QVector<DomainEvent> header(bool is3P)
{
    QVector<DomainEvent> v;
    v << testjournal::sessionStarted(is3P ? Discipline::ThreePositions50m
                                          : Discipline::Prone50m,
                                     QStringLiteral("Training"),
                                     QStringLiteral("Alex Example"));
    return v;
}

// Folds a script WITHOUT a snapshot, builds a StateSnapshot from the result,
// and returns it — the way a real snapshot would be produced mid-session.
DomainEvent snapshotOf(const QVector<DomainEvent>& prefix)
{
    MemoryJournalFile f;
    testjournal::writeScript(f, prefix);
    const ReplayResult r = ReplayEngine::replayBytes(
        f.data, QString::fromLatin1(testjournal::kSid), /*useSnapshot*/ false);
    return buildStateSnapshot(r.state);
}

void expectShot(const SessionState& s, int idx, qint32 shotId, bool sighterFlag,
                bool valid, bool calm, qint16 deg, qint32 hundredths,
                const char* what)
{
    if (idx >= s.wmShots.size()) {
        check(false, QString(QStringLiteral("recovery: %1 — shot %2 missing"))
                         .arg(QLatin1String(what)).arg(idx));
        return;
    }
    const WindMapShotRecord& r = s.wmShots[idx];
    check(r.shotId == shotId && r.sighter == sighterFlag
          && r.windValid == valid && r.windCalm == calm
          && r.windDirectionDegrees == deg && r.windSpeedHundredthMs == hundredths,
          QString(QStringLiteral("recovery: %1")).arg(QLatin1String(what)),
          QString(QStringLiteral("got id=%1 sighter=%2 valid=%3 calm=%4 deg=%5 sp=%6"))
              .arg(r.shotId).arg(r.sighter).arg(r.windValid).arg(r.windCalm)
              .arg(r.windDirectionDegrees).arg(r.windSpeedHundredthMs));
}

} // namespace

void run_windmap_recovery_tests()
{
    std::printf("--- Wind Map recovery matrix (stage 4) ---\n");

    // 1. empty Wind Map session
    {
        bool ok = false;
        auto s = replayScript(header(false) << windStart(false), &ok);
        check(ok, "1. empty session replays");
        check(s.wmActive && !s.wmCompleted, "1. session is active, not complete");
        check(s.wmDisciplineId == QLatin1String("PRONE50"), "1. discipline restored");
        check(!s.wmThreePositions && s.wmCurrentPosition == 0, "1. Prone has no 3P position");
        check(s.wmShots.isEmpty() && s.wmConditionChanges == 0, "1. no shots, no changes");
        check(!s.wmWindValid, "1. no standing condition yet — absence, not calm");
    }

    // 2. condition set, no shots
    {
        bool ok = false;
        auto s = replayScript(header(false) << windStart(false)
                                            << condition(wmMeasured(90, 250, 5000)), &ok);
        check(ok && s.wmWindValid && !s.wmWindCalm, "2. standing condition restored");
        check(s.wmWindDirectionDegrees == 90 && s.wmWindSpeedHundredthMs == 250,
              "2. direction and speed restored exactly");
        check(s.wmConditionChanges == 1 && s.wmShots.isEmpty(), "2. counted the change, no shots");
    }

    // 3. Calm condition  /  4. NoReading
    {
        bool ok = false;
        auto s = replayScript(header(false) << windStart(false)
                                            << condition(wmCalm(6000)), &ok);
        check(ok && s.wmWindValid && s.wmWindCalm, "3. Calm restored as a valid reading");
        check(s.wmWindDirectionDegrees == 0 && s.wmWindSpeedHundredthMs == 0,
              "3. Calm carries no inferred direction");

        auto t = replayScript(header(false) << windStart(false)
                                            << condition(wmNone()), &ok);
        check(ok && !t.wmWindValid && !t.wmWindCalm,
              "4. NoReading restored as absence, NOT as calm");
    }

    // 5. condition changed before the first shot
    {
        bool ok = false;
        auto s = replayScript(header(false) << windStart(false)
                                            << condition(wmMeasured(0, 100, 1000))
                                            << counted(1, 0, wmMeasured(0, 100, 1000)), &ok);
        check(ok && s.wmShots.size() == 1, "5. one shot recorded");
        expectShot(s, 0, 1, false, true, false, 0, 100, "5. shot carries the standing condition");
    }

    // 6/7. multiple changes between shots — EARLIER SHOTS KEEP THEIR OWN SNAPSHOT
    {
        bool ok = false;
        auto s = replayScript(header(false) << windStart(false)
            << condition(wmMeasured(270, 250, 1000))
            << counted(1, 0, wmMeasured(270, 250, 1000))
            << condition(wmMeasured(90, 500, 2000))
            << counted(2, 0, wmMeasured(90, 500, 2000))
            << condition(wmCalm(3000))
            << counted(3, 0, wmCalm(3000)), &ok);
        check(ok && s.wmShots.size() == 3, "6. three shots across three conditions");
        check(s.wmConditionChanges == 3, "6. three condition changes counted");
        expectShot(s, 0, 1, false, true, false, 270, 250, "7. shot 1 kept its ORIGINAL 270/2.50");
        expectShot(s, 1, 2, false, true, false,  90, 500, "7. shot 2 kept its own 90/5.00");
        expectShot(s, 2, 3, false, true, true,    0,   0, "7. shot 3 kept Calm");
        check(s.wmWindCalm, "7. the standing condition is the LAST one set");
    }

    // 8. sighters and counted shots remain distinct
    {
        bool ok = false;
        auto s = replayScript(header(false) << windStart(false)
            << condition(wmMeasured(45, 300, 1000))
            << sighter(1, 0, wmMeasured(45, 300, 1000))
            << counted(2, 0, wmMeasured(45, 300, 1000)), &ok);
        check(ok && s.wmShots.size() == 2, "8. both shots recorded");
        expectShot(s, 0, 1, true,  true, false, 45, 300, "8. the sighter is flagged as a sighter");
        expectShot(s, 1, 2, false, true, false, 45, 300, "8. the counted shot is not");
        int sighters = 0, countedN = 0;
        for (const auto& r : s.wmShots) (r.sighter ? sighters : countedN)++;
        check(sighters == 1 && countedN == 1, "8. sighters never merge into counted shots");
    }

    // 9/10. crash immediately after a condition change / after an accepted shot
    {
        bool ok = false;
        auto s = replayScript(header(false) << windStart(false)
            << condition(wmMeasured(180, 700, 1000)), &ok);
        check(ok && s.wmWindValid && s.wmWindSpeedHundredthMs == 700 && s.wmShots.isEmpty(),
              "9. crash after a condition change loses nothing");

        auto t = replayScript(header(false) << windStart(false)
            << condition(wmMeasured(180, 700, 1000))
            << counted(1, 0, wmMeasured(180, 700, 1000)), &ok);
        check(ok && t.wmShots.size() == 1 && t.wmNextShotId == 2,
              "10. crash after an accepted shot keeps the shot and the next id");
    }

    // 11-14. 3P positions — change, and recovery in each position
    {
        struct PosCase { qint8 pos; const char* name; };
        const PosCase cases[] = { {1, "12. Kneeling"}, {2, "13. Prone"}, {3, "14. Standing"} };
        for (const PosCase& c : cases) {
            QVector<DomainEvent> script = header(true);
            script << windStart(true) << condition(wmMeasured(135, 220, 1000));
            if (c.pos != 1) {
                WindMapPositionChanged mv; mv.fromPosition = 1; mv.toPosition = c.pos;
                script << mv;
            }
            script << counted(1, c.pos, wmMeasured(135, 220, 1000));
            bool ok = false;
            auto s = replayScript(script, &ok);
            check(ok && s.wmThreePositions && s.wmCurrentPosition == c.pos,
                  QString(QStringLiteral("%1: position restored"))
                      .arg(QLatin1String(c.name)));
            check(s.wmShots.size() == 1 && s.wmShots[0].position == c.pos,
                  QString(QStringLiteral("%1: the shot kept its position"))
                      .arg(QLatin1String(c.name)));
        }

        // 11. crash right after a position change, before any shot in it
        QVector<DomainEvent> script = header(true);
        WindMapPositionChanged mv; mv.fromPosition = 1; mv.toPosition = 3;
        script << windStart(true) << condition(wmCalm(1000)) << mv;
        bool ok = false;
        auto s = replayScript(script, &ok);
        check(ok && s.wmCurrentPosition == 3 && s.wmShots.isEmpty(),
              "11. crash after a position change keeps the new position");

        // 3P positions never pool: three shots, one per position, stay separate
        QVector<DomainEvent> multi = header(true);
        multi << windStart(true) << condition(wmMeasured(0, 100, 1000))
              << counted(1, 1, wmMeasured(0, 100, 1000));
        WindMapPositionChanged toP; toP.fromPosition = 1; toP.toPosition = 2;
        multi << toP << counted(2, 2, wmMeasured(0, 100, 1000));
        WindMapPositionChanged toS; toS.fromPosition = 2; toS.toPosition = 3;
        multi << toS << counted(3, 3, wmMeasured(0, 100, 1000));
        auto m = replayScript(multi, &ok);
        check(ok && m.wmShots.size() == 3, "3P: three shots recorded");
        check(m.wmShots[0].position == 1 && m.wmShots[1].position == 2
              && m.wmShots[2].position == 3,
              "3P: Kneeling / Prone / Standing stay separate in state — never pooled");
    }

    // 15. completed session
    {
        WindMapSessionCompleted done; done.countedShots = 1; done.sighterShots = 0;
        done.conditionChanges = 1;
        bool ok = false;
        auto s = replayScript(header(false) << windStart(false)
            << condition(wmCalm(1000)) << counted(1, 0, wmCalm(1000)) << done, &ok);
        check(ok && s.wmCompleted, "15. completed session restores as completed");
        check(s.lifecycle == Lifecycle::Complete, "15. lifecycle folded to Complete");
        check(s.wmShots.size() == 1, "15. its shot survived");
    }

    // ── 16-19. THE SNAPSHOT BOUNDARY — the reason state v4 exists ───────────

    // 16. snapshot BEFORE the Wind Map session starts
    {
        QVector<DomainEvent> pre = header(false);
        QVector<DomainEvent> script = pre;
        script << snapshotOf(pre)
               << windStart(false) << condition(wmCalm(1000))
               << counted(1, 0, wmCalm(1000));
        bool ok = false;
        auto s = replayScript(script, &ok);
        check(ok && s.wmActive && s.wmShots.size() == 1,
              "16. snapshot before the session — everything after it survives");
    }

    // 17. snapshot DURING the session, with wind state already recorded
    {
        QVector<DomainEvent> pre = header(false);
        pre << windStart(false) << condition(wmMeasured(270, 250, 1000));
        QVector<DomainEvent> script = pre;
        script << snapshotOf(pre)
               << counted(1, 0, wmMeasured(270, 250, 1000));
        bool ok = false;
        auto s = replayScript(script, &ok);
        check(ok, "17. snapshot mid-session replays");
        // Everything below is PRE-snapshot state. Without windMap in the
        // snapshot these are all lost.
        check(s.wmActive, "17. wmActive survives the snapshot boundary");
        check(s.wmDisciplineId == QLatin1String("PRONE50"),
              "17. discipline survives the snapshot boundary");
        check(s.wmWindValid && s.wmWindDirectionDegrees == 270
              && s.wmWindSpeedHundredthMs == 250,
              "17. the STANDING CONDITION survives the snapshot boundary");
        check(s.wmConditionChanges == 1,
              "17. the condition-change count survives the snapshot boundary");
        check(s.wmShots.size() == 1, "17. the post-snapshot shot is folded in");
    }

    // 18/19. snapshot AFTER several shots, then more shots in the tail
    {
        QVector<DomainEvent> pre = header(false);
        pre << windStart(false)
            << condition(wmMeasured(270, 250, 1000))
            << sighter(1, 0, wmMeasured(270, 250, 1000))
            << counted(2, 0, wmMeasured(270, 250, 1000))
            << condition(wmCalm(2000))
            << counted(3, 0, wmCalm(2000));
        QVector<DomainEvent> script = pre;
        script << snapshotOf(pre)
               << condition(wmMeasured(90, 800, 3000))
               << counted(4, 0, wmMeasured(90, 800, 3000));
        bool ok = false;
        auto s = replayScript(script, &ok);
        check(ok, "18. snapshot after several shots replays");
        check(s.wmShots.size() == 4,
              "18. PRE-SNAPSHOT SHOTS ARE NOT LOST — all four are present",
              QString::number(s.wmShots.size()));
        expectShot(s, 0, 1, true,  true, false, 270, 250, "18. pre-snapshot sighter survives with its snapshot");
        expectShot(s, 1, 2, false, true, false, 270, 250, "18. pre-snapshot shot 2 keeps 270/2.50");
        expectShot(s, 2, 3, false, true, true,    0,   0, "18. pre-snapshot shot 3 keeps Calm");
        expectShot(s, 3, 4, false, true, false,  90, 800, "19. the tail shot folds in after the boundary");
        check(s.wmConditionChanges == 3, "19. condition changes span the boundary correctly");
        check(s.wmNextShotId == 5, "19. the next shot id spans the boundary");

        // Same journal, snapshot path DISABLED, must fold to the same state.
        MemoryJournalFile f;
        testjournal::writeScript(f, script);
        const ReplayResult noSnap = ReplayEngine::replayBytes(
            f.data, QString::fromLatin1(testjournal::kSid), /*useSnapshot*/ false);
        check(noSnap.ok, "19. fold-from-zero also succeeds");
        check(noSnap.state == s,
              "19. SNAPSHOT PATH AND FOLD-FROM-ZERO AGREE — the proof that "
              "nothing is lost at the boundary");
    }

    // 20. a corrupt Wind Map event fails closed
    {
        // An unsupported discipline is refused by the payload validator, so the
        // event never reaches the reducer and no partial session is produced.
        WindMapSessionStarted bad;
        bad.disciplineId = QStringLiteral("AR10");
        bad.is3P = false;
        check(!bad.validate().ok, "20. an unsupported discipline is refused");

        WindMapShotAccepted contradictory;
        contradictory.shot = testjournal::shot(1, 100);
        contradictory.shotId = 1; contradictory.position = 0;
        contradictory.windValid = false;
        contradictory.windCalm = true;      // absence claiming to be calm
        check(!contradictory.validate().ok,
              "20. a no-reading claiming to be calm is refused");

        // A 3P position on a Prone session is refused by the reducer.
        bool ok = false;
        auto s = replayScript(header(false) << windStart(false)
                                            << counted(1, 3, wmCalm(1000)), &ok);
        check(!ok || s.wmShots.isEmpty(),
              "20. a 3P position on a Prone session does not produce a partial session");
    }

    // 21. a missing reading is never inferred, even next to readings
    {
        bool ok = false;
        auto s = replayScript(header(false) << windStart(false)
            << condition(wmMeasured(270, 250, 1000))
            << counted(1, 0, wmMeasured(270, 250, 1000))
            << counted(2, 0, wmNone())            // recorded with NO reading
            << counted(3, 0, wmMeasured(270, 250, 1000)), &ok);
        check(ok && s.wmShots.size() == 3, "21. three shots recorded");
        expectShot(s, 1, 2, false, false, false, 0, 0,
                   "21. the middle shot stays a NO READING — not back-filled from either neighbour");
        check(s.wmWindValid,
              "21. the standing condition is untouched by a no-reading shot");
    }

    // 22. existing non-Wind-Map journals replay unchanged
    {
        bool ok = false;
        auto s = replayScript(header(false)
            << testjournal::sessionStarted(Discipline::Prone50m, QStringLiteral("60"),
                                           QStringLiteral("Alex Example")), &ok);
        check(!s.wmActive && s.wmShots.isEmpty() && s.wmConditionChanges == 0,
              "22. a journal with no Wind Map events leaves Wind Map state empty");
        check(!s.wmWindValid && !s.wmWindCalm,
              "22. and its standing condition is absence, not a default calm");
    }

    // Backward compatibility: a v3 state JSON has no windMap keys at all and
    // must still load, with Wind Map absent rather than an error.
    {
        SessionState round;
        const QByteArray v4 = serializeSessionState(SessionState());
        QByteArray v3 = v4;
        // Strip the two v4 keys to simulate a pre-Wind-Map snapshot.
        const int wmAt = v3.indexOf("\"windMap\":");
        check(wmAt > 0, "compat: the v4 payload contains the windMap object");
        const int shotsEnd = v3.indexOf("\"windMapShots\":[]") + 17;
        if (wmAt > 0 && shotsEnd > 17) {
            v3.remove(wmAt, shotsEnd - wmAt + 1);   // + the trailing comma
            v3.replace("\"stateVersion\":4", "\"stateVersion\":3");
            const ReliabilityResult r = deserializeSessionState(v3, &round);
            check(r.ok, "compat: a v3 state with no windMap keys still loads",
                  r.error.technicalDetail);
            check(!round.wmActive && round.wmShots.isEmpty(),
                  "compat: it restores to 'no Wind Map session', not an error");
            check(!round.wmWindValid,
                  "compat: and its wind reading is absent, not inferred");
        }
    }
}
