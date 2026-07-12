#include "Finals3PController.h"

#include <QApplication>
#include <QDateTime>
#include <QProcessEnvironment>
#include <QDebug>

using namespace techaim::finals;

namespace {
const int kTickMs = 50;   // UI refresh; time itself is monotonic, not tick-counted
}

Finals3PController::Finals3PController(QObject* parent)
    : QObject(parent)
{
    m_tick.setInterval(kTickMs);
    connect(&m_tick, &QTimer::timeout, this, &Finals3PController::tick);

    // Developer-only accelerated mode (never exposed in production UI).
    bool ok = false;
    const double envScale = QProcessEnvironment::systemEnvironment()
                                .value(QStringLiteral("TECHAIM_FINALS_TIMESCALE"))
                                .toDouble(&ok);
    if (ok && envScale > 0.0)
        m_timeScale = envScale;
}

// ── time ─────────────────────────────────────────────────────────────────

qint64 Finals3PController::scaledNow() const
{
    if (!m_monoStarted)
        return 0;
    qint64 raw = m_mono.elapsed() - m_pausedTotalRaw;
    if (m_paused)
        raw -= (m_mono.elapsed() - m_pauseStartRaw);
    return static_cast<qint64>(raw * m_timeScale);
}

qint64 Finals3PController::elapsedMs() const
{
    return m_monoStarted ? (scaledNow() - m_phaseStartScaled) : 0;
}

qint64 Finals3PController::remainingMs() const
{
    if (!m_monoStarted || m_segmentEndScaled <= 0)
        return 0;
    const qint64 r = m_segmentEndScaled - scaledNow();
    return r > 0 ? r : 0;
}

QString Finals3PController::remainingFormatted() const
{
    const qint64 totalSec = (remainingMs() + 999) / 1000;   // ceil: 22:00 -> 00:00
    return QStringLiteral("%1:%2")
        .arg(totalSec / 60, 2, 10, QLatin1Char('0'))
        .arg(totalSec % 60, 2, 10, QLatin1Char('0'));
}

void Finals3PController::setTimeScale(double s)
{
    if (s <= 0.0 || qFuzzyCompare(m_timeScale, s))
        return;
    // Changing mid-run would re-scale history; restrict to Idle for safety.
    if (m_stage == Stage::Idle) {
        m_timeScale = s;
        emit timeScaleChanged();
    } else {
        qWarning() << "FINALS3P: timeScale can only change while Idle";
    }
}

void Finals3PController::setCeremonyMode(int m)
{
    if (m < 0 || m > 2 || m == static_cast<int>(m_cfg.ceremonyMode))
        return;
    m_cfg.ceremonyMode = static_cast<CeremonyMode>(m);
    emit configChanged();
}

// ── main API ─────────────────────────────────────────────────────────────

void Finals3PController::startFinal()
{
    if (m_stage != Stage::Idle)
        return;
    m_mono.start();
    m_monoStarted = true;
    m_paused = false;
    m_pausedTotalRaw = 0;
    m_stage1Started = false;
    m_s1Warn1 = m_s1Warn2 = false;
    m_kneelingFired = m_proneFired = 0;
    m_commandSeq = 0;
    m_windowId = 0;
    m_shotEventId = 0;
    m_events.clear();
    m_tick.start();

    if (m_cfg.ceremonyMode == CeremonyMode::Skip)
        enterStage(Stage::KneelingPrepSight);
    else
        enterStage(Stage::Ceremony);
}

void Finals3PController::skipCeremony()
{
    if (m_stage == Stage::Ceremony)
        enterStage(Stage::KneelingPrepSight);
}

void Finals3PController::abortFinal()
{
    if (m_stage == Stage::Idle || m_stage == Stage::Complete || m_stage == Stage::Aborted)
        return;
    setWindow(WindowState::Closed);
    m_tick.stop();
    m_stage = Stage::Aborted;
    m_segmentEndScaled = 0;
    emit phaseChanged();
    emit countdownChanged();
}

void Finals3PController::resetFinal()
{
    m_tick.stop();
    m_monoStarted = false;
    m_paused = false;
    m_stage = Stage::Idle;
    m_window = WindowState::Closed;
    m_targetMode = TargetMode::Sighter;
    m_shotsInStage = 0;
    m_segmentEndScaled = 0;
    m_commandText.clear();
    emit phaseChanged();
    emit windowStateChanged();
    emit targetModeChanged();
    emit shotCountsChanged();
    emit countdownChanged();
}

void Finals3PController::pauseTrainingSimulation()
{
    if (!m_monoStarted || m_paused || !running())
        return;
    m_paused = true;
    m_pauseStartRaw = m_mono.elapsed();
    emit pausedChanged();
}

void Finals3PController::resumeTrainingSimulation()
{
    if (!m_paused)
        return;
    m_pausedTotalRaw += (m_mono.elapsed() - m_pauseStartRaw);
    m_paused = false;
    emit pausedChanged();
}

bool Finals3PController::running() const
{
    return m_stage != Stage::Idle && m_stage != Stage::Complete && m_stage != Stage::Aborted;
}

// ── athlete target-mode control (Stage 1 only; design doc §2) ────────────

void Finals3PController::setTargetMode(int mode)
{
    const TargetMode want = mode == 1 ? TargetMode::Match : TargetMode::Sighter;
    if (want == m_targetMode)
        return;

    // Legal athlete transitions only. The ONLY automatic Match switch is the
    // EST-officer reset at the end of the 5-minute preparation/sighting.
    if (m_stage == Stage::KneelingMatch && want == TargetMode::Sighter && m_stage1Started) {
        applyTargetModeInternal(want);
        enterStage(Stage::ProneSighting);
    } else if (m_stage == Stage::ProneSighting && want == TargetMode::Match) {
        applyTargetModeInternal(want);
        enterStage(Stage::ProneMatch);
    } else if (m_stage == Stage::ProneMatch && want == TargetMode::Sighter) {
        applyTargetModeInternal(want);
        enterStage(Stage::StandingSighting);
    } else if (m_stage == Stage::StandingSighting && want == TargetMode::Match) {
        // Ready for the commanded series: sighting closes, no stage change.
        applyTargetModeInternal(want);
        setWindow(WindowState::Closed);
    } else {
        qInfo() << "FINALS3P: illegal target-mode change ignored in stage"
                << stageName(m_stage) << "want" << mode;
    }
}

void Finals3PController::applyTargetModeInternal(TargetMode m)
{
    if (m_targetMode == m)
        return;
    m_targetMode = m;
    emit targetModeChanged();
}

// ── shots ────────────────────────────────────────────────────────────────

void Finals3PController::registerShot(double xMm, double yMm, double decimalScore)
{
    if (m_window == WindowState::Closed) {
        rejectShot(RejectReason::WindowClosed, xMm, yMm, false);
        return;
    }
    if (m_window == WindowState::SightingOpen) {
        acceptShot(true, xMm, yMm, decimalScore, false);
        return;
    }
    if (m_shotsInStage >= stageShotLimit()) {
        rejectShot(RejectReason::StageShotLimitReached, xMm, yMm, false);
        return;
    }
    acceptShot(false, xMm, yMm, decimalScore, false);
}

void Finals3PController::simulateShot()
{
    // Phase A dry-run control: identical acceptance path, flagged simulated.
    if (m_window == WindowState::Closed) {
        rejectShot(RejectReason::WindowClosed, 0, 0, true);
        return;
    }
    if (m_window == WindowState::SightingOpen) {
        acceptShot(true, 0, 0, 0, true);
        return;
    }
    if (m_shotsInStage >= stageShotLimit()) {
        rejectShot(RejectReason::StageShotLimitReached, 0, 0, true);
        return;
    }
    acceptShot(false, 0, 0, 0, true);
}

void Finals3PController::acceptShot(bool sighter, double xMm, double yMm,
                                    double score, bool simulated)
{
    QVariantMap shot;
    shot[QStringLiteral("eventId")] = ++m_shotEventId;
    shot[QStringLiteral("sighter")] = sighter;
    shot[QStringLiteral("simulated")] = simulated;
    shot[QStringLiteral("stageId")] = stageId();
    shot[QStringLiteral("stageLabel")] = stageName(m_stage);
    shot[QStringLiteral("position")] = positionLabel();
    shot[QStringLiteral("windowId")] = m_windowId;
    shot[QStringLiteral("targetModeAtReceipt")] = targetMode();
    shot[QStringLiteral("xMm")] = xMm;
    shot[QStringLiteral("yMm")] = yMm;
    shot[QStringLiteral("decimalScore")] = score;
    shot[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    shot[QStringLiteral("timeUsedInWindow")] = scaledNow() - m_phaseStartScaled;

    if (!sighter) {
        ++m_shotsInStage;
        if (m_stage == Stage::KneelingMatch) m_kneelingFired = m_shotsInStage;
        if (m_stage == Stage::ProneMatch)    m_proneFired = m_shotsInStage;
        shot[QStringLiteral("finalShotNumber")] = stageShotNumberBase() + m_shotsInStage;
        shot[QStringLiteral("shotWithinStage")] = m_shotsInStage;
        emit shotCountsChanged();
    }
    emit shotAccepted(shot);

    // Window auto-close rules (commanded stages only).
    if (!sighter) {
        if (isSingleStage() && m_shotsInStage >= m_cfg.singleShots) {
            closeWindowAndStop();
            advanceAfterCommandStage();
        } else if (isSeriesStage() && m_shotsInStage >= m_cfg.seriesShots
                   && m_cfg.stopSeriesWhenAthleteCompletes) {
            closeWindowAndStop();
            advanceAfterCommandStage();
        }
    }
}

void Finals3PController::rejectShot(RejectReason reason, double xMm, double yMm,
                                    bool simulated)
{
    QVariantMap rej;
    rej[QStringLiteral("eventId")] = ++m_shotEventId;
    rej[QStringLiteral("reason")] = reason == RejectReason::WindowClosed
        ? QStringLiteral("WindowClosed")
        : reason == RejectReason::StageShotLimitReached
            ? QStringLiteral("StageShotLimitReached")
            : QStringLiteral("DuplicateEvent");
    rej[QStringLiteral("stageId")] = stageId();
    rej[QStringLiteral("stageLabel")] = stageName(m_stage);
    rej[QStringLiteral("xMm")] = xMm;
    rej[QStringLiteral("yMm")] = yMm;
    rej[QStringLiteral("simulated")] = simulated;
    rej[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    qInfo() << "FINALS3P: shot rejected —" << rej.value(QStringLiteral("reason")).toString()
            << "in" << stageName(m_stage);
    emit shotRejected(rej);
}

int Finals3PController::stageShotLimit() const
{
    switch (m_stage) {
    case Stage::KneelingMatch:   return m_cfg.kneelingShots;
    case Stage::ProneMatch:      return m_cfg.proneShots;
    case Stage::StandingSeries1:
    case Stage::StandingSeries2: return m_cfg.seriesShots;
    case Stage::StandingSingle1:
    case Stage::StandingSingle2:
    case Stage::StandingSingle3:
    case Stage::StandingSingle4:
    case Stage::StandingSingle5: return m_cfg.singleShots;
    default:                     return 0;
    }
}

int Finals3PController::stageShotNumberBase() const
{
    switch (m_stage) {
    case Stage::KneelingMatch:   return 0;
    case Stage::ProneMatch:      return 10;
    case Stage::StandingSeries1: return 20;
    case Stage::StandingSeries2: return 25;
    case Stage::StandingSingle1: return 30;
    case Stage::StandingSingle2: return 31;
    case Stage::StandingSingle3: return 32;
    case Stage::StandingSingle4: return 33;
    case Stage::StandingSingle5: return 34;
    default:                     return 0;
    }
}

int Finals3PController::nextShotNumber() const
{
    const int limit = stageShotLimit();
    if (limit <= 0 || m_shotsInStage >= limit)
        return 0;
    return stageShotNumberBase() + m_shotsInStage + 1;
}

// ── stages ───────────────────────────────────────────────────────────────

bool Finals3PController::inStage1() const
{
    return m_stage == Stage::KneelingMatch || m_stage == Stage::ProneSighting
        || m_stage == Stage::ProneMatch || m_stage == Stage::StandingSighting;
}

bool Finals3PController::isSingleStage() const
{
    return m_stage >= Stage::StandingSingle1 && m_stage <= Stage::StandingSingle5;
}

void Finals3PController::enterStage(Stage s)
{
    m_stage = s;
    m_phaseStartScaled = scaledNow();
    m_seqStep = 0;
    m_shotsInStage = 0;
    m_warn1Fired = m_warn2Fired = false;

    switch (s) {
    case Stage::Ceremony:
        if (m_cfg.ceremonyMode == CeremonyMode::Full) {
            issueCommand(CommandType::AthletesToLine, QStringLiteral("ATHLETES TO THE LINE"));
            m_segmentEndScaled = m_phaseStartScaled + m_cfg.introMs;
            m_seqStep = 0;
        } else { // Short: straight to the hold
            issueCommand(CommandType::TakeYourPositions, QStringLiteral("TAKE YOUR POSITIONS"));
            m_segmentEndScaled = m_phaseStartScaled + m_cfg.holdMs;
            m_seqStep = 1;
        }
        setWindow(WindowState::Closed);
        break;

    case Stage::KneelingPrepSight:
        issueCommand(CommandType::PreparationSightingStart,
                     QStringLiteral("FIVE MINUTES PREPARATION AND SIGHTING TIME…START"));
        applyTargetModeInternal(TargetMode::Sighter);
        ++m_windowId;
        setWindow(WindowState::SightingOpen);
        m_segmentEndScaled = m_phaseStartScaled + m_cfg.prepSightMs;
        break;

    case Stage::KneelingMatch:
        // Announcement now; MATCH FIRING START (and the 22:00 clock) at +5 s.
        issueCommand(CommandType::StageOneAnnouncement,
                     QStringLiteral("FINALISTS HAVE TWENTY-TWO MINUTES TO FIRE TEN SHOTS "
                                    "IN EACH OF THE KNEELING AND PRONE POSITIONS AND "
                                    "PREPARE FOR THE STANDING POSITION"));
        setWindow(WindowState::Closed);
        m_segmentEndScaled = m_phaseStartScaled + m_cfg.loadDelayMs;
        m_seqStep = 0;
        break;

    case Stage::ProneSighting:
    case Stage::StandingSighting:
        ++m_windowId;
        setWindow(WindowState::SightingOpen);
        m_segmentEndScaled = m_stage1StartScaled + m_cfg.stage1Ms;   // shared clock
        break;

    case Stage::ProneMatch:
        ++m_windowId;
        setWindow(WindowState::MatchOpen);
        m_segmentEndScaled = m_stage1StartScaled + m_cfg.stage1Ms;
        break;

    case Stage::StandingSeries1:
        beginCommandSequence(m_cfg.preSeriesGapMs);
        break;
    case Stage::StandingSeries2:
    case Stage::StandingSingle1:
    case Stage::StandingSingle2:
    case Stage::StandingSingle3:
    case Stage::StandingSingle4:
    case Stage::StandingSingle5:
        beginCommandSequence(m_cfg.betweenGapMs);
        break;

    case Stage::Complete:
        setWindow(WindowState::Closed);
        m_tick.stop();
        m_segmentEndScaled = 0;
        issueCommand(CommandType::Unload, QStringLiteral("STOP…UNLOAD"));
        issueCommand(CommandType::ResultsFinal, QStringLiteral("RESULTS ARE FINAL"));
        emit phaseChanged();
        emit countdownChanged();
        emit finalCompleted();
        return;

    default:
        setWindow(WindowState::Closed);
        m_segmentEndScaled = 0;
        break;
    }
    emit phaseChanged();
    emit shotCountsChanged();
    emit countdownChanged();
}

void Finals3PController::beginCommandSequence(qint64 gapMs)
{
    setWindow(WindowState::Closed);
    m_seqStep = 0;   // 0 = announcer gap, 1 = loaded, 2 = firing
    m_segmentEndScaled = m_phaseStartScaled + gapMs;
}

void Finals3PController::openCommandWindow(qint64 durationMs)
{
    applyTargetModeInternal(TargetMode::Match);
    ++m_windowId;
    setWindow(WindowState::MatchOpen);
    m_seqStep = 2;
    m_segmentEndScaled = scaledNow() + durationMs;
}

void Finals3PController::closeWindowAndStop()
{
    setWindow(WindowState::Closed);
    issueCommand(CommandType::Stop, QStringLiteral("STOP"));
    emit stageCompleted(stageId());
}

void Finals3PController::infoNoticeForCompletedStage(Stage s)
{
    QString note;
    switch (s) {
    case Stage::StandingSeries2: note = QStringLiteral("8th and 7th places would be eliminated here"); break;
    case Stage::StandingSingle1: note = QStringLiteral("6th place would be eliminated here"); break;
    case Stage::StandingSingle2: note = QStringLiteral("5th place would be eliminated here"); break;
    case Stage::StandingSingle3: note = QStringLiteral("4th place would be eliminated here"); break;
    case Stage::StandingSingle4: note = QStringLiteral("Bronze medal (3rd place) would be decided here"); break;
    case Stage::StandingSingle5: note = QStringLiteral("Silver and gold would be decided here"); break;
    default: return;
    }
    issueCommand(CommandType::InfoNotice, note);
}

void Finals3PController::advanceAfterCommandStage()
{
    const Stage finished = m_stage;
    infoNoticeForCompletedStage(finished);
    switch (finished) {
    case Stage::StandingSeries1: enterStage(Stage::StandingSeries2); break;
    case Stage::StandingSeries2: enterStage(Stage::StandingSingle1); break;
    case Stage::StandingSingle1: enterStage(Stage::StandingSingle2); break;
    case Stage::StandingSingle2: enterStage(Stage::StandingSingle3); break;
    case Stage::StandingSingle3: enterStage(Stage::StandingSingle4); break;
    case Stage::StandingSingle4: enterStage(Stage::StandingSingle5); break;
    case Stage::StandingSingle5: enterStage(Stage::Complete); break;
    default: break;
    }
}

// ── the single time authority tick ───────────────────────────────────────

void Finals3PController::tick()
{
    if (m_paused || !running())
        return;
    const qint64 now = scaledNow();

    switch (m_stage) {
    case Stage::Ceremony:
        if (m_seqStep == 0 && now >= m_segmentEndScaled) {
            issueCommand(CommandType::TakeYourPositions, QStringLiteral("TAKE YOUR POSITIONS"));
            m_seqStep = 1;
            m_segmentEndScaled = now + m_cfg.holdMs;   // 30 s hold, NOT part of the 5:00
        } else if (m_seqStep == 1 && now >= m_segmentEndScaled) {
            enterStage(Stage::KneelingPrepSight);
        }
        break;

    case Stage::KneelingPrepSight:
        if (!m_warn1Fired && now >= m_phaseStartScaled + m_cfg.prepWarnMs) {
            m_warn1Fired = true;
            issueCommand(CommandType::ThirtySeconds, QStringLiteral("30 SECONDS"));
        }
        if (now >= m_segmentEndScaled) {
            setWindow(WindowState::Closed);
            issueCommand(CommandType::Stop, QStringLiteral("STOP"));
            // EST Technical Officer resets targets to MATCH — the ONLY
            // automatic Match switch in the final (design doc §2).
            applyTargetModeInternal(TargetMode::Match);
            emit stageCompleted(stageId());
            enterStage(Stage::KneelingMatch);
        }
        break;

    case Stage::KneelingMatch:
        if (m_seqStep == 0 && now >= m_segmentEndScaled) {
            m_seqStep = 1;
            issueCommand(CommandType::MatchFiringStart, QStringLiteral("MATCH FIRING START"));
            m_stage1Started = true;
            m_stage1StartScaled = now;
            m_s1Warn1 = m_s1Warn2 = false;
            ++m_windowId;
            setWindow(WindowState::MatchOpen);
            m_segmentEndScaled = m_stage1StartScaled + m_cfg.stage1Ms;
        }
        Q_FALLTHROUGH();
    case Stage::ProneSighting:
    case Stage::ProneMatch:
    case Stage::StandingSighting:
        if (m_stage1Started) {
            const qint64 s1 = now - m_stage1StartScaled;
            if (!m_s1Warn1 && s1 >= m_cfg.stage1Warn1Ms) {
                m_s1Warn1 = true;
                issueCommand(CommandType::FiveMinutes, QStringLiteral("FIVE MINUTES"));
            }
            if (!m_s1Warn2 && s1 >= m_cfg.stage1Warn2Ms) {
                m_s1Warn2 = true;
                issueCommand(CommandType::ThirtySeconds, QStringLiteral("THIRTY SECONDS"));
            }
            if (s1 >= m_cfg.stage1Ms) {
                setWindow(WindowState::Closed);
                issueCommand(CommandType::Stop, QStringLiteral("STOP"));
                // [P1 — UNRESOLVED] fillUnfiredShotsWithZero is disabled by
                // default: no synthetic shots; Phase B records MissingShot
                // entries instead. Phase A just closes the window.
                emit stageCompleted(stageId());
                enterStage(Stage::StandingSeries1);
            }
        }
        break;

    case Stage::StandingSeries1:
    case Stage::StandingSeries2:
        if (m_seqStep == 0 && now >= m_segmentEndScaled) {
            m_seqStep = 1;
            issueCommand(CommandType::LoadSeries,
                         QStringLiteral("FOR THE NEXT COMPETITION SERIES…LOAD"));
            m_segmentEndScaled = now + m_cfg.loadDelayMs;
        } else if (m_seqStep == 1 && now >= m_segmentEndScaled) {
            issueCommand(CommandType::StartSeries, QStringLiteral("START"));
            openCommandWindow(m_cfg.seriesMs);
        } else if (m_seqStep == 2 && now >= m_segmentEndScaled) {
            closeWindowAndStop();
            advanceAfterCommandStage();
        }
        break;

    case Stage::StandingSingle1:
    case Stage::StandingSingle2:
    case Stage::StandingSingle3:
    case Stage::StandingSingle4:
    case Stage::StandingSingle5:
        if (m_seqStep == 0 && now >= m_segmentEndScaled) {
            m_seqStep = 1;
            issueCommand(CommandType::LoadSingle,
                         QStringLiteral("FOR THE NEXT COMPETITION SHOT…LOAD"));
            m_segmentEndScaled = now + m_cfg.loadDelayMs;
        } else if (m_seqStep == 1 && now >= m_segmentEndScaled) {
            issueCommand(CommandType::StartSingle, QStringLiteral("START"));
            openCommandWindow(m_cfg.singleMs);
        } else if (m_seqStep == 2 && now >= m_segmentEndScaled) {
            closeWindowAndStop();
            advanceAfterCommandStage();
        }
        break;

    default:
        break;
    }
    emit countdownChanged();
}

// ── commands, window, labels ─────────────────────────────────────────────

void Finals3PController::issueCommand(CommandType type, const QString& text)
{
    QVariantMap ev;
    ev[QStringLiteral("commandId")] = ++m_commandSeq;
    ev[QStringLiteral("commandType")] = static_cast<int>(type);
    ev[QStringLiteral("typeName")] = commandTypeName(type);
    ev[QStringLiteral("text")] = text;
    ev[QStringLiteral("issuedAt")] = scaledNow();
    ev[QStringLiteral("effectiveAt")] = scaledNow();
    ev[QStringLiteral("stage")] = stageName(m_stage);
    ev[QStringLiteral("sequenceNumber")] = m_commandSeq;
    ev[QStringLiteral("audioCueId")] = commandTypeName(type).toLower();
    m_events.append(ev);
    m_commandText = text;
    qInfo() << "FINALS3P cmd" << m_commandSeq << commandTypeName(type) << text;
    // Phase A audio baseline: system beep (FinalsAudioService lands in Phase D).
    QApplication::beep();
    emit commandIssued(ev);
}

void Finals3PController::setWindow(WindowState w)
{
    if (m_window == w)
        return;
    m_window = w;
    emit windowStateChanged();
}

QString Finals3PController::stageLabel() const
{
    switch (m_stage) {
    case Stage::Idle:              return QStringLiteral("IDLE");
    case Stage::Ceremony:          return QStringLiteral("CEREMONY");
    case Stage::KneelingPrepSight: return QStringLiteral("PREPARATION · SIGHTING");
    case Stage::KneelingMatch:     return QStringLiteral("KNEELING · MATCH");
    case Stage::ProneSighting:     return QStringLiteral("PRONE · SIGHTING");
    case Stage::ProneMatch:        return QStringLiteral("PRONE · MATCH");
    case Stage::StandingSighting:  return QStringLiteral("STANDING · SIGHTING");
    case Stage::StandingSeries1:   return QStringLiteral("STANDING · SERIES 1");
    case Stage::StandingSeries2:   return QStringLiteral("STANDING · SERIES 2");
    case Stage::StandingSingle1:   return QStringLiteral("SINGLE SHOT 31");
    case Stage::StandingSingle2:   return QStringLiteral("SINGLE SHOT 32");
    case Stage::StandingSingle3:   return QStringLiteral("SINGLE SHOT 33");
    case Stage::StandingSingle4:   return QStringLiteral("SINGLE SHOT 34");
    case Stage::StandingSingle5:   return QStringLiteral("SINGLE SHOT 35");
    case Stage::Complete:          return QStringLiteral("FINAL COMPLETE");
    case Stage::Aborted:           return QStringLiteral("ABORTED");
    }
    return QString();
}

QString Finals3PController::positionLabel() const
{
    switch (m_stage) {
    case Stage::Ceremony:
    case Stage::KneelingPrepSight:
    case Stage::KneelingMatch:     return QStringLiteral("KNEELING");
    case Stage::ProneSighting:
    case Stage::ProneMatch:        return QStringLiteral("PRONE");
    case Stage::StandingSighting:
    case Stage::StandingSeries1:
    case Stage::StandingSeries2:
    case Stage::StandingSingle1:
    case Stage::StandingSingle2:
    case Stage::StandingSingle3:
    case Stage::StandingSingle4:
    case Stage::StandingSingle5:   return QStringLiteral("STANDING");
    default:                       return QString();
    }
}

QStringList Finals3PController::stepLabels() const
{
    // Singles 31-35 are normal commanded final shots — NOT "SHOOT-OFF"
    // (that label is reserved for future tie-breaking, design doc §14).
    return { QStringLiteral("PREP"), QStringLiteral("KNEEL"), QStringLiteral("PRONE"),
             QStringLiteral("STAND SIGHT"), QStringLiteral("S1"), QStringLiteral("S2"),
             QStringLiteral("31"), QStringLiteral("32"), QStringLiteral("33"),
             QStringLiteral("34"), QStringLiteral("35"), QStringLiteral("DONE") };
}

int Finals3PController::stepIndex() const
{
    switch (m_stage) {
    case Stage::Idle:
    case Stage::Ceremony:
    case Stage::KneelingPrepSight: return 0;
    case Stage::KneelingMatch:     return 1;
    case Stage::ProneSighting:
    case Stage::ProneMatch:        return 2;
    case Stage::StandingSighting:  return 3;
    case Stage::StandingSeries1:   return 4;
    case Stage::StandingSeries2:   return 5;
    case Stage::StandingSingle1:   return 6;
    case Stage::StandingSingle2:   return 7;
    case Stage::StandingSingle3:   return 8;
    case Stage::StandingSingle4:   return 9;
    case Stage::StandingSingle5:   return 10;
    case Stage::Complete:          return 11;
    case Stage::Aborted:           return 11;
    }
    return 0;
}

// ── RMS hooks (stubs; the future Range Management System replaces the local
//    time authority through these) ─────────────────────────────────────────

void Finals3PController::startPhaseFromServer(const QVariantMap& command)
{
    qInfo() << "FINALS3P: startPhaseFromServer (stub)" << command;
}

void Finals3PController::stopPhaseFromServer(const QVariantMap& command)
{
    qInfo() << "FINALS3P: stopPhaseFromServer (stub)" << command;
}

void Finals3PController::synchroniseClock(qint64 serverTimestamp, qint64 offset)
{
    qInfo() << "FINALS3P: synchroniseClock (stub)" << serverTimestamp << offset;
}

void Finals3PController::applyAuthoritativeState(const QVariantMap& snapshot)
{
    qInfo() << "FINALS3P: applyAuthoritativeState (stub)" << snapshot.keys();
}
