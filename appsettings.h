#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include <QObject>
#include <QFileSystemWatcher>

#include "ModReader/forms/tachuswidget.h"
#include "defines.h"

class AppSettings : public QObject
{
    Q_OBJECT

public:
    AppSettings(QString fileName);
    void setTachusWidget(TachusWidget* widget);
    Q_INVOKABLE bool getAppMode();
    Q_INVOKABLE QString getBrandName();
    Q_INVOKABLE void saveMatch(bool createNew = false);
    Q_INVOKABLE void autoSaveMatch();
    Q_INVOKABLE void autoSaveMatchScore(int index, double xCor, double yCor);
    Q_INVOKABLE bool isEulaAccepted();
    Q_INVOKABLE void eulaAccepted();
    Q_INVOKABLE QString getLoadFileLocation();
    Q_INVOKABLE void setLoadFileLocation(QString path);

    Q_INVOKABLE int getUserHistoryCount() {
        return m_userHistory.count();
    }

    Q_INVOKABLE QString getUserHistoryData(int index) {
        if (index < m_userHistory.count())
            return m_userHistory[index];
    }

    Q_INVOKABLE void setUsername(QString name);
    Q_INVOKABLE QString getUserName() {
        return user_name;
    }
    Q_INVOKABLE void setGameMode(int mode);
    Q_INVOKABLE int getGameMode() {
        return game_mode;
    }
    Q_INVOKABLE void setGameEvent(int event);
    Q_INVOKABLE int getGameEvent() {
        return game_event;
    }
    Q_INVOKABLE void setGameSubMode(int mode);
    Q_INVOKABLE int getGameSubMode() {
        return game_sub_mode;
    }
    Q_INVOKABLE int getGameType() {
        return game_is_sighter_mode;
    }

    Q_INVOKABLE bool uploadGame();
    Q_INVOKABLE int getLoadedGameShotCount();
    Q_INVOKABLE double getLoadedGameX(int index);
    Q_INVOKABLE double getLoadedGameY(int index);
    Q_INVOKABLE double getLoadedGameTime(int index);
    Q_INVOKABLE QString getLoadedGameTimeStamp(int index);

    Q_INVOKABLE void clearLoadedData();
    Q_INVOKABLE int getTimeCount(int shootCount);

    bool isLogEnabled() {
        return m_isLogEnabled;
    }

    Q_INVOKABLE void setGame_is_sighter_mode(int value);

    double getMotor_movement_time() const;
    Q_INVOKABLE void setMotor_movement_time(double value, double sighterFeedTime);

    Q_INVOKABLE int getGame_distance() const;

    int getLaneNumber() const;
    QString getLaneString();
    void setLaneNumber(int laneNumber);

    QString getPortText() const;
    void setPortText(const QString &portText);
    Q_INVOKABLE int get10or50mRange() const;
    Q_INVOKABLE void set10or50mRange(int range);

    // seta modifications
    QString getSetaServerPath() const;
    void setSetaServerPath(const QString &setaServerPath);
    Q_INVOKABLE QString selectSetaSettingsFile();
    Q_INVOKABLE void setSetaSettingsFilePathFromQML(QString filePath);
    void addSetaServerPathToWatcher();
    QString checkForSetaServerSettingFile();
    QString checkForSetaLaneConcrolFile();
    void uploadSetaServerSettings();
    void uploadSetaServerSettingsCSV();
    Q_INVOKABLE void updateStatusFeedbackFile(int flag = 1); // 1 for home, 2 for sighter 3 for match

    // end seta modification
    QString getSetaSettingsFilePath() const;
    void setSetaSettingsFilePath(const QString &setaSettingsFilePath);
    QString getStatusFileName();
    QString getShootDataFileName();
    QString getShootDataReportSummaryFileName();
    QString getEachShootDataReportSummaryFileName();
    Q_INVOKABLE QString getPrintPDFFilePath();

    QString getGame_mode_string() const;
    void setGame_mode_string(const QString &game_mode_string);

    Q_INVOKABLE bool getIs15Shoot() const;

    Q_INVOKABLE bool getIsSingleDecimal() const;
    Q_INVOKABLE void setIsSingleDecimal(bool isSingleDecimal);

    Q_INVOKABLE double getMatch_meter() const;
    void setMatch_meter(double match_meter);

    Q_INVOKABLE bool getIsPalletTypeNormal() const;

    Q_INVOKABLE bool getShowZoomButton() const;

    Q_INVOKABLE bool getSighter_series() const;
    Q_INVOKABLE bool timer() const;
    Q_INVOKABLE bool getScoringSystem() const;
    Q_INVOKABLE bool getShowGroupAndMPI() const;

    Q_INVOKABLE bool getNotificationForLastShot() const;    

   void fetchUserHistoryData();
    Q_INVOKABLE void updateUserHistoryData(QString name);
    Q_INVOKABLE bool getIsPalletTRansparent() const;
    void setIsPalletTRansparent(bool isPalletTRansparent);

    Q_INVOKABLE bool isGPUVersion();

    Q_INVOKABLE double bullet_diameter() const;

private slots:
    void readServerSettings(const QString &path);
    void readControlFile(const QString &path);
    void serverDirectoryChanged(const QString &path);
    void readControlCSVFile(const QString &path);

signals:
    void userNameChanged(QString name);
    void portNumberChanged(QString port);
    void laneNumberChanged(QString lane_number);
    void startSighter();
    void startMatch();
    void backHome();
    void printPDF();

private:
    bool m_appMode = false; // false for demo, true for live
    QString m_brandName = "tachus";
    QSettings* m_settings = NULL;
    QString user_name = "";
    int game_mode = 0;
    int game_event = 0;
    int game_sub_mode = 0;   // 0 = Prone/Air, 1 = 50m Rifle 3 Positions
    int game_is_sighter_mode = 1; // for sighter mode
    int game_distance = 10; // its for 10 meter game only
    double motor_movement_time = 2.5;
    double motor_movement_time_sighter = 2.5;
 		bool m_is24Shots_match=false;
    bool m_isLogEnabled = true;
    TachusWidget* tachusWidget = NULL;
    QList<double> x_valueList;
    QList<double>y_valueList;
    QList<double>timeList;
    QStringList timeStampList;
    QMap<int, int> shootCountAndTimeMap;
    QFile* m_matchSavedFile = NULL;
    QSettings* m_serverSettings = NULL;
    QString m_serverSettingsFilePath;

    QFileSystemWatcher* m_watcher = NULL;
    int m_laneNumber;
    QString m_portText;

    bool m_isOverwriteOldSaveFile = false;
    bool m_isPalletTypeNormal = false;
    bool m_showZoomButton = false;
    bool m_sighter_series =false;
    bool m_timer=false;
    bool m_scoringSystem=false;
    bool m_showGroupAndMPI = false;
    bool m_notificationForLastShot = false;

    // Analytics Settings
    int m_series_start_at = 1;
    int m_series_end_at = 6;
    int m_shot_interval = 20;
    double m_green_zone_start = 10.0;
    double m_green_zone_end = 10.9;
    double m_yellow_zone_start = 9;
    double m_yellow_zone_end = 9.9;
    double m_red_zone_start = 8;
    double m_red_zone_end = 8.9;
    QStringList m_userHistory;
    int m_10or50mRange = 10;
    // seta modifications
    QString m_setaServerPath;
    QString m_setaSettingsFilePath;
    QString m_game_mode_string;
    bool m_isLaneStatusFileAvailalble;
    bool m_internalStatusFileModification = false;
    bool m_is15Shoot = false;
    bool m_isSingleDecimal = true;
    double m_match_meter = 10;
    bool m_isPalletTRansparent = false;
    double m_bullet_diameter = 5.6;
};

#endif // APPSETTINGS_H
