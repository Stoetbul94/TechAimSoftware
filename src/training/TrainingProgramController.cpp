#include "TrainingProgramController.h"

#include "mode/OperatingMode.h"

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
    const QString err = m_cfg.validate();
    if (!err.isEmpty()) { m_lastError = err; return false; }
    if (m_store->active()) { m_lastError = QStringLiteral("A session is already active."); return false; }

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
    if (!r.ok) { m_lastError = r.error.operatorMessage; return false; }

    TrainingSessionStarted ev;
    ev.programId = QLatin1String(kProgramTechnicalBlocks);
    ev.discipline = m_cfg.discipline;
    ev.blockCount = static_cast<qint16>(m_cfg.blockCount);
    ev.shotsPerBlock = static_cast<qint16>(m_cfg.shotsPerBlock);
    ev.visibilityMode = static_cast<qint8>(m_cfg.visibility);
    ev.technicalFocus = m_cfg.technicalFocus;
    ev.startPosition = static_cast<qint8>(m_cfg.positionForBlock(1));
    if (!submit(DomainEvent(ev))) return false;

    TrainingBlockStarted b;
    b.blockIndex = 1;
    b.position = static_cast<qint8>(m_cfg.positionForBlock(1));
    if (!submit(DomainEvent(b))) return false;

    m_currentBlock = 1;
    m_lastExternalId = -1;
    m_blockStartMonoMs = m_store->nowMonotonicMs();
    m_phase = 2;                          // BlockActive
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
    if (m_phase != 2 || !m_store->active()) {
        emit shotRejected(QStringLiteral("TrainingNotActive"));
        return false;
    }
    // duplicate/retransmission guard (same contract as the finals)
    if (externalId >= 0 && externalId <= m_lastExternalId) {
        emit shotRejected(QStringLiteral("DuplicateShot"));
        return false;
    }
    // block cap — no spill into the next block, ever
    const int within = shotsInBlock();
    if (within >= m_cfg.shotsPerBlock) {
        emit shotRejected(QStringLiteral("BlockFull"));
        return false;
    }
    if (!(decimalScore >= 0.0) || decimalScore > 11.0
        || !(xMm > -500.0 && xMm < 500.0) || !(yMm > -500.0 && yMm < 500.0)) {
        emit shotRejected(QStringLiteral("InvalidShotData"));
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
    TrainingBlockStarted b;
    b.blockIndex = static_cast<qint16>(m_currentBlock + 1);
    b.position = static_cast<qint8>(m_cfg.positionForBlock(m_currentBlock + 1));
    if (!submit(DomainEvent(b))) return false;
    ++m_currentBlock;
    m_blockStartMonoMs = m_store->nowMonotonicMs();
    m_phase = 2;
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
    if (m_store && m_store->active())
        m_store->closeSession(CloseReason::Clean);
    emit phaseChanged();
}

void TrainingProgramController::resetTraining()
{
    if (m_store && m_store->active())
        m_store->closeSession(CloseReason::Clean);
    m_phase = 0;
    m_currentBlock = 0;
    m_lastExternalId = -1;
    emit phaseChanged(); emit progressChanged();
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
    return positionLabel(m_cfg.positionForBlock(qMax(1, m_currentBlock)));
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
    // Duplicate guard continues past every recovered externalId.
    m_lastExternalId = -1;
    for (const TrainingBlockData& b : s.trainingBlocks)
        for (const ShotCore& sc : b.shots)
            if (sc.externalId > m_lastExternalId) m_lastExternalId = sc.externalId;
    // Phase re-derivation from the durable record only:
    //   completed  -> summary; current block completed -> review (never
    //   auto-start the next block); otherwise -> active block.
    m_currentBlock = qMax<int>(1, s.trainingCurrentBlock);
    bool currentCompleted = false;
    for (const TrainingBlockData& b : s.trainingBlocks)
        if (b.blockIndex == m_currentBlock) currentCompleted = b.completed;
    if (s.trainingCompleted)      m_phase = 4;
    else if (currentCompleted)    m_phase = 3;
    else                          m_phase = 2;
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
