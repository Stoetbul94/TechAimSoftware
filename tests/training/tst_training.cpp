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

// T1.3: start training AND leave the (now separate) Sighters phase into the
// first counted block — the state every pre-T1.3 test assumed after start.
bool startCounted(TrainingProgramController& c, const QString& athlete)
{
    if (!c.startTraining(athlete)) return false;
    return c.startBlock();
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
        startCounted(c, QStringLiteral("A"));
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
        startCounted(c, QStringLiteral("A"));
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
        startCounted(c, QStringLiteral("A"));
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
        startCounted(c, QStringLiteral("A"));
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
        startCounted(c, QStringLiteral("A"));
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
        startCounted(c, QStringLiteral("A"));
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
    startCounted(c, QStringLiteral("A"));
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
    startCounted(c, QStringLiteral("A"));
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
        startCounted(c, QStringLiteral("A"));
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
        check(a.startBlock(), "resume(a): start block 1 out of Sighters");
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
        a.startBlock();
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

    // (c) crash after TrainingCompleted → the coordinator recognises it as
    //     complete (via TrainingCompleted, not MatchCompleted), auto-archives
    //     it, and NEVER offers it as an unfinished resume. This is the fix for
    //     the recurring "session still running" prompt after finishing.
    QString sid3;
    {
        TrainingProgramController a;
        a.configureDefaults(QStringLiteral("AR10"));
        a.setBlockCount(1); a.setShotsPerBlock(3);
        a.setTechnicalFocus(QStringLiteral("Hold"));
        a.setOperatingMode(1);
        a.startTraining(QStringLiteral("C"));
        a.startBlock();
        sid3 = a.sessionId();
        qint64 ext = 7500;
        for (int i = 0; i < 3; ++i) a.registerShot(1, 1, 10.0, ++ext, 0, 1);
        a.continueToNextBlock();                      // TrainingCompleted; not closed
    }                                                 // crash before Home
    {
        RecoveryCoordinator coord;
        bool offered = false;
        for (const RecoveryCandidate& cand : coord.scan())
            if (cand.sessionId == sid3 && cand.resumable) offered = true;
        check(!offered, "resume(c): completed training NOT offered as unfinished resume");
        // A completed (now-archived) session is not re-opened as an active one.
        TrainingProgramController b;
        b.setOperatingMode(1);
        check(!b.resumeFromRecovery(sid3),
              "resume(c): a completed session is not resumed as active");
    }

    StoragePaths::setRootOverrideForTesting(QString());
}

// ── 6c. T1.1 start-failure matrix + Demo operator journey ────────────────
static void testStartMatrixAndJourney()
{
    std::printf("--- start-failure matrix + demo journey ---\n");
    // Distinct, user-facing reasons (never internal enums, never generic).
    {
        TrainingProgramController c;
        MemFile f; ManualClock clk;
        c.storeForTesting()->setClockForTesting(&clk);
        c.storeForTesting()->setJournalFileForTesting(&f);
        c.setOperatingMode(1);
        check(!c.startTraining(QString()), "matrix: empty athlete refused");
        check(c.lastStartError().contains(QLatin1String("athlete")),
              "matrix: athlete reason is specific");
        check(!c.startTraining(QStringLiteral("A")), "matrix: no programme refused");
        check(c.lastStartError().contains(QLatin1String("programme")),
              "matrix: programme reason is specific");
        c.configureDefaults(QStringLiteral("AR10"));
        c.setBlockCount(0);
        check(!c.startTraining(QStringLiteral("A")), "matrix: invalid blocks refused");
        check(c.lastStartError().contains(QLatin1String("Blocks")),
              "matrix: block-count reason is specific");
        c.setBlockCount(5); c.setShotsPerBlock(2);
        check(!c.startTraining(QStringLiteral("A")), "matrix: invalid shots refused");
        check(c.lastStartError().contains(QLatin1String("Shots per block")),
              "matrix: shots reason is specific");
        c.setShotsPerBlock(6);
        c.setTechnicalFocus(QStringLiteral("Trigger"));
        // Demo with NO physical connection: must succeed (no COM gate exists).
        check(c.startTraining(QStringLiteral("Philemon")),
              "matrix: Demo start succeeds with no physical connection");
        check(c.lastStartError().isEmpty(), "matrix: success clears the error");
        // Stale-session self-heal — the reported defect: an open session left
        // behind by a legacy exit no longer blocks Start; it is closed cleanly
        // and a NEW session begins with a new id.
        const QString sid1 = c.sessionId();
        check(c.startTraining(QStringLiteral("Philemon")),
              "matrix: start over a stale open session self-heals");
        check(c.sessionId() != sid1 && !c.sessionId().isEmpty(),
              "matrix: self-heal produced a NEW session id");
        c.resetTraining();
    }
    // Reported screenshot path, end to end: PRONE50, NPA focus, Mode C,
    // Demo, no connection — 2x3 journey through summary/new-session reset.
    {
        TrainingProgramController c;
        MemFile f; ManualClock clk;
        c.storeForTesting()->setClockForTesting(&clk);
        c.storeForTesting()->setJournalFileForTesting(&f);
        c.setOperatingMode(1);
        c.configureDefaults(QStringLiteral("PRONE50"));
        c.setTechnicalFocus(QStringLiteral("Natural point of aim"));
        c.setVisibilityMode(2);                       // Impact only
        c.setBlockCount(2); c.setShotsPerBlock(3);    // shortened valid setup
        check(c.validateConfig().isEmpty(), "journey: shortened setup valid");
        check(c.startTraining(QStringLiteral("Philemon")),
              "journey: PRONE50 Demo start succeeds (screenshot path)");
        check(c.startBlock(), "journey: start block 1 out of Sighters");
        check(fire(c, clk, 3) == 3, "journey: three simulated shots accepted");
        check(c.phase() == 3, "journey: review opens at block limit");
        check(c.saveNote(QStringLiteral("ok")), "journey: note saved");
        check(c.continueToNextBlock() && c.currentBlock() == 2,
              "journey: continue starts block 2");
        fire(c, clk, 3);
        check(c.continueToNextBlock() && c.phase() == 4,
              "journey: final summary after last block");
        const QString oldSid = c.sessionId();
        c.resetTraining();                            // Home / New Session path
        check(!c.active(), "journey: home closes the session durably");
        check(countType(f.data, "TrainingCompleted") == 1,
              "journey: journal intact after home (1 completion)");
        // New Session: fresh id, zero state
        c.configureDefaults(QStringLiteral("PRONE50"));
        c.setTechnicalFocus(QStringLiteral("Trigger"));
        c.setBlockCount(2); c.setShotsPerBlock(3);
        MemFile f2; c.storeForTesting()->setJournalFileForTesting(&f2);
        check(c.startTraining(QStringLiteral("Philemon"))
                  && c.sessionId() != oldSid && c.shotsInBlock() == 0,
              "journey: new session starts fresh with new id, zero shots");
        c.resetTraining();
    }
}

// ── 7. separation ────────────────────────────────────────────────────────
static void testSeparation()
{
    std::printf("--- competition separation ---\n");
    TrainingProgramController c;
    MemFile f; ManualClock clk;
    prepare(c, f, clk, "AR10", 1);
    c.setBlockCount(1); c.setShotsPerBlock(3);
    startCounted(c, QStringLiteral("A"));
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

// ── 8. T1.3 sighter separation ───────────────────────────────────────────
static void testSighters()
{
    std::printf("--- T1.3 sighters separated from counted blocks ---\n");

    // Single-position: starts in Sighters; sighters measured, NEVER counted.
    {
        TrainingProgramController c; MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", /*Demo*/1);
        check(c.startTraining(QStringLiteral("A")), "single: training starts");
        check(c.phase() == 1 && c.inSighters(), "single: opens in Sighters phase");
        check(c.pendingBlock() == 1, "single: pending counted block is 1");
        check(c.startBlockLabel() == QLatin1String("Start Block 1"),
              "single: primary action labelled 'Start Block 1'");
        check(countType(f.data, "TrainingBlockStarted") == 0,
              "single: NO block started during Sighters");
        // a sighter shot
        clk.advance(3000);
        check(c.registerShot(2, 2, 10.3, 1, 0.0, 1), "single: sighter shot accepted");
        check(c.sighterCount() == 1, "single: sighter count increments");
        check(c.shotsInBlock() == 0 && c.totalShots() == 0,
              "single: counted block/total progress stays 0");
        check(countType(f.data, "TrainingSighterAccepted") == 1,
              "single: journalled as TrainingSighterAccepted");
        check(countType(f.data, "TrainingShotAccepted") == 0,
              "single: sighter creates NO counted TrainingShotAccepted");
        // source gate still applies to sighters
        check(!c.registerShot(2, 2, 10.0, 2, 0.0, /*Physical*/0),
              "single: wrong-source sighter rejected under Demo gate");
        check(c.sighterCount() == 1, "single: rejected-source sighter not recorded");
        // unlimited sighters
        clk.advance(3000); c.registerShot(1, 1, 9.9, 3, 0.0, 1);
        clk.advance(3000); c.registerShot(0, 1, 9.5, 4, 0.0, 1);
        check(c.sighterCount() == 3, "single: sighters may be unlimited");
        check(c.blockReviewMetrics(1).isEmpty(),
              "single: sighters produce no block metrics");
        // Start Block 1 — one BlockStarted, phase BlockActive, counted reset 0
        check(c.startBlock(), "single: Start Block 1 accepted");
        check(c.phase() == 2, "single: phase -> BlockActive");
        check(countType(f.data, "TrainingBlockStarted") == 1,
              "single: exactly one TrainingBlockStarted");
        check(c.shotsInBlock() == 0, "single: visible counted progress reset to 0/N");
        // repeated Start Block rejected (no double start)
        check(!c.startBlock(), "single: repeated Start Block rejected");
        check(countType(f.data, "TrainingBlockStarted") == 1,
              "single: no duplicate TrainingBlockStarted on repeat");
        // next shot is counted Shot 1
        clk.advance(3000);
        check(c.registerShot(1, 0, 10.4, 10, 0.0, 1), "single: first counted shot accepted");
        check(c.shotsInBlock() == 1, "single: first counted shot is Shot 1 of N");
        check(countType(f.data, "TrainingShotAccepted") == 1,
              "single: exactly one counted shot event");
        check(c.sighterCount() == 3, "single: no sighter copied into Block 1");
        c.resetTraining();
    }

    // Zero-sighter Start Block allowed (sighters optional, no minimum).
    {
        TrainingProgramController c; MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", 1);
        c.startTraining(QStringLiteral("Z"));
        check(c.sighterCount() == 0, "zero: no sighters fired");
        check(c.startBlock(), "zero: Start Block allowed with zero sighters");
        check(c.phase() == 2, "zero: block active with no sighters");
        c.resetTraining();
    }

    // Metrics + comparison exclude sighters end-to-end. Off-centre sighters
    // would wreck MPI/group if they leaked into the counted block.
    {
        TrainingProgramController c; MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", 1);
        c.setBlockCount(1); c.setShotsPerBlock(3);
        c.startTraining(QStringLiteral("M"));
        clk.advance(2000); c.registerShot(40, 40, 1.0, 1, 0.0, 1);   // wild sighters
        clk.advance(2000); c.registerShot(-40, -40, 1.0, 2, 0.0, 1);
        check(c.startBlock(), "exclude: block starts after wild sighters");
        fire(c, clk, 3, 10.0, 0.5, 0.5);                             // tight counted group
        check(c.phase() == 3, "exclude: counted block completes");
        const QVariantMap m = c.blockReviewMetrics(1);
        check(m.value(QStringLiteral("shotCount")).toInt() == 3,
              "exclude: block metrics count only the 3 counted shots");
        check(qAbs(m.value(QStringLiteral("mpiX")).toDouble()) < 5.0
                  && qAbs(m.value(QStringLiteral("mpiY")).toDouble()) < 5.0,
              "exclude: MPI reflects counted shots only (sighters excluded)");
        check(c.blockShotPlot(1).size() == 3,
              "exclude: Block Review plot contains only counted shots");
        const QVariantMap au = c.sighterAudit();
        check(au.value(QStringLiteral("total")).toInt() == 2,
              "exclude: sighter audit still records the 2 sighters (audit only)");
        c.resetTraining();
    }

    // Visibility: sighters visible in every mode; the hidden block begins after
    // the sighter phase (controller-observable part of 'begins clean').
    {
        TrainingProgramController c; MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", 1);
        c.setVisibilityMode(0);                       // Full hidden
        c.startTraining(QStringLiteral("V"));
        check(c.showImpacts() && c.showScores(),
              "vis: sighters fully visible before a Full-Hidden block");
        clk.advance(2000); c.registerShot(1, 1, 10.0, 1, 0.0, 1);
        check(c.startBlock(), "vis: Full-Hidden block starts");
        check(!c.showImpacts() && !c.showScores() && !c.showGroup(),
              "vis: Full-Hidden counted block begins hidden/clean");
        check(c.shotsInBlock() == 0, "vis: counted block begins at 0 (clean)");
        c.resetTraining();
    }
    {
        TrainingProgramController c; MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", 1);
        c.setVisibilityMode(1);                       // Group only
        c.startTraining(QStringLiteral("G"));
        c.registerShot(1, 1, 10.0, 1, 0.0, 1);
        check(c.startBlock() && c.showGroup() && !c.showScores(),
              "vis: Group-Only block shows group, no numbers");
        check(c.shotsInBlock() == 0, "vis: Group-Only block begins clean (0 counted)");
        c.resetTraining();
    }

    // 3P: separate sighter phase before the FIRST block of each position, in
    // order; each position's sighters are isolated; analytics exclude sighters.
    {
        TrainingProgramController c; MemFile f; ManualClock clk;
        prepare(c, f, clk, "3P50", 1);
        c.startTraining(QStringLiteral("3"));
        check(c.phase() == 1 && c.positionName() == QLatin1String("Kneeling"),
              "3P: opens in Kneeling sighters");
        check(c.startBlockLabel() == QLatin1String("Start Kneeling Block 1"),
              "3P: Kneeling start label");
        // sighter externalId = -1: not part of the counted retransmission
        // sequence, so it never collides with fire()'s shared counter.
        clk.advance(2000); c.registerShot(5, 5, 9.0, -1, 0.0, 1);
        clk.advance(2000); c.registerShot(5, 4, 9.0, -1, 0.0, 1);
        check(c.sighterCount() == 2, "3P: Kneeling sighter count");
        c.startBlock();                               // K block 1
        fire(c, clk, 6);
        check(c.continueToNextBlock() && c.phase() == 2
                  && c.positionName() == QLatin1String("Kneeling"),
              "3P: Kneeling Block 2 starts directly (no mid-position sighters)");
        fire(c, clk, 6);
        check(c.phase() == 3, "3P: Kneeling Block 2 review");
        check(c.continueToNextBlock() && c.phase() == 1
                  && c.positionName() == QLatin1String("Prone"),
              "3P: Prone sighters open ONLY after both Kneeling blocks");
        check(c.startBlockLabel() == QLatin1String("Start Prone Block 1"),
              "3P: Prone start label");
        check(c.sighterCount() == 0,
              "3P: Prone sighter count fresh (Kneeling sighters separate)");
        check(countType(f.data, "TrainingSighterPhaseStarted") >= 1,
              "3P: sighter-phase transition journalled");
        clk.advance(2000); c.registerShot(3, 3, 9.0, -1, 0.0, 1);
        check(c.sighterCount() == 1, "3P: Prone sighter counted in Prone phase only");
        c.startBlock(); fire(c, clk, 6);
        c.continueToNextBlock(); fire(c, clk, 6);
        check(c.phase() == 3, "3P: Prone Block 2 review");
        check(c.continueToNextBlock() && c.phase() == 1
                  && c.positionName() == QLatin1String("Standing"),
              "3P: Standing sighters open ONLY after both Prone blocks");
        check(c.sighterCount() == 0, "3P: Standing sighters separate");
        clk.advance(2000); c.registerShot(2, 2, 9.0, -1, 0.0, 1);
        clk.advance(2000); c.registerShot(2, 1, 9.0, -1, 0.0, 1);
        c.startBlock(); fire(c, clk, 6);
        c.continueToNextBlock(); fire(c, clk, 6);
        check(c.continueToNextBlock() && c.phase() == 4, "3P: completes to summary");
        // per-position audit + every block holds only 6 counted shots
        const QVariantMap au = c.sighterAudit();
        check(au.value(QStringLiteral("kneeling")).toInt() == 2
                  && au.value(QStringLiteral("prone")).toInt() == 1
                  && au.value(QStringLiteral("standing")).toInt() == 2,
              "3P: sighter audit per position (K2 / P1 / S2)");
        check(au.value(QStringLiteral("total")).toInt() == 5, "3P: sighter audit total 5");
        bool allSix = true;
        for (int b = 1; b <= 6; ++b)
            if (c.blockShotPlot(b).size() != 6) allSix = false;
        check(allSix, "3P: every counted block holds exactly 6 shots (sighters excluded)");
        // replay separation: sighters durable, competition record empty
        const ValidationReport rep = JournalValidator::validateBytes(f.data);
        const ReplayResult rr = ReplayEngine::replay(rep.validEnvelopes);
        check(rr.state.trainingSighters.size() == 5, "3P: replay restores 5 durable sighters");
        check(rr.state.officials.isEmpty() && rr.state.sighters.isEmpty(),
              "3P: no competition official/sighter record from Training");
        int cShots = 0;
        for (const TrainingBlockData& bd : rr.state.trainingBlocks) cShots += bd.shots.size();
        check(cShots == 36, "3P: replay counted block shots total 36 (no sighter leak)");
        c.resetTraining();
    }

    // Ownership: exactly-once classification at the routing boundary.
    {
        TrainingProgramController c; MemFile f; ManualClock clk;
        prepare(c, f, clk, "AR10", 1);
        c.startTraining(QStringLiteral("O"));
        clk.advance(2000); c.registerShot(1, 1, 10.0, 1, 0.0, 1);   // sighter
        check(countType(f.data, "ShotAccepted") == 0
                  && countType(f.data, "SighterAccepted") == 0,
              "own: Training sighter creates NO qualification event");
        check(countType(f.data, "TrainingShotAccepted") == 0,
              "own: Training sighter creates NO counted block event");
        c.startBlock();
        clk.advance(2000); c.registerShot(1, 1, 10.0, 2, 0.0, 1);   // counted
        check(countType(f.data, "TrainingSighterAccepted") == 1,
              "own: counted block shot creates NO new sighter event");
        check(countType(f.data, "TrainingShotAccepted") == 1,
              "own: counted block shot creates exactly one counted event");
        c.resetTraining();
    }
    // Open Practice / no active Training: the controller owns classification and
    // refuses everything when it is not the active owner (routing gate parity).
    {
        TrainingProgramController c;
        c.setOperatingMode(1);
        check(!c.registerShot(1, 1, 10.0, 1, 0.0, 1),
              "own: no shot classified while Training inactive (Open Practice)");
        check(!c.startBlock(), "own: Start Block refused while Training inactive");
    }
}

// ── 8b. T1.3 sighter recovery (real journals) ────────────────────────────
static void testSighterRecovery()
{
    std::printf("--- T1.3 sighter recovery ---\n");
    const QString root = QDir::temp().filePath(
        QStringLiteral("techaim_t13_recover_%1").arg(QCoreApplication::applicationPid()));
    QDir(root).removeRecursively();
    StoragePaths::setRootOverrideForTesting(root);
    StoragePaths::initialize();

    // (a) crash mid-sighters → resume restores the Sighters phase, sighter
    //     count, config; Start Block works once; next shot is counted Shot 1.
    QString sid;
    {
        TrainingProgramController a;
        a.configureDefaults(QStringLiteral("AR10"));
        a.setTechnicalFocus(QStringLiteral("Aim"));
        a.setVisibilityMode(2);
        a.setOperatingMode(1);
        check(a.startTraining(QStringLiteral("A")), "recover(a): session starts");
        sid = a.sessionId();
        qint64 ext = 3000;
        a.registerShot(2, 2, 9.8, ++ext, 0, 1);
        a.registerShot(2, 3, 9.9, ++ext, 0, 1);
        a.registerShot(1, 2, 9.5, ++ext, 0, 1);       // 3 sighters, then crash
    }
    {
        TrainingProgramController b;
        b.setOperatingMode(1);
        check(b.resumeFromRecovery(sid), "recover(a): resume succeeds");
        check(b.phase() == 1 && b.inSighters(), "recover(a): restored INTO Sighters phase");
        check(b.sighterCount() == 3, "recover(a): sighter count restored (3)");
        check(b.shotsInBlock() == 0 && b.totalShots() == 0,
              "recover(a): no counted progress after sighter recovery");
        check(b.technicalFocus() == QLatin1String("Aim") && b.visibilityMode() == 2,
              "recover(a): focus + visibility restored");
        check(b.recoveredSighterShots().size() == 3,
              "recover(a): sighter markers exposed for the face restore");
        check(b.blockReviewMetrics(1).isEmpty(),
              "recover(a): sighters remain excluded from block metrics");
        check(!b.registerShot(2, 2, 9.0, 3001, 0, 1),
              "recover(a): pre-crash sighter externalId still refused (dup guard)");
        check(b.startBlock(), "recover(a): Start Block works once after recovery");
        check(!b.startBlock(), "recover(a): repeated Start Block still rejected");
        check(b.phase() == 2 && b.shotsInBlock() == 0,
              "recover(a): counted block active with zero counted shots");
        check(b.registerShot(0, 0, 10.5, 4000, 0, 1) && b.shotsInBlock() == 1,
              "recover(a): next shot becomes counted Shot 1");
        b.resetTraining();
    }

    // (b) crash AFTER Start Block but BEFORE the first counted shot → resume
    //     lands in BlockActive, zero counted, no duplicate BlockStarted.
    QString sid2;
    {
        TrainingProgramController a;
        a.configureDefaults(QStringLiteral("AR10"));
        a.setTechnicalFocus(QStringLiteral("Hold"));
        a.setOperatingMode(1);
        a.startTraining(QStringLiteral("B"));
        sid2 = a.sessionId();
        a.registerShot(2, 2, 9.0, 5000, 0, 1);        // one sighter
        a.startBlock();                               // durable BlockActive, no counted shot
    }
    {
        TrainingProgramController b;
        b.setOperatingMode(1);
        check(b.resumeFromRecovery(sid2), "recover(b): resume succeeds");
        check(b.phase() == 2, "recover(b): restored INTO BlockActive");
        check(b.currentBlock() == 1 && b.shotsInBlock() == 0,
              "recover(b): block 1 active with zero counted shots");
        const ValidationReport rep =
            JournalValidator::validateFile(b.store()->currentJournalPath());
        check(rep.firstInvalidLine == -1, "recover(b): hash chain valid");
        int starts = 0;
        for (const EventEnvelope& e : rep.validEnvelopes)
            if (qstrcmp(eventTypeId(e.payload), "TrainingBlockStarted") == 0) ++starts;
        check(starts == 1, "recover(b): no duplicate TrainingBlockStarted");
        check(b.registerShot(0, 0, 10.4, 6000, 0, 1) && b.shotsInBlock() == 1,
              "recover(b): next shot is counted Shot 1");
        b.resetTraining();
    }

    // (c) 3P: crash mid-Prone-sighters → resume restores Prone Sighters with
    //     only the Prone sighters visible (Kneeling sighters stay separate).
    QString sid3;
    {
        TrainingProgramController a;
        a.configureDefaults(QStringLiteral("3P50"));
        a.setTechnicalFocus(QStringLiteral("Trigger"));
        a.setOperatingMode(1);
        a.startTraining(QStringLiteral("C"));
        sid3 = a.sessionId();
        qint64 ext = 7000;
        a.registerShot(5, 5, 9.0, ++ext, 0, 1);       // K sighter
        a.startBlock();
        for (int i = 0; i < 6; ++i) a.registerShot(1, 1, 10.0, ++ext, 0, 1);
        a.continueToNextBlock();
        for (int i = 0; i < 6; ++i) a.registerShot(1, 1, 10.0, ++ext, 0, 1);
        a.continueToNextBlock();                      // -> Prone sighters
        a.registerShot(3, 3, 9.0, ++ext, 0, 1);
        a.registerShot(3, 2, 9.0, ++ext, 0, 1);       // 2 Prone sighters, crash
    }
    {
        TrainingProgramController b;
        b.setOperatingMode(1);
        check(b.resumeFromRecovery(sid3), "recover(c): 3P Prone-sighter resume succeeds");
        check(b.phase() == 1 && b.positionName() == QLatin1String("Prone"),
              "recover(c): restored INTO Prone Sighters");
        check(b.sighterCount() == 2,
              "recover(c): only Prone sighters counted in this phase");
        check(b.recoveredSighterShots().size() == 2,
              "recover(c): only Prone sighter markers restored (Kneeling separate)");
        check(b.startBlockLabel() == QLatin1String("Start Prone Block 1"),
              "recover(c): Prone start label restored");
        check(b.startBlock() && b.phase() == 2 && b.currentBlock() == 3,
              "recover(c): Start Block begins Prone Block 1 (block index 3)");
        b.resetTraining();
    }

    StoragePaths::setRootOverrideForTesting(QString());
}

// ── 9. T1.4 shot-counter semantics (Shot 0..N, no off-by-one) ────────────
static void testShotCounter()
{
    std::printf("--- T1.4 shot counter (shotsCompleted) ---\n");
    TrainingProgramController c; MemFile f; ManualClock clk;
    prepare(c, f, clk, "AR10", 1);
    c.setBlockCount(1); c.setShotsPerBlock(4);
    c.startTraining(QStringLiteral("A"));
    c.registerShot(1, 1, 10.0, -1, 0, 1);          // a sighter (must NOT count)
    check(c.startBlock(), "counter: block starts");
    check(c.shotsInBlock() == 0, "counter: block begins at 0 (before any counted shot)");
    fire(c, clk, 1);
    check(c.shotsInBlock() == 1, "counter: first accepted counted shot -> 1");
    fire(c, clk, 1);
    check(c.shotsInBlock() == 2, "counter: intermediate -> 2 (no off-by-one, no sighter)");
    fire(c, clk, 1);
    check(c.shotsInBlock() == 3, "counter: -> 3");
    fire(c, clk, 1);
    check(c.shotsInBlock() == 4, "counter: final accepted -> 4 of 4");
    check(c.phase() == 3, "counter: block completes at N");
    c.resetTraining();

    // recovery keeps the authoritative counted count (zero and partial).
    const QString root = QDir::temp().filePath(
        QStringLiteral("techaim_t14_counter_%1").arg(QCoreApplication::applicationPid()));
    QDir(root).removeRecursively();
    StoragePaths::setRootOverrideForTesting(root);
    StoragePaths::initialize();
    QString sZero, sPart;
    {
        TrainingProgramController a; a.configureDefaults(QStringLiteral("AR10"));
        a.setTechnicalFocus(QStringLiteral("Aim")); a.setOperatingMode(1);
        a.startTraining(QStringLiteral("Z")); sZero = a.sessionId();
        a.startBlock();                                // BlockActive, zero counted
    }
    {
        TrainingProgramController b; b.setOperatingMode(1);
        check(b.resumeFromRecovery(sZero) && b.shotsInBlock() == 0,
              "counter: recovery after Start Block (before Shot 1) restores 0 counted");
        b.resetTraining();
    }
    {
        TrainingProgramController a; a.configureDefaults(QStringLiteral("AR10"));
        a.setTechnicalFocus(QStringLiteral("Aim")); a.setOperatingMode(1);
        a.startTraining(QStringLiteral("P")); sPart = a.sessionId();
        a.registerShot(9, 9, 9.0, 910000, 0, 1);       // sighter (excluded)
        a.startBlock();
        qint64 e = 910100;
        a.registerShot(1, 1, 10.0, ++e, 0, 1);
        a.registerShot(1, 2, 10.0, ++e, 0, 1);         // 2 counted, then crash
    }
    {
        TrainingProgramController b; b.setOperatingMode(1);
        check(b.resumeFromRecovery(sPart) && b.shotsInBlock() == 2,
              "counter: recovery with partial block restores exactly 2 counted (no sighter)");
        b.resetTraining();
    }
    StoragePaths::setRootOverrideForTesting(QString());
}

// ── 10. T1.4 report/coaching data model (measured only, correct direction) ─
static void testReportModel()
{
    std::printf("--- T1.4 report model + observations ---\n");
    TrainingProgramController c; MemFile f; ManualClock clk;
    prepare(c, f, clk, "AR10", 1);
    c.setBlockCount(2); c.setShotsPerBlock(3);
    c.startTraining(QStringLiteral("A"));
    c.startBlock();
    // Block 1: tight group near centre, high scores.
    clk.advance(4000); c.registerShot(0.0, 0.0, 10.5, 1001, 0, 1);
    clk.advance(4000); c.registerShot(0.5, 0.0, 10.4, 1002, 0, 1);
    clk.advance(4000); c.registerShot(0.0, 0.5, 10.4, 1003, 0, 1);
    check(c.phase() == 3, "report: block 1 complete");
    c.continueToNextBlock();
    // Block 2: wider group, offset RIGHT and HIGH (+x,+y), lower scores.
    clk.advance(4000); c.registerShot(9.0, 9.0, 8.0, 1004, 0, 1);
    clk.advance(4000); c.registerShot(15.0, 9.0, 8.2, 1005, 0, 1);
    clk.advance(4000); c.registerShot(9.0, 15.0, 8.1, 1006, 0, 1);
    // block-2 delta vs block 1
    const QVariantMap d = c.blockDelta(2);
    check(d.value(QStringLiteral("hasPrev")).toBool(), "report: block 2 has a previous block");
    check(d.value(QStringLiteral("diameterDeltaMm")).toDouble() > 0.0,
          "report: block 2 group is larger than block 1 (positive diameter delta)");
    check(d.value(QStringLiteral("averageScoreDelta")).toDouble() < 0.0,
          "report: block 2 average score is lower (negative delta)");
    // observations use CORRECT direction wording (+x right, +y high)
    const QStringList obs = c.blockObservations(2);
    bool rightHigh = false;
    for (const QString& s : obs)
        if (s.contains(QStringLiteral("right")) && s.contains(QStringLiteral("high"))) rightHigh = true;
    check(rightHigh, "report: block-2 MPI observed as right AND high (+x,+y, Y not inverted)");
    c.continueToNextBlock();                       // complete
    check(c.phase() == 4, "report: session complete");
    // full report model
    const QVariantMap r = c.trainingReportModel();
    check(r.value(QStringLiteral("countedShots")).toInt() == 6, "report: 6 counted shots");
    check(r.value(QStringLiteral("completedBlocks")).toInt() == 2, "report: 2 completed blocks");
    check(r.value(QStringLiteral("blocks")).toList().size() == 2, "report: 2 block entries");
    check(!r.value(QStringLiteral("endedEarly")).toBool(), "report: not ended early");
    const QStringList sobs = c.sessionObservations();
    bool tightBest = false;
    for (const QString& s : sobs)
        if (s.contains(QStringLiteral("Block 1"))) tightBest = true;
    check(tightBest, "report: session observations name Block 1 (best/tightest)");
    // no diagnostic wording leaks into observations
    bool clean = true;
    const QStringList all = sobs + obs;
    for (const QString& s : all)
        if (s.contains(QStringLiteral("trigger"), Qt::CaseInsensitive)
            || s.contains(QStringLiteral("error"), Qt::CaseInsensitive)
            || s.contains(QStringLiteral("fault"), Qt::CaseInsensitive)) clean = false;
    check(clean, "report: observations contain no diagnostic-cause wording");
    c.resetTraining();
}

// ── 11. T1.4 clean close / stale-session defect ──────────────────────────
static void testCleanClose()
{
    std::printf("--- T1.4 clean close + recovery scan ---\n");
    const QString root = QDir::temp().filePath(
        QStringLiteral("techaim_t14_close_%1").arg(QCoreApplication::applicationPid()));
    QDir(root).removeRecursively();
    StoragePaths::setRootOverrideForTesting(root);
    StoragePaths::initialize();

    auto candidate = [](const QString& sid, bool* resumable)->bool {
        RecoveryCoordinator coord;
        for (const RecoveryCandidate& c : coord.scan())
            if (c.sessionId == sid) { if (resumable) *resumable = c.resumable; return true; }
        return false;
    };

    // (a) completed session → closeCleanly → NOT a candidate (archived).
    {
        TrainingProgramController a; a.configureDefaults(QStringLiteral("AR10"));
        a.setBlockCount(1); a.setShotsPerBlock(3);
        a.setTechnicalFocus(QStringLiteral("Hold")); a.setOperatingMode(1);
        a.startTraining(QStringLiteral("A")); const QString sid = a.sessionId();
        a.startBlock();
        qint64 e = 20000; for (int i = 0; i < 3; ++i) a.registerShot(1, 1, 10.0, ++e, 0, 1);
        a.continueToNextBlock();                       // completed (phase 4)
        check(a.phase() == 4, "close(a): session completed");
        check(a.closeCleanly(), "close(a): completed session closes cleanly");
        check(!candidate(sid, nullptr), "close(a): completed+closed leaves NO recovery candidate");
    }
    // (b) active block → Save and Close (closeCleanly) → NOT a candidate.
    {
        TrainingProgramController a; a.configureDefaults(QStringLiteral("AR10"));
        a.setTechnicalFocus(QStringLiteral("Aim")); a.setOperatingMode(1);
        a.startTraining(QStringLiteral("B")); const QString sid = a.sessionId();
        a.startBlock();
        qint64 e = 21000; a.registerShot(1, 1, 10.0, ++e, 0, 1);   // mid-block
        check(a.closeCleanly(), "close(b): active block Save&Close succeeds");
        check(!candidate(sid, nullptr), "close(b): Save&Close leaves NO candidate");
    }
    // (c) active block left OPEN (Keep for Recovery / hard kill) → candidate exists.
    {
        QString sid;
        {
            TrainingProgramController a; a.configureDefaults(QStringLiteral("AR10"));
            a.setTechnicalFocus(QStringLiteral("Aim")); a.setOperatingMode(1);
            a.startTraining(QStringLiteral("C")); sid = a.sessionId();
            a.startBlock();
            qint64 e = 22000; a.registerShot(1, 1, 10.0, ++e, 0, 1);
        }                                              // destroyed WITHOUT close
        bool resumable = false;
        check(candidate(sid, &resumable) && resumable,
              "close(c): unclosed active block IS a resumable recovery candidate");
        RecoveryCoordinator coord; coord.scan(); coord.archiveOrDiscard(sid, true);  // tidy
    }
    // (d) completed but NOT closed (crash after complete) → classified complete,
    //     auto-archived, never offered as "still active".
    {
        QString sid;
        {
            TrainingProgramController a; a.configureDefaults(QStringLiteral("AR10"));
            a.setBlockCount(1); a.setShotsPerBlock(3);
            a.setTechnicalFocus(QStringLiteral("Hold")); a.setOperatingMode(1);
            a.startTraining(QStringLiteral("D")); sid = a.sessionId();
            a.startBlock();
            qint64 e = 23000; for (int i = 0; i < 3; ++i) a.registerShot(1, 1, 10.0, ++e, 0, 1);
            a.continueToNextBlock();                   // completed, but NOT closed
        }                                              // destroyed WITHOUT close
        check(!candidate(sid, nullptr),
              "close(d): completed-but-unclosed classified complete, not an active candidate");
    }

    StoragePaths::setRootOverrideForTesting(QString());
}

// ── 12. T1.4 PDF report model (data feeding the Training PDF) ─────────────
static void runReportSession(const char* disc, int blocks, int spb,
                             bool withNote, bool earlyEnd, QVariantMap* out)
{
    TrainingProgramController c; MemFile f; ManualClock clk;
    c.storeForTesting()->setClockForTesting(&clk);
    c.storeForTesting()->setJournalFileForTesting(&f);
    c.configureDefaults(QString::fromLatin1(disc));
    c.setTechnicalFocus(QStringLiteral("Trigger"));
    if (qstrcmp(disc, "3P50") != 0)        // keep the 3P block shape intact
        { c.setBlockCount(blocks); c.setShotsPerBlock(spb); }
    c.setOperatingMode(1);
    c.startTraining(QStringLiteral("Rae"));
    c.startBlock();
    const int nblocks = c.blockCount();
    for (int b = 1; b <= nblocks; ++b) {
        for (int i = 0; i < c.shotsPerBlock(); ++i) {
            clk.advance(4000);
            c.registerShot(0.5 + b, 0.5, 10.0, 30000 + b * 100 + i, 0, 1);
        }
        if (withNote) c.saveNote(QStringLiteral("felt %1").arg(b));
        if (earlyEnd && b == 1) { c.endTrainingEarly(); break; }
        c.continueToNextBlock();
    }
    *out = c.trainingReportModel();
    c.resetTraining();
}

static void testPdfModel()
{
    std::printf("--- T1.4 PDF report model ---\n");
    struct Case { const char* disc; int b; int spb; };
    const Case cases[] = { {"AR10", 2, 3}, {"AP10", 2, 3}, {"PRONE50", 2, 3}, {"3P50", 6, 6} };
    for (const Case& cs : cases) {
        QVariantMap r; runReportSession(cs.disc, cs.b, cs.spb, true, false, &r);
        const QString tag = QString::fromLatin1(cs.disc);
        check(r.contains(QStringLiteral("athlete")) && r.value(QStringLiteral("athlete")).toString() == QLatin1String("Rae"),
              (tag + ": report model carries athlete").toUtf8().constData());
        check(!r.value(QStringLiteral("sessionId")).toString().isEmpty(),
              (tag + ": report model carries session id").toUtf8().constData());
        check(r.value(QStringLiteral("visibilityLabel")).toString().length() > 0,
              (tag + ": report model carries visibility label").toUtf8().constData());
        const QVariantList blocks = r.value(QStringLiteral("blocks")).toList();
        check(blocks.size() == r.value(QStringLiteral("completedBlocks")).toInt() && blocks.size() > 0,
              (tag + ": report blocks match completed count").toUtf8().constData());
        // every block entry has a note (we saved one) + a plot + measured fields
        bool blocksOk = true;
        for (const QVariant& bv : blocks) {
            const QVariantMap bm = bv.toMap();
            if (bm.value(QStringLiteral("note")).toString().isEmpty()) blocksOk = false;
            if (bm.value(QStringLiteral("plot")).toList().isEmpty()) blocksOk = false;
            if (!bm.contains(QStringLiteral("averageScore"))) blocksOk = false;
        }
        check(blocksOk, (tag + ": each block entry has note + plot + metrics").toUtf8().constData());
        // no official-competition wording anywhere in the observations
        bool cleanWording = true;
        const QStringList obs = r.value(QStringLiteral("observations")).toStringList();
        for (const QString& s : obs)
            if (s.contains(QStringLiteral("match"), Qt::CaseInsensitive)
                || s.contains(QStringLiteral("official"), Qt::CaseInsensitive)
                || s.contains(QStringLiteral("rank"), Qt::CaseInsensitive)) cleanWording = false;
        check(cleanWording, (tag + ": no competition/ranking wording in observations").toUtf8().constData());
    }
    // notes absent → block note empty, model still complete
    {
        QVariantMap r; runReportSession("AR10", 2, 3, /*withNote*/false, false, &r);
        const QVariantList blocks = r.value(QStringLiteral("blocks")).toList();
        check(blocks.size() == 2 && blocks[0].toMap().value(QStringLiteral("note")).toString().isEmpty(),
              "pdf: notes-absent session yields empty block notes, model intact");
    }
    // early-ended → flagged, only completed blocks reported
    {
        QVariantMap r; runReportSession("AR10", 3, 3, false, /*earlyEnd*/true, &r);
        check(r.value(QStringLiteral("endedEarly")).toBool(), "pdf: early-ended flagged in model");
        check(r.value(QStringLiteral("blocks")).toList().size() == 1,
              "pdf: early-ended reports only the completed block");
    }
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
    testStartMatrixAndJourney();
    testSeparation();
    testSighters();
    testSighterRecovery();
    testShotCounter();
    testReportModel();
    testCleanClose();
    testPdfModel();
    std::printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
    std::fflush(stdout);
    return g_failures == 0 ? 0 : 1;
}
