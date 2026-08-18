// QUAL-3P-001 — the 50 m three-position course, journalled and recovered.
//
// One engine, three courses: ISSF 3x20, DSB 1.40 3x20 and DSB 1.60 3x40 differ
// in how many shots a position takes and how long the master clock runs, and in
// nothing else. These checks are written so that a course arriving from the
// adopted definition is the ONLY thing that separates them - if the engine ever
// starts inferring the course from a shot total, 1.40 and 1.60 stop being
// distinguishable and these fail.
//
// The master clock is the sharp edge. It is anchored once and must survive two
// position changes; a re-anchor would hand the athlete a fresh 105 or 165
// minutes, which is why that has its own negative control.

#include "qualification/QualificationController.h"
#include "reliability/replay/ReplayEngine.h"
#include "reliability/journal/JournalValidator.h"
#include "test_support.h"

#include <QVariantMap>

using namespace ta::rel;

namespace {

QVariantMap authority(const char* programmeId, const char* rule, const char* variant,
                      qint64 matchMs, const char* perPosition)
{
    QVariantMap m;
    m[QStringLiteral("programmeId")] = QString::fromLatin1(programmeId);
    m[QStringLiteral("rulesetId")] = QStringLiteral("dsb");
    m[QStringLiteral("rulesetVersion")] = QStringLiteral("2026-01-01");
    m[QStringLiteral("ruleNumber")] = QString::fromLatin1(rule);
    m[QStringLiteral("programmeVariant")] = QString::fromLatin1(variant);
    m[QStringLiteral("competitionContext")] = QStringLiteral("DM_2026");
    m[QStringLiteral("scoringMode")] = QStringLiteral("INTEGER");
    m[QStringLiteral("timingModel")] = QStringLiteral("SINGLE_MATCH_CLOCK");
    m[QStringLiteral("targetStandardId")] = QStringLiteral("issf.50m.rifle");
    m[QStringLiteral("disciplineId")] = QStringLiteral("RIFLE50_3P");
    m[QStringLiteral("distanceM")] = 50;
    m[QStringLiteral("preparationMs")] = 900000;
    m[QStringLiteral("matchMs")] = matchMs;
    m[QStringLiteral("positionSequence")] = QStringLiteral("KNEELING,PRONE,STANDING");
    m[QStringLiteral("shotsPerPosition")] = QString::fromLatin1(perPosition);
    return m;
}

QVariantMap a140() { return authority("dsb.50m.rifle.3x20", "1.40", "3x20",
                                      6300000, "20,20,20"); }
QVariantMap a160() { return authority("dsb.50m.rifle.3x40", "1.60", "3x40",
                                      9900000, "40,40,40"); }

struct Rig {
    MemoryJournalFile file;
    ManualClock clock;
    QualificationController qc;
    int fired = 0;
    Rig()
    {
        qc.storeForTesting()->setClockForTesting(&clock);
        qc.storeForTesting()->setJournalFileForTesting(&file);
    }
    // Start a 3P course from an adopted definition (empty = the ISSF course,
    // which declares nothing).
    void start(const QVariantMap& a, int totalShots, qint64 matchMs)
    {
        if (!a.isEmpty())
            qc.adoptRuleAuthority(a);
        qc.startSession(QStringLiteral("3P50"), QStringLiteral("3P"),
                        QStringLiteral("A"), totalShots, matchMs, 900000, -1,
                        QString(), QString());
        qc.beginPreparation();
        clock.advance(60000);
        qc.beginSighting();
        clock.advance(60000);
        qc.beginOfficialMatch();          // the master clock is anchored HERE
    }
    int fire(int n, qint64 eachMs = 1000)
    {
        int ok = 0;
        for (int i = 0; i < n; ++i) {
            clock.advance(eachMs);
            if (qc.submitOfficial(0.4, -0.4, 10.0, 5000 + (++fired), 0, true)) ++ok;
        }
        return ok;
    }
    // The position change as the page performs it: record the position, then
    // sighting for the new one. Neither event touches the clock.
    void changePosition(int index)
    {
        qc.changePosition(index);
        qc.beginSighting();
        clock.advance(30000);
        qc.beginOfficialMatch();
    }
};

SessionState replayOf(const MemoryJournalFile& f)
{
    const ValidationReport rep = JournalValidator::validateBytes(f.data);
    return ReplayEngine::replay(rep.validEnvelopes).state;
}

// Remaining master time exactly as the recovery path computes it.
qint64 frozenRemaining(const MemoryJournalFile& f)
{
    const ValidationReport rep = JournalValidator::validateBytes(f.data);
    const ReplayResult rr = ReplayEngine::replay(rep.validEnvelopes);
    const qint64 last = rep.validEnvelopes.isEmpty()
        ? 0 : rep.validEnvelopes.last().monotonicMs;
    return rr.state.timer.durationMs - (last - rr.state.timer.startedAtMonoMs);
}

int positionShots(const SessionState& s, int position)
{
    int n = 0;
    for (const StateShotRecord& r : s.officials)
        if (r.shot.seriesIndex == position && !r.invalidated) ++n;
    return n;
}

} // namespace

void run_qual3p_tests()
{
    std::printf("--- 50 m three-position journal + recovery (QUAL-3P-001) ---\n");

    // ── DSB 1.40: 3x20 on a 105-minute master clock ──────────────────────
    {
        Rig r;
        r.start(a140(), 60, 6300000);
        check(r.fire(12) == 12, "QUAL-3P-001: 1.40 kneeling accepts 12 shots");

        SessionState s = replayOf(r.file);
        check(s.discipline == Discipline::ThreePositions50m,
              "QUAL-3P-001: the session records itself as a three-position course");
        check(s.ruleAuthority.ruleNumber == QLatin1String("1.40")
              && s.ruleAuthority.programmeVariant == QLatin1String("3x20")
              && s.ruleAuthority.shotsPerPosition == QLatin1String("20,20,20")
              && s.ruleAuthority.matchMs == 6300000
              && s.ruleAuthority.scoringMode == QLatin1String("INTEGER")
              && s.ruleAuthority.targetStandardId == QLatin1String("issf.50m.rifle"),
              "QUAL-3P-001 (B): 1.40 recovers as DSB 1.40, 3x20, integer, 50 m "
              "rifle, on a 105-minute clock", s.ruleAuthority.auditLine());
        check(s.positionIndex == 0 && positionShots(s, 0) == 12
              && s.officials.size() == 12,
              "QUAL-3P-001 (B): kneeling, 12 of 20, 12 of 60");
        check(s.config.perPositionShots == 20,
              "QUAL-3P-001: the COURSE is on disk - 20 per position",
              QString::number(s.config.perPositionShots));

        // (C)(D) into prone, and the clock must not restart.
        const qint64 beforeChange = frozenRemaining(r.file);
        r.fire(8);                       // complete kneeling
        r.changePosition(1);
        s = replayOf(r.file);
        check(s.positionIndex == 1 && positionShots(s, 0) == 20,
              "QUAL-3P-001 (C): prone, with kneeling's 20 shots kept as "
              "kneeling's");
        check(frozenRemaining(r.file) < beforeChange,
              "QUAL-3P-001 (C): the master clock kept running through the "
              "position change - it did not stop and it did not restart",
              QString::number(frozenRemaining(r.file)));
        check(frozenRemaining(r.file) < 6300000,
              "QUAL-3P-001 (C): and it is still BELOW the full 105 minutes");

        r.fire(11);
        s = replayOf(r.file);
        check(s.positionIndex == 1 && positionShots(s, 1) == 11
              && s.officials.size() == 31,
              "QUAL-3P-001 (D): prone 11 of 20, 31 of 60 overall",
              QString::number(s.officials.size()));

        // (E)(F) into standing.
        r.fire(9);
        r.changePosition(2);
        r.fire(12);
        s = replayOf(r.file);
        check(s.positionIndex == 2 && positionShots(s, 2) == 12
              && s.officials.size() == 52,
              "QUAL-3P-001 (F): standing 12 of 20, 52 of 60 overall");
        check(positionShots(s, 0) == 20 && positionShots(s, 1) == 20,
              "QUAL-3P-001: and the earlier positions keep their own 20s - the "
              "match is not a flat list of 52 shots");
    }

    // ── DSB 1.60: the SAME engine, told a different course ───────────────
    {
        Rig r;
        r.start(a160(), 120, 9900000);
        r.fire(25);
        SessionState s = replayOf(r.file);
        check(s.ruleAuthority.ruleNumber == QLatin1String("1.60")
              && s.ruleAuthority.shotsPerPosition == QLatin1String("40,40,40")
              && s.ruleAuthority.matchMs == 9900000
              && s.config.perPositionShots == 40
              && s.config.officialShots == 120,
              "QUAL-3P-001 (B): 1.60 recovers as 3x40 on 165 minutes - never as "
              "a 60-shot course", s.ruleAuthority.auditLine());
        check(s.positionIndex == 0 && positionShots(s, 0) == 25,
              "QUAL-3P-001 (B): kneeling 25 of 40");

        r.fire(15);
        r.changePosition(1);
        r.fire(1);
        s = replayOf(r.file);
        check(s.positionIndex == 1 && s.officials.size() == 41
              && positionShots(s, 1) == 1,
              "QUAL-3P-001 (C): the first prone shot is overall 41 and prone's "
              "first - the boundary follows 40/40/40");

        r.fire(24);
        s = replayOf(r.file);
        check(s.officials.size() == 65 && positionShots(s, 1) == 25,
              "QUAL-3P-001 (D): 65 of 120 overall, 25 of 40 in prone");

        r.fire(15);
        r.changePosition(2);
        r.fire(1);
        s = replayOf(r.file);
        check(s.officials.size() == 81 && positionShots(s, 2) == 1,
              "QUAL-3P-001 (E): the first standing shot is overall 81");

        r.fire(19);
        s = replayOf(r.file);
        check(s.officials.size() == 100 && positionShots(s, 2) == 20,
              "QUAL-3P-001 (F): 100 of 120, standing 20 of 40");

        r.fire(19);
        s = replayOf(r.file);
        check(s.officials.size() == 119 && positionShots(s, 2) == 39,
              "QUAL-3P-001 (G): 119 of 120 - the course does not stop at 60");
        check(positionShots(s, 0) == 40 && positionShots(s, 1) == 40,
              "QUAL-3P-001: 40 / 40 / 39 by position, and the total is their sum",
              QString::number(positionShots(s, 0)));

        // THE MASTER CLOCK. Two position changes and 119 shots later it is
        // still the one that started, and it is nowhere near 165 minutes.
        const qint64 remaining = frozenRemaining(r.file);
        check(remaining > 0 && remaining < 9900000,
              "QUAL-3P-001: after two position changes the remaining time is "
              "still ONE running 165-minute clock",
              QString::number(remaining));
        check(s.timer.durationMs == 9900000 && s.timer.timerId == TimerId::Match,
              "QUAL-3P-001: and its duration is the match clock's, never a "
              "position clock's");
    }

    // ── ISSF 50 m 3P: declares no course, and is unchanged ───────────────
    {
        Rig r;
        r.start(QVariantMap(), 60, 4500000);     // no adopted authority
        r.fire(20);
        r.changePosition(1);
        r.fire(20);
        r.changePosition(2);
        r.fire(20);
        const SessionState s = replayOf(r.file);
        check(!s.ruleAuthority.isPresent(),
              "QUAL-3P-001: an ISSF session adopts nothing and records as "
              "LEGACY - the migration gave it persistence, not new rules");
        check(s.config.perPositionShots == 20 && s.config.officialShots == 60,
              "QUAL-3P-001: its course is still 20/20/20 out of 60, derived "
              "from the course itself",
              QString::number(s.config.perPositionShots));
        check(positionShots(s, 0) == 20 && positionShots(s, 1) == 20
              && positionShots(s, 2) == 20 && s.officials.size() == 60,
              "QUAL-3P-001: 20 / 20 / 20 and 60 in total, unchanged");
    }

    // ── NEGATIVE CONTROLS ────────────────────────────────────────────────
    {
        // 1. A re-anchored clock. beginOfficialMatch() is called again at every
        //    position change; if it anchored again, the remaining time would
        //    jump back to the full duration. This is the check that would have
        //    caught it.
        Rig r;
        r.start(a160(), 120, 9900000);
        r.fire(40);
        const qint64 beforeChange = frozenRemaining(r.file);
        r.changePosition(1);
        const qint64 afterChange = frozenRemaining(r.file);
        check(afterChange < beforeChange && afterChange < 9900000,
              "QUAL-3P-001 control: the position change did NOT restart the "
              "master clock",
              QString::number(afterChange));

        // 2. A 1.60 session must not be describable as a 60-shot course. Both
        //    its total and its per-position count are on disk.
        const SessionState s = replayOf(r.file);
        check(s.config.officialShots == 120 && s.config.perPositionShots == 40,
              "QUAL-3P-001 control: nothing in the persisted session says 60 or "
              "20, so it cannot recover as 3x20");

        // 3. The position must come from the journal, not from arithmetic on
        //    the shot count: 40 shots fired, and the position is prone because
        //    a position change was RECORDED, not because 40 divides.
        check(s.positionIndex == 1 && positionShots(s, 0) == 40
              && positionShots(s, 1) == 0,
              "QUAL-3P-001 control: the position is a recorded fact - all 40 "
              "shots belong to kneeling, and prone has none yet");

        // 4. A single-position discipline cannot change position at all.
        Rig prone;
        prone.qc.storeForTesting()->setJournalFileForTesting(&prone.file);
        prone.qc.startSession(QStringLiteral("PRONE50"), QStringLiteral("60"),
                              QStringLiteral("A"), 60, 4500000, 900000, -1,
                              QString(), QString());
        check(!prone.qc.changePosition(1),
              "QUAL-3P-001 control: a course without positions refuses a "
              "position change");
    }
}
