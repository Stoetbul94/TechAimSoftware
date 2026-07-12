#pragma once
// 3P FINAL — dedicated finals controller / state machine.
//
// Single time authority for the ISSF 50m Rifle 3P Final training simulation
// (ISSF Rule Book 2026 Edition 2025, Second Print 07/2026, effective
// 1 July 2026). Exposed to QML as the FINALS3P context property; the shooting
// page binds to it but never owns finals timing logic. Fully separate from
// qualification (is3PMatch) — see docs/3p-finals-discipline.md.
//
// Timing: one monotonic clock (QElapsedTimer); a fast UI tick recomputes
// remaining = duration - monotonicElapsed (never a per-second decrement).
// Developer-only timeScale accelerates the whole machine for dry runs/tests.

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QVariantMap>
#include <QVariantList>
#include <QStringList>

#include "Finals3PTypes.h"
#include "Finals3PConfig.h"
#include "Finals3PShotRecord.h"

class Finals3PController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int stageId READ stageId NOTIFY phaseChanged)
    Q_PROPERTY(QString stageLabel READ stageLabel NOTIFY phaseChanged)
    Q_PROPERTY(int stepIndex READ stepIndex NOTIFY phaseChanged)
    Q_PROPERTY(QString positionLabel READ positionLabel NOTIFY phaseChanged)
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
    Q_PROPERTY(QString advanceLabel READ advanceLabel NOTIFY advanceLabelChanged)
    // Contextual primary action for the bottom action bar (FIX2): the
    // controller owns legality and labelling; QML only renders and invokes.
    Q_PROPERTY(bool primaryActionVisible READ primaryActionVisible NOTIFY advanceLabelChanged)
    Q_PROPERTY(bool primaryActionEnabled READ primaryActionEnabled NOTIFY advanceLabelChanged)
    Q_PROPERTY(QString primaryActionLabel READ primaryActionLabel NOTIFY advanceLabelChanged)
    Q_PROPERTY(int officialShotCount READ officialShotCount NOTIFY shotCountsChanged)
    Q_PROPERTY(double cumulativeTotal READ cumulativeTotal NOTIFY totalsChanged)
    Q_PROPERTY(double stageSubtotal READ stageSubtotal NOTIFY totalsChanged)
    Q_PROPERTY(double timeScale READ timeScale WRITE setTimeScale NOTIFY timeScaleChanged)
    Q_PROPERTY(int ceremonyMode READ ceremonyMode WRITE setCeremonyMode NOTIFY configChanged)

public:
    explicit Finals3PController(QObject* parent = nullptr);

    // ── Main API (design doc §1; Phase-B plan §4) ────────────────────────
    Q_INVOKABLE void startFinal();
    Q_INVOKABLE void skipCeremony();
    // Stage-1 position transitions: ONE stage-aware action. The controller
    // decides which transition is legal; QML only ever calls advanceStage1().
    // Advancing below the stage shot limit first emits
    // advanceConfirmationRequired and waits for confirmStage1Advance().
    Q_INVOKABLE void advanceStage1();
    Q_INVOKABLE void executePrimaryAction();          // bottom-bar entry point
    // Developer-only: bypass the hard block for testing. Never wired to a
    // production control (FinalsDeveloperDrawer only).
    Q_INVOKABLE void devForceAdvanceStage1();
    // Retained for API compatibility; production advance is hard-blocked, so
    // these are inert (developer bypass is devForceAdvanceStage1).
    Q_INVOKABLE void confirmStage1Advance();
    Q_INVOKABLE void cancelStage1Advance();
    Q_INVOKABLE void registerShot(double xMm, double yMm, double decimalScore,
                                  int externalShotId = -1);
    Q_INVOKABLE void abortFinal();
    Q_INVOKABLE void resetFinal();
    Q_INVOKABLE void pauseTrainingSimulation();
    Q_INVOKABLE void resumeTrainingSimulation();

    // Phase A dry-run: same acceptance path as registerShot, no coordinates.
    Q_INVOKABLE void simulateShot();

    // Structured command history (FinalsCommandEvent maps, design doc §12).
    Q_INVOKABLE QVariantList commandEvents() const { return m_events; }
    Q_INVOKABLE QStringList stepLabels() const;
    // [P1 = Option B] Missing expected shots (report layer renders them as
    // DNS / 0.0 provisional); never entries in the detected-shot models.
    Q_INVOKABLE QVariantList missingShots() const { return m_missingShots; }
    // Complete role-schema template (all roles, default values). Appended once
    // and cleared at startup to LOCK the shared ListModels to the role union
    // before any qualification or finals append (plan §2 / role locking).
    Q_INVOKABLE QVariantMap templateShotRecord() const;

    // ── RMS hooks (stubs until the Range Management System exists) ──────
    Q_INVOKABLE void startPhaseFromServer(const QVariantMap& command);
    Q_INVOKABLE void stopPhaseFromServer(const QVariantMap& command);
    Q_INVOKABLE void synchroniseClock(qint64 serverTimestamp, qint64 offset);
    Q_INVOKABLE void applyAuthoritativeState(const QVariantMap& snapshot);

    // ── Property getters ─────────────────────────────────────────────────
    int stageId() const { return static_cast<int>(m_stage); }
    QString stageLabel() const;
    int stepIndex() const;
    QString positionLabel() const;
    int targetMode() const { return static_cast<int>(m_targetMode); }
    int windowState() const { return static_cast<int>(m_window); }
    bool isFiringWindowOpen() const { return m_window != techaim::finals::WindowState::Closed; }
    qint64 remainingMs() const;
    QString remainingFormatted() const;
    qint64 elapsedMs() const;
    QString commandText() const { return m_commandText; }
    bool running() const;
    bool paused() const { return m_paused; }
    int shotsInStage() const { return m_shotsInStage; }
    int nextShotNumber() const;
    QString advanceLabel() const;
    bool primaryActionVisible() const;
    bool primaryActionEnabled() const;
    QString primaryActionLabel() const;
    int officialShotCount() const { return m_officialShotCount; }
    double cumulativeTotal() const { return m_cumulativeTotal; }
    double stageSubtotal() const { return m_stageSubtotal; }
    double timeScale() const { return m_timeScale; }
    void setTimeScale(double s);
    int ceremonyMode() const { return static_cast<int>(m_cfg.ceremonyMode); }
    void setCeremonyMode(int m);

signals:
    void phaseChanged();
    void advanceLabelChanged();
    void advanceConfirmationRequired(int shotsFired, int stageLimit);
    void transitionRejected(const QVariantMap& info);
    void missingShotRecorded(const QVariantMap& record);
    void totalsChanged();
    void commandIssued(const QVariantMap& event);
    void countdownChanged();
    void shotAccepted(const QVariantMap& shot);
    void shotRejected(const QVariantMap& rejection);
    void stageCompleted(int stageId);
    void finalCompleted();
    void targetModeChanged();
    void windowStateChanged();
    void pausedChanged();
    void shotCountsChanged();
    void timeScaleChanged();
    void configChanged();

private slots:
    void tick();

private:
    using Stage = techaim::finals::Stage;
    using WindowState = techaim::finals::WindowState;
    using TargetMode = techaim::finals::TargetMode;
    using CommandType = techaim::finals::CommandType;
    using CeremonyMode = techaim::finals::CeremonyMode;
    using RejectReason = techaim::finals::RejectReason;

    // Monotonic, pause-aware, timeScale-scaled milliseconds since startFinal().
    qint64 scaledNow() const;

    void enterStage(Stage s);
    void issueCommand(CommandType type, const QString& text);
    void setWindow(WindowState w);
    void applyTargetModeInternal(TargetMode m);
    void performStage1Advance();                      // stage-checked transition
    void openCommandWindow(qint64 durationMs);       // series/single firing window
    void beginCommandSequence(qint64 gapMs);          // gap -> LOAD -> START steps
    void closeWindowAndStop();
    void recordMissingShots();                        // [P1=B] shortfall -> MissingShot records
    void advanceAfterCommandStage();
    techaim::finals::ShotContext currentShotContext() const;
    void acceptShot(bool sighter, double xMm, double yMm, double score,
                    bool simulated, qint64 externalShotId);
    void rejectShot(RejectReason reason, double xMm, double yMm,
                    bool simulated, qint64 externalShotId);
    int stageShotLimit() const;
    int stageShotNumberBase() const;
    bool inStage1() const;
    bool isSeriesStage() const { return m_stage == Stage::StandingSeries1 || m_stage == Stage::StandingSeries2; }
    bool isSingleStage() const;
    void infoNoticeForCompletedStage(Stage s);

    techaim::finals::Finals3PConfig m_cfg;

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

    // Phase/segment timing (all in scaled ms since start).
    qint64 m_phaseStartScaled = 0;
    qint64 m_segmentEndScaled = 0;    // what the countdown shows
    int m_seqStep = 0;                // command stages: 0 gap, 1 loaded, 2 firing
    bool m_warn1Fired = false;        // per-phase one-shot milestones
    bool m_warn2Fired = false;

    // Stage-1 shared 22:00 clock.
    bool m_stage1Started = false;
    qint64 m_stage1StartScaled = 0;
    bool m_s1Warn1 = false, m_s1Warn2 = false;

    // Shots. The controller itself never writes models; it validates and
    // emits complete records (the QML router appends — Phase-B plan §1).
    int m_shotsInStage = 0;
    int m_kneelingFired = 0, m_proneFired = 0;
    int m_windowId = 0;
    quint64 m_shotEventId = 0;
    bool m_pendingAdvance = false;
    qint64 m_lastExternalId = -1;      // per-window duplicate guard
    qint64 m_windowOpenedScaled = 0;
    qint64 m_lastAcceptScaled = 0;
    QVariantList m_missingShots;
    // Decimal totals over ACCEPTED official shots only (plan §5); the model
    // sum is the cross-check, this is the incremental source for the UI.
    double m_cumulativeTotal = 0.0;
    double m_stageSubtotal = 0.0;
    int m_officialShotCount = 0;

    // Append-only session journal (plan §9): one JSON line per accepted shot
    // and phase transition. Crash-safe; restart-recovery UI deferred.
    void archiveExistingJournal();
    void writeJournal(const QString& type, const QVariantMap& payload);
    bool m_journalEnabled = true;

    // Command events.
    int m_commandSeq = 0;
    QString m_commandText;
    QVariantList m_events;
};
