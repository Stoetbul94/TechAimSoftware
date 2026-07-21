#include "Finals10mController.h"
#include "../reliability/storage/StoragePaths.h"
#include "../reliability/core/FixedPoint.h"
#include "../incident/EstIncidentController.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QProcessEnvironment>
#include <QDebug>
#include <QUuid>

using namespace techaim::finals10m;

namespace {
const int kTickMs = 50;   // UI refresh; time itself is monotonic, not counted

// Single-athlete course checkpoint label (rule 6.17.2 g). These are COURSE
// checkpoints only — never a finishing place (that requires the future
// competition coordinator's authoritative opponent data).
QString checkpointLabelForShot(int shotNumber)
{
    switch (shotNumber) {
    case 12: return QStringLiteral("Elimination checkpoint — shot 12");
    case 14: return QStringLiteral("Elimination checkpoint — shot 14");
    case 16: return QStringLiteral("Elimination checkpoint — shot 16");
    case 18: return QStringLiteral("Elimination checkpoint — shot 18");
    case 20: return QStringLiteral("Medal checkpoint — shot 20");
    case 22: return QStringLiteral("Medal checkpoint — shot 22");
    case 24: return QStringLiteral("Final course complete — shot 24");
    default: return QString();
    }
}
}

// ── construction ───────────────────────────────────────────────────────────

Finals10mController::Finals10mController(QObject* parent)
    : QObject(parent)
    , m_store(std::make_unique<ta::rel::SessionStore>())
{
    m_tick.setInterval(kTickMs);
    connect(&m_tick, &QTimer::timeout, this, &Finals10mController::tick);

    connect(m_store.get(), &ta::rel::SessionStore::journalWriteFailed,
            this, [this](QString path, QString detail) {
        qWarning() << "FINALS10M journal write failed at" << path << ":" << detail;
        if (m_journalFailureNotified)
            return;
        m_journalFailureNotified = true;
        emit journalWriteFailed(path, detail);
    });
    connect(m_store.get(), &ta::rel::SessionStore::persistenceHealthChanged,
            this, [this](ta::rel::Health h) {
        emit persistenceHealthChanged(static_cast<int>(h));
    });
    connect(m_store.get(), &ta::rel::SessionStore::criticalPersistenceFailure,
            this, [this](QString msg) {
        emit journalWriteFailed(sessionJournalPath(), msg);
    });

    bool ok = false;
    const double envScale = QProcessEnvironment::systemEnvironment()
                                .value(QStringLiteral("TECHAIM_FINALS_TIMESCALE"))
                                .toDouble(&ok);
    if (ok && envScale > 0.0)
        m_timeScale = envScale;
}

void Finals10mController::configureDiscipline(const QString& disciplineId)
{
    if (running())
        return;   // never re-configure mid-course
    ta::rel::Discipline d = ta::rel::Discipline::AirRifleFinal10m;
    if (disciplineId == QLatin1String("FINAL_AP10"))
        d = ta::rel::Discipline::AirPistolFinal10m;
    const CeremonyMode prevCeremony = m_cfg.ceremonyMode;
    m_cfg = Finals10mConfig::forDiscipline(d);
    m_cfg.ceremonyMode = prevCeremony;   // preserve a caller-set ceremony mode
    emit configChanged();
}

QString Finals10mController::disciplineId() const
{
    return QString::fromLatin1(ta::rel::disciplineId(m_cfg.discipline));
}

// ── time ─────────────────────────────────────────────────────────────────

qint64 Finals10mController::scaledNow() const
{
    if (!m_monoStarted)
        return 0;
    qint64 raw = m_mono.elapsed() - m_pausedTotalRaw;
    if (m_paused)
        raw -= (m_mono.elapsed() - m_pauseStartRaw);
    return static_cast<qint64>(raw * m_timeScale);
}

qint64 Finals10mController::elapsedMs() const
{
    return m_monoStarted ? (scaledNow() - m_phaseStartScaled) : 0;
}

qint64 Finals10mController::remainingMs() const
{
    if (!m_monoStarted || m_segmentEndScaled <= 0)
        return 0;
    const qint64 r = m_segmentEndScaled - scaledNow();
    return r > 0 ? r : 0;
}

QString Finals10mController::remainingFormatted() const
{
    const qint64 totalSec = (remainingMs() + 999) / 1000;
    return QStringLiteral("%1:%2")
        .arg(totalSec / 60, 2, 10, QLatin1Char('0'))
        .arg(totalSec % 60, 2, 10, QLatin1Char('0'));
}

void Finals10mController::setTimeScale(double s)
{
    if (s <= 0.0 || qFuzzyCompare(m_timeScale, s))
        return;
    if (m_stage == Stage::Idle) {
        m_timeScale = s;
        emit timeScaleChanged();
    } else {
        qWarning() << "FINALS10M: timeScale can only change while Idle";
    }
}

void Finals10mController::setCeremonyMode(int m)
{
    if (m < 0 || m > 2 || m == static_cast<int>(m_cfg.ceremonyMode))
        return;
    m_cfg.ceremonyMode = static_cast<CeremonyMode>(m);
    emit configChanged();
}

// ── main API ─────────────────────────────────────────────────────────────

void Finals10mController::startFinal()
{
    if (m_stage != Stage::Idle)
        return;
    m_mono.start();
    m_monoStarted = true;
    m_paused = false;
    m_pausedTotalRaw = 0;
    m_commandSeq = 0;
    m_windowId = 0;
    m_shotEventId = 0;
    m_lastExternalId = -1;
    m_lastAcceptScaled = 0;
    m_singleIndex = 0;
    m_events.clear();
    m_journalFailureNotified = false;
    m_missingShots.clear();
    m_officialShotRecords.clear();
    m_rejectionRecords.clear();
    m_sighterCount = 0;
    m_cumulativeTotal = 0.0;
    m_officialShotCount = 0;
    m_lastCheckpointLabel.clear();
    m_checkpointTotals.clear();
    m_seriesSubtotal.clear();
    m_stageStatus.clear();
    emit totalsChanged();
    beginJournalSession();
    m_tick.start();

    if (m_cfg.ceremonyMode == CeremonyMode::Skip)
        enterStage(Stage::PrepSighting);
    else
        enterStage(Stage::Presentation);
}

void Finals10mController::skipCeremony()
{
    if (m_stage == Stage::Presentation)
        enterStage(Stage::PrepSighting);
}

void Finals10mController::abortFinal()
{
    if (m_stage == Stage::Idle || m_stage == Stage::Complete || m_stage == Stage::Aborted)
        return;
    if (stageShotLimit() > 0)
        setStageStatus(currentFineStageId(), StageStatus::Aborted);
    setWindow(WindowState::Closed);
    m_tick.stop();
    m_stage = Stage::Aborted;
    m_segmentEndScaled = 0;
    emit phaseChanged();
    emit countdownChanged();
}

void Finals10mController::resetFinal()
{
    m_tick.stop();
    m_monoStarted = false;
    m_paused = false;
    m_stage = Stage::Idle;
    m_window = WindowState::Closed;
    m_targetMode = TargetMode::Sighter;
    m_singleIndex = 0;
    m_shotsInStage = 0;
    m_segmentEndScaled = 0;
    m_commandText.clear();
    emit phaseChanged();
    emit windowStateChanged();
    emit targetModeChanged();
    emit shotCountsChanged();
    emit countdownChanged();
}

void Finals10mController::pauseTrainingSimulation()
{
    if (!m_monoStarted || m_paused || !running())
        return;
    m_paused = true;
    m_pauseStartRaw = m_mono.elapsed();
    emit pausedChanged();
}

void Finals10mController::resumeTrainingSimulation()
{
    if (!m_paused)
        return;
    m_pausedTotalRaw += (m_mono.elapsed() - m_pauseStartRaw);
    m_paused = false;
    emit pausedChanged();
}

bool Finals10mController::running() const
{
    return m_stage != Stage::Idle && m_stage != Stage::Complete
        && m_stage != Stage::Aborted;
}

void Finals10mController::executePrimaryAction()
{
    if (m_stage == Stage::Complete) {
        emit reportRequested();
        return;
    }
}

bool Finals10mController::primaryActionVisible() const
{
    return m_stage == Stage::Complete;
}

bool Finals10mController::primaryActionEnabled() const
{
    return m_stage == Stage::Complete;
}

QString Finals10mController::primaryActionLabel() const
{
    return m_stage == Stage::Complete ? QStringLiteral("VIEW REPORT  →") : QString();
}

// ── shots ────────────────────────────────────────────────────────────────

void Finals10mController::registerShot(double xMm, double yMm, double decimalScore,
                                       int externalShotId, double direction)
{
    if (!running()) {
        rejectShot(RejectReason::FinalsNotActive, xMm, yMm, false, externalShotId);
        return;
    }
    if (!(decimalScore >= 0.0) || decimalScore > 11.0
            || !(xMm > -500.0 && xMm < 500.0) || !(yMm > -500.0 && yMm < 500.0)) {
        rejectShot(RejectReason::InvalidShotData, xMm, yMm, false, externalShotId);
        return;
    }
    if (m_window == WindowState::Closed) {
        rejectShot(RejectReason::WindowClosed, xMm, yMm, false, externalShotId);
        return;
    }
    if (externalShotId >= 0 && externalShotId <= m_lastExternalId) {
        rejectShot(RejectReason::DuplicateShot, xMm, yMm, false, externalShotId);
        return;
    }
    if ((m_window == WindowState::MatchOpen && m_targetMode != TargetMode::Match)
            || (m_window == WindowState::SightingOpen && m_targetMode != TargetMode::Sighter)) {
        rejectShot(RejectReason::WrongTargetMode, xMm, yMm, false, externalShotId);
        return;
    }
    // Phase-E resume gate (generic authority model): OFFICIAL shots refused
    // while an unresolved EST incident requires an authorised decision.
    if (m_window == WindowState::MatchOpen && m_store && m_store->active()
            && EstIncidentController::officialsBlocked(m_store->state())) {
        rejectShot(RejectReason::EstIncidentBlocked, xMm, yMm, false, externalShotId);
        return;
    }
    if (externalShotId >= 0)
        m_lastExternalId = externalShotId;
    if (m_window == WindowState::SightingOpen) {
        acceptShot(true, xMm, yMm, decimalScore, false, externalShotId, direction);
        return;
    }
    if (m_shotsInStage >= stageShotLimit()) {
        rejectShot(RejectReason::StageShotLimitReached, xMm, yMm, false, externalShotId);
        return;
    }
    acceptShot(false, xMm, yMm, decimalScore, false, externalShotId, direction);
}

void Finals10mController::simulateShot()
{
    if (!running()) {
        rejectShot(RejectReason::FinalsNotActive, 0, 0, true, -1);
        return;
    }
    if (m_window == WindowState::Closed) {
        rejectShot(RejectReason::WindowClosed, 0, 0, true, -1);
        return;
    }
    if (m_window == WindowState::SightingOpen) {
        acceptShot(true, 0, 0, 0, true, -1);
        return;
    }
    if (m_shotsInStage >= stageShotLimit()) {
        rejectShot(RejectReason::StageShotLimitReached, 0, 0, true, -1);
        return;
    }
    acceptShot(false, 0, 0, 0, true, -1);
}

QVariantMap Finals10mController::makeShotRecord(bool sighter, double xMm, double yMm,
                                                double score, int finalNumber,
                                                int withinStage, qint64 externalShotId,
                                                double direction, bool simulated,
                                                quint64 eventId)
{
    QVariantMap m;
    m[QStringLiteral("xmm")] = xMm;
    m[QStringLiteral("ymm")] = yMm;
    m[QStringLiteral("calculatedscore")] = QString::number(score, 'f', 1);
    m[QStringLiteral("score")] = score;
    m[QStringLiteral("sighter")] = sighter;
    m[QStringLiteral("finalShotNumber")] = finalNumber;
    m[QStringLiteral("withinStage")] = withinStage;
    m[QStringLiteral("seriesIndex")] = seriesIndexFor(m_stage, m_singleIndex);
    m[QStringLiteral("stageLabel")] = stageLabel();
    m[QStringLiteral("direction")] = QString::number(direction, 'f', 2);
    m[QStringLiteral("simulated")] = simulated;
    m[QStringLiteral("externalId")] = static_cast<qlonglong>(externalShotId);
    m[QStringLiteral("eventId")] = static_cast<qulonglong>(eventId);
    return m;
}

void Finals10mController::acceptShot(bool sighter, double xMm, double yMm,
                                     double score, bool simulated,
                                     qint64 externalShotId, double direction)
{
    int finalNumber = 0, withinStage = 0;
    if (!sighter) {
        ++m_shotsInStage;
        withinStage = m_shotsInStage;
        finalNumber = stageShotNumberBase() + m_shotsInStage;
        m_cumulativeTotal += score;
        m_seriesSubtotal[currentFineStageId()] =
            m_seriesSubtotal.value(currentFineStageId(), 0.0) + score;
        if (m_shotsInStage >= stageShotLimit())
            setStageStatus(currentFineStageId(), StageStatus::Complete);
        ++m_officialShotCount;
        emit totalsChanged();
        emit shotCountsChanged();
    }
    m_lastAcceptScaled = scaledNow();

    const QVariantMap shot = makeShotRecord(sighter, xMm, yMm, score, finalNumber,
                                            withinStage, externalShotId, direction,
                                            simulated, ++m_shotEventId);
    if (sighter)
        ++m_sighterCount;
    else
        m_officialShotRecords.append(shot);

    const ta::rel::ShotCore core = buildShotCore(xMm, yMm, score, finalNumber,
                                                 withinStage, externalShotId,
                                                 direction, simulated);
    if (sighter)
        submitEvent(ta::rel::DomainEvent(ta::rel::SighterAccepted{core}));
    else
        submitEvent(ta::rel::DomainEvent(ta::rel::ShotAccepted{core}));
    emit shotAccepted(shot);

    // Window auto-close: the commanded window ends when the athlete completes
    // its shot(s) (single-athlete training; real CRO waits for all finalists).
    if (!sighter && m_cfg.stopWhenAthleteCompletes
            && m_shotsInStage >= stageShotLimit()) {
        closeWindowAndStop();
        advanceAfterCommandStage();
    }
}

void Finals10mController::rejectShot(RejectReason reason, double xMm, double yMm,
                                     bool simulated, qint64 externalShotId)
{
    QVariantMap rej;
    rej[QStringLiteral("reason")] = rejectReasonName(reason);
    rej[QStringLiteral("xmm")] = xMm;
    rej[QStringLiteral("ymm")] = yMm;
    rej[QStringLiteral("stageLabel")] = stageLabel();
    rej[QStringLiteral("simulated")] = simulated;
    rej[QStringLiteral("eventId")] = static_cast<qulonglong>(++m_shotEventId);
    m_rejectionRecords.append(rej);
    {
        ta::rel::ShotRejected ev;
        ev.reason = rejectReasonName(reason);
        ev.externalId = externalShotId < 0 ? 0 : externalShotId;
        ta::rel::CoordinateHundredthMm cx, cy;
        ta::rel::CoordinateHundredthMm::fromDouble(xMm, &cx);
        ta::rel::CoordinateHundredthMm::fromDouble(yMm, &cy);
        ev.xHundredthMm = cx.raw();
        ev.yHundredthMm = cy.raw();
        ev.simulated = simulated;
        submitEvent(ta::rel::DomainEvent(ev));
    }
    qInfo() << "FINALS10M: shot rejected —" << rejectReasonName(reason)
            << "in" << stageName(m_stage);
    emit shotRejected(rej);
}

void Finals10mController::recordMissingShots()
{
    const int limit = stageShotLimit();
    const int base = stageShotNumberBase();
    for (int i = m_shotsInStage + 1; i <= limit; ++i) {
        const int expected = base + i;
        QVariantMap rec;
        rec[QStringLiteral("expectedNumber")] = expected;
        rec[QStringLiteral("stageLabel")] = stageLabel();
        rec[QStringLiteral("reason")] = QStringLiteral("TimeExpired");
        m_missingShots.append(rec);
        ta::rel::MissingShotRecorded ev;
        ev.expectedNumber = static_cast<qint16>(expected);
        ev.stageId = static_cast<qint16>(currentFineStageId());
        ev.reason = QStringLiteral("TimeExpired");
        submitEvent(ta::rel::DomainEvent(ev));
        emit missingShotRecorded(rec);
    }
    setStageStatus(currentFineStageId(),
                   m_shotsInStage >= limit ? StageStatus::Complete
                                           : StageStatus::Incomplete);
}

void Finals10mController::emitCheckpointIfDue(int completedShotNumber)
{
    const QString label = checkpointLabelForShot(completedShotNumber);
    if (label.isEmpty())
        return;
    m_checkpointTotals[completedShotNumber] = m_cumulativeTotal;
    m_lastCheckpointLabel = label;
    issueCommand(CommandType::CheckpointNotice, label);
    emit checkpointReached(completedShotNumber, label, m_cumulativeTotal);
    emit totalsChanged();
}

// ── stage machine ──────────────────────────────────────────────────────────

int Finals10mController::stageShotLimit() const
{
    switch (m_stage) {
    case Stage::Series1:
    case Stage::Series2: return m_cfg.shotsPerSeries;
    case Stage::Singles: return 1;
    default:             return 0;
    }
}

int Finals10mController::stageShotNumberBase() const
{
    switch (m_stage) {
    case Stage::Series1: return 0;
    case Stage::Series2: return m_cfg.shotsPerSeries;                 // 5
    case Stage::Singles: return 2 * m_cfg.shotsPerSeries + (m_singleIndex - 1); // 10 + (n-1)
    default:             return 0;
    }
}

int Finals10mController::currentFineStageId() const
{
    return fineStageId(m_stage, m_singleIndex);
}

int Finals10mController::nextShotNumber() const
{
    const int limit = stageShotLimit();
    if (limit <= 0 || m_shotsInStage >= limit)
        return 0;
    return stageShotNumberBase() + m_shotsInStage + 1;
}

int Finals10mController::seriesNumber() const
{
    if (m_stage == Stage::Series1) return 1;
    if (m_stage == Stage::Series2) return 2;
    return 0;
}

void Finals10mController::enterStage(Stage s)
{
    m_stage = s;
    m_phaseStartScaled = scaledNow();
    m_seqStep = 0;
    m_shotsInStage = 0;
    m_warn1Fired = false;
    if (s != Stage::Singles)
        m_singleIndex = 0;
    if (stageShotLimit() > 0)
        setStageStatus(currentFineStageId(), StageStatus::InProgress);
    emit totalsChanged();

    switch (s) {
    case Stage::Presentation:
        if (m_cfg.ceremonyMode == CeremonyMode::Full) {
            issueCommand(CommandType::AthletesToLine,
                         QStringLiteral("ATHLETES TO THE LINE"));
            if (!m_athleteName.trimmed().isEmpty())
                issueCommand(CommandType::InfoNotice,
                             QStringLiteral("INTRODUCING — %1")
                                 .arg(m_athleteName.trimmed().toUpper()));
            m_segmentEndScaled = m_phaseStartScaled + m_cfg.introMs;
            m_seqStep = 0;
        } else { // Short
            issueCommand(CommandType::TakeYourPositions,
                         QStringLiteral("TAKE YOUR POSITIONS"));
            m_segmentEndScaled = m_phaseStartScaled + m_cfg.takePositionsDelayMs;
            m_seqStep = 1;
        }
        setWindow(WindowState::Closed);
        break;

    case Stage::PrepSighting:
        issueCommand(CommandType::PreparationSightingStart,
                     QStringLiteral("FIVE MINUTES PREPARATION AND SIGHTING TIME…START"));
        applyTargetModeInternal(TargetMode::Sighter);
        ++m_windowId;
        setWindow(WindowState::SightingOpen);
        m_segmentEndScaled = m_phaseStartScaled + m_cfg.preparationSightingMs;
        break;

    case Stage::Series1:
        beginCommandSequence(m_cfg.preSeriesGapMs);
        break;
    case Stage::Series2:
        beginCommandSequence(m_cfg.betweenGapMs);
        break;

    case Stage::Complete:
        setWindow(WindowState::Closed);
        m_tick.stop();
        m_segmentEndScaled = 0;
        issueCommand(CommandType::Unload, QStringLiteral("STOP…UNLOAD"));
        issueCommand(CommandType::ResultsFinal, QStringLiteral("RESULTS ARE FINAL"));
        if (m_store && m_store->active()) {
            const ta::rel::SessionState& st = m_store->state();
            submitEvent(ta::rel::DomainEvent(ta::rel::MatchCompleted{
                st.totalTenths, static_cast<qint16>(st.officials.size())}));
        }
        emit phaseChanged();
        emit advanceLabelChanged();
        emit countdownChanged();
        emit finalCompleted();
        return;

    default:
        setWindow(WindowState::Closed);
        m_segmentEndScaled = 0;
        break;
    }
    submitStagePhase(s);
    submitEvent(ta::rel::DomainEvent(ta::rel::StageEntered{
        static_cast<qint16>(currentFineStageId())}));
    emit phaseChanged();
    emit advanceLabelChanged();
    emit shotCountsChanged();
    emit countdownChanged();
}

void Finals10mController::enterSingle(int index)
{
    m_singleIndex = index;
    m_stage = Stage::Singles;
    m_phaseStartScaled = scaledNow();
    m_seqStep = 0;
    m_shotsInStage = 0;
    m_warn1Fired = false;
    setStageStatus(currentFineStageId(), StageStatus::InProgress);
    // First single follows Series2 with an announcer gap; subsequent singles
    // follow the previous single's STOP with the same gap.
    beginCommandSequence(m_cfg.betweenGapMs);
    submitStagePhase(Stage::Singles);
    submitEvent(ta::rel::DomainEvent(ta::rel::StageEntered{
        static_cast<qint16>(currentFineStageId())}));
    emit phaseChanged();
    emit advanceLabelChanged();
    emit shotCountsChanged();
    emit countdownChanged();
}

void Finals10mController::beginCommandSequence(qint64 gapMs)
{
    setWindow(WindowState::Closed);
    m_seqStep = 0;
    m_segmentEndScaled = m_phaseStartScaled + gapMs;
}

void Finals10mController::openCommandWindow(qint64 durationMs)
{
    applyTargetModeInternal(TargetMode::Match);
    ++m_windowId;
    setWindow(WindowState::MatchOpen);
    m_seqStep = 2;
    m_segmentEndScaled = scaledNow() + durationMs;
}

void Finals10mController::closeWindowAndStop()
{
    setWindow(WindowState::Closed);
    issueCommand(CommandType::Stop, QStringLiteral("STOP"));
    emit stageCompleted(currentFineStageId());
}

void Finals10mController::advanceAfterCommandStage()
{
    const Stage finished = m_stage;
    if (finished == Stage::Series1) {
        enterStage(Stage::Series2);
    } else if (finished == Stage::Series2) {
        enterSingle(1);
    } else if (finished == Stage::Singles) {
        const int completedShot = 2 * m_cfg.shotsPerSeries + m_singleIndex; // 10+n
        emitCheckpointIfDue(completedShot);
        if (m_singleIndex >= m_cfg.singleShotCount)
            enterStage(Stage::Complete);
        else
            enterSingle(m_singleIndex + 1);
    }
}

// ── the single time authority tick ─────────────────────────────────────────

void Finals10mController::tick()
{
    if (m_paused || !running())
        return;
    if (m_store && m_store->active()
            && m_store->persistenceHealth() != ta::rel::Health::Healthy)
        m_store->pumpRetryQueue();
    const qint64 now = scaledNow();

    switch (m_stage) {
    case Stage::Presentation:
        if (m_seqStep == 0 && now >= m_segmentEndScaled) {
            issueCommand(CommandType::TakeYourPositions,
                         QStringLiteral("TAKE YOUR POSITIONS"));
            m_seqStep = 1;
            m_segmentEndScaled = now + m_cfg.takePositionsDelayMs;   // 30s/10s hold
        } else if (m_seqStep == 1 && now >= m_segmentEndScaled) {
            enterStage(Stage::PrepSighting);
        }
        break;

    case Stage::PrepSighting:
        if (!m_warn1Fired && now >= m_phaseStartScaled + m_cfg.prepWarnMs) {
            m_warn1Fired = true;
            issueCommand(CommandType::ThirtySeconds, QStringLiteral("30 SECONDS"));
        }
        if (now >= m_segmentEndScaled) {
            setWindow(WindowState::Closed);
            issueCommand(CommandType::Stop, QStringLiteral("STOP…UNLOAD"));
            applyTargetModeInternal(TargetMode::Match);
            emit stageCompleted(currentFineStageId());
            enterStage(Stage::Series1);
        }
        break;

    case Stage::Series1:
    case Stage::Series2:
        if (m_seqStep == 0 && now >= m_segmentEndScaled) {
            m_seqStep = 1;
            issueCommand(CommandType::LoadSeries,
                         QStringLiteral("FOR THE NEXT COMPETITION SERIES…LOAD"));
            m_segmentEndScaled = now + m_cfg.loadDelayMs;
        } else if (m_seqStep == 1 && now >= m_segmentEndScaled) {
            issueCommand(CommandType::StartSeries, QStringLiteral("START"));
            openCommandWindow(m_cfg.seriesWindowMs);
        } else if (m_seqStep == 2 && now >= m_segmentEndScaled) {
            recordMissingShots();
            closeWindowAndStop();
            advanceAfterCommandStage();
        }
        break;

    case Stage::Singles:
        if (m_seqStep == 0 && now >= m_segmentEndScaled) {
            m_seqStep = 1;
            issueCommand(CommandType::LoadSingle,
                         QStringLiteral("FOR THE NEXT COMPETITION SHOT…LOAD"));
            m_segmentEndScaled = now + m_cfg.loadDelayMs;
        } else if (m_seqStep == 1 && now >= m_segmentEndScaled) {
            issueCommand(CommandType::StartSingle, QStringLiteral("START"));
            openCommandWindow(m_cfg.singleShotWindowMs);
        } else if (m_seqStep == 2 && now >= m_segmentEndScaled) {
            recordMissingShots();
            closeWindowAndStop();
            advanceAfterCommandStage();
        }
        break;

    default:
        break;
    }
    emit countdownChanged();
}

// ── commands, window ─────────────────────────────────────────────────────

void Finals10mController::issueCommand(CommandType type, const QString& text)
{
    QVariantMap ev;
    ev[QStringLiteral("commandId")] = ++m_commandSeq;
    ev[QStringLiteral("commandType")] = static_cast<int>(type);
    ev[QStringLiteral("typeName")] = commandTypeName(type);
    ev[QStringLiteral("text")] = text;
    ev[QStringLiteral("issuedAt")] = scaledNow();
    ev[QStringLiteral("stage")] = stageName(m_stage);
    ev[QStringLiteral("sequenceNumber")] = m_commandSeq;
    ev[QStringLiteral("audioCueId")] = commandTypeName(type).toLower();
    m_events.append(ev);
    {
        ta::rel::CommandIssued ce;
        ce.commandId = m_commandSeq;
        ce.commandType = static_cast<qint16>(type);
        ce.text = text;
        ce.sequenceNumber = static_cast<qint16>(m_commandSeq);
        ce.audioCueId = commandTypeName(type).toLower();
        ce.issuedAtMs = scaledNow();
        ce.effectiveAtMs = scaledNow();
        submitEvent(ta::rel::DomainEvent(ce));
    }
    m_commandText = text;
    qInfo() << "FINALS10M cmd" << m_commandSeq << commandTypeName(type) << text;
    emit commandIssued(ev);
}

void Finals10mController::setWindow(WindowState w)
{
    if (m_window == w)
        return;
    m_window = w;
    if (w != WindowState::Closed) {
        m_windowOpenedScaled = scaledNow();
        m_lastExternalId = -1;
        m_lastAcceptScaled = 0;
    }
    if (w == WindowState::Closed)
        submitEvent(ta::rel::DomainEvent(ta::rel::WindowClosed{
            static_cast<qint16>(m_windowId)}));
    else
        submitEvent(ta::rel::DomainEvent(ta::rel::WindowOpened{
            static_cast<qint16>(m_windowId)}));
    emit windowStateChanged();
}

void Finals10mController::applyTargetModeInternal(TargetMode m)
{
    if (m_targetMode == m)
        return;
    m_targetMode = m;
    emit targetModeChanged();
}

// ── labels ───────────────────────────────────────────────────────────────

QString Finals10mController::stageLabel() const
{
    switch (m_stage) {
    case Stage::Idle:         return QStringLiteral("IDLE");
    case Stage::Presentation: return QStringLiteral("PRESENTATION");
    case Stage::PrepSighting: return QStringLiteral("PREPARATION · SIGHTING");
    case Stage::Series1:      return QStringLiteral("COMPETITION SERIES 1");
    case Stage::Series2:      return QStringLiteral("COMPETITION SERIES 2");
    case Stage::Singles:      return QStringLiteral("SINGLE SHOT %1 / %2")
                                     .arg(m_singleIndex).arg(m_cfg.singleShotCount);
    case Stage::Complete:     return QStringLiteral("FINAL COURSE COMPLETE");
    case Stage::Aborted:      return QStringLiteral("ABORTED");
    }
    return QString();
}

QStringList Finals10mController::stepLabels() const
{
    return { QStringLiteral("PREP"), QStringLiteral("S1"), QStringLiteral("S2"),
             QStringLiteral("SINGLES"), QStringLiteral("DONE") };
}

int Finals10mController::stepIndex() const
{
    switch (m_stage) {
    case Stage::Idle:
    case Stage::Presentation:
    case Stage::PrepSighting: return 0;
    case Stage::Series1:      return 1;
    case Stage::Series2:      return 2;
    case Stage::Singles:      return 3;
    case Stage::Complete:
    case Stage::Aborted:      return 4;
    }
    return 0;
}

QVariantMap Finals10mController::checkpointTotals() const
{
    QVariantMap m;
    for (auto it = m_checkpointTotals.constBegin(); it != m_checkpointTotals.constEnd(); ++it)
        m[QString::number(it.key())] = it.value();
    return m;
}

QVariantList Finals10mController::seriesSubtotals() const
{
    QVariantList list;
    for (int stage = 2; stage <= 3 + m_cfg.singleShotCount; ++stage) {
        if (!m_seriesSubtotal.contains(stage))
            continue;
        QVariantMap m;
        m[QStringLiteral("stageId")] = stage;
        m[QStringLiteral("subtotal")] = m_seriesSubtotal.value(stage);
        list.append(m);
    }
    return list;
}

// ── Session Reliability Layer persistence (F1) ─────────────────────────────

QString Finals10mController::sessionJournalPath() const
{
    return m_store ? m_store->currentJournalPath() : QString();
}

int Finals10mController::persistenceHealth() const
{
    return m_store ? static_cast<int>(m_store->persistenceHealth())
                   : static_cast<int>(ta::rel::Health::Healthy);
}

void Finals10mController::beginJournalSession()
{
    if (!m_store)
        return;
    if (m_store->active())
        m_store->closeSession(ta::rel::CloseReason::Archive);

    ta::rel::SessionHeader header;
    header.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    header.appVersion = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("dev") : QCoreApplication::applicationVersion();
    header.athlete = m_athleteName.trimmed();
    header.matchType = m_cfg.matchType;
    header.discipline = m_cfg.discipline;
    header.config.officialShots = m_cfg.maximumMatchShots;
    header.config.seriesSize = m_cfg.shotsPerSeries;
    header.config.matchMs =
        m_cfg.seriesCount * m_cfg.seriesWindowMs
        + m_cfg.singleShotCount * m_cfg.singleShotWindowMs;

    const ta::rel::ReliabilityResult r = m_store->beginSession(header);
    m_journalFailureNotified = false;
    if (!r.ok) {
        qWarning() << "FINALS10M: beginSession failed:" << r.error.technicalDetail;
        m_journalFailureNotified = true;
        emit journalWriteFailed(sessionJournalPath(), r.error.technicalDetail);
    }
}

void Finals10mController::submitEvent(const ta::rel::DomainEvent& event)
{
    if (!m_store || !m_store->active())
        return;
    const ta::rel::SubmitResult r = m_store->submit(event);
    if (!r.ok)
        qWarning() << "FINALS10M: event" << ta::rel::eventTypeId(event)
                   << "rejected by reducer:" << r.error.technicalDetail;
}

void Finals10mController::submitStagePhase(Stage s)
{
    using namespace ta::rel;
    const qint16 id = static_cast<qint16>(currentFineStageId());
    switch (s) {
    case Stage::PrepSighting:
        submitEvent(DomainEvent(PreparationStarted{id}));
        submitEvent(DomainEvent(SightingStarted{id}));
        break;
    case Stage::Series1:
    case Stage::Series2:
    case Stage::Singles:
        submitEvent(DomainEvent(OfficialMatchStarted{id}));
        break;
    default:
        break;
    }
}

void Finals10mController::setStageStatus(int fineId, StageStatus status)
{
    const int cur = m_stageStatus.value(fineId, 0);
    if (cur == static_cast<int>(status)
            || cur >= static_cast<int>(StageStatus::Complete))
        return;
    m_stageStatus[fineId] = static_cast<int>(status);
    submitEvent(ta::rel::DomainEvent(ta::rel::StageStatusChanged{
        static_cast<qint16>(fineId), static_cast<qint8>(status)}));
}

ta::rel::ShotCore Finals10mController::buildShotCore(
    double xMm, double yMm, double score, int finalNumber, int withinStage,
    qint64 externalShotId, double direction, bool simulated) const
{
    using namespace ta::rel;
    ShotCore s;
    s.shotNumber = static_cast<qint16>(finalNumber);
    s.withinStage = static_cast<qint16>(withinStage);
    s.stageId = static_cast<qint16>(currentFineStageId());
    s.seriesIndex = static_cast<qint8>(seriesIndexFor(m_stage, m_singleIndex));
    CoordinateHundredthMm cx, cy;
    CoordinateHundredthMm::fromDouble(xMm, &cx);
    CoordinateHundredthMm::fromDouble(yMm, &cy);
    s.xHundredthMm = cx.raw();
    s.yHundredthMm = cy.raw();
    ScoreTenths st;
    ScoreTenths::fromDouble(score, &st);
    s.scoreTenths = st.raw();
    CentiDegrees cd;
    CentiDegrees::fromDouble(direction, &cd);
    s.directionCentiDeg = cd.raw();
    const qint64 now = scaledNow();
    const qint64 split = m_lastAcceptScaled > 0 ? (now - m_lastAcceptScaled)
                                                : (now - m_windowOpenedScaled);
    s.splitMs = static_cast<qint32>(split > 0 ? split : 0);
    s.windowId = static_cast<qint16>(m_windowId);
    s.targetMode = static_cast<qint8>(m_targetMode);
    s.externalId = externalShotId < 0 ? 0 : externalShotId;
    s.simulated = simulated;
    return s;
}

// ── recovery (F3/F5) ───────────────────────────────────────────────────────

void Finals10mController::loadRecoveredState(const ta::rel::RecoveredMatchState& recovered)
{
    using namespace ta::rel;
    const SessionState& s = recovered.state;

    const ReliabilityResult rr = m_store->resumeSession(recovered);
    m_journalFailureNotified = false;
    if (!rr.ok) {
        qWarning() << "FINALS10M: resumeSession failed:" << rr.error.technicalDetail;
        emit journalWriteFailed(sessionJournalPath(), rr.error.technicalDetail);
        return;
    }

    // Adopt config for the recovered discipline; restore EXCLUSIVELY from the
    // reducer state.
    m_cfg = techaim::finals10m::Finals10mConfig::forDiscipline(s.discipline);
    m_athleteName = s.athlete;

    const int fine = s.currentStageId;
    if (fine <= 1) {
        m_stage = Stage::PrepSighting;
        m_singleIndex = 0;
    } else if (fine == 2) {
        m_stage = Stage::Series1;
        m_singleIndex = 0;
    } else if (fine == 3) {
        m_stage = Stage::Series2;
        m_singleIndex = 0;
    } else {
        m_stage = Stage::Singles;
        m_singleIndex = fine - 3;                  // fine 4..17 -> single 1..14
    }

    m_officialShotCount = s.officials.size();
    m_sighterCount = s.sighters.size();
    m_cumulativeTotal = s.totalTenths / 10.0;

    // per-stage subtotals, checkpoint totals, and current-stage shot count —
    // all derived from the official record (in shot order).
    m_seriesSubtotal.clear();
    m_checkpointTotals.clear();
    m_stageStatus.clear();
    for (auto it = s.stageStatuses.constBegin(); it != s.stageStatuses.constEnd(); ++it)
        m_stageStatus.insert(it.key(), it.value());
    int shotsInCurrentStage = 0;
    qint64 maxExternalId = -1;
    double running = 0.0;
    for (const StateShotRecord& r : s.officials) {
        const double v = r.effectiveTenths() / 10.0;
        running += v;
        m_seriesSubtotal[r.shot.stageId] =
            m_seriesSubtotal.value(r.shot.stageId, 0.0) + v;
        if (checkpointLabelForShot(r.shot.shotNumber).size() > 0)
            m_checkpointTotals[r.shot.shotNumber] = running;
        if (r.shot.stageId == currentFineStageId())
            ++shotsInCurrentStage;
        maxExternalId = qMax(maxExternalId, r.shot.externalId);
    }
    m_shotsInStage = shotsInCurrentStage;
    m_lastExternalId = maxExternalId;
    m_shotEventId = static_cast<quint64>(s.officials.size() + s.sighters.size()
                                         + s.incidents.size());
    if (const auto* f = std::get_if<Finals10mState>(&s.disc))
        m_windowId = f->windowId;

    // Refill display sources (signals only — no re-journaling).
    m_officialShotRecords.clear();
    m_rejectionRecords.clear();
    m_missingShots.clear();
    m_events.clear();
    for (const StateShotRecord& r : s.sighters) {
        const double xMm = r.shot.xHundredthMm / 100.0;
        const double yMm = r.shot.yHundredthMm / 100.0;
        emit shotAccepted(makeShotRecord(true, xMm, yMm, r.effectiveTenths() / 10.0,
                                         0, 0, r.shot.externalId,
                                         r.shot.directionCentiDeg / 100.0,
                                         r.shot.simulated, r.seq));
    }
    for (const StateShotRecord& r : s.officials) {
        const double xMm = r.shot.xHundredthMm / 100.0;
        const double yMm = r.shot.yHundredthMm / 100.0;
        const QVariantMap rec = makeShotRecord(false, xMm, yMm,
                                               r.effectiveTenths() / 10.0,
                                               r.shot.shotNumber, r.shot.withinStage,
                                               r.shot.externalId,
                                               r.shot.directionCentiDeg / 100.0,
                                               r.shot.simulated, r.seq);
        m_officialShotRecords.append(rec);
        emit shotAccepted(rec);
    }

    // Re-establish a firing-ready window for the recovered stage; the countdown
    // resumes from the remaining time at the crash (spec §16), never restarted.
    m_mono.start();
    m_monoStarted = true;
    m_paused = false;
    m_pausedTotalRaw = 0;
    m_phaseStartScaled = 0;
    qint64 elapsedScaledMs = 0;
    if (recovered.stageClockStartMonoMs > 0
            && recovered.lastEventMonoMs >= recovered.stageClockStartMonoMs) {
        const qint64 elapsedReal =
            recovered.lastEventMonoMs - recovered.stageClockStartMonoMs;
        elapsedScaledMs = static_cast<qint64>(elapsedReal * m_timeScale);
    }
    restoreStageFiringState(elapsedScaledMs);
    m_tick.start();

    emit configChanged();
    emit totalsChanged();
    emit shotCountsChanged();
    emit phaseChanged();
    emit advanceLabelChanged();
    emit countdownChanged();
    qInfo() << "FINALS10M: recovered session" << recovered.sessionId
            << "stage" << stageName(m_stage) << "single" << m_singleIndex
            << "officials" << m_officialShotCount << "total" << m_cumulativeTotal;
}

void Finals10mController::restoreStageFiringState(qint64 elapsedScaledMs)
{
    const qint64 now = scaledNow();
    if (elapsedScaledMs < 0)
        elapsedScaledMs = 0;
    m_windowOpenedScaled = now;
    m_lastExternalId = -1;
    m_lastAcceptScaled = 0;
    m_warn1Fired = false;

    auto rebasedEnd = [now](qint64 elapsed, qint64 duration) {
        const qint64 remaining = duration - qBound<qint64>(0, elapsed, duration);
        return now + remaining;
    };

    switch (m_stage) {
    case Stage::PrepSighting:
        m_targetMode = TargetMode::Sighter;
        m_window = WindowState::SightingOpen;
        m_seqStep = 0;
        m_segmentEndScaled = rebasedEnd(elapsedScaledMs, m_cfg.preparationSightingMs);
        break;
    case Stage::Series1:
    case Stage::Series2:
        m_targetMode = TargetMode::Match;
        m_window = WindowState::MatchOpen;
        m_seqStep = 2;
        m_segmentEndScaled = rebasedEnd(elapsedScaledMs, m_cfg.seriesWindowMs);
        break;
    case Stage::Singles:
        m_targetMode = TargetMode::Match;
        m_window = WindowState::MatchOpen;
        m_seqStep = 2;
        m_segmentEndScaled = rebasedEnd(elapsedScaledMs, m_cfg.singleShotWindowMs);
        break;
    default:
        m_window = WindowState::Closed;
        m_segmentEndScaled = 0;
        break;
    }
    emit windowStateChanged();
    emit targetModeChanged();
    emit countdownChanged();
}

QVariantList Finals10mController::scanForRecovery()
{
    if (!m_recovery)
        m_recovery = std::make_unique<ta::rel::RecoveryCoordinator>();
    QVariantList out;
    for (const QVariant& v : m_recovery->scanForQml())
        out.append(v.toMap());
    return out;
}

bool Finals10mController::resumeFromRecovery(const QString& sessionId)
{
    if (!m_recovery)
        return false;
    ta::rel::RecoveredMatchState rec;
    ta::rel::ErrorInfo err;
    if (!m_recovery->buildRecoveredState(sessionId, &rec, &err)) {
        qWarning() << "FINALS10M: resumeFromRecovery failed:" << err.technicalDetail;
        emit journalWriteFailed(rec.journalPath, err.technicalDetail);
        return false;
    }
    if (!ta::rel::isFinals10mDiscipline(rec.state.discipline)) {
        qWarning() << "FINALS10M: refusing to resume a non-10m-final session";
        return false;
    }
    loadRecoveredState(rec);
    return true;
}

void Finals10mController::discardRecovery(const QString& sessionId)
{
    if (!m_recovery)
        m_recovery = std::make_unique<ta::rel::RecoveryCoordinator>();
    m_recovery->scan();
    m_recovery->archiveOrDiscard(sessionId, /*discarded*/ true);
}
