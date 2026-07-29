// Wind Map — controller, workflow and resume (Stage 5).
//
// Everything here drives the REAL WindMapController with an injected in-memory
// journal + ManualClock, so it is deterministic and touches no disk. Compiling
// the controller into this QT=core harness is also the proof that it carries
// no QML/GUI dependency.
//
// What the file is meant to prove, in order:
//   · Wind Map runs on 50m Prone and 50m 3P and NOWHERE else — closed, not
//     coerced.
//   · A wind condition is one of THREE distinct recorded states (measured,
//     calm, no reading) and invalid input is refused rather than defaulted.
//   · m/s -> hundredths happens in the controller; QML never sees hundredths.
//   · Each accepted shot takes an IMMUTABLE COPY of the standing condition.
//   · Phase transitions fail CLOSED; nothing is clamped or skipped.
//   · An interrupted session resumes into the SAME phase, including the case
//     that motivated journalling the phase at all: counted shots begun with
//     no counted shot yet fired.
//   · A resume re-journals NOTHING.

#include "training/WindMapController.h"
#include "reliability/events/EventSerializer.h"
#include "reliability/journal/JournalValidator.h"
#include "reliability/replay/ReplayEngine.h"
#include "test_support.h"

using namespace ta::rel;
using namespace ta::training;

namespace {

// Phase ids, spelled out so a failure message reads as the workflow does.
constexpr int kIdle = 0, kSetup = 1, kSighters = 2, kCounted = 3;
constexpr int kPositionReview = 4, kSessionReview = 5, kCompleted = 6;

struct Rig {
    MemoryJournalFile file;
    ManualClock clock;
    WindMapController wm;

    Rig()
    {
        wm.storeForTesting()->setClockForTesting(&clock);
        wm.storeForTesting()->setJournalFileForTesting(&file);
    }
    bool start(const char* disciplineId, int plan = 40, bool sighters = true)
    {
        return wm.configureSession(QString::fromLatin1(disciplineId), plan, sighters)
            && wm.startWindMap(QStringLiteral("Alex Example"));
    }
    // A shot from the physical target (source 0), with a fresh external id.
    bool shoot(double x, double y, double score = 10.2)
    {
        return wm.registerShot(x, y, score, ++m_ext, 0.0, 0);
    }
    qint64 m_ext = 1000;
};

// Replays whatever the rig has journalled so far and hands back a recovered
// state, built the SAME way RecoveryCoordinator::buildRecoveredState builds
// it (validate -> replay the valid prefix -> carry the chain seed). Only the
// disk scan is replaced; nothing about the resume itself is simulated.
RecoveredMatchState recoveredFrom(const MemoryJournalFile& file)
{
    const ValidationReport rep = JournalValidator::validateBytes(file.data);
    const ReplayResult r = ReplayEngine::replay(rep.validEnvelopes);
    RecoveredMatchState rec;
    rec.state = r.state;
    rec.sessionId = r.state.sessionId;
    rec.journalPath = QStringLiteral("<memory>");
    rec.lastValidSeq = rep.lastValidSeq;
    rec.recoveryClass = RecoveryClass::Recoverable;
    if (!rep.validEnvelopes.isEmpty()) {
        rec.lastEventWallIso = rep.validEnvelopes.last().wallTimestampIso;
        rec.lastLineHash = rep.validEnvelopes.last().currentHash;
        rec.lastEventMonoMs = rep.validEnvelopes.last().monotonicMs;
        qint64 bytes = 0;
        for (const EventEnvelope& env : rep.validEnvelopes) {
            QByteArray line;
            if (!EventSerializer::serializeCompleteEnvelope(env, &line).ok) { bytes = -1; break; }
            bytes += line.size() + 1;
        }
        rec.validByteLength = bytes;
    }
    return rec;
}

} // namespace

void run_windmap_controller_tests()
{
    fputs("\n--- wind map controller (stage 5) ---\n", stdout);

    // ── 1. discipline scope: closed, never coerced ──────────────────────
    {
        check(WindMapController::isDisciplineSupported(QStringLiteral("PRONE50"))
              && WindMapController::isDisciplineSupported(QStringLiteral("3P50")),
              "1. 50m Prone and 50m 3P are supported");
        const char* rejected[] = { "AR10", "AP10", "FINAL3P", "TRAINING", "", "prone50" };
        bool allClosed = true;
        for (const char* id : rejected)
            if (WindMapController::isDisciplineSupported(QString::fromLatin1(id)))
                allClosed = false;
        check(allClosed, "1. every other discipline id is refused (including case)");

        Rig r;
        check(!r.wm.configureSession(QStringLiteral("AR10"), 40, true),
              "1. configuring 10m Air Rifle is refused");
        check(r.wm.disciplineId().isEmpty(),
              "1. a refused discipline leaves NO partial configuration");
        check(!r.wm.startWindMap(QStringLiteral("Alex Example")),
              "1. an unconfigured session cannot start");
        check(r.wm.phase() == kIdle, "1. a refused start stays Idle");
    }

    // ── 2. start ────────────────────────────────────────────────────────
    {
        Rig r;
        check(r.start("PRONE50"), "2. 50m Prone Wind Map starts");
        check(r.wm.phase() == kSetup, "2. it opens in Setup");
        check(!r.wm.threePositions(), "2. Prone is not a 3P session");
        check(r.wm.currentPosition() == 0, "2. Prone has no named position");
        check(!r.wm.hasWindReading(),
              "2. no standing condition until one is recorded — NOT calm, NOT 0 deg");
        check(r.wm.conditionSummary() == QLatin1String("No wind reading recorded"),
              "2. absence reads as absence");

        Rig t;
        check(t.start("3P50"), "2. 50m 3P Wind Map starts");
        check(t.wm.threePositions() && t.wm.currentPosition() == 1,
              "2. 3P opens at Kneeling");
        check(t.wm.positionName() == QLatin1String("Kneeling"), "2. named Kneeling");

        Rig n;
        n.wm.configureSession(QStringLiteral("PRONE50"), 40, true);
        check(!n.wm.startWindMap(QStringLiteral("   ")),
              "2. a blank athlete name is refused");
        check(!n.wm.lastStartError().isEmpty(),
              "2. the refusal carries an operator-facing reason");
    }

    // ── 3. the three condition states ───────────────────────────────────
    {
        Rig r; r.start("PRONE50");
        check(r.wm.setMeasuredCondition(270, 2.5, QStringLiteral("gusting")),
              "3. a measured condition is recorded");
        check(r.wm.directionDegrees() == 270 && r.wm.directionLabel() == QLatin1String("W"),
              "3. 270 deg reads as W");
        check(qAbs(r.wm.speedMetresPerSecond() - 2.5) < 1e-9,
              "3. speed comes back as m/s, not hundredths");
        check(r.wm.conditionNote() == QLatin1String("gusting"), "3. the note is kept");

        check(r.wm.setCalmCondition(QStringLiteral("flags limp")), "3. calm is recorded");
        check(r.wm.isCalm() && r.wm.hasWindReading(),
              "3. calm is a READING, distinct from no reading");
        check(r.wm.directionDegrees() == 0 && r.wm.speedMetresPerSecond() == 0.0,
              "3. calm carries no direction — 0 here means 'none', never North");
        check(r.wm.conditionSummary() == QLatin1String("Calm"), "3. calm reads as Calm");

        check(r.wm.setNoReadingCondition(), "3. 'no reading' is recorded explicitly");
        check(!r.wm.hasWindReading() && !r.wm.isCalm(),
              "3. no reading is neither a reading nor calm");
        check(r.wm.conditionChanges() == 3, "3. all three changes are counted");
    }

    // ── 4. invalid input is refused, never defaulted ────────────────────
    {
        Rig r; r.start("PRONE50");
        check(!r.wm.setMeasuredCondition(90, -1.0), "4. a negative speed is refused");
        check(!r.wm.setMeasuredCondition(90, qQNaN()), "4. NaN speed is refused");
        check(!r.wm.setMeasuredCondition(90, 2000.0), "4. an absurd speed is refused");
        check(!r.wm.hasWindReading(),
              "4. NOTHING was recorded by the refusals — no silent 0 deg North");
        check(r.wm.conditionChanges() == 0, "4. a refusal is not journalled");

        // Out-of-range DIRECTIONS normalise (a compass wraps); speeds do not.
        check(r.wm.setMeasuredCondition(400, 1.0) && r.wm.directionDegrees() == 40,
              "4. 400 deg normalises to 40 deg");
        check(r.wm.setMeasuredCondition(-90, 1.0) && r.wm.directionDegrees() == 270,
              "4. -90 deg normalises to 270 deg");
    }

    // ── 5. the immutable shot/condition association ──────────────────────
    {
        Rig r; r.start("PRONE50");
        r.wm.setMeasuredCondition(90, 2.5);
        r.wm.beginCountedShots();
        r.shoot(1.0, 1.0);
        // Change the standing condition AFTER the shot: the shot must not move.
        r.wm.setMeasuredCondition(180, 6.0);
        r.shoot(2.0, 2.0);
        r.wm.setNoReadingCondition();
        r.shoot(3.0, 3.0);

        const QVariantList shots = r.wm.reviewShots();
        check(shots.size() == 3, "5. three counted shots recorded");
        const QVariantMap a = shots.value(0).toMap();
        const QVariantMap b = shots.value(1).toMap();
        const QVariantMap c = shots.value(2).toMap();
        check(a.value(QStringLiteral("directionDegrees")).toInt() == 90
              && qAbs(a.value(QStringLiteral("speedMetresPerSecond")).toDouble() - 2.5) < 1e-9,
              "5. shot 1 KEPT its original 90 deg / 2.5 m/s");
        check(b.value(QStringLiteral("directionDegrees")).toInt() == 180
              && b.value(QStringLiteral("band")).toString() == QLatin1String("Strong"),
              "5. shot 2 kept its own 180 deg / 6.0 m/s (Strong)");
        check(!c.value(QStringLiteral("hasWindReading")).toBool()
              && !c.value(QStringLiteral("calm")).toBool(),
              "5. shot 3 recorded NO reading — never back-filled from a neighbour");
        check(c.value(QStringLiteral("directionLabel")).toString().isEmpty(),
              "5. a shot with no reading exposes no direction label");
    }

    // ── 6. sighters never merge into counted shots ──────────────────────
    {
        Rig r; r.start("PRONE50");
        r.wm.setCalmCondition();
        check(r.wm.beginSighters(), "6. sighters begin");
        r.shoot(0.5, 0.5); r.shoot(0.6, 0.6);
        check(r.wm.sighterCount() == 2 && r.wm.countedShots() == 0,
              "6. sighters are counted as sighters and nothing else");
        check(r.wm.finishSighters(), "6. sighters finish");
        r.shoot(1.0, 1.0);
        check(r.wm.countedShots() == 1 && r.wm.sighterCount() == 2,
              "6. the counted shot did not absorb the sighters");
        const QVariantMap sum = r.wm.reviewSummary();
        check(sum.value(QStringLiteral("countedShots")).toInt() == 1
              && sum.value(QStringLiteral("sighterShots")).toInt() == 2,
              "6. the summary keeps them separate");
        check(sum.value(QStringLiteral("countedCalm")).toInt() == 1,
              "6. condition coverage counts COUNTED shots only");
    }

    // ── 7. phase transitions fail closed ────────────────────────────────
    {
        Rig a; a.start("PRONE50");
        check(!a.wm.endPosition(),
              "7. 50m Prone can never enter a position review");
        check(!a.wm.changePosition(2), "7. 50m Prone refuses a position change");
        check(!a.wm.completeSession(),
              "7. a session cannot complete before its review");
        check(a.wm.phase() == kSetup, "7. every refusal left the phase untouched");

        Rig b; b.start("PRONE50");
        b.wm.beginCountedShots();
        check(!b.wm.beginSighters(),
              "7. sighters cannot be re-entered once counted shots have begun");
        check(b.wm.phase() == kCounted, "7. the refusal did not clamp the phase");
        check(b.wm.endCapture() && b.wm.phase() == kSessionReview, "7. capture ends into review");
        check(b.wm.completeSession() && b.wm.phase() == kCompleted, "7. review completes");
        check(!b.wm.beginCountedShots() && !b.wm.endCapture(),
              "7. Completed is terminal");
    }

    // ── 8. shots outside a shooting phase are ignored ───────────────────
    {
        Rig r; r.start("PRONE50");
        r.wm.setCalmCondition();
        check(!r.shoot(1.0, 1.0), "8. a shot fired during Setup is refused");
        check(r.wm.reviewShots().isEmpty(), "8. and nothing was journalled");
        r.wm.beginCountedShots();
        check(r.shoot(1.0, 1.0), "8. the same shot is accepted once counting");
        check(!r.wm.registerShot(1.0, 1.0, 10.0, r.m_ext, 0.0, 0),
              "8. a repeated external id is refused as a duplicate");
        check(r.wm.countedShots() == 1, "8. the duplicate did not double-count");
        check(!r.wm.registerShot(9999.0, 0.0, 10.0, ++r.m_ext, 0.0, 0),
              "8. an impossible coordinate is refused");
    }

    // ── 9. 3P positions stay separate ───────────────────────────────────
    {
        Rig r; r.start("3P50");
        r.wm.setMeasuredCondition(45, 3.0);
        r.wm.beginCountedShots();
        r.shoot(1.0, 1.0);
        check(r.wm.endPosition() && r.wm.phase() == kPositionReview,
              "9. a 3P position closes into its review");
        check(r.wm.changePosition(2), "9. Kneeling -> Prone");
        check(r.wm.currentPosition() == 2 && r.wm.positionName() == QLatin1String("Prone"),
              "9. the position advanced");
        check(!r.wm.changePosition(2), "9. changing to the same position is refused");
        check(!r.wm.changePosition(9), "9. an unknown position is refused");
        r.wm.beginCountedShots();
        r.shoot(2.0, 2.0); r.shoot(2.1, 2.1);
        check(r.wm.countedShots() == 2,
              "9. the per-position count is Prone's alone");
        check(r.wm.totalCountedShots() == 3, "9. the session total spans both");
        r.wm.endPosition(); r.wm.changePosition(3); r.wm.beginCountedShots();
        r.shoot(3.0, 3.0);
        const QVariantList pos = r.wm.reviewSummary().value(QStringLiteral("positions")).toList();
        check(pos.size() == 3, "9. all three positions appear separately");
        check(pos.value(0).toMap().value(QStringLiteral("countedShots")).toInt() == 1
              && pos.value(1).toMap().value(QStringLiteral("countedShots")).toInt() == 2
              && pos.value(2).toMap().value(QStringLiteral("countedShots")).toInt() == 1,
              "9. nothing was pooled across positions");
    }

    // ── 10. resume: the case that made the phase durable ────────────────
    {
        // Counted shots BEGUN, none fired yet, then a crash. No derivation
        // from the recorded shots could tell this from the sighter phase —
        // and getting it wrong would record the next shot as a sighter.
        Rig r; r.start("PRONE50");
        r.wm.setMeasuredCondition(270, 2.5);
        r.wm.beginSighters();
        r.shoot(0.5, 0.5);
        r.wm.finishSighters();          // <- counted shots begin
        // crash here: no counted shot exists.
        const RecoveredMatchState rec = recoveredFrom(r.file);
        check(rec.state.wmPhase == kCounted,
              "10. the journal records the counted phase before any counted shot");

        Rig back;
        check(back.wm.resumeFromRecoveredState(rec), "10. the session resumes");
        check(back.wm.phase() == kCounted,
              "10. it resumes into COUNTED SHOTS, not sighters");
        check(back.wm.sighterCount() == 1 && back.wm.countedShots() == 0,
              "10. the sighter survived and no counted shot was invented");
        check(back.wm.hasWindReading() && back.wm.directionDegrees() == 270
              && qAbs(back.wm.speedMetresPerSecond() - 2.5) < 1e-9,
              "10. the standing condition survived exactly");
        // The next shot must be COUNTED — the point of the whole exercise.
        back.wm.registerShot(1.0, 1.0, 10.4, 5001, 0.0, 0);
        check(back.wm.countedShots() == 1 && back.wm.sighterCount() == 1,
              "10. the first shot after the resume is COUNTED, not a sighter");
    }

    // ── 11. resume across every phase ───────────────────────────────────
    {
        struct Case { const char* disc; int stopAfter; int expect; const char* what; };
        const Case cases[] = {
            { "PRONE50", 0, kSetup,          "crash in Setup" },
            { "PRONE50", 1, kSighters,       "crash during sighters" },
            { "PRONE50", 2, kCounted,        "crash during counted shots" },
            { "PRONE50", 3, kSessionReview,  "crash in the session review" },
            { "3P50",    4, kPositionReview, "crash in a 3P position review" },
        };
        for (const Case& c : cases) {
            Rig r; r.start(c.disc);
            r.wm.setCalmCondition();
            if (c.stopAfter >= 1) { r.wm.beginSighters(); r.shoot(0.4, 0.4); }
            if (c.stopAfter >= 2 && c.stopAfter != 4) { r.wm.finishSighters(); r.shoot(1.0, 1.0); }
            if (c.stopAfter == 3) r.wm.endCapture();
            if (c.stopAfter == 4) { r.wm.finishSighters(); r.shoot(1.0, 1.0); r.wm.endPosition(); }
            Rig back;
            const bool ok = back.wm.resumeFromRecoveredState(recoveredFrom(r.file));
            check(ok && back.wm.phase() == c.expect,
                  QString(QStringLiteral("11. %1 resumes into the same phase"))
                      .arg(QLatin1String(c.what)),
                  QStringLiteral("phase %1").arg(back.wm.phase()));
        }
    }

    // ── 12. a resume re-journals nothing ────────────────────────────────
    {
        Rig r; r.start("PRONE50");
        r.wm.setMeasuredCondition(180, 4.0);
        r.wm.beginCountedShots();
        r.shoot(1.0, 1.0); r.shoot(2.0, 2.0);
        const RecoveredMatchState rec = recoveredFrom(r.file);
        // Count the Wind Map PAYLOAD lines before the resume. Recovery markers
        // are expected and legitimate; what must never reappear is the
        // programme's own content.
        auto windMapLines = [](const QByteArray& data) {
            int n = 0;
            for (const QByteArray& line : data.split('\n'))
                if (line.contains("\"WindMap") || line.contains("\"WindCondition")) ++n;
            return n;
        };
        const int before = windMapLines(r.file.data);

        Rig back;
        back.file.data = r.file.data;          // resume onto the same journal
        check(back.wm.resumeFromRecoveredState(rec), "12. resume succeeds");
        check(back.wm.countedShots() == 2,
              "12. the two shots are present once, not duplicated");
        check(windMapLines(back.file.data) == before,
              "12. the resume re-journalled NO wind map event",
              QStringLiteral("%1 -> %2").arg(before).arg(windMapLines(back.file.data)));
        check(back.wm.conditionChanges() == 1,
              "12. the condition change was not replayed into a second one");
    }

    // ── 13. resume refuses anything that is not a Wind Map session ──────
    {
        Rig r; r.start("PRONE50");
        RecoveredMatchState rec = recoveredFrom(r.file);
        RecoveredMatchState notTraining = rec;
        notTraining.state.sessionKind = QStringLiteral("");
        Rig a;
        check(!a.wm.resumeFromRecoveredState(notTraining),
              "13. a competition journal is refused");
        RecoveredMatchState otherProgram = rec;
        otherProgram.state.wmProgramId = QStringLiteral("position_transition");
        Rig b;
        check(!b.wm.resumeFromRecoveredState(otherProgram),
              "13. another Training programme's journal is refused");
        RecoveredMatchState badDiscipline = rec;
        badDiscipline.state.wmDisciplineId = QStringLiteral("AR10");
        Rig c;
        check(!c.wm.resumeFromRecoveredState(badDiscipline),
              "13. an unsupported discipline is refused, not coerced");
        check(c.wm.phase() == kIdle, "13. a refused resume stays Idle");
    }

    // ── 14. close returns the controller to a clean slate ───────────────
    {
        Rig r; r.start("PRONE50");
        r.wm.setMeasuredCondition(90, 3.0);
        r.wm.beginCountedShots();
        r.shoot(1.0, 1.0);
        check(r.wm.closeSessionCleanly(), "14. the session closes cleanly");
        check(r.wm.phase() == kIdle, "14. the phase returns to Idle");
        check(!r.wm.hasWindReading(), "14. the standing condition is cleared");
        check(r.wm.totalCountedShots() == 0 && r.wm.reviewShots().isEmpty(),
              "14. no stale counters survive into the next session");
    }

    // ── 15. the review stays factual ────────────────────────────────────
    {
        Rig r; r.start("PRONE50", 40, true);
        r.wm.setMeasuredCondition(90, 3.0);
        r.wm.beginCountedShots();
        r.shoot(1.0, 1.0);
        r.wm.setNoReadingCondition();
        r.shoot(2.0, 2.0);
        r.wm.endCapture();
        const QVariantMap m = r.wm.reviewSummary();
        check(m.value(QStringLiteral("programme")).toString()
                  == QStringLiteral("Wind Map — Post-Session Review"),
              "15. the approved programme name is used",
              m.value(QStringLiteral("programme")).toString());
        check(m.value(QStringLiteral("countedWithReading")).toInt() == 1
              && m.value(QStringLiteral("countedNoReading")).toInt() == 1,
              "15. reading coverage is reported as a fact");
        check(m.value(QStringLiteral("disciplineName")).toString()
                  == QLatin1String("50 m Rifle Prone"),
              "15. the discipline is named, not inferred");
        check(!m.value(QStringLiteral("disclaimer")).toString().isEmpty(),
              "15. the training-only disclaimer is present");
        // Stage 5 produces no analysis: there is no comparison, ranking or
        // recommendation key to leak one.
        check(!m.contains(QStringLiteral("comparison"))
              && !m.contains(QStringLiteral("recommendation"))
              && !m.contains(QStringLiteral("observations")),
              "15. no analytical or advisory content is produced in this stage");
    }
}
