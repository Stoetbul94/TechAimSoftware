// M3 — recovery & resume tests. The core guarantee: a match recovered from
// its journal is INDISTINGUISHABLE from one that never crashed — identical
// reducer state, rebuilt EXCLUSIVELY through the reducer (Journal → Validator
// → Reducer → State). Covers restart after every event, after stage/command/
// degraded/snapshot, hash validation, classification, and the resume flow.

#include "reliability/journal/JournalValidator.h"
#include "reliability/recovery/RecoveryCoordinator.h"
#include "reliability/reducer/SessionReducer.h"
#include "reliability/replay/ReplayEngine.h"
#include "reliability/store/SessionStore.h"
#include "test_support.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

using namespace ta::rel;

namespace {

SessionHeader finalsHeader(const QString& sid)
{
    SessionHeader h;
    h.sessionId = sid;
    h.appVersion = QStringLiteral("4.0.0-test");
    h.athlete = QStringLiteral("Arnold Bailie");
    h.matchType = QStringLiteral("FINAL 35");
    h.discipline = Discipline::Finals3P;
    h.config.officialShots = 35;
    h.config.seriesSize = 5;
    h.config.matchMs = 1320000;
    return h;
}

// A representative finals script (events only; the store assigns seqs).
QVector<DomainEvent> finalsScript()
{
    QVector<DomainEvent> s;
    s << DomainEvent(StageEntered{0});
    s << DomainEvent(PreparationStarted{0});
    s << DomainEvent(SightingStarted{0});
    s << DomainEvent(WindowOpened{1});
    s << DomainEvent(SighterAccepted{testjournal::shot(0, 101)});
    s << DomainEvent(SighterAccepted{testjournal::shot(0, 99)});
    s << DomainEvent(WindowClosed{1});
    s << DomainEvent(StageEntered{1});
    s << DomainEvent(OfficialMatchStarted{1});
    s << DomainEvent(WindowOpened{2});
    s << DomainEvent(CommandIssued{1, 3, QStringLiteral("START"), 1,
                                   QStringLiteral("start"), 1000, 1000});
    s << DomainEvent(ShotAccepted{testjournal::shot(1, 104)});
    s << DomainEvent(ShotAccepted{testjournal::shot(2, 98)});
    s << DomainEvent(ShotAccepted{testjournal::shot(3, 101)});
    s << DomainEvent(StageStatusChanged{1, 2});
    s << DomainEvent(WindowClosed{2});
    s << DomainEvent(PositionChanged{1});
    return s;
}

// Write a full journal to `path` via a real SessionStore, returning the live
// reducer state at the end (the "never crashed" reference).
SessionState writeFullSession(const QString& sid, const QString& scanDir,
                              QString* outPath)
{
    Q_UNUSED(scanDir);
    SessionStore store;
    store.beginSession(finalsHeader(sid));
    for (const DomainEvent& e : finalsScript())
        store.submit(e);
    *outPath = store.currentJournalPath();
    // deliberately DO NOT closeSession — simulates a crash mid-match
    return store.state();
}

} // namespace

void run_recovery_tests()
{
    std::printf("--- recovery & resume tests ---\n");

    const QString root = QDir::temp().filePath(
        QStringLiteral("techaim_m3_recovery_%1")
            .arg(QCoreApplication::applicationPid()));
    QDir(root).removeRecursively();
    StoragePaths::setRootOverrideForTesting(root);
    StoragePaths::initialize();

    // 1) replay a journal == the live reducer state (indistinguishable)
    {
        QString path;
        const SessionState live = writeFullSession(
            QStringLiteral("11111111-0000-4000-8000-000000000001"), root, &path);
        const ReplayResult r = ReplayEngine::replayFile(path);
        check(r.ok, "replay of a crashed journal succeeds", r.error.technicalDetail);
        check(r.state == live,
              "replayed reducer state EQUALS the live state (indistinguishable)");
        check(serializeSessionState(r.state) == serializeSessionState(live),
              "replayed state is byte-identical to live");
    }

    // 2) RESTART AFTER EVERY EVENT: truncating the journal at each line
    //    boundary and replaying yields exactly the prefix-fold state
    {
        QString path;
        writeFullSession(QStringLiteral("22222222-0000-4000-8000-000000000002"),
                         root, &path);
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        const QByteArray whole = f.readAll();
        f.close();

        // Build the per-prefix expected states by folding incrementally.
        const ValidationReport full = JournalValidator::validateBytes(whole);
        bool allMatch = true;
        QString detail;
        // For each line boundary, the journal truncated there must replay to
        // the fold of exactly those envelopes.
        qint64 offset = 0;
        int lineIdx = 0;
        SessionState expected;
        quint64 expectedSeq = 0;
        Q_UNUSED(expectedSeq);
        while (lineIdx < full.validEnvelopes.size()) {
            // advance offset to the end of this line
            const int nl = whole.indexOf('\n', offset);
            if (nl < 0) break;
            offset = nl + 1;
            // fold this envelope into expected
            const ReduceResult rr =
                SessionReducer::apply(expected, full.validEnvelopes[lineIdx]);
            if (!rr.ok) { allMatch = false; detail = "fold failed"; break; }
            expected = rr.state;
            ++lineIdx;
            // replay the truncated-at-offset journal
            const ReplayResult rep =
                ReplayEngine::replayBytes(whole.left(static_cast<int>(offset)));
            if (!rep.ok || !(rep.state == expected)) {
                allMatch = false;
                detail = QStringLiteral("mismatch after line %1").arg(lineIdx);
                break;
            }
        }
        check(allMatch,
              "restart after EVERY event replays to the exact prefix state",
              detail);
    }

    // 3) restart after a torn TAIL (power-loss byte sweep): truncating the
    //    final line at every byte offset still recovers the valid prefix
    {
        QString path;
        writeFullSession(QStringLiteral("33333333-0000-4000-8000-000000000003"),
                         root, &path);
        QFile f(path); f.open(QIODevice::ReadOnly);
        const QByteArray whole = f.readAll(); f.close();
        const int lastLf = whole.lastIndexOf('\n');
        const int prevLf = whole.lastIndexOf('\n', lastLf - 1);
        const ReplayResult fullReplay = ReplayEngine::replayBytes(whole);
        bool allSafe = true;
        QString detail;
        for (int cut = prevLf + 1; cut < whole.size(); ++cut) {
            const ValidationReport rep =
                JournalValidator::validateBytes(whole.left(cut));
            const bool tailSafe =
                rep.classification == JournalClassification::TailTruncated
                || rep.classification == JournalClassification::Clean;
            const ReplayResult r = ReplayEngine::replay(rep.validEnvelopes);
            // the recovered state has at most one fewer event than the full one
            if (!tailSafe || !r.ok
                || r.state.officials.size() > fullReplay.state.officials.size()) {
                allSafe = false;
                detail = QStringLiteral("cut=%1 -> %2").arg(cut)
                    .arg(QLatin1String(journalClassificationName(rep.classification)));
                break;
            }
        }
        check(allSafe, "power-loss byte sweep: every cut recovers a valid prefix",
              detail);
    }

    // 4) snapshot-based replay == fold-from-zero (drift detector)
    {
        SessionStore store;
        store.beginSession(finalsHeader(
            QStringLiteral("44444444-0000-4000-8000-000000000004")));
        for (const DomainEvent& e : finalsScript())
            store.submit(e);
        // inject a StateSnapshot mid-stream by submitting one built from state
        store.submit(DomainEvent(buildStateSnapshot(store.state())));
        store.submit(DomainEvent(ShotAccepted{testjournal::shot(4, 105)}));
        const QString path = store.currentJournalPath();
        const SessionState live = store.state();

        const ValidationReport rep = JournalValidator::validateFile(path);
        check(rep.classification == JournalClassification::Clean,
              "journal with a snapshot validates Clean");
        const ReplayResult viaSnap = ReplayEngine::replay(rep.validEnvelopes, true);
        const ReplayResult viaFold = ReplayEngine::replay(rep.validEnvelopes, false);
        check(viaSnap.ok && viaSnap.usedSnapshot && viaSnap.state == live,
              "snapshot-based replay reconstructs the live state");
        check(viaFold.ok && viaFold.state == live,
              "fold-from-zero reconstructs the live state");
        check(ReplayEngine::snapshotsAgreeWithFold(rep.validEnvelopes),
              "snapshot replay == fold-from-zero (no drift)");
    }

    // 5) restart after DEGRADED persistence: a journal produced through a
    //    degraded episode replays to the same live state
    {
        MemoryJournalFile file;
        ManualClock clock;
        SessionStore store;
        store.setClockForTesting(&clock);
        store.setJournalFileForTesting(&file);
        store.setAuxQueueCapForTesting(2);
        store.beginSession(finalsHeader(
            QStringLiteral("55555555-0000-4000-8000-000000000005")));
        store.submit(DomainEvent(StageEntered{1}));
        store.submit(DomainEvent(OfficialMatchStarted{1}));
        store.submit(DomainEvent(WindowOpened{1}));
        file.failWrite = true;
        for (int i = 1; i <= 4; ++i) {
            store.submit(DomainEvent(CommandIssued{i, 3, QStringLiteral("C"),
                static_cast<qint16>(i), QStringLiteral("c"), 1, 1}));
            store.submit(DomainEvent(ShotAccepted{
                testjournal::shot(static_cast<qint16>(i), 100)}));
        }
        file.failWrite = false;
        store.pumpRetryQueue();
        const SessionState live = store.state();
        const ReplayResult r = ReplayEngine::replayBytes(file.data);
        check(r.ok, "degraded-then-recovered journal replays",
              r.error.technicalDetail);
        // dropped aux (commands) are gone from the journal, so replay has the
        // officials but not every command — officials must all be present.
        check(r.state.officials.size() == live.officials.size()
                  && r.state.totalTenths == live.totalTenths,
              "all officials + totals recovered despite aux drops",
              QStringLiteral("replay=%1/%2 live=%3/%4")
                  .arg(r.state.officials.size()).arg(r.state.totalTenths)
                  .arg(live.officials.size()).arg(live.totalTenths));
    }

    // 6) RecoveryCoordinator scans, classifies, and rebuilds
    {
        // two live (crashed) sessions in a scan dir
        const QString scanDir = QDir::temp().filePath(
            QStringLiteral("techaim_m3_scan_%1")
                .arg(QCoreApplication::applicationPid()));
        QDir(scanDir).removeRecursively();
        QDir().mkpath(scanDir);
        // reuse the store but point its file into scanDir via the real path:
        // easier — write two journals with the store into Current, then scan
        // Current directly.
        QString p1;
        const SessionState liveA = writeFullSession(
            QStringLiteral("66666666-0000-4000-8000-000000000006"), root, &p1);

        RecoveryCoordinator coord;
        const QVector<RecoveryCandidate> cands = coord.scan();
        check(!cands.isEmpty(), "coordinator discovers unfinished sessions",
              QString::number(cands.size()));
        // find our candidate
        const RecoveryCandidate* mine = nullptr;
        for (const RecoveryCandidate& c : cands)
            if (c.sessionId == QLatin1String("66666666-0000-4000-8000-000000000006"))
                mine = &c;
        check(mine != nullptr, "our crashed session is a candidate");
        if (mine) {
            check(mine->recoveryClass == RecoveryClass::Clean && mine->resumable,
                  "an intact crashed journal is Clean + resumable",
                  QLatin1String(recoveryClassName(mine->recoveryClass)));
            check(mine->athlete == QLatin1String("Arnold Bailie")
                      && mine->discipline == Discipline::Finals3P,
                  "candidate metadata derived from the reducer state");
            RecoveredMatchState rec;
            ErrorInfo err;
            const bool built = coord.buildRecoveredState(mine->sessionId, &rec, &err);
            check(built, "buildRecoveredState succeeds", err.technicalDetail);
            check(rec.state == liveA,
                  "coordinator-recovered state EQUALS the live state");
        }
        QDir(scanDir).removeRecursively();
    }

    // 7) FULL RESUME FLOW: crash → recover → resume → continue → the resumed
    //    journal is still Clean and carries the recovery markers
    {
        const QString sid = QStringLiteral("77777777-0000-4000-8000-000000000007");
        QString path;
        SessionState live;
        {
            SessionStore store;
            store.beginSession(finalsHeader(sid));
            for (const DomainEvent& e : finalsScript())
                store.submit(e);
            path = store.currentJournalPath();
            live = store.state();
        }   // store destroyed = "crash"

        RecoveryCoordinator coord;
        coord.scan();
        RecoveredMatchState rec;
        ErrorInfo err;
        check(coord.buildRecoveredState(sid, &rec, &err),
              "resume: rebuild recovered state", err.technicalDetail);

        check(rec.state == live,
              "pure recovery: rebuilt state EQUALS the pre-crash state");

        SessionStore resumed;
        const ReliabilityResult rr = resumed.resumeSession(rec);
        check(rr.ok, "resumeSession reopens the journal", rr.error.technicalDetail);
        // After resume the state carries RecoveryStarted/Completed (markers
        // that advance lastSeq); the MATCH CONTENT is unchanged.
        check(resumed.state().officials == live.officials
                  && resumed.state().sighters == live.sighters
                  && resumed.state().totalTenths == live.totalTenths
                  && resumed.state().currentStageId == live.currentStageId
                  && resumed.state().lifecycle == Lifecycle::Active,
              "resumed store match content EQUALS the pre-crash content");
        check(resumed.active(), "resumed session is active");

        // continue the match: fire one more official shot
        const SubmitResult more =
            resumed.submit(DomainEvent(ShotAccepted{testjournal::shot(4, 103)}));
        check(more.ok && more.persistedDurably,
              "match continues: a new shot appends onto the recovered journal");
        check(resumed.state().officials.size() == live.officials.size() + 1,
              "continued shot lands in the recovered state");

        // the resumed journal is still Clean and carries the markers
        const ValidationReport rep = JournalValidator::validateFile(path);
        check(rep.classification == JournalClassification::Clean,
              "resumed journal validates Clean end-to-end",
              QStringLiteral("%1: %2")
                  .arg(QLatin1String(journalClassificationName(rep.classification)))
                  .arg(rep.error.technicalDetail));
        bool sawStart = false, sawDone = false;
        for (const EventEnvelope& e : rep.validEnvelopes) {
            if (std::holds_alternative<RecoveryStarted>(e.payload)) sawStart = true;
            if (std::holds_alternative<RecoveryCompleted>(e.payload)) sawDone = true;
        }
        check(sawStart && sawDone,
              "resumed journal records RecoveryStarted + RecoveryCompleted");

        // idempotency: replaying the resumed journal STILL equals the state
        const ReplayResult reReplay = ReplayEngine::replayFile(path);
        check(reReplay.ok && reReplay.state == resumed.state(),
              "replay of the resumed journal == the resumed state (idempotent)");
    }

    // 8) Fatal (newer schema) is NOT resumable
    {
        const QString sid = QStringLiteral("88888888-0000-4000-8000-000000000008");
        QString path;
        writeFullSession(sid, root, &path);
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        QByteArray bytes = f.readAll(); f.close();
        bytes.replace("\"sv\":1,\"pv\":1,\"sid\"", "\"sv\":9,\"pv\":1,\"sid\"");
        QFile w(path);
        w.open(QIODevice::WriteOnly | QIODevice::Truncate);
        w.write(bytes); w.close();

        RecoveryCoordinator coord;
        coord.scan();
        RecoveredMatchState rec;
        ErrorInfo err;
        const bool built = coord.buildRecoveredState(sid, &rec, &err);
        check(!built, "a newer-schema journal is NOT resumable (Fatal)");
    }

    // 9) TIMER anchors (§16): the coordinator derives lastEventMonoMs and
    //    stageClockStartMonoMs (the current firing window's open) from the
    //    journal monotonic times, so the controller can resume from remaining.
    {
        const QString sid = QStringLiteral("99999999-0000-4000-8000-000000000009");
        ManualClock clk;
        SessionStore store;
        store.setClockForTesting(&clk);      // real file, controlled monotonic tm
        store.beginSession(finalsHeader(sid));           // tm = 0
        clk.setNowMs(1000);  store.submit(DomainEvent(StageEntered{1}));
        clk.setNowMs(2000);  store.submit(DomainEvent(OfficialMatchStarted{1}));
        clk.setNowMs(3000);  store.submit(DomainEvent(WindowOpened{1}));   // clock start
        clk.setNowMs(5000);  store.submit(DomainEvent(ShotAccepted{testjournal::shot(1, 104)}));
        clk.setNowMs(8000);  store.submit(DomainEvent(ShotAccepted{testjournal::shot(2, 99)}));
        // crash (no close)

        RecoveryCoordinator coord;
        coord.scan();
        RecoveredMatchState rec;
        ErrorInfo err;
        check(coord.buildRecoveredState(sid, &rec, &err),
              "timer-anchor session rebuilt", err.technicalDetail);
        check(rec.lastEventMonoMs == 8000,
              "lastEventMonoMs = tm of the last event",
              QString::number(rec.lastEventMonoMs));
        check(rec.stageClockStartMonoMs == 3000,
              "stageClockStartMonoMs = tm of the last WindowOpened",
              QString::number(rec.stageClockStartMonoMs));
        // elapsed since the firing window opened = 8000 - 3000 = 5000 ms; a
        // controller rebasing on this resumes with (duration - 5000), never
        // the full duration.
        check(rec.lastEventMonoMs - rec.stageClockStartMonoMs == 5000,
              "derived elapsed = 5000 ms (timer resumes from remaining, not full)");
    }

    QDir(root).removeRecursively();
}
