#include "RecoveryCoordinator.h"

#include "reliability/journal/JournalValidator.h"
#include "reliability/events/EventSerializer.h"
#include "reliability/replay/ReplayEngine.h"
#include "reliability/storage/StoragePaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace ta {
namespace rel {

namespace {

QString disciplineLabel(Discipline d, const QString& matchType)
{
    switch (d) {
    case Discipline::Finals3P:          return QStringLiteral("50m Rifle 3 Positions Final");
    case Discipline::ThreePositions50m: return QStringLiteral("50m Rifle 3 Positions");
    case Discipline::Prone50m:          return QStringLiteral("50m Rifle Prone");
    case Discipline::AirRifle10m:       return QStringLiteral("10m Air Rifle");
    case Discipline::AirPistol10m:      return QStringLiteral("10m Air Pistol");
    case Discipline::AirRifleFinal10m:  return QStringLiteral("10m Air Rifle Final");
    case Discipline::AirPistolFinal10m: return QStringLiteral("10m Air Pistol Final");
    case Discipline::Training:          return QStringLiteral("Training");
    case Discipline::None:              break;
    }
    return matchType.isEmpty() ? QStringLiteral("Session") : matchType;
}

// Map the validator's structural classification + prefix length to the
// operator-facing recovery verdict (spec Task 3).
RecoveryClass mapClass(const ValidationReport& rep)
{
    switch (rep.classification) {
    case JournalClassification::Clean:
        return RecoveryClass::Clean;
    case JournalClassification::TailTruncated:
        // A valid prefix with a torn tail — resume up to the last valid event.
        return rep.validPrefixCount >= 1 ? RecoveryClass::Recoverable
                                         : RecoveryClass::Fatal;
    case JournalClassification::CorruptInternal:
        // Interior damage: recover the valid prefix if it at least has a
        // usable header; otherwise nothing can be trusted.
        return rep.validPrefixCount >= 1 ? RecoveryClass::Corrupt
                                         : RecoveryClass::Fatal;
    case JournalClassification::UnsupportedVersion:
    case JournalClassification::SessionMismatch:
    case JournalClassification::Empty:
        return RecoveryClass::Fatal;
    }
    return RecoveryClass::Fatal;
}

} // namespace

RecoveryCoordinator::RecoveryCoordinator(QObject* parent)
    : QObject(parent)
{
}

QString RecoveryCoordinator::scanDirectory() const
{
    return m_scanDirOverride.isEmpty()
        ? StoragePaths::currentSessionsDirectory()
        : m_scanDirOverride;
}

RecoveryCandidate RecoveryCoordinator::classifyFile(const QString& path)
{
    RecoveryCandidate c;
    c.journalPath = path;
    const QFileInfo info(path);
    c.lastModifiedIso = info.lastModified().toString(Qt::ISODateWithMs);

    const ValidationReport rep = JournalValidator::validateFile(path);
    c.recoveryClass = mapClass(rep);
    c.validationDetail = rep.error.operatorMessage.isEmpty()
        ? QString::fromLatin1(journalClassificationName(rep.classification))
        : rep.error.operatorMessage;
    c.sessionId = rep.sessionId;
    c.lastValidSeq = rep.lastValidSeq;
    c.complete = rep.sawMatchCompleted;

    // Derive the human-facing fields from the replayed reducer state (never
    // from anything but the reducer).
    const ReplayResult replay = ReplayEngine::replay(rep.validEnvelopes);
    // T1: Training sessions complete with TrainingCompleted, not MatchCompleted,
    // so the quick sawMatchCompleted flag misses them. A completed Training
    // session (or any session the reducer folded to Complete) must be treated
    // as complete — auto-archived below, never offered as an unfinished resume.
    if (replay.ok && (replay.state.trainingCompleted
                      || replay.state.lifecycle == Lifecycle::Complete))
        c.complete = true;
    if (replay.ok) {
        const SessionState& s = replay.state;
        c.athlete = s.athlete;
        c.discipline = s.discipline;
        c.matchType = s.matchType;
        c.operatingMode = s.operatingMode;   // F10: recorded mode (empty = Unknown/Legacy)
        c.sessionKind = s.sessionKind;       // T1: Training vs competition
        // T2/T4: Call & Diagnose and Position Transition are also
        // sessionKind=Training but record their programme id in cdProgramId /
        // ptProgramId — surface whichever is set so the recovery dispatcher
        // routes to the right controller.
        c.trainingProgramId = !s.trainingProgramId.isEmpty() ? s.trainingProgramId
                            : (!s.cdProgramId.isEmpty() ? s.cdProgramId : s.ptProgramId);
        c.trainingBlock = s.trainingCurrentBlock;
        c.disciplineLabel = disciplineLabel(s.discipline, s.matchType);
        c.startedAtIso = s.createdAtIso;
        c.currentStageId = s.currentStageId;
        c.officialShots = s.officials.size();
        c.expectedShots = s.config.officialShots;
        c.totalTenths = s.totalTenths;
        // Generic dialog metadata (Phase D): the reducer phase and — when the
        // session anchored a phase clock via TimerStarted — the FROZEN
        // remaining time at the last valid event. Discipline-agnostic: pure
        // reducer/envelope values; -1 = no active clock (e.g. finals, which
        // anchors on command windows instead).
        c.phaseId = static_cast<qint8>(s.phase);
        if (s.timer.active && s.timer.durationMs > 0
                && !rep.validEnvelopes.isEmpty()) {
            // Pause-aware frozen remaining plus authorised credits (Phase E):
            // pure reducer-state math — an incident freezes the clock via
            // TimerPaused, and Jury credit is separate generic incident data.
            const qint64 lastMono = rep.validEnvelopes.last().monotonicMs;
            const qint64 stopAt = s.timer.paused ? s.timer.pausedAtMonoMs
                                                 : lastMono;
            qint64 elapsedRunning =
                stopAt - s.timer.startedAtMonoMs - s.timer.pausedAccumMs;
            if (elapsedRunning < 0)
                elapsedRunning = 0;
            qint64 credit = 0;
            for (const EstIncidentRecord& r : s.estIncidents)
                credit += r.timeCreditMs;
            const qint64 remaining =
                s.timer.durationMs - elapsedRunning + credit;
            c.remainingMs = remaining > 0 ? remaining : 0;
        }
        // Unresolved-incident projection for the recovery dialog (Phase E):
        // generic reducer values only.
        for (const EstIncidentRecord& r : s.estIncidents) {
            if (r.status == static_cast<quint8>(IncidentStatus::Open)
                    && !r.officialResumeAuthorised) {
                c.openIncident = true;
                c.incidentTypeId = static_cast<qint8>(r.incidentType);
                c.incidentScopeId = static_cast<qint8>(r.scope);
                c.incidentCreditDecision = static_cast<qint8>(r.creditDecision);
                c.incidentCreditMs = r.timeCreditMs;
                break;
            }
        }
    }
    if (!rep.validEnvelopes.isEmpty())
        c.lastEventWallIso = rep.validEnvelopes.last().wallTimestampIso;

    c.resumable = c.recoveryClass == RecoveryClass::Clean
               || c.recoveryClass == RecoveryClass::Recoverable;
    return c;
}

QVector<RecoveryCandidate> RecoveryCoordinator::scan()
{
    m_candidates.clear();
    QDir dir(scanDirectory());
    if (!dir.exists())
        return m_candidates;

    const QStringList files = dir.entryList(
        QStringList() << QStringLiteral("session_*.jsonl"),
        QDir::Files, QDir::Time);   // newest first
    for (const QString& name : files) {
        const QString path = dir.absoluteFilePath(name);
        RecoveryCandidate c = classifyFile(path);

        // A cleanly-closed session that somehow remained in Current is
        // auto-archived and never offered for resume (spec §15).
        if (c.complete) {
            archiveOrDiscard(c.sessionId.isEmpty() ? path : c.sessionId, false);
            continue;
        }
        // A journal with no usable content at all is not a candidate.
        if (c.recoveryClass == RecoveryClass::Fatal && c.lastValidSeq == 0
                && c.athlete.isEmpty())
            continue;
        m_candidates.append(c);
    }

    if (!m_candidates.isEmpty())
        emit recoveryChoicesReady(m_candidates.size());
    return m_candidates;
}

QVariantList RecoveryCoordinator::scanForQml()
{
    QVariantList out;
    for (const RecoveryCandidate& c : scan()) {
        QVariantMap m;
        m[QStringLiteral("sessionId")] = c.sessionId;
        m[QStringLiteral("journalPath")] = c.journalPath;
        m[QStringLiteral("athlete")] = c.athlete;
        // Stable machine id (AR10 / AP10 / PRONE50 / 3P50 / FINAL3P / ...) for
        // the QML recovery dispatcher to select the discipline restorer. The
        // human label stays separate for display.
        m[QStringLiteral("disciplineId")] =
            QString::fromLatin1(disciplineId(c.discipline));
        m[QStringLiteral("disciplineLabel")] = c.disciplineLabel;
        m[QStringLiteral("matchType")] = c.matchType;
        m[QStringLiteral("operatingMode")] = c.operatingMode;   // F10 (empty = Unknown/Legacy)
        m[QStringLiteral("sessionKind")] = c.sessionKind;       // T1: "Training" or ""
        m[QStringLiteral("trainingProgramId")] = c.trainingProgramId;
        m[QStringLiteral("trainingBlock")] = c.trainingBlock;
        m[QStringLiteral("startedAtIso")] = c.startedAtIso;
        m[QStringLiteral("lastEventWallIso")] = c.lastEventWallIso;
        m[QStringLiteral("lastModifiedIso")] = c.lastModifiedIso;
        m[QStringLiteral("currentStageId")] = c.currentStageId;
        m[QStringLiteral("officialShots")] = c.officialShots;
        m[QStringLiteral("expectedShots")] = c.expectedShots;
        m[QStringLiteral("totalTenths")] = c.totalTenths;
        m[QStringLiteral("phaseId")] = static_cast<int>(c.phaseId);
        m[QStringLiteral("remainingMs")] = static_cast<qlonglong>(c.remainingMs);
        m[QStringLiteral("openIncident")] = c.openIncident;
        m[QStringLiteral("incidentTypeId")] = static_cast<int>(c.incidentTypeId);
        m[QStringLiteral("incidentScopeId")] = static_cast<int>(c.incidentScopeId);
        m[QStringLiteral("incidentCreditDecision")] =
            static_cast<int>(c.incidentCreditDecision);
        m[QStringLiteral("incidentCreditMs")] =
            static_cast<qlonglong>(c.incidentCreditMs);
        m[QStringLiteral("recoveryClass")] =
            QString::fromLatin1(recoveryClassName(c.recoveryClass));
        m[QStringLiteral("validationDetail")] = c.validationDetail;
        m[QStringLiteral("resumable")] = c.resumable;
        out.append(m);
    }
    return out;
}

RecoveryCandidate* RecoveryCoordinator::find(const QString& sessionId)
{
    for (RecoveryCandidate& c : m_candidates)
        if (c.sessionId == sessionId || c.journalPath == sessionId)
            return &c;
    return nullptr;
}

bool RecoveryCoordinator::buildRecoveredState(const QString& sessionId,
                                              RecoveredMatchState* out,
                                              ErrorInfo* err)
{
    RecoveryCandidate* c = find(sessionId);
    if (!c) {
        if (err)
            *err = ReliabilityResult::failure(ReliabilityError::InvalidArgument,
                QStringLiteral("Session not found for recovery."),
                QStringLiteral("no scanned candidate '%1'").arg(sessionId)).error;
        return false;
    }
    if (!c->resumable) {
        if (err)
            *err = ReliabilityResult::failure(ReliabilityError::CorruptJournal,
                QStringLiteral("This session cannot be resumed."),
                QStringLiteral("recovery class %1")
                    .arg(QLatin1String(recoveryClassName(c->recoveryClass)))).error;
        return false;
    }

    const ValidationReport rep = JournalValidator::validateFile(c->journalPath);
    const ReplayResult replay = ReplayEngine::replay(rep.validEnvelopes);
    if (!replay.ok) {
        if (err)
            *err = replay.error;
        return false;
    }
    out->state = replay.state;
    out->sessionId = c->sessionId;
    out->journalPath = c->journalPath;
    out->lastValidSeq = rep.lastValidSeq;
    out->recoveryClass = c->recoveryClass;
    if (!rep.validEnvelopes.isEmpty()) {
        out->lastEventWallIso = rep.validEnvelopes.last().wallTimestampIso;
        out->lastLineHash = rep.validEnvelopes.last().currentHash;
        // Timing anchors for the controller clock rebase (§16). The last
        // event's monotonic time, and the start of the current firing window
        // (the most recent WindowOpened) — elapsed = last − stageStart.
        out->lastEventMonoMs = rep.validEnvelopes.last().monotonicMs;
        for (int i = rep.validEnvelopes.size() - 1; i >= 0; --i) {
            if (std::holds_alternative<WindowOpened>(rep.validEnvelopes[i].payload)) {
                out->stageClockStartMonoMs = rep.validEnvelopes[i].monotonicMs;
                break;
            }
        }
        // Byte length of the valid prefix. The serializer is deterministic,
        // so re-serializing the validated envelopes reproduces the exact
        // on-disk bytes — the offset at which the file is safely truncated
        // to drop any torn tail before resuming appends (spec §15).
        qint64 bytes = 0;
        bool ok = true;
        for (const EventEnvelope& env : rep.validEnvelopes) {
            QByteArray line;
            if (!EventSerializer::serializeCompleteEnvelope(env, &line).ok) {
                ok = false;
                break;
            }
            bytes += line.size() + 1;   // + LF
        }
        out->validByteLength = ok ? bytes : -1;
    }
    return true;
}

StorageResult RecoveryCoordinator::archiveOrDiscard(const QString& sessionId,
                                                    bool discarded)
{
    // Resolve the path from the id/path.
    QString path = sessionId;
    if (!QFile::exists(path)) {
        RecoveryCandidate* c = find(sessionId);
        if (c)
            path = c->journalPath;
    }
    if (!QFile::exists(path))
        return StorageResult::success(QStringLiteral("no file to archive"));

    const QFileInfo info(path);
    QString dest;
    if (discarded) {
        // Preserve the byte-exact original under Sessions/Corrupt.
        QDir().mkpath(StoragePaths::corruptedSessionsDirectory());
        dest = StoragePaths::corruptedSessionsDirectory()
             + QLatin1Char('/') + info.fileName();
    } else {
        const QString iso = info.lastModified().toString(Qt::ISODate);
        const QString year = iso.left(4);
        const QString month = iso.mid(5, 2);
        dest = StoragePaths::archivedSessionMonthPath(year, month, info.fileName());
    }
    if (dest.isEmpty())
        return StorageResult::failure(StorageError::DirectoryCreateFailed,
            QStringLiteral("The session could not be moved."),
            QStringLiteral("archive/corrupt directory unavailable"), path);
    // Never overwrite: numbered suffix on collision.
    for (int n = 1; QFileInfo::exists(dest); ++n) {
        const QString base = info.completeBaseName();
        dest = QFileInfo(dest).absolutePath() + QStringLiteral("/%1_%2.jsonl")
                   .arg(base).arg(n);
    }
    if (QFile::rename(path, dest))
        return StorageResult::success(
            QStringLiteral("moved %1 -> %2").arg(path, dest));
    if (QFile::copy(path, dest) && QFileInfo(dest).size() == info.size()) {
        QFile::remove(path);
        return StorageResult::success(
            QStringLiteral("copied %1 -> %2").arg(path, dest));
    }
    QFile::remove(dest);
    return StorageResult::failure(StorageError::MigrationFailed,
        QStringLiteral("The session could not be moved."),
        QStringLiteral("rename and copy both failed"), path);
}

} // namespace rel
} // namespace ta
