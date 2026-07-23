#ifndef TA_TRAINING_POSITIONTRANSITIONCONTROLLER_H
#define TA_TRAINING_POSITIONTRANSITIONCONTROLLER_H

// Position Transition (T4) — controller (QML context property POSTRANS). 50m
// Rifle 3 Positions ONLY. Owns ALL state via its OWN reliability SessionStore
// (sessionKind=Training, programId=position_transition). Reuses the accepted
// Training infrastructure (SessionStore/journal/replay/recovery, the F10 source
// gate, the analytics helpers) but is fully separate from the 3P qualification
// and Final workflows and from the other Training programmes.
//
// Per position (and repeat): build the position (setup timer), confirm Position
// Ready, fire optional sighters, then a short counted verification block. It
// measures setup/transition timing, sighters, first-shot readiness and early
// group quality — never a technical cause.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

#include "PositionTransitionTypes.h"
#include "reliability/store/SessionStore.h"
#include "reliability/recovery/RecoveryCoordinator.h"

class PositionTransitionController : public QObject
{
    Q_OBJECT
    // phase: 0 Idle, 1 PositionSetup, 2 Sighters, 3 VerificationActive,
    //        4 PositionReview, 5 Complete
    Q_PROPERTY(int phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool active READ active NOTIFY phaseChanged)
    Q_PROPERTY(bool inSetup READ inSetup NOTIFY phaseChanged)
    Q_PROPERTY(bool inSighters READ inSighters NOTIFY phaseChanged)
    Q_PROPERTY(bool verifying READ verifying NOTIFY phaseChanged)
    Q_PROPERTY(bool reviewOpen READ reviewOpen NOTIFY phaseChanged)
    Q_PROPERTY(bool completed READ isCompleted NOTIFY phaseChanged)
    Q_PROPERTY(int verificationShots READ verificationShots NOTIFY configChanged)
    Q_PROPERTY(int shotsCompleted READ shotsCompleted NOTIFY progressChanged)   // verif shots in current
    Q_PROPERTY(int sighterCount READ sighterCount NOTIFY progressChanged)
    Q_PROPERTY(int currentRepeat READ currentRepeat NOTIFY progressChanged)
    Q_PROPERTY(int totalRepeats READ totalRepeats NOTIFY configChanged)
    Q_PROPERTY(QString technicalFocus READ technicalFocus NOTIFY configChanged)
    Q_PROPERTY(QString positionName READ positionName NOTIFY progressChanged)
    Q_PROPERTY(QString nextPositionName READ nextPositionName NOTIFY progressChanged)
    Q_PROPERTY(QString sequenceArrow READ sequenceArrow NOTIFY configChanged)
    Q_PROPERTY(QString startVerifyLabel READ startVerifyLabel NOTIFY progressChanged)
    Q_PROPERTY(QString estimatedTime READ estimatedTime NOTIFY configChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY phaseChanged)
    Q_PROPERTY(QString sessionOperatingMode READ sessionOperatingMode NOTIFY phaseChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY phaseChanged)
    Q_PROPERTY(QString lastStartError READ lastStartError NOTIFY startErrorChanged)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY progressChanged)

public:
    explicit PositionTransitionController(QObject* parent = nullptr);
    ~PositionTransitionController() override;

    // ── configuration ─────────────────────────────────────────────────────
    Q_INVOKABLE void configureDefaults();
    Q_INVOKABLE void setSequencePreset(int preset);   // 0 full,1 K→P,2 P→S,3 single-K,4 single-P,5 single-S
    Q_INVOKABLE void setVerificationShots(int n);
    Q_INVOKABLE void setRepeats(int n);
    Q_INVOKABLE void setChecklistMode(int mode);
    Q_INVOKABLE void setTechnicalFocus(const QString& focus);
    Q_INVOKABLE QString validateConfig() const;
    Q_INVOKABLE QStringList focusOptionsForDiscipline() const;
    Q_INVOKABLE QString sequenceString() const;
    Q_INVOKABLE int checklistMode() const { return m_cfg.checklistMode; }

    // ── lifecycle ───────────────────────────────────────────────────────────
    Q_INVOKABLE bool startPositionTransition(const QString& athlete);
    Q_INVOKABLE bool positionReady();                 // Setup → Sighters
    Q_INVOKABLE bool startVerification();             // Sighters → VerificationActive
    Q_INVOKABLE bool registerShot(double xMm, double yMm, double decimalScore,
                                  qint64 externalId, double directionDeg, int shotSource);
    Q_INVOKABLE bool setChecklistItem(int itemIndex, int state);
    Q_INVOKABLE bool saveNote(const QString& note);   // current position/repeat
    Q_INVOKABLE bool continueToNext();                // PositionReview → next / complete
    Q_INVOKABLE bool endEarly();
    Q_INVOKABLE bool closeCleanly();
    Q_INVOKABLE void resetPositionTransition();
    Q_INVOKABLE void setOperatingMode(int mode) { m_operatingMode = mode; }

    // ── recovery ──────────────────────────────────────────────────────────
    Q_INVOKABLE bool resumeFromRecovery(const QString& sessionId);
    Q_INVOKABLE void discardRecovery(const QString& sessionId);
    Q_INVOKABLE QVariantList recoveredSighterShots() const;
    Q_INVOKABLE qint64 recoveredMaxExternalId() const { return m_lastExternalId; }

    // ── projections ─────────────────────────────────────────────────────────
    int phase() const { return m_phase; }
    bool active() const { return m_store && m_store->active(); }
    bool inSetup() const { return m_phase == 1; }
    bool inSighters() const { return m_phase == 2; }
    bool verifying() const { return m_phase == 3; }
    bool reviewOpen() const { return m_phase == 4; }
    bool isCompleted() const { return m_phase == 5; }
    int verificationShots() const { return m_cfg.verificationShots; }
    int shotsCompleted() const;
    int sighterCount() const;
    int currentRepeat() const { return m_currentRepeat; }
    int totalRepeats() const { return m_cfg.repeats; }
    QString technicalFocus() const { return m_cfg.technicalFocus; }
    QString positionName() const;
    QString nextPositionName() const;
    QString sequenceArrow() const { return m_cfg.sequenceArrowText(); }
    QString startVerifyLabel() const;
    QString estimatedTime() const { return m_cfg.estimatedTime; }
    QString sessionId() const;
    QString sessionOperatingMode() const;
    QString lastError() const { return m_lastError; }
    QString lastStartError() const { return m_lastStartError; }
    bool hasNext() const;

    // elapsed seconds since setup began / since Position Ready (0 when N/A)
    Q_INVOKABLE int setupElapsedSec() const;
    Q_INVOKABLE int readyElapsedSec() const;
    // checklist items (labels) for the current position + their states.
    Q_INVOKABLE QVariantList checklistItems() const;
    // per-position measured review (timing + group metrics + first-shot + pattern).
    Q_INVOKABLE QVariantMap positionReview(int position, int repeat) const;
    Q_INVOKABLE QVariantMap currentReview() const;
    Q_INVOKABLE QVariantList positionComparison() const;   // all completed records
    Q_INVOKABLE QVariantList repeatComparison() const;     // same position across repeats
    Q_INVOKABLE QStringList sessionObservations() const;
    Q_INVOKABLE QVariantMap sessionInsights() const;       // "What You Should Take"
    Q_INVOKABLE int sessionDurationSec() const;
    Q_INVOKABLE QVariantList verificationPlot(int position, int repeat) const;
    Q_INVOKABLE QVariantMap reportModel() const;

    // test seam
    ta::rel::SessionStore* storeForTesting() { return m_store.get(); }
    ta::rel::SessionStore* store() { return m_store.get(); }

signals:
    void phaseChanged();
    void configChanged();
    void progressChanged();
    void startErrorChanged();
    void sightersCleared();
    void sighterAccepted(QVariantMap record);
    void verificationShotAccepted(QVariantMap record);
    void shotRejected(QString reason);
    void positionCompleted(int position, int repeat);
    void completedChanged();
    void journalWriteFailed(QString path, QString detail);

private:
    bool submit(const ta::rel::DomainEvent& ev);
    bool acceptSighter(double xMm, double yMm, double decimalScore,
                       qint64 externalId, double directionDeg, int shotSource);
    bool acceptVerifShot(double xMm, double yMm, double decimalScore,
                         qint64 externalId, double directionDeg, int shotSource);
    const ta::rel::SessionState& st() const { return m_store->state(); }
    const ta::rel::PtPositionRecord* record(int position, int repeat) const;
    int currentPositionId() const;         // the position id for m_seqIndex
    int repeatForIndex(int seqIndex) const;
    bool advanceIndex();                   // move to next (pos,repeat); false if none

    std::unique_ptr<ta::rel::SessionStore> m_store;
    std::unique_ptr<ta::rel::RecoveryCoordinator> m_recovery;
    ta::training::PositionTransitionConfig m_cfg;
    int m_phase = 0;
    int m_seqIndex = 0;                    // 0-based over sequence×repeats
    int m_currentRepeat = 1;
    int m_operatingMode = -1;
    qint64 m_lastExternalId = -1;
    qint64 m_setupStartMonoMs = 0;         // setup/transition timer anchor
    qint64 m_readyMonoMs = 0;              // Position Ready anchor
    QString m_lastError;
    QString m_lastStartError;
};

#endif // TA_TRAINING_POSITIONTRANSITIONCONTROLLER_H
