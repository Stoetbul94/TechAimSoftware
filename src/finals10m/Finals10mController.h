#pragma once
// 10m Air Rifle / Air Pistol FINAL — dedicated single-athlete finals
// controller / state machine (F1).
//
// Single time authority for the ISSF 10m individual Final training course
// (rule 6.17.2; docs/issf-rules/10m-finals-shared.md). One athlete on one
// TechAim target fires the full 24-shot course of fire (2×5 series + 14
// singles), decimal scoring, command-driven timing. Exposed to QML as the
// FINALS10M context property (F2).
//
// Reuses the discipline-agnostic Session Reliability Layer (SessionStore,
// typed events, reducer-owned SessionState, RecoveryCoordinator) and the
// 3P controller's timing/command PATTERNS — but shares NO 3P stages, positions
// or durations, and is a NEW controller (not derived from Finals3PController).
//
// Multi-athlete ranking / eliminations / ties / shoot-offs are OUT OF SCOPE
// here (future Finals10mCompetitionCoordinator). The elimination points are
// single-athlete course CHECKPOINTS only — no finishing place is ever assigned.
//
// Timing: one monotonic pause-aware clock (QElapsedTimer); a fast UI tick
// recomputes remaining = duration − elapsed. TECHAIM_FINALS_TIMESCALE
// accelerates the whole machine for dry runs/tests.

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QVariantMap>
#include <QVariantList>
#include <QStringList>
#include <QHash>

#include <memory>

#include "Finals10mTypes.h"
#include "Finals10mConfig.h"

#include "reliability/store/SessionStore.h"
#include "reliability/recovery/RecoveryCoordinator.h"

class Finals10mController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int stageId READ stageId NOTIFY phaseChanged)
    Q_PROPERTY(QString stageLabel READ stageLabel NOTIFY phaseChanged)
    Q_PROPERTY(int stepIndex READ stepIndex NOTIFY phaseChanged)
    Q_PROPERTY(int targetMode READ targetMode NOTIFY targetModeChanged)
    Q_PROPERTY(int windowState READ windowState NOTIFY windowStateChanged)
    Q_PROPERTY(bool isFiringWindowOpen READ isFiringWindowOpen NOTIFY windowStateChanged)
    Q_PROPERTY(qint64 remainingMs READ remainingMs NOTIFY countdownChanged)
    Q_PROPERTY(QString remainingFormatted READ remainingFormatted NOTIFY countdownChanged)
    Q_PROPERTY(qint64 elapsedMs READ elapsedMs NOTIFY countdownChanged)
    Q_PROPERTY(QString commandText READ commandText NOTIFY commandIssued)
    Q_PROPERTY(bool running READ running NOTIFY phaseChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(int shotsInStage READ shotsInStage NOTIFY shotCountsChanged)
    Q_PROPERTY(int nextShotNumber READ nextShotNumber NOTIFY shotCountsChanged)
    Q_PROPERTY(int officialShotCount READ officialShotCount NOTIFY shotCountsChanged)
    Q_PROPERTY(int maximumMatchShots READ maximumMatchShots NOTIFY configChanged)
    Q_PROPERTY(int stageShotCapacity READ stageShotLimit NOTIFY phaseChanged)
    Q_PROPERTY(double cumulativeTotal READ cumulativeTotal NOTIFY totalsChanged)
    Q_PROPERTY(QString checkpointLabel READ checkpointLabel NOTIFY totalsChanged)
    // F7 athlete-display projections (read-only, derived — never a second
    // authoritative source). Last accepted shot for the right-hand command
    // panel, and live per-segment subtotals for the score summary.
    // F9: LAST SHOT projections are the last OFFICIAL shot only (— when none) —
    // a sighter must never appear as the Final's last official result.
    Q_PROPERTY(double lastShotScore READ lastShotScore NOTIFY lastShotChanged)
    Q_PROPERTY(int lastShotNumber READ lastShotNumber NOTIFY lastShotChanged)
    Q_PROPERTY(int lastShotTimeSec READ lastShotTimeSec NOTIFY lastShotChanged)
    Q_PROPERTY(double series1Subtotal READ series1Subtotal NOTIFY totalsChanged)
    Q_PROPERTY(double series2Subtotal READ series2Subtotal NOTIFY totalsChanged)
    Q_PROPERTY(double singlesSubtotal READ singlesSubtotal NOTIFY totalsChanged)
    Q_PROPERTY(int seriesNumber READ seriesNumber NOTIFY phaseChanged)
    Q_PROPERTY(int singleIndex READ singleIndex NOTIFY phaseChanged)
    Q_PROPERTY(QString disciplineId READ disciplineId NOTIFY configChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY configChanged)
    Q_PROPERTY(int targetType READ targetType NOTIFY configChanged)
    // F9: completion / session identity for the exit workflow + report.
    Q_PROPERTY(bool complete READ complete NOTIFY phaseChanged)
    Q_PROPERTY(int missingShotCount READ missingShotCount NOTIFY totalsChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY configChanged)
    Q_PROPERTY(bool primaryActionVisible READ primaryActionVisible NOTIFY advanceLabelChanged)
    Q_PROPERTY(bool primaryActionEnabled READ primaryActionEnabled NOTIFY advanceLabelChanged)
    Q_PROPERTY(QString primaryActionLabel READ primaryActionLabel NOTIFY advanceLabelChanged)
    Q_PROPERTY(double timeScale READ timeScale WRITE setTimeScale NOTIFY timeScaleChanged)
    Q_PROPERTY(int ceremonyMode READ ceremonyMode WRITE setCeremonyMode NOTIFY configChanged)
    Q_PROPERTY(int persistenceHealth READ persistenceHealth NOTIFY persistenceHealthChanged)
    Q_PROPERTY(QString athleteName READ athleteName WRITE setAthleteName NOTIFY configChanged)

public:
    explicit Finals10mController(QObject* parent = nullptr);

    // ── configuration ────────────────────────────────────────────────────
    // Choose the discipline before startFinal(). "FINAL_AR10" / "FINAL_AP10".
    Q_INVOKABLE void configureDiscipline(const QString& disciplineId);

    // ── main API ─────────────────────────────────────────────────────────
    Q_INVOKABLE void startFinal();
    Q_INVOKABLE void skipCeremony();
    Q_INVOKABLE void registerShot(double xMm, double yMm, double decimalScore,
                                  int externalShotId = -1,
                                  double direction = 0.0);
    Q_INVOKABLE void simulateShot();       // dry-run: same path, no coordinates
    Q_INVOKABLE void executePrimaryAction();
    Q_INVOKABLE void abortFinal();
    Q_INVOKABLE void resetFinal();
    Q_INVOKABLE void pauseTrainingSimulation();
    Q_INVOKABLE void resumeTrainingSimulation();

    Q_INVOKABLE QVariantList commandEvents() const { return m_events; }
    Q_INVOKABLE QStringList stepLabels() const;
    Q_INVOKABLE QVariantList missingShots() const { return m_missingShots; }
    Q_INVOKABLE QVariantList officialShotRecords() const { return m_officialShotRecords; }
    Q_INVOKABLE QVariantList rejectionRecords() const { return m_rejectionRecords; }
    Q_INVOKABLE int sighterCount() const { return m_sighterCount; }
    // Per-checkpoint cumulative totals (decimal) — {12: 123.4, ...}. Course
    // checkpoints only; never a placement (single-athlete training).
    Q_INVOKABLE QVariantMap checkpointTotals() const;
    Q_INVOKABLE QVariantList seriesSubtotals() const;

    // ── property getters ─────────────────────────────────────────────────
    int stageId() const { return static_cast<int>(m_stage); }
    QString stageLabel() const;
    int stepIndex() const;
    int targetMode() const { return static_cast<int>(m_targetMode); }
    int windowState() const { return static_cast<int>(m_window); }
    bool isFiringWindowOpen() const { return m_window != techaim::finals10m::WindowState::Closed; }
    qint64 remainingMs() const;
    QString remainingFormatted() const;
    qint64 elapsedMs() const;
    QString commandText() const { return m_commandText; }
    bool running() const;
    bool paused() const { return m_paused; }
    int shotsInStage() const { return m_shotsInStage; }
    int nextShotNumber() const;
    int officialShotCount() const { return m_officialShotCount; }
    int maximumMatchShots() const { return m_cfg.maximumMatchShots; }
    double cumulativeTotal() const { return m_cumulativeTotal; }
    QString checkpointLabel() const { return m_lastCheckpointLabel; }
    double lastShotScore() const { return m_lastShotScore; }
    int lastShotNumber() const { return m_lastShotNumber; }
    int lastShotTimeSec() const { return m_lastShotTimeSec; }
    double series1Subtotal() const { return m_seriesSubtotal.value(2, 0.0); }
    double series2Subtotal() const { return m_seriesSubtotal.value(3, 0.0); }
    double singlesSubtotal() const;
    bool complete() const { return m_stage == Stage::Complete; }
    int missingShotCount() const { return m_cfg.maximumMatchShots - m_officialShotCount; }
    QString sessionId() const;   // active/completed session id (empty if none)
    // True while an unresolved EST incident blocks official shots (Phase-E
    // authority model). Polled by the command panel on INCIDENTS.incidentChanged.
    Q_INVOKABLE bool officialsBlockedNow() const;
    // F9 exit workflow: cleanly close the completed (or active) session so it is
    // NOT offered as an unfinished recovery candidate. Idempotent — appends at
    // most one clean close; never a second MatchCompleted.
    Q_INVOKABLE void closeFinalSession();
    int seriesNumber() const;
    int singleIndex() const { return m_singleIndex; }
    QString disciplineId() const;
    QString displayName() const { return m_cfg.displayName; }
    int targetType() const { return m_cfg.targetType; }
    bool primaryActionVisible() const;
    bool primaryActionEnabled() const;
    QString primaryActionLabel() const;
    double timeScale() const { return m_timeScale; }
    void setTimeScale(double s);
    int ceremonyMode() const { return static_cast<int>(m_cfg.ceremonyMode); }
    void setCeremonyMode(int m);
    QString athleteName() const { return m_athleteName; }
    void setAthleteName(const QString& n)
        { if (n != m_athleteName) { m_athleteName = n; emit configChanged(); } }

    // ── Session Reliability Layer (F1) ───────────────────────────────────
    ta::rel::SessionStore* sessionStore() { return m_store.get(); }
    ta::rel::SessionStore* store() { return m_store.get(); }   // Phase-E gate
    QString sessionJournalPath() const;
    int persistenceHealth() const;

    // ── recovery (F3/F5) ─────────────────────────────────────────────────
    void loadRecoveredState(const ta::rel::RecoveredMatchState& recovered);
    Q_INVOKABLE QVariantList scanForRecovery();
    Q_INVOKABLE bool resumeFromRecovery(const QString& sessionId);
    Q_INVOKABLE void discardRecovery(const QString& sessionId);

signals:
    void phaseChanged();
    void advanceLabelChanged();
    void totalsChanged();
    void commandIssued(const QVariantMap& event);
    void countdownChanged();
    void shotAccepted(const QVariantMap& shot);
    void shotRejected(const QVariantMap& rejection);
    void stageCompleted(int stageId);
    void checkpointReached(int shotNumber, const QString& label, double total);
    void lastShotChanged();
    void finalCompleted();
    void reportRequested();
    void missingShotRecorded(const QVariantMap& record);
    void journalWriteFailed(QString path, QString detail);
    void persistenceHealthChanged(int health);
    void targetModeChanged();
    void windowStateChanged();
    void pausedChanged();
    void shotCountsChanged();
    void timeScaleChanged();
    void configChanged();

private slots:
    void tick();

private:
    using Stage = techaim::finals10m::Stage;
    using WindowState = techaim::finals10m::WindowState;
    using TargetMode = techaim::finals10m::TargetMode;
    using CommandType = techaim::finals10m::CommandType;
    using CeremonyMode = techaim::finals10m::CeremonyMode;
    using StageStatus = techaim::finals10m::StageStatus;
    using RejectReason = techaim::finals10m::RejectReason;

    qint64 scaledNow() const;

    void enterStage(Stage s);
    void enterSingle(int index);                 // Singles stage, index 1..14
    void issueCommand(CommandType type, const QString& text);
    void setWindow(WindowState w);
    void applyTargetModeInternal(TargetMode m);
    void beginCommandSequence(qint64 gapMs);
    void openCommandWindow(qint64 durationMs);
    void closeWindowAndStop();
    void advanceAfterCommandStage();
    void recordMissingShots();
    void setStageStatus(int fineStageId, StageStatus st);
    void acceptShot(bool sighter, double xMm, double yMm, double score,
                    bool simulated, qint64 externalShotId, double direction = 0.0);
    void rejectShot(RejectReason reason, double xMm, double yMm,
                    bool simulated, qint64 externalShotId);
    void emitCheckpointIfDue(int completedShotNumber);

    int stageShotLimit() const;
    int stageShotNumberBase() const;
    int currentFineStageId() const;
    bool isSeriesStage() const
        { return m_stage == Stage::Series1 || m_stage == Stage::Series2; }
    bool isSingleStage() const { return m_stage == Stage::Singles; }

    // reliability
    void beginJournalSession();
    void submitEvent(const ta::rel::DomainEvent& event);
    void submitStagePhase(Stage s);
    void restoreStageFiringState(qint64 elapsedScaledMs);
    ta::rel::ShotCore buildShotCore(double xMm, double yMm, double score,
                                    int finalNumber, int withinStage,
                                    qint64 externalShotId, double direction,
                                    bool simulated) const;
    QVariantMap makeShotRecord(bool sighter, double xMm, double yMm,
                               double score, int finalNumber, int withinStage,
                               qint64 externalShotId, double direction,
                               bool simulated, quint64 eventId);

    techaim::finals10m::Finals10mConfig m_cfg;
    QString m_athleteName;

    QTimer m_tick;
    QElapsedTimer m_mono;
    bool m_monoStarted = false;
    bool m_paused = false;
    qint64 m_pausedTotalRaw = 0;
    qint64 m_pauseStartRaw = 0;
    double m_timeScale = 1.0;

    Stage m_stage = Stage::Idle;
    WindowState m_window = WindowState::Closed;
    TargetMode m_targetMode = TargetMode::Sighter;
    int m_singleIndex = 0;                        // 1..14 while in Singles

    qint64 m_phaseStartScaled = 0;
    qint64 m_segmentEndScaled = 0;
    int m_seqStep = 0;                            // 0 gap, 1 loaded, 2 firing
    bool m_warn1Fired = false;

    int m_shotsInStage = 0;
    int m_windowId = 0;
    quint64 m_shotEventId = 0;
    qint64 m_lastExternalId = -1;
    qint64 m_windowOpenedScaled = 0;
    qint64 m_lastAcceptScaled = 0;

    QVariantList m_missingShots;
    QVariantList m_officialShotRecords;
    QVariantList m_rejectionRecords;
    int m_sighterCount = 0;
    double m_cumulativeTotal = 0.0;
    int m_officialShotCount = 0;
    // F7 last-accepted-shot projection (right-hand command panel).
    double m_lastShotScore = -1.0;      // -1 = no official shot yet
    int m_lastShotNumber = 0;           // official number (0 = none)
    int m_lastShotTimeSec = 0;
    QString m_lastCheckpointLabel;
    QHash<int, double> m_checkpointTotals;        // shotNumber -> cumulative
    QHash<int, double> m_seriesSubtotal;          // fineStageId -> subtotal
    QHash<int, int> m_stageStatus;                // fineStageId -> StageStatus

    std::unique_ptr<ta::rel::SessionStore> m_store;
    std::unique_ptr<ta::rel::RecoveryCoordinator> m_recovery;
    bool m_journalFailureNotified = false;

    int m_commandSeq = 0;
    QString m_commandText;
    QVariantList m_events;
};
