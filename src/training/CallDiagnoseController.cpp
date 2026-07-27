#include "CallDiagnoseController.h"

#include "mode/OperatingMode.h"
#include "TargetGeometry.h"

#include <QCoreApplication>
#include <QDebug>
#include <QUuid>
#include <cmath>

using namespace ta::rel;
using namespace ta::training;

CallDiagnoseController::CallDiagnoseController(QObject* parent)
    : QObject(parent)
    , m_store(std::make_unique<SessionStore>())
{
    connect(m_store.get(), &SessionStore::journalWriteFailed,
            this, [this](QString path, QString detail) {
                emit journalWriteFailed(path, detail);
            });
}

CallDiagnoseController::~CallDiagnoseController() = default;

// ── configuration ──────────────────────────────────────────────────────────

void CallDiagnoseController::configureDefaults(const QString& disciplineId)
{
    Discipline d = Discipline::None;
    if (!disciplineFromId(disciplineId, &d)) {
        m_lastError = QStringLiteral("Unknown discipline '%1'.").arg(disciplineId);
        return;
    }
    m_cfg = CallDiagnoseConfig::defaultsFor(d);
    m_lastError.clear();
    emit configChanged();
}

void CallDiagnoseController::setShotCount(int n)
{
    m_cfg.shotCount = n;
    emit configChanged();
}

void CallDiagnoseController::setTechnicalFocus(const QString& focus)
{
    m_cfg.technicalFocus = focus;
    emit configChanged();
}

QString CallDiagnoseController::validateConfig() const { return m_cfg.validate(); }

QStringList CallDiagnoseController::focusOptionsForDiscipline() const
{
    return focusOptions(m_cfg.discipline == Discipline::AirPistol10m);
}

// ── lifecycle ────────────────────────────────────────────────────────────��─

bool CallDiagnoseController::startCallDiagnose(const QString& athlete)
{
    auto refuse = [this](const QString& userMsg, const char* code) {
        m_lastStartError = userMsg; m_lastError = userMsg;
        qWarning().noquote() << "CALLDIAG start refused —" << code
            << "| mode" << m_operatingMode
            << "| discipline" << disciplineId(m_cfg.discipline)
            << "| shots" << m_cfg.shotCount
            << "| storeActive" << (m_store && m_store->active());
        emit startErrorChanged();
        return false;
    };
    if (athlete.trimmed().isEmpty())
        return refuse(QStringLiteral("No athlete has been selected.\n\n"
                          "Enter or choose an athlete name, then start again."), "no-athlete");
    if (m_cfg.discipline == Discipline::None)
        return refuse(QStringLiteral("No programme has been configured.\n\n"
                          "Open Training Lab, choose Call & Diagnose and confirm the setup."), "no-programme");
    const QString err = m_cfg.validate();
    if (!err.isEmpty())
        return refuse(QStringLiteral("The Call & Diagnose setup is not valid.\n\n%1").arg(err), "invalid-config");
    if (m_store->active()) {
        qInfo() << "CALLDIAG: closing stale open session"
                << st().sessionId.left(8) << "before starting a new one";
        m_store->closeSession(CloseReason::Clean);
    }

    SessionHeader header;
    header.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    header.appVersion = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("dev") : QCoreApplication::applicationVersion();
    header.athlete = athlete.trimmed();
    header.matchType = QStringLiteral("TRAINING call_and_diagnose");
    header.discipline = m_cfg.discipline;
    header.sessionKind = QStringLiteral("Training");     // Training classification
    if (m_operatingMode >= 0)
        header.operatingMode =
            ta::mode::modeToConfigString(static_cast<ta::mode::Mode>(m_operatingMode));
    header.config.officialShots = 0;                     // NEVER an official record

    const ReliabilityResult r = m_store->beginSession(header);
    if (!r.ok)
        return refuse(QStringLiteral("The training session journal could not be created.\n\n%1")
                          .arg(r.error.operatorMessage), "journal-open-failed");
    m_lastStartError.clear();
    emit startErrorChanged();

    CallDiagnoseSessionStarted ev;
    ev.programId = QLatin1String(kProgramCallDiagnose);
    ev.shotCount = static_cast<qint16>(m_cfg.shotCount);
    ev.technicalFocus = m_cfg.technicalFocus;
    ev.startPosition = 0;
    ev.threePositions = m_cfg.threePositions;
    if (!submit(DomainEvent(ev))) return false;

    m_currentPosition = 0;
    m_lastExternalId = -1;
    m_hasPendingCall = false;
    m_pendingShotNumber = 0;
    m_sessionStartMonoMs = m_store->nowMonotonicMs();
    m_phase = 1;                          // Sighters
    emit phaseChanged(); emit progressChanged();
    return true;
}

bool CallDiagnoseController::startProgramme()
{
    if (m_phase != 1) { m_lastError = QStringLiteral("Sighters are not active."); return false; }
    CallDiagnoseStarted ev;
    ev.position = static_cast<qint8>(m_currentPosition);
    if (!submit(DomainEvent(ev))) return false;   // repeated tap: reducer moved on
    m_phase = 2;                                   // AwaitingShot
    emit sightersCleared();                        // clear sighter markers
    emit phaseChanged(); emit progressChanged();
    return true;
}

// THE classification boundary: source gate → (Sighters) sighter, (AwaitingShot)
// actual shot; every other phase refuses so a shot never pairs with the wrong
// call. Only ONE unresolved shot may exist at a time.
bool CallDiagnoseController::registerShot(double xMm, double yMm, double decimalScore,
                                          qint64 externalId, double directionDeg, int shotSource)
{
    if (m_operatingMode >= 0
        && !ta::mode::sourceAllowed(static_cast<ta::mode::Mode>(m_operatingMode),
                                    static_cast<ta::mode::ShotSource>(shotSource))) {
        emit shotRejected(QStringLiteral("WrongInputSource"));
        return false;
    }
    if (!m_store->active()) { emit shotRejected(QStringLiteral("CallDiagnoseNotActive")); return false; }
    if (!(decimalScore >= 0.0) || decimalScore > 11.0
        || !(xMm > -500.0 && xMm < 500.0) || !(yMm > -500.0 && yMm < 500.0)) {
        emit shotRejected(QStringLiteral("InvalidShotData"));
        return false;
    }
    if (externalId >= 0 && externalId <= m_lastExternalId) {
        emit shotRejected(QStringLiteral("DuplicateShot"));
        return false;
    }
    if (m_phase == 1)
        return acceptSighter(xMm, yMm, decimalScore, externalId, directionDeg, shotSource);
    if (m_phase == 2)
        return acceptActualShot(xMm, yMm, decimalScore, externalId, directionDeg, shotSource);
    // AwaitingCall / Reveal / Complete / Idle — refuse the next shot until the
    // current one is resolved (prevents a call pairing with the wrong actual).
    emit shotRejected(QStringLiteral("ResolveCurrentShotFirst"));
    return false;
}

bool CallDiagnoseController::acceptSighter(double xMm, double yMm, double decimalScore,
                                           qint64 externalId, double directionDeg, int shotSource)
{
    TrainingSighterAccepted ev;
    ev.position = static_cast<qint8>(m_currentPosition);
    ev.beforeBlock = 1;
    ShotCore& s = ev.shot;
    s.shotNumber = 0;
    s.xHundredthMm = static_cast<qint32>(qRound(xMm * 100.0));
    s.yHundredthMm = static_cast<qint32>(qRound(yMm * 100.0));
    s.scoreTenths = static_cast<qint16>(qBound<int>(0, qRound(decimalScore * 10.0), 110));
    s.directionCentiDeg = static_cast<qint32>(qRound(directionDeg * 100.0));
    s.splitMs = static_cast<qint32>(m_store->nowMonotonicMs() > 0
        ? qMin<qint64>(m_store->nowMonotonicMs(), INT32_MAX) : 0);
    s.externalId = externalId;
    s.simulated = (shotSource == 1);
    if (!submit(DomainEvent(ev))) return false;
    if (externalId >= 0) m_lastExternalId = externalId;
    QVariantMap rec;
    rec[QStringLiteral("xMm")] = xMm; rec[QStringLiteral("yMm")] = yMm;
    rec[QStringLiteral("score")] = decimalScore; rec[QStringLiteral("sighter")] = true;
    emit sighterAccepted(rec);
    emit progressChanged();
    return true;
}

bool CallDiagnoseController::acceptActualShot(double xMm, double yMm, double decimalScore,
                                              qint64 externalId, double directionDeg, int shotSource)
{
    const int shotNo = receivedInPosition(m_currentPosition) + 1;
    CallDiagnoseShotReceived ev;
    ev.shotNumber = static_cast<qint16>(shotNo);
    ev.position = static_cast<qint8>(m_currentPosition);
    ShotCore& s = ev.shot;
    s.shotNumber = static_cast<qint16>(shotNo);
    s.xHundredthMm = static_cast<qint32>(qRound(xMm * 100.0));
    s.yHundredthMm = static_cast<qint32>(qRound(yMm * 100.0));
    s.scoreTenths = static_cast<qint16>(qBound<int>(0, qRound(decimalScore * 10.0), 110));
    s.directionCentiDeg = static_cast<qint32>(qRound(directionDeg * 100.0));
    s.splitMs = static_cast<qint32>(m_store->nowMonotonicMs() > 0
        ? qMin<qint64>(m_store->nowMonotonicMs(), INT32_MAX) : 0);
    s.externalId = externalId;
    s.simulated = (shotSource == 1);
    if (!submit(DomainEvent(ev))) return false;
    if (externalId >= 0) m_lastExternalId = externalId;
    m_pendingShotNumber = static_cast<qint16>(shotNo);
    m_hasPendingCall = false;
    m_phase = 3;                          // AwaitingCall (actual stored HIDDEN)
    emit shotReceived();                  // no coords — the impact stays hidden
    emit phaseChanged(); emit progressChanged(); emit callChanged();
    return true;
}

bool CallDiagnoseController::submitCall(double calledXMm, double calledYMm)
{
    if (m_phase != 3) { m_lastError = QStringLiteral("No shot is awaiting a call."); return false; }
    if (!(calledXMm > -500.0 && calledXMm < 500.0 && calledYMm > -500.0 && calledYMm < 500.0)) {
        m_lastError = QStringLiteral("Call position is off the target."); return false;
    }
    m_pendingCalledXMm = calledXMm;
    m_pendingCalledYMm = calledYMm;
    m_hasPendingCall = true;
    emit callChanged();
    return true;
}

void CallDiagnoseController::clearCall()
{
    m_hasPendingCall = false;
    emit callChanged();
}

bool CallDiagnoseController::confirmCall()
{
    if (m_phase != 3) { m_lastError = QStringLiteral("No shot is awaiting a call."); return false; }
    if (!m_hasPendingCall) { m_lastError = QStringLiteral("Place your call first."); return false; }
    CallDiagnoseCallRecorded ev;
    ev.shotNumber = m_pendingShotNumber;
    ev.position = static_cast<qint8>(m_currentPosition);
    ev.calledXHundredthMm = static_cast<qint32>(qRound(m_pendingCalledXMm * 100.0));
    ev.calledYHundredthMm = static_cast<qint32>(qRound(m_pendingCalledYMm * 100.0));
    ev.callSplitMs = static_cast<qint32>(m_store->nowMonotonicMs() > 0
        ? qMin<qint64>(m_store->nowMonotonicMs(), INT32_MAX) : 0);
    if (!submit(DomainEvent(ev))) return false;
    m_hasPendingCall = false;
    m_phase = 4;                          // Reveal
    emit callRevealed(revealCurrent());
    emit phaseChanged(); emit progressChanged(); emit callChanged();
    return true;
}

bool CallDiagnoseController::saveShotNote(const QString& note)
{
    if (m_phase != 4 && m_phase != 5) {
        m_lastError = QStringLiteral("Notes are saved after the reveal."); return false;
    }
    const CallDiagnoseShotRecord* rec = lastResolvedShot();
    if (!rec) return false;
    CallDiagnoseNoteSaved ev;
    ev.shotNumber = rec->shotNumber;
    ev.position = rec->position;
    ev.note = note;
    return submit(DomainEvent(ev));
}

bool CallDiagnoseController::continueToNext()
{
    if (m_phase != 4) { m_lastError = QStringLiteral("Nothing to continue from."); return false; }
    const int doneInPos = completedInPosition(m_currentPosition);
    if (doneInPos >= m_cfg.shotCount) {
        // position finished
        if (m_cfg.threePositions && m_currentPosition < 2) {
            TrainingSighterPhaseStarted sp;
            sp.position = static_cast<qint8>(m_currentPosition + 1);
            sp.beforeBlock = 1;
            if (!submit(DomainEvent(sp))) return false;
            ++m_currentPosition;
            m_phase = 1;                  // next position's Sighters
            emit sightersCleared();
            emit phaseChanged(); emit progressChanged();
            return true;
        }
        // whole session finished
        int total = 0;
        for (const CallDiagnoseShotRecord& r : st().cdShots) if (r.hasCall) ++total;
        CallDiagnoseCompleted ev;
        ev.completedShots = static_cast<qint16>(total);
        if (!submit(DomainEvent(ev))) return false;
        m_phase = 5;                      // Complete
        emit completedChanged();
        emit phaseChanged(); emit progressChanged();
        return true;
    }
    // more shots in this position
    m_phase = 2;                          // AwaitingShot
    emit revealCleared();                 // clear call/actual markers
    emit phaseChanged(); emit progressChanged();
    return true;
}

bool CallDiagnoseController::endEarly()
{
    if (m_phase == 0 || m_phase == 5) { m_lastError = QStringLiteral("Nothing to end."); return false; }
    if (m_phase == 3) { m_lastError = QStringLiteral("Confirm or clear the current call first."); return false; }
    int total = 0;
    for (const CallDiagnoseShotRecord& r : st().cdShots) if (r.hasCall) ++total;
    CallDiagnoseCompleted ev;
    ev.completedShots = static_cast<qint16>(total);
    if (!submit(DomainEvent(ev))) return false;
    m_phase = 5;
    emit completedChanged();
    emit phaseChanged(); emit progressChanged();
    return true;
}

// ── close / recovery ───────────────────────────────────────────────────────

bool CallDiagnoseController::closeCleanly()
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
    m_phase = 0; m_currentPosition = 0; m_lastExternalId = -1; m_hasPendingCall = false;
    emit phaseChanged(); emit progressChanged();
    return true;
}

void CallDiagnoseController::resetCallDiagnose() { closeCleanly(); }

bool CallDiagnoseController::resumeFromRecovery(const QString& sessionId)
{
    if (!m_recovery) m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();
    RecoveredMatchState rec; ErrorInfo err;
    if (!m_recovery->buildRecoveredState(sessionId, &rec, &err)) {
        emit journalWriteFailed(rec.journalPath, err.technicalDetail);
        return false;
    }
    if (rec.state.sessionKind != QLatin1String("Training")
        || rec.state.cdProgramId != QLatin1String(kProgramCallDiagnose)) {
        m_lastError = QStringLiteral("Not a Call & Diagnose session.");
        return false;
    }
    const ReliabilityResult rr = m_store->resumeSession(rec);
    if (!rr.ok) { emit journalWriteFailed(rec.journalPath, rr.error.technicalDetail); return false; }

    const SessionState& s = st();
    m_cfg = CallDiagnoseConfig::defaultsFor(s.discipline);
    m_cfg.discipline = s.discipline;
    m_cfg.shotCount = s.cdShotCount;
    m_cfg.threePositions = s.cdThreePositions;
    m_cfg.technicalFocus = s.cdFocus;
    m_currentPosition = s.cdCurrentPosition;
    m_hasPendingCall = false;

    // duplicate guard past every recovered externalId (actual shots + sighters)
    m_lastExternalId = -1;
    for (const CallDiagnoseShotRecord& r : s.cdShots)
        if (r.actual.externalId > m_lastExternalId) m_lastExternalId = r.actual.externalId;
    for (const ShotCore& sc : s.trainingSighters)
        if (sc.externalId > m_lastExternalId) m_lastExternalId = sc.externalId;

    // phase re-derivation from durable state only
    if (s.cdCompleted) {
        m_phase = 5;
        emit completedChanged();
    } else {
        const CallDiagnoseShotRecord* pending = currentUnresolvedShot();
        if (pending) {
            // actual received, call not recorded → AwaitingCall (hidden)
            m_currentPosition = pending->position;
            m_pendingShotNumber = pending->shotNumber;
            m_phase = 3;
        } else if (s.trainingInSighterPhase || !s.cdCallingActive) {
            m_phase = 1;                  // Sighters
        } else {
            m_phase = 2;                  // AwaitingShot
        }
    }
    m_sessionStartMonoMs = m_store->nowMonotonicMs();
    emit configChanged(); emit phaseChanged(); emit progressChanged(); emit callChanged();
    qInfo() << "CALLDIAG: resumed" << s.sessionId.left(8) << "phase" << m_phase
            << "pos" << m_currentPosition;
    return true;
}

void CallDiagnoseController::discardRecovery(const QString& sessionId)
{
    if (!m_recovery) m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();
    m_recovery->archiveOrDiscard(sessionId, /*discarded*/ true);
}

QVariantList CallDiagnoseController::recoveredSighterShots() const
{
    QVariantList out;
    if (!m_store || m_phase != 1) return out;
    const QVector<ShotCore>& s = st().trainingSighters;
    const QVector<qint8>& p = st().trainingSighterPos;
    for (int i = 0; i < s.size(); ++i) {
        if (i < p.size() && static_cast<int>(p[i]) != m_currentPosition) continue;
        QVariantMap m;
        m[QStringLiteral("xMm")] = s[i].xHundredthMm / 100.0;
        m[QStringLiteral("yMm")] = s[i].yHundredthMm / 100.0;
        out.append(m);
    }
    return out;
}

// ── projections ────────────────────────────────────────────────────────────

int CallDiagnoseController::completedInPosition(int pos) const
{
    if (!m_store) return 0;
    int n = 0;
    for (const CallDiagnoseShotRecord& r : st().cdShots)
        if (r.position == pos && r.hasCall) ++n;
    return n;
}

int CallDiagnoseController::receivedInPosition(int pos) const
{
    if (!m_store) return 0;
    int n = 0;
    for (const CallDiagnoseShotRecord& r : st().cdShots)
        if (r.position == pos) ++n;
    return n;
}

int CallDiagnoseController::shotsCompleted() const { return completedInPosition(m_currentPosition); }

int CallDiagnoseController::pendingShotNumber() const
{
    if (m_phase == 3 || m_phase == 4) return m_pendingShotNumber;
    return completedInPosition(m_currentPosition) + 1;   // next to be called
}

int CallDiagnoseController::sighterCount() const
{
    if (!m_store) return 0;
    const QVector<qint8>& p = st().trainingSighterPos;
    int n = 0;
    for (qint8 pos : p) if (static_cast<int>(pos) == m_currentPosition) ++n;
    return n;
}

QString CallDiagnoseController::positionName() const
{
    if (!m_cfg.threePositions) return QString();
    return positionLabel(static_cast<Position>(qBound(0, m_currentPosition, 2)));
}

QString CallDiagnoseController::startLabel() const
{
    if (!m_cfg.threePositions) return QStringLiteral("Start Call & Diagnose");
    return QStringLiteral("Start %1 Calls").arg(positionName());
}

QString CallDiagnoseController::sessionId() const { return m_store ? st().sessionId : QString(); }
QString CallDiagnoseController::sessionOperatingMode() const { return m_store ? st().operatingMode : QString(); }

const CallDiagnoseShotRecord* CallDiagnoseController::currentUnresolvedShot() const
{
    if (!m_store) return nullptr;
    const QVector<CallDiagnoseShotRecord>& v = st().cdShots;
    for (int i = v.size() - 1; i >= 0; --i)
        if (!v[i].hasCall) return &v[i];
    return nullptr;
}

const CallDiagnoseShotRecord* CallDiagnoseController::lastResolvedShot() const
{
    if (!m_store) return nullptr;
    const QVector<CallDiagnoseShotRecord>& v = st().cdShots;
    for (int i = v.size() - 1; i >= 0; --i)
        if (v[i].hasCall) return &v[i];
    return nullptr;
}

namespace {
QString bandKey(double errMm, double ring)
{
    if (ring <= 0.0) return QStringLiteral("more");
    const double r = errMm / ring;
    if (r <= 0.5) return QStringLiteral("half");
    if (r <= 1.0) return QStringLiteral("one");
    return QStringLiteral("more");
}
QString bandText(const QString& key)
{
    if (key == QLatin1String("half")) return QStringLiteral("Within half a ring");
    if (key == QLatin1String("one"))  return QStringLiteral("Within one ring");
    return QStringLiteral("More than one ring");
}
QString horizSentence(double ex)
{
    if (std::fabs(ex) < 0.05)
        return QStringLiteral("Your call and the measured impact were horizontally aligned.");
    return QStringLiteral("Your call was %1 mm %2 of the measured impact.")
        .arg(std::fabs(ex), 0, 'f', 1).arg(ex >= 0 ? "right" : "left");
}
QString vertSentence(double ey)
{
    if (std::fabs(ey) < 0.05)
        return QStringLiteral("Your call and the measured impact were vertically aligned.");
    return QStringLiteral("Your call was %1 mm %2 of the measured impact.")
        .arg(std::fabs(ey), 0, 'f', 1).arg(ey >= 0 ? "high" : "low");
}
} // namespace

double CallDiagnoseController::ringSpacingMm() const { return issfRingSpacingMm(m_cfg.discipline); }
double CallDiagnoseController::faceRadiusMm() const { return issfFaceRadiusMm(m_cfg.discipline); }

QVariantMap CallDiagnoseController::revealCurrent() const
{
    QVariantMap m;
    // Only in Reveal / Complete — the pending actual stays hidden in every
    // other phase (AwaitingShot / AwaitingCall show no reveal data).
    if (m_phase != 4 && m_phase != 5) return m;
    const CallDiagnoseShotRecord* r = lastResolvedShot();
    if (!r) return m;
    const double ax = r->actual.xHundredthMm / 100.0, ay = r->actual.yHundredthMm / 100.0;
    const double cx = r->calledXHundredthMm / 100.0, cy = r->calledYHundredthMm / 100.0;
    const double ex = cx - ax, ey = cy - ay;          // signed: call − actual
    const double err = std::sqrt(ex * ex + ey * ey);
    const double ring = ringSpacingMm();
    const double face = faceRadiusMm();
    m[QStringLiteral("shotNumber")] = r->shotNumber;
    m[QStringLiteral("actualXMm")] = ax; m[QStringLiteral("actualYMm")] = ay;
    m[QStringLiteral("calledXMm")] = cx; m[QStringLiteral("calledYMm")] = cy;
    m[QStringLiteral("errorXMm")] = ex; m[QStringLiteral("errorYMm")] = ey;
    m[QStringLiteral("errorMm")] = err;
    m[QStringLiteral("actualScore")] = r->actual.scoreTenths / 10.0;
    m[QStringLiteral("exact")] = (err < 0.05);
    // legacy short phrases (kept for the compact panel)
    m[QStringLiteral("xPhrase")] = (std::fabs(ex) < 0.05) ? QStringLiteral("on line")
        : QStringLiteral("%1 mm %2").arg(std::fabs(ex), 0, 'f', 1).arg(ex >= 0 ? "right" : "left");
    m[QStringLiteral("yPhrase")] = (std::fabs(ey) < 0.05) ? QStringLiteral("on line")
        : QStringLiteral("%1 mm %2").arg(std::fabs(ey), 0, 'f', 1).arg(ey >= 0 ? "high" : "low");
    // T2.1: full sentences, ring normalisation, band, adaptive bounds
    m[QStringLiteral("horizontalSentence")] = horizSentence(ex);
    m[QStringLiteral("verticalSentence")] = vertSentence(ey);
    m[QStringLiteral("errorRingSpacings")] = (ring > 0.0) ? err / ring : 0.0;
    const QString bk = bandKey(err, ring);
    m[QStringLiteral("band")] = bk;
    m[QStringLiteral("bandText")] = bandText(bk);
    const CompareBounds cb = comparisonBounds(cx, cy, ax, ay, ring, 3.0);
    const CompareBounds tb = targetBounds(cx, cy, ax, ay, face, 3.0);
    m[QStringLiteral("comparisonHalfRangeMm")] = cb.halfRangeMm;
    m[QStringLiteral("targetHalfRangeMm")] = tb.halfRangeMm;
    m[QStringLiteral("faceRadiusMm")] = face;
    m[QStringLiteral("outsideFace")] = tb.outsideFace;
    // one measured immediate observation (compare vs earlier calls this session)
    QVector<double> priorErr;
    double prevErr = -1.0, minPrior = 1e18;
    for (const CallDiagnoseShotRecord& rec : st().cdShots) {
        if (!rec.hasCall) continue;
        const double dx = rec.calledXHundredthMm / 100.0 - rec.actual.xHundredthMm / 100.0;
        const double dy = rec.calledYHundredthMm / 100.0 - rec.actual.yHundredthMm / 100.0;
        const double e = std::sqrt(dx * dx + dy * dy);
        if (rec.shotNumber == r->shotNumber && rec.position == r->position) break;
        priorErr.append(e); prevErr = e; if (e < minPrior) minPrior = e;
    }
    QString feedback;
    if (bk == QLatin1String("half")) feedback = QStringLiteral("Your call was within half a ring of the measured impact.");
    else if (bk == QLatin1String("one")) feedback = QStringLiteral("Your call was within one ring of the measured impact.");
    else feedback = QStringLiteral("Your call was more than one ring from the measured impact.");
    if (!priorErr.isEmpty()) {
        if (err < minPrior - 0.05) feedback = QStringLiteral("This was your closest call of the session so far.");
        else if (prevErr >= 0.0 && err < prevErr - 0.05) feedback = QStringLiteral("This call was closer than your previous call.");
        else if (prevErr >= 0.0 && err > prevErr + 0.05) feedback = QStringLiteral("This call was wider than your previous call.");
    }
    m[QStringLiteral("feedback")] = feedback;
    // call-quality summary (band-based, neutral)
    m[QStringLiteral("callQuality")] = bandText(bk)
        + (ring > 0.0 ? QStringLiteral(" (~%1 ring spacings)").arg(err / ring, 0, 'f', 1) : QString());
    return m;
}

QVariantMap CallDiagnoseController::sessionStats(int positionFilter) const
{
    QVariantMap out;
    if (!m_store) return out;
    QVector<CallDiagnoseShotRecord> scoped;
    for (const CallDiagnoseShotRecord& r : st().cdShots)
        if (positionFilter < 0 || r.position == positionFilter) scoped.append(r);
    const QVector<CallShotStat> stats = computeCallShotStats(scoped);
    const CallSessionStats s = computeCallSessionStats(stats);
    out[QStringLiteral("count")] = s.count;
    out[QStringLiteral("averageError")] = s.averageError;
    out[QStringLiteral("medianError")] = s.medianError;
    out[QStringLiteral("smallestError")] = s.smallestError;
    out[QStringLiteral("largestError")] = s.largestError;
    out[QStringLiteral("errorStdDev")] = s.errorStdDev;
    out[QStringLiteral("avgAbsX")] = s.avgAbsX;
    out[QStringLiteral("avgAbsY")] = s.avgAbsY;
    out[QStringLiteral("biasX")] = s.biasX;
    out[QStringLiteral("biasY")] = s.biasY;
    out[QStringLiteral("hasBias")] = s.hasBias;
    out[QStringLiteral("trendSlope")] = s.trendSlope;
    out[QStringLiteral("hasTrend")] = s.hasTrend;
    out[QStringLiteral("bestShotNumber")] = s.bestShotNumber;
    out[QStringLiteral("worstShotNumber")] = s.worstShotNumber;
    return out;
}

QStringList CallDiagnoseController::sessionObservations() const
{
    QStringList out;
    if (!m_store) return out;
    const CallSessionStats s = computeCallSessionStats(computeCallShotStats(st().cdShots));
    if (s.count == 0) return out;
    out << QStringLiteral("Average call error was %1 mm across %2 called shots.")
               .arg(s.averageError, 0, 'f', 1).arg(s.count);
    if (s.hasBias) {
        if (std::fabs(s.biasX) >= 0.5)
            out << QStringLiteral("Calls averaged %1 mm %2 of the measured impacts.")
                       .arg(std::fabs(s.biasX), 0, 'f', 1).arg(s.biasX >= 0 ? "right" : "left");
        if (std::fabs(s.biasY) >= 0.5)
            out << QStringLiteral("Calls averaged %1 mm %2.")
                       .arg(std::fabs(s.biasY), 0, 'f', 1).arg(s.biasY >= 0 ? "high" : "low");
        if (std::fabs(s.biasX) < 0.5 && std::fabs(s.biasY) < 0.5)
            out << QStringLiteral("No clear directional call bias was measured.");
        if (s.avgAbsX > s.avgAbsY * 1.25)
            out << QStringLiteral("Horizontal call error was greater than vertical call error.");
        else if (s.avgAbsY > s.avgAbsX * 1.25)
            out << QStringLiteral("Vertical call error was greater than horizontal call error.");
    }
    if (s.hasTrend && std::fabs(s.trendSlope) >= 0.02)
        out << (s.trendSlope < 0
                ? QStringLiteral("Call accuracy tended to improve through the session.")
                : QStringLiteral("Call error tended to increase through the session."));
    return out;
}

QVariantList CallDiagnoseController::shotReviewList() const
{
    QVariantList out;
    if (!m_store) return out;
    // Only REVEALED shots (a completed call). Never expose an unresolved actual.
    const QVector<CallShotStat> stats = computeCallShotStats(st().cdShots);
    const CallSessionStats ss = computeCallSessionStats(stats);
    const double ring = ringSpacingMm();
    // identify review-worthy shots (largest/closest/largest H/largest V)
    int largestH = -1, largestV = -1; double bH = -1, bV = -1;
    for (const CallShotStat& s : stats) {
        if (std::fabs(s.errorXMm) > bH) { bH = std::fabs(s.errorXMm); largestH = s.shotNumber; }
        if (std::fabs(s.errorYMm) > bV) { bV = std::fabs(s.errorYMm); largestV = s.shotNumber; }
    }
    for (const CallShotStat& s : stats) {
        QVariantMap m;
        m[QStringLiteral("shotNumber")] = s.shotNumber;
        m[QStringLiteral("position")] = s.position;
        m[QStringLiteral("positionName")] = m_cfg.threePositions
            ? positionLabel(static_cast<Position>(s.position)) : QString();
        m[QStringLiteral("actualXMm")] = s.actualXMm; m[QStringLiteral("actualYMm")] = s.actualYMm;
        m[QStringLiteral("calledXMm")] = s.calledXMm; m[QStringLiteral("calledYMm")] = s.calledYMm;
        m[QStringLiteral("errorMm")] = s.errorMm;
        m[QStringLiteral("errorXMm")] = s.errorXMm; m[QStringLiteral("errorYMm")] = s.errorYMm;
        m[QStringLiteral("actualScore")] = s.actualScore;
        m[QStringLiteral("errorRingSpacings")] = (ring > 0.0) ? s.errorMm / ring : 0.0;
        const QString bk = bandKey(s.errorMm, ring);
        m[QStringLiteral("band")] = bk;
        m[QStringLiteral("bandText")] = bandText(bk);
        m[QStringLiteral("horizontalSentence")] = horizSentence(s.errorXMm);
        m[QStringLiteral("verticalSentence")] = vertSentence(s.errorYMm);
        const CompareBounds cb = comparisonBounds(s.calledXMm, s.calledYMm, s.actualXMm, s.actualYMm, ring, 3.0);
        m[QStringLiteral("comparisonHalfRangeMm")] = cb.halfRangeMm;
        m[QStringLiteral("isOutlier")] = (ss.outlierCount > 0 && s.errorMm > ss.outlierThreshold);
        // neutral review tag
        QString tag;
        if (s.shotNumber == ss.bestShotNumber) tag = QStringLiteral("closest");
        else if (s.shotNumber == ss.worstShotNumber) tag = QStringLiteral("largest difference");
        else if (s.shotNumber == largestH) tag = QStringLiteral("largest horizontal");
        else if (s.shotNumber == largestV) tag = QStringLiteral("largest vertical");
        m[QStringLiteral("reviewTag")] = tag;
        for (const CallDiagnoseShotRecord& r : st().cdShots)
            if (r.position == s.position && r.shotNumber == s.shotNumber) {
                m[QStringLiteral("note")] = r.note; break;
            }
        out.append(m);
    }
    return out;
}

QVariantMap CallDiagnoseController::callInsights() const
{
    QVariantMap ins;
    if (!m_store) return ins;
    const QVector<CallShotStat> stats = computeCallShotStats(st().cdShots);
    const CallSessionStats s = computeCallSessionStats(stats);
    const double ring = ringSpacingMm();
    ins[QStringLiteral("count")] = s.count;
    ins[QStringLiteral("ringSpacingMm")] = ring;
    if (s.count == 0) { ins[QStringLiteral("hasData")] = false; return ins; }
    ins[QStringLiteral("hasData")] = true;

    // 1) TYPICAL CALL ACCURACY (median is primary)
    int withinHalf = 0, withinOne = 0;
    for (const CallShotStat& st1 : stats) {
        const QString bk = bandKey(st1.errorMm, ring);
        if (bk == QLatin1String("half")) { ++withinHalf; ++withinOne; }
        else if (bk == QLatin1String("one")) ++withinOne;
    }
    ins[QStringLiteral("medianError")] = s.medianError;
    ins[QStringLiteral("medianRingSpacings")] = (ring > 0.0) ? s.medianError / ring : 0.0;
    ins[QStringLiteral("averageError")] = s.averageError;
    ins[QStringLiteral("largestError")] = s.largestError;
    ins[QStringLiteral("withinHalfRing")] = withinHalf;
    ins[QStringLiteral("withinOneRing")] = withinOne;
    ins[QStringLiteral("typicalText")] = QStringLiteral(
        "Your median call difference was %1 mm. Half of your calls were within %1 mm of the measured impact.")
        .arg(s.medianError, 0, 'f', 1);
    ins[QStringLiteral("withinText")] = QStringLiteral("%1 of %2 calls were within one ring spacing.")
        .arg(withinOne).arg(s.count);

    // 2) AVERAGE vs MEDIAN / OUTLIER explanation
    QString avgVsMedian;
    if (s.averageError > s.medianError * 1.25 + 0.5 && s.outlierCount > 0)
        avgVsMedian = QStringLiteral(
            "Your typical call was about %1 mm from the measured impact. The %2 mm average was raised by %3 large call difference%4, including a %5 mm outlier.")
            .arg(s.medianError, 0, 'f', 1).arg(s.averageError, 0, 'f', 1)
            .arg(s.outlierCount).arg(s.outlierCount == 1 ? "" : "s").arg(s.largestError, 0, 'f', 1);
    else
        avgVsMedian = QStringLiteral("Your average (%1 mm) and median (%2 mm) call differences were similar.")
            .arg(s.averageError, 0, 'f', 1).arg(s.medianError, 0, 'f', 1);
    ins[QStringLiteral("averageVsMedian")] = avgVsMedian;
    ins[QStringLiteral("outlierCount")] = s.outlierCount;

    // 3) DIRECTIONAL BIAS (measured — never a sight-adjustment recommendation)
    QStringList bias;
    if (s.hasBias) {
        if (std::fabs(s.biasX) >= 0.5)
            bias << QStringLiteral("Calls averaged %1 mm %2 of the measured impacts.")
                       .arg(std::fabs(s.biasX), 0, 'f', 1).arg(s.biasX >= 0 ? "right" : "left");
        if (std::fabs(s.biasY) >= 0.5)
            bias << QStringLiteral("Calls averaged %1 mm %2.")
                       .arg(std::fabs(s.biasY), 0, 'f', 1).arg(s.biasY >= 0 ? "high" : "low");
        if (bias.isEmpty())
            bias << QStringLiteral("No clear directional call bias was measured.");
    } else {
        bias << QStringLiteral("Too few calls to measure a directional bias.");
    }
    ins[QStringLiteral("biasSentences")] = bias;
    ins[QStringLiteral("biasCaveat")] = QStringLiteral(
        "This describes a difference in shot perception. It does not indicate that the sights or the shots should be moved.");

    // 4) CONSISTENCY
    ins[QStringLiteral("consistencySd")] = s.errorStdDev;
    ins[QStringLiteral("consistencyText")] = QStringLiteral(
        "Call differences varied by about %1 mm. Lower variation means more repeatable call accuracy.")
        .arg(s.errorStdDev, 0, 'f', 1);

    // 5) TREND
    QString trend;
    if (!s.hasTrend) trend = QStringLiteral("Too few completed calls to determine a meaningful trend.");
    else if (s.hasHalves && s.secondHalfAvg < s.firstHalfAvg - 0.5)
        trend = QStringLiteral("Call accuracy improved through the session (later calls averaged %1 mm vs %2 mm early).")
            .arg(s.secondHalfAvg, 0, 'f', 1).arg(s.firstHalfAvg, 0, 'f', 1);
    else if (s.hasHalves && s.secondHalfAvg > s.firstHalfAvg + 0.5)
        trend = QStringLiteral("Call differences became larger in the later shots (%1 mm vs %2 mm early).")
            .arg(s.secondHalfAvg, 0, 'f', 1).arg(s.firstHalfAvg, 0, 'f', 1);
    else trend = QStringLiteral("Call accuracy remained broadly stable through the session.");
    ins[QStringLiteral("trendText")] = trend;

    // SHOTS TO REVIEW (neutral, measured)
    QVariantList review;
    auto addReview = [&](int shotNo, const QString& text) {
        if (shotNo <= 0) return;
        for (const QVariant& v : review) if (v.toMap().value(QStringLiteral("shotNumber")).toInt() == shotNo) return;
        QVariantMap rm; rm[QStringLiteral("shotNumber")] = shotNo; rm[QStringLiteral("text")] = text;
        review.append(rm);
    };
    addReview(s.worstShotNumber, QStringLiteral("Largest call difference (%1 mm).").arg(s.largestError, 0, 'f', 1));
    addReview(s.bestShotNumber, QStringLiteral("Closest call of the session (%1 mm).").arg(s.smallestError, 0, 'f', 1));
    // a low-scoring shot that was called accurately (awareness despite a poor result)
    int awareLow = 0; double awareLowScore = 99;
    for (const CallShotStat& st1 : stats)
        if (bandKey(st1.errorMm, ring) == QLatin1String("half") && st1.actualScore < 8.0 && st1.actualScore < awareLowScore) {
            awareLow = st1.shotNumber; awareLowScore = st1.actualScore;
        }
    if (awareLow > 0)
        addReview(awareLow, QStringLiteral("A low-scoring shot you still called accurately — good awareness."));
    ins[QStringLiteral("reviewShots")] = review;

    // AWARENESS vs RESULT groups (neutral; central = score >= 9.0)
    int cc = 0, co = 0, wc = 0, wo = 0;
    for (const CallShotStat& st1 : stats) {
        const bool close = bandKey(st1.errorMm, ring) != QLatin1String("more");
        const bool central = st1.actualScore >= 9.0;
        if (close && central) ++cc; else if (close && !central) ++co;
        else if (!close && central) ++wc; else ++wo;
    }
    QVariantMap aw;
    aw[QStringLiteral("closeCentral")] = cc; aw[QStringLiteral("closeOuter")] = co;
    aw[QStringLiteral("wideCentral")] = wc; aw[QStringLiteral("wideOuter")] = wo;
    ins[QStringLiteral("awareness")] = aw;
    ins[QStringLiteral("awarenessText")] = QStringLiteral(
        "Call accuracy and shot score are different measurements. A low-scoring shot can still be accurately called; a high-scoring shot can be poorly called.");
    return ins;
}

int CallDiagnoseController::sessionDurationSec() const
{
    if (!m_store) return 0;
    qint64 lo = -1, hi = -1;
    for (const CallDiagnoseShotRecord& r : st().cdShots) {
        if (lo < 0 || r.actual.splitMs < lo) lo = r.actual.splitMs;
        if (hi < 0 || r.actual.splitMs > hi) hi = r.actual.splitMs;
    }
    if (lo < 0 || hi <= lo) return 0;
    return static_cast<int>((hi - lo) / 1000);
}

QVariantMap CallDiagnoseController::reportModel() const
{
    QVariantMap r;
    if (!m_store) return r;
    const SessionState& s = st();
    r[QStringLiteral("programme")] = QStringLiteral("Call & Diagnose");
    r[QStringLiteral("sessionId")] = s.sessionId;
    r[QStringLiteral("athlete")] = s.athlete;
    r[QStringLiteral("createdAtIso")] = s.createdAtIso;
    r[QStringLiteral("operatingMode")] = s.operatingMode.isEmpty()
        ? QStringLiteral("Legacy") : s.operatingMode;
    r[QStringLiteral("focus")] = m_cfg.technicalFocus;
    r[QStringLiteral("threePositions")] = m_cfg.threePositions;
    r[QStringLiteral("shotCount")] = m_cfg.shotCount;
    r[QStringLiteral("plannedShots")] = m_cfg.totalShots();
    int called = 0, sighters = st().trainingSighters.size();
    for (const CallDiagnoseShotRecord& rec : s.cdShots) if (rec.hasCall) ++called;
    r[QStringLiteral("calledShots")] = called;
    r[QStringLiteral("sighters")] = sighters;
    r[QStringLiteral("durationSec")] = sessionDurationSec();
    r[QStringLiteral("stats")] = sessionStats(-1);
    r[QStringLiteral("observations")] = sessionObservations();
    r[QStringLiteral("insights")] = callInsights();
    r[QStringLiteral("ringSpacingMm")] = ringSpacingMm();
    r[QStringLiteral("shots")] = shotReviewList();
    r[QStringLiteral("sessionNote")] = s.cdSessionNote;
    r[QStringLiteral("completed")] = s.cdCompleted;
    if (m_cfg.threePositions) {
        QVariantList byPos;
        for (int p = 0; p < 3; ++p) {
            QVariantMap pm = sessionStats(p);
            pm[QStringLiteral("positionName")] = positionLabel(static_cast<Position>(p));
            byPos.append(pm);
        }
        r[QStringLiteral("byPosition")] = byPos;
    }
    return r;
}

bool CallDiagnoseController::submit(const DomainEvent& ev)
{
    const SubmitResult r = m_store->submit(ev);
    if (!r.ok) {
        m_lastError = r.error.operatorMessage;
        qWarning() << "CALLDIAG: submit refused —" << r.error.technicalDetail;
        return false;
    }
    return true;
}
