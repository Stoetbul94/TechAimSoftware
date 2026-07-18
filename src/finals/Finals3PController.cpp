#include "Finals3PController.h"
#include "FinalsReportBuilder.h"
#include "../reliability/storage/StoragePaths.h"
#include "../reliability/core/FixedPoint.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QProcessEnvironment>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

using namespace techaim::finals;

namespace {
const int kTickMs = 50;   // UI refresh; time itself is monotonic, not tick-counted
// M0 (Session Reliability Layer): the journal lives under the AppData tree
// (Sessions/Current), resolved by ta::rel::StoragePaths — never the process
// working directory. Event schema/content/ordering are unchanged.
}

// ── session journal (plan §9): append-only, crash-safe ──────────────────

QVariantMap Finals3PController::stageStatuses() const
{
    QVariantMap m;
    for (auto it = m_stageStatus.constBegin(); it != m_stageStatus.constEnd(); ++it)
        m[stageName(static_cast<Stage>(it.key()))] = it.value();
    return m;
}

QVariantMap Finals3PController::stageSubtotals() const
{
    QVariantMap m;
    for (auto it = m_stageSubtotalsMap.constBegin(); it != m_stageSubtotalsMap.constEnd(); ++it)
        m[stageName(static_cast<Stage>(it.key()))] = it.value();
    return m;
}

// Phase D1: immutable report assembled from the controller's stored session
// state only. `meta` may carry athlete / eventName / dateTime; sighterCount is
// filled in from the controller unless the caller overrides it.
QVariantMap Finals3PController::buildReport(const QVariantMap& meta) const
{
    QVariantMap m = meta;
    if (!m.contains(QStringLiteral("sighterCount")))
        m[QStringLiteral("sighterCount")] = m_sighterCount;
    const FinalsReportData data = FinalsReportBuilder::build(
        m_officialShotRecords, m_missingShots, m_rejectionRecords,
        stageStatuses(), stageSubtotals(), m_cumulativeTotal, m_events, m);
    return FinalsReportBuilder::toVariant(data);
}

void Finals3PController::setStageStatus(Stage st, techaim::finals::StageStatus status)
{
    const int id = static_cast<int>(st);
    const int cur = m_stageStatus.value(id, 0);
    // A finished verdict (Complete/Incomplete/Aborted) is never downgraded.
    if (cur == static_cast<int>(status)
            || cur >= static_cast<int>(techaim::finals::StageStatus::Complete))
        return;
    m_stageStatus[id] = static_cast<int>(status);
    submitEvent(ta::rel::DomainEvent(ta::rel::StageStatusChanged{
        static_cast<qint16>(id), static_cast<qint8>(status)}));
    emit stageStatusChanged(id, static_cast<int>(status));
}

// ── Session Reliability Layer persistence (M2) ──────────────────────────
// The controller submits typed domain events to its SessionStore; the store
// owns the journal, hash chain, retry queue and degraded-mode machinery.

QString Finals3PController::sessionJournalPath() const
{
    return m_store ? m_store->currentJournalPath() : QString();
}

int Finals3PController::persistenceHealth() const
{
    return m_store ? static_cast<int>(m_store->persistenceHealth())
                   : static_cast<int>(ta::rel::Health::Healthy);
}

void Finals3PController::beginJournalSession()
{
    if (!m_store)
        return;
    // Archive any still-open previous session before opening a new one (the
    // old archiveExistingJournal semantic, now handled by the store).
    if (m_store->active())
        m_store->closeSession(ta::rel::CloseReason::Archive);

    ta::rel::SessionHeader header;
    header.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    header.appVersion = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("dev") : QCoreApplication::applicationVersion();
    header.athlete = m_athleteName.trimmed();
    header.matchType = QStringLiteral("FINAL 35");
    header.discipline = ta::rel::Discipline::Finals3P;
    header.config.officialShots =
        m_cfg.kneelingShots + m_cfg.proneShots
        + 2 * m_cfg.seriesShots + 5 * m_cfg.singleShots;
    header.config.seriesSize = m_cfg.seriesShots;
    header.config.matchMs = m_cfg.stage1Ms;

    const ta::rel::ReliabilityResult r = m_store->beginSession(header);
    m_journalFailureNotified = false;
    if (!r.ok) {
        qWarning() << "FINALS3P: beginSession failed:" << r.error.technicalDetail;
        m_journalFailureNotified = true;
        emit journalWriteFailed(sessionJournalPath(), r.error.technicalDetail);
    }
}

// M3: rebuild a display shot-record (QVariantMap) from a reducer shot record
// so the UI models + report sources can be refilled on recovery. This is a
// pure projection of reducer state — no controller internals are read.
static QVariantMap recordFromReducerShot(const ta::rel::StateShotRecord& r,
                                         bool sighter)
{
    using namespace techaim::finals;
    ShotContext ctx;
    ctx.stage = static_cast<Stage>(r.shot.stageId);
    ctx.windowId = r.shot.windowId;
    ctx.targetMode = r.shot.targetMode;
    const double xMm = r.shot.xHundredthMm / 100.0;
    const double yMm = r.shot.yHundredthMm / 100.0;
    const double score = r.effectiveTenths() / 10.0;
    QVariantMap m = acceptedShotRecord(ctx, xMm, yMm, score, sighter,
                                       r.shot.shotNumber, r.shot.withinStage,
                                       r.seq, r.shot.externalId, r.shot.simulated);
    m[QStringLiteral("finalShotNumber")] = r.shot.shotNumber;
    m[QStringLiteral("direction")] =
        QString::number(r.shot.directionCentiDeg / 100.0, 'f', 2);
    return m;
}

void Finals3PController::loadRecoveredState(const ta::rel::RecoveredMatchState& recovered)
{
    using namespace ta::rel;
    const SessionState& s = recovered.state;

    // 1) Reopen the SAME journal in append mode and adopt the reducer-rebuilt
    //    state (the store writes RecoveryStarted/RecoveryCompleted).
    const ReliabilityResult rr = m_store->resumeSession(recovered);
    m_journalFailureNotified = false;
    if (!rr.ok) {
        qWarning() << "FINALS3P: resumeSession failed:" << rr.error.technicalDetail;
        emit journalWriteFailed(sessionJournalPath(), rr.error.technicalDetail);
        return;
    }

    // 2) Restore controller data — EXCLUSIVELY from the reducer state.
    m_athleteName = s.athlete;
    m_stage = static_cast<Stage>(s.currentStageId >= 0 ? s.currentStageId
                                                       : static_cast<int>(Stage::Idle));
    m_officialShotCount = s.officials.size();
    m_sighterCount = s.sighters.size();
    m_cumulativeTotal = s.totalTenths / 10.0;

    m_stageStatus.clear();
    for (auto it = s.stageStatuses.constBegin(); it != s.stageStatuses.constEnd(); ++it)
        m_stageStatus.insert(it.key(), it.value());
    m_stageSubtotalsMap.clear();
    for (auto it = s.stageSubtotalTenths.constBegin();
         it != s.stageSubtotalTenths.constEnd(); ++it)
        m_stageSubtotalsMap.insert(it.key(), it.value() / 10.0);

    // per-stage fired counts derived from the official records
    m_kneelingFired = m_proneFired = 0;
    int shotsInCurrentStage = 0;
    qint64 maxExternalId = -1;
    for (const StateShotRecord& r : s.officials) {
        if (r.shot.stageId == static_cast<int>(Stage::KneelingMatch)) ++m_kneelingFired;
        if (r.shot.stageId == static_cast<int>(Stage::ProneMatch))    ++m_proneFired;
        if (r.shot.stageId == static_cast<int>(m_stage))              ++shotsInCurrentStage;
        maxExternalId = qMax(maxExternalId, r.shot.externalId);
    }
    m_shotsInStage = shotsInCurrentStage;
    m_stageSubtotal = m_stageSubtotalsMap.value(static_cast<int>(m_stage), 0.0);
    m_lastExternalId = maxExternalId;
    // event-id / window counters continue past what is already journalled
    m_shotEventId = static_cast<quint64>(s.officials.size() + s.sighters.size()
                                         + s.incidents.size());
    if (const auto* f = std::get_if<Finals3PState>(&s.disc))
        m_windowId = f->windowId;

    // 3) Rebuild report sources + refill the UI models (display-only; the
    //    store is active but these are signals, NOT submits — no re-journaling).
    m_officialShotRecords.clear();
    m_rejectionRecords.clear();
    m_missingShots.clear();
    m_events.clear();
    for (const StateShotRecord& r : s.sighters)
        emit shotAccepted(recordFromReducerShot(r, /*sighter*/ true));
    for (const StateShotRecord& r : s.officials) {
        const QVariantMap rec = recordFromReducerShot(r, /*sighter*/ false);
        m_officialShotRecords.append(rec);
        emit shotAccepted(rec);
    }

    // 4) Re-establish a FIRING-READY window + target mode for the recovered
    //    stage so the athlete can continue immediately. The stage COUNTDOWN
    //    RESUMES from the remaining time at the crash (spec §16): elapsed =
    //    lastEventTm − stageClockStart (journal monotonic-ms), scaled into the
    //    controller's clock domain. Interruption time does not count; the timer
    //    is never restarted. These are live controller members — set directly,
    //    NOT via setWindow/issueCommand, so recovery adds no journal events.
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

    emit totalsChanged();
    emit shotCountsChanged();
    emit phaseChanged();
    emit advanceLabelChanged();
    emit countdownChanged();
    qInfo() << "FINALS3P: recovered session" << recovered.sessionId
            << "stage" << stageName(m_stage)
            << "officials" << m_officialShotCount
            << "total" << m_cumulativeTotal;
}

QVariantList Finals3PController::scanForRecovery()
{
    if (!m_recovery)
        m_recovery = std::make_unique<ta::rel::RecoveryCoordinator>();
    // Only surface finals sessions for resume in the finals controller; other
    // disciplines are ignored here (they adopt the same coordinator later).
    QVariantList out;
    for (const QVariant& v : m_recovery->scanForQml()) {
        const QVariantMap m = v.toMap();
        // The candidate's discipline label already tells the operator; we
        // leave filtering to the dialog, but finals-resume only injects finals.
        out.append(m);
    }
    return out;
}

bool Finals3PController::resumeFromRecovery(const QString& sessionId)
{
    if (!m_recovery)
        return false;
    ta::rel::RecoveredMatchState rec;
    ta::rel::ErrorInfo err;
    if (!m_recovery->buildRecoveredState(sessionId, &rec, &err)) {
        qWarning() << "FINALS3P: resumeFromRecovery failed:" << err.technicalDetail;
        emit journalWriteFailed(rec.journalPath, err.technicalDetail);
        return false;
    }
    if (rec.state.discipline != ta::rel::Discipline::Finals3P) {
        qWarning() << "FINALS3P: refusing to resume a non-finals session";
        return false;
    }
    loadRecoveredState(rec);
    return true;
}

void Finals3PController::discardRecovery(const QString& sessionId)
{
    if (!m_recovery)
        m_recovery = std::make_unique<ta::rel::RecoveryCoordinator>();
    m_recovery->scan();   // ensure the candidate is known
    m_recovery->archiveOrDiscard(sessionId, /*discarded*/ true);
}

void Finals3PController::submitEvent(const ta::rel::DomainEvent& event)
{
    if (!m_store || !m_store->active())
        return;
    const ta::rel::SubmitResult r = m_store->submit(event);
    // A reducer rejection is a journal-fidelity diagnostic — the controller's
    // own state machine remains authoritative for scoring/behaviour. Log it;
    // never let it alter the final.
    if (!r.ok)
        qWarning() << "FINALS3P: event" << ta::rel::eventTypeId(event)
                   << "rejected by reducer:" << r.error.technicalDetail;
}

// Map a finals stage entry to the reducer's coarse phase (so shot/sighter
// acceptance is legal in the reduced state), then record the specific stage.
void Finals3PController::submitStagePhase(Stage s)
{
    using namespace ta::rel;
    const qint16 id = static_cast<qint16>(s);
    switch (s) {
    case Stage::KneelingPrepSight:
        submitEvent(DomainEvent(PreparationStarted{id}));
        submitEvent(DomainEvent(SightingStarted{id}));
        break;
    case Stage::ProneSighting:
    case Stage::StandingSighting:
        submitEvent(DomainEvent(SightingStarted{id}));
        break;
    case Stage::KneelingMatch:
    case Stage::ProneMatch:
    case Stage::StandingSeries1:
    case Stage::StandingSeries2:
    case Stage::StandingSingle1:
    case Stage::StandingSingle2:
    case Stage::StandingSingle3:
    case Stage::StandingSingle4:
    case Stage::StandingSingle5:
        submitEvent(DomainEvent(OfficialMatchStarted{id}));
        break;
    default:
        break;   // Ceremony et al.: no shots, no phase change
    }
}

// M3: put the controller into a firing-ready window for the recovered stage.
// Mirrors how each stage establishes its window in enterStage()/tick(), but
// REBASED by elapsedScaledMs so the countdown resumes from where it was at the
// crash (spec §16, never restarted), and WITHOUT journaling (recovery injects
// no new events; the window/mode are live display+acceptance state only).
void Finals3PController::restoreStageFiringState(qint64 elapsedScaledMs)
{
    const qint64 now = scaledNow();
    if (elapsedScaledMs < 0)
        elapsedScaledMs = 0;
    m_windowOpenedScaled = now;
    m_lastExternalId = -1;        // per-window duplicate guard reset
    m_lastAcceptScaled = 0;
    m_stage1Started = false;
    m_s1Warn1 = m_s1Warn2 = false;
    m_warn1Fired = m_warn2Fired = false;

    // Clamp elapsed to a stage duration so a bad anchor can never push the
    // deadline into the past (which would instantly expire the window).
    auto rebasedEnd = [now](qint64 elapsed, qint64 duration) {
        const qint64 remaining = duration - qBound<qint64>(0, elapsed, duration);
        return now + remaining;
    };
    // For the shared stage-1 clock, place its START in the past by `elapsed`
    // so `segmentEnd = start + stage1Ms` yields the correct remaining time.
    auto rebasedStart = [now](qint64 elapsed, qint64 duration) {
        return now - qBound<qint64>(0, elapsed, duration);
    };

    switch (m_stage) {
    case Stage::KneelingPrepSight:
        m_targetMode = TargetMode::Sighter;
        m_window = WindowState::SightingOpen;
        m_seqStep = 0;
        m_segmentEndScaled = rebasedEnd(elapsedScaledMs, m_cfg.prepSightMs);
        break;
    case Stage::ProneSighting:
    case Stage::StandingSighting:
        m_targetMode = TargetMode::Sighter;
        m_window = WindowState::SightingOpen;
        m_stage1Started = true;
        m_stage1StartScaled = rebasedStart(elapsedScaledMs, m_cfg.stage1Ms);
        m_segmentEndScaled = m_stage1StartScaled + m_cfg.stage1Ms;
        break;
    case Stage::KneelingMatch:
    case Stage::ProneMatch:
        // Stage-1 shared 22:00 clock; seqStep 1 = past the announcement so the
        // tick runs the clock (not the LOAD/START sequence) and holds the
        // window open. Start is rebased so the shared clock resumes.
        m_targetMode = TargetMode::Match;
        m_window = WindowState::MatchOpen;
        m_seqStep = 1;
        m_stage1Started = true;
        m_stage1StartScaled = rebasedStart(elapsedScaledMs, m_cfg.stage1Ms);
        m_segmentEndScaled = m_stage1StartScaled + m_cfg.stage1Ms;
        break;
    case Stage::StandingSeries1:
    case Stage::StandingSeries2:
        m_targetMode = TargetMode::Match;
        m_window = WindowState::MatchOpen;
        m_seqStep = 2;            // firing
        m_segmentEndScaled = rebasedEnd(elapsedScaledMs, m_cfg.seriesMs);
        break;
    case Stage::StandingSingle1:
    case Stage::StandingSingle2:
    case Stage::StandingSingle3:
    case Stage::StandingSingle4:
    case Stage::StandingSingle5:
        m_targetMode = TargetMode::Match;
        m_window = WindowState::MatchOpen;
        m_seqStep = 2;            // firing
        m_segmentEndScaled = rebasedEnd(elapsedScaledMs, m_cfg.singleMs);
        break;
    default:                       // Ceremony/Idle/Complete/Aborted: no firing
        m_window = WindowState::Closed;
        m_segmentEndScaled = 0;
        break;
    }
    emit windowStateChanged();
    emit targetModeChanged();
    emit countdownChanged();
}

ta::rel::ShotCore Finals3PController::buildShotCore(
    const techaim::finals::ShotContext& ctx, double xMm, double yMm,
    double score, int finalNumber, int withinStage, qint64 externalShotId,
    double direction, bool simulated) const
{
    using namespace ta::rel;
    ShotCore s;
    s.shotNumber = static_cast<qint16>(finalNumber);
    s.withinStage = static_cast<qint16>(withinStage);
    s.stageId = static_cast<qint16>(stageId());
    s.seriesIndex = static_cast<qint8>(seriesIndexFor(ctx.stage));
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
    // Per-shot split in ms (same source as the display's timeComsumed).
    s.splitMs = static_cast<qint32>(
        ctx.hasPrevInWindow ? ctx.sincePrevShotMs : ctx.windowElapsedMs);
    s.windowId = static_cast<qint16>(ctx.windowId);
    s.targetMode = static_cast<qint8>(ctx.targetMode);
    // A negative external id (simulated / no hardware id) must not collide in
    // the reducer's duplicate check — normalise to 0 (no external id).
    s.externalId = externalShotId < 0 ? 0 : externalShotId;
    s.simulated = simulated;
    return s;
}

Finals3PController::Finals3PController(QObject* parent)
    : QObject(parent)
    , m_store(std::make_unique<ta::rel::SessionStore>())
{
    m_tick.setInterval(kTickMs);
    connect(&m_tick, &QTimer::timeout, this, &Finals3PController::tick);

    // Route the store's persistence signals to the controller's, so main.qml
    // can drive the persistence banner / error dialog without knowing about
    // the reliability layer directly (one integration point per §18).
    connect(m_store.get(), &ta::rel::SessionStore::journalWriteFailed,
            this, [this](QString path, QString detail) {
        qWarning() << "FINALS3P journal write failed at" << path << ":" << detail;
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
    m_pendingAdvance = false;
    m_lastExternalId = -1;
    m_lastAcceptScaled = 0;
    m_events.clear();
    m_journalFailureNotified = false;
    m_missingShots.clear();
    m_officialShotRecords.clear();
    m_rejectionRecords.clear();
    m_sighterCount = 0;
    m_cumulativeTotal = 0.0;
    m_stageSubtotal = 0.0;
    m_officialShotCount = 0;
    m_stageStatus.clear();
    m_stageSubtotalsMap.clear();
    emit totalsChanged();
    beginJournalSession();
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
    if (stageShotLimit() > 0)
        setStageStatus(m_stage, techaim::finals::StageStatus::Aborted);
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

// ── athlete Stage-1 transitions (single stage-aware action; plan §4) ─────
// The controller decides legality; QML only ever calls advanceStage1(). The
// legal chain is linear: K_MATCH -> P_SIGHT -> P_MATCH -> S_SIGHT -> ready.
// The ONLY automatic Match switch remains the EST-officer reset at prep end.

QString Finals3PController::advanceLabel() const
{
    switch (m_stage) {
    case Stage::KneelingMatch:
        return (m_stage1Started && m_shotsInStage >= m_cfg.kneelingShots)
            ? QStringLiteral("CHANGE TO PRONE — SIGHTERS") : QString();
    case Stage::ProneSighting: return QStringLiteral("START PRONE MATCH");
    case Stage::ProneMatch:
        return m_shotsInStage >= m_cfg.proneShots
            ? QStringLiteral("CHANGE TO STANDING — SIGHTERS") : QString();
    default:                   return QString();
    }
}

// ── bottom action bar (FIX2): controller-owned contextual primary action ──

bool Finals3PController::primaryActionVisible() const
{
    switch (m_stage) {
    case Stage::KneelingMatch:    return m_stage1Started;
    case Stage::ProneSighting:
    case Stage::ProneMatch:
    case Stage::StandingSighting: return true;
    case Stage::Complete:         return true;   // D3: VIEW REPORT
    default:                      return false;
    }
}

bool Finals3PController::primaryActionEnabled() const
{
    if (m_stage == Stage::Complete)
        return true;
    return !advanceLabel().isEmpty();
}

QString Finals3PController::primaryActionLabel() const
{
    const QString adv = advanceLabel();
    if (!adv.isEmpty())
        return adv;
    switch (m_stage) {
    case Stage::KneelingMatch:
        return QStringLiteral("KNEELING — %1 / %2").arg(m_shotsInStage).arg(m_cfg.kneelingShots);
    case Stage::ProneMatch:
        return QStringLiteral("PRONE — %1 / %2").arg(m_shotsInStage).arg(m_cfg.proneShots);
    case Stage::StandingSighting:
        return QStringLiteral("STANDING SIGHTING — WAIT FOR STOP");
    case Stage::Complete:
        return QStringLiteral("VIEW REPORT  →");
    default:
        return QString();
    }
}

void Finals3PController::advanceStage1()
{
    // Legal chain, forward only. StandingSighting has NO manual advance: the
    // controller sets the target to Match/Ready itself at the 22:00 STOP.
    const bool legal = (m_stage == Stage::KneelingMatch && m_stage1Started)
        || m_stage == Stage::ProneSighting
        || m_stage == Stage::ProneMatch;
    if (!legal) {
        QVariantMap info;
        info[QStringLiteral("reason")] = QStringLiteral("IllegalTransition");
        info[QStringLiteral("stageId")] = stageId();
        info[QStringLiteral("stageLabel")] = stageName(m_stage);
        qInfo() << "FINALS3P: illegal stage-1 advance ignored in" << stageName(m_stage);
        emit transitionRejected(info);
        return;
    }
    // HARD BLOCK (user decision, supersedes the earlier confirmation flow):
    // Kneeling/Prone Match may not advance until all 10 accepted match shots
    // exist. Early transition is forbidden in production; the only bypass is
    // the developer-only devForceAdvanceStage1().
    const bool underLimit = (m_stage == Stage::KneelingMatch || m_stage == Stage::ProneMatch)
                            && m_shotsInStage < stageShotLimit();
    if (underLimit) {
        QVariantMap info;
        info[QStringLiteral("reason")] = QStringLiteral("StageIncomplete");
        info[QStringLiteral("stageId")] = stageId();
        info[QStringLiteral("stageLabel")] = stageName(m_stage);
        info[QStringLiteral("shotsFired")] = m_shotsInStage;
        info[QStringLiteral("stageLimit")] = stageShotLimit();
        qInfo() << "FINALS3P: advance hard-blocked —" << m_shotsInStage << "/"
                << stageShotLimit() << "in" << stageName(m_stage);
        emit transitionRejected(info);
        return;
    }
    performStage1Advance();
}

void Finals3PController::executePrimaryAction()
{
    if (!primaryActionVisible() || !primaryActionEnabled())
        return;
    // After RESULTS ARE FINAL the action opens the finals report; the
    // controller stays UI-independent — QML routes the intent signal.
    if (m_stage == Stage::Complete) {
        emit reportRequested();
        return;
    }
    advanceStage1();
}

void Finals3PController::devForceAdvanceStage1()
{
    // Developer-only hard-block bypass (drawer control; never production UI).
    const bool legal = (m_stage == Stage::KneelingMatch && m_stage1Started)
        || m_stage == Stage::ProneSighting
        || m_stage == Stage::ProneMatch
        || (m_stage == Stage::StandingSighting && m_targetMode == TargetMode::Sighter);
    if (legal) {
        qInfo() << "FINALS3P: DEV force-advance from" << stageName(m_stage);
        performStage1Advance();
    }
}

void Finals3PController::confirmStage1Advance()
{
    // Inert: production advance is hard-blocked (see advanceStage1); the
    // developer bypass is devForceAdvanceStage1().
}

void Finals3PController::cancelStage1Advance()
{
    m_pendingAdvance = false;
}

void Finals3PController::performStage1Advance()
{
    switch (m_stage) {
    case Stage::KneelingMatch:
        applyTargetModeInternal(TargetMode::Sighter);
        enterStage(Stage::ProneSighting);
        break;
    case Stage::ProneSighting:
        applyTargetModeInternal(TargetMode::Match);
        enterStage(Stage::ProneMatch);
        break;
    case Stage::ProneMatch:
        applyTargetModeInternal(TargetMode::Sighter);
        enterStage(Stage::StandingSighting);
        break;
    case Stage::StandingSighting:
        // Ready for the commanded series: sighting closes, no stage change.
        applyTargetModeInternal(TargetMode::Match);
        setWindow(WindowState::Closed);
        emit advanceLabelChanged();
        break;
    default:
        break;
    }
}

void Finals3PController::applyTargetModeInternal(TargetMode m)
{
    if (m_targetMode == m)
        return;
    m_targetMode = m;
    emit targetModeChanged();
    emit advanceLabelChanged();
}

// ── shots ────────────────────────────────────────────────────────────────

void Finals3PController::registerShot(double xMm, double yMm, double decimalScore,
                                      int externalShotId, double direction)
{
    // Validation chain (plan §8). Order: active -> data -> window -> duplicate
    // -> defensive mode check -> stage limit.
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

void Finals3PController::simulateShot()
{
    // Dry-run control: identical acceptance path, flagged simulated.
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

QVariantMap Finals3PController::templateShotRecord() const
{
    // Role union used to pre-lock the shared ListModels at startup: the full
    // finals schema (which already reuses every qualification role name) plus
    // any extra qualification-only role.
    techaim::finals::ShotContext ctx;
    QVariantMap m = techaim::finals::acceptedShotRecord(ctx, 0, 0, 0, false,
                                                        0, 0, 0, -1, true);
    m[QStringLiteral("finalShotNumber")] = 0;
    return m;
}

techaim::finals::ShotContext Finals3PController::currentShotContext() const
{
    techaim::finals::ShotContext ctx;
    ctx.stage = m_stage;
    ctx.windowId = m_windowId;
    ctx.targetMode = targetMode();
    const qint64 now = scaledNow();
    ctx.windowElapsedMs = m_window != WindowState::Closed ? (now - m_windowOpenedScaled) : 0;
    ctx.stageElapsedMs = now - m_phaseStartScaled;
    ctx.sincePrevShotMs = m_lastAcceptScaled > 0 ? (now - m_lastAcceptScaled) : 0;
    // m_lastAcceptScaled is reset on every window open, so this is precisely
    // "a shot was already accepted in THIS window" (FIX-R3 split semantics).
    ctx.hasPrevInWindow = m_lastAcceptScaled > 0;
    return ctx;
}

void Finals3PController::acceptShot(bool sighter, double xMm, double yMm,
                                    double score, bool simulated, qint64 externalShotId,
                                    double direction)
{
    const techaim::finals::ShotContext ctx = currentShotContext();
    int finalNumber = 0, withinStage = 0;
    if (!sighter) {
        ++m_shotsInStage;
        if (m_stage == Stage::KneelingMatch) m_kneelingFired = m_shotsInStage;
        if (m_stage == Stage::ProneMatch)    m_proneFired = m_shotsInStage;
        withinStage = m_shotsInStage;
        finalNumber = stageShotNumberBase() + m_shotsInStage;
        m_stageSubtotal += score;
        m_cumulativeTotal += score;
        m_stageSubtotalsMap[stageId()] = m_stageSubtotalsMap.value(stageId(), 0.0) + score;
        if (m_shotsInStage >= stageShotLimit())
            setStageStatus(m_stage, techaim::finals::StageStatus::Complete);
        ++m_officialShotCount;
        emit totalsChanged();
        emit shotCountsChanged();
        // Completion flips the stage label and primary action ("KNEELING
        // COMPLETE" -> "CHANGE TO PRONE — SIGHTERS") the moment shot 10 lands.
        emit advanceLabelChanged();
        emit phaseChanged();
    }
    m_lastAcceptScaled = scaledNow();
    QVariantMap shot = techaim::finals::acceptedShotRecord(
        ctx, xMm, yMm, score, sighter, finalNumber, withinStage,
        ++m_shotEventId, externalShotId, simulated);
    shot[QStringLiteral("finalShotNumber")] = finalNumber;   // Phase-A compat key
    shot[QStringLiteral("direction")] = QString::number(direction, 'f', 2);
    // Phase D1: retain the pre-router record (decimal score, real direction —
    // the QML router's polar-display overrides never reach this copy).
    if (sighter)
        ++m_sighterCount;
    else
        m_officialShotRecords.append(shot);
    // M2: persist through the reliability store as a typed shot event.
    const ta::rel::ShotCore core = buildShotCore(
        ctx, xMm, yMm, score, finalNumber, withinStage, externalShotId,
        direction, simulated);
    if (sighter)
        submitEvent(ta::rel::DomainEvent(ta::rel::SighterAccepted{core}));
    else
        submitEvent(ta::rel::DomainEvent(ta::rel::ShotAccepted{core}));
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
                                    bool simulated, qint64 externalShotId)
{
    QVariantMap rej = techaim::finals::rejectionRecord(
        currentShotContext(), reason, xMm, yMm, ++m_shotEventId,
        externalShotId, simulated);
    // Part 8: stage completion is NORMAL — the athlete-facing text guides the
    // next action; the diagnostic reason stays in logs/incident records.
    if (reason == RejectReason::StageShotLimitReached) {
        if (m_stage == Stage::KneelingMatch)
            rej[QStringLiteral("displayText")] = QStringLiteral("KNEELING COMPLETE — CHANGE TO PRONE");
        else if (m_stage == Stage::ProneMatch)
            rej[QStringLiteral("displayText")] = QStringLiteral("PRONE COMPLETE — CHANGE TO STANDING");
    }
    m_rejectionRecords.append(rej);   // Phase D1: incident source for the report
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
    qInfo() << "FINALS3P: shot rejected —" << rejectReasonName(reason)
            << "in" << stageName(m_stage);
    emit shotRejected(rej);
}

// [P1 = Option B] Record the shortfall of expected match shots when a firing
// window/stage expires. MissingShot records only — never model entries.
void Finals3PController::recordMissingShots()
{
    auto addRange = [this](Stage s, int fired, int limit, int base) {
        for (int i = fired + 1; i <= limit; ++i) {
            const QVariantMap rec = techaim::finals::missingShotRecord(s, base + i);
            m_missingShots.append(rec);
            ta::rel::MissingShotRecorded ev;
            ev.expectedNumber = static_cast<qint16>(base + i);
            ev.stageId = static_cast<qint16>(s);
            ev.reason = QStringLiteral("TimeExpired");
            submitEvent(ta::rel::DomainEvent(ev));
            emit missingShotRecorded(rec);
        }
    };
    if (inStage1()) {
        addRange(Stage::KneelingMatch, m_kneelingFired, m_cfg.kneelingShots, 0);
        addRange(Stage::ProneMatch, m_proneFired, m_cfg.proneShots, 10);
        setStageStatus(Stage::KneelingMatch,
                       m_kneelingFired >= m_cfg.kneelingShots
                           ? techaim::finals::StageStatus::Complete
                           : techaim::finals::StageStatus::Incomplete);
        setStageStatus(Stage::ProneMatch,
                       m_proneFired >= m_cfg.proneShots
                           ? techaim::finals::StageStatus::Complete
                           : techaim::finals::StageStatus::Incomplete);
    } else if (isSeriesStage() || isSingleStage()) {
        addRange(m_stage, m_shotsInStage, stageShotLimit(), stageShotNumberBase());
        setStageStatus(m_stage, m_shotsInStage >= stageShotLimit()
                                    ? techaim::finals::StageStatus::Complete
                                    : techaim::finals::StageStatus::Incomplete);
    }
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
    m_stageSubtotal = 0.0;
    if (stageShotLimit() > 0)
        setStageStatus(s, techaim::finals::StageStatus::InProgress);
    emit totalsChanged();
    m_warn1Fired = m_warn2Fired = false;

    switch (s) {
    case Stage::Ceremony:
        if (m_cfg.ceremonyMode == CeremonyMode::Full) {
            issueCommand(CommandType::AthletesToLine, QStringLiteral("ATHLETES TO THE LINE"));
            // D4 ceremony polish: announce the athlete by name during the
            // introduction window (single-athlete training simulation). Only
            // when a name is configured — command ordering is otherwise
            // identical to Phase A.
            if (!m_athleteName.trimmed().isEmpty())
                issueCommand(CommandType::InfoNotice,
                             QStringLiteral("INTRODUCING — %1")
                                 .arg(m_athleteName.trimmed().toUpper()));
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
        // MatchCompleted must agree with the reduced totals — pass the
        // store's own reduced values so the reducer's equality check holds.
        if (m_store && m_store->active()) {
            const ta::rel::SessionState& st = m_store->state();
            submitEvent(ta::rel::DomainEvent(ta::rel::MatchCompleted{
                st.totalTenths, static_cast<qint16>(st.officials.size())}));
        }
        emit phaseChanged();
        // The primary action flips to VIEW REPORT on completion (D3); its
        // properties notify via advanceLabelChanged.
        emit advanceLabelChanged();
        emit countdownChanged();
        emit finalCompleted();
        return;

    default:
        setWindow(WindowState::Closed);
        m_segmentEndScaled = 0;
        break;
    }
    // Set the reducer phase for this stage, then record the stage entry.
    submitStagePhase(s);
    submitEvent(ta::rel::DomainEvent(ta::rel::StageEntered{
        static_cast<qint16>(s)}));
    emit phaseChanged();
    emit advanceLabelChanged();
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
    // M2: opportunistically drain the persistence retry queue on the UI tick
    // whenever storage has recovered (the degraded->restored path, §9D). The
    // shot path itself never waits on retries (never-refuse-to-score, §9C).
    if (m_store && m_store->active()
            && m_store->persistenceHealth() != ta::rel::Health::Healthy)
        m_store->pumpRetryQueue();
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
                // Controller places the standing target in MATCH/READY for the
                // first commanded series (sanctioned automatic switch at the
                // 22:00 STOP — the athlete has no manual action here).
                applyTargetModeInternal(TargetMode::Match);
                // [P1 = Option B] no synthetic shots — the kneeling/prone
                // shortfall becomes MissingShot records for the report layer.
                recordMissingShots();
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
            recordMissingShots();   // [P1=B] series expired with shots unfired
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
            recordMissingShots();   // [P1=B] single expired unfired
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
    qInfo() << "FINALS3P cmd" << m_commandSeq << commandTypeName(type) << text;
    // Audio: FinalsAudioService listens to commandIssued (D4) — the
    // controller itself has no audio dependency.
    emit commandIssued(ev);
}

void Finals3PController::setWindow(WindowState w)
{
    if (m_window == w)
        return;
    m_window = w;
    if (w != WindowState::Closed) {
        m_windowOpenedScaled = scaledNow();
        m_lastExternalId = -1;      // duplicate guard is per firing window
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

QString Finals3PController::stageLabel() const
{
    switch (m_stage) {
    case Stage::Idle:              return QStringLiteral("IDLE");
    case Stage::Ceremony:          return QStringLiteral("CEREMONY");
    case Stage::KneelingPrepSight: return QStringLiteral("PREPARATION · SIGHTING");
    case Stage::KneelingMatch:
        return (m_stage1Started && m_shotsInStage >= m_cfg.kneelingShots)
            ? QStringLiteral("KNEELING · COMPLETE") : QStringLiteral("KNEELING · MATCH");
    case Stage::ProneSighting:     return QStringLiteral("PRONE · SIGHTING");
    case Stage::ProneMatch:
        return m_shotsInStage >= m_cfg.proneShots
            ? QStringLiteral("PRONE · COMPLETE") : QStringLiteral("PRONE · MATCH");
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
