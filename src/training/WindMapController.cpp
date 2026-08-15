#include "WindMapController.h"

#include "mode/OperatingMode.h"
#include "WindMapAnalytics.h"
#include "WindMapVerdict.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QUuid>
#include <climits>

using namespace ta::rel;
using namespace ta::training;

namespace {

// The programme id recorded in the journal and used by the recovery
// dispatcher. Must match the value the reducer projects into wmProgramId.
constexpr const char* kProgramWindMap = "wind_map";

// Shot plan bounds. The plan is descriptive — it drives the progress readout
// and nothing else. It never caps, rejects or auto-completes a shot: this is
// training, and an athlete who fires more than planned has still fired.
constexpr int kMinShotPlan = 5;
constexpr int kMaxShotPlan = 200;

ShotCore makeShot(SessionStore* store, double xMm, double yMm, double decimalScore,
                  qint64 externalId, double directionDeg, int shotSource, int within)
{
    ShotCore s;
    s.shotNumber = static_cast<qint16>(within);
    s.xHundredthMm = static_cast<qint32>(qRound(xMm * 100.0));
    s.yHundredthMm = static_cast<qint32>(qRound(yMm * 100.0));
    s.scoreTenths = static_cast<qint16>(qBound<int>(0, qRound(decimalScore * 10.0), 110));
    s.directionCentiDeg = static_cast<qint32>(qRound(directionDeg * 100.0));
    s.splitMs = static_cast<qint32>(store->nowMonotonicMs() > 0
        ? qMin<qint64>(store->nowMonotonicMs(), INT32_MAX) : 0);
    s.externalId = externalId;
    s.simulated = (shotSource == 1);
    return s;
}

// Copies a snapshot into an event's wind fields. One place, so a shot's
// snapshot can never drift from the standing condition it was taken from.
void fillWind(WindSnapshotFields* f, const WindConditionSnapshot& w)
{
    f->windValid = w.valid;
    f->windCalm = w.calm;
    f->windDirectionDegrees = w.directionDegrees;
    f->windSpeedHundredthMs = w.speedHundredthMs;
    f->windSource = static_cast<qint8>(w.source);
    f->windRecordedMs = w.recordedMsSinceEpoch;
    f->windNote = w.note;
}

// The reverse, for rebuilding the standing condition after a resume.
WindConditionSnapshot snapshotFromState(const SessionState& s)
{
    WindConditionSnapshot w;
    w.valid = s.wmWindValid;
    w.calm = s.wmWindCalm;
    w.directionDegrees = s.wmWindDirectionDegrees;
    w.speedHundredthMs = s.wmWindSpeedHundredthMs;
    w.source = (s.wmWindSource == 1) ? WindSource::WeatherStation : WindSource::Manual;
    w.recordedMsSinceEpoch = s.wmWindRecordedMs;
    w.note = s.wmWindNote;
    return w;
}

WindConditionSnapshot snapshotFromRecord(const WindMapShotRecord& r)
{
    WindConditionSnapshot w;
    w.valid = r.windValid;
    w.calm = r.windCalm;
    w.directionDegrees = r.windDirectionDegrees;
    w.speedHundredthMs = r.windSpeedHundredthMs;
    w.source = (r.windSource == 1) ? WindSource::WeatherStation : WindSource::Manual;
    w.recordedMsSinceEpoch = r.windRecordedMs;
    w.note = r.windNote;
    return w;
}

} // namespace

WindMapController::WindMapController(QObject* parent)
    : QObject(parent)
    , m_store(std::make_unique<SessionStore>())
{
    connect(m_store.get(), &SessionStore::journalWriteFailed,
            this, [this](QString path, QString detail) { emit journalWriteFailed(path, detail); });
}

WindMapController::~WindMapController() = default;

// ── availability ───────────────────────────────────────────────────────────

bool WindMapController::isDisciplineSupported(const QString& disciplineId)
{
    return isWindMapDiscipline(disciplineId);
}

QString WindMapController::unsupportedDisciplineMessage(const QString& disciplineId)
{
    if (isWindMapDiscipline(disciplineId))
        return QString();
    return QStringLiteral(
        "Wind Map is available for 50 m Rifle Prone and 50 m Rifle 3 Positions only.\n\n"
        "The selected discipline (%1) records no wind conditions.")
        .arg(disciplineId.isEmpty() ? QStringLiteral("none selected") : disciplineId);
}

// ── setup ──────────────────────────────────────────────────────────────────

bool WindMapController::configureSession(const QString& disciplineId, int shotPlan,
                                         bool enableSighters)
{
    if (m_phase != WindMapPhase::Idle)
        return fail(QStringLiteral("A Wind Map session is already running."), "already-active");
    // Fails CLOSED. An unsupported discipline is never coerced into a
    // supported one, and no partial configuration is retained.
    if (!isWindMapDiscipline(disciplineId)) {
        m_disciplineId.clear();
        return fail(unsupportedDisciplineMessage(disciplineId), "unsupported-discipline");
    }
    m_disciplineId = disciplineId;
    m_threePositions = windMapDisciplineIs3P(disciplineId);
    m_shotPlan = shotPlan;
    m_sightersEnabled = enableSighters;
    m_lastError.clear();
    emit configChanged();
    return true;
}

bool WindMapController::setFiringDirection(int degrees)
{
    // Optional metadata. Out-of-range input is REFUSED rather than clamped —
    // a wrong firing direction would silently mislabel every relative
    // description in the analysis.
    if (degrees < 0) { clearFiringDirection(); return true; }
    if (degrees > 359)
        return fail(QStringLiteral("Enter the firing direction as 0 to 359 degrees, "
                                   "or leave it unrecorded."), "bad-firing-direction");
    m_firingDirectionDeg = degrees;
    m_analysisCache.clear();
    m_analysisKey.clear();
    emit configChanged();
    return true;
}

void WindMapController::clearFiringDirection()
{
    m_firingDirectionDeg = -1;
    m_analysisCache.clear();
    m_analysisKey.clear();
    emit configChanged();
}

QString WindMapController::validateConfig() const
{
    if (!isWindMapDiscipline(m_disciplineId))
        return QStringLiteral("Select 50 m Rifle Prone or 50 m Rifle 3 Positions.");
    if (m_shotPlan < kMinShotPlan || m_shotPlan > kMaxShotPlan)
        return QStringLiteral("Planned shots must be between %1 and %2.")
            .arg(kMinShotPlan).arg(kMaxShotPlan);
    return QString();
}

QString WindMapController::positionSequence() const
{
    return m_threePositions ? QStringLiteral("K-P-S") : QString();
}

bool WindMapController::startWindMap(const QString& athlete)
{
    auto refuse = [this](const QString& userMsg, const char* code) {
        m_lastStartError = userMsg;
        m_lastError = userMsg;
        qWarning().noquote() << "WINDMAP start refused —" << code;
        emit startErrorChanged();
        return false;
    };
    if (athlete.trimmed().isEmpty())
        return refuse(QStringLiteral("No athlete has been selected.\n\n"
                                     "Enter or choose an athlete name, then start again."),
                      "no-athlete");
    const QString err = validateConfig();
    if (!err.isEmpty())
        return refuse(QStringLiteral("The Wind Map setup is not valid.\n\n%1").arg(err),
                      "invalid-config");
    if (m_store->active())
        m_store->closeSession(CloseReason::Clean);

    SessionHeader header;
    header.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    header.appVersion = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("dev") : QCoreApplication::applicationVersion();
    header.athlete = athlete.trimmed();
    header.matchType = QStringLiteral("TRAINING wind_map");
    header.discipline = m_threePositions ? Discipline::ThreePositions50m
                                         : Discipline::Prone50m;
    header.sessionKind = QStringLiteral("Training");
    if (m_operatingMode >= 0)
        header.operatingMode = ta::mode::modeToConfigString(
            static_cast<ta::mode::Mode>(m_operatingMode));
    // Wind Map counts nothing officially. officialShots stays 0 so no
    // qualification projection can ever read a target from this session.
    header.config.officialShots = 0;

    const ReliabilityResult r = m_store->beginSession(header);
    if (!r.ok)
        return refuse(QStringLiteral("The training session journal could not be created.\n\n%1")
                          .arg(r.error.operatorMessage), "journal-open-failed");
    m_lastStartError.clear();
    emit startErrorChanged();

    WindMapSessionStarted ev;
    ev.disciplineId = m_disciplineId;
    ev.is3P = m_threePositions;
    ev.positionSequence = positionSequence();
    if (!submit(DomainEvent(ev)))
        return false;

    // The reducer puts the session in Setup; the controller mirrors it rather
    // than assuming it, so the two can never disagree from the first event on.
    m_phase = static_cast<WindMapPhase>(st().wmPhase);
    m_lastExternalId = -1;
    m_wind = WindConditionSnapshot::noReading();
    emit phaseChanged();
    emit configChanged();
    emit progressChanged();
    emit conditionChanged();
    return true;
}

// ── wind condition ─────────────────────────────────────────────────────────

bool WindMapController::applyCondition(const WindConditionSnapshot& snap)
{
    if (m_phase == WindMapPhase::Idle || m_phase == WindMapPhase::Completed)
        return fail(QStringLiteral("No Wind Map session is recording."), "not-recording");
    if (!snap.isStructurallyValid())
        return fail(QStringLiteral("The wind reading is not valid."), "invalid-snapshot");

    WindConditionChanged ev;
    fillWind(&ev, snap);
    if (!submit(DomainEvent(ev)))
        return false;
    m_wind = snap;
    emit conditionChanged();
    return true;
}

bool WindMapController::setCalmCondition(const QString& note)
{
    // Calm is a RECORDED OBSERVATION, distinct from "no reading". It carries
    // no direction — a calm wind has none, and inventing 0 degrees would read
    // as North.
    return applyCondition(WindConditionSnapshot::calmAt(
        QDateTime::currentMSecsSinceEpoch(), WindSource::Manual, note.trimmed()));
}

bool WindMapController::setMeasuredCondition(int directionDegrees, double speedMetresPerSecond,
                                             const QString& note)
{
    // QML passes m/s. The conversion to the stored hundredths happens HERE,
    // through the one domain helper, and the raw value is never returned to
    // QML. An out-of-range or non-finite value is refused, not clamped.
    WindConditionSnapshot snap;
    if (!WindConditionSnapshot::measured(static_cast<double>(directionDegrees),
                                         speedMetresPerSecond,
                                         QDateTime::currentMSecsSinceEpoch(),
                                         &snap, WindSource::Manual, note.trimmed())) {
        return fail(QStringLiteral("Enter a direction in degrees and a wind speed in metres "
                                   "per second (0 or more)."),
                    "invalid-measurement");
    }
    return applyCondition(snap);
}

bool WindMapController::setNoReadingCondition()
{
    // Explicitly recording ABSENCE. This is not calm and not zero wind: a
    // shot taken under it is excluded from condition comparisons rather than
    // being compared against an invented reading.
    return applyCondition(WindConditionSnapshot::noReading());
}

// ── phase transitions ──────────────────────────────────────────────────────

bool WindMapController::goToPhase(WindMapPhase to)
{
    if (!windMapTransitionAllowed(m_phase, to, m_threePositions)) {
        return fail(QStringLiteral("%1 cannot follow %2 in a Wind Map session.")
                        .arg(windMapPhaseName(to), windMapPhaseName(m_phase)),
                    "illegal-transition");
    }
    WindMapPhaseChanged ev;
    ev.fromPhase = static_cast<qint8>(m_phase);
    ev.toPhase = static_cast<qint8>(to);
    if (!submit(DomainEvent(ev)))
        return false;
    m_phase = to;
    emit phaseChanged();
    emit progressChanged();
    return true;
}

bool WindMapController::beginSighters()
{
    if (!m_sightersEnabled)
        return fail(QStringLiteral("Sighters are switched off for this session."), "no-sighters");
    return goToPhase(WindMapPhase::Sighters);
}

bool WindMapController::finishSighters()   { return goToPhase(WindMapPhase::CountedShots); }
bool WindMapController::beginCountedShots(){ return goToPhase(WindMapPhase::CountedShots); }
bool WindMapController::endPosition()      { return goToPhase(WindMapPhase::PositionReview); }
bool WindMapController::endCapture()       { return goToPhase(WindMapPhase::SessionReview); }

bool WindMapController::changePosition(int toPosition)
{
    // 50 m Prone has ONE position. A position change there is a workflow
    // error, not something to absorb quietly.
    if (!m_threePositions)
        return fail(QStringLiteral("50 m Prone has a single position — there is nothing to "
                                   "change to."), "not-3p");
    if (m_phase != WindMapPhase::PositionReview)
        return fail(QStringLiteral("Finish the current position before changing to another."),
                    "wrong-phase");
    if (!isValidWindMapPosition(static_cast<qint8>(toPosition), true))
        return fail(QStringLiteral("Choose Kneeling, Prone or Standing."), "bad-position");
    if (toPosition == currentPosition())
        return fail(QStringLiteral("That position is already selected."), "same-position");

    WindMapPositionChanged ev;
    ev.fromPosition = static_cast<qint8>(currentPosition());
    ev.toPosition = static_cast<qint8>(toPosition);
    if (!submit(DomainEvent(ev)))
        return false;
    emit progressChanged();
    return true;
}

// ── shots ──────────────────────────────────────────────────────────────────

bool WindMapController::registerShot(double xMm, double yMm, double decimalScore,
                                     qint64 externalId, double directionDeg, int shotSource)
{
    if (m_operatingMode >= 0
        && !ta::mode::sourceAllowed(static_cast<ta::mode::Mode>(m_operatingMode),
                                    static_cast<ta::mode::ShotSource>(shotSource))) {
        emit shotRejected(QStringLiteral("WrongInputSource"));
        return false;
    }
    if (!m_store->active()) { emit shotRejected(QStringLiteral("NotActive")); return false; }
    if (!(decimalScore >= 0.0) || decimalScore > 11.0
        || !(xMm > -500.0 && xMm < 500.0) || !(yMm > -500.0 && yMm < 500.0)) {
        emit shotRejected(QStringLiteral("InvalidShotData"));
        return false;
    }
    if (externalId >= 0 && externalId <= m_lastExternalId) {
        emit shotRejected(QStringLiteral("DuplicateShot"));
        return false;
    }
    // The phase decides the classification, and it is durable — so a shot
    // fired outside a shooting phase is IGNORED rather than guessed at.
    if (m_phase == WindMapPhase::Sighters)
        return acceptShotInternal(true, xMm, yMm, decimalScore, externalId, directionDeg, shotSource);
    if (m_phase == WindMapPhase::CountedShots)
        return acceptShotInternal(false, xMm, yMm, decimalScore, externalId, directionDeg, shotSource);
    emit shotRejected(m_phase == WindMapPhase::Setup ? QStringLiteral("SetupShotIgnored")
                                                     : QStringLiteral("NotShootingPhase"));
    return false;
}

bool WindMapController::acceptShotInternal(bool sighter, double xMm, double yMm,
                                           double decimalScore, qint64 externalId,
                                           double directionDeg, int shotSource)
{
    const qint32 shotId = st().wmNextShotId;
    const qint8 pos = static_cast<qint8>(currentPosition());
    const ShotCore shot = makeShot(m_store.get(), xMm, yMm, decimalScore,
                                   externalId, directionDeg, shotSource,
                                   sighter ? 0 : countedShots() + 1);
    // The IMMUTABLE association: the shot takes a COPY of the standing
    // condition at accept time. A later condition change does not reach back
    // into it, and an absent reading is written as absent — never back-filled
    // from a neighbouring shot.
    bool ok = false;
    if (sighter) {
        WindMapSighterAccepted ev;
        ev.shot = shot; ev.shotId = shotId; ev.position = pos;
        fillWind(&ev, m_wind);
        ok = submit(DomainEvent(ev));
    } else {
        WindMapShotAccepted ev;
        ev.shot = shot; ev.shotId = shotId; ev.position = pos;
        fillWind(&ev, m_wind);
        ok = submit(DomainEvent(ev));
    }
    if (!ok) {
        emit shotRejected(QStringLiteral("NotRecorded"));
        return false;
    }
    if (externalId >= 0) m_lastExternalId = externalId;
    emit shotAccepted(sighter, static_cast<int>(shotId), static_cast<int>(pos));
    emit progressChanged();
    return true;
}

// ── completion / close ─────────────────────────────────────────────────────

bool WindMapController::completeSession()
{
    if (m_phase != WindMapPhase::SessionReview)
        return fail(QStringLiteral("Finish the capture before completing the session."),
                    "not-in-review");
    // Only ONE event is needed here: the reducer moves the phase to Completed
    // as part of applying it, so there is no separate phase transition to keep
    // in step with it.
    WindMapSessionCompleted ev;
    ev.countedShots = static_cast<qint32>(totalCountedShots());
    ev.sighterShots = static_cast<qint32>(totalSighterShots());
    ev.conditionChanges = static_cast<qint32>(conditionChanges());
    if (!submit(DomainEvent(ev)))
        return false;
    m_phase = WindMapPhase::Completed;
    emit phaseChanged();
    emit progressChanged();
    return true;
}

bool WindMapController::closeSessionCleanly()
{
    if (m_store && m_store->active()) {
        const ReliabilityResult rr = m_store->closeSession(CloseReason::Clean);
        if (!rr.ok) {
            m_lastError = rr.error.operatorMessage.isEmpty()
                ? QStringLiteral("The session could not be closed safely.")
                : rr.error.operatorMessage;
            emit journalWriteFailed(rr.error.affectedPath, rr.error.technicalDetail);
            return false;
        }
    }
    m_phase = WindMapPhase::Idle;
    m_lastExternalId = -1;
    m_wind = WindConditionSnapshot::noReading();
    m_analysisCache.clear();
    m_analysisKey.clear();
    emit phaseChanged();
    emit progressChanged();
    emit conditionChanged();
    return true;
}

// ── recovery ───────────────────────────────────────────────────────────────

void WindMapController::rebuildFromState()
{
    const SessionState& s = st();
    m_disciplineId = s.wmDisciplineId;
    m_threePositions = s.wmThreePositions;
    // The phase is READ, not re-derived. That is the whole reason it is
    // journalled: after a crash during counted shots with no counted shot yet
    // fired, no derivation from the recorded shots could tell it apart from
    // the sighter phase.
    m_phase = static_cast<WindMapPhase>(s.wmPhase);
    m_wind = snapshotFromState(s);
    // Sighters were enabled if any sighter was recorded; otherwise assume they
    // were, which only affects whether the sighter button is offered — it can
    // never reclassify an already-recorded shot.
    m_sightersEnabled = true;
    m_lastExternalId = -1;
    for (const WindMapShotRecord& r : s.wmShots)
        if (r.shot.externalId > m_lastExternalId) m_lastExternalId = r.shot.externalId;
}

bool WindMapController::resumeFromRecovery(const QString& sessionId)
{
    if (!m_recovery) m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();
    RecoveredMatchState rec;
    ErrorInfo err;
    if (!m_recovery->buildRecoveredState(sessionId, &rec, &err)) {
        emit journalWriteFailed(rec.journalPath, err.technicalDetail);
        return false;
    }
    return resumeFromRecoveredState(rec);
}

bool WindMapController::resumeFromRecoveredState(const RecoveredMatchState& rec)
{
    // Fails CLOSED on classification: a competition journal, or another
    // Training programme's journal, is refused rather than partially applied.
    if (rec.state.sessionKind != QLatin1String("Training")
        || rec.state.wmProgramId != QLatin1String(kProgramWindMap)) {
        m_lastError = QStringLiteral("Not a Wind Map session.");
        return false;
    }
    if (!isWindMapDiscipline(rec.state.wmDisciplineId)) {
        m_lastError = QStringLiteral("This Wind Map session records an unsupported discipline.");
        return false;
    }
    const ReliabilityResult rr = m_store->resumeSession(rec);
    if (!rr.ok) {
        emit journalWriteFailed(rec.journalPath, rr.error.technicalDetail);
        return false;
    }
    // NOTHING is submitted here. A resume must not re-journal the start, the
    // conditions or the shots — the journal already holds them.
    rebuildFromState();
    emit configChanged();
    emit phaseChanged();
    emit progressChanged();
    emit conditionChanged();
    qInfo() << "WINDMAP: resumed" << st().sessionId.left(8)
            << "phase" << static_cast<int>(m_phase)
            << "position" << currentPosition()
            << "shots" << st().wmShots.size();
    return true;
}

void WindMapController::discardRecovery(const QString& sessionId)
{
    if (!m_recovery) m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();
    m_recovery->archiveOrDiscard(sessionId, true);
}

qint64 WindMapController::recoveredMaxExternalId() const
{
    return m_lastExternalId;
}

// ── projections ────────────────────────────────────────────────────────────

int WindMapController::currentPosition() const
{
    if (!m_store || !m_store->active()) return 0;
    return static_cast<int>(st().wmCurrentPosition);
}

QString WindMapController::positionName() const
{
    return windMapPositionName(static_cast<WindMapPosition>(currentPosition()));
}

int WindMapController::countedShots() const
{
    if (!m_store || !m_store->active()) return 0;
    const qint8 pos = st().wmCurrentPosition;
    int n = 0;
    for (const WindMapShotRecord& r : st().wmShots)
        if (!r.sighter && r.position == pos) ++n;
    return n;
}

int WindMapController::sighterCount() const
{
    if (!m_store || !m_store->active()) return 0;
    const qint8 pos = st().wmCurrentPosition;
    int n = 0;
    for (const WindMapShotRecord& r : st().wmShots)
        if (r.sighter && r.position == pos) ++n;
    return n;
}

int WindMapController::totalCountedShots() const
{
    if (!m_store || !m_store->active()) return 0;
    int n = 0;
    for (const WindMapShotRecord& r : st().wmShots) if (!r.sighter) ++n;
    return n;
}

int WindMapController::totalSighterShots() const
{
    if (!m_store || !m_store->active()) return 0;
    int n = 0;
    for (const WindMapShotRecord& r : st().wmShots) if (r.sighter) ++n;
    return n;
}

int WindMapController::conditionChanges() const
{
    if (!m_store || !m_store->active()) return 0;
    return static_cast<int>(st().wmConditionChanges);
}

QString WindMapController::directionLabel() const
{
    if (!m_wind.isMeasured()) return QString();
    return windSectorLabel(m_wind.sector());
}

QString WindMapController::conditionSummary() const
{
    // Descriptive only. It states what was recorded — it does not interpret,
    // compare, or suggest anything.
    if (!m_wind.hasReading())
        return QStringLiteral("No wind reading recorded");
    if (m_wind.isCalm())
        return QStringLiteral("Calm");
    return QStringLiteral("%1° %2 · %3 m/s · %4")
        .arg(m_wind.directionDegrees)
        .arg(windSectorLabel(m_wind.sector()),
             QString::number(m_wind.speedMetresPerSecond(), 'f', 1),
             windSpeedBandLabel(m_wind.band()));
}

QString WindMapController::sessionId() const
{
    return m_store ? st().sessionId : QString();
}

QVariantMap WindMapController::reviewSummary() const
{
    // STAGE 5 SCOPE: a FACTUAL capture summary — counts, positions and what
    // was recorded. No condition comparison, no coaching narrative, no
    // performance claim. Those belong to the analytics stage.
    QVariantMap m;
    if (!m_store || !m_store->active()) return m;
    const SessionState& s = st();
    m[QStringLiteral("programme")] = QStringLiteral("Wind Map — Post-Session Review");
    m[QStringLiteral("sessionId")] = s.sessionId;
    m[QStringLiteral("athlete")] = s.athlete;
    m[QStringLiteral("createdAtIso")] = s.createdAtIso;
    m[QStringLiteral("operatingMode")] = s.operatingMode.isEmpty()
        ? QStringLiteral("Legacy") : s.operatingMode;
    m[QStringLiteral("disciplineId")] = s.wmDisciplineId;
    m[QStringLiteral("disciplineName")] = s.wmThreePositions
        ? QStringLiteral("50 m Rifle 3 Positions") : QStringLiteral("50 m Rifle Prone");
    m[QStringLiteral("threePositions")] = s.wmThreePositions;
    m[QStringLiteral("plannedShots")] = m_shotPlan;
    m[QStringLiteral("countedShots")] = totalCountedShots();
    m[QStringLiteral("sighterShots")] = totalSighterShots();
    m[QStringLiteral("conditionChanges")] = conditionChanges();
    m[QStringLiteral("completed")] = s.wmCompleted;
    m[QStringLiteral("phase")] = static_cast<int>(m_phase);
    m[QStringLiteral("phaseName")] = windMapPhaseName(m_phase);

    // ── Stage 5.1: SIX DEFINED COUNTS ───────────────────────────────────
    //
    // The old summary had one ambiguous "CONDITIONS" tile fed straight from
    // wmConditionChanges. That counts JOURNAL EVENTS — one per press of
    // RECORD / CALM / NO READING — so a session whose table showed four
    // distinct conditions reported 13. The label read as "distinct conditions
    // observed" and the number was "times a condition was entered".
    //
    // Each count below has ONE definition and its own test. Nothing here mixes
    // QML selector changes, unrecorded pending input, journal events, shots
    // and unique conditions.
    //
    //   uniqueConditions   distinct condition VALUES under which COUNTED shots
    //                      were recorded (identity = reading?/calm?/dir/speed,
    //                      never the timestamp or note)
    //   conditionEntries   condition-change EVENTS recorded in the journal
    //   countedWithReading COUNTED shots whose snapshot is a measured reading
    //   countedCalm        COUNTED shots whose snapshot is a recorded calm
    //   countedNoReading   COUNTED shots recorded with NO reading
    //   sighterShots       sighters (never in any counted total)
    //
    // INVARIANT, asserted by test:
    //   countedWithReading + countedCalm + countedNoReading == countedShots
    int withReading = 0, calm = 0, noReading = 0;
    QVector<WindConditionSnapshot> distinct;
    for (const WindMapShotRecord& r : s.wmShots) {
        if (r.sighter) continue;
        if (!r.windValid) ++noReading;
        else if (r.windCalm) ++calm;
        else ++withReading;
        const WindConditionSnapshot w = snapshotFromRecord(r);
        bool seen = false;
        for (const WindConditionSnapshot& d : distinct)
            if (d.sameConditionAs(w)) { seen = true; break; }
        if (!seen) distinct.append(w);
    }
    m[QStringLiteral("uniqueConditions")] = distinct.size();
    m[QStringLiteral("conditionEntries")] = conditionChanges();
    m[QStringLiteral("countedWithReading")] = withReading;
    m[QStringLiteral("countedCalm")] = calm;
    m[QStringLiteral("countedNoReading")] = noReading;

    if (s.wmThreePositions) {
        QVariantList positions;
        for (int p = 1; p <= 3; ++p) {
            int counted = 0, sighters = 0;
            for (const WindMapShotRecord& r : s.wmShots) {
                if (r.position != p) continue;
                if (r.sighter) ++sighters; else ++counted;
            }
            if (counted == 0 && sighters == 0) continue;
            QVariantMap pm;
            pm[QStringLiteral("position")] = p;
            pm[QStringLiteral("positionName")] =
                windMapPositionName(static_cast<WindMapPosition>(p));
            pm[QStringLiteral("countedShots")] = counted;
            pm[QStringLiteral("sighterShots")] = sighters;
            positions.append(pm);
        }
        m[QStringLiteral("positions")] = positions;
    }
    m[QStringLiteral("disclaimer")] = QStringLiteral(
        "A record of the conditions you observed alongside the shots you fired. "
        "Training material only — never an official result.");
    return m;
}

// ── Stage 6.1: the analysis view model ─────────────────────────────────────
//
// A pure PROJECTION of WindMapAnalyticsEngine output. Every number here is
// COPIED from the engine; nothing is recomputed, re-derived or rounded into a
// different value. That is what lets the on-screen analysis and the PDF share
// one model and be asserted equal to the engine.
//
// A metric whose sample does not support it is NOT emitted as a number. The
// has* flag travels with it so a view cannot print a zero that looks like a
// measurement.

namespace {

QVariantMap groupToVariant(const ta::training::GroupStats& g)
{
    QVariantMap m;
    m[QStringLiteral("label")] = g.label;
    m[QStringLiteral("n")] = g.n;
    m[QStringLiteral("evidence")] = ta::training::evidenceLabel(g.evidence);
    m[QStringLiteral("hasReading")] = g.key.hasReading;
    m[QStringLiteral("calm")] = g.key.calm;

    m[QStringLiteral("hasMeanScore")] = g.hasMeanScore;
    if (g.hasMeanScore) m[QStringLiteral("meanScore")] = g.meanScore;

    m[QStringLiteral("hasMpi")] = g.hasMpi;
    if (g.hasMpi) {
        m[QStringLiteral("mpiXMm")] = g.mpiXMm;
        m[QStringLiteral("mpiYMm")] = g.mpiYMm;
    } else {
        m[QStringLiteral("shotsNeededForMpi")] = g.shotsNeededForMpi();
    }

    m[QStringLiteral("hasDispersion")] = g.hasDispersion;
    // Filled by the caller, which knows the session's firing direction.
    m[QStringLiteral("hasRelativeWind")] = false;
    m[QStringLiteral("relativeWind")] = QString();
    if (g.hasDispersion) {
        // Classification metric first — this is what the verdicts are built on.
        m[QStringLiteral("radialRmsMm")] = g.radialRmsMm;
        m[QStringLiteral("horizontalSdMm")] = g.horizontalSdMm;
        m[QStringLiteral("verticalSdMm")] = g.verticalSdMm;
        m[QStringLiteral("meanRadiusMm")] = g.meanRadiusMm;
        // Descriptive only. QML formats these; it must never compare them.
        m[QStringLiteral("groupDiameterMm")] = g.groupDiameterMm;
        m[QStringLiteral("horizontalSpreadMm")] = g.horizontalSpreadMm;
        m[QStringLiteral("verticalSpreadMm")] = g.verticalSpreadMm;
        m[QStringLiteral("scoreStdDev")] = g.scoreStdDev;
        m[QStringLiteral("radiusStdDev")] = g.radiusStdDev;
    } else {
        m[QStringLiteral("shotsNeededForDispersion")] = g.shotsNeededForDispersion();
    }
    return m;
}

QVariantMap shiftToVariant(const ta::training::ShiftVector& v)
{
    QVariantMap m;
    m[QStringLiteral("label")] = v.label;
    m[QStringLiteral("n")] = v.n;
    m[QStringLiteral("referenceN")] = v.referenceN;
    m[QStringLiteral("valid")] = v.valid;
    m[QStringLiteral("evidence")] = ta::training::evidenceLabel(v.evidence);
    if (v.valid) {
        m[QStringLiteral("dxMm")] = v.dxMm;
        m[QStringLiteral("dyMm")] = v.dyMm;
        m[QStringLiteral("magnitudeMm")] = v.magnitudeMm;
        m[QStringLiteral("bearingDegrees")] = v.bearingDegrees;
        m[QStringLiteral("directionWords")] = v.directionWords;
    } else {
        m[QStringLiteral("shotsNeeded")] = v.shotsNeeded;
    }
    return m;
}

} // namespace

// The cache key. It must change whenever the analysis would change: a
// different session, a different number of recorded shots, or a different
// phase (completion adds the terminal state). Anything else in the session is
// immutable once recorded, which is what makes memoising safe at all.
QString WindMapController::analysisCacheKey() const
{
    if (!m_store || !m_store->active()) return QString();
    const ta::rel::SessionState& s = st();
    return QStringLiteral("%1|%2|%3|%4|%5")
        .arg(s.sessionId)
        .arg(s.wmShots.size())
        .arg(static_cast<int>(m_phase))
        .arg(s.wmConditionChanges)
        .arg(m_firingDirectionDeg);
}

QVariantMap WindMapController::analysisModel() const
{
    QVariantMap out;
    if (!m_store || !m_store->active()) return out;

    // UI-WIND-003: return the cached projection when nothing that feeds it has
    // changed. Switching page or position in the view must not re-run the
    // engine — that was a measured cost on every tab click.
    const QString key = analysisCacheKey();
    if (!key.isEmpty() && key == m_analysisKey && !m_analysisCache.isEmpty())
        return m_analysisCache;

    const ta::training::SessionAnalysis a =
        ta::training::WindMapAnalyticsEngine::analyse(st());
    if (!a.valid) return out;
    ++m_analysisBuilds;

    const ta::rel::SessionState& s = st();

    // 1. SESSION OVERVIEW — the tested Stage 5.1 definitions, unchanged.
    QVariantMap session;
    session[QStringLiteral("programme")] = QStringLiteral("Wind Map — Post-Session Review");
    session[QStringLiteral("sessionId")] = s.sessionId;
    session[QStringLiteral("athlete")] = s.athlete;
    session[QStringLiteral("createdAtIso")] = s.createdAtIso;
    session[QStringLiteral("operatingMode")] = s.operatingMode.isEmpty()
        ? QStringLiteral("Legacy") : s.operatingMode;
    // WHICH analysis method produced this. A report rendered from an older
    // build classified dispersion by extreme spread; this string is how the two
    // are told apart, so it is never omitted.
    session[QStringLiteral("analyticsVersion")] = a.analyticsVersion;
    session[QStringLiteral("ringSpacingMm")] = a.ringSpacingMm;
    session[QStringLiteral("disciplineId")] = a.disciplineId;
    session[QStringLiteral("disciplineName")] = a.threePositions
        ? QStringLiteral("50 m Rifle 3 Positions") : QStringLiteral("50 m Rifle Prone");
    session[QStringLiteral("threePositions")] = a.threePositions;
    session[QStringLiteral("completed")] = s.wmCompleted;
    QStringList posNames;
    for (const ta::training::PositionAnalysis& p : a.positions) posNames << p.positionName;
    session[QStringLiteral("positionsRepresented")] = posNames;
    // Stage 6.1.3: optional, and absent means absent.
    session[QStringLiteral("hasFiringDirection")] = (m_firingDirectionDeg >= 0);
    session[QStringLiteral("firingDirection")] = m_firingDirectionDeg;
    session[QStringLiteral("relativeWindNote")] = (m_firingDirectionDeg >= 0)
        ? QString()
        : ta::training::relativeWindLabel(ta::training::RelativeWind::Unavailable);
    out[QStringLiteral("session")] = session;

    QVariantMap summary;
    summary[QStringLiteral("countedShots")] = a.countedShots;
    summary[QStringLiteral("sighterShots")] = a.sighterShots;
    summary[QStringLiteral("uniqueConditions")] = a.uniqueConditions;
    summary[QStringLiteral("conditionEntries")] = a.conditionEntries;
    summary[QStringLiteral("countedWithReading")] = a.countedWithReading;
    summary[QStringLiteral("countedCalm")] = a.countedCalm;
    summary[QStringLiteral("countedNoReading")] = a.countedNoReading;
    // Data quality is a STATEMENT OF COVERAGE, not a score.
    const int usable = a.countedWithReading + a.countedCalm;
    QString quality;
    if (a.countedShots == 0)                 quality = QStringLiteral("No counted shots");
    else if (a.countedNoReading == 0)        quality = QStringLiteral("Every counted shot carries a reading");
    else if (usable >= a.countedShots / 2)   quality = QStringLiteral("%1 of %2 counted shots carry a reading")
                                                          .arg(usable).arg(a.countedShots);
    else                                     quality = QStringLiteral("Only %1 of %2 counted shots carry a reading")
                                                          .arg(usable).arg(a.countedShots);
    summary[QStringLiteral("dataQuality")] = quality;
    out[QStringLiteral("summary")] = summary;

    // 2-8. PER POSITION — each with its OWN reference centre and groupings.
    QVariantList positions;
    for (const ta::training::PositionAnalysis& p : a.positions) {
        QVariantMap pm;
        pm[QStringLiteral("position")] = p.position;
        pm[QStringLiteral("positionName")] = p.positionName;
        pm[QStringLiteral("countedShots")] = p.countedShots;
        pm[QStringLiteral("sighterShots")] = p.sighterShots;
        pm[QStringLiteral("hasOverallMpi")] = p.hasOverallMpi;
        if (p.hasOverallMpi) {
            pm[QStringLiteral("overallMpiXMm")] = p.overallMpiXMm;
            pm[QStringLiteral("overallMpiYMm")] = p.overallMpiYMm;
        }
        QVariantMap ref;
        ref[QStringLiteral("kind")] = ta::training::referenceKindLabel(p.reference.kind);
        ref[QStringLiteral("label")] = p.reference.label;
        ref[QStringLiteral("valid")] = p.reference.valid;
        ref[QStringLiteral("n")] = p.reference.n;
        if (p.reference.valid) {
            ref[QStringLiteral("xMm")] = p.reference.xMm;
            ref[QStringLiteral("yMm")] = p.reference.yMm;
        }
        pm[QStringLiteral("reference")] = ref;

        QVariantList dirs, bands, exact, shifts;
        for (const ta::training::GroupStats& g : p.byDirection)      dirs.append(groupToVariant(g));
        for (const ta::training::GroupStats& g : p.bySpeedBand)      bands.append(groupToVariant(g));
        for (const ta::training::GroupStats& g : p.byExactCondition) {
            QVariantMap gm = groupToVariant(g);
            if (g.key.hasReading && !g.key.calm) {
                const ta::training::RelativeWind rel =
                    ta::training::relativeWindFor(g.key.directionDegrees, m_firingDirectionDeg);
                gm[QStringLiteral("hasRelativeWind")] =
                    (rel != ta::training::RelativeWind::Unavailable);
                gm[QStringLiteral("relativeWind")] = ta::training::relativeWindLabel(rel);
            }
            exact.append(gm);
        }
        for (const ta::training::ShiftVector& v : p.shifts)          shifts.append(shiftToVariant(v));
        pm[QStringLiteral("byDirection")] = dirs;
        pm[QStringLiteral("bySpeedBand")] = bands;
        pm[QStringLiteral("byExactCondition")] = exact;
        pm[QStringLiteral("shifts")] = shifts;
        positions.append(pm);
    }
    out[QStringLiteral("positions")] = positions;

    // Stage 6.1.2: the VERDICTS. QML and the PDF both read these; neither
    // composes verdict text of its own.
    {
        const QVector<ta::training::Verdict> verdicts =
            ta::training::WindMapVerdictEngine::evaluate(a);
        QVariantList vl;
        for (const ta::training::Verdict& v : verdicts) {
            QVariantMap vm;
            vm[QStringLiteral("verdictId")] = v.verdictId;
            vm[QStringLiteral("category")] = ta::training::verdictCategoryLabel(v.category);
            vm[QStringLiteral("scope")] = ta::training::verdictScopeLabel(v.scope);
            vm[QStringLiteral("scopeIsSession")] = (v.scope == ta::training::VerdictScope::Session);
            vm[QStringLiteral("position")] = v.position;
            vm[QStringLiteral("positionName")] = v.positionName;
            vm[QStringLiteral("referenceCondition")] = v.referenceCondition;
            vm[QStringLiteral("comparedCondition")] = v.comparedCondition;
            vm[QStringLiteral("sampleCountReference")] = v.sampleCountReference;
            vm[QStringLiteral("sampleCountCompared")] = v.sampleCountCompared;
            vm[QStringLiteral("evidence")] = ta::training::evidenceLevelLabel(v.evidence);
            vm[QStringLiteral("evidenceExplanation")] =
                ta::training::evidenceLevelExplanation(v.evidence);
            vm[QStringLiteral("headline")] = v.headline;
            vm[QStringLiteral("observedPattern")] = v.observedPattern;
            vm[QStringLiteral("interpretation")] = v.interpretation;
            vm[QStringLiteral("nextTrainingStep")] = v.nextTrainingStep;
            vm[QStringLiteral("coachDecision")] = v.coachDecision;
            vm[QStringLiteral("limitations")] = v.limitations;
            vm[QStringLiteral("supportingMetricIds")] = v.supportingMetricIds;
            vm[QStringLiteral("priority")] = v.priority;
            // The scope badge the view shows, worded once here.
            vm[QStringLiteral("scopeLabel")] =
                (v.scope == ta::training::VerdictScope::Session)
                    ? QStringLiteral("SESSION-LEVEL POSITION COMPARISON")
                    : (v.scope == ta::training::VerdictScope::Condition
                       ? QStringLiteral("CONDITION: %1").arg(v.comparedCondition)
                       : (v.positionName.isEmpty() ? QStringLiteral("THIS SESSION")
                                                   : v.positionName.toUpper()));
            vl.append(vm);
        }
        out[QStringLiteral("verdicts")] = vl;
    }

    // 9. WHAT THE DATA SUGGESTS — rendered, never re-worded by the view.
    QVariantList findings;
    for (const ta::training::Finding& f : a.findings) {
        QVariantMap fm;
        fm[QStringLiteral("category")] = ta::training::patternCategoryLabel(f.category);
        // UI-WIND-006: the scope travels with the finding so the view can filter
        // by position and label a session-level comparison as session-level.
        fm[QStringLiteral("scope")] = ta::training::findingScopeLabel(f.scope);
        fm[QStringLiteral("scopeIsSession")] = (f.scope == ta::training::FindingScope::Session);
        fm[QStringLiteral("position")] = f.position;
        fm[QStringLiteral("positionName")] = f.positionName;
        fm[QStringLiteral("conditionLabel")] = f.conditionLabel;
        fm[QStringLiteral("scopeLabel")] = (f.scope == ta::training::FindingScope::Session)
            ? QStringLiteral("SESSION-LEVEL POSITION COMPARISON")
            : (f.scope == ta::training::FindingScope::Condition
               ? QStringLiteral("CONDITION: %1").arg(f.conditionLabel)
               : (f.positionName.isEmpty() ? QStringLiteral("THIS SESSION")
                                           : f.positionName.toUpper()));
        fm[QStringLiteral("text")] = f.text;
        fm[QStringLiteral("suggestion")] = f.suggestion;
        fm[QStringLiteral("n")] = f.n;
        fm[QStringLiteral("shotsNeeded")] = f.shotsNeeded;
        findings.append(fm);
    }
    out[QStringLiteral("findings")] = findings;

    // 7. TIMELINE + the raw appendix rows. Same source, different projection:
    // the timeline carries the boundary markers, the rows carry every recorded
    // field the PDF appendix must reproduce.
    QVariantList timeline, shotRows;
    for (const ta::training::TimelineEntry& e : a.timeline) {
        QVariantMap tm;
        tm[QStringLiteral("shotId")] = e.shotId;
        tm[QStringLiteral("sighter")] = e.sighter;
        tm[QStringLiteral("type")] = e.sighter ? QStringLiteral("Sighter")
                                               : QStringLiteral("Counted");
        tm[QStringLiteral("position")] = e.position;
        tm[QStringLiteral("positionName")] = e.positionName;
        tm[QStringLiteral("xMm")] = e.xMm;
        tm[QStringLiteral("yMm")] = e.yMm;
        tm[QStringLiteral("score")] = e.score;
        tm[QStringLiteral("conditionLabel")] = e.conditionLabel;
        tm[QStringLiteral("conditionChangedBefore")] = e.conditionChangedBefore;
        tm[QStringLiteral("phaseChangedBefore")] = e.phaseChangedBefore;
        timeline.append(tm);

        // The appendix row keeps the IMMUTABLE snapshot exactly as recorded.
        QVariantMap rw = tm;
        rw[QStringLiteral("timestampMs")] = static_cast<qlonglong>(e.splitMs);
        rw[QStringLiteral("hasWindReading")] = e.wind.hasReading();
        rw[QStringLiteral("calm")] = e.wind.isCalm();
        rw[QStringLiteral("directionDegrees")] = e.wind.isMeasured() ? e.wind.directionDegrees : 0;
        rw[QStringLiteral("directionLabel")] = e.wind.isMeasured()
            ? ta::training::windSectorLabel(e.wind.sector()) : QString();
        rw[QStringLiteral("speedMetresPerSecond")] = e.wind.isMeasured()
            ? e.wind.speedMetresPerSecond() : 0.0;
        rw[QStringLiteral("note")] = e.wind.note;
        // DERIVED, never stored: the recorded compass value above is
        // untouched. Absent firing direction yields the unavailable label.
        const ta::training::RelativeWind rel = e.wind.isMeasured()
            ? ta::training::relativeWindFor(e.wind.directionDegrees, m_firingDirectionDeg)
            : ta::training::RelativeWind::Unavailable;
        rw[QStringLiteral("hasRelativeWind")] =
            (rel != ta::training::RelativeWind::Unavailable);
        rw[QStringLiteral("relativeWind")] = ta::training::relativeWindLabel(rel);
        shotRows.append(rw);
    }
    out[QStringLiteral("timeline")] = timeline;
    out[QStringLiteral("shotRows")] = shotRows;

    out[QStringLiteral("limitations")] = a.limitations;

    m_analysisKey = key;
    m_analysisCache = out;
    return out;
}

QVariantList WindMapController::reviewShots() const
{
    QVariantList out;
    if (!m_store || !m_store->active()) return out;
    for (const WindMapShotRecord& r : st().wmShots) {
        const WindConditionSnapshot w = snapshotFromRecord(r);
        QVariantMap m;
        m[QStringLiteral("shotId")] = static_cast<int>(r.shotId);
        m[QStringLiteral("sighter")] = r.sighter;
        m[QStringLiteral("position")] = static_cast<int>(r.position);
        m[QStringLiteral("positionName")] =
            windMapPositionName(static_cast<WindMapPosition>(r.position));
        m[QStringLiteral("xMm")] = r.shot.xHundredthMm / 100.0;
        m[QStringLiteral("yMm")] = r.shot.yHundredthMm / 100.0;
        m[QStringLiteral("score")] = r.shot.scoreTenths / 10.0;
        // Operator-facing wind values only — the stored hundredths never leave
        // the domain layer.
        m[QStringLiteral("hasWindReading")] = w.hasReading();
        m[QStringLiteral("calm")] = w.isCalm();
        m[QStringLiteral("directionDegrees")] = w.isMeasured() ? w.directionDegrees : 0;
        m[QStringLiteral("directionLabel")] = w.isMeasured() ? windSectorLabel(w.sector())
                                                             : QString();
        m[QStringLiteral("speedMetresPerSecond")] = w.isMeasured() ? w.speedMetresPerSecond() : 0.0;
        m[QStringLiteral("band")] = w.hasReading() ? windSpeedBandLabel(w.band()) : QString();
        m[QStringLiteral("note")] = w.note;
        out.append(m);
    }
    return out;
}

// ── plumbing ───────────────────────────────────────────────────────────────

bool WindMapController::submit(const DomainEvent& ev)
{
    const SubmitResult r = m_store->submit(ev);
    if (!r.ok) {
        m_lastError = r.error.operatorMessage;
        qWarning() << "WINDMAP: submit refused —" << r.error.technicalDetail;
        return false;
    }
    return true;
}

bool WindMapController::fail(const QString& message, const char* code)
{
    m_lastError = message;
    qWarning().noquote() << "WINDMAP refused —" << code;
    emit phaseChanged();       // republishes lastError to QML
    return false;
}
