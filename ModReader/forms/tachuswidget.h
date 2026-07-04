#ifndef TACHUSWIDGET_H
#define TACHUSWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QThread>
#include <QDebug>
#include <QMessageBox>
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

// flush hardware shoot wount
//////////////////////////////////////
/// \brief The FlushShootCountThread class
//////////////////////////////////////

class TachusWidget;
class WorkerThread : public QThread
{
public:
    explicit WorkerThread(MainWindow* mainWindow, QObject* parent = 0)
        : m_mainWindow(mainWindow), QThread(parent)
    {
    }
    ~WorkerThread() {
    }

protected:
    void run() override {
        QThread::msleep(2600);
        //m_tachusWidget->clearShootCount();

        //
        // reset the hardware
        // register 2001 Hex = 8193 decimal
        m_mainWindow->modbusWriteSingleRegister(8193, 0);
    }

private:
    MainWindow* m_mainWindow;
};

////////////////////////////////
/// \brief The TachusWidget class
///
////////////////////////////////

class TachusWidget : public QWidget
{
    Q_OBJECT

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
    int getShootCount() {
        return m_oldResetCount + m_currentShootsCount;
    }

    double getTime(int index);
    QString getTimeStamp(int index);
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
    void resetShootinCount() {
        m_currentShootsCount = 0;
        m_oldResetCount = 0;
        // Bounded retry: with the modbus response timeout in place this write
        // returns within ~1s per attempt instead of freezing the GUI thread;
        // retry because a failed reset desyncs the hardware shot counter.
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (m_mainWindow->modbusWriteSingleRegister(8193, 0) != -1)
                break;
            LogFile::instance().appendToLogFile(
                QString("resetShootinCount: hw counter reset failed (attempt %1)").arg(attempt + 1),
                LogType::BackendLevel);
        }
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

private slots:
    void on_pushButton_3_clicked();
    void checkForNewShots(bool motorAutoMode = true);
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
    void shootCountChanged(int count);
    void hardwareDisconnected();
    void hardwareReconnected();
    void masterConnectionChanged(bool isConnected);
    void matchDetails(int gametype, int matchmode, int sighterTime, int matchtime, int sigherTime,int matchpf);
    void matchDetailsSetaModification(int gametype, int matchmode);
    void startMatchFromServer();

private:
    Ui::TachusWidget *ui;
    MainWindow* m_mainWindow;
    int m_currentShootsCount = 0;
    int m_oldResetCount = 0;
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
    int m_currentShootsCount_game = 0;
    int m_oldResetCount_game = 0;
    int m_currentShootsCount_sighter = 0;
    int m_oldResetCount_sighter = 0;
    MotorThread* m_motorThread = nullptr;
    WorkerThread* m_flushCount = nullptr;

    QTcpServer* m_tcpServer = nullptr;
    double m_motor_movement_duration = 2.5;
    double m_motor_movement_duration_sighter = 2.5;
    QString m_laneName = "lane_NA";
    bool m_flushStarted = false;
    QString m_ipAddress;
    bool m_isMasterConnected = false;
    bool m_hardwareDisconnected = false;
    bool m_hardwareCheckDisabled = true;
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
