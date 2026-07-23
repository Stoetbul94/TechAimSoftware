#include "TrainingProgramController.h"

#include "mode/OperatingMode.h"
#include "GroupPatternAnalyzer.h"
#include "TargetGeometry.h"

#include <QCoreApplication>
#include <QDebug>
#include <QUuid>

using namespace ta::rel;
using namespace ta::training;

TrainingProgramController::TrainingProgramController(QObject* parent)
    : QObject(parent)
    , m_store(std::make_unique<SessionStore>())
{
    connect(m_store.get(), &SessionStore::journalWriteFailed,
            this, [this](QString path, QString detail) {
                emit journalWriteFailed(path, detail);
            });
}

TrainingProgramController::~TrainingProgramController() = default;

// ── configuration ────────────────────────────────────────────────────────

void TrainingProgramController::configureDefaults(const QString& disciplineId)
{
    Discipline d = Discipline::None;
    if (!disciplineFromId(disciplineId, &d)) {
        m_lastError = QStringLiteral("Unknown discipline '%1'.").arg(disciplineId);
        return;
    }
    m_cfg = TechnicalBlocksConfig::defaultsFor(d);
    m_lastError.clear();
    emit configChanged();
}

void TrainingProgramController::setBlockCount(int n)
{
    if (m_cfg.threePositions) {
        // 3P: keep the K/P/S split exact — n is blocks TOTAL, must divide by 3.
        if (n % 3 == 0) { m_cfg.blockCount = n; m_cfg.blocksPerPosition = n / 3; }
        else { m_lastError = QStringLiteral("3P block count must be a multiple of 3."); return; }
    } else {
        m_cfg.blockCount = n;
    }
    emit configChanged();
}

void TrainingProgramController::setShotsPerBlock(int n)
{
    m_cfg.shotsPerBlock = n;
    emit configChanged();
}

void TrainingProgramController::setVisibilityMode(int mode)
{
    m_cfg.visibility = static_cast<Visibility>(qBound(0, mode, 2));
    emit configChanged();
}

void TrainingProgramController::setTechnicalFocus(const QString& focus)
{
    m_cfg.technicalFocus = focus;
    emit configChanged();
}

QString TrainingProgramController::validateConfig() const
{
    return m_cfg.validate();
}

QStringList TrainingProgramController::focusOptionsForDiscipline() const
{
    return focusOptions(m_cfg.discipline == Discipline::AirPistol10m);
}

// ── lifecycle ────────────────────────────────────────────────────────────

bool TrainingProgramController::startTraining(const QString& athlete)
{
    // T1.1: every refusal sets a specific, actionable, user-facing reason in
    // m_lastStartError and logs a structured (non-private) diagnostic.
    auto refuse = [this](const QString& userMsg, const char* code) {
        m_lastStartError = userMsg;
        m_lastError = userMsg;
        qWarning().noquote()
            << "TRAINING start refused —" << code
            << "| mode" << m_operatingMode
            << "| discipline" << disciplineId(m_cfg.discipline)
            << "| cfg" << m_cfg.blockCount << "x" << m_cfg.shotsPerBlock
            << "| storeActive" << (m_store && m_store->active());
        emit startErrorChanged();
        return false;
    };
    if (athlete.trimmed().isEmpty())
        return refuse(QStringLiteral("No athlete has been selected.\n\n"
                          "Enter or choose an athlete name, then start again."),
                      "no-athlete");
    if (m_cfg.discipline == Discipline::None)
        return refuse(QStringLiteral("No programme has been configured.\n\n"
                          "Open Training Lab, choose Technical Blocks and confirm the setup."),
                      "no-programme");
    const QString err = m_cfg.validate();
    if (!err.isEmpty())
        return refuse(QStringLiteral("The Technical Blocks setup is not valid.\n\n%1").arg(err),
                      "invalid-config");
    // Stale session self-heal: this store belongs EXCLUSIVELY to Training, so
    // an active session here can only be an earlier Training session that was
    // left open (e.g. leaving via a legacy Home path). "Start training" states
    // the operator's intent — close the stale session cleanly (its journal is
    // preserved/archived) and start fresh, instead of failing cryptically.
    if (m_store->active()) {
        qInfo() << "TRAINING: closing stale open training session"
                << st().sessionId.left(8) << "before starting a new one";
        m_store->closeSession(CloseReason::Clean);
    }

    SessionHeader header;
    header.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    header.appVersion = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("dev") : QCoreApplication::applicationVersion();
    header.athlete = athlete.trimmed();
    header.matchType = QStringLiteral("TRAINING technical_blocks");
    header.discipline = m_cfg.discipline;
    // T1: distinct classification + operating mode captured at start.
    header.sessionKind = QStringLiteral("Training");
    if (m_operatingMode >= 0)
        header.operatingMode =
            ta::mode::modeToConfigString(static_cast<ta::mode::Mode>(m_operatingMode));
    header.config.officialShots = 0;     // NEVER an official competition record

    const ReliabilityResult r = m_store->beginSession(header);
    if (!r.ok)
        return refuse(QStringLiteral("The training session journal could not be created.\n\n%1")
                          .arg(r.error.operatorMessage),
                      "journal-open-failed");
    m_lastStartError.clear();
    emit startErrorChanged();

    TrainingSessionStarted ev;
    ev.programId = QLatin1String(kProgramTechnicalBlocks);
    ev.discipline = m_cfg.discipline;
    ev.blockCount = static_cast<qint16>(m_cfg.blockCount);
    ev.shotsPerBlock = static_cast<qint16>(m_cfg.shotsPerBlock);
    ev.visibilityMode = static_cast<qint8>(m_cfg.visibility);
    ev.technicalFocus = m_cfg.technicalFocus;
    ev.startPosition = static_cast<qint8>(m_cfg.positionForBlock(1));
    if (!submit(DomainEvent(ev))) return false;

    // T1.3: a Training session opens in the SIGHTERS phase before Block 1 —
    // NOT counted. TrainingSessionStarted already put the reducer in the
    // sighter phase; the block starts only on an explicit Start Block action.
    m_currentBlock = 0;                   // no counted block started yet
    m_lastExternalId = -1;
    m_blockStartMonoMs = m_store->nowMonotonicMs();
    m_phase = 1;                          // Sighters
    emit phaseChanged(); emit progressChanged();
    return true;
}

bool TrainingProgramController::startBlock()
{
    if (m_phase != 1) {
        m_lastError = QStringLiteral("Sighters are not active.");
        return false;
    }
    const int b = pendingBlock();
    if (b < 1 || b > m_cfg.blockCount) {
        m_lastError = QStringLiteral("No block to start.");
        return false;
    }
    TrainingBlockStarted ev;
    ev.blockIndex = static_cast<qint16>(b);
    ev.position = static_cast<qint8>(m_cfg.positionForBlock(b));
    if (!submit(DomainEvent(ev))) return false;   // repeated tap: reducer state
    m_currentBlock = b;                            //   already moved on -> phase
    m_blockStartMonoMs = m_store->nowMonotonicMs();//   guard below prevents dup
    m_phase = 2;                                   // BlockActive
    emit sightersCleared();                        // UI clears all sighter markers
    emit phaseChanged(); emit progressChanged();
    return true;
}

bool TrainingProgramController::registerShot(double xMm, double yMm,
                                             double decimalScore,
                                             qint64 externalId,
                                             double directionDeg,
                                             int shotSource)
{
    // F10 authoritative input-source gate — BEFORE any durable acceptance.
    if (m_operatingMode >= 0
        && !ta::mode::sourceAllowed(static_cast<ta::mode::Mode>(m_operatingMode),
                                    static_cast<ta::mode::ShotSource>(shotSource))) {
        emit shotRejected(QStringLiteral("WrongInputSource"));
        return false;
    }
    if (!m_store->active()
        || (m_phase != 1 && m_phase != 2)) {   // only Sighters or BlockActive score
        emit shotRejected(QStringLiteral("TrainingNotActive"));
        return false;
    }
    if (!(decimalScore >= 0.0) || decimalScore > 11.0
        || !(xMm > -500.0 && xMm < 500.0) || !(yMm > -500.0 && yMm < 500.0)) {
        emit shotRejected(QStringLiteral("InvalidShotData"));
        return false;
    }
    // duplicate/retransmission guard (shared externalId sequence across sighters
    // and counted shots — both classify exactly once).
    if (externalId >= 0 && externalId <= m_lastExternalId) {
        emit shotRejected(QStringLiteral("DuplicateShot"));
        return false;
    }
    // T1.3 — THE classification boundary (TrainingProgramController owns it):
    // in the Sighters phase every shot is a Training sighter (measured, shown,
    // NEVER counted); in BlockActive it is a counted block shot. Never both.
    if (m_phase == 1)
        return acceptSighter(xMm, yMm, decimalScore, externalId, directionDeg, shotSource);
    // block cap — no spill into the next block, ever
    const int within = shotsInBlock();
    if (within >= m_cfg.shotsPerBlock) {
        emit shotRejected(QStringLiteral("BlockFull"));
        return false;
    }

    TrainingShotAccepted ev;
    ev.blockIndex = static_cast<qint16>(m_currentBlock);
    ev.withinBlock = static_cast<qint16>(within + 1);
    ev.position = static_cast<qint8>(m_cfg.positionForBlock(m_currentBlock));
    ShotCore& s = ev.shot;
    s.shotNumber = static_cast<qint16>(totalShots() + 1);
    s.withinStage = ev.withinBlock;
    s.stageId = ev.blockIndex;
    s.xHundredthMm = static_cast<qint32>(qRound(xMm * 100.0));
    s.yHundredthMm = static_cast<qint32>(qRound(yMm * 100.0));
    s.scoreTenths = static_cast<qint16>(qBound<int>(0, qRound(decimalScore * 10.0), 110));
    s.directionCentiDeg = static_cast<qint32>(qRound(directionDeg * 100.0));
    s.splitMs = static_cast<qint32>(m_store->nowMonotonicMs() > 0
        ? qMin<qint64>(m_store->nowMonotonicMs(), INT32_MAX) : 0);
    s.externalId = externalId;
    s.simulated = (shotSource == 1);

    if (!submit(DomainEvent(ev)))
        return false;
    if (externalId >= 0)
        m_lastExternalId = externalId;

    // Visibility-safe projection: coordinates only in modes that show impacts;
    // numerical score never during a live block (revealed at block completion).
    QVariantMap rec;
    rec[QStringLiteral("withinBlock")] = ev.withinBlock;
    rec[QStringLiteral("blockIndex")] = ev.blockIndex;
    if (showImpacts()) {
        rec[QStringLiteral("xMm")] = xMm;
        rec[QStringLiteral("yMm")] = yMm;
    }
    emit shotAccepted(rec);

    // Block complete? Exactly one TrainingBlockCompleted, then Block Review.
    if (shotsInBlock() >= m_cfg.shotsPerBlock) {
        TrainingBlockCompleted done;
        done.blockIndex = static_cast<qint16>(m_currentBlock);
        done.shotCount = static_cast<qint16>(m_cfg.shotsPerBlock);
        if (submit(DomainEvent(done))) {
            m_phase = 3;                  // BlockReview (never auto-dismissed)
            emit blockCompleted(m_currentBlock);
            emit phaseChanged();
        }
    }
    emit progressChanged();
    return true;
}

// T1.3: a Training sighter — measured and shown, but NEVER counted. Journalled
// as TrainingSighterAccepted (excluded from every block/metric/total); scoped
// to the current sighter phase's position so 3P positions stay separate.
bool TrainingProgramController::acceptSighter(double xMm, double yMm,
                                              double decimalScore, qint64 externalId,
                                              double directionDeg, int shotSource)
{
    const int pos = static_cast<int>(m_cfg.positionForBlock(pendingBlock()));
    TrainingSighterAccepted ev;
    ev.position = static_cast<qint8>(pos);
    ev.beforeBlock = static_cast<qint16>(pendingBlock());
    ShotCore& s = ev.shot;
    s.shotNumber = 0;                    // 0 = sighter marker (never a block index)
    s.xHundredthMm = static_cast<qint32>(qRound(xMm * 100.0));
    s.yHundredthMm = static_cast<qint32>(qRound(yMm * 100.0));
    s.scoreTenths = static_cast<qint16>(qBound<int>(0, qRound(decimalScore * 10.0), 110));
    s.directionCentiDeg = static_cast<qint32>(qRound(directionDeg * 100.0));
    s.splitMs = static_cast<qint32>(m_store->nowMonotonicMs() > 0
        ? qMin<qint64>(m_store->nowMonotonicMs(), INT32_MAX) : 0);
    s.externalId = externalId;
    s.simulated = (shotSource == 1);
    if (!submit(DomainEvent(ev)))
        return false;
    if (externalId >= 0)
        m_lastExternalId = externalId;
    // Sighters may always show their impact + score (they are not part of any
    // hidden block); the visibility modes apply to counted blocks only.
    QVariantMap rec;
    rec[QStringLiteral("xMm")] = xMm;
    rec[QStringLiteral("yMm")] = yMm;
    rec[QStringLiteral("score")] = decimalScore;
    rec[QStringLiteral("sighter")] = true;
    emit sighterAccepted(rec);
    emit progressChanged();
    return true;
}

bool TrainingProgramController::saveNote(const QString& note)
{
    if (m_phase != 3 && m_phase != 4) {
        m_lastError = QStringLiteral("Notes are saved from the block review.");
        return false;
    }
    TrainingNoteSaved ev;
    ev.blockIndex = static_cast<qint16>(m_currentBlock);
    ev.note = note;
    return submit(DomainEvent(ev));
}

bool TrainingProgramController::continueToNextBlock()
{
    if (m_phase != 3) { m_lastError = QStringLiteral("No block review to continue from."); return false; }

    if (m_currentBlock >= m_cfg.blockCount) {
        // final block reviewed → exactly one TrainingCompleted
        TrainingCompleted ev;
        ev.completedBlocks = static_cast<qint16>(m_cfg.blockCount);
        if (!submit(DomainEvent(ev))) return false;
        m_phase = 4;                      // Complete (final comparison)
        emit trainingCompleted();
        emit phaseChanged(); emit progressChanged();
        return true;
    }
    const int next = m_currentBlock + 1;
    // T1.3 — 3P position boundary: before the FIRST block of a new position,
    // enter a separate SIGHTERS phase for that position (Kneeling → Prone →
    // Standing). Ordinary blocks within a position never reopen sighters.
    if (m_cfg.threePositions
        && m_cfg.positionForBlock(next) != m_cfg.positionForBlock(m_currentBlock)) {
        TrainingSighterPhaseStarted sp;
        sp.position = static_cast<qint8>(m_cfg.positionForBlock(next));
        sp.beforeBlock = static_cast<qint16>(next);
        if (!submit(DomainEvent(sp))) return false;
        // m_currentBlock stays at the completed block; pendingBlock() == next.
        m_phase = 1;                      // Sighters (new position)
        emit sightersCleared();           // clear the previous position's markers
        emit phaseChanged(); emit progressChanged();
        return true;
    }
    // Same position / non-3P: start the next block directly (no sighters).
    TrainingBlockStarted b;
    b.blockIndex = static_cast<qint16>(next);
    b.position = static_cast<qint8>(m_cfg.positionForBlock(next));
    if (!submit(DomainEvent(b))) return false;
    ++m_currentBlock;
    m_blockStartMonoMs = m_store->nowMonotonicMs();
    m_phase = 2;
    emit sightersCleared();               // clear the prior block's markers
    emit phaseChanged(); emit progressChanged();
    return true;
}

bool TrainingProgramController::endTrainingEarly()
{
    if (m_phase != 3) { m_lastError = QStringLiteral("End training from the block review."); return false; }
    // Count the blocks that ACTUALLY completed — never fabricate the rest.
    int completed = 0;
    for (const TrainingBlockData& b : st().trainingBlocks)
        if (b.completed) ++completed;
    TrainingCompleted ev;
    ev.completedBlocks = static_cast<qint16>(completed);
    if (!submit(DomainEvent(ev))) return false;
    m_phase = 4;
    emit trainingCompleted();
    emit phaseChanged(); emit progressChanged();
    return true;
}

bool TrainingProgramController::endedEarly() const
{
    if (!m_store || !st().trainingCompleted) return false;
    int completed = 0;
    for (const TrainingBlockData& b : st().trainingBlocks)
        if (b.completed) ++completed;
    return completed < st().trainingBlockCount;
}

int TrainingProgramController::blockElapsedSec() const
{
    if (m_phase != 2 && m_phase != 3) return 0;
    const qint64 now = m_store ? m_store->nowMonotonicMs() : 0;
    return now > m_blockStartMonoMs
        ? static_cast<int>((now - m_blockStartMonoMs) / 1000) : 0;
}

void TrainingProgramController::closeTrainingSession()
{
    closeCleanly();
}

// T1.4 clean-close contract: durably close the Training session (SessionClosed
// + CleanShutdown + archive out of Current, via SessionStore) AND clear the
// controller projections. Returns false with lastError set when the durable
// close fails — the caller must NOT navigate away (keep the journal recoverable
// and offer Retry). Idempotent: closing when already inactive succeeds.
bool TrainingProgramController::closeCleanly()
{
    if (m_store && m_store->active()) {
        const ReliabilityResult rr = m_store->closeSession(CloseReason::Clean);
        if (!rr.ok) {
            m_lastError = rr.error.operatorMessage.isEmpty()
                ? QStringLiteral("The training session could not be closed safely.")
                : rr.error.operatorMessage;
            emit journalWriteFailed(rr.error.affectedPath, rr.error.technicalDetail);
            return false;                       // stay put — recoverable, retriable
        }
    }
    m_phase = 0;
    m_currentBlock = 0;
    m_lastExternalId = -1;
    emit phaseChanged(); emit progressChanged();
    return true;
}

void TrainingProgramController::resetTraining()
{
    closeCleanly();                             // best-effort; see closeCleanly()
}

// ── projections ──────────────────────────────────────────────────────────

int TrainingProgramController::shotsInBlock() const
{
    if (!m_store) return 0;
    for (const TrainingBlockData& b : st().trainingBlocks)
        if (b.blockIndex == m_currentBlock) return b.shots.size();
    return 0;
}

int TrainingProgramController::totalShots() const
{
    if (!m_store) return 0;
    int n = 0;
    for (const TrainingBlockData& b : st().trainingBlocks) n += b.shots.size();
    return n;
}

QString TrainingProgramController::positionName() const
{
    if (!m_cfg.threePositions) return QString();
    // In the Sighters phase the "current" position is the one whose block is
    // pending (m_currentBlock still points at the previous/last block).
    const int b = (m_phase == 1) ? pendingBlock() : qMax(1, m_currentBlock);
    return positionLabel(m_cfg.positionForBlock(b));
}

// T1.3 — the 1-based counted block that startBlock()/continueToNextBlock()
// will begin next (during Sighters this is m_currentBlock + 1).
int TrainingProgramController::pendingBlock() const
{
    return qBound(1, m_currentBlock + 1, qMax(1, m_cfg.blockCount));
}

// Sighters fired in the CURRENT phase only. Each 3P position owns exactly one
// sighter phase, so counting by that phase's position keeps positions separate;
// non-3P sessions have a single phase (position 0) and count all sighters.
int TrainingProgramController::sighterCount() const
{
    if (!m_store) return 0;
    const int pos = static_cast<int>(m_cfg.positionForBlock(pendingBlock()));
    const QVector<ShotCore>& s = st().trainingSighters;
    const QVector<qint8>&    p = st().trainingSighterPos;
    int n = 0;
    for (int i = 0; i < s.size(); ++i)
        if (i >= p.size() || static_cast<int>(p[i]) == pos) ++n;
    return n;
}

// Primary Sighters action label. Never "Start Match" — always "Start Block N",
// and per-position for 3P ("Start Kneeling Block 1", etc.).
QString TrainingProgramController::startBlockLabel() const
{
    const int b = pendingBlock();
    if (!m_cfg.threePositions)
        return QStringLiteral("Start Block %1").arg(b);
    const int within = m_cfg.blocksPerPosition > 0
        ? ((b - 1) % m_cfg.blocksPerPosition) + 1 : b;
    return QStringLiteral("Start %1 Block %2")
        .arg(positionLabel(m_cfg.positionForBlock(b))).arg(within);
}

QString TrainingProgramController::sessionId() const
{
    return m_store ? st().sessionId : QString();
}

QString TrainingProgramController::sessionOperatingMode() const
{
    return m_store ? st().operatingMode : QString();
}

// Visibility projections. During a live block (phase 2) the mode gates what
// the athlete may see; review/comparison phases always reveal measured data.
bool TrainingProgramController::showImpacts() const
{
    if (m_phase != 2) return true;
    return m_cfg.visibility != Visibility::FullHidden;
}
bool TrainingProgramController::showScores() const
{
    return m_phase != 2;                 // never during a live block
}
bool TrainingProgramController::showGroup() const
{
    if (m_phase != 2) return true;
    return m_cfg.visibility != Visibility::FullHidden;
}

// ── measured results (revealed data only) ────────────────────────────────

QVariantMap TrainingProgramController::blockReviewMetrics(int blockIndex1) const
{
    QVariantMap m;
    if (!m_store) return m;
    // A live (uncompleted) block's data is not exposed — hidden values must
    // not leak through this API while the block is still being fired.
    for (const TrainingBlockData& b : st().trainingBlocks) {
        if (b.blockIndex != blockIndex1) continue;
        if (!b.completed && m_phase == 2) return m;
        const BlockMetrics bm = computeBlockMetrics(b.shots);
        m[QStringLiteral("blockIndex")] = b.blockIndex;
        m[QStringLiteral("position")] = positionLabel(static_cast<Position>(b.position));
        m[QStringLiteral("focus")] = m_cfg.technicalFocus;
        m[QStringLiteral("shotCount")] = bm.shotCount;
        m[QStringLiteral("hasGroup")] = bm.hasGroup;
        m[QStringLiteral("totalScore")] = bm.totalScore;
        m[QStringLiteral("averageScore")] = bm.averageScore;
        m[QStringLiteral("scoreStdDev")] = bm.scoreStdDev;
        m[QStringLiteral("mpiX")] = bm.mpiX;
        m[QStringLiteral("mpiY")] = bm.mpiY;
        m[QStringLiteral("groupRadius")] = bm.groupRadius;
        m[QStringLiteral("groupDiameter")] = bm.groupDiameter;
        m[QStringLiteral("horizontalSpread")] = bm.horizontalSpread;
        m[QStringLiteral("verticalSpread")] = bm.verticalSpread;
        m[QStringLiteral("hasTiming")] = bm.hasTiming;
        m[QStringLiteral("averageShotTime")] = bm.averageShotTime;
        m[QStringLiteral("shotTimeStdDev")] = bm.shotTimeStdDev;
        m[QStringLiteral("note")] = b.note;
        return m;
    }
    return m;
}

QVariantList TrainingProgramController::completedBlockSummaries() const
{
    QVariantList out;
    if (!m_store) return out;
    for (const TrainingBlockData& b : st().trainingBlocks)
        if (b.completed)
            out.append(blockReviewMetrics(b.blockIndex));
    return out;
}

QVariantMap TrainingProgramController::finalComparison() const
{
    QVariantMap m;
    if (!m_store) return m;
    QVector<BlockMetrics> metrics;
    for (const TrainingBlockData& b : st().trainingBlocks)
        metrics.append(b.completed ? computeBlockMetrics(b.shots) : BlockMetrics{});
    const BlockComparison c = compareBlocks(metrics);
    m[QStringLiteral("bestScoreBlock")] = c.bestScoreBlock;
    m[QStringLiteral("tightestGroupBlock")] = c.tightestGroupBlock;
    m[QStringLiteral("mostRepeatableBlock")] = c.mostRepeatableBlock;
    m[QStringLiteral("hasDrift")] = c.hasDrift;
    m[QStringLiteral("centreDriftMm")] = c.centreDriftMm;
    m[QStringLiteral("centreDriftX")] = c.centreDriftX;
    m[QStringLiteral("centreDriftY")] = c.centreDriftY;
    m[QStringLiteral("hasSizeChange")] = c.hasSizeChange;
    m[QStringLiteral("groupSizeChangePct")] = c.groupSizeChangePct;
    return m;
}

QVariantMap TrainingProgramController::sighterAudit() const
{
    QVariantMap m;
    m[QStringLiteral("total")] = 0;
    m[QStringLiteral("threePositions")] = m_cfg.threePositions;
    m[QStringLiteral("kneeling")] = 0;
    m[QStringLiteral("prone")] = 0;
    m[QStringLiteral("standing")] = 0;
    if (!m_store) return m;
    const QVector<qint8>& p = st().trainingSighterPos;
    int k = 0, pr = 0, sd = 0;
    for (qint8 pos : p) {
        if (pos == 1) ++pr; else if (pos == 2) ++sd; else ++k;
    }
    m[QStringLiteral("total")] = p.size();
    m[QStringLiteral("kneeling")] = k;
    m[QStringLiteral("prone")] = pr;
    m[QStringLiteral("standing")] = sd;
    return m;
}

// ── T1.4 report / coaching projections (measured only) ───────────────────
namespace {
// "3.1 mm right" / "8.3 mm high" — MPI is +x right, +y high. Guard tiny values.
QString mpiPhrase(double vMm, const QString& pos, const QString& neg)
{
    if (qAbs(vMm) < 0.05) return QStringLiteral("centred");
    return QStringLiteral("%1 mm %2").arg(qAbs(vMm), 0, 'f', 1)
        .arg(vMm >= 0 ? pos : neg);
}
} // namespace

QVariantMap TrainingProgramController::blockDelta(int blockIndex1) const
{
    QVariantMap d;
    if (!m_store) return d;
    // metrics of the target block and the previous COMPLETED block
    const TrainingBlockData* cur = nullptr;
    const TrainingBlockData* prev = nullptr;
    for (const TrainingBlockData& b : st().trainingBlocks) {
        if (!b.completed) continue;
        if (b.blockIndex == blockIndex1) { cur = &b; break; }
        prev = &b;                       // last completed before the target
    }
    if (!cur) return d;
    const BlockMetrics cm = computeBlockMetrics(cur->shots);
    d[QStringLiteral("hasPrev")] = (prev != nullptr);
    if (!prev) return d;
    const BlockMetrics pm = computeBlockMetrics(prev->shots);
    d[QStringLiteral("prevBlock")] = prev->blockIndex;
    d[QStringLiteral("diameterDeltaMm")] = cm.groupDiameter - pm.groupDiameter;
    d[QStringLiteral("averageScoreDelta")] = cm.averageScore - pm.averageScore;
    d[QStringLiteral("scoreStdDevDelta")] = cm.scoreStdDev - pm.scoreStdDev;
    if (cm.hasTiming && pm.hasTiming) {
        d[QStringLiteral("avgTimeDelta")] = cm.averageShotTime - pm.averageShotTime;
        d[QStringLiteral("timeStdDevDelta")] = cm.shotTimeStdDev - pm.shotTimeStdDev;
    }
    return d;
}

QStringList TrainingProgramController::blockObservations(int blockIndex1) const
{
    QStringList out;
    if (!m_store) return out;
    const TrainingBlockData* cur = nullptr;
    for (const TrainingBlockData& b : st().trainingBlocks)
        if (b.completed && b.blockIndex == blockIndex1) { cur = &b; break; }
    if (!cur) return out;
    const BlockMetrics m = computeBlockMetrics(cur->shots);
    if (m.hasGroup) {
        if (m.horizontalSpread > m.verticalSpread * 1.25)
            out << QStringLiteral("The group was wider horizontally than vertically.");
        else if (m.verticalSpread > m.horizontalSpread * 1.25)
            out << QStringLiteral("The group was taller vertically than horizontally.");
        else
            out << QStringLiteral("The vertical and horizontal spreads were similar.");
        out << QStringLiteral("The group centre was %1 and %2.")
                   .arg(mpiPhrase(m.mpiX, QStringLiteral("right"), QStringLiteral("left")))
                   .arg(mpiPhrase(m.mpiY, QStringLiteral("high"), QStringLiteral("low")));
    }
    const QVariantMap d = blockDelta(blockIndex1);
    if (d.value(QStringLiteral("hasPrev")).toBool()) {
        const double dd = d.value(QStringLiteral("diameterDeltaMm")).toDouble();
        if (qAbs(dd) >= 0.5)
            out << QStringLiteral("The group was %1 mm %2 than Block %3.")
                       .arg(qAbs(dd), 0, 'f', 1)
                       .arg(dd > 0 ? QStringLiteral("larger") : QStringLiteral("smaller"))
                       .arg(d.value(QStringLiteral("prevBlock")).toInt());
    }
    return out;
}

QStringList TrainingProgramController::sessionObservations() const
{
    QStringList out;
    if (!m_store) return out;
    QVector<BlockMetrics> metrics;
    for (const TrainingBlockData& b : st().trainingBlocks)
        metrics.append(b.completed ? computeBlockMetrics(b.shots) : BlockMetrics{});
    const BlockComparison c = compareBlocks(metrics);
    if (c.bestScoreBlock > 0)
        out << QStringLiteral("Block %1 had the highest average score.").arg(c.bestScoreBlock);
    if (c.tightestGroupBlock > 0)
        out << QStringLiteral("Block %1 had the tightest group.").arg(c.tightestGroupBlock);
    if (c.mostRepeatableBlock > 0)
        out << QStringLiteral("Block %1 had the most consistent shot scores.").arg(c.mostRepeatableBlock);
    if (c.bestScoreBlock > 0 && c.bestScoreBlock == c.tightestGroupBlock)
        out << QStringLiteral("The highest-scoring block was also the tightest group.");
    if (c.hasSizeChange && qAbs(c.groupSizeChangePct) >= 1.0)
        out << QStringLiteral("Group size %1 %2% from the first to the last block.")
                   .arg(c.groupSizeChangePct > 0 ? QStringLiteral("grew") : QStringLiteral("reduced"))
                   .arg(qAbs(c.groupSizeChangePct), 0, 'f', 0);
    if (c.hasDrift && c.centreDriftMm >= 0.5)
        out << QStringLiteral("The group centre moved %1 (%2, %3) from the first to the last block.")
                   .arg(c.centreDriftMm, 0, 'f', 1)
                   .arg(mpiPhrase(c.centreDriftX, QStringLiteral("right"), QStringLiteral("left")))
                   .arg(mpiPhrase(c.centreDriftY, QStringLiteral("high"), QStringLiteral("low")));
    return out;
}

int TrainingProgramController::countedShotsTotal() const
{
    if (!m_store) return 0;
    int n = 0;
    for (const TrainingBlockData& b : st().trainingBlocks) n += b.shots.size();
    return n;
}

int TrainingProgramController::completedBlockCount() const
{
    if (!m_store) return 0;
    int n = 0;
    for (const TrainingBlockData& b : st().trainingBlocks) if (b.completed) ++n;
    return n;
}

int TrainingProgramController::sessionDurationSec() const
{
    if (!m_store) return 0;
    // splitMs is monotonic ms stamped at each shot; span first→last counted shot.
    qint64 lo = -1, hi = -1;
    for (const TrainingBlockData& b : st().trainingBlocks)
        for (const ShotCore& s : b.shots) {
            if (lo < 0 || s.splitMs < lo) lo = s.splitMs;
            if (hi < 0 || s.splitMs > hi) hi = s.splitMs;
        }
    if (lo < 0 || hi <= lo) return 0;
    return static_cast<int>((hi - lo) / 1000);
}

QVariantMap TrainingProgramController::trainingReportModel() const
{
    QVariantMap r;
    if (!m_store) return r;
    const SessionState& s = st();
    // session meta
    r[QStringLiteral("sessionId")] = s.sessionId;
    r[QStringLiteral("athlete")] = s.athlete;
    r[QStringLiteral("createdAtIso")] = s.createdAtIso;
    r[QStringLiteral("operatingMode")] = s.operatingMode.isEmpty()
            ? QStringLiteral("Legacy") : s.operatingMode;
    r[QStringLiteral("focus")] = m_cfg.technicalFocus;
    r[QStringLiteral("visibilityMode")] = static_cast<int>(m_cfg.visibility);
    r[QStringLiteral("visibilityLabel")] = visibilityLabel(m_cfg.visibility);
    r[QStringLiteral("threePositions")] = m_cfg.threePositions;
    r[QStringLiteral("blockCount")] = m_cfg.blockCount;
    r[QStringLiteral("shotsPerBlock")] = m_cfg.shotsPerBlock;
    r[QStringLiteral("plannedShots")] = m_cfg.totalShots();
    r[QStringLiteral("countedShots")] = countedShotsTotal();
    r[QStringLiteral("completedBlocks")] = completedBlockCount();
    r[QStringLiteral("endedEarly")] = endedEarly();
    r[QStringLiteral("durationSec")] = sessionDurationSec();
    r[QStringLiteral("sighterAudit")] = sighterAudit();
    r[QStringLiteral("observations")] = sessionObservations();
    // per completed block
    QVariantList blocks;
    for (const TrainingBlockData& b : s.trainingBlocks) {
        if (!b.completed) continue;
        QVariantMap bm = blockReviewMetrics(b.blockIndex);
        bm[QStringLiteral("plot")] = blockShotPlot(b.blockIndex);
        bm[QStringLiteral("delta")] = blockDelta(b.blockIndex);
        bm[QStringLiteral("observations")] = blockObservations(b.blockIndex);
        bm[QStringLiteral("groupPattern")] = groupPattern(b.blockIndex);
        bm[QStringLiteral("positionName")] = m_cfg.threePositions
                ? positionLabel(static_cast<Position>(b.position)) : QString();
        blocks.append(bm);
    }
    r[QStringLiteral("blocks")] = blocks;
    r[QStringLiteral("comparison")] = finalComparison();
    return r;
}

QVariantList TrainingProgramController::blockShotPlot(int blockIndex1) const
{
    QVariantList out;
    if (!m_store) return out;
    for (const TrainingBlockData& b : st().trainingBlocks) {
        if (b.blockIndex != blockIndex1) continue;
        if (!b.completed && m_phase == 2) return out;   // no live-block leak
        for (const ShotCore& s : b.shots) {
            QVariantMap p;
            p[QStringLiteral("xMm")] = s.xHundredthMm / 100.0;
            p[QStringLiteral("yMm")] = s.yHundredthMm / 100.0;
            p[QStringLiteral("score")] = s.scoreTenths / 10.0;
            out.append(p);
        }
        return out;
    }
    return out;
}

QVariantMap TrainingProgramController::groupPattern(int blockIndex1) const
{
    QVariantMap m;
    if (!m_store) return m;
    for (const TrainingBlockData& b : st().trainingBlocks) {
        if (b.blockIndex != blockIndex1) continue;
        if (!b.completed && m_phase == 2) return m;      // no live-block leak
        QVector<double> xs, ys;
        for (const ShotCore& s : b.shots) {
            xs.append(s.xHundredthMm / 100.0);
            ys.append(s.yHundredthMm / 100.0);
        }
        const GroupPatternResult gp =
            analyzeGroup(xs, ys, issfRingSpacingMm(m_cfg.discipline));
        m[QStringLiteral("hasData")] = gp.hasData;
        m[QStringLiteral("shotCount")] = gp.shotCount;
        m[QStringLiteral("blockIndex")] = b.blockIndex;
        m[QStringLiteral("positionName")] = m_cfg.threePositions
            ? positionLabel(static_cast<Position>(b.position)) : QString();
        m[QStringLiteral("diameterMm")] = gp.diameterMm;
        m[QStringLiteral("radiusMm")] = gp.radiusMm;
        m[QStringLiteral("hSpreadMm")] = gp.hSpreadMm;
        m[QStringLiteral("vSpreadMm")] = gp.vSpreadMm;
        m[QStringLiteral("hvRatio")] = gp.hvRatio;
        m[QStringLiteral("centreOffsetMm")] = gp.centreOffsetMm;
        m[QStringLiteral("driftMm")] = gp.driftMm;
        m[QStringLiteral("primaryLabel")] = gp.primaryLabel();
        QVariantList props;
        QString primaryKey;
        for (const GroupProperty& p : gp.properties) {
            QVariantMap pm;
            pm[QStringLiteral("key")] = p.key;
            pm[QStringLiteral("label")] = p.label;
            pm[QStringLiteral("evidence")] = p.evidence;
            pm[QStringLiteral("confidence")] = confidenceLabel(p.confidence);
            props.append(pm);
            if (primaryKey.isEmpty()) primaryKey = p.key;
        }
        m[QStringLiteral("properties")] = props;
        // coach discussion PROMPT — a question, never a cause statement.
        QString prompt;
        if (primaryKey == QLatin1String("vertical_string"))
            prompt = QStringLiteral("Did the aiming picture appear to move mainly vertically?");
        else if (primaryKey == QLatin1String("horizontal_string"))
            prompt = QStringLiteral("Did the aiming picture appear to move mainly horizontally?");
        else if (primaryKey == QLatin1String("diagonal_string"))
            prompt = QStringLiteral("Did the hold appear to move diagonally through the block?");
        else if (primaryKey == QLatin1String("two_clusters"))
            prompt = QStringLiteral("Was there a noticeable change between the two clusters?");
        else if (primaryKey == QLatin1String("progressive_drift"))
            prompt = QStringLiteral("Did the group begin to drift later in the block?");
        else if (primaryKey == QLatin1String("isolated_outlier"))
            prompt = QStringLiteral("What felt different on the isolated shot?");
        else if (primaryKey == QLatin1String("group_expansion"))
            prompt = QStringLiteral("Did anything change as the block progressed?");
        else if (primaryKey == QLatin1String("tight_offset"))
            prompt = QStringLiteral("Where were you aiming relative to the centre?");
        m[QStringLiteral("prompt")] = prompt;
        m[QStringLiteral("disclaimer")] =
            QStringLiteral("This describes the measured group shape. It does not identify the technical cause.");
        return m;
    }
    return m;
}

// ── T1 closure: in-place recovery ────────────────────────────────────────

bool TrainingProgramController::resumeFromRecovery(const QString& sessionId)
{
    if (!m_recovery)
        m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();                       // idempotent candidate cache
    RecoveredMatchState rec;
    ErrorInfo err;
    if (!m_recovery->buildRecoveredState(sessionId, &rec, &err)) {
        emit journalWriteFailed(rec.journalPath, err.technicalDetail);
        return false;
    }
    // Owner check by classification, not discipline: only Training sessions
    // (technical_blocks) are resumed here — a competition journal is refused.
    if (rec.state.sessionKind != QLatin1String("Training")
        || rec.state.trainingProgramId != QLatin1String(kProgramTechnicalBlocks)) {
        m_lastError = QStringLiteral("Not a Training session.");
        return false;
    }
    const ReliabilityResult rr = m_store->resumeSession(rec);
    if (!rr.ok) {
        emit journalWriteFailed(rec.journalPath, rr.error.technicalDetail);
        return false;
    }
    // Rebuild the configuration from the replayed state (never re-defaults).
    const SessionState& s = st();
    m_cfg = TechnicalBlocksConfig::defaultsFor(s.discipline);   // shape + est. time
    m_cfg.discipline = s.discipline;
    m_cfg.blockCount = s.trainingBlockCount;
    m_cfg.shotsPerBlock = s.trainingShotsPerBlock;
    m_cfg.visibility = static_cast<Visibility>(qBound<int>(0, s.trainingVisibility, 2));
    m_cfg.technicalFocus = s.trainingFocus;
    if (m_cfg.threePositions)
        m_cfg.blocksPerPosition = qMax(1, m_cfg.blockCount / 3);
    // Duplicate guard continues past every recovered externalId — counted
    // shots AND sighters share the one externalId sequence.
    m_lastExternalId = -1;
    for (const TrainingBlockData& b : s.trainingBlocks)
        for (const ShotCore& sc : b.shots)
            if (sc.externalId > m_lastExternalId) m_lastExternalId = sc.externalId;
    for (const ShotCore& sc : s.trainingSighters)
        if (sc.externalId > m_lastExternalId) m_lastExternalId = sc.externalId;
    // Phase re-derivation from the durable record only:
    //   completed -> summary; sighter phase durably open -> Sighters (no block
    //   started yet); current block completed -> review (never auto-start the
    //   next block); otherwise -> active block.
    if (s.trainingCompleted) {
        m_currentBlock = qMax<int>(1, s.trainingCurrentBlock);
        m_phase = 4;
    } else if (s.trainingInSighterPhase) {
        // pendingBlock() must resolve to the block the sighters precede.
        m_currentBlock = qMax<int>(0, s.trainingSighterBeforeBlock - 1);
        m_phase = 1;
    } else {
        m_currentBlock = qMax<int>(1, s.trainingCurrentBlock);
        bool currentCompleted = false;
        for (const TrainingBlockData& b : s.trainingBlocks)
            if (b.blockIndex == m_currentBlock) currentCompleted = b.completed;
        m_phase = currentCompleted ? 3 : 2;
    }
    m_blockStartMonoMs = m_store->nowMonotonicMs();   // elapsed restarts at resume
    emit configChanged(); emit phaseChanged(); emit progressChanged();
    if (m_phase == 4) emit trainingCompleted();
    qInfo() << "TRAINING: resumed session" << s.sessionId.left(8)
            << "block" << m_currentBlock << "phase" << m_phase;
    return true;
}

void TrainingProgramController::discardRecovery(const QString& sessionId)
{
    if (!m_recovery)
        m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();
    m_recovery->archiveOrDiscard(sessionId, /*discarded*/ true);
}

QVariantList TrainingProgramController::recoveredCurrentBlockShots() const
{
    // Shots of the CURRENT block only (mm + within-block index). The QML
    // restorer applies the visibility mode: Mode A buffers, B/C draw.
    QVariantList out;
    if (!m_store) return out;
    for (const TrainingBlockData& b : st().trainingBlocks) {
        if (b.blockIndex != m_currentBlock) continue;
        for (const ShotCore& sc : b.shots) {
            QVariantMap m;
            m[QStringLiteral("xMm")] = sc.xHundredthMm / 100.0;
            m[QStringLiteral("yMm")] = sc.yHundredthMm / 100.0;
            out.append(m);
        }
    }
    return out;
}

QVariantList TrainingProgramController::recoveredSighterShots() const
{
    // Sighters of the CURRENT phase's position only (mm) — the counted target
    // never mixes them, but a recovered Sighters phase redraws them verbatim.
    QVariantList out;
    if (!m_store || m_phase != 1) return out;
    const int pos = static_cast<int>(m_cfg.positionForBlock(pendingBlock()));
    const QVector<ShotCore>& s = st().trainingSighters;
    const QVector<qint8>&    p = st().trainingSighterPos;
    for (int i = 0; i < s.size(); ++i) {
        if (i < p.size() && static_cast<int>(p[i]) != pos) continue;
        QVariantMap m;
        m[QStringLiteral("xMm")] = s[i].xHundredthMm / 100.0;
        m[QStringLiteral("yMm")] = s[i].yHundredthMm / 100.0;
        m[QStringLiteral("score")] = s[i].scoreTenths / 10.0;
        out.append(m);
    }
    return out;
}

// ── internals ────────────────────────────────────────────────────────────

bool TrainingProgramController::submit(const DomainEvent& ev)
{
    const SubmitResult r = m_store->submit(ev);
    if (!r.ok) {
        m_lastError = r.error.operatorMessage;
        qWarning() << "TRAINING: submit refused —" << r.error.technicalDetail;
        return false;
    }
    return true;
}
