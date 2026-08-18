#include "QualificationController.h"

#include "incident/EstIncidentController.h"
#include "mode/OperatingMode.h"

#include <QCoreApplication>
#include <QUuid>
#include <QtGlobal>

using namespace ta::rel;

QualificationController::QualificationController(QObject* parent)
    : QObject(parent)
    , m_store(std::make_unique<SessionStore>())
{
    connect(m_store.get(), &SessionStore::persistenceHealthChanged,
            this, [this](Health h) {
                emit persistenceHealthChanged(static_cast<int>(h));
            });
    connect(m_store.get(), &SessionStore::journalWriteFailed,
            this, [this](QString path, QString detail) {
                emit journalWriteFailed(path, detail);
            });
    connect(m_store.get(), &SessionStore::stateChanged,
            this, [this]() {
                emit shotCountsChanged();
                emit totalsChanged();
            });
}

QualificationController::~QualificationController() = default;

// ── adopted rule authority ─────────────────────────────────

namespace {
// Machine values only. A display label would be worse than nothing here: it
// is translated, so the same competition journalled on a German range and on
// an English one would claim two different identities.
ta::rel::RuleAuthority authorityFromMap(const QVariantMap& m)
{
    ta::rel::RuleAuthority a;
    const auto str = [&m](const char* k) {
        return m.value(QLatin1String(k)).toString();
    };
    a.programmeId = str("programmeId");
    if (a.programmeId.isEmpty())
        return ta::rel::RuleAuthority();   // no profile -> LEGACY, explicitly
    a.rulesetId = str("rulesetId");
    a.rulesetVersion = str("rulesetVersion");
    a.ruleNumber = str("ruleNumber");
    a.programmeVariant = str("programmeVariant");
    a.competitionContext = str("competitionContext");
    a.scoringMode = str("scoringMode");
    a.timingModel = str("timingModel");
    a.targetStandardId = str("targetStandardId");
    a.disciplineId = str("disciplineId");
    a.distanceM = m.value(QStringLiteral("distanceM")).toInt();
    a.preparationMs = m.value(QStringLiteral("preparationMs")).toLongLong();
    a.matchMs = m.value(QStringLiteral("matchMs")).toLongLong();
    a.positionSequence = str("positionSequence");
    a.positionDurationsMs = str("positionDurationsMs");
    a.shotsPerPosition = str("shotsPerPosition");
    return a;
}

QVariantMap authorityToMap(const ta::rel::RuleAuthority& a)
{
    QVariantMap m;
    m[QStringLiteral("present")] = a.isPresent();
    m[QStringLiteral("programmeId")] = a.programmeId;
    m[QStringLiteral("rulesetId")] = a.rulesetId;
    m[QStringLiteral("rulesetVersion")] = a.rulesetVersion;
    m[QStringLiteral("ruleNumber")] = a.ruleNumber;
    m[QStringLiteral("programmeVariant")] = a.programmeVariant;
    m[QStringLiteral("competitionContext")] = a.competitionContext;
    m[QStringLiteral("scoringMode")] = a.scoringMode;
    m[QStringLiteral("timingModel")] = a.timingModel;
    m[QStringLiteral("targetStandardId")] = a.targetStandardId;
    m[QStringLiteral("disciplineId")] = a.disciplineId;
    m[QStringLiteral("distanceM")] = a.distanceM;
    m[QStringLiteral("preparationMs")] = static_cast<qlonglong>(a.preparationMs);
    m[QStringLiteral("matchMs")] = static_cast<qlonglong>(a.matchMs);
    m[QStringLiteral("positionSequence")] = a.positionSequence;
    m[QStringLiteral("positionDurationsMs")] = a.positionDurationsMs;
    m[QStringLiteral("shotsPerPosition")] = a.shotsPerPosition;
    m[QStringLiteral("authorityVersion")] = a.authorityVersion;
    return m;
}
} // namespace

void QualificationController::adoptRuleAuthority(const QVariantMap& authority)
{
    m_pendingAuthority = authorityFromMap(authority);
}

QVariantMap QualificationController::sessionRuleAuthority() const
{
    return authorityToMap(m_store->state().ruleAuthority);
}

// ── lifecycle ──────────────────────────────────────────────────────────────

bool QualificationController::startSession(const QString& disciplineId,
                                           const QString& matchType,
                                           const QString& athlete,
                                           int officialShots, qint64 matchMs,
                                           qint64 prepMs, int sighterLimit,
                                           const QString& lane,
                                           const QString& targetId)
{
    // Only the three qualification disciplines this seam serves. Finals and 3P
    // qualification / 25m are intentionally NOT accepted here.
    if (disciplineId == QLatin1String("AR10"))
        m_discipline = Discipline::AirRifle10m;
    else if (disciplineId == QLatin1String("AP10"))
        m_discipline = Discipline::AirPistol10m;
    else if (disciplineId == QLatin1String("PRONE50"))
        m_discipline = Discipline::Prone50m;
    else if (disciplineId == QLatin1String("3P50"))
        m_discipline = Discipline::ThreePositions50m;
    else {
        m_discipline = Discipline::None;
        return false;
    }

    // A previous session that never closed cleanly is archived out of the way
    // before opening the new one (mirrors the finals controller).
    if (m_store->active())
        m_store->closeSession(CloseReason::Archive);

    m_officialShots = officialShots;
    m_prepMs = prepMs;
    m_matchMs = matchMs;
    m_matchClockAnchored = false;

    SessionHeader header;
    header.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    header.appVersion = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("dev") : QCoreApplication::applicationVersion();
    header.athlete = athlete.trimmed();
    header.lane = lane;
    header.targetId = targetId;
    header.matchType = matchType;
    header.discipline = m_discipline;
    // F10: stamp the session with the operating mode it started in (empty →
    // Unknown/Legacy when no concrete mode was set, e.g. the harness).
    if (m_operatingMode >= 0)
        header.operatingMode =
            ta::mode::modeToConfigString(static_cast<ta::mode::Mode>(m_operatingMode));
    header.config.officialShots = officialShots;
    header.config.seriesSize = 10;             // ISSF qualification series size
    header.config.matchMs = matchMs;
    header.config.sighterLimit = sighterLimit; // -1 = unlimited
    // A position course records how it divides. Taken from the ADOPTED
    // definition when there is one (DSB 1.40 is 20/20/20, 1.60 is 40/40/40) and
    // otherwise from the course itself, which is how ISSF's 60-shot 3x20 keeps
    // working without declaring anything.
    if (m_discipline == Discipline::ThreePositions50m) {
        const QStringList perPos =
            m_pendingAuthority.shotsPerPosition.split(QChar(','), Qt::SkipEmptyParts);
        header.config.perPositionShots = perPos.size() == 3
            ? perPos.first().trimmed().toInt()
            : officialShots / 3;
    }
    // prepMs is carried as a timer anchor via TimerStarted, not part of config;
    // stored here only so the caller's intent is explicit at the call site.
    Q_UNUSED(prepMs);

    // The ADOPTED definition is snapshotted into the header here and nowhere
    // else. After this the session owns its rules: no later catalogue edition,
    // and no later UI selection, can change what governed it.
    header.ruleAuthority = m_pendingAuthority;
    qInfo("competition created: %s discipline=%s shots=%d",
          qUtf8Printable(header.ruleAuthority.auditLine()),
          ta::rel::disciplineId(m_discipline), officialShots);
    const ReliabilityResult r = m_store->beginSession(header);
    m_journalFailureNotified = false;
    if (!r.ok) {
        m_journalFailureNotified = true;
        emit journalWriteFailed(journalPath(), r.error.technicalDetail);
        return false;
    }
    // A position course starts IN its first position, and that is a
    // recorded fact rather than an assumption a reader has to make: the
    // reducer holds -1 until something says otherwise, and a shot fired
    // before the first position change would then belong to no position.
    if (m_discipline == Discipline::ThreePositions50m)
        submitEvent(DomainEvent(PositionChanged{0}));
    // Consumed. A profile must never leak into a later, unrelated session.
    m_pendingAuthority = RuleAuthority();
    emit sessionChanged();
    emit shotCountsChanged();
    emit totalsChanged();
    return true;
}

void QualificationController::beginPreparation()
{
    submitEvent(DomainEvent(PreparationStarted{kSightingStageId}));
    // Anchor the combined 15-min Preparation+Sighting clock (Phase C). The
    // reducer records durationMs + startedAtMonoMs so a crash during sighting
    // recovers the frozen remaining prep time.
    if (m_prepMs > 0)
        submitEvent(DomainEvent(TimerStarted{TimerId::Preparation, m_prepMs}));
}

void QualificationController::beginSighting()
{
    submitEvent(DomainEvent(SightingStarted{kSightingStageId}));
}

bool QualificationController::changePosition(int positionIndex)
{
    if (!m_store || !m_store->active())
        return false;
    if (m_discipline != Discipline::ThreePositions50m)
        return false;      // a course without positions cannot change position
    if (positionIndex < 0 || positionIndex > 2)
        return false;
    const SubmitResult r = m_store->submit(DomainEvent(PositionChanged{
        static_cast<qint8>(positionIndex)}));
    if (!r.ok)
        return false;
    qInfo("competition position: %s position=%d shots=%d/%d",
          qUtf8Printable(m_store->state().ruleAuthority.auditLine()),
          positionIndex, officialShotCount(), m_officialShots);
    emit sessionChanged();
    return true;
}

int QualificationController::currentPositionIndex() const
{
    if (!m_store || m_discipline != Discipline::ThreePositions50m)
        return -1;
    return m_store->state().positionIndex;
}

int QualificationController::matchShotsInPosition(int positionIndex) const
{
    if (!m_store)
        return 0;
    int n = 0;
    for (const StateShotRecord& r : m_store->state().officials)
        if (r.shot.seriesIndex == positionIndex && !r.invalidated)
            ++n;
    return n;
}

void QualificationController::beginOfficialMatch()
{
    submitEvent(DomainEvent(OfficialMatchStarted{kOfficialStageId}));
    if (m_matchClockAnchored)
        return;   // a position change re-enters the match; the clock runs on
    m_matchClockAnchored = true;
    // Anchor the official match clock (Phase C): TimerStarted resets the
    // reducer timer to the match duration, so a crash during the match recovers
    // the frozen remaining match time (never the full duration).
    if (m_matchMs > 0)
        submitEvent(DomainEvent(TimerStarted{TimerId::Match, m_matchMs}));
}

// ── shots ────────────────────────────────────────────────────────────────

bool QualificationController::submitSighter(double xMm, double yMm, double score,
                                            qint64 externalId, double directionDeg,
                                            bool simulated)
{
    return submitShot(true, xMm, yMm, score, externalId, directionDeg, simulated);
}

bool QualificationController::submitOfficial(double xMm, double yMm, double score,
                                             qint64 externalId, double directionDeg,
                                             bool simulated)
{
    return submitShot(false, xMm, yMm, score, externalId, directionDeg, simulated);
}

bool QualificationController::submitShot(bool sighter, double xMm, double yMm,
                                         double score, qint64 externalId,
                                         double directionDeg, bool simulated)
{
    if (!m_store || !m_store->active())
        return false;

    // F10 authoritative input-source gate: a shot whose origin does not match
    // the running operating mode is refused here — before any reducer submit —
    // so a Demo click cannot score in Live nor a physical shot in Demo, and no
    // journal event is created for the rejected shot. (-1 = unset/permissive for
    // standalone harness use; the runtime always sets a concrete mode.)
    if (m_operatingMode >= 0
        && !ta::mode::sourceAllowed(static_cast<ta::mode::Mode>(m_operatingMode),
               simulated ? ta::mode::ShotSource::Simulated : ta::mode::ShotSource::Physical))
        return false;

    // Phase E resume gate: while an unresolved EST incident requires an
    // authorised decision, OFFICIAL shots are refused at the controller —
    // never merely a disabled button. Sighters stay allowed (the authorised
    // recovery-sighting phase depends on them). See EstIncidentController.
    if (!sighter && EstIncidentController::officialsBlocked(m_store->state()))
        return false;

    // Configured official cap: a shot beyond the cap is refused here, so the
    // journal never contains a 61st (for a 60-shot event). Sighters are
    // unlimited. m_officialShots <= 0 means uncapped (free practice).
    if (!sighter && m_officialShots > 0
            && m_store->state().officials.size() >= m_officialShots)
        return false;

    // Official shot number is authoritative from the reducer (never the UI
    // index): the next official is officials.size() + 1; sighters carry 0.
    const qint16 shotNumber = sighter
        ? qint16(0)
        : static_cast<qint16>(m_store->state().officials.size() + 1);

    // Fixed-point conversion (score is decimal for rifle, floored integer for
    // full-ring pistol — decided by the caller). Clamp to the reducer's
    // accepted 0..110 tenths range (11.0 defensive ceiling).
    qint16 scoreTenths = static_cast<qint16>(qRound(score * 10.0));
    scoreTenths = static_cast<qint16>(qBound<int>(0, scoreTenths, 110));

    ShotCore core;
    core.shotNumber = shotNumber;
    core.withinStage = shotNumber;              // single stage
    core.stageId = sighter ? kSightingStageId : kOfficialStageId;
    // The POSITION this shot was fired in, so a three-position course keeps its
    // groups in the journal and not only on screen. 0 for every single-position
    // discipline, which is what it always was.
    core.seriesIndex = static_cast<qint8>(qMax(0, m_store->state().positionIndex));
    core.xHundredthMm = static_cast<qint32>(qRound(xMm * 100.0));
    core.yHundredthMm = static_cast<qint32>(qRound(yMm * 100.0));
    core.scoreTenths = scoreTenths;
    core.directionCentiDeg = static_cast<qint32>(qRound(directionDeg * 100.0));
    core.splitMs = 0;
    core.windowId = 0;
    core.targetMode = 0;
    core.externalId = externalId;
    core.simulated = simulated;

    const SubmitResult r = sighter
        ? m_store->submit(DomainEvent(SighterAccepted{core}))
        : m_store->submit(DomainEvent(ShotAccepted{core}));
    if (!r.ok)
        return false;   // reducer refused (duplicate / phase) — not accepted

    // Durable submit succeeded (journalled or elastically queued): project it.
    QVariantMap rec;
    rec[QStringLiteral("isSighter")] = sighter;
    rec[QStringLiteral("shotNumber")] = shotNumber;
    rec[QStringLiteral("calculatedscore")] = scoreTenths / 10.0;
    rec[QStringLiteral("xmm")] = xMm;
    rec[QStringLiteral("ymm")] = yMm;
    rec[QStringLiteral("direction")] = QString::number(directionDeg, 'f', 2);
    rec[QStringLiteral("externalId")] = externalId;
    rec[QStringLiteral("simulated")] = simulated;
    emit shotAccepted(rec);
    emit shotCountsChanged();
    emit totalsChanged();
    return true;
}

void QualificationController::completeMatch()
{
    if (!m_store || !m_store->active())
        return;
    const SessionState& st = m_store->state();
    submitEvent(DomainEvent(MatchCompleted{
        st.totalTenths, static_cast<qint16>(st.officials.size())}));
}

void QualificationController::closeSession()
{
    if (m_store && m_store->active())
        m_store->closeSession(CloseReason::Clean);
    emit sessionChanged();
}

void QualificationController::pumpRetryQueue()
{
    if (m_store && m_store->active()
            && m_store->persistenceHealth() != Health::Healthy)
        m_store->pumpRetryQueue();
}

void QualificationController::submitEvent(const DomainEvent& event)
{
    if (!m_store || !m_store->active())
        return;
    m_store->submit(event);
}

// ── Phase D: crash recovery / resume ────────────────────────────────────────

QVariantList QualificationController::scanForRecovery()
{
    if (!m_recovery)
        m_recovery = std::make_unique<RecoveryCoordinator>();
    return m_recovery->scanForQml();
}

bool QualificationController::resumeFromRecovery(const QString& sessionId)
{
    if (!m_recovery)
        m_recovery = std::make_unique<RecoveryCoordinator>();
    // Ensure the candidate is cached before we build its recovered state — the
    // scan that populated the dialog may have run on a DIFFERENT controller's
    // coordinator (e.g. the startup scan), so scan here too. Idempotent.
    m_recovery->scan();
    RecoveredMatchState rec;
    ErrorInfo err;
    if (!m_recovery->buildRecoveredState(sessionId, &rec, &err)) {
        emit journalWriteFailed(rec.journalPath, err.technicalDetail);
        return false;
    }
    // Only the three qualification disciplines this seam owns. Anything else
    // (finals, 3P-qual, 25m) is refused — never resumed here.
    const Discipline d = rec.state.discipline;
    if (d != Discipline::AirRifle10m && d != Discipline::AirPistol10m
            && d != Discipline::Prone50m && d != Discipline::ThreePositions50m)
        return false;
    loadRecoveredState(rec);
    return true;
}

void QualificationController::discardRecovery(const QString& sessionId)
{
    if (!m_recovery)
        m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();   // ensure the candidate is known
    m_recovery->archiveOrDiscard(sessionId, /*discarded*/ true);
}

void QualificationController::loadRecoveredState(const RecoveredMatchState& recovered)
{
    const SessionState& s = recovered.state;
    // Reopen the SAME journal in append mode and adopt the reducer-rebuilt
    // state (the store writes RecoveryStarted/RecoveryCompleted). No new
    // session, no archive of the interrupted journal.
    const ReliabilityResult rr = m_store->resumeSession(recovered);
    if (!rr.ok) {
        emit journalWriteFailed(journalPath(), rr.error.technicalDetail);
        return;
    }
    // Restore controller config EXCLUSIVELY from the reducer state.
    m_discipline = s.discipline;
    m_officialShots = s.config.officialShots;
    m_matchMs = s.config.matchMs;
    m_recoveredLastEventMonoMs = recovered.lastEventMonoMs;
    // The recovered journal already carries the anchor; re-anchoring on the
    // next position change would restart the master clock.
    m_matchClockAnchored = (s.timer.timerId == TimerId::Match && s.timer.active);
    m_recovered = true;
    // The recovered session brings its OWN rules. Whatever the operator has
    // since selected in the UI is irrelevant to it, and the catalogue is not
    // consulted - that is the entire point of snapshotting the authority.
    qInfo("competition recovered: %s discipline=%s position=%d shots=%d/%d "
          "perPosition=%d",
          qUtf8Printable(s.ruleAuthority.auditLine()),
          ta::rel::disciplineId(s.discipline), s.positionIndex,
          int(s.officials.size()), s.config.officialShots,
          s.config.perPositionShots);
    emit sessionChanged();
    emit shotCountsChanged();
    emit totalsChanged();
    emit persistenceHealthChanged(persistenceHealth());
}

namespace {
QVariantMap recordFromReducerShot(const StateShotRecord& r, bool sighter)
{
    QVariantMap m;
    m[QStringLiteral("isSighter")] = sighter;
    m[QStringLiteral("shotNumber")] = r.shot.shotNumber;
    m[QStringLiteral("calculatedscore")] = r.effectiveTenths() / 10.0;
    m[QStringLiteral("xmm")] = r.shot.xHundredthMm / 100.0;
    m[QStringLiteral("ymm")] = r.shot.yHundredthMm / 100.0;
    m[QStringLiteral("direction")] =
        QString::number(r.shot.directionCentiDeg / 100.0, 'f', 2);
    m[QStringLiteral("externalId")] = static_cast<qlonglong>(r.shot.externalId);
    m[QStringLiteral("simulated")] = r.shot.simulated;
    return m;
}
} // namespace

QVariantList QualificationController::recoveredShots() const
{
    QVariantList out;
    if (!m_store)
        return out;
    const SessionState& s = m_store->state();
    for (const StateShotRecord& r : s.sighters)
        out.append(recordFromReducerShot(r, /*sighter*/ true));
    for (const StateShotRecord& r : s.officials)
        out.append(recordFromReducerShot(r, /*sighter*/ false));
    return out;
}

qint64 QualificationController::recoveredRemainingMs() const
{
    if (!m_store)
        return 0;
    // Phase E: the ONE authoritative computation — pause-aware (an incident
    // freezes the clock via TimerPaused) plus authorised credits. Crash time
    // and incident time never consume competition time.
    return EstIncidentController::remainingCompetitionMsFor(
        m_store->state(), m_recoveredLastEventMonoMs);
}

int QualificationController::recoveredPhaseId() const
{
    return m_store ? static_cast<int>(m_store->state().phase) : 0;
}

int QualificationController::recoveredTimerId() const
{
    return m_store ? static_cast<int>(m_store->state().timer.timerId) : 0;
}

qint64 QualificationController::recoveredMaxExternalId() const
{
    if (!m_store)
        return 0;
    qint64 mx = 0;
    const SessionState& s = m_store->state();
    for (const StateShotRecord& r : s.sighters)
        mx = qMax(mx, r.shot.externalId);
    for (const StateShotRecord& r : s.officials)
        mx = qMax(mx, r.shot.externalId);
    return mx;
}

// ── reads ──────────────────────────────────────────────────────────────────

int QualificationController::persistenceHealth() const
{
    return m_store ? static_cast<int>(m_store->persistenceHealth())
                   : static_cast<int>(Health::Healthy);
}

QString QualificationController::journalPath() const
{
    return m_store ? m_store->currentJournalPath() : QString();
}

QString QualificationController::sessionId() const
{
    return m_store ? m_store->state().sessionId : QString();
}

bool QualificationController::active() const
{
    return m_store && m_store->active();
}

int QualificationController::officialShotCount() const
{
    return m_store ? m_store->state().officials.size() : 0;
}

int QualificationController::sighterCount() const
{
    return m_store ? m_store->state().sighters.size() : 0;
}

int QualificationController::nextOfficialShotNumber() const
{
    return officialShotCount() + 1;
}

double QualificationController::totalDecimal() const
{
    return m_store ? m_store->state().totalTenths / 10.0 : 0.0;
}
