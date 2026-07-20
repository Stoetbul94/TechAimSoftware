#pragma once
// Phase E — EST incident workflow service (application layer).
//
// The single seam between the operator-facing incident UI and the generic
// reliability events. Exposed to QML as the INCIDENTS context property. It
// submits typed events (EstIncidentRaised / EstDecisionRecorded /
// TimeCreditGranted / RecoveryPhaseEntered / TargetReassigned /
// EstIncidentResolved, plus the existing TimerPaused/TimerResumed for the
// competition-clock freeze) through the ACTIVE session's SessionStore; the
// reducer-owned EstIncidentRecord is the authoritative incident state — QML
// only renders projections of it.
//
// AUTHORITY MODEL (docs/issf-rules/est-malfunctions.md): this service may
// detect, measure, classify and RECOMMEND, but every allowance — time credit,
// 5-minute preparation, recovery sighting, target reassignment, backup-score
// acceptance, official resume — happens only through an explicit, journalled,
// authorised action invoked from the Jury/RO workflow. Nothing here auto-awards
// anything, and the exact 3:00/5:00 boundaries are surfaced as boundary cases
// requiring an authorised decision (never auto-classified).
//
// TIME CONCEPTS kept separate: the configured duration lives in config/
// TimerStarted; consumed time derives from the pause-aware TimerState; the
// frozen remaining is derived, never stored; the interruption duration is
// incident data; Jury credit is separate incident data; the ONE authoritative
// clock adjustment is remainingCompetitionMs() = duration − elapsedRunning +
// Σcredits, which replay reproduces deterministically.
//
// Ownership (single-lane build): exactly one session is active at a time, so
// an incident of ANY scope journals into that session; the scope field is
// preserved data for the future RMS, which will own cross-lane propagation.
//
// QtCore-only — exercised by the reliability console harness.

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include <functional>

#include "reliability/store/SessionStore.h"

class EstIncidentController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool hasOpenIncident READ hasOpenIncident NOTIFY incidentChanged)
    Q_PROPERTY(bool officialShotsBlocked READ officialShotsBlockedQml NOTIFY incidentChanged)

public:
    explicit EstIncidentController(QObject* parent = nullptr);

    // main.cpp wires this to return the ACTIVE session's store (QUAL or
    // FINALS3P), or nullptr when no session is running.
    void setStoreProvider(std::function<ta::rel::SessionStore*()> provider);

    // ── authorised workflow actions (each journals a typed event) ────────
    // Raise a range-caused incident. typeId/scopeId use the generic enums;
    // firingPoints may carry a comma-separated list when scope is
    // SelectedFiringPoints. Freezes the competition clock (TimerPaused) when
    // one is running.
    Q_INVOKABLE bool raiseIncident(int typeId, int scopeId,
                                   const QString& firingPoints,
                                   const QString& relayId,
                                   const QString& reason);
    // Jury confirms the official interruption duration.
    Q_INVOKABLE bool recordAcceptedDuration(qint64 durationMs,
                                            const QString& authorisedBy,
                                            const QString& reason);
    // Explicit, journalled "no additional allowance" ruling (distinct from
    // decision-pending; refused if a credit was already granted).
    Q_INVOKABLE bool recordNoAllowance(const QString& authorisedBy,
                                       const QString& reason);
    // Jury-authorised time credit (one credit per incident; refused if an
    // allowance ruling already exists). Never touches a clock directly.
    Q_INVOKABLE bool grantTimeCredit(qint64 durationMs,
                                     const QString& authorisedBy,
                                     const QString& reason);
    // Authorised recovery phases (RecoveryPhaseEntered). Preparation carries
    // the 5-minute allowance; completion of a phase is represented by entering
    // the next one (documented in est-malfunctions.md).
    Q_INVOKABLE bool beginRecoveryPreparation(const QString& authorisedBy);
    Q_INVOKABLE bool beginRecoverySighting(const QString& authorisedBy);
    // The official-resume gate: journals the authorisation and restarts the
    // frozen competition clock (TimerResumed).
    Q_INVOKABLE bool authoriseOfficialResume(const QString& authorisedBy);
    // Authorised move to a reserve target (journalled when it happens).
    Q_INVOKABLE bool reassignTarget(const QString& originalTarget,
                                    const QString& reserveTarget,
                                    const QString& authorisedBy,
                                    const QString& reason);
    // Backup-score review outcome: 1 accepted, 2 rejected, 3 inconclusive.
    Q_INVOKABLE bool recordBackupReview(int outcome,
                                        const QString& authorisedBy,
                                        const QString& reason);
    // Close the incident with the official record (status Resolved), or
    // cancel it (status Abandoned).
    Q_INVOKABLE bool resolveIncident(const QString& juryNote,
                                     const QString& rangeOfficerNote,
                                     const QString& reportRef);
    Q_INVOKABLE bool cancelIncident(const QString& reason);

    // ── projections (reducer state → QML) ────────────────────────────────
    bool hasOpenIncident() const;
    bool officialShotsBlockedQml() const;
    // Full projection of the open incident (empty map when none): every record
    // field plus derived statusKey/boundaryKey/estimated duration/remaining.
    Q_INVOKABLE QVariantMap activeIncident() const;
    // Every incident of the current session (the permanent per-session
    // history; the journal itself is the audit trail).
    Q_INVOKABLE QVariantList incidentHistory() const;
    // Frozen/adjusted remaining competition time now (pause-aware + credits).
    Q_INVOKABLE qint64 remainingCompetitionMs() const;

    // ── generic static policy helpers (pure state functions) ─────────────
    // True while any incident requires an authorised decision before official
    // shots may continue: status Open and official resume not yet authorised.
    static bool officialsBlocked(const ta::rel::SessionState& s)
    {
        using namespace ta::rel;
        for (const EstIncidentRecord& r : s.estIncidents)
            if (r.status == static_cast<quint8>(IncidentStatus::Open)
                    && !r.officialResumeAuthorised)
                return true;
        return false;
    }
    // The ONE authoritative clock computation (replay-deterministic):
    // remaining = duration − elapsedRunning(pause-aware) + Σ authorised credits.
    static qint64 remainingCompetitionMsFor(const ta::rel::SessionState& s,
                                            qint64 atMonoMs)
    {
        using namespace ta::rel;
        const TimerState& t = s.timer;
        if (!t.active || t.durationMs <= 0)
            return 0;
        const qint64 stopAt = t.paused ? t.pausedAtMonoMs : atMonoMs;
        qint64 elapsedRunning = stopAt - t.startedAtMonoMs - t.pausedAccumMs;
        if (elapsedRunning < 0)
            elapsedRunning = 0;
        qint64 credit = 0;
        for (const EstIncidentRecord& r : s.estIncidents)
            credit += r.timeCreditMs;
        const qint64 remaining = t.durationMs - elapsedRunning + credit;
        return remaining > 0 ? remaining : 0;
    }
    // Boundary guidance for the supplied 6.11.2 thresholds. PRESENTATION ONLY —
    // exactly 3:00 and exactly 5:00 return boundary keys that require an
    // authorised interpretation (the app never auto-classifies them).
    Q_INVOKABLE static QString boundaryKey(qint64 durationMs)
    {
        if (durationMs < 0)          return QStringLiteral("unknown");
        if (durationMs < 180000)     return QStringLiteral("under-3");
        if (durationMs == 180000)    return QStringLiteral("exactly-3-boundary");
        if (durationMs < 300000)     return QStringLiteral("over-3");
        if (durationMs == 300000)    return QStringLiteral("exactly-5-boundary");
        return QStringLiteral("over-5");
    }

signals:
    void incidentChanged();
    void actionFailed(QString detail);

private:
    ta::rel::SessionStore* store() const
    { return m_provider ? m_provider() : nullptr; }
    const ta::rel::EstIncidentRecord* openRecord() const;
    bool submit(const ta::rel::DomainEvent& event);
    QVariantMap projectRecord(const ta::rel::EstIncidentRecord& r) const;

    std::function<ta::rel::SessionStore*()> m_provider;
};
