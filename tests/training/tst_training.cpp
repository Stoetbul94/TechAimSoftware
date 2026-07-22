// Training Lab (T1) — Technical Blocks harness.
//
// Covers the mandated areas: configuration (defaults/custom/invalid/3P),
// shot acceptance + the F10 Live/Demo gate (wrong source -> no event), the
// visibility projections, the block lifecycle (once-only events, notes, no
// spill), analytics (MPI/radius/diameter/spread/SD/timing + edges), recovery
// by replay (no duplicates, hidden values stay hidden, hash chain valid) and
// competition separation (kind=Training, never a competition shot/candidate).
// Deterministic: injected in-memory journal + manual clock, no disk, no GUI.

#include "training/TrainingProgramController.h"
#include "training/TrainingBlockMetrics.h"
#include "reliability/journal/JournalWriter.h"
#include "reliability/journal/JournalValidator.h"
#include "reliability/replay/ReplayEngine.h"
#include "reliability/store/MonotonicClock.h"
#include "reliability/storage/StoragePaths.h"

#include <QDir>

#include <QCoreApplication>
#include <QVariantMap>

#include <cstdio>
#include <variant>

using namespace ta::rel;
using namespace ta::training;

static int g_checks = 0;
static int g_failures = 0;

static void check(bool ok, const char* name, const QString& detail = QString())
{
    ++g_checks;
    if (ok) {
        std::printf("PASS  %s\n", name);
    } else {
        ++g_failures;
        std::printf("FAIL  %s  %s\n", name, detail.toUtf8().constData());
    }
    std::fflush(stdout);
}

// Minimal in-memory journal double (same contract as the reliability harness).
struct MemFile : public IJournalFile {
    QByteArray data;
    bool opened = false;
    bool open() override { opened = true; return true; }
    bool isOpen() const override { return opened; }
    qint64 write(const QByteArray& b) override { data += b; return b.size(); }
    bool flush() override { return true; }
    bool sync() override { return true; }
    QString path() const override { return QStringLiteral("<memory>"); }
    QString lastErrorDetail() const override { return QString(); }
};

namespace {

int countType(const QByteArray& bytes, const char* type)
{
    const ValidationReport rep = JournalValidator::validateBytes(bytes);
    int n = 0;
    for (const EventEnvelope& e : rep.validEnvelopes)
        if (qstrcmp(eventTypeId(e.payload), type) == 0) ++n;
    return n;
}

// Set up a controller on an injected journal/clock with defaults + focus.
void prepare(TrainingProgramController& c, MemFile& f, ManualClock& clk,
             const char* disciplineId, int opMode)
{
    c.storeForTesting()->setClockForTesting(&clk);
    c.storeForTesting()->setJournalFileForTesting(&f);
    c.configureDefaults(QString::fromLatin1(disciplineId));
    c.setTechnicalFocus(QStringLiteral("Trigger"));
    if (opMode >= 0)
        c.setOperatingMode(opMode);
}

// Fire n valid simulated shots (Demo source) advancing the clock.
int fire(TrainingProgramController& c, ManualClock& clk, int n,
         double score = 10.2, double x = 1.0, double y = -1.0)
{
    static qint64 ext = 100;
    int accepted = 0;
    for (int i = 0; i < n; ++i) {
        clk.advance(5000);
        if (c.registerShot(x + i * 0.1, y, score, ++ext, 0.0, /*Simulated*/1))
            ++accepted;
    }
    return accepted;
}

} // namespace

// ── 1. configuration ─────────────────────────────────────────────────────
static void testConfiguration()
{
    std::printf("--- configuration ---\n");
    {
        TrainingProgramController c;
        c.configureDefaults(QStringLiteral("AR10"));
        check(c.blockCount() == 5 && c.shotsPerBlock() == 6,
              "AR default: 5 blocks x 6 shots (30 total)");
        check(c.validateConfig().isEmpty(), "AR default config valid");
    }
    {
        TrainingProgramController c;
        c.configureDefaults(QStringLiteral("AP10"));
        check(c.blockCount() == 6 && c.shotsPerBlock() == 5,
              "AP default: 6 blocks x 5 shots (30 total)");
        const QStringList f = c.focusOptionsForDiscipline();
        check(!f.contains(QStringLiteral("Shoulder contact")),
              "AP focus list does not foreground rifle-only contact terms");
    }
    {
        TrainingProgramController c;
        c.configureDefaults(QStringLiteral("PRONE50"));
        check(c.blockCount() == 5 && c.shotsPerBlock() == 6,
              "Prone default: 5 blocks x 6 shots");
        check(c.focusOptionsForDiscipline().contains(QStringLiteral("Shoulder contact")),
              "rifle focus list includes contact terms");
    }
    {
        TrainingProgramController c;
        c.configureDefaults(QStringLiteral("3P50"));
        check(c.blockCount() == 6 && c.shotsPerBlock() == 6,
              "3P default: 6 blocks (2 per position) x 6 shots = 36");
        // sequence: blocks 1-2 K, 3-4 P, 5-6 S
        TechnicalBlocksConfig cfg = TechnicalBlocksConfig::defaultsFor(Discipline::ThreePositions50m);
        check(cfg.positionForBlock(1) == Position::Kneeling
                  && cfg.positionForBlock(2) == Position::Kneeling
                  && cfg.positionForBlock(3) == Position::Prone
                  && cfg.positionForBlock(4) == Position::Prone
                  && cfg.positionForBlock(5) == Position::Standing
                  && cfg.positionForBlock(6) == Position::Standing,
              "3P block sequence is K,K,P,P,S,S — explicit order preserved");
    }
    {
        TrainingProgramController c;
        c.configureDefaults(QStringLiteral("AR10"));
        c.setBlockCount(8); c.setShotsPerBlock(4);
        check(c.validateConfig().isEmpty(), "valid custom values accepted (8x4)");
        c.setBlockCount(0);
        check(!c.validateConfig().isEmpty(), "zero blocks rejected");
        c.setBlockCount(13);
        check(!c.validateConfig().isEmpty(), "13 blocks rejected (max 12)");
        c.setBlockCount(12); c.setShotsPerBlock(2);
        check(!c.validateConfig().isEmpty(), "2 shots/block rejected (min 3)");
        c.setShotsPerBlock(20);
        check(!c.validateConfig().isEmpty(),
              "12x20=240 rejected (over training maximum)");
        c.setShotsPerBlock(21);
        check(!c.validateConfig().isEmpty(), "21 shots/block rejected (max 20)");
    }
    {
        TrainingProgramController c;
        MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", /*Demo*/1);
        check(c.startTraining(QStringLiteral("A")), "training session starts");
        const ValidationReport rep = JournalValidator::validateBytes(f.data);
        const ReplayResult rr = ReplayEngine::replay(rep.validEnvelopes);
        check(rr.state.sessionKind == QLatin1String("Training"),
              "session kind persisted as Training");
        check(rr.state.trainingProgramId == QLatin1String("technical_blocks"),
              "programme ID persisted as technical_blocks");
        check(rr.state.trainingFocus == QLatin1String("Trigger"),
              "technical focus stored");
        check(rr.state.trainingVisibility == 0, "visibility mode stored");
        check(rr.state.operatingMode == QLatin1String("Demo"),
              "operating mode captured at session start");
        c.resetTraining();
    }
}

// ── 2. shot acceptance + F10 gate ────────────────────────────────────────
static void testShotAcceptance()
{
    std::printf("--- shot acceptance + source gate ---\n");
    {
        // Demo mode: simulated accepted, physical rejected with NO event.
        TrainingProgramController c;
        MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", /*Demo*/1);
        c.startTraining(QStringLiteral("A"));
        check(c.registerShot(1.0, 1.0, 10.4, 1, 0.0, /*Simulated*/1),
              "Demo: simulated shot accepted");
        const int before = countType(f.data, "TrainingShotAccepted");
        check(!c.registerShot(1.0, 1.0, 10.4, 2, 0.0, /*Physical*/0),
              "Demo: physical shot rejected");
        check(countType(f.data, "TrainingShotAccepted") == before,
              "rejected physical source created no journal event");
        check(c.shotsInBlock() == 1, "rejected source did not change block count");
        c.resetTraining();
    }
    {
        // Live mode: physical accepted, simulated rejected with NO event.
        TrainingProgramController c;
        MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", /*Live*/0);
        c.startTraining(QStringLiteral("A"));
        check(c.registerShot(1.0, 1.0, 9.8, 1, 0.0, /*Physical*/0),
              "Live: physical shot accepted");
        check(!c.registerShot(1.0, 1.0, 9.8, 2, 0.0, /*Simulated*/1),
              "Live: simulated click rejected");
        check(countType(f.data, "TrainingShotAccepted") == 1,
              "Live: exactly one accepted shot journalled");
        c.resetTraining();
    }
    {
        // Fixed-point + progression + duplicate + block cap.
        TrainingProgramController c;
        MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", 1);
        c.startTraining(QStringLiteral("A"));
        clk.advance(4000);                       // shot arrives 4 s into session
        c.registerShot(2.5, -3.25, 10.47, 10, 0.0, 1);
        const ValidationReport rep = JournalValidator::validateBytes(f.data);
        const ReplayResult rr = ReplayEngine::replay(rep.validEnvelopes);
        check(rr.state.trainingBlocks.size() == 1
                  && rr.state.trainingBlocks[0].shots.size() == 1
                  && rr.state.trainingBlocks[0].shots[0].scoreTenths == 105,
              "scoreTenths fixed-point (10.47 -> 105 tenths, rounded)");
        check(rr.state.trainingBlocks[0].shots[0].splitMs > 0,
              "timing retained on the journalled shot");
        check(!c.registerShot(1, 1, 10.0, 10, 0.0, 1),
              "duplicate external id rejected");
        fire(c, clk, 5);   // completes the 6-shot block
        check(c.phase() == 3, "block completes into review at configured count");
        check(!c.registerShot(1, 1, 10.0, 999, 0.0, 1),
              "no extra shot spills past the block cap");
        c.resetTraining();
    }
}

// ── 3. visibility projections ────────────────────────────────────────────
static void testVisibility()
{
    std::printf("--- visibility projections ---\n");
    {
        TrainingProgramController c;
        MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", 1);
        c.setVisibilityMode(0);   // Mode A
        c.startTraining(QStringLiteral("A"));
        fire(c, clk, 1);
        check(!c.showImpacts() && !c.showScores() && !c.showGroup(),
              "Mode A live block: no impact, no score, no group");
        // Controller data APIs must not leak the live block either.
        check(c.blockReviewMetrics(1).isEmpty(),
              "Mode A: live-block metrics not exposed");
        check(c.blockShotPlot(1).isEmpty(),
              "Mode A: live-block shot plot not exposed");
        fire(c, clk, 5);
        check(c.phase() == 3 && c.showScores() && c.showImpacts(),
              "block completion reveals measured result");
        check(!c.blockReviewMetrics(1).isEmpty(),
              "review metrics available after completion");
        c.resetTraining();
    }
    {
        TrainingProgramController c;
        MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", 1);
        c.setVisibilityMode(1);   // Mode B
        c.startTraining(QStringLiteral("A"));
        fire(c, clk, 1);
        check(c.showImpacts() && c.showGroup() && !c.showScores(),
              "Mode B live block: markers/group only, no numbers");
        c.resetTraining();
    }
    {
        TrainingProgramController c;
        MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", 1);
        c.setVisibilityMode(2);   // Mode C
        c.startTraining(QStringLiteral("A"));
        fire(c, clk, 1);
        check(c.showImpacts() && !c.showScores(),
              "Mode C live block: impact visible, score hidden");
        c.resetTraining();
    }
}

// ── 4. lifecycle ─────────────────────────────────────────────────────────
static void testLifecycle()
{
    std::printf("--- block lifecycle ---\n");
    TrainingProgramController c;
    MemFile f; ManualClock clk;
    prepare(c, f, clk, "AR10", 1);
    c.setBlockCount(2); c.setShotsPerBlock(3);
    c.startTraining(QStringLiteral("A"));
    check(countType(f.data, "TrainingBlockStarted") == 1, "block 1 started once");
    fire(c, clk, 3);
    check(countType(f.data, "TrainingBlockCompleted") == 1, "block 1 completed once");
    check(c.phase() == 3, "review shown (not auto-dismissed)");
    check(c.saveNote(QStringLiteral("felt rushed")), "athlete note saved");
    check(countType(f.data, "TrainingNoteSaved") == 1, "note journalled");
    check(c.continueToNextBlock(), "explicit continue starts block 2");
    check(countType(f.data, "TrainingBlockStarted") == 2, "block 2 started once");
    fire(c, clk, 3);
    check(countType(f.data, "TrainingBlockCompleted") == 2, "final block completed once");
    check(c.continueToNextBlock(), "continue after final review completes training");
    check(countType(f.data, "TrainingCompleted") == 1, "TrainingCompleted exactly once");
    check(c.phase() == 4, "phase Complete (final comparison)");
    check(!c.registerShot(1, 1, 10.0, 900, 0.0, 1),
          "completed session rejects further shots");
    check(!c.continueToNextBlock(), "no duplicate completion possible");
    check(countType(f.data, "TrainingCompleted") == 1, "still exactly one TrainingCompleted");
    // replay: note + completion survive
    const ValidationReport rep = JournalValidator::validateBytes(f.data);
    const ReplayResult rr = ReplayEngine::replay(rep.validEnvelopes);
    check(rr.state.trainingCompleted, "replay: completion state restored");
    check(rr.state.trainingBlocks.size() == 2
              && rr.state.trainingBlocks[1].note == QLatin1String(""),
          "replay: blocks restored");
    check(rr.state.trainingBlocks[0].note == QLatin1String("felt rushed"),
          "replay: note restored on its block");
    c.resetTraining();
}

// ── 4b. early end (T1 part 2) ────────────────────────────────────────────
static void testEarlyEnd()
{
    std::printf("--- early end ---\n");
    TrainingProgramController c;
    MemFile f; ManualClock clk;
    prepare(c, f, clk, "AR10", 1);
    c.setBlockCount(3); c.setShotsPerBlock(3);
    c.startTraining(QStringLiteral("A"));
    check(!c.endTrainingEarly(), "early end refused while block active");
    fire(c, clk, 3);                              // block 1 -> review
    check(c.blockElapsedSec() > 0, "block elapsed time projection > 0");
    check(c.endTrainingEarly(), "early end accepted from block review");
    check(c.phase() == 4, "early end reaches summary phase");
    check(c.endedEarly(), "endedEarly() true for a partial session");
    // TrainingCompleted carries the TRUE completed count (1 of 3) — never
    // fabricates the uncompleted blocks.
    const ValidationReport rep = JournalValidator::validateBytes(f.data);
    int completedBlocksField = -1;
    for (const EventEnvelope& e : rep.validEnvelopes)
        if (auto* tc = std::get_if<TrainingCompleted>(&e.payload))
            completedBlocksField = tc->completedBlocks;
    check(completedBlocksField == 1,
          "TrainingCompleted records the true completed count (1 of 3)");
    check(countType(f.data, "TrainingCompleted") == 1, "exactly one TrainingCompleted");
    check(!c.registerShot(1, 1, 10.0, 700, 0.0, 1), "ended session rejects shots");
    c.resetTraining();
}

// ── 5. analytics ─────────────────────────────────────────────────────────
static void testAnalytics()
{
    std::printf("--- block metrics / comparison ---\n");
    auto shot = [](double xMm, double yMm, int tenths, int splitMs) {
        ShotCore s;
        s.shotNumber = 1; s.withinStage = 1; s.stageId = 1;
        s.xHundredthMm = int(xMm * 100); s.yHundredthMm = int(yMm * 100);
        s.scoreTenths = qint16(tenths); s.splitMs = splitMs;
        return s;
    };
    {
        // symmetric square around (1,1): MPI (1,1), known geometry
        QVector<ShotCore> shots{ shot(0, 0, 100, 4000), shot(2, 0, 102, 5000),
                                 shot(0, 2, 104, 6000), shot(2, 2, 106, 5000) };
        const BlockMetrics m = computeBlockMetrics(shots);
        check(qAbs(m.mpiX - 1.0) < 1e-9 && qAbs(m.mpiY - 1.0) < 1e-9, "MPI correct");
        check(qAbs(m.groupRadius - std::sqrt(2.0)) < 1e-6,
              "group radius (mean dist from MPI) correct");
        check(qAbs(m.groupDiameter - std::sqrt(8.0)) < 1e-6,
              "group diameter (extreme spread) correct");
        check(qAbs(m.horizontalSpread - m.verticalSpread) < 1e-9,
              "H/V spread symmetric for the square");
        check(qAbs(m.averageScore - 10.3) < 1e-9, "average score correct");
        check(m.scoreStdDev > 0.0, "score SD computed");
        check(m.hasTiming && qAbs(m.averageShotTime - 5.0) < 1e-9,
              "timing mean correct (5.0 s)");
    }
    {
        const BlockMetrics none = computeBlockMetrics({});
        check(!none.hasData && !none.hasGroup, "zero-shot edge: no data, no group");
        const BlockMetrics one = computeBlockMetrics({ shot(1, 1, 105, 4000) });
        check(one.hasData && !one.hasGroup,
              "one-shot edge: data but never a 'group'");
        QVector<ShotCore> same{ shot(1, 1, 100, 4000), shot(1, 1, 100, 4000),
                                shot(1, 1, 100, 4000) };
        const BlockMetrics ident = computeBlockMetrics(same);
        check(ident.groupDiameter < 1e-9 && ident.scoreStdDev < 1e-9,
              "identical-shot edge: zero spread, zero SD");
    }
    {
        // comparison: block2 tighter + higher; drift computed; guards hold
        QVector<BlockMetrics> blocks;
        blocks.append(computeBlockMetrics({ shot(0, 0, 95, 4000), shot(4, 0, 96, 5000),
                                            shot(0, 4, 97, 6000) }));
        blocks.append(computeBlockMetrics({ shot(5, 5, 104, 4000), shot(6, 5, 104, 4200),
                                            shot(5, 6, 104, 4100) }));   // SD 0: clearly most repeatable
        const BlockComparison c = compareBlocks(blocks);
        check(c.bestScoreBlock == 2, "best-score block identified");
        check(c.tightestGroupBlock == 2, "tightest-group block identified");
        check(c.mostRepeatableBlock == 2, "most repeatable block identified");
        check(c.hasDrift && c.centreDriftMm > 0.0, "centre drift measured");
        check(c.hasSizeChange && c.groupSizeChangePct < 0.0,
              "group-size reduction measured");
        // empty-block guard: no observation from missing data
        QVector<BlockMetrics> withEmpty{ BlockMetrics{}, blocks[1] };
        const BlockComparison g = compareBlocks(withEmpty);
        check(!g.hasDrift && !g.hasSizeChange,
              "single-populated-block: no drift/size claims");
    }
}

// ── 6. recovery by replay ────────────────────────────────────────────────
static void testRecovery()
{
    std::printf("--- recovery (replay) ---\n");
    // Build a crashed mid-block-2 journal, then replay it.
    MemFile f; ManualClock clk;
    {
        TrainingProgramController c;
        prepare(c, f, clk, "3P50", 1);
        c.startTraining(QStringLiteral("A"));
        fire(c, clk, 6);                       // block 1 (Kneeling) complete
        c.saveNote(QStringLiteral("steady"));
        c.continueToNextBlock();               // block 2 (Kneeling)
        fire(c, clk, 2);                       // crash mid-block-2
    }   // controller destroyed; journal bytes survive
    const ValidationReport rep = JournalValidator::validateBytes(f.data);
    check(rep.firstInvalidLine == -1 && rep.validPrefixCount == rep.totalLines,
          "hash chain valid after crash (every line validates)");
    const ReplayResult rr = ReplayEngine::replay(rep.validEnvelopes);
    check(rr.ok, "replay succeeds");
    const SessionState& s = rr.state;
    check(s.sessionKind == QLatin1String("Training"),
          "recovered session classified Training (not competition)");
    check(s.trainingProgramId == QLatin1String("technical_blocks"),
          "programme restored");
    check(s.discipline == Discipline::ThreePositions50m, "discipline restored");
    check(s.trainingVisibility == 0, "visibility restored");
    check(s.trainingFocus == QLatin1String("Trigger"), "focus restored");
    check(s.trainingCurrentBlock == 2, "current block restored");
    check(s.trainingBlocks.size() == 2
              && s.trainingBlocks[0].shots.size() == 6
              && s.trainingBlocks[1].shots.size() == 2,
          "per-block shot counts restored (6 + 2)");
    check(s.trainingBlocks[0].completed && !s.trainingBlocks[1].completed,
          "block completion states restored");
    check(s.trainingBlocks[0].note == QLatin1String("steady"), "note restored");
    check(!s.trainingCompleted, "not completed (mid-session crash)");
    check(countType(f.data, "TrainingBlockCompleted") == 1,
          "no duplicate TrainingBlockCompleted");
    check(countType(f.data, "TrainingCompleted") == 0,
          "no TrainingCompleted before the end");
    check(s.officials.isEmpty(),
          "no competition officials from Training shots (separation)");
    check(s.trainingBlocks[0].position == 0 && s.trainingBlocks[1].position == 0,
          "3P: kneeling blocks carry Kneeling identity");
}

// ── 6b. in-place resume (T1 closure) ─────────────────────────────────────
static void testInPlaceResume()
{
    std::printf("--- in-place resume (real journals) ---\n");
    const QString root = QDir::temp().filePath(
        QStringLiteral("techaim_t1_resume_%1").arg(QCoreApplication::applicationPid()));
    QDir(root).removeRecursively();
    StoragePaths::setRootOverrideForTesting(root);
    StoragePaths::initialize();

    // (a) crash mid-block-2 → resume restores config/phase/progress; firing
    //     continues with the duplicate guard intact and completes cleanly.
    QString sid;
    {
        TrainingProgramController a;
        a.configureDefaults(QStringLiteral("3P50"));
        a.setTechnicalFocus(QStringLiteral("Rhythm"));
        a.setVisibilityMode(1);                       // Mode B
        a.setOperatingMode(1);                        // Demo
        check(a.startTraining(QStringLiteral("A")), "resume(a): session starts on disk");
        sid = a.sessionId();
        static qint64 ext = 5000;
        for (int i = 0; i < 6; ++i) a.registerShot(1, 1, 10.0, ++ext, 0, 1);  // block 1
        a.saveNote(QStringLiteral("k-steady"));
        a.continueToNextBlock();                      // block 2 (Kneeling)
        a.registerShot(2, 2, 9.8, ++ext, 0, 1);
        a.registerShot(2, 1, 9.9, ++ext, 0, 1);       // crash mid-block-2
    }
    {
        TrainingProgramController b;
        b.setOperatingMode(1);
        check(b.resumeFromRecovery(sid), "resume(a): in-place resume succeeds");
        check(b.phase() == 2, "resume(a): active-block phase restored");
        check(b.currentBlock() == 2 && b.shotsInBlock() == 2,
              "resume(a): block 2 with 2 shots restored");
        check(b.blockCount() == 6 && b.shotsPerBlock() == 6,
              "resume(a): 3P configuration restored (6x6)");
        check(b.visibilityMode() == 1 && b.technicalFocus() == QLatin1String("Rhythm"),
              "resume(a): visibility + focus restored");
        check(b.positionName() == QLatin1String("Kneeling"),
              "resume(a): 3P position restored (block 2 = Kneeling)");
        check(b.recoveredCurrentBlockShots().size() == 2,
              "resume(a): current-block shots exposed for the face restore");
        check(!b.registerShot(3, 3, 10.0, 5001, 0, 1),
              "resume(a): pre-crash externalId still refused (duplicate guard)");
        check(!b.registerShot(3, 3, 10.0, 9000, 0, /*Physical*/0),
              "resume(a): wrong-source gate still active after resume");
        // finish block 2; review appears exactly once
        qint64 ext2 = 9100;
        for (int i = 0; i < 4; ++i) b.registerShot(1, 2, 10.1, ++ext2, 0, 1);
        check(b.phase() == 3, "resume(a): block completes into review after resume");
        const ValidationReport rep =
            JournalValidator::validateFile(b.store()->currentJournalPath());
        check(rep.firstInvalidLine == -1, "resume(a): hash chain valid after resume+shots");
        int starts = 0, completes = 0;
        for (const EventEnvelope& e : rep.validEnvelopes) {
            if (qstrcmp(eventTypeId(e.payload), "TrainingSessionStarted") == 0) ++starts;
            if (qstrcmp(eventTypeId(e.payload), "TrainingBlockCompleted") == 0) ++completes;
        }
        check(starts == 1, "resume(a): no duplicate TrainingSessionStarted");
        check(completes == 2, "resume(a): block completions exactly once each");
        b.resetTraining();
    }

    // (b) crash while Block Review durable (block completed) → resume lands in
    //     review, shots rejected, note restored; continue does not double-start.
    QString sid2;
    {
        TrainingProgramController a;
        a.configureDefaults(QStringLiteral("AR10"));
        a.setTechnicalFocus(QStringLiteral("Trigger"));
        a.setOperatingMode(1);
        a.startTraining(QStringLiteral("B"));
        sid2 = a.sessionId();
        qint64 ext = 7000;
        for (int i = 0; i < 6; ++i) a.registerShot(1, 1, 10.2, ++ext, 0, 1);
        a.saveNote(QStringLiteral("review-note"));    // crash during review
    }
    {
        TrainingProgramController b;
        b.setOperatingMode(1);
        check(b.resumeFromRecovery(sid2), "resume(b): review-phase resume succeeds");
        check(b.phase() == 3, "resume(b): restored INTO block review (no auto next)");
        check(!b.registerShot(1, 1, 10.0, 8000, 0, 1),
              "resume(b): shots rejected while review restored");
        const QVariantMap m = b.blockReviewMetrics(1);
        check(m.value(QStringLiteral("note")).toString() == QLatin1String("review-note"),
              "resume(b): saved note restored exactly");
        check(b.continueToNextBlock() && b.currentBlock() == 2 && b.shotsInBlock() == 0,
              "resume(b): continue starts block 2 once with zero shots");
        b.resetTraining();
    }

    // (c) crash after TrainingCompleted → resume shows summary, rejects shots,
    //     appends no duplicate completion.
    QString sid3;
    {
        TrainingProgramController a;
        a.configureDefaults(QStringLiteral("AR10"));
        a.setBlockCount(1); a.setShotsPerBlock(3);
        a.setTechnicalFocus(QStringLiteral("Hold"));
        a.setOperatingMode(1);
        a.startTraining(QStringLiteral("C"));
        sid3 = a.sessionId();
        qint64 ext = 7500;
        for (int i = 0; i < 3; ++i) a.registerShot(1, 1, 10.0, ++ext, 0, 1);
        a.continueToNextBlock();                      // TrainingCompleted
    }                                                 // crash before Home
    {
        TrainingProgramController b;
        b.setOperatingMode(1);
        check(b.resumeFromRecovery(sid3), "resume(c): completed-session resume succeeds");
        check(b.phase() == 4, "resume(c): summary restored (no active block)");
        check(!b.registerShot(1, 1, 10.0, 9500, 0, 1),
              "resume(c): completed session accepts no shots");
        const ValidationReport rep =
            JournalValidator::validateFile(b.store()->currentJournalPath());
        int tc = 0;
        for (const EventEnvelope& e : rep.validEnvelopes)
            if (qstrcmp(eventTypeId(e.payload), "TrainingCompleted") == 0) ++tc;
        check(tc == 1, "resume(c): no duplicate TrainingCompleted");
        b.resetTraining();
    }
    StoragePaths::setRootOverrideForTesting(QString());
}

// ── 7. separation ────────────────────────────────────────────────────────
static void testSeparation()
{
    std::printf("--- competition separation ---\n");
    TrainingProgramController c;
    MemFile f; ManualClock clk;
    prepare(c, f, clk, "AR10", 1);
    c.setBlockCount(1); c.setShotsPerBlock(3);
    c.startTraining(QStringLiteral("A"));
    fire(c, clk, 3);
    c.continueToNextBlock();                    // completes training
    const ValidationReport rep = JournalValidator::validateBytes(f.data);
    const ReplayResult rr = ReplayEngine::replay(rep.validEnvelopes);
    check(countType(f.data, "ShotAccepted") == 0,
          "Training shots are never competition ShotAccepted events");
    check(countType(f.data, "SighterAccepted") == 0, "no sighter events either");
    check(countType(f.data, "MatchCompleted") == 0,
          "Training completion is not MatchCompleted");
    check(rr.state.officials.isEmpty() && rr.state.sighters.isEmpty(),
          "replayed state has no official/sighter competition record");
    check(rr.state.sessionKind == QLatin1String("Training"),
          "kind stays Training end-to-end");
    check(rr.state.trainingCompleted, "training completion recorded distinctly");
    c.resetTraining();
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::printf("=== Training Lab T1 acceptance ===\n");
    testConfiguration();
    testShotAcceptance();
    testVisibility();
    testLifecycle();
    testEarlyEnd();
    testAnalytics();
    testRecovery();
    testInPlaceResume();
    testSeparation();
    std::printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
