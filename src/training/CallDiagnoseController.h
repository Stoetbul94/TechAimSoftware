#ifndef TA_TRAINING_CALLDIAGNOSECONTROLLER_H
#define TA_TRAINING_CALLDIAGNOSECONTROLLER_H

// Call & Diagnose (T2) — controller (QML context property CALLDIAG). Owns ALL
// Call & Diagnose state via its OWN reliability SessionStore (sessionKind =
// "Training", programId = "call_and_diagnose"). It shares the accepted Training
// infrastructure (SessionStore/journal/replay/recovery, the F10 source gate and
// the Training-owned sighter events) but does NOT couple to TechnicalBlocks
// state — Technical Blocks is frozen.
//
// The athlete CALLS each shot before the actual impact is revealed. The actual
// shot is journalled hidden; the call is journalled separately; the reveal
// compares them. Exactly ONE unresolved shot may exist at a time — the next
// shot is refused until the current call is confirmed and continued.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

#include "CallDiagnoseTypes.h"
#include "CallDiagnoseAnalytics.h"
#include "reliability/store/SessionStore.h"
#include "reliability/recovery/RecoveryCoordinator.h"

class CallDiagnoseController : public QObject
{
    Q_OBJECT
    // phase: 0 Idle, 1 Sighters, 2 AwaitingShot, 3 AwaitingCall, 4 Reveal, 5 Complete
    Q_PROPERTY(int phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool active READ active NOTIFY phaseChanged)
    Q_PROPERTY(bool inSighters READ inSighters NOTIFY phaseChanged)
    Q_PROPERTY(bool awaitingShot READ awaitingShot NOTIFY phaseChanged)
    Q_PROPERTY(bool awaitingCall READ awaitingCall NOTIFY phaseChanged)
    Q_PROPERTY(bool revealOpen READ revealOpen NOTIFY phaseChanged)
    Q_PROPERTY(bool completed READ isCompleted NOTIFY phaseChanged)
    Q_PROPERTY(int shotCount READ shotCount NOTIFY configChanged)
    Q_PROPERTY(int shotsCompleted READ shotsCompleted NOTIFY progressChanged)
    Q_PROPERTY(int pendingShotNumber READ pendingShotNumber NOTIFY progressChanged)
    Q_PROPERTY(int sighterCount READ sighterCount NOTIFY progressChanged)
    Q_PROPERTY(bool hasPendingCall READ hasPendingCall NOTIFY callChanged)
    Q_PROPERTY(QString technicalFocus READ technicalFocus NOTIFY configChanged)
    Q_PROPERTY(QString positionName READ positionName NOTIFY progressChanged)
    Q_PROPERTY(QString startLabel READ startLabel NOTIFY progressChanged)
    Q_PROPERTY(QString estimatedTime READ estimatedTime NOTIFY configChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY phaseChanged)
    Q_PROPERTY(QString sessionOperatingMode READ sessionOperatingMode NOTIFY phaseChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY phaseChanged)
    Q_PROPERTY(QString lastStartError READ lastStartError NOTIFY startErrorChanged)

public:
    explicit CallDiagnoseController(QObject* parent = nullptr);
    ~CallDiagnoseController() override;

    // ── configuration ────────────────────────────────────────────────────
    Q_INVOKABLE void configureDefaults(const QString& disciplineId);
    Q_INVOKABLE void setShotCount(int n);
    Q_INVOKABLE void setTechnicalFocus(const QString& focus);
    Q_INVOKABLE QString validateConfig() const;
    Q_INVOKABLE QStringList focusOptionsForDiscipline() const;

    // ── lifecycle ─────────────────────────────────────────────────────────
    Q_INVOKABLE bool startCallDiagnose(const QString& athlete);
    // Sighters → calling. Closes the sighter phase durably and begins accepting
    // called shots for the current position. Idempotent (double-tap safe).
    Q_INVOKABLE bool startProgramme();
    // Measured shot from the shared pipeline. shotSource: 0 Physical / 1 Simulated.
    Q_INVOKABLE bool registerShot(double xMm, double yMm, double decimalScore,
                                  qint64 externalId, double directionDeg, int shotSource);
    // The athlete's call (memory only until confirmed). Only in AwaitingCall.
    Q_INVOKABLE bool submitCall(double calledXMm, double calledYMm);
    Q_INVOKABLE void clearCall();
    // Durably record the pending call and reveal the actual. Rejects duplicates.
    Q_INVOKABLE bool confirmCall();
    Q_INVOKABLE bool saveShotNote(const QString& note);        // current shot
    Q_INVOKABLE bool continueToNext();                          // Reveal → next
    Q_INVOKABLE bool endEarly();                                // finish with the shots so far
    Q_INVOKABLE bool closeCleanly();                            // durable clean close
    Q_INVOKABLE void resetCallDiagnose();
    Q_INVOKABLE void setOperatingMode(int mode) { m_operatingMode = mode; }

    // ── recovery ──────────────────────────────────────────────────────────
    Q_INVOKABLE bool resumeFromRecovery(const QString& sessionId);
    Q_INVOKABLE void discardRecovery(const QString& sessionId);
    Q_INVOKABLE QVariantList recoveredSighterShots() const;
    Q_INVOKABLE qint64 recoveredMaxExternalId() const { return m_lastExternalId; }

    // ── projections ────────────────────────────────────────────────────��──
    int phase() const { return m_phase; }
    bool active() const { return m_store && m_store->active(); }
    bool inSighters() const { return m_phase == 1; }
    bool awaitingShot() const { return m_phase == 2; }
    bool awaitingCall() const { return m_phase == 3; }
    bool revealOpen() const { return m_phase == 4; }
    bool isCompleted() const { return m_phase == 5; }
    int shotCount() const { return m_cfg.shotCount; }
    int shotsCompleted() const;                 // completed calls in current position
    int pendingShotNumber() const;              // 1-based shot being (or next to be) called
    int sighterCount() const;                   // sighters in the current phase
    bool hasPendingCall() const { return m_hasPendingCall; }
    QString technicalFocus() const { return m_cfg.technicalFocus; }
    QString positionName() const;
    QString startLabel() const;                 // "Start Call & Diagnose" / "Start Kneeling …"
    QString estimatedTime() const { return m_cfg.estimatedTime; }
    QString sessionId() const;
    QString sessionOperatingMode() const;
    QString lastError() const { return m_lastError; }
    QString lastStartError() const { return m_lastStartError; }

    // Reveal for the CURRENT shot (only valid in Reveal/Complete): called/actual
    // mm, signed X/Y error, radial error, actual score, direction phrase.
    Q_INVOKABLE QVariantMap revealCurrent() const;
    // Session analytics (measured only). Optional position filter (-1 = all).
    Q_INVOKABLE QVariantMap sessionStats(int positionFilter = -1) const;
    Q_INVOKABLE QStringList sessionObservations() const;   // measured statements
    Q_INVOKABLE QVariantList shotReviewList() const;       // revealed shots only
    Q_INVOKABLE int sessionDurationSec() const;
    Q_INVOKABLE QVariantMap reportModel() const;           // PDF/summary source

    // test seam
    ta::rel::SessionStore* storeForTesting() { return m_store.get(); }
    ta::rel::SessionStore* store() { return m_store.get(); }

signals:
    void phaseChanged();
    void configChanged();
    void progressChanged();
    void callChanged();
    void startErrorChanged();
    void sightersCleared();                     // clear sighter markers
    void sighterAccepted(QVariantMap record);   // projection-safe sighter (shown)
    void shotReceived();                        // actual arrived (HIDDEN) — prompt a call
    void callRevealed(QVariantMap record);      // reveal called+actual+error
    void revealCleared();                       // clear call/actual before next shot
    void shotRejected(QString reason);
    void completedChanged();
    void journalWriteFailed(QString path, QString detail);

private:
    bool submit(const ta::rel::DomainEvent& ev);
    bool acceptSighter(double xMm, double yMm, double decimalScore,
                       qint64 externalId, double directionDeg, int shotSource);
    bool acceptActualShot(double xMm, double yMm, double decimalScore,
                          qint64 externalId, double directionDeg, int shotSource);
    const ta::rel::SessionState& st() const { return m_store->state(); }
    int currentPositionIndex() const { return m_currentPosition; }
    int completedInPosition(int pos) const;
    int receivedInPosition(int pos) const;
    const ta::rel::CallDiagnoseShotRecord* currentUnresolvedShot() const;
    const ta::rel::CallDiagnoseShotRecord* lastResolvedShot() const;

    std::unique_ptr<ta::rel::SessionStore> m_store;
    std::unique_ptr<ta::rel::RecoveryCoordinator> m_recovery;
    ta::training::CallDiagnoseConfig m_cfg;
    int m_phase = 0;
    int m_currentPosition = 0;       // 0-based (K/P/S for 3P)
    int m_operatingMode = -1;
    qint64 m_lastExternalId = -1;
    qint64 m_sessionStartMonoMs = 0;
    // in-memory pending call (durable only on confirmCall)
    bool m_hasPendingCall = false;
    double m_pendingCalledXMm = 0.0;
    double m_pendingCalledYMm = 0.0;
    qint16 m_pendingShotNumber = 0;  // shot currently awaiting a call
    QString m_lastError;
    QString m_lastStartError;
};

#endif // TA_TRAINING_CALLDIAGNOSECONTROLLER_H
