// M1 — reducer tests (step 14): legal/illegal transitions, duplicate shot
// identity, totals derived from records, sighters excluded, corrections,
// position changes, timers, completion, determinism, purity.

#include "reliability/reducer/SessionReducer.h"
#include "test_support.h"

using namespace ta::rel;

namespace {

EventEnvelope env(quint64 seq, const DomainEvent& payload, qint64 tm = 0)
{
    EventEnvelope e;
    e.payloadVersion = eventPayloadVersion(payload);
    e.sessionId = QString::fromLatin1(testjournal::kSid);
    e.lane = QStringLiteral("L1");
    e.seq = seq;
    e.wallTimestampIso = QString::fromLatin1(testjournal::kWall);
    e.monotonicMs = tm;
    e.eventType = QString::fromLatin1(eventTypeId(payload));
    e.payload = payload;
    e.previousHash = QByteArray(32, '0');
    e.currentHash = QByteArray(32, '0');
    return e;
}

// Apply a script from scratch; every step must succeed.
SessionState fold(const QVector<DomainEvent>& events, quint64 startSeq = 0)
{
    SessionState s;
    quint64 seq = startSeq;
    for (const DomainEvent& e : events) {
        const ReduceResult r = SessionReducer::apply(s, env(seq, e));
        check(r.ok, "fold step applies", r.error.technicalDetail);
        if (!r.ok)
            break;
        s = r.state;
        ++seq;
    }
    return s;
}

const QVector<DomainEvent> kBaseline = {
    DomainEvent(testjournal::sessionStarted(Discipline::ThreePositions50m,
                                            QStringLiteral("120"),
                                            QStringLiteral("Athlete"))),
    DomainEvent(PreparationStarted{0}),
    DomainEvent(SightingStarted{0}),
    DomainEvent(SighterAccepted{testjournal::shot(0, 95)}),
    DomainEvent(OfficialMatchStarted{1}),
    DomainEvent(ShotAccepted{testjournal::shot(1, 104)}),
    DomainEvent(ShotAccepted{testjournal::shot(2, 98)}),
};

} // namespace

void run_reducer_tests()
{
    std::printf("--- reducer tests ---\n");

    // legal flow: state fields after the baseline script
    {
        const SessionState s = fold(kBaseline);
        check(s.started && s.lifecycle == Lifecycle::Active
                  && s.phase == MatchPhase::OfficialMatch,
              "baseline lifecycle/phase correct");
        check(s.athlete == QLatin1String("Athlete")
                  && s.discipline == Discipline::ThreePositions50m,
              "identity fields applied from the header");
        check(s.officials.size() == 2 && s.sighters.size() == 1,
              "records appended by category");
        check(s.totalTenths == 202,
              "total = sum of official tenths (104 + 98)",
              QString::number(s.totalTenths));
        check(s.stageSubtotalTenths.value(1) == 202,
              "stage subtotal derived from records");
        check(s.lastSeq == 6, "lastSeq tracks the applied envelope");
        check(SessionReducer::checkInvariants(s).ok, "invariants hold");
        check(std::holds_alternative<QualificationState>(s.disc),
              "3P qualification selects QualificationState");
    }

    // sighters are excluded from official totals
    {
        QVector<DomainEvent> script = kBaseline;
        script.append(DomainEvent(SighterAccepted{testjournal::shot(0, 109)}));
        const SessionState s = fold(script);
        check(s.totalTenths == 202 && s.sighters.size() == 2,
              "sighter does not change the official total");
    }

    // determinism: same input, same output
    {
        const SessionState a = fold(kBaseline);
        const SessionState b = fold(kBaseline);
        check(a == b, "reducer is deterministic (state equality)");
        check(serializeSessionState(a) == serializeSessionState(b),
              "reducer is deterministic (byte equality)");
    }

    // illegal transitions
    {
        SessionState fresh;
        const ReduceResult beforeStart = SessionReducer::apply(
            fresh, env(0, DomainEvent(SightingStarted{0})));
        check(!beforeStart.ok, "events before SessionStarted rejected");

        const SessionState s = fold(kBaseline);
        const ReduceResult restart = SessionReducer::apply(
            s, env(7, DomainEvent(testjournal::sessionStarted(
                          Discipline::Prone50m, QStringLiteral("60"),
                          QStringLiteral("B")))));
        check(!restart.ok
                  && restart.error.code == ReliabilityError::InvalidStateTransition,
              "second SessionStarted rejected");

        // shot outside the official phase
        const SessionState sighting = fold({kBaseline[0], kBaseline[1]});
        const ReduceResult early = SessionReducer::apply(
            sighting, env(2, DomainEvent(ShotAccepted{testjournal::shot(1, 100)})));
        check(!early.ok
                  && early.error.code == ReliabilityError::InvalidStateTransition,
              "official shot outside OfficialMatch phase rejected");

        // resume without suspend
        const ReduceResult resume = SessionReducer::apply(
            s, env(7, DomainEvent(SessionResumed{})));
        check(!resume.ok, "SessionResumed without suspension rejected");

        // rejected events leave the input state untouched
        check(restart.state == s, "rejected event returns the unchanged state");
    }

    // sequence defence layer
    {
        const SessionState s = fold(kBaseline);
        const ReduceResult gap = SessionReducer::apply(
            s, env(9, DomainEvent(SighterAccepted{testjournal::shot(0, 90)})));
        check(!gap.ok && gap.error.code == ReliabilityError::InvalidSequence,
              "sequence gap rejected by the reducer defence layer");
    }

    // duplicate shot identity
    {
        const SessionState s = fold(kBaseline);
        ShotCore dupNumber = testjournal::shot(2, 101);
        dupNumber.externalId = 5555;
        const ReduceResult r1 = SessionReducer::apply(
            s, env(7, DomainEvent(ShotAccepted{dupNumber})));
        check(!r1.ok && r1.error.code == ReliabilityError::ReducerRejected,
              "duplicate shotNumber rejected");
        ShotCore dupExternal = testjournal::shot(3, 101);
        dupExternal.externalId = 1001;   // shot 1's hardware id
        const ReduceResult r2 = SessionReducer::apply(
            s, env(7, DomainEvent(ShotAccepted{dupExternal})));
        check(!r2.ok && r2.error.code == ReliabilityError::ReducerRejected,
              "duplicate externalId rejected");
    }

    // position changes
    {
        QVector<DomainEvent> script = kBaseline;
        script.append(DomainEvent(PositionChanged{1}));
        const SessionState s = fold(script);
        check(s.positionIndex == 1, "positionIndex updated");
        const auto* q = std::get_if<QualificationState>(&s.disc);
        check(q && q->positionIndex == 1,
              "discipline state mirrors the position");
    }

    // timers: start/pause/resume/expire + illegal orders
    {
        QVector<DomainEvent> script = kBaseline;
        script.append(DomainEvent(TimerStarted{TimerId::Match, 900000}));
        script.append(DomainEvent(TimerPaused{TimerId::Match, 5000}));
        script.append(DomainEvent(TimerResumed{TimerId::Match, 9000}));
        const SessionState s = fold(script);
        check(s.timer.active && !s.timer.paused
                  && s.timer.pausedAccumMs == 4000,
              "pause bookkeeping exact (9000-5000)",
              QString::number(s.timer.pausedAccumMs));
        const ReduceResult resumeAgain = SessionReducer::apply(
            s, env(10, DomainEvent(TimerResumed{TimerId::Match, 9500})));
        check(!resumeAgain.ok, "resume of a running timer rejected");
        QVector<DomainEvent> expireScript = script;
        expireScript.append(DomainEvent(TimerExpired{TimerId::Match}));
        const SessionState se = fold(expireScript);
        check(!se.timer.active, "expiry deactivates the timer");
        const ReduceResult pauseDead = SessionReducer::apply(
            se, env(11, DomainEvent(TimerPaused{TimerId::Match, 12000})));
        check(!pauseDead.ok, "pause of an inactive timer rejected");
    }

    // corrections: rescore, invalidate, series adjust, penalty
    {
        QVector<DomainEvent> script = kBaseline;   // shots at seq 5 and 6
        script.append(DomainEvent(
            ShotRescored{5, 105, false, 0, 0, QStringLiteral("jury 7.11.2"),
                         Authority::Jury}));
        SessionState s = fold(script);
        check(s.totalTenths == 203 && s.corrections.size() == 1
                  && s.corrections[0].fromTenths == 104
                  && s.corrections[0].toTenths == 105,
              "rescore adjusts totals and records the correction",
              QString::number(s.totalTenths));
        check(s.officials[0].rescoredTenths == 105
                  && s.officials[0].shot.scoreTenths == 104,
              "original score retained beside the rescore");

        const ReduceResult inval = SessionReducer::apply(
            s, env(8, DomainEvent(ShotInvalidated{6, QStringLiteral("cross"),
                                                  Authority::Jury})));
        check(inval.ok && inval.state.totalTenths == 105
                  && inval.state.officials[1].invalidated,
              "invalidation zeroes the shot in totals, keeps the record",
              QString::number(inval.state.totalTenths));
        const ReduceResult invalAgain = SessionReducer::apply(
            inval.state, env(9, DomainEvent(ShotInvalidated{
                                  6, QStringLiteral("again"), Authority::Jury})));
        check(!invalAgain.ok, "double invalidation rejected");
        const ReduceResult missing = SessionReducer::apply(
            s, env(8, DomainEvent(ShotInvalidated{99, QStringLiteral("x"),
                                                  Authority::Jury})));
        check(!missing.ok, "correction of a non-existent shot rejected");

        const ReduceResult adj = SessionReducer::apply(
            inval.state, env(9, DomainEvent(SeriesAdjusted{
                                  1, -10, QStringLiteral("jury"),
                                  Authority::Jury})));
        check(adj.ok && adj.state.totalTenths == 95
                  && adj.state.stageSubtotalTenths.value(1) == 95,
              "series adjustment applied to stage and total");
        const ReduceResult pen = SessionReducer::apply(
            adj.state, env(10, DomainEvent(PenaltyIssued{
                                 -20, QStringLiteral("7.4.5"),
                                 Authority::Jury})));
        check(pen.ok && pen.state.totalTenths == 75
                  && pen.state.stageSubtotalTenths.value(1) == 95,
              "penalty applies to the match total only");
        check(SessionReducer::checkInvariants(pen.state).ok,
              "invariants hold after corrections");
    }

    // cross shots and incidents do not score
    {
        QVector<DomainEvent> script = kBaseline;
        script.append(DomainEvent(CrossShotRecorded{
            testjournal::shot(0, 103), QStringLiteral("L3")}));
        script.append(DomainEvent(EquipmentMalfunctionRecorded{
            QStringLiteral("sling broke"), 900000}));
        const SessionState s = fold(script);
        check(s.crossShots.size() == 1 && s.incidents.size() == 1
                  && s.totalTenths == 202,
              "cross shot and incident recorded without scoring");
    }

    // completion: totals must match, then lifecycle transitions
    {
        const SessionState s = fold(kBaseline);
        MatchCompleted wrong;
        wrong.totalTenths = 999;
        wrong.officialCount = 2;
        const ReduceResult bad = SessionReducer::apply(
            s, env(7, DomainEvent(wrong)));
        check(!bad.ok && bad.error.code == ReliabilityError::ReducerRejected,
              "MatchCompleted with wrong totals rejected");
        MatchCompleted right;
        right.totalTenths = 202;
        right.officialCount = 2;
        const ReduceResult done = SessionReducer::apply(
            s, env(7, DomainEvent(right)));
        check(done.ok && done.state.lifecycle == Lifecycle::Complete,
              "MatchCompleted with matching totals completes the session");
        const ReduceResult after = SessionReducer::apply(
            done.state,
            env(8, DomainEvent(ShotAccepted{testjournal::shot(3, 100)})));
        check(!after.ok, "shots after completion rejected");
        const ReduceResult closed = SessionReducer::apply(
            done.state, env(8, DomainEvent(SessionClosed{CloseReason::Clean})));
        check(closed.ok && closed.state.lifecycle == Lifecycle::Closed,
              "SessionClosed transitions Complete -> Closed");
    }

    // suspend/resume + recovery markers
    {
        const SessionState s = fold(kBaseline);
        const ReduceResult susp = SessionReducer::apply(
            s, env(7, DomainEvent(SessionSuspended{SuspendReason::AppSuspend})));
        check(susp.ok && susp.state.lifecycle == Lifecycle::Suspended,
              "suspension recorded");
        const ReduceResult shotWhileSuspended = SessionReducer::apply(
            susp.state,
            env(8, DomainEvent(ShotAccepted{testjournal::shot(3, 100)})));
        check(!shotWhileSuspended.ok, "shots while suspended rejected");
        const ReduceResult res = SessionReducer::apply(
            susp.state, env(8, DomainEvent(SessionResumed{})));
        check(res.ok && res.state.lifecycle == Lifecycle::Active,
              "resume restores Active");
        const ReduceResult rcs = SessionReducer::apply(
            res.state, env(9, DomainEvent(RecoveryStarted{9})));
        check(rcs.ok && rcs.state.lastSeq == 9,
              "RecoveryStarted is a marker that only advances lastSeq");
    }
}
