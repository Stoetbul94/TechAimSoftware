#include "PositionTransitionController.h"

#include "mode/OperatingMode.h"
#include "TargetGeometry.h"
#include "TrainingBlockMetrics.h"
#include "GroupPatternAnalyzer.h"

#include <QCoreApplication>
#include <QDebug>
#include <QUuid>
#include <climits>
#include <cmath>

using namespace ta::rel;
using namespace ta::training;

PositionTransitionController::PositionTransitionController(QObject* parent)
    : QObject(parent)
    , m_store(std::make_unique<SessionStore>())
{
    connect(m_store.get(), &SessionStore::journalWriteFailed,
            this, [this](QString path, QString detail) { emit journalWriteFailed(path, detail); });
    m_cfg = PositionTransitionConfig::defaults();
}

PositionTransitionController::~PositionTransitionController() = default;

// ── configuration ──────────────────────────────────────────────────────────

void PositionTransitionController::configureDefaults()
{
    m_cfg = PositionTransitionConfig::defaults();
    m_lastError.clear();
    emit configChanged();
}

void PositionTransitionController::setSequencePreset(int preset)
{
    switch (preset) {
    case 0: m_cfg.sequence = { 0, 1, 2 }; break;   // full 3P
    case 1: m_cfg.sequence = { 0, 1 };    break;   // Kneeling → Prone
    case 2: m_cfg.sequence = { 1, 2 };    break;   // Prone → Standing
    case 3: m_cfg.sequence = { 0 };       break;   // single Kneeling (repeat)
    case 4: m_cfg.sequence = { 1 };       break;   // single Prone
    case 5: m_cfg.sequence = { 2 };       break;   // single Standing
    default: m_cfg.sequence = { 0, 1, 2 }; break;
    }
    emit configChanged();
}

void PositionTransitionController::setVerificationShots(int n) { m_cfg.verificationShots = n; emit configChanged(); }
void PositionTransitionController::setRepeats(int n) { m_cfg.repeats = n; emit configChanged(); }
void PositionTransitionController::setChecklistMode(int mode) { m_cfg.checklistMode = qBound(0, mode, 2); emit configChanged(); }
void PositionTransitionController::setTechnicalFocus(const QString& focus) { m_cfg.technicalFocus = focus; emit configChanged(); }
QString PositionTransitionController::validateConfig() const { return m_cfg.validate(); }
QStringList PositionTransitionController::focusOptionsForDiscipline() const { return focusOptions(false); }
QString PositionTransitionController::sequenceString() const { return m_cfg.sequenceString(); }

// ── helpers ──────────────────────────────────────────────────────────────

int PositionTransitionController::currentPositionId() const
{
    if (m_cfg.sequence.isEmpty()) return 0;
    return m_cfg.sequence[m_seqIndex % m_cfg.sequence.size()];
}

int PositionTransitionController::repeatForIndex(int seqIndex) const
{
    if (m_cfg.sequence.isEmpty()) return 1;
    return seqIndex / m_cfg.sequence.size() + 1;
}

bool PositionTransitionController::advanceIndex()
{
    const int total = m_cfg.sequence.size() * m_cfg.repeats;
    if (m_seqIndex + 1 >= total) return false;
    ++m_seqIndex;
    m_currentRepeat = repeatForIndex(m_seqIndex);
    return true;
}

bool PositionTransitionController::hasNext() const
{
    const int total = m_cfg.sequence.size() * m_cfg.repeats;
    return m_seqIndex + 1 < total;
}

const PtPositionRecord* PositionTransitionController::record(int position, int repeat) const
{
    if (!m_store) return nullptr;
    for (const PtPositionRecord& r : st().ptRecords)
        if (r.position == position && r.repeat == repeat) return &r;
    return nullptr;
}

// ── lifecycle ────────────────────────────────────────────────────────────

bool PositionTransitionController::startPositionTransition(const QString& athlete)
{
    auto refuse = [this](const QString& userMsg, const char* code) {
        m_lastStartError = userMsg; m_lastError = userMsg;
        qWarning().noquote() << "POSTRANS start refused —" << code;
        emit startErrorChanged();
        return false;
    };
    if (athlete.trimmed().isEmpty())
        return refuse(QStringLiteral("No athlete has been selected.\n\nEnter or choose an athlete name, then start again."), "no-athlete");
    const QString err = m_cfg.validate();
    if (!err.isEmpty())
        return refuse(QStringLiteral("The Position Transition setup is not valid.\n\n%1").arg(err), "invalid-config");
    if (m_store->active())
        m_store->closeSession(CloseReason::Clean);

    SessionHeader header;
    header.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    header.appVersion = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("dev") : QCoreApplication::applicationVersion();
    header.athlete = athlete.trimmed();
    header.matchType = QStringLiteral("TRAINING position_transition");
    header.discipline = Discipline::ThreePositions50m;   // 3P only
    header.sessionKind = QStringLiteral("Training");
    if (m_operatingMode >= 0)
        header.operatingMode = ta::mode::modeToConfigString(static_cast<ta::mode::Mode>(m_operatingMode));
    header.config.officialShots = 0;

    const ReliabilityResult r = m_store->beginSession(header);
    if (!r.ok)
        return refuse(QStringLiteral("The training session journal could not be created.\n\n%1").arg(r.error.operatorMessage), "journal-open-failed");
    m_lastStartError.clear();
    emit startErrorChanged();

    PositionTransitionSessionStarted ev;
    ev.programId = QLatin1String(kProgramPositionTransition);
    ev.sequence = m_cfg.sequenceString();
    ev.verificationShots = static_cast<qint16>(m_cfg.verificationShots);
    ev.repeats = static_cast<qint16>(m_cfg.repeats);
    ev.checklistMode = static_cast<qint8>(m_cfg.checklistMode);
    ev.technicalFocus = m_cfg.technicalFocus;
    if (!submit(DomainEvent(ev))) return false;

    m_seqIndex = 0;
    m_currentRepeat = 1;
    m_lastExternalId = -1;
    m_setupStartMonoMs = m_store->nowMonotonicMs();   // initial setup timer
    // begin the first position's setup
    PositionSetupStarted ss;
    ss.position = static_cast<qint8>(currentPositionId());
    ss.repeat = static_cast<qint16>(m_currentRepeat);
    if (!submit(DomainEvent(ss))) return false;
    m_phase = 1;                                       // PositionSetup
    emit phaseChanged(); emit progressChanged();
    return true;
}

bool PositionTransitionController::positionReady()
{
    if (m_phase != 1) { m_lastError = QStringLiteral("Setup is not active."); return false; }
    const qint64 now = m_store->nowMonotonicMs();
    const qint32 dur = static_cast<qint32>(qMax<qint64>(0, qMin<qint64>(now - m_setupStartMonoMs, INT32_MAX)));
    PositionReady ev;
    ev.position = static_cast<qint8>(currentPositionId());
    ev.repeat = static_cast<qint16>(m_currentRepeat);
    ev.setupDurationMs = dur;
    if (!submit(DomainEvent(ev))) return false;
    m_readyMonoMs = now;
    m_phase = 2;                                       // Sighters
    emit phaseChanged(); emit progressChanged();
    return true;
}

bool PositionTransitionController::startVerification()
{
    if (m_phase != 2) { m_lastError = QStringLiteral("Sighters are not active."); return false; }
    PositionVerificationStarted ev;
    ev.position = static_cast<qint8>(currentPositionId());
    ev.repeat = static_cast<qint16>(m_currentRepeat);
    ev.readyMonoMs = static_cast<qint32>(qMin<qint64>(m_readyMonoMs, INT32_MAX));
    if (!submit(DomainEvent(ev))) return false;
    m_phase = 3;                                       // VerificationActive
    emit sightersCleared();
    emit phaseChanged(); emit progressChanged();
    return true;
}

bool PositionTransitionController::registerShot(double xMm, double yMm, double decimalScore,
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
        emit shotRejected(QStringLiteral("InvalidShotData")); return false;
    }
    if (externalId >= 0 && externalId <= m_lastExternalId) {
        emit shotRejected(QStringLiteral("DuplicateShot")); return false;
    }
    // Documented rule: shots fired during PositionSetup are IGNORED — not
    // journalled as any Training result.
    if (m_phase == 1) { emit shotRejected(QStringLiteral("SetupShotIgnored")); return false; }
    if (m_phase == 2) return acceptSighter(xMm, yMm, decimalScore, externalId, directionDeg, shotSource);
    if (m_phase == 3) return acceptVerifShot(xMm, yMm, decimalScore, externalId, directionDeg, shotSource);
    emit shotRejected(QStringLiteral("NotShootingPhase"));
    return false;
}

static ShotCore makeShot(SessionStore* store, double xMm, double yMm, double decimalScore,
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

bool PositionTransitionController::acceptSighter(double xMm, double yMm, double decimalScore,
                                                 qint64 externalId, double directionDeg, int shotSource)
{
    PositionSighterAccepted ev;
    ev.position = static_cast<qint8>(currentPositionId());
    ev.repeat = static_cast<qint16>(m_currentRepeat);
    ev.shot = makeShot(m_store.get(), xMm, yMm, decimalScore, externalId, directionDeg, shotSource, 0);
    if (!submit(DomainEvent(ev))) return false;
    if (externalId >= 0) m_lastExternalId = externalId;
    QVariantMap rec;
    rec[QStringLiteral("xMm")] = xMm; rec[QStringLiteral("yMm")] = yMm;
    rec[QStringLiteral("score")] = decimalScore; rec[QStringLiteral("sighter")] = true;
    emit sighterAccepted(rec);
    emit progressChanged();
    return true;
}

bool PositionTransitionController::acceptVerifShot(double xMm, double yMm, double decimalScore,
                                                   qint64 externalId, double directionDeg, int shotSource)
{
    const int within = shotsCompleted();
    if (within >= m_cfg.verificationShots) { emit shotRejected(QStringLiteral("BlockFull")); return false; }
    PositionVerificationShotAccepted ev;
    ev.position = static_cast<qint8>(currentPositionId());
    ev.repeat = static_cast<qint16>(m_currentRepeat);
    ev.withinBlock = static_cast<qint16>(within + 1);
    ev.shot = makeShot(m_store.get(), xMm, yMm, decimalScore, externalId, directionDeg, shotSource, within + 1);
    if (!submit(DomainEvent(ev))) return false;
    if (externalId >= 0) m_lastExternalId = externalId;
    QVariantMap rec;
    rec[QStringLiteral("xMm")] = xMm; rec[QStringLiteral("yMm")] = yMm;
    rec[QStringLiteral("withinBlock")] = ev.withinBlock;
    emit verificationShotAccepted(rec);
    // block complete?
    if (shotsCompleted() >= m_cfg.verificationShots) {
        PositionVerificationCompleted done;
        done.position = ev.position; done.repeat = ev.repeat;
        done.shotCount = static_cast<qint16>(m_cfg.verificationShots);
        if (submit(DomainEvent(done))) {
            m_phase = 4;                               // PositionReview
            emit positionCompleted(ev.position, ev.repeat);
            emit phaseChanged();
        }
    }
    emit progressChanged();
    return true;
}

bool PositionTransitionController::setChecklistItem(int itemIndex, int state)
{
    if (m_phase != 1 && m_phase != 2 && m_phase != 4) {
        m_lastError = QStringLiteral("Checklist is available during setup or review."); return false;
    }
    PositionChecklistUpdated ev;
    ev.position = static_cast<qint8>(currentPositionId());
    ev.repeat = static_cast<qint16>(m_currentRepeat);
    ev.itemIndex = static_cast<qint8>(itemIndex);
    ev.state = static_cast<qint8>(qBound(0, state, 3));
    return submit(DomainEvent(ev));
}

bool PositionTransitionController::saveNote(const QString& note)
{
    if (!m_store || !m_store->active()) return false;
    PositionNoteSaved ev;
    ev.position = static_cast<qint8>(currentPositionId());
    ev.repeat = static_cast<qint16>(m_currentRepeat);
    ev.note = note;
    return submit(DomainEvent(ev));
}

bool PositionTransitionController::continueToNext()
{
    if (m_phase != 4) { m_lastError = QStringLiteral("No position review to continue from."); return false; }
    const int fromPos = currentPositionId();
    if (advanceIndex()) {
        NextPositionTransitionStarted nt;
        nt.fromPosition = static_cast<qint8>(fromPos);
        nt.toPosition = static_cast<qint8>(currentPositionId());
        nt.toRepeat = static_cast<qint16>(m_currentRepeat);
        if (!submit(DomainEvent(nt))) return false;
        m_setupStartMonoMs = m_store->nowMonotonicMs();   // transition timer starts here
        PositionSetupStarted ss;
        ss.position = static_cast<qint8>(currentPositionId());
        ss.repeat = static_cast<qint16>(m_currentRepeat);
        if (!submit(DomainEvent(ss))) return false;
        m_phase = 1;                                       // next PositionSetup
        emit sightersCleared();
        emit phaseChanged(); emit progressChanged();
        return true;
    }
    // session complete
    int completed = 0;
    for (const PtPositionRecord& r : st().ptRecords) if (r.completed) ++completed;
    PositionTransitionCompleted ev;
    ev.completedPositions = static_cast<qint16>(completed);
    if (!submit(DomainEvent(ev))) return false;
    m_phase = 5;                                            // Complete
    emit completedChanged();
    emit phaseChanged(); emit progressChanged();
    return true;
}

bool PositionTransitionController::endEarly()
{
    if (m_phase == 0 || m_phase == 5) { m_lastError = QStringLiteral("Nothing to end."); return false; }
    int completed = 0;
    for (const PtPositionRecord& r : st().ptRecords) if (r.completed) ++completed;
    PositionTransitionCompleted ev;
    ev.completedPositions = static_cast<qint16>(completed);
    if (!submit(DomainEvent(ev))) return false;
    m_phase = 5;
    emit completedChanged();
    emit phaseChanged(); emit progressChanged();
    return true;
}

// ── close / recovery ───────────────────────────────────────────────────────

bool PositionTransitionController::closeCleanly()
{
    if (m_store && m_store->active()) {
        const ReliabilityResult rr = m_store->closeSession(CloseReason::Clean);
        if (!rr.ok) {
            m_lastError = rr.error.operatorMessage.isEmpty()
                ? QStringLiteral("The session could not be closed safely.") : rr.error.operatorMessage;
            emit journalWriteFailed(rr.error.affectedPath, rr.error.technicalDetail);
            return false;
        }
    }
    m_phase = 0; m_seqIndex = 0; m_currentRepeat = 1; m_lastExternalId = -1;
    emit phaseChanged(); emit progressChanged();
    return true;
}

void PositionTransitionController::resetPositionTransition() { closeCleanly(); }

bool PositionTransitionController::resumeFromRecovery(const QString& sessionId)
{
    if (!m_recovery) m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();
    RecoveredMatchState rec; ErrorInfo err;
    if (!m_recovery->buildRecoveredState(sessionId, &rec, &err)) {
        emit journalWriteFailed(rec.journalPath, err.technicalDetail); return false;
    }
    if (rec.state.sessionKind != QLatin1String("Training")
        || rec.state.ptProgramId != QLatin1String(kProgramPositionTransition)) {
        m_lastError = QStringLiteral("Not a Position Transition session."); return false;
    }
    const ReliabilityResult rr = m_store->resumeSession(rec);
    if (!rr.ok) { emit journalWriteFailed(rec.journalPath, rr.error.technicalDetail); return false; }

    const SessionState& s = st();
    m_cfg = PositionTransitionConfig::defaults();
    m_cfg.sequence = PositionTransitionConfig::parseSequence(s.ptSequence);
    if (m_cfg.sequence.isEmpty()) m_cfg.sequence = { 0, 1, 2 };
    m_cfg.verificationShots = s.ptVerificationShots;
    m_cfg.repeats = s.ptRepeats;
    m_cfg.checklistMode = s.ptChecklistMode;
    m_cfg.technicalFocus = s.ptFocus;
    m_currentRepeat = s.ptCurrentRepeat;
    // reconstruct m_seqIndex from current position + repeat
    m_seqIndex = 0;
    const int seqLen = m_cfg.sequence.size();
    for (int i = 0; i < seqLen * m_cfg.repeats; ++i)
        if (m_cfg.sequence[i % seqLen] == s.ptCurrentPosition && (i / seqLen + 1) == s.ptCurrentRepeat) { m_seqIndex = i; break; }
    // duplicate guard past every recovered externalId (verif + sighters)
    m_lastExternalId = -1;
    for (const PtPositionRecord& r : s.ptRecords) {
        for (const ShotCore& sc : r.verifShots) if (sc.externalId > m_lastExternalId) m_lastExternalId = sc.externalId;
        for (const ShotCore& sc : r.sighters) if (sc.externalId > m_lastExternalId) m_lastExternalId = sc.externalId;
    }
    // phase re-derivation from durable state
    const PtPositionRecord* cur = record(s.ptCurrentPosition, s.ptCurrentRepeat);
    if (s.ptCompleted) m_phase = 5;
    else if (cur && cur->completed) m_phase = 4;             // PositionReview
    else if (s.ptVerifying) m_phase = 3;                     // VerificationActive
    else if (s.ptInSetup) m_phase = 1;                       // PositionSetup
    else if (cur && cur->readyMonoMs != 0) m_phase = 2;      // Sighters (ready, not verifying)
    else m_phase = 1;
    m_setupStartMonoMs = m_store->nowMonotonicMs();
    m_readyMonoMs = cur ? cur->readyMonoMs : 0;
    emit configChanged(); emit phaseChanged(); emit progressChanged();
    if (m_phase == 5) emit completedChanged();
    qInfo() << "POSTRANS: resumed" << s.sessionId.left(8) << "phase" << m_phase
            << "pos" << s.ptCurrentPosition << "repeat" << s.ptCurrentRepeat;
    return true;
}

void PositionTransitionController::discardRecovery(const QString& sessionId)
{
    if (!m_recovery) m_recovery = std::make_unique<RecoveryCoordinator>();
    m_recovery->scan();
    m_recovery->archiveOrDiscard(sessionId, true);
}

QVariantList PositionTransitionController::recoveredSighterShots() const
{
    QVariantList out;
    if (!m_store || m_phase != 2) return out;
    const PtPositionRecord* r = record(currentPositionId(), m_currentRepeat);
    if (!r) return out;
    for (const ShotCore& s : r->sighters) {
        QVariantMap m;
        m[QStringLiteral("xMm")] = s.xHundredthMm / 100.0;
        m[QStringLiteral("yMm")] = s.yHundredthMm / 100.0;
        out.append(m);
    }
    return out;
}

// ── projections ────────────────────────────────────────────────────────────

int PositionTransitionController::shotsCompleted() const
{
    const PtPositionRecord* r = record(currentPositionId(), m_currentRepeat);
    return r ? r->verifShots.size() : 0;
}

int PositionTransitionController::sighterCount() const
{
    const PtPositionRecord* r = record(currentPositionId(), m_currentRepeat);
    return r ? r->sighters.size() : 0;
}

QString PositionTransitionController::positionName() const
{
    return positionLabel(static_cast<Position>(qBound(0, currentPositionId(), 2)));
}

QString PositionTransitionController::nextPositionName() const
{
    if (!hasNext()) return QString();
    const int seqLen = m_cfg.sequence.size();
    if (seqLen == 0) return QString();
    const int nextId = m_cfg.sequence[(m_seqIndex + 1) % seqLen];
    return positionLabel(static_cast<Position>(qBound(0, nextId, 2)));
}

QString PositionTransitionController::startVerifyLabel() const
{
    return QStringLiteral("Start %1 Verification").arg(positionName());
}

QString PositionTransitionController::sessionId() const { return m_store ? st().sessionId : QString(); }
QString PositionTransitionController::sessionOperatingMode() const { return m_store ? st().operatingMode : QString(); }

int PositionTransitionController::setupElapsedSec() const
{
    if (m_phase != 1 || !m_store) return 0;
    const qint64 now = m_store->nowMonotonicMs();
    return now > m_setupStartMonoMs ? static_cast<int>((now - m_setupStartMonoMs) / 1000) : 0;
}

int PositionTransitionController::readyElapsedSec() const
{
    if ((m_phase != 2 && m_phase != 3) || !m_store || m_readyMonoMs == 0) return 0;
    const qint64 now = m_store->nowMonotonicMs();
    return now > m_readyMonoMs ? static_cast<int>((now - m_readyMonoMs) / 1000) : 0;
}

QVariantList PositionTransitionController::checklistItems() const
{
    QVariantList out;
    const QStringList labels = positionChecklist(static_cast<Position>(qBound(0, currentPositionId(), 2)));
    const PtPositionRecord* r = record(currentPositionId(), m_currentRepeat);
    for (int i = 0; i < labels.size(); ++i) {
        QVariantMap m;
        m[QStringLiteral("index")] = i;
        m[QStringLiteral("label")] = labels[i];
        m[QStringLiteral("state")] = (r && i < r->checklist.size()) ? static_cast<int>(r->checklist[i]) : 0;
        out.append(m);
    }
    return out;
}

QVariantMap PositionTransitionController::positionReview(int position, int repeat) const
{
    QVariantMap m;
    const PtPositionRecord* r = record(position, repeat);
    if (!r) return m;
    m[QStringLiteral("position")] = position;
    m[QStringLiteral("positionName")] = positionLabel(static_cast<Position>(qBound(0, position, 2)));
    m[QStringLiteral("repeat")] = repeat;
    m[QStringLiteral("completed")] = r->completed;
    m[QStringLiteral("setupDurationMs")] = r->setupDurationMs;
    m[QStringLiteral("sighterCount")] = r->sighters.size();
    m[QStringLiteral("verificationShots")] = r->verifShots.size();
    // sighter duration (ready → last sighter) and verification duration
    qint64 lastSighter = r->readyMonoMs;
    for (const ShotCore& s : r->sighters) if (s.splitMs > lastSighter) lastSighter = s.splitMs;
    m[QStringLiteral("sighterDurationMs")] = static_cast<int>(qMax<qint64>(0, lastSighter - r->readyMonoMs));
    // group metrics (verification only)
    const BlockMetrics bm = computeBlockMetrics(r->verifShots);
    m[QStringLiteral("hasGroup")] = bm.hasGroup;
    m[QStringLiteral("averageScore")] = bm.averageScore;
    m[QStringLiteral("groupDiameter")] = bm.groupDiameter;
    m[QStringLiteral("groupRadius")] = bm.groupRadius;
    m[QStringLiteral("mpiX")] = bm.mpiX; m[QStringLiteral("mpiY")] = bm.mpiY;
    m[QStringLiteral("hSpread")] = bm.horizontalSpread; m[QStringLiteral("vSpread")] = bm.verticalSpread;
    m[QStringLiteral("scoreSd")] = bm.scoreStdDev;
    m[QStringLiteral("timingSd")] = bm.shotTimeStdDev;
    // first-shot metrics
    if (!r->verifShots.isEmpty()) {
        const ShotCore& fs = r->verifShots.first();
        const double fx = fs.xHundredthMm / 100.0, fy = fs.yHundredthMm / 100.0;
        m[QStringLiteral("firstShotScore")] = fs.scoreTenths / 10.0;
        m[QStringLiteral("firstShotXMm")] = fx; m[QStringLiteral("firstShotYMm")] = fy;
        const double dist = std::sqrt((fx - bm.mpiX) * (fx - bm.mpiX) + (fy - bm.mpiY) * (fy - bm.mpiY));
        m[QStringLiteral("firstShotDistMm")] = dist;
        m[QStringLiteral("firstShotInGroup")] = bm.hasGroup ? (dist <= bm.groupRadius * 1.5) : true;
        m[QStringLiteral("firstShotSeparated")] = bm.hasGroup ? (dist > bm.groupRadius * 2.0) : false;
        if (r->readyMonoMs != 0)
            m[QStringLiteral("readyToFirstShotMs")] = static_cast<int>(qMax<qint64>(0, fs.splitMs - r->readyMonoMs));
        // verification block duration (ready → last shot)
        qint64 lastShot = r->readyMonoMs;
        for (const ShotCore& s : r->verifShots) if (s.splitMs > lastShot) lastShot = s.splitMs;
        m[QStringLiteral("verificationDurationMs")] = static_cast<int>(qMax<qint64>(0, lastShot - r->readyMonoMs));
    }
    // group pattern insights (>= 5 shots)
    QVector<double> xs, ys;
    for (const ShotCore& s : r->verifShots) { xs.append(s.xHundredthMm / 100.0); ys.append(s.yHundredthMm / 100.0); }
    const GroupPatternResult gp = analyzeGroup(xs, ys, issfRingSpacingMm(Discipline::ThreePositions50m));
    QVariantMap gpm; gpm[QStringLiteral("hasData")] = gp.hasData;
    gpm[QStringLiteral("primaryLabel")] = gp.primaryLabel();
    QVariantList props;
    for (const GroupProperty& p : gp.properties) {
        QVariantMap pm; pm[QStringLiteral("label")] = p.label; pm[QStringLiteral("evidence")] = p.evidence;
        pm[QStringLiteral("confidence")] = confidenceLabel(p.confidence); props.append(pm);
    }
    gpm[QStringLiteral("properties")] = props;
    m[QStringLiteral("groupPattern")] = gpm;
    // checklist summary
    int checked = 0, skipped = 0;
    for (qint8 c : r->checklist) { if (c == 1) ++checked; else if (c == 2) ++skipped; }
    m[QStringLiteral("checklistChecked")] = checked;
    m[QStringLiteral("checklistSkipped")] = skipped;
    m[QStringLiteral("note")] = r->note;
    m[QStringLiteral("disclaimer")] =
        QStringLiteral("Measured setup and early-group data. It does not identify the technical cause.");
    return m;
}

QVariantMap PositionTransitionController::currentReview() const
{
    return positionReview(currentPositionId(), m_currentRepeat);
}

QVariantList PositionTransitionController::positionComparison() const
{
    QVariantList out;
    if (!m_store) return out;
    for (const PtPositionRecord& r : st().ptRecords)
        if (r.completed) out.append(positionReview(r.position, r.repeat));
    return out;
}

QVariantList PositionTransitionController::repeatComparison() const
{
    QVariantList out;
    if (!m_store || m_cfg.repeats < 2) return out;
    // for each position appearing in the sequence, list its repeats' key metrics
    QVector<int> seen;
    for (int p : m_cfg.sequence) {
        if (seen.contains(p)) continue; seen.append(p);
        QVariantList repeats;
        for (int rp = 1; rp <= m_cfg.repeats; ++rp) {
            const PtPositionRecord* r = record(p, rp);
            if (!r || !r->completed) continue;
            const BlockMetrics bm = computeBlockMetrics(r->verifShots);
            QVariantMap rm;
            rm[QStringLiteral("repeat")] = rp;
            rm[QStringLiteral("setupDurationMs")] = r->setupDurationMs;
            rm[QStringLiteral("sighterCount")] = r->sighters.size();
            rm[QStringLiteral("groupDiameter")] = bm.groupDiameter;
            rm[QStringLiteral("averageScore")] = bm.averageScore;
            if (!r->verifShots.isEmpty() && r->readyMonoMs != 0)
                rm[QStringLiteral("readyToFirstShotMs")] = static_cast<int>(qMax<qint64>(0, r->verifShots.first().splitMs - r->readyMonoMs));
            repeats.append(rm);
        }
        if (repeats.size() >= 2) {
            QVariantMap pm;
            pm[QStringLiteral("position")] = p;
            pm[QStringLiteral("positionName")] = positionLabel(static_cast<Position>(qBound(0, p, 2)));
            pm[QStringLiteral("repeats")] = repeats;
            out.append(pm);
        }
    }
    return out;
}

QStringList PositionTransitionController::sessionObservations() const
{
    QStringList out;
    const QVariantList comp = positionComparison();
    if (comp.isEmpty()) return out;
    // longest setup + fastest ready-to-first-shot (measured, position-by-position)
    QString longestPos; int longestMs = -1;
    QString fastestPos; int fastestMs = INT_MAX;
    int mostSighters = -1; QString mostSighterPos;
    for (const QVariant& v : comp) {
        const QVariantMap r = v.toMap();
        const int setup = r.value(QStringLiteral("setupDurationMs")).toInt();
        if (setup > longestMs) { longestMs = setup; longestPos = r.value(QStringLiteral("positionName")).toString(); }
        if (r.contains(QStringLiteral("readyToFirstShotMs"))) {
            const int f = r.value(QStringLiteral("readyToFirstShotMs")).toInt();
            if (f < fastestMs) { fastestMs = f; fastestPos = r.value(QStringLiteral("positionName")).toString(); }
        }
        const int sc = r.value(QStringLiteral("sighterCount")).toInt();
        if (sc > mostSighters) { mostSighters = sc; mostSighterPos = r.value(QStringLiteral("positionName")).toString(); }
    }
    if (!longestPos.isEmpty())
        out << QStringLiteral("%1 took the longest to set up (%2 s).").arg(longestPos).arg(longestMs / 1000);
    if (!fastestPos.isEmpty() && fastestMs != INT_MAX)
        out << QStringLiteral("%1 had the fastest first shot after Position Ready (%2 s).").arg(fastestPos).arg(fastestMs / 1000);
    if (!mostSighterPos.isEmpty() && mostSighters > 0)
        out << QStringLiteral("%1 required the most sighters (%2).").arg(mostSighterPos).arg(mostSighters);
    out << QStringLiteral("Positions have different stability demands — compare each position to itself across repeats, not against another position.");
    return out;
}

QVariantMap PositionTransitionController::sessionInsights() const
{
    QVariantMap ins;
    ins[QStringLiteral("observations")] = sessionObservations();
    ins[QStringLiteral("repeatComparison")] = repeatComparison();
    // items skipped most often across positions
    QStringList reviewItems;
    for (const QVariant& v : positionComparison()) {
        const QVariantMap r = v.toMap();
        if (r.value(QStringLiteral("firstShotSeparated")).toBool())
            reviewItems << QStringLiteral("%1: the first shot was separated from the verification group.")
                              .arg(r.value(QStringLiteral("positionName")).toString());
    }
    ins[QStringLiteral("reviewItems")] = reviewItems;
    return ins;
}

int PositionTransitionController::sessionDurationSec() const
{
    if (!m_store) return 0;
    qint64 lo = -1, hi = -1;
    for (const PtPositionRecord& r : st().ptRecords)
        for (const ShotCore& s : r.verifShots) {
            if (lo < 0 || s.splitMs < lo) lo = s.splitMs;
            if (hi < 0 || s.splitMs > hi) hi = s.splitMs;
        }
    if (lo < 0 || hi <= lo) return 0;
    return static_cast<int>((hi - lo) / 1000);
}

QVariantList PositionTransitionController::verificationPlot(int position, int repeat) const
{
    QVariantList out;
    const PtPositionRecord* r = record(position, repeat);
    if (!r) return out;
    int i = 0;
    for (const ShotCore& s : r->verifShots) {
        QVariantMap p;
        p[QStringLiteral("xMm")] = s.xHundredthMm / 100.0;
        p[QStringLiteral("yMm")] = s.yHundredthMm / 100.0;
        p[QStringLiteral("score")] = s.scoreTenths / 10.0;
        p[QStringLiteral("first")] = (i == 0);
        out.append(p); ++i;
    }
    return out;
}

QVariantMap PositionTransitionController::reportModel() const
{
    QVariantMap r;
    if (!m_store) return r;
    const SessionState& s = st();
    r[QStringLiteral("programme")] = QStringLiteral("Position Transition");
    r[QStringLiteral("sessionId")] = s.sessionId;
    r[QStringLiteral("athlete")] = s.athlete;
    r[QStringLiteral("createdAtIso")] = s.createdAtIso;
    r[QStringLiteral("operatingMode")] = s.operatingMode.isEmpty() ? QStringLiteral("Legacy") : s.operatingMode;
    r[QStringLiteral("focus")] = m_cfg.technicalFocus;
    r[QStringLiteral("sequence")] = m_cfg.sequenceArrowText();
    r[QStringLiteral("verificationShots")] = m_cfg.verificationShots;
    r[QStringLiteral("repeats")] = m_cfg.repeats;
    r[QStringLiteral("checklistMode")] = m_cfg.checklistMode;
    r[QStringLiteral("durationSec")] = sessionDurationSec();
    int totalSighters = 0, totalCounted = 0, positionsDone = 0;
    for (const PtPositionRecord& rec : s.ptRecords) {
        totalSighters += rec.sighters.size();
        totalCounted += rec.verifShots.size();
        if (rec.completed) ++positionsDone;
    }
    r[QStringLiteral("totalSighters")] = totalSighters;
    r[QStringLiteral("countedShots")] = totalCounted;
    r[QStringLiteral("positionsCompleted")] = positionsDone;
    r[QStringLiteral("completed")] = s.ptCompleted;
    r[QStringLiteral("sessionNote")] = s.ptSessionNote;
    // per-position pages
    QVariantList positions;
    for (const PtPositionRecord& rec : s.ptRecords) {
        if (!rec.completed) continue;
        QVariantMap pm = positionReview(rec.position, rec.repeat);
        pm[QStringLiteral("plot")] = verificationPlot(rec.position, rec.repeat);
        const QStringList labels = positionChecklist(static_cast<Position>(qBound(0, int(rec.position), 2)));
        QVariantList checklist;
        for (int i = 0; i < labels.size(); ++i) {
            QVariantMap cm; cm[QStringLiteral("label")] = labels[i];
            cm[QStringLiteral("state")] = (i < rec.checklist.size()) ? int(rec.checklist[i]) : 0;
            checklist.append(cm);
        }
        pm[QStringLiteral("checklist")] = checklist;
        positions.append(pm);
    }
    r[QStringLiteral("positions")] = positions;
    r[QStringLiteral("comparison")] = positionComparison();
    r[QStringLiteral("insights")] = sessionInsights();
    r[QStringLiteral("observations")] = sessionObservations();
    return r;
}

bool PositionTransitionController::submit(const DomainEvent& ev)
{
    const SubmitResult r = m_store->submit(ev);
    if (!r.ok) {
        m_lastError = r.error.operatorMessage;
        qWarning() << "POSTRANS: submit refused —" << r.error.technicalDetail;
        return false;
    }
    return true;
}
