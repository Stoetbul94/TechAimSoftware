#include "QualificationController.h"

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
    else {
        m_discipline = Discipline::None;
        return false;
    }

    // A previous session that never closed cleanly is archived out of the way
    // before opening the new one (mirrors the finals controller).
    if (m_store->active())
        m_store->closeSession(CloseReason::Archive);

    m_officialShots = officialShots;

    SessionHeader header;
    header.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    header.appVersion = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("dev") : QCoreApplication::applicationVersion();
    header.athlete = athlete.trimmed();
    header.lane = lane;
    header.targetId = targetId;
    header.matchType = matchType;
    header.discipline = m_discipline;
    header.config.officialShots = officialShots;
    header.config.seriesSize = 10;             // ISSF qualification series size
    header.config.matchMs = matchMs;
    header.config.sighterLimit = sighterLimit; // -1 = unlimited
    // prepMs is carried as a timer anchor via TimerStarted, not part of config;
    // stored here only so the caller's intent is explicit at the call site.
    Q_UNUSED(prepMs);

    const ReliabilityResult r = m_store->beginSession(header);
    m_journalFailureNotified = false;
    if (!r.ok) {
        m_journalFailureNotified = true;
        emit journalWriteFailed(journalPath(), r.error.technicalDetail);
        return false;
    }
    emit sessionChanged();
    emit shotCountsChanged();
    emit totalsChanged();
    return true;
}

void QualificationController::beginPreparation()
{
    submitEvent(DomainEvent(PreparationStarted{kSightingStageId}));
}

void QualificationController::beginSighting()
{
    submitEvent(DomainEvent(SightingStarted{kSightingStageId}));
}

void QualificationController::beginOfficialMatch()
{
    submitEvent(DomainEvent(OfficialMatchStarted{kOfficialStageId}));
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
    core.seriesIndex = 0;
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
