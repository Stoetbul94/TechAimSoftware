#ifndef TACHUSWIDGET_H
#define TACHUSWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QThread>

#include "target/TargetDeviceFingerprint.h"
#include "target/PaperFeedCoordinator.h"
#include "target/SerialDeviceProvider.h"
#include "target/AcquisitionSequencer.h"
#include <QDebug>
#include <QTcpServer>
#include <QElapsedTimer>

#include "../src/mainwindow.h"
#include "logfile.h"

#define FLUSH_SHOOT_COUNT 10
using namespace std;

// This pair is used to store the X and Y
// coordinate of a point respectively
#define pdd pair<double, double>

namespace Ui {
class TachusWidget;
}

class MotorThread : public QThread
{
public:
    explicit MotorThread(MainWindow* mainWindow, QObject* parent = 0)
        : m_mainWindow(mainWindow), QThread(parent)
    {
    }
    ~MotorThread() {

    }
    void setMotorMovementTime(double time) {
        motor_movement_time = time;
    }

protected:
    void run() override {
        startMotor();
        QThread::msleep(motor_movement_time*1000);
        stopMotor();
    }

private:
    void startMotor() {
        //start motor
        LogFile::instance().appendToLogFile("Send motor movement signal", LogType::BackendLevel);

        m_mainWindow->modbusWriteSingleRegister(8196, 32768);
        QThread::msleep(100);

        //while loop to check, motor status - bounded by MOTOR_STATUS_TIMEOUT_MS
        //so a non-responding motor controller can't hang this thread forever
        bool motorStarted = false;
        QElapsedTimer timeoutTimer;
        timeoutTimer.start();
        while (!motorStarted)
        {
            if (timeoutTimer.hasExpired(MOTOR_STATUS_TIMEOUT_MS)) {
                LogFile::instance().appendToLogFile(
                    QString("Motor start status timed out after %1 ms - giving up, motor may not have started")
                        .arg(MOTOR_STATUS_TIMEOUT_MS), LogType::BackendLevel);
                break;
            }

            uint8_t dest[1024]; //setup memory for data
            uint16_t * dest16 = (uint16_t *) dest;
            memset(dest, 0, 1024);


            m_mainWindow->modbusReadRegistry(8196, 2, dest16);
            motorStarted = dest16[0] == 32768 ? true : false;
            LogFile::instance().appendToLogFile(motorStarted ? QString("Reading motor status -> started") :
                                                             QString("Reading motor status -> not-started"), LogType::BackendLevel);
//            QMessageBox msgBox;
//            msgBox.setText(QString("%1 From Start - is motor running %2").arg(dest16[0]).arg(dest16[1]));
//            msgBox.exec();
        }

    }

    void stopMotor() {
        // stop motor
        LogFile::instance().appendToLogFile("Send motor stop signal", LogType::BackendLevel);
        QThread::msleep(100);

        m_mainWindow->modbusWriteSingleRegister(8196, 0);
        //while loop to check, motor status - bounded by MOTOR_STATUS_TIMEOUT_MS,
        //same reasoning as startMotor() above
        bool motorStoped = false;
        QElapsedTimer timeoutTimer;
        timeoutTimer.start();
        while (!motorStoped)
        {
            if (timeoutTimer.hasExpired(MOTOR_STATUS_TIMEOUT_MS)) {
                LogFile::instance().appendToLogFile(
                    QString("Motor stop status timed out after %1 ms - giving up, motor may not have stopped")
                        .arg(MOTOR_STATUS_TIMEOUT_MS), LogType::BackendLevel);
                break;
            }

            uint8_t dest[1024]; //setup memory for data
            uint16_t * dest16 = (uint16_t *) dest;
            memset(dest, 0, 1024);

            m_mainWindow->modbusReadRegistry(8196, 2, dest16);
            motorStoped = (dest16[0] == 0) ? true : false;
            LogFile::instance().appendToLogFile(motorStoped ? QString("Reading motor status -> stoped") :
                                                        QString("Reading motor status -> not-stoped"), LogType::BackendLevel);

//            QMessageBox msgBox;
//            msgBox.setText(QString("%1 From Stop - is motor running %2").arg(dest16[0]).arg(dest16[1]));
//            msgBox.exec();
        }
    }
private:
    static const int MOTOR_STATUS_TIMEOUT_MS = 5000;
    MainWindow* m_mainWindow;
    double motor_movement_time = 0;
//    TachusWidget* tachusWidget = NULL;
};

// ACQ-FLUSH-001. THE DEFERRED HARDWARE RESET IS GONE.
//
// A WorkerThread used to live here. Its entire body was:
//
//     QThread::msleep(2600);
//     m_mainWindow->modbusWriteSingleRegister(8193, 0);
//
// The flush zeroed the APPLICATION baseline at once and started that thread to
// zero the HARDWARE counter 2.6 seconds later. The 100 ms acquisition poll ran
// about 26 times in between, read baseline 0 against target 10, and correctly
// concluded that ten shots had been missed - so it stopped acquisition. The
// four-tablet evidence of 2026-08-23 shows that firing 12 times out of 12,
// always 57-156 ms after the reset and always ~2.70 s before the hardware
// caught up. It also wrote Modbus from a second thread while the poll thread
// was reading it (THREAD-MODBUS-006).
//
// The reset is now issued on the poll thread and CONFIRMED by reading the
// counter back; the application baseline moves only on that proof, so no
// interval exists in which the two disagree by accident. See
// src/target/AcquisitionSequencer.h.

class TachusWidget;


////////////////////////////////
/// \brief The TachusWidget class
///
////////////////////////////////

class TachusWidget : public QWidget
{
    Q_OBJECT

    // ── AUTHORITATIVE TARGET STATUS, for the operator ────────────────────
    // The single source the UI binds to. Every state the acquisition engine
    // reaches is published here, so a discipline screen consumes target state
    // rather than reimplementing it.
    //
    // targetDevice is the description of the device ACTUALLY enumerated and
    // selected - it is not a hardcoded adapter name. Any USB-serial target is
    // shown by whatever Windows reports for it.
    //
    // targetPort is the port CURRENTLY connected, never a remembered settings
    // value. The connection area displayed a stale "COM7" that did not exist on
    // the machine; that is the defect this property exists to prevent.
    Q_PROPERTY(QString targetState  READ targetState  NOTIFY targetStatusChanged)
    Q_PROPERTY(QString targetDevice READ targetDevice NOTIFY targetStatusChanged)
    Q_PROPERTY(QString targetPort   READ targetPort   NOTIFY targetStatusChanged)
    Q_PROPERTY(QString targetDetail READ targetDetail NOTIFY targetStatusChanged)
    Q_PROPERTY(bool    targetReady  READ targetReady  NOTIFY targetStatusChanged)

public:
    QString targetState()  const { return m_targetState; }
    QString targetDevice() const { return m_targetDevice; }
    QString targetPort()   const { return m_targetPort; }
    QString targetDetail() const { return m_targetDetail; }
    // READY is a strong claim: identified target, open transport, valid Modbus
    // AND a synchronized acquisition baseline. An open COM port is not enough.
    bool    targetReady()  const {
        return m_targetState == QLatin1String("TARGET CONNECTED")
            && m_acqState == AcquisitionState::Acquiring;
    }

public:
    explicit TachusWidget(MainWindow* mainwindow, QWidget *parent = 0);
    ~TachusWidget();
    void initialiseConnection();
    void setMotorMovementTime(double time, double sighterMotorTime) {
        if (m_motorThread)
            m_motorThread->setMotorMovementTime(time);
        m_motor_movement_duration = time;
        m_motor_movement_duration_sighter = sighterMotorTime;
    }

    QString getIpAddress() const;

    void setIsMasterConnected(bool isMasterConnected);

    Q_INVOKABLE void setIsAppDemoMode(bool value);

    bool getOnLoginPage() const;

    QString getGermanDecimalNumber(QString data);
    int getGamemode() const;
    void setGamemode(int gamemode);

    int getCurrentMatchTotalShotsCount() const;

    Q_INVOKABLE int getGame_distance() const;
    void setGame_distance(int game_distance);

    Q_INVOKABLE int getGame_range() const;
    Q_INVOKABLE void setGame_range(int game_range);

    Q_INVOKABLE double getFormatedSCore(double value);
    double getFormatedValueFoeTwoDecimal(double value);

    QString getServerPath() const;
    void setServerPath(const QString &serverPath);

    QString getServerLaneFilePath() const;
    void setServerLaneFilePath(const QString &serverLaneFilePath);

    QString getLaneName() const;
    void setLaneName(const QString &laneName);

    QString getSetaServerPath() const;
    void setSetaServerPath(const QString &setaServerPath);

    QString getSetaServerSettingPath() const;
    void setSetaServerSettingPath(const QString &setaServerSettingPath);

    QString getSetaLaneStatusPath() const;
    void setSetaLaneStatusPath(const QString &setaLaneStatusPath);

    QString getSetaLaneShootDataFilePath() const;
    void setSetaLaneShootDataFilePath(const QString &setaLaneShootDataFilePath);
    Q_INVOKABLE void removeSetaLaneShootDataFile();
    void removeAllShootdatForThisLane();


    QString getSetaLaneScoreSummaryFilePath() const;
    void setSetaLaneScoreSummaryFilePath(const QString &setaLaneScoreSummaryFilePath);

    double getMatch_distance_new() const;
    void setMatch_distance_new(double match_distance_new);
    QString getSetaLaneEachScoreDataFilePath() const;
	
    QStringList getPDFString();
    QStringList getSeriesComparisionData();
    QStringList getShotIntervalData();
    QStringList getTimeSeriesData();
    QStringList getZoneTableData();

    int getShotPerSeries() const;

    bool getIsAppDemoMode() const;
	int getSeries_start_at() const;
    void setSeries_start_at(int series_start_at);

    int getSeries_end_at() const;
    void setSeries_end_at(int series_end_at);
	
	    int getShot_interval() const;
    void setShot_interval(int shot_interval);

    double getGreen_zone_start() const;
    void setGreen_zone_start(double green_zone_start);

    double getGreen_zone_end() const;
    void setGreen_zone_end(double green_zone_end);

    double getYellow_zone_start() const;
    void setYellow_zone_start(double yellow_zone_start);

    double getYellow_zone_end() const;
    void setYellow_zone_end(double yellow_zone_end);

    double getRed_zone_end() const;
    void setRed_zone_end(double red_zone_end);

    double getRed_zone_start() const;
    void setRed_zone_start(double red_zone_start);
	
    void setSetaLaneEachScoreDataFilePath(const QString &setaLaneEachScoreDataFilePath);

public slots:
    bool getIsServerNetworkEnabled() const;
    void setIsServerNetworkEnabled(bool isServerNetworkEnabled);
    bool getIsSingleDecimal() const;
    void setIsSingleDecimal(bool isSingleDecimal);
    void setShotPerSeries(int shotPerSeries);
    void setOnLoginPage(bool onLoginPage);
    bool isModBusConnected();
    bool isHardwareConnected();
    bool isMasterSystemConnected();
    bool connectedModbus(QString portName = QString());
    int validateLicence(QString mail);
    bool disconnectModbus();
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    bool isValidLicence();
    void uxShoot(double xCor, double yCor);
    // INVARIANT A (ACQ-DESYNC-002). The logical shot count is the number of
    // coordinates actually captured - the only count that can be PROVED from
    // data rather than believed from bookkeeping.
    //
    // It used to be m_oldResetCount + m_currentShootsCount. On 2026-08-23 a
    // reconnect adopted "target reports 1" while ten coordinates were held;
    // that sum then answered 11, 12, 13 while the arrays held 10, 11, 12, and
    // every later shot read one index past the end. getXCord() returned its -1
    // sentinel and -1.00/-1.00 mm scored 10.8 for the rest of three sessions.
    // Deriving the count from the data makes that arithmetic impossible.
    int getShootCount() {
        return m_xCordList.count();
    }

    double getTime(int index);
    QString getTimeStamp(int index);
    // A coordinate exists for this shot number. ACQ-SENTINEL-003: ask this, do
    // not recognise a magic return value.
    //
    // PUBLIC and Q_INVOKABLE, and both matter. QML is the layer that turns a
    // coordinate into a score, so it is the layer that has to be able to ask
    // before it does. Declared private - which is where it started - moc still
    // registers it and QML still refuses it at runtime with "is not a
    // function", which aborts the calling handler. The guard would have been
    // not merely inert but actively harmful: onShootCountChanged would have
    // stopped before drawing or scoring any shot at all.
    Q_INVOKABLE bool coordinateHasValue(int index) const;
    double getXCord(int index);
    double getXMPI(int series = -1);
    double getGroup(int pageIndex, bool withPalletOffset = true);
    double getGroupFromList(QList<double> xList, QList<double> yList);
    double getGroup_1(int pageIndex);
    double getXGroup();
    double getYGroup();
    void lineFromPoints(pdd P, pdd Q, double &a,
                        double &b, double &c);
    void perpendicularBisectorFromLine(pdd P, pdd Q,
                                       double &a, double &b, double &c);
    pdd lineLineIntersection(double a1, double b1, double c1,
                              double a2, double b2, double c2);
    pdd findCircumCenter(pdd P, pdd Q, pdd R);

    bool inBoundAllPoints(pdd center, double dia, int startIndex, int endIndex);
    double getXMPIForShoot(int series, int shootNumber);
    double getYCord(int index);
    double getYMPI(int series = -1);
    double getYMPIForShoot(int series, int shootNumber);
    double getTeiler(int series = -1);
    double getTeilerForShoot(int series, int shootNumber = -1);
    double getTeilerForShootOfMatch(int shootNumber);
    double getScore(int index);
    void setScore(double value);
    void initiateMotorMovement();

    // ── RC2 target discovery + paper feed ─────────────────────────────────
    // Selection and fingerprinting live in src/target and are unit-tested
    // without hardware; these only act on the decision.
    ta::target::SelectionResult autoSelectTargetDevice();
    // Decides the port for the AUTOMATIC path. Empty means "nothing
    // confident" - the caller must NOT then connect speculatively.
    QString chooseStartupPort();
    ta::target::TargetDeviceFingerprint rememberedTargetDevice() const;
    void rememberTargetDevice(const ta::target::SerialDeviceInfo &d);
    Q_INVOKABLE void forgetTargetDevice();
    Q_INVOKABLE QVariantList targetCandidates();
    // Called once per ACCEPTED, durably recorded physical shot. This is the
    // only automatic route to the motor; QML must not call the motor itself.
    void onPhysicalShotAccepted(qint64 shotIdentity, bool isSighter);
    void setReplayInProgress(bool replaying) { m_replayInProgress = replaying; }
    void bindFeedCoordinator();
    // RC2a: correlated per-shot stage trace. Cheap by construction.
    void traceShotStage(const char* stage, qint64 seq, const QString& detail = QString());
    Q_INVOKABLE void traceShotStageFromQml(QString stage, int seq)
    { traceShotStage(stage.toLatin1().constData(), seq); }
    Q_INVOKABLE void setTraceSessionTag(QString tag) { m_traceSessionTag = tag; }
    void resetShootinCount() {
        // Only touch the hardware shot counter in LIVE mode. In demo mode a COM
        // port may be open with no target answering (e.g. a spare/virtual port);
        // the blocking retries below would then freeze the GUI on the demo path
        // (login -> shooting page). Demo shots come from the UI, so the reset is
        // pointless there. (isAppDemoMode is really "is live" — set from appMode.)
        // Bounded retry: with the modbus response timeout in place this write
        // returns within ~1s per attempt instead of freezing forever; retry
        // because a failed reset desyncs the hardware shot counter.
        if (isAppDemoMode) {
            for (int attempt = 0; attempt < 3; ++attempt) {
                if (m_mainWindow->modbusWriteSingleRegister(8193, 0) != -1)
                    break;
                LogFile::instance().appendToLogFile(
                    QString("resetShootinCount: hw counter reset failed (attempt %1)").arg(attempt + 1),
                    LogType::BackendLevel);
            }
        }
        // FALSE-SHOT-001. Do NOT assume the hardware counter is now zero.
        //
        // On 2026-08-08 the reset above reported success, yet 25 s later the
        // target still answered "1" and the application decoded the PREVIOUS
        // session's shot (x=2.5 y=2.9, identical to a shot fired 26 minutes
        // earlier) as a brand-new one. It consumed sequence 1 and its paper
        // feed, so the athlete's real first shot collided with it and its feed
        // was suppressed as a duplicate.
        //
        // Trusting a write we cannot verify is what created a phantom shot, so
        // read the counter back and ADOPT whatever it really is as the
        // baseline. This cannot hide a real shot: a genuine shot always pushes
        // the counter ABOVE the baseline, and detection is
        // `newShotsCount > m_currentShootsCount`. It only makes pre-existing
        // residue invisible, which is exactly what it is.
        // RC2g-DIAG: this whole path was invisible. The reset only logged on
        // FAILURE and the readback only logged a NON-ZERO result, so a
        // successful reset produced no trace at all - which is why three root
        // cause theories about it survived as long as they did.
        LogFile::instance().appendToLogFile(
            QStringLiteral("ACQDIAG resetShootinCount ENTER baselineBefore=%1 "
                           "liveFlag=%2 modbusConnected=%3 onLoginPage=%4")
                .arg(m_seq.hardwareBaseline())
                .arg(isAppDemoMode ? 1 : 0)     // NOTE: this flag means "is LIVE"
                .arg(m_mainWindow && m_mainWindow->isModBusConnected() ? 1 : 0)
                .arg(m_onLoginPage ? 1 : 0), LogType::BackendLevel);

        if (isAppDemoMode) {            // "is live" - the name is inverted
            uint8_t probe[64];
            uint16_t* probe16 = (uint16_t*) probe;
            memset(probe, 0, sizeof(probe));
            const int probeRc = m_mainWindow->modbusReadRegistry(8192, 2, probe16);
            LogFile::instance().appendToLogFile(
                QStringLiteral("ACQDIAG resetShootinCount READBACK rc=%1 hwCounter=%2")
                    .arg(probeRc).arg(probeRc < 0 ? -1 : int(probe16[1])),
                LogType::BackendLevel);
            if (probeRc != -1) {
                const int actual = probe16[1];
                if (actual != 0) {
                    // FALSE-SHOT-001. Do NOT assume the hardware counter is now
                    // zero. On 2026-08-08 the reset reported success, yet 25 s
                    // later the target still answered 1 and the application
                    // decoded the PREVIOUS session's shot as a brand-new one.
                    // The baseline is not written here: resetAll() below leaves
                    // the sequencer SYNCHRONIZING, and the next poll adopts
                    // whatever the target really reports. One adoption path,
                    // not two.
                    LogFile::instance().appendToLogFile(
                        QString("resetShootinCount: hw counter still reads %1 after reset - "
                                "the next poll will adopt it, so residue is not decoded as a shot")
                            .arg(actual), LogType::BackendLevel);
                }
            } else {
                LogFile::instance().appendToLogFile(
                    QString("resetShootinCount: could not read back the hw counter; "
                            "the next poll will synchronize"), LogType::BackendLevel);
            }
        }

        // PAPER-FEED-002. THE CENTRAL RESET.
        // This function is the one authority that clears the shot count, so it
        // is the one place the feed coordinator must be told. RC2c notified it
        // from changeSighterMode only; the count also resets on Home ->
        // Practice, discard-and-restart, new match and recovery, and each of
        // those left the coordinator remembering identities that were about to
        // be reissued from 1. Notifying here covers every path by construction,
        // including ones added later.
        m_feed.noteShotNumberingReset(QStringLiteral("shot count reset"));

        // The baseline has just been cleared, so it is no longer known to agree
        // with the target. Re-enter SYNCHRONIZING and let the next poll adopt
        // the real hardware value. This is the single place every numbering
        // reset passes through - startup, Home -> Practice, sighter/match swap,
        // new match and recovery - so no caller can forget it.
        m_acqState = AcquisitionState::Synchronizing;

        // The sequencer holds the acquisition state machine; it must return to
        // a proved-empty state through the SAME central reset, or a later poll
        // would judge a fresh session against a stale baseline.
        m_seq.resetAll();

        m_xCordList.clear();
        m_yCordList.clear();
        m_xCordList_gameMode.clear();
        m_yCordList_gameMode.clear();
        m_xCordList_sighterMode.clear();
        m_yCordList_sighterMode.clear();
        m_scoreList_sighterMode.clear();
        m_scoreList_gameMode.clear();
        clearTimeStampAndTimeConsumed();
        clearShotDirection();
    }

    void intiateAutoMovementSetup();
    void intiateAutoMovementSighterSetup();
    bool checkAutoFeedMode(bool showPopup=true);
    void showMessage(QString string);
    void changeSighterMode(bool flag);
    void appendToLogFile(QString string, LogType type = LogType::UXLevel);
    void connectToMaster(QString laneName);
    void startTCP();
    void stopTCP();
    void attemptReconnection();
    void setCurrentMatchTotalShotsCount(int currentMatchTotalShotsCount);
    void saveNameAndPort(QString name, QString port, QString networkPath);
    QString getUserName();
    QString getPortNumber();
    QString getNetworkPath();
    void updateSetaShootSummaryData();
    void updateSetaEachShootData();
    void setTotalScoreWOD(int totalScoreWOD);
    void setTotalScoreWD(double totalScoreWD);
    void updateSeriesScore(int index, int value);
    void updateSeriesScoreWD(int index, double value);
    void appendTimeConsumed(QString data);
    void appendTimeStamp(QString data);
    void appendShotDirection(int direction);

private:
    // SYNC-001. The caller used to infer "new shots" from the baseline having
    // MOVED - it captured `from` before checkForNewShots() and `to` after, and
    // fetched everything between. Once synchronization could legitimately move
    // the baseline, that inference replayed stale slots as real shots: two
    // phantom shots, two scores and two paper feeds on 2026-08-09.
    //
    // The poll now states WHAT happened instead of leaving the caller to guess
    // from a side effect. Only NewShots may enter the coordinate-fetch loop.
    // What ONE counter read produced. It carries no judgement: the meaning of
    // the number belongs to ta::target::AcquisitionSequencer, which the harness
    // drives directly. Splitting the read from the decision is what let the
    // 10-shot flush race and the reconnect desynchronisation be tested at all.
    struct CounterRead {
        bool ok = false;        // false => transport failure, already handled
        int  counter = 0;       // the target shot counter, when ok
    };
    CounterRead readShotCounter();
    const char* acquisitionStateName() const;

    // stopAcquisition=false for REPORT paths: the same full diagnostic is
    // written, but nothing is being acquired there, so the operator must not
    // be shown an acquisition fault for opening a result sheet.
    void reportCoordinateIndexInvalid(const char* who, int index,
                                      bool stopAcquisition = true);

    // Sequencer outcomes, each with the diagnostics the operator needs.
    void issueCounterReset(const ta::target::SeqStep& step);
    void reportAcquisitionFault(const ta::target::SeqStep& step);
    void reportSynchronized(const ta::target::SeqStep& step);

    // checkForNewShots() is gone. It read the counter AND decided what the
    // reading meant AND moved the baseline, inside a QWidget slot no test
    // could reach. readShotCounter() above does the read;
    // ta::target::AcquisitionSequencer makes every decision.

private slots:
    void on_pushButton_3_clicked();
    int getRealValue(int value);
    void broadCastNewShoot(int count);
    void updateShootData(int count);
    void updateSetaShootData(int count);
    void clearTimeStampAndTimeConsumed();
    void clearShotDirection();

private:
    void clearShootCount();
    QString getEncryptedText(QString data, bool onlyDefault=false);
    QString getDencryptedText(QString data, QString encryptionText, bool onlyDefault=false);
    void licValidated();

signals:
    // TechAim dialog framework (C5): user-facing messages render in the QML
    // dialogManager — never a native QMessageBox. type: info|warning|error.
    void uiDialogRequested(QString type, QString title, QString message);
    void shootCountChanged(int count);
    void hardwareDisconnected();
    void hardwareReconnected();
    void masterConnectionChanged(bool isConnected);
    void matchDetails(int gametype, int matchmode, int sighterTime, int matchtime, int sigherTime,int matchpf);
    void matchDetailsSetaModification(int gametype, int matchmode);
    void startMatchFromServer();
    // RC2: several plausible adapters - ask, never guess.
    void targetSelectionRequired();
    // SCANNING / TARGET DETECTED / TARGET CONNECTED / TARGET NOT DETECTED /
    // TARGET NOT CONNECTED / MANUAL SELECTION REQUIRED / TARGET DISCONNECTED.
    void targetStateChanged(QString state, QString portName);
    // Single notification for the QML-bindable target status below.
    void targetStatusChanged();

private:
    Ui::TachusWidget *ui;
    MainWindow* m_mainWindow;
    // ACQ-DESYNC-002. m_currentShootsCount and m_oldResetCount used to live
     // here as a SECOND accounting of the same thing, and getShootCount()
     // returned their sum. A reconnect moved one of them without the other and
     // the sum ran ahead of the coordinate arrays for the rest of the session.
     // The hardware counter now has exactly one owner - m_seq - and the logical
     // shot count is the coordinate count. The per-mode copies below are kept
     // only because changeSighterMode() swaps whole data sets.

    QTimer* m_timer = NULL;
    bool autoModeOn = false;
    bool isSighterMode = false; // as in contructor we would initialise with true
    bool isAppDemoMode = true;
    QList<double> m_xCordList;
    QList<double> m_yCordList;
    QStringList m_timeConsumedList;
    QStringList m_timeStampList;
    QList<int> m_shotsRotation;
    QList<double> m_xCordList_gameMode;
    QList<double> m_yCordList_gameMode;
    QList<double> m_scoreList_gameMode;
    QStringList m_timeConsumedList_gameMode;
    QStringList m_timeStampList_gameMode;
    QList<int> m_shotsRotation_gameMode;
    QList<double> m_xCordList_sighterMode;
    QList<double> m_yCordList_sighterMode;
    QList<double> m_scoreList_sighterMode;
    QStringList m_timeConsumedList_sighterMode;
    QStringList m_timeStampList_sighterMode;
    QList<int> m_shotsRotation_sighterMode;

    MotorThread* m_motorThread = nullptr;
    // RC2: one scan at a time, and one central feed authority.
    bool m_scanActive = false;
    // Set while a journal replay or historical load is feeding shots through
    // the acceptance path. Those shots are genuinely accepted and durably
    // recorded, so every other feed guard would pass - this is the one that
    // stops paper feeding for a shot fired an hour ago.
    bool m_replayInProgress = false;
    // The device the SELECTOR resolved for the current attempt. Remembered
    // only once communication is confirmed, so a failed guess is never stored.
    ta::target::SerialDeviceInfo m_pendingAutoDevice;
    bool m_hasPendingAutoDevice = false;
    QString m_traceSessionTag;
    ta::target::PaperFeedCoordinator m_feed;
    bool m_feedBound = false;

    QTcpServer* m_tcpServer = nullptr;
    double m_motor_movement_duration = 2.5;
    double m_motor_movement_duration_sighter = 2.5;
    QString m_laneName = "lane_NA";
    QString m_ipAddress;
    bool m_isMasterConnected = false;
    bool m_hardwareDisconnected = false;
    bool m_hardwareCheckDisabled = true;

    // ── RECONNECT-001: authoritative target link state ───────────────────
    // Owned here because TachusWidget is the single acquisition path every
    // discipline uses, so all Live workflows inherit this by construction.
    enum class TargetLinkState { Connected, Disconnected, Reconnecting };
    TargetLinkState m_linkState = TargetLinkState::Connected;
    int     m_consecutiveReadFailures = 0;
    int     m_reconnectAttempts = 0;
    qint64  m_lastReconnectAttemptMs = 0;
    QString m_activePortName;
    QString m_targetState = QStringLiteral("TARGET NOT CONNECTED");
    QString m_targetDevice;
    QString m_targetPort;
    QString m_targetDetail;

    // One funnel for every target state change. Keeps the legacy signal for
    // existing consumers and drives the QML-bindable properties from the same
    // call, so the two can never disagree. An empty device/port leaves the
    // previous value untouched - transient states like SCANNING should not
    // blank out an identity the operator is relying on.
    void setTargetStatus(const QString& state,
                         const QString& port = QString(),
                         const QString& device = QString(),
                         const QString& detail = QString());
    // 3 failures at the 100 ms poll = ~300 ms before declaring the link lost:
    // long enough to ride out a single glitched frame, short enough that an
    // operator is told before firing again.
    static const int kReadFailuresBeforeLinkLost = 3;
    static const int kReconnectIntervalMs = 2000;

    // LOGIN-LINK-001. On the login/home page shot acquisition is suspended, so
    // nothing was reading the target and an unplug went unnoticed indefinitely.
    // A liveness probe runs there instead, every kLoginLivenessPolls ticks of
    // the 100 ms poll (~1 s). Not every tick: the probe is a real Modbus read
    // and the home screen has no shots to race with.
    static const int kLoginLivenessPolls = 10;
    int m_loginLivenessTick = 0;

    void onTargetLinkLost();
    void attemptTargetReconnect();

    // ── Acquisition state. READY is never claimed on a COM string alone ──
    // Synchronizing: baseline not yet read FROM the target. No shot may be
    //   accepted. Entered at startup, after reconnect and after a count reset.
    // Acquiring:     baseline agreed with the target; delta==1 is a shot.
    // Fault:         an anomaly that cannot be resolved safely. Latched, and
    //   deliberately not self-clearing - the operator must be told rather than
    //   have the software quietly resume and hide a gap.
    enum class AcquisitionState { Synchronizing, Acquiring, Fault };
    AcquisitionState m_acqState = AcquisitionState::Synchronizing;

    // ACQ-FLUSH-001 / ACQ-DESYNC-002 / ACQ-SENTINEL-003. The acquisition
    // sequence - counter reset, shot numbering and reconnect reconciliation -
    // lives in ta::target so the harness exercises this exact code and not a
    // copy of it. See src/target/AcquisitionSequencer.h.
    ta::target::AcquisitionSequencer m_seq{FLUSH_SHOOT_COUNT};


    // ── RC2g-DIAG: acquisition observability. Reporting only. ────────────
    qint64 m_diagPollSeq = 0;
    int    m_diagLastRawCounter = -999;   // impossible value forces a first log
    int    m_diagLastBaseline = -999;
    bool m_onLoginPage = true;
    QString m_lastManuallyConnectedPort = "";
    int m_gamemode = 0;
    int m_currentMatchTotalShotsCount;

    int m_game_distance = 10;
    int m_game_range = 10;
    double m_match_distance_new = 10;
	int m_shotPerSeries = 10;
    QString m_serverSettingsFilePath;
    QString m_serverLaneFilePath;

    double m_xGroup;
    double m_yGroup;

    // seta SCMA changes
    QString m_setaServerPath;
    QString m_setaServerSettingPath; // not used
    QString m_setaLaneStatusPath;    // not used
    QString m_setaLaneShootDataFilePath;
    QString m_setaLaneScoreSummaryFilePath;
    QString m_setaLaneEachScoreDataFilePath;
    bool m_isSingleDecimal = true;

    // summary data file
    int m_totalScoreWOD;
    double m_totalScoreWD;
    QMap<int, int> m_seriesScore;
    QMap<int, double> m_seriesScoreWD;

    // Analytics Settings
    /*
    series_start_at=1
    series_end_at=6
    shot_interval=20
    green_zone_start=10
    green_zone_end=10.9
    yellow_zone_start=9
    yellow_zone_end=9.9
    red_zone_end=8.9
    red_zone_start=8
    */

    int m_series_start_at = 1;
    int m_series_end_at = 6;
    int m_shot_interval = 20;
    double m_green_zone_start = 10.0;
    double m_green_zone_end = 10.9;
    double m_yellow_zone_start = 9;
    double m_yellow_zone_end = 9.9;
    double m_red_zone_start = 8;
    double m_red_zone_end = 8.9;

    bool m_isServerNetworkEnabled = true;
};

#endif // TACHUSWIDGET_H
