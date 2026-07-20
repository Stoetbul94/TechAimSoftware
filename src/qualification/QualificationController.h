#pragma once
// Phase B — shared qualification persistence seam.
//
// The discipline-agnostic write-path controller for the qualification
// disciplines (10m Air Rifle, 10m Air Pistol, 50m Rifle Prone). Exposed to QML
// as the QUAL context property. It owns a reliability SessionStore and turns
// classified, scored shots into typed journal events so the journal + reducer
// become the authoritative match record (mirrors Finals3PController's M2 write
// path, minus stages / command windows / audio).
//
// It contains NO ISSF rule: official-shot count, match/prep durations, the
// sighter limit and the decimal-vs-integer scoring decision are all supplied by
// the caller (QML reads them from APPSETTINGS / the discipline selectors). ISSF
// rules live in docs/issf-rules/, never in the reliability engine.
//
// Durability boundary (docs/session-reliability-phaseB-qualification.md): a shot
// is not "accepted" until its SighterAccepted/ShotAccepted event has been
// submitted to the SessionStore; only then is shotAccepted() emitted for the UI
// to project. SessionStore's never-refuse-to-score policy keeps officials
// durable (journalled or elastically queued) even when persistence is degraded.
//
// QtCore-only (QObject for signals/invokables) — no QtGui/QML dependency, so it
// is exercised by the QtCore reliability harness.

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

#include "reliability/store/SessionStore.h"
#include "reliability/recovery/RecoveryCoordinator.h"

class QualificationController : public QObject
{
    Q_OBJECT

    // Persistence health for the PersistenceBanner (int mirrors ta::rel::Health:
    // 0 Healthy … 5 Failed).
    Q_PROPERTY(int persistenceHealth READ persistenceHealth NOTIFY persistenceHealthChanged)
    Q_PROPERTY(QString journalPath READ journalPath NOTIFY sessionChanged)
    Q_PROPERTY(bool active READ active NOTIFY sessionChanged)
    Q_PROPERTY(int officialShotCount READ officialShotCount NOTIFY shotCountsChanged)
    Q_PROPERTY(int sighterCount READ sighterCount NOTIFY shotCountsChanged)
    // The next official shot number the reducer will accept (officials + 1).
    Q_PROPERTY(int nextOfficialShotNumber READ nextOfficialShotNumber NOTIFY shotCountsChanged)
    Q_PROPERTY(double totalDecimal READ totalDecimal NOTIFY totalsChanged)

public:
    explicit QualificationController(QObject* parent = nullptr);
    ~QualificationController() override;

    // ── lifecycle ────────────────────────────────────────────────────────
    // disciplineId is the stable machine id: "AR10" | "AP10" | "PRONE50". The
    // caller supplies the ISSF course parameters; this class stores them and
    // enforces nothing rule-specific. Returns false if the discipline id is not
    // a supported qualification discipline or the journal header write fails.
    Q_INVOKABLE bool startSession(const QString& disciplineId,
                                  const QString& matchType,
                                  const QString& athlete,
                                  int officialShots, qint64 matchMs,
                                  qint64 prepMs, int sighterLimit,
                                  const QString& lane, const QString& targetId);
    Q_INVOKABLE void beginPreparation();
    Q_INVOKABLE void beginSighting();
    Q_INVOKABLE void beginOfficialMatch();

    // Submit a scored shot. `score` is the discipline's final scored value
    // (decimal for rifle, floored integer for full-ring pistol — the caller
    // decides); it is persisted as fixed-point tenths. Returns false only if the
    // reducer rejects the shot (e.g. official beyond the phase, duplicate).
    Q_INVOKABLE bool submitSighter(double xMm, double yMm, double score,
                                   qint64 externalId, double directionDeg,
                                   bool simulated);
    Q_INVOKABLE bool submitOfficial(double xMm, double yMm, double score,
                                    qint64 externalId, double directionDeg,
                                    bool simulated);

    Q_INVOKABLE void completeMatch();   // MatchCompleted (totals from reducer)
    Q_INVOKABLE void closeSession();    // SessionClosed + CleanShutdown + archive

    Q_INVOKABLE void pumpRetryQueue();  // drive the store's retry drain

    // ── Phase D: crash recovery / resume ─────────────────────────────────
    // Scan Sessions/Current for unfinished sessions (QVariantList of candidate
    // maps for the recovery dialog). Discipline-agnostic — the dialog/dispatcher
    // routes by disciplineId.
    Q_INVOKABLE QVariantList scanForRecovery();
    // Resume a crashed qualification session: reopen its journal in append
    // mode, adopt the reducer-rebuilt state, and prime the recovered shots /
    // timer for QML projection. Refuses non-qualification (AR10/AP10/PRONE50)
    // sessions — it never creates a new session or archives the interrupted
    // journal. Returns false if the session cannot be resumed.
    Q_INVOKABLE bool resumeFromRecovery(const QString& sessionId);
    Q_INVOKABLE void discardRecovery(const QString& sessionId);

    // Recovered shots for the QML projection (sighters first, then officials,
    // each in the same shape as the live shotAccepted record). Read-only
    // projection of reducer state — nothing is re-journalled.
    Q_INVOKABLE QVariantList recoveredShots() const;
    // Frozen remaining competition time (ms) for the recovered phase clock:
    // durationMs − (lastEventMonoMs − timer.startedAtMonoMs). 0 if no timer.
    Q_INVOKABLE qint64 recoveredRemainingMs() const;
    // Recovered MatchPhase as int (0 None, 1 Preparation, 2 Sighting,
    // 3 OfficialMatch) and TimerId (0 Preparation … 3 Window).
    Q_INVOKABLE int recoveredPhaseId() const;
    Q_INVOKABLE int recoveredTimerId() const;
    // Largest external shot id seen — used to rebase live shot identity so a
    // re-reported hardware measurement after resume is not double-counted.
    Q_INVOKABLE qint64 recoveredMaxExternalId() const;
    // True once a session has been resumed (drives the recovery-projection
    // path in QML).
    Q_INVOKABLE bool recovered() const { return m_recovered; }

    // ── reads (all derived from the reducer's authoritative state) ─────────
    int persistenceHealth() const;
    QString journalPath() const;
    QString sessionId() const;
    bool active() const;
    int officialShotCount() const;
    int sighterCount() const;
    int nextOfficialShotNumber() const;
    double totalDecimal() const;

    // Test seam: inject an in-memory journal file BEFORE startSession so the
    // controller can be exercised deterministically in the console harness.
    ta::rel::SessionStore* storeForTesting() { return m_store.get(); }
    // Phase E: the incident workflow service submits its typed events through
    // the ACTIVE session's store (main.cpp wires the provider).
    ta::rel::SessionStore* store() { return m_store.get(); }

signals:
    void sessionChanged();
    void shotCountsChanged();
    void totalsChanged();
    void persistenceHealthChanged(int health);
    // Emitted AFTER a durable submit — the UI projects this into its models.
    void shotAccepted(QVariantMap record);
    void journalWriteFailed(QString path, QString detail);

private:
    bool submitShot(bool sighter, double xMm, double yMm, double score,
                    qint64 externalId, double directionDeg, bool simulated);
    void submitEvent(const ta::rel::DomainEvent& event);
    void loadRecoveredState(const ta::rel::RecoveredMatchState& recovered);

    std::unique_ptr<ta::rel::SessionStore> m_store;
    std::unique_ptr<ta::rel::RecoveryCoordinator> m_recovery;
    bool m_recovered = false;
    // tm of the last valid recovered event (for the timer rebase). Set only on
    // resume; the reducer stays the sole authority for everything else.
    qint64 m_recoveredLastEventMonoMs = 0;
    ta::rel::Discipline m_discipline = ta::rel::Discipline::None;
    bool m_journalFailureNotified = false;
    // Configured official-shot cap (from startSession; <= 0 = uncapped/free
    // practice). Enforced at the durable boundary so a shot beyond the cap is
    // never journalled — the ISSF count is a caller-supplied config value, not
    // a rule hardcoded in the engine.
    int m_officialShots = 0;
    // Phase-clock durations (ms) supplied at startSession, journalled as
    // TimerStarted anchors (Phase C) so a recovered session can rebase the
    // competition clock to its frozen remaining value: remaining = durationMs −
    // (lastEventMonoMs − timer.startedAtMonoMs). <= 0 means no timed clock
    // (free practice) and no TimerStarted is emitted.
    qint64 m_prepMs = 0;
    qint64 m_matchMs = 0;

    // Qualification is a single official stage; sighting is stage 0.
    static constexpr qint16 kSightingStageId = 0;
    static constexpr qint16 kOfficialStageId = 1;
};
