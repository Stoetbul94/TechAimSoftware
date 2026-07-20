#include "EstIncidentController.h"

#include <QDateTime>
#include <QUuid>

using namespace ta::rel;

EstIncidentController::EstIncidentController(QObject* parent)
    : QObject(parent)
{
}

void EstIncidentController::setStoreProvider(
    std::function<ta::rel::SessionStore*()> provider)
{
    m_provider = std::move(provider);
}

const EstIncidentRecord* EstIncidentController::openRecord() const
{
    SessionStore* s = store();
    if (!s || !s->active())
        return nullptr;
    // The workflow drives ONE incident at a time: the most recent open record.
    const SessionState& st = s->state();
    for (int i = st.estIncidents.size() - 1; i >= 0; --i) {
        const EstIncidentRecord& r = st.estIncidents.at(i);
        if (r.status == static_cast<quint8>(IncidentStatus::Open))
            return &st.estIncidents.at(i);
    }
    return nullptr;
}

bool EstIncidentController::submit(const DomainEvent& event)
{
    SessionStore* s = store();
    if (!s || !s->active()) {
        emit actionFailed(QStringLiteral("No active session."));
        return false;
    }
    const SubmitResult r = s->submit(event);
    if (!r.ok) {
        emit actionFailed(r.error.technicalDetail);
        return false;
    }
    emit incidentChanged();
    return true;
}

// ── authorised workflow actions ─────────────────────────────────────────────

bool EstIncidentController::raiseIncident(int typeId, int scopeId,
                                          const QString& firingPoints,
                                          const QString& relayId,
                                          const QString& reason)
{
    SessionStore* s = store();
    if (!s || !s->active()) {
        emit actionFailed(QStringLiteral("No active session."));
        return false;
    }
    if (openRecord()) {
        emit actionFailed(QStringLiteral("An incident is already open."));
        return false;
    }
    EstIncidentRaised ev;
    ev.incidentId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ev.incidentType = static_cast<IncidentType>(qBound(0, typeId, 9));
    ev.scope = static_cast<IncidentScope>(qBound(0, scopeId, 2));
    ev.firingPoint = firingPoints;
    ev.relayId = relayId;
    // Persisted UTC wall-clock (survives reboot) — the outage interval never
    // relies on process monotonic time alone (est-malfunctions.md §5.2).
    ev.interruptionStartUtc = QDateTime::currentDateTimeUtc()
        .toString(Qt::ISODateWithMs);
    ev.reason = reason;
    if (!submit(DomainEvent(ev)))
        return false;
    // Freeze the competition clock (existing generic timer event; the reducer
    // accumulates the pause). Application downtime / incident time never
    // consumes competition time.
    const TimerState& t = s->state().timer;
    if (t.active && !t.paused)
        submit(DomainEvent(TimerPaused{t.timerId, s->nowMonotonicMs()}));
    return true;
}

bool EstIncidentController::recordAcceptedDuration(qint64 durationMs,
                                                   const QString& authorisedBy,
                                                   const QString& reason)
{
    const EstIncidentRecord* r = openRecord();
    if (!r) { emit actionFailed(QStringLiteral("No open incident.")); return false; }
    EstDecisionRecorded ev;
    ev.incidentId = r->incidentId;
    ev.decision = EstDecisionKind::DurationAccepted;
    ev.acceptedDurationMs = durationMs;
    ev.authorisedBy = authorisedBy;
    ev.reason = reason;
    return submit(DomainEvent(ev));
}

bool EstIncidentController::recordNoAllowance(const QString& authorisedBy,
                                              const QString& reason)
{
    const EstIncidentRecord* r = openRecord();
    if (!r) { emit actionFailed(QStringLiteral("No open incident.")); return false; }
    EstDecisionRecorded ev;
    ev.incidentId = r->incidentId;
    ev.decision = EstDecisionKind::NoAllowance;
    ev.authorisedBy = authorisedBy;
    ev.reason = reason;
    return submit(DomainEvent(ev));   // reducer rejects a duplicate/contradiction
}

bool EstIncidentController::grantTimeCredit(qint64 durationMs,
                                            const QString& authorisedBy,
                                            const QString& reason)
{
    const EstIncidentRecord* r = openRecord();
    if (!r) { emit actionFailed(QStringLiteral("No open incident.")); return false; }
    if (durationMs <= 0) {
        emit actionFailed(QStringLiteral("Credit must be positive — use the "
                                         "no-allowance decision instead."));
        return false;
    }
    // Workflow-level double-application guard (one authorised credit per
    // incident); replay applies the single journalled event exactly once.
    if (r->creditDecision != 0) {
        emit actionFailed(QStringLiteral("An allowance decision was already "
                                         "recorded for this incident."));
        return false;
    }
    TimeCreditGranted ev;
    ev.incidentId = r->incidentId;
    ev.durationMs = durationMs;
    ev.authorisedBy = authorisedBy;
    ev.reason = reason;
    return submit(DomainEvent(ev));
}

bool EstIncidentController::beginRecoveryPreparation(const QString& authorisedBy)
{
    const EstIncidentRecord* r = openRecord();
    if (!r) { emit actionFailed(QStringLiteral("No open incident.")); return false; }
    RecoveryPhaseEntered ev;
    ev.incidentId = r->incidentId;
    ev.phase = RecoveryPhaseKind::Preparation;
    ev.durationMs = 300000;   // the authorised 5-minute allowance (6.11.2)
    ev.authorisedBy = authorisedBy;
    return submit(DomainEvent(ev));
}

bool EstIncidentController::beginRecoverySighting(const QString& authorisedBy)
{
    const EstIncidentRecord* r = openRecord();
    if (!r) { emit actionFailed(QStringLiteral("No open incident.")); return false; }
    RecoveryPhaseEntered ev;
    ev.incidentId = r->incidentId;
    ev.phase = RecoveryPhaseKind::Sighting;
    ev.durationMs = 0;        // unlimited sighting period
    ev.authorisedBy = authorisedBy;
    return submit(DomainEvent(ev));
}

bool EstIncidentController::authoriseOfficialResume(const QString& authorisedBy)
{
    SessionStore* s = store();
    const EstIncidentRecord* r = openRecord();
    if (!s || !r) { emit actionFailed(QStringLiteral("No open incident.")); return false; }
    RecoveryPhaseEntered ev;
    ev.incidentId = r->incidentId;
    ev.phase = RecoveryPhaseKind::OfficialResume;
    ev.durationMs = 0;
    ev.authorisedBy = authorisedBy;
    if (!submit(DomainEvent(ev)))
        return false;
    // Unfreeze the competition clock: the remaining time from here is the ONE
    // authoritative computation (frozen remaining + authorised credit).
    const TimerState& t = s->state().timer;
    if (t.active && t.paused)
        submit(DomainEvent(TimerResumed{t.timerId, s->nowMonotonicMs()}));
    return true;
}

bool EstIncidentController::reassignTarget(const QString& originalTarget,
                                           const QString& reserveTarget,
                                           const QString& authorisedBy,
                                           const QString& reason)
{
    const EstIncidentRecord* r = openRecord();
    if (!r) { emit actionFailed(QStringLiteral("No open incident.")); return false; }
    TargetReassigned ev;
    ev.incidentId = r->incidentId;
    ev.originalTarget = originalTarget;
    ev.reserveTarget = reserveTarget;
    ev.authorisedBy = authorisedBy;
    ev.reason = reason;
    return submit(DomainEvent(ev));
}

bool EstIncidentController::recordBackupReview(int outcome,
                                               const QString& authorisedBy,
                                               const QString& reason)
{
    const EstIncidentRecord* r = openRecord();
    if (!r) { emit actionFailed(QStringLiteral("No open incident.")); return false; }
    EstDecisionRecorded ev;
    ev.incidentId = r->incidentId;
    ev.decision = outcome == 1 ? EstDecisionKind::BackupAccepted
                : outcome == 2 ? EstDecisionKind::BackupRejected
                               : EstDecisionKind::BackupInconclusive;
    ev.authorisedBy = authorisedBy;
    ev.reason = reason;
    return submit(DomainEvent(ev));
}

bool EstIncidentController::resolveIncident(const QString& juryNote,
                                            const QString& rangeOfficerNote,
                                            const QString& reportRef)
{
    SessionStore* s = store();
    const EstIncidentRecord* r = openRecord();
    if (!s || !r) { emit actionFailed(QStringLiteral("No open incident.")); return false; }
    EstIncidentResolved ev;
    ev.incidentId = r->incidentId;
    ev.status = IncidentStatus::Resolved;
    // Calculated (estimated) duration from the persisted UTC start to now —
    // the OFFICIAL number remains the separately recorded accepted duration.
    const QDateTime start = QDateTime::fromString(r->interruptionStartUtc,
                                                  Qt::ISODateWithMs);
    ev.calculatedDurationMs = start.isValid()
        ? start.msecsTo(QDateTime::currentDateTimeUtc()) : -1;
    ev.officiallyAcceptedDurationMs =
        r->officiallyAcceptedDurationMs >= 0 ? r->officiallyAcceptedDurationMs : 0;
    ev.systemRestoredUtc = QDateTime::currentDateTimeUtc()
        .toString(Qt::ISODateWithMs);
    ev.targetMoved = r->targetMoved;
    ev.originalTarget = r->originalTarget;
    ev.reserveTarget = r->reserveTarget;
    ev.backupScoreReviewed = r->backupScoreReviewed;
    ev.juryNote = juryNote;
    ev.rangeOfficerNote = rangeOfficerNote;
    ev.incidentReportRef = reportRef;
    return submit(DomainEvent(ev));
}

bool EstIncidentController::cancelIncident(const QString& reason)
{
    SessionStore* s = store();
    const EstIncidentRecord* r = openRecord();
    if (!s || !r) { emit actionFailed(QStringLiteral("No open incident.")); return false; }
    EstIncidentResolved ev;
    ev.incidentId = r->incidentId;
    ev.status = IncidentStatus::Abandoned;
    ev.officiallyAcceptedDurationMs = 0;
    ev.juryNote = reason;
    if (!submit(DomainEvent(ev)))
        return false;
    // A cancelled incident releases the frozen clock (no credit involved).
    const TimerState& t = s->state().timer;
    if (t.active && t.paused)
        submit(DomainEvent(TimerResumed{t.timerId, s->nowMonotonicMs()}));
    return true;
}

// ── projections ─────────────────────────────────────────────────────────────

bool EstIncidentController::hasOpenIncident() const
{
    return openRecord() != nullptr;
}

bool EstIncidentController::officialShotsBlockedQml() const
{
    SessionStore* s = store();
    return s && s->active() && officialsBlocked(s->state());
}

qint64 EstIncidentController::remainingCompetitionMs() const
{
    SessionStore* s = store();
    if (!s || !s->active())
        return 0;
    return remainingCompetitionMsFor(s->state(), s->nowMonotonicMs());
}

QVariantMap EstIncidentController::projectRecord(const EstIncidentRecord& r) const
{
    QVariantMap m;
    m[QStringLiteral("incidentId")] = r.incidentId;
    m[QStringLiteral("typeId")] = static_cast<int>(r.incidentType);
    m[QStringLiteral("scopeId")] = static_cast<int>(r.scope);
    m[QStringLiteral("firingPoint")] = r.firingPoint;
    m[QStringLiteral("relayId")] = r.relayId;
    m[QStringLiteral("interruptionStartUtc")] = r.interruptionStartUtc;
    m[QStringLiteral("systemRestoredUtc")] = r.systemRestoredUtc;
    m[QStringLiteral("calculatedDurationMs")] =
        static_cast<qlonglong>(r.calculatedDurationMs);
    m[QStringLiteral("acceptedDurationMs")] =
        static_cast<qlonglong>(r.officiallyAcceptedDurationMs);
    m[QStringLiteral("targetMoved")] = r.targetMoved;
    m[QStringLiteral("originalTarget")] = r.originalTarget;
    m[QStringLiteral("reserveTarget")] = r.reserveTarget;
    m[QStringLiteral("backupReview")] = static_cast<int>(r.backupReview);
    m[QStringLiteral("timeCreditMs")] = static_cast<qlonglong>(r.timeCreditMs);
    m[QStringLiteral("creditDecision")] = static_cast<int>(r.creditDecision);
    m[QStringLiteral("preparationGranted")] = r.preparationGranted;
    m[QStringLiteral("sightingGranted")] = r.sightingGranted;
    m[QStringLiteral("officialResumeAuthorised")] = r.officialResumeAuthorised;
    m[QStringLiteral("authorisedBy")] = r.authorisedBy;
    m[QStringLiteral("juryNote")] = r.juryNote;
    m[QStringLiteral("rangeOfficerNote")] = r.rangeOfficerNote;
    m[QStringLiteral("incidentReportRef")] = r.incidentReportRef;
    m[QStringLiteral("reason")] = r.reason;
    m[QStringLiteral("statusId")] = static_cast<int>(r.status);

    // Derived workflow status (presentation key; QML maps to labels).
    QString statusKey;
    if (r.status == static_cast<quint8>(IncidentStatus::Abandoned))
        statusKey = QStringLiteral("cancelled");
    else if (r.status == static_cast<quint8>(IncidentStatus::Resolved))
        statusKey = QStringLiteral("resolved");
    else if (r.officialResumeAuthorised)
        statusKey = QStringLiteral("resumed");
    else if (r.sightingGranted)
        statusKey = QStringLiteral("recovery-sighting");
    else if (r.preparationGranted)
        statusKey = QStringLiteral("recovery-preparation");
    else if (r.creditDecision != 0)
        statusKey = QStringLiteral("awaiting-official-resume");
    else
        statusKey = QStringLiteral("awaiting-jury-decision");
    m[QStringLiteral("statusKey")] = statusKey;

    // Live estimated interruption duration (UTC-based; Jury confirms/corrects).
    const QDateTime start = QDateTime::fromString(r.interruptionStartUtc,
                                                  Qt::ISODateWithMs);
    const qint64 estimated = start.isValid()
        ? start.msecsTo(QDateTime::currentDateTimeUtc()) : -1;
    m[QStringLiteral("estimatedDurationMs")] = static_cast<qlonglong>(estimated);
    // Boundary guidance for the OFFICIAL duration when recorded, else the
    // estimate. Exactly 3:00 / 5:00 => boundary keys (manual decision).
    m[QStringLiteral("boundaryKey")] = boundaryKey(
        r.officiallyAcceptedDurationMs >= 0 ? r.officiallyAcceptedDurationMs
                                            : estimated);
    return m;
}

QVariantMap EstIncidentController::activeIncident() const
{
    const EstIncidentRecord* r = openRecord();
    return r ? projectRecord(*r) : QVariantMap();
}

QVariantList EstIncidentController::incidentHistory() const
{
    QVariantList out;
    SessionStore* s = store();
    if (!s || !s->active())
        return out;
    for (const EstIncidentRecord& r : s->state().estIncidents)
        out.append(projectRecord(r));
    return out;
}
