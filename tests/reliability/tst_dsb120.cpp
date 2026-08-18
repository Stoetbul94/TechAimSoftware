// DSB-120-001 — the gated independent-position-clock sequencer (DSB 1.20).
//
// These checks are written against the RULE, not against the implementation:
// three separate clocks, a gate no completion can open, sighting that costs
// competition time in prone and standing but not in kneeling, and a sighting
// door that closes for good on a position's first match shot.
//
// Every negative case below is one an athlete could be cheated by. They are
// asserted as refusals from the ENGINE, not as absent buttons.

#include "dsb/Dsb120Controller.h"
#include "reliability/replay/ReplayEngine.h"
#include "reliability/journal/JournalValidator.h"
#include "test_support.h"

#include <QVariantMap>

using namespace ta::rel;
using ta::dsb::Phase;

namespace {

QVariantMap authority(const char* programmeId, const char* variant, int shots,
                      const char* durations)
{
    QVariantMap m;
    m[QStringLiteral("programmeId")] = QString::fromLatin1(programmeId);
    m[QStringLiteral("rulesetId")] = QStringLiteral("dsb");
    m[QStringLiteral("rulesetVersion")] = QStringLiteral("2026-01-01");
    m[QStringLiteral("ruleNumber")] = QStringLiteral("1.20");
    m[QStringLiteral("programmeVariant")] = QString::fromLatin1(variant);
    m[QStringLiteral("competitionContext")] = QStringLiteral("DM_2026");
    m[QStringLiteral("scoringMode")] = QStringLiteral("INTEGER");
    m[QStringLiteral("timingModel")] = QStringLiteral("INDEPENDENT_POSITION_CLOCKS");
    m[QStringLiteral("targetStandardId")] = QStringLiteral("issf.10m.air-rifle");
    m[QStringLiteral("disciplineId")] = QStringLiteral("AR10_3P");
    m[QStringLiteral("distanceM")] = 10;
    m[QStringLiteral("preparationMs")] = 900000;
    m[QStringLiteral("matchMs")] = 0;
    m[QStringLiteral("shotCount")] = shots;
    m[QStringLiteral("positionSequence")] = QStringLiteral("KNEELING,PRONE,STANDING");
    m[QStringLiteral("positionDurationsMs")] = QString::fromLatin1(durations);
    return m;
}

QVariantMap a3x10() { return authority("dsb.10m.air-rifle.3x10", "3x10", 30,
                                       "1500000,1200000,1800000"); }
QVariantMap a3x20() { return authority("dsb.10m.air-rifle.3x20", "3x20", 60,
                                       "2100000,1800000,2400000"); }

struct Rig {
    MemoryJournalFile file;
    ManualClock clock;
    Dsb120Controller c;
    Rig()
    {
        c.storeForTesting()->setClockForTesting(&clock);
        c.storeForTesting()->setJournalFileForTesting(&file);
    }
    // Fire `n` accepted shots, advancing the clock between them.
    int fire(int n, qint64 eachMs = 1000, qint64 idBase = 1000)
    {
        int ok = 0;
        for (int i = 0; i < n; ++i) {
            clock.advance(eachMs);
            if (c.submitShot(0.5, -0.5, 10.0, idBase + i, 0, true)) ++ok;
        }
        return ok;
    }
};

SessionState replayOf(const MemoryJournalFile& f)
{
    const ValidationReport rep = JournalValidator::validateBytes(f.data);
    return ReplayEngine::replay(rep.validEnvelopes).state;
}

const Dsb120State* discOf(const SessionState& s)
{
    return std::get_if<Dsb120State>(&s.disc);
}

} // namespace

void run_dsb120_tests()
{
    std::printf("--- DSB 1.20 gated position sequencer (DSB-120-001) ---\n");

    // ── the two courses are what the rule says ───────────────────────────
    {
        Rig r;
        check(r.c.startSession(a3x10(), QStringLiteral("A")),
              "DSB-120-001: 3x10 starts");
        check(r.c.shotsPerPosition() == 10 && r.c.totalShotsRequired() == 30,
              "DSB-120-001: 3x10 is 10 match shots per position, 30 in total");
        Rig r2;
        r2.c.startSession(a3x20(), QStringLiteral("A"));
        check(r2.c.shotsPerPosition() == 20 && r2.c.totalShotsRequired() == 60,
              "DSB-120-001: 3x20 is 20 match shots per position, 60 in total");
    }

    // A programme that does not declare independent clocks is REFUSED. The
    // sequencer must not activate merely because a course has three positions:
    // DSB 1.40 and 1.60 also do, on a single master clock.
    {
        Rig r;
        QVariantMap single = a3x20();
        single[QStringLiteral("timingModel")] = QStringLiteral("SINGLE_MATCH_CLOCK");
        single[QStringLiteral("positionDurationsMs")] = QString();
        check(!r.c.startSession(single, QStringLiteral("A")),
              "DSB-120-001: a SINGLE_MATCH_CLOCK three-position course (1.40 / "
              "1.60) is refused by this sequencer - three positions is not what "
              "activates it");
    }

    // ── preparation, then a GATE - never a kneeling clock ────────────────
    {
        Rig r;
        r.c.startSession(a3x20(), QStringLiteral("A"));
        check(r.c.phaseId() == int(Phase::Idle), "DSB-120-001: a fresh session is idle");
        check(r.c.startPreparation(), "DSB-120-001: preparation starts");
        check(r.c.phaseId() == int(Phase::WaitingStart)
              && r.c.nextPositionIndex() == 0,
              "DSB-120-001: preparation ends AT THE GATE with kneeling armed - "
              "the kneeling clock does not start with it",
              QString::number(r.c.phaseId()));

        // The 15 minutes are the preparation's own, and they are not kneeling's.
        r.clock.advance(120000);
        check(r.c.startPosition(1) == false,
              "DSB-120-001: only the ARMED position may start - prone cannot "
              "jump the queue");
        check(r.c.startPosition(0), "DSB-120-001: authorised START_KNEELING");
        check(r.c.remainingMs() == 2100000,
              "DSB-120-001: kneeling starts on its OWN 35 minutes, undiminished "
              "by the preparation period that preceded it",
              QString::number(r.c.remainingMs()));
        check(r.c.phaseId() == int(Phase::PositionMatch),
              "DSB-120-001: kneeling opens IN MATCH - its sighting was the "
              "shared preparation period, not a second sighting inside its clock");
    }

    // ── independent clocks, and NO automatic chaining ────────────────────
    {
        Rig r;
        r.c.startSession(a3x10(), QStringLiteral("A"));
        r.c.startPreparation();
        r.c.startPosition(0);
        check(r.c.remainingMs() == 1500000,
              "DSB-120-001: 3x10 kneeling is 25 minutes");

        check(r.fire(10) == 10, "DSB-120-001: ten kneeling match shots accepted");
        check(r.c.matchShotsIn(0) == 10 && r.c.totalMatchShots() == 10,
              "DSB-120-001: they count in kneeling and in the total");
        check(!r.c.submitShot(0, 0, 10.0, 5555, 0, true),
              "DSB-120-001: an eleventh kneeling match shot is refused - the "
              "position's course is complete");

        check(r.c.endPosition(), "DSB-120-001: kneeling ends");
        check(r.c.phaseId() == int(Phase::WaitingStart)
              && r.c.nextPositionIndex() == 1,
              "DSB-120-001: completing kneeling ARMS prone - it does not start it");
        check(r.c.remainingMs() == -1,
              "DSB-120-001: NOTHING is running at the gate; there is no next "
              "clock quietly counting down",
              QString::number(r.c.remainingMs()));

        // The gate is real: time passing does not open it.
        r.clock.advance(600000);
        check(r.c.phaseId() == int(Phase::WaitingStart) && r.c.positionIndex() == 0,
              "DSB-120-001: ten minutes at the gate changes nothing - only an "
              "authorised start does");
        check(!r.c.submitShot(0, 0, 10.0, 6001, 0, true),
              "DSB-120-001: no shot is accepted at the gate, of any kind");

        check(r.c.startPosition(1), "DSB-120-001: authorised START_PRONE");
        check(r.c.remainingMs() == 1200000 && r.c.phaseId() == int(Phase::PositionSighting),
              "DSB-120-001: prone starts on its own 20 minutes, IN SIGHTING, "
              "with the clock already running",
              QString::number(r.c.remainingMs()));
    }

    // ── prone sighters cost prone time, and MATCH does not reset it ──────
    {
        Rig r;
        r.c.startSession(a3x20(), QStringLiteral("A"));
        r.c.startPreparation();
        r.c.startPosition(0);
        r.fire(20);
        r.c.endPosition();
        r.c.startPosition(1);
        check(r.c.remainingMs() == 1800000, "DSB-120-001: prone opens on 30 minutes");

        // 2:18 of sighting, exactly the worked example in the rule brief.
        r.clock.advance(138000);
        check(r.c.submitShot(0, 0, 10.0, 7001, 0, true),
              "DSB-120-001: a prone sighter is accepted");
        check(r.c.matchShotsIn(1) == 0,
              "DSB-120-001: a prone sighter does NOT count as a match shot");
        check(r.c.remainingMs() <= 1800000 - 138000,
              "DSB-120-001: prone sighting CONSUMES prone competition time",
              QString::number(r.c.remainingMs()));

        const qint64 before = r.c.remainingMs();
        check(r.c.enterMatchPhase(), "DSB-120-001: prone moves to match");
        check(r.c.remainingMs() == before,
              "DSB-120-001: SIGHTING -> MATCH does not reset the position clock "
              "- roughly 27:42 remains, not 30:00",
              QString::number(r.c.remainingMs() / 1000));
        check(r.c.remainingMs() < 1800000 - 130000 && r.c.remainingMs() > 1800000 - 145000,
              "DSB-120-001: the remaining prone time is the rule's 27:42, "
              "give or take the shot interval",
              QString::number(r.c.remainingMs() / 1000));

        // ── the sighting lock ────────────────────────────────────────────
        check(r.c.enterMatchPhase() == false,
              "DSB-120-001: re-entering the match phase is refused");
        r.fire(1, 1000, 7100);
        check(r.c.matchShotsIn(1) == 1, "DSB-120-001: the first prone match shot counts");
        check(r.c.enterMatchPhase() == false,
              "DSB-120-001: after the first prone match shot the ENGINE refuses "
              "a return to sighting - not merely a hidden button");

        // ── standing behaves the same ────────────────────────────────────
        r.fire(19, 1000, 7200);
        check(r.c.matchShotsIn(1) == 20, "DSB-120-001: prone completes at 20");
        r.c.endPosition();
        check(r.c.nextPositionIndex() == 2 && r.c.remainingMs() == -1,
              "DSB-120-001: prone completion arms standing without starting it");
        r.c.startPosition(2);
        check(r.c.remainingMs() == 2400000 && r.c.phaseId() == int(Phase::PositionSighting),
              "DSB-120-001: standing opens on its own 40 minutes, in sighting");
        r.clock.advance(60000);
        r.c.submitShot(0, 0, 10.0, 8001, 0, true);
        check(r.c.remainingMs() <= 2400000 - 60000,
              "DSB-120-001: standing sighting consumes standing time");
        const qint64 standingBefore = r.c.remainingMs();
        r.c.enterMatchPhase();
        check(r.c.remainingMs() == standingBefore,
              "DSB-120-001: standing SIGHTING -> MATCH does not reset the clock");
        r.fire(1, 1000, 8100);
        check(r.c.enterMatchPhase() == false,
              "DSB-120-001: the standing sighting lock closes on its first match shot");

        r.fire(19, 1000, 8200);
        check(r.c.matchShotsIn(0) == 20 && r.c.matchShotsIn(1) == 20
              && r.c.matchShotsIn(2) == 20 && r.c.totalMatchShots() == 60,
              "DSB-120-001: 20 / 20 / 20 and 60 in total, each position's shots "
              "kept as its own group");
        check(r.c.endPosition() && r.c.phaseId() == int(Phase::Finished),
              "DSB-120-001: ending standing finishes the match");
    }

    // ── the match record keeps its position identity ─────────────────────
    {
        Rig r;
        r.c.startSession(a3x10(), QStringLiteral("A"));
        r.c.startPreparation();
        r.c.startPosition(0);
        r.fire(10, 1000, 100);
        r.c.endPosition();
        r.c.startPosition(1);
        r.c.enterMatchPhase();
        r.fire(10, 1000, 200);
        check(r.c.positionSubtotalTenths(0) == 1000
              && r.c.positionSubtotalTenths(1) == 1000
              && r.c.positionSubtotalTenths(2) == 0,
              "DSB-120-001: subtotals stay per position, which is what a DSB "
              "report will need",
              QString::number(r.c.positionSubtotalTenths(0)));

        const SessionState s = replayOf(r.file);
        check(s.ruleAuthority.ruleNumber == QLatin1String("1.20")
              && s.ruleAuthority.scoringMode == QLatin1String("INTEGER")
              && s.ruleAuthority.targetStandardId == QLatin1String("issf.10m.air-rifle"),
              "DSB-120-001: the session records rule 1.20, INTEGER scoring and "
              "the EXISTING 10 m air rifle target standard - no new geometry");
        check(s.ruleAuthority.matchMs == 0,
              "DSB-120-001: there is NO master duration on disk - a 75 or 105 "
              "minute clock cannot be reconstructed from this session");
        check(s.config.matchMs == 0,
              "DSB-120-001: and the session config declares no master clock either");
    }

    // ── recovery, in every state a restart can land in ───────────────────
    // A..J from the task brief. D and E are ONE durable state by design: no
    // authorised action separates "kneeling just completed" from "waiting for
    // prone", so persisting them apart would invent a step the rule does not
    // have. Both are exercised; both restore to the gate with prone armed.
    {
        struct Case { const char* name; int phase; int position; int next; int matchShots; };

        // (A) preparation
        {
            Rig r;
            r.c.startSession(a3x20(), QStringLiteral("A"));
            r.c.startPreparation();
            r.clock.advance(379000);           // 08:41 remaining of 15:00
            const SessionState s = replayOf(r.file);
            const Dsb120State* d = discOf(s);
            check(d && d->phase == quint8(Phase::WaitingStart) && d->nextPositionIndex == 0,
                  "DSB-120-001 recovery A: a restart during preparation comes "
                  "back at the gate with kneeling armed, not mid-clock");
        }
        // (C) kneeling match, part-way
        {
            Rig r;
            r.c.startSession(a3x20(), QStringLiteral("A"));
            r.c.startPreparation();
            r.c.startPosition(0);
            r.fire(7, 60000, 300);             // 7 of 20, seven minutes gone
            const SessionState s = replayOf(r.file);
            const Dsb120State* d = discOf(s);
            int kneeling = 0;
            for (const StateShotRecord& rec : s.officials)
                if (rec.shot.stageId == 1) ++kneeling;
            check(d && d->phase == quint8(Phase::PositionMatch) && d->positionIndex == 0,
                  "DSB-120-001 recovery C: kneeling match is restored as kneeling "
                  "match");
            check(kneeling == 7,
                  "DSB-120-001 recovery C: 7 of 20 kneeling shots, still kneeling's",
                  QString::number(kneeling));
            const qint64 remaining =
                s.timer.durationMs - (420000);   // seven minutes of shots
            check(s.timer.durationMs == 2100000 && remaining == 1680000,
                  "DSB-120-001 recovery C: the restored clock is the KNEELING "
                  "clock at its frozen remaining, never a fresh 35 minutes");
        }
        // (D/E) the gate after kneeling
        {
            Rig r;
            r.c.startSession(a3x10(), QStringLiteral("A"));
            r.c.startPreparation();
            r.c.startPosition(0);
            r.fire(10, 1000, 400);
            r.c.endPosition();
            const SessionState s = replayOf(r.file);
            const Dsb120State* d = discOf(s);
            check(d && d->phase == quint8(Phase::WaitingStart)
                  && d->nextPositionIndex == 1 && d->completedPositions == 1,
                  "DSB-120-001 recovery D/E: a restart at the position change "
                  "comes back AT THE GATE - prone armed, nothing running");
        }
        // (F) prone sighting, clock already running
        {
            Rig r;
            r.c.startSession(a3x20(), QStringLiteral("A"));
            r.c.startPreparation();
            r.c.startPosition(0);
            r.fire(20, 1000, 500);
            r.c.endPosition();
            r.c.startPosition(1);
            r.clock.advance(138000);
            r.c.submitShot(0, 0, 10.0, 600, 0, true);
            const SessionState s = replayOf(r.file);
            const Dsb120State* d = discOf(s);
            check(d && d->phase == quint8(Phase::PositionSighting) && d->positionIndex == 1,
                  "DSB-120-001 recovery F: prone SIGHTING is restored as prone "
                  "sighting - the sighter is not promoted to a match shot");
            check(s.sighters.size() == 1 && s.officials.size() == 20,
                  "DSB-120-001 recovery F: sighters stay separate from match shots",
                  QString::number(s.officials.size()));
            check(s.timer.durationMs == 1800000,
                  "DSB-120-001 recovery F: the restored clock is prone's 30 minutes");
        }
        // (G) prone match, part-way, and (I/J) standing
        {
            Rig r;
            r.c.startSession(a3x20(), QStringLiteral("A"));
            r.c.startPreparation();
            r.c.startPosition(0);
            r.fire(20, 1000, 700);
            r.c.endPosition();
            r.c.startPosition(1);
            r.c.enterMatchPhase();
            r.fire(7, 1000, 800);
            SessionState s = replayOf(r.file);
            const Dsb120State* d = discOf(s);
            int prone = 0;
            for (const StateShotRecord& rec : s.officials)
                if (rec.shot.stageId == 2) ++prone;
            check(d && d->phase == quint8(Phase::PositionMatch) && d->positionIndex == 1
                  && prone == 7,
                  "DSB-120-001 recovery G: prone match at 7 of 20 restores as "
                  "prone match at 7 of 20", QString::number(prone));

            r.fire(13, 1000, 900);
            r.c.endPosition();
            r.c.startPosition(2);
            s = replayOf(r.file);
            d = discOf(s);
            check(d && d->phase == quint8(Phase::PositionSighting) && d->positionIndex == 2
                  && d->completedPositions == 2,
                  "DSB-120-001 recovery I: standing sighting restores as standing "
                  "sighting with two positions behind it");
            r.c.enterMatchPhase();
            r.fire(3, 1000, 950);
            s = replayOf(r.file);
            d = discOf(s);
            check(d && d->phase == quint8(Phase::PositionMatch) && d->positionIndex == 2
                  && d->sightingLocked == false,
                  "DSB-120-001 recovery J: standing match restores as standing "
                  "match");
            // The lock is DERIVED from the shots, so a restart cannot lose it.
            int standing = 0;
            for (const StateShotRecord& rec : s.officials)
                if (rec.shot.stageId == 3) ++standing;
            check(standing == 3,
                  "DSB-120-001 recovery J: and the standing match shots that "
                  "closed the sighting door are still there",
                  QString::number(standing));
        }
    }

    // ── NEGATIVE CONTROLS ────────────────────────────────────────────────
    // Each of these is a way the competition could be silently run wrong. They
    // fail as REFUSALS from the engine.
    {
        Rig r;
        r.c.startSession(a3x10(), QStringLiteral("A"));
        check(!r.c.startPosition(0),
              "DSB-120-001 control: a position cannot start before preparation "
              "has armed the gate");
        r.c.startPreparation();
        r.c.startPosition(0);
        check(!r.c.startPosition(1),
              "DSB-120-001 control: prone cannot start while kneeling runs - "
              "two clocks can never run at once");
        check(!r.c.enterMatchPhase(),
              "DSB-120-001 control: kneeling has no sighting phase to leave");
        r.fire(10);
        r.c.endPosition();
        check(!r.c.startPosition(2),
              "DSB-120-001 control: standing cannot be started out of order");
        check(!r.c.endPosition(),
              "DSB-120-001 control: a gate cannot be 'ended' into the next "
              "position - only an authorised start opens it");

        // A journal-level guarantee, not just a controller one: replaying a
        // hand-forged start that skips the gate is refused by the reducer.
        const SessionState s = replayOf(r.file);
        check(discOf(s) && discOf(s)->phase == quint8(Phase::WaitingStart),
              "DSB-120-001 control: the journal itself says WAITING, so even a "
              "replay cannot produce a running prone clock");
    }
}
