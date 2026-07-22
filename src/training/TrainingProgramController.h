#ifndef TA_TRAINING_PROGRAMCONTROLLER_H
#define TA_TRAINING_PROGRAMCONTROLLER_H

// Training Lab (T1) — TrainingProgramController (QML context property
// TRAINING). Owns ALL Training Lab state — qualification/Final controllers
// never own it. Mirrors the proven QualificationController shape: a private
// reliability SessionStore journals Training-specific events (sessionKind =
// "Training"), the reducer state is authoritative, and the athlete-facing
// visibility is a CONTROLLER projection over always-measured journal data.
//
// Live/Demo: the accepted F10 input-source gate runs before any durable
// acceptance — a wrong-source shot produces NO event, NO block progress, NO
// display change. The operating mode captured at session start is stamped
// into the session header and stays authoritative for the session.

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

#include "TrainingProgramTypes.h"
#include "TrainingBlockMetrics.h"
#include "reliability/store/SessionStore.h"
#include "reliability/recovery/RecoveryCoordinator.h"

class TrainingProgramController : public QObject
{
    Q_OBJECT
    // phase: 0 Idle, 1 SetupConfirmed(ready), 2 BlockActive, 3 BlockReview,
    //        4 Complete
    Q_PROPERTY(int phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool active READ active NOTIFY phaseChanged)
    Q_PROPERTY(int currentBlock READ currentBlock NOTIFY progressChanged)
    Q_PROPERTY(int blockCount READ blockCount NOTIFY configChanged)
    Q_PROPERTY(int shotsPerBlock READ shotsPerBlock NOTIFY configChanged)
    Q_PROPERTY(int shotsInBlock READ shotsInBlock NOTIFY progressChanged)
    Q_PROPERTY(int totalShots READ totalShots NOTIFY progressChanged)
    Q_PROPERTY(int visibilityMode READ visibilityMode NOTIFY configChanged)
    Q_PROPERTY(QString technicalFocus READ technicalFocus NOTIFY configChanged)
    Q_PROPERTY(QString positionName READ positionName NOTIFY progressChanged)
    Q_PROPERTY(QString estimatedTime READ estimatedTime NOTIFY configChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY phaseChanged)
    Q_PROPERTY(QString sessionOperatingMode READ sessionOperatingMode NOTIFY phaseChanged)
    // Visibility projections — QML must bind to THESE, never raw scores.
    Q_PROPERTY(bool showImpacts READ showImpacts NOTIFY progressChanged)
    Q_PROPERTY(bool showScores READ showScores NOTIFY progressChanged)
    Q_PROPERTY(bool showGroup READ showGroup NOTIFY progressChanged)
    // T1.1: user-facing reason the last start attempt failed ("" = none).
    // Plain language, no internal enums — shown verbatim in the dialog.
    Q_PROPERTY(QString lastStartError READ lastStartError NOTIFY startErrorChanged)

public:
    explicit TrainingProgramController(QObject* parent = nullptr);
    ~TrainingProgramController() override;

    // ── configuration (before start) ─────────────────────────────────────
    Q_INVOKABLE void configureDefaults(const QString& disciplineId);
    Q_INVOKABLE void setBlockCount(int n);
    Q_INVOKABLE void setShotsPerBlock(int n);
    Q_INVOKABLE void setVisibilityMode(int mode);
    Q_INVOKABLE void setTechnicalFocus(const QString& focus);
    Q_INVOKABLE QString validateConfig() const;   // "" = valid
    Q_INVOKABLE QStringList focusOptionsForDiscipline() const;

    // ── lifecycle ────────────────────────────────────────────────────────
    // Journals SessionStarted(kind=Training) + TrainingSessionStarted +
    // TrainingBlockStarted(1). Returns false (with lastError) when invalid.
    Q_INVOKABLE bool startTraining(const QString& athlete);
    // Measured shot from the shared detection pipeline. shotSource: 0=Physical,
    // 1=Simulated. Returns true when durably accepted.
    Q_INVOKABLE bool registerShot(double xMm, double yMm, double decimalScore,
                                  qint64 externalId, double directionDeg,
                                  int shotSource);
    Q_INVOKABLE bool saveNote(const QString& note);      // one per block review
    Q_INVOKABLE bool continueToNextBlock();              // explicit action
    // End from Block Review before the configured final block: appends ONE
    // TrainingCompleted carrying the TRUE completed-block count (never
    // fabricates uncompleted blocks) and moves to the summary. Completed
    // blocks stay intact; endedEarly() distinguishes the partial session.
    Q_INVOKABLE bool endTrainingEarly();
    Q_INVOKABLE bool endedEarly() const;
    Q_INVOKABLE void closeTrainingSession();             // durable clean close
    Q_INVOKABLE void resetTraining();                    // clear projections
    // Elapsed seconds since the current block started (0 when none).
    Q_INVOKABLE int blockElapsedSec() const;

    // ── T1 closure: in-place Training recovery ───────────────────────────
    // Reopens the crashed journal in append mode, adopts the replayed state
    // and re-derives the controller phase (active block / review / complete).
    // Refuses non-Training sessions — a competition journal is NEVER resumed
    // here (and Training is never resumed by a competition controller).
    Q_INVOKABLE bool resumeFromRecovery(const QString& sessionId);
    Q_INVOKABLE void discardRecovery(const QString& sessionId);
    // Recovered shots for the face restore (mm coords + block/position);
    // the QML restorer applies the visibility mode before drawing.
    Q_INVOKABLE QVariantList recoveredCurrentBlockShots() const;
    Q_INVOKABLE qint64 recoveredMaxExternalId() const { return m_lastExternalId; }

    // F10 gate: running operating mode (0=Live, 1=Demo, -1 unset/permissive).
    Q_INVOKABLE void setOperatingMode(int mode) { m_operatingMode = mode; }

    // ── projections ──────────────────────────────────────────────────────
    int phase() const { return m_phase; }
    bool active() const { return m_store && m_store->active(); }
    int currentBlock() const { return m_currentBlock; }
    int blockCount() const { return m_cfg.blockCount; }
    int shotsPerBlock() const { return m_cfg.shotsPerBlock; }
    int shotsInBlock() const;
    int totalShots() const;
    int visibilityMode() const { return static_cast<int>(m_cfg.visibility); }
    QString technicalFocus() const { return m_cfg.technicalFocus; }
    QString positionName() const;
    QString estimatedTime() const { return m_cfg.estimatedTime; }
    QString sessionId() const;
    QString sessionOperatingMode() const;
    QString lastError() const { return m_lastError; }
    QString lastStartError() const { return m_lastStartError; }
    bool showImpacts() const;
    bool showScores() const;
    bool showGroup() const;

    // Measured results — REVEALED data only (block review / final comparison).
    // Live-block hidden values are not exposed through these.
    Q_INVOKABLE QVariantMap blockReviewMetrics(int blockIndex1) const;
    Q_INVOKABLE QVariantList completedBlockSummaries() const;
    Q_INVOKABLE QVariantMap finalComparison() const;
    // Shot plot for a REVEALED block (review/comparison only).
    Q_INVOKABLE QVariantList blockShotPlot(int blockIndex1) const;

    // test seam (harness injects clock/file before startTraining)
    ta::rel::SessionStore* storeForTesting() { return m_store.get(); }
    ta::rel::SessionStore* store() { return m_store.get(); }

signals:
    void phaseChanged();
    void configChanged();
    void progressChanged();
    void startErrorChanged();
    void shotAccepted(QVariantMap record);   // projection-safe (no score in hidden modes)
    void shotRejected(QString reason);
    void blockCompleted(int blockIndex);
    void trainingCompleted();
    void journalWriteFailed(QString path, QString detail);

private:
    bool submit(const ta::rel::DomainEvent& ev);
    const ta::rel::SessionState& st() const { return m_store->state(); }

    std::unique_ptr<ta::rel::SessionStore> m_store;
    std::unique_ptr<ta::rel::RecoveryCoordinator> m_recovery;
    ta::training::TechnicalBlocksConfig m_cfg;
    int m_phase = 0;
    int m_currentBlock = 0;          // 1-based
    int m_operatingMode = -1;        // -1 unset/permissive (harness); runtime sets
    qint64 m_lastExternalId = -1;
    qint64 m_blockStartMonoMs = 0;   // block-elapsed projection anchor
    QString m_lastError;
    QString m_lastStartError;        // T1.1: user-facing start-failure reason
};

#endif // TA_TRAINING_PROGRAMCONTROLLER_H
