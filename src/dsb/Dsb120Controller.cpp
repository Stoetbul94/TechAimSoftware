#include "Dsb120Controller.h"

#include "mode/OperatingMode.h"

#include <QCoreApplication>
#include <QUuid>
#include <QtGlobal>

using namespace ta::rel;
using ta::dsb::Phase;

namespace {

// Positions are 0..2 and their stage ids are 1..3, so an official shot carries
// the position it was fired in and per-position subtotals are folded from the
// shots themselves. Nothing keeps a second, parallel count that could drift.
inline qint16 stageIdFor(int positionIndex) { return static_cast<qint16>(positionIndex + 1); }

QVector<qint64> parseDurations(const QString& csv)
{
    QVector<qint64> out;
    const QStringList parts = csv.split(QChar(','), Qt::SkipEmptyParts);
    for (const QString& p : parts)
        out.append(p.trimmed().toLongLong());
    return out;
}

} // namespace

Dsb120Controller::Dsb120Controller(QObject* parent)
    : QObject(parent)
    , m_store(std::make_unique<SessionStore>())
{
    connect(m_store.get(), &SessionStore::journalWriteFailed,
            this, [this](QString path, QString detail) {
                emit journalWriteFailed(path, detail);
            });
    connect(m_store.get(), &SessionStore::stateChanged,
            this, [this]() { emit shotCountsChanged(); });
}

Dsb120Controller::~Dsb120Controller() = default;

// ── lifecycle ──────────────────────────────────────────────────────────────

bool Dsb120Controller::startSession(const QVariantMap& authority,
                                    const QString& athlete,
                                    const QString& lane, const QString& targetId)
{
    // The adopted definition is the ONLY source of this session's timing. A
    // programme that does not declare independent position clocks is refused
    // outright: this controller conducts one rule shape, and quietly running
    // something else on it is exactly the failure it exists to prevent.
    if (authority.value(QStringLiteral("timingModel")).toString()
            != QLatin1String("INDEPENDENT_POSITION_CLOCKS"))
        return false;

    const QVector<qint64> positions =
        parseDurations(authority.value(QStringLiteral("positionDurationsMs")).toString());
    if (positions.size() != 3)
        return false;
    const qint64 prepMs = authority.value(QStringLiteral("preparationMs")).toLongLong();
    const int shots = authority.value(QStringLiteral("shotCount")).toInt();
    if (prepMs <= 0 || shots <= 0 || shots % 3 != 0)
        return false;

    if (m_store->active())
        m_store->closeSession(CloseReason::Archive);

    m_preparationMs = prepMs;
    m_positionMs = positions;
    m_shotsPerPosition = shots / 3;
    m_scoringMode = authority.value(QStringLiteral("scoringMode")).toString();
    m_recovered = false;

    SessionHeader header;
    header.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    header.appVersion = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("dev") : QCoreApplication::applicationVersion();
    header.athlete = athlete.trimmed();
    header.lane = lane;
    header.targetId = targetId;
    header.matchType = authority.value(QStringLiteral("programmeVariant")).toString();
    // It IS 10 m air rifle; what differs is how it is conducted, and that is
    // carried by the adopted timing model rather than by a new discipline id.
    header.discipline = Discipline::AirRifle10m;
    if (m_operatingMode >= 0)
        header.operatingMode =
            ta::mode::modeToConfigString(static_cast<ta::mode::Mode>(m_operatingMode));
    header.config.officialShots = shots;
    header.config.seriesSize = 10;
    header.config.matchMs = 0;          // there is NO master clock, and 0 says so
    header.config.perPositionShots = m_shotsPerPosition;
    header.config.sighterLimit = -1;    // unlimited

    RuleAuthority a;
    a.programmeId = authority.value(QStringLiteral("programmeId")).toString();
    a.rulesetId = authority.value(QStringLiteral("rulesetId")).toString();
    a.rulesetVersion = authority.value(QStringLiteral("rulesetVersion")).toString();
    a.ruleNumber = authority.value(QStringLiteral("ruleNumber")).toString();
    a.programmeVariant = authority.value(QStringLiteral("programmeVariant")).toString();
    a.competitionContext = authority.value(QStringLiteral("competitionContext")).toString();
    a.scoringMode = m_scoringMode;
    a.timingModel = QStringLiteral("INDEPENDENT_POSITION_CLOCKS");
    a.targetStandardId = authority.value(QStringLiteral("targetStandardId")).toString();
    a.disciplineId = authority.value(QStringLiteral("disciplineId")).toString();
    a.distanceM = authority.value(QStringLiteral("distanceM")).toInt();
    a.preparationMs = prepMs;
    a.matchMs = 0;
    a.positionSequence = authority.value(QStringLiteral("positionSequence")).toString();
    a.positionDurationsMs =
        authority.value(QStringLiteral("positionDurationsMs")).toString();
    header.ruleAuthority = a;

    qInfo("competition created: %s positions=%s shotsPerPosition=%d",
          qUtf8Printable(a.auditLine()), qUtf8Printable(a.positionDurationsMs),
          m_shotsPerPosition);

    const ReliabilityResult r = m_store->beginSession(header);
    if (!r.ok) {
        emit journalWriteFailed(journalPath(), r.error.technicalDetail);
        return false;
    }
    emit sessionChanged();
    emit stateChanged();
    emit shotCountsChanged();
    return true;
}

void Dsb120Controller::closeSession()
{
    if (m_store && m_store->active())
        m_store->closeSession(CloseReason::Clean);
    emit sessionChanged();
    emit stateChanged();
}

// ── authorised competition-control actions ─────────────────────────────────

bool Dsb120Controller::startPreparation()
{
    if (!active() || phase() != Phase::Idle)
        return false;
    // The generic phase is driven too, so the reliability layer classifies
    // shots the same way this controller does: sighting accepts sighters, and
    // official shots are impossible until a position is in its match phase.
    submitEvent(DomainEvent(PreparationStarted{0}));
    submitEvent(DomainEvent(SightingStarted{0}));
    m_resumeRemainingMs = -1;   // a new clock has its own anchor
    submitEvent(DomainEvent(TimerStarted{TimerId::Preparation, m_preparationMs}));
    if (!step(Dsb120Step::PreparationStarted, -1, m_preparationMs))
        return false;
    // Preparation ENDS at the gate, not at a kneeling clock: arming kneeling is
    // a separate recorded fact, and starting it needs an authorised action.
    return step(Dsb120Step::PositionArmed, 0, 0);
}

bool Dsb120Controller::startPosition(int positionIndex)
{
    if (!active() || phase() != Phase::WaitingStart)
        return false;
    if (positionIndex != nextPositionIndex())
        return false;

    const qint64 duration = durationForPosition(positionIndex);
    if (duration <= 0)
        return false;

    // THE position clock, anchored once. Sighting inside a position runs on
    // this same anchor, which is what makes prone and standing sighters cost
    // competition time; there is no second TimerStarted when the athlete moves
    // to the match phase, so nothing can reset it.
    m_resumeRemainingMs = -1;   // a new clock has its own anchor
    submitEvent(DomainEvent(TimerStarted{TimerId::Match, duration}));
    // Kneeling had its sighting in the shared preparation period and therefore
    // opens IN match. Prone and standing open in sighting.
    if (positionIndex == 0)
        submitEvent(DomainEvent(OfficialMatchStarted{stageIdFor(positionIndex)}));
    else
        submitEvent(DomainEvent(SightingStarted{stageIdFor(positionIndex)}));
    return step(Dsb120Step::PositionStarted, positionIndex, duration);
}

bool Dsb120Controller::enterMatchPhase()
{
    if (!active() || phase() != Phase::PositionSighting)
        return false;
    submitEvent(DomainEvent(OfficialMatchStarted{stageIdFor(positionIndex())}));
    return step(Dsb120Step::MatchPhaseEntered, positionIndex(), 0);
}

bool Dsb120Controller::endPosition()
{
    const Phase p = phase();
    if (!active() || (p != Phase::PositionMatch && p != Phase::PositionSighting))
        return false;
    const int closed = positionIndex();
    if (!step(Dsb120Step::PositionCompleted, closed, 0))
        return false;
    // A finished position never starts the next one. It arms the gate; match
    // control starts the clock.
    if (closed >= 2)
        return step(Dsb120Step::MatchFinished, closed, 0);
    return step(Dsb120Step::PositionArmed, closed + 1, 0);
}

bool Dsb120Controller::finishMatch()
{
    if (!active() || phase() == Phase::Finished)
        return false;
    return step(Dsb120Step::MatchFinished, positionIndex(), 0);
}

// ── shots ──────────────────────────────────────────────────────────────────

bool Dsb120Controller::submitShot(double xMm, double yMm, double score,
                                  qint64 externalId, double directionDeg,
                                  bool simulated)
{
    if (!active())
        return false;
    if (m_operatingMode >= 0
        && !ta::mode::sourceAllowed(static_cast<ta::mode::Mode>(m_operatingMode),
               simulated ? ta::mode::ShotSource::Simulated : ta::mode::ShotSource::Physical))
        return false;

    const Phase p = phase();
    // CLASSIFICATION COMES FROM THE COMPETITION STATE. Not from a caller's
    // flag, not from which button is lit: the state decides what this shot is.
    bool sighter;
    switch (p) {
    case Phase::PreparationSighting:
    case Phase::PositionSighting:
        sighter = true;
        break;
    case Phase::PositionMatch:
        sighter = false;
        break;
    default:
        return false;   // gate, idle or finished: no shot is accepted at all
    }

    const int position = positionIndex();
    if (!sighter && matchShotsInPosition() >= m_shotsPerPosition)
        return false;   // the position's course is already complete

    const qint16 shotNumber = sighter
        ? qint16(0)
        : static_cast<qint16>(m_store->state().officials.size() + 1);

    qint16 scoreTenths = static_cast<qint16>(qRound(score * 10.0));
    scoreTenths = static_cast<qint16>(qBound<int>(0, scoreTenths, 110));

    ShotCore core;
    core.shotNumber = shotNumber;
    core.withinStage = static_cast<qint16>(matchShotsInPosition() + 1);
    core.stageId = sighter ? qint16(0) : stageIdFor(position);
    core.seriesIndex = static_cast<qint8>(position < 0 ? 0 : position);
    core.xHundredthMm = static_cast<qint32>(qRound(xMm * 100.0));
    core.yHundredthMm = static_cast<qint32>(qRound(yMm * 100.0));
    core.scoreTenths = scoreTenths;
    core.directionCentiDeg = static_cast<qint32>(qRound(directionDeg * 100.0));
    core.externalId = externalId;
    core.simulated = simulated;

    const SubmitResult r = sighter
        ? m_store->submit(DomainEvent(SighterAccepted{core}))
        : m_store->submit(DomainEvent(ShotAccepted{core}));
    if (!r.ok)
        return false;

    // The FIRST match shot of a position closes sighting for that position.
    // Recorded as state, not as a hidden button: enterMatchPhase() is refused
    // from the match phase, so there is nothing to hide.
    QVariantMap rec;
    rec[QStringLiteral("isSighter")] = sighter;
    rec[QStringLiteral("shotNumber")] = shotNumber;
    rec[QStringLiteral("positionIndex")] = position;
    rec[QStringLiteral("calculatedscore")] = scoreTenths / 10.0;
    rec[QStringLiteral("xmm")] = xMm;
    rec[QStringLiteral("ymm")] = yMm;
    rec[QStringLiteral("direction")] = QString::number(directionDeg, 'f', 2);
    rec[QStringLiteral("externalId")] = externalId;
    rec[QStringLiteral("simulated")] = simulated;
    emit shotAccepted(rec);
    emit shotCountsChanged();
    emit stateChanged();
    return true;
}

// ── recovery ───────────────────────────────────────────────────────────────

QVariantList Dsb120Controller::scanForRecovery()
{
    if (!m_recovery)
        m_recovery = std::make_unique<RecoveryCoordinator>();
    return m_recovery->scanForQml();
}

bool Dsb120Controller::resumeFromRecovery(const QString& sessionId)
{
    if (!m_recovery)
        m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();
    RecoveredMatchState rec;
    ErrorInfo err;
    if (!m_recovery->buildRecoveredState(sessionId, &rec, &err)) {
        emit journalWriteFailed(rec.journalPath, err.technicalDetail);
        return false;
    }
    // Only a session this controller can conduct.
    if (!std::holds_alternative<Dsb120State>(rec.state.disc))
        return false;

    // The frozen remaining time is computed from the RECOVERED state, before
    // the store reopens the journal: resuming appends events, and the timer
    // anchor is rebased with them.
    m_resumeRemainingMs = rec.state.timer.active
        ? rec.state.timer.durationMs
          - (rec.lastEventMonoMs - rec.state.timer.startedAtMonoMs)
        : -1;
    if (m_resumeRemainingMs < 0 && rec.state.timer.active)
        m_resumeRemainingMs = 0;

    const ReliabilityResult rr = m_store->resumeSession(rec);
    if (!rr.ok) {
        emit journalWriteFailed(journalPath(), rr.error.technicalDetail);
        return false;
    }
    // EVERY duration comes back from the session's own adopted authority. The
    // catalogue is not consulted: a 2027 edition must not re-time a 2026 match.
    const RuleAuthority& a = rec.state.ruleAuthority;
    m_preparationMs = a.preparationMs;
    m_positionMs = parseDurations(a.positionDurationsMs);
    m_shotsPerPosition = rec.state.config.perPositionShots > 0
        ? rec.state.config.perPositionShots
        : rec.state.config.officialShots / 3;
    m_scoringMode = a.scoringMode;
    m_resumeAtMonoMs = m_store->nowMonotonicMs();
    m_recovered = true;

    qInfo("competition recovered: %s position=%d phase=%d matchShots=%d",
          qUtf8Printable(a.auditLine()), positionIndex(), phaseId(),
          totalMatchShots());

    emit sessionChanged();
    emit stateChanged();
    emit shotCountsChanged();
    return true;
}

void Dsb120Controller::discardRecovery(const QString& sessionId)
{
    if (!m_recovery)
        m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();
    m_recovery->archiveOrDiscard(sessionId, /*discarded*/ true);
}

QVariantList Dsb120Controller::recoveredShots() const
{
    QVariantList out;
    const SessionState& s = m_store->state();
    const auto add = [&out](const StateShotRecord& r, bool sighter) {
        QVariantMap m;
        m[QStringLiteral("isSighter")] = sighter;
        m[QStringLiteral("shotNumber")] = r.shot.shotNumber;
        m[QStringLiteral("positionIndex")] = r.shot.seriesIndex;
        m[QStringLiteral("calculatedscore")] = r.effectiveTenths() / 10.0;
        m[QStringLiteral("xmm")] = r.shot.xHundredthMm / 100.0;
        m[QStringLiteral("ymm")] = r.shot.yHundredthMm / 100.0;
        m[QStringLiteral("externalId")] = static_cast<qlonglong>(r.shot.externalId);
        out.append(m);
    };
    for (const StateShotRecord& r : s.sighters) add(r, true);
    for (const StateShotRecord& r : s.officials) add(r, false);
    return out;
}

QVariantMap Dsb120Controller::sessionRuleAuthority() const
{
    const RuleAuthority& a = m_store->state().ruleAuthority;
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
    m[QStringLiteral("distanceM")] = a.distanceM;
    m[QStringLiteral("preparationMs")] = static_cast<qlonglong>(a.preparationMs);
    m[QStringLiteral("matchMs")] = static_cast<qlonglong>(a.matchMs);
    m[QStringLiteral("positionSequence")] = a.positionSequence;
    m[QStringLiteral("positionDurationsMs")] = a.positionDurationsMs;
    return m;
}

// ── reads ──────────────────────────────────────────────────────────────────

Phase Dsb120Controller::phase() const
{
    if (const auto* d = std::get_if<Dsb120State>(&m_store->state().disc))
        return static_cast<Phase>(d->phase);
    return Phase::Idle;
}

bool Dsb120Controller::active() const { return m_store && m_store->active(); }
int Dsb120Controller::phaseId() const { return static_cast<int>(phase()); }

int Dsb120Controller::positionIndex() const
{
    if (const auto* d = std::get_if<Dsb120State>(&m_store->state().disc))
        return d->positionIndex;
    return -1;
}

int Dsb120Controller::nextPositionIndex() const
{
    if (const auto* d = std::get_if<Dsb120State>(&m_store->state().disc))
        return d->nextPositionIndex;
    return -1;
}

// Locked as soon as the position has an accepted match shot. Derived, so it can
// never disagree with the shots themselves.
bool Dsb120Controller::sightingLocked() const
{
    return positionIndex() >= 0 && matchShotsIn(positionIndex()) > 0;
}

int Dsb120Controller::matchShotsIn(int position) const
{
    if (position < 0)
        return 0;
    int n = 0;
    for (const StateShotRecord& r : m_store->state().officials)
        if (r.shot.stageId == stageIdFor(position) && !r.invalidated)
            ++n;
    return n;
}

int Dsb120Controller::matchShotsInPosition() const { return matchShotsIn(positionIndex()); }

int Dsb120Controller::totalMatchShots() const
{
    return m_store->state().officials.size();
}

int Dsb120Controller::positionSubtotalTenths(int position) const
{
    int total = 0;
    for (const StateShotRecord& r : m_store->state().officials)
        if (r.shot.stageId == stageIdFor(position))
            total += r.effectiveTenths();
    return total;
}

qint64 Dsb120Controller::durationForPosition(int index) const
{
    return (index >= 0 && index < m_positionMs.size()) ? m_positionMs.at(index) : 0;
}

qint64 Dsb120Controller::positionDurationMs() const
{
    switch (phase()) {
    case Phase::PreparationSighting: return m_preparationMs;
    case Phase::PositionSighting:
    case Phase::PositionMatch:       return durationForPosition(positionIndex());
    default:                         return 0;   // a gate has no clock
    }
}

qint64 Dsb120Controller::remainingMs() const
{
    const SessionState& s = m_store->state();
    switch (phase()) {
    case Phase::PreparationSighting:
    case Phase::PositionSighting:
    case Phase::PositionMatch:
        break;
    default:
        return -1;   // NOTHING is running during a gate, and -1 says exactly that
    }
    if (!s.timer.active)
        return -1;
    // A RESUMED position continues from the time it had when the application
    // stopped, and runs on from there. Anything else either restarts the
    // position at full duration or leaves it frozen - both of which would be
    // a different competition than the one that was interrupted.
    if (m_recovered && m_resumeRemainingMs >= 0) {
        const qint64 remaining =
            m_resumeRemainingMs - (m_store->nowMonotonicMs() - m_resumeAtMonoMs);
        return remaining > 0 ? remaining : 0;
    }
    const qint64 remaining =
        s.timer.durationMs - (m_store->nowMonotonicMs() - s.timer.startedAtMonoMs);
    return remaining > 0 ? remaining : 0;
}

QString Dsb120Controller::journalPath() const
{
    return m_store ? m_store->currentJournalPath() : QString();
}

// ── internals ──────────────────────────────────────────────────────────────

bool Dsb120Controller::step(Dsb120Step s, int position, qint64 durationMs)
{
    Dsb120StepRecorded e;
    e.step = static_cast<quint8>(s);
    e.positionIndex = static_cast<qint8>(position);
    e.durationMs = durationMs;
    const SubmitResult r = m_store->submit(DomainEvent(e));
    if (r.ok) {
        emit stateChanged();
        return true;
    }
    return false;
}

void Dsb120Controller::submitEvent(const DomainEvent& event)
{
    m_store->submit(event);
}
