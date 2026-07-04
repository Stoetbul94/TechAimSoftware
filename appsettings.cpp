#include "appsettings.h"
#include "defines.h"

#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QXmlStreamWriter>
#include <QFileDialog>
#include <QDomDocument>
#include <QFileInfo>
#include <QCoreApplication>

#define CONTROL_FILE "status.csv"
#define REPORT_FILE "report.pdf"
#define SHOOTDATA_FILE "shootdata"
#define SHOOT_FILE "score"
#define EACH_SHOOT_FILE "scoreData"

AppSettings::AppSettings(QString fileName)
{
    //with ini file
    m_iniFileName = fileName;
    m_settings = new QSettings( fileName, QSettings::IniFormat );
    m_settings->beginGroup("App_Settings");
    //m_settings->setValue("app_mode", "Demo");
    //m_settings->setValue("brand_name", "tachus");
    m_appMode = m_settings->value("app_mode", "Demo").toString() == "Live" ? true : false;
    qDebug() << m_settings->value("app_mode", "Demo").toString() << "*******" << fileName;

#ifdef BRAND_TACHUS
    m_brandName = "tachus"; //m_settings->value("brand_name", "tachus").toString();
#else
  m_brandName = "seta";// m_settings->value("brand_name", "tachus").toString();
#endif
    motor_movement_time = m_settings->value("motor_movement_time", 2.5).toDouble();
    motor_movement_time_sighter = m_settings->value("motor_movement_time_sighter", 2.5).toDouble();
    game_distance = m_settings->value("game_distance", 10).toInt();
    m_laneNumber = m_settings->value("lane_number", 2).toInt();
    qDebug() << m_settings->value("game_distance").toInt() << "*******" << "game_distance";
    m_is15Shoot = false;
    m_bullet_diameter = m_settings->value("bullet_size", 5.6).toDouble();
    qDebug() << " m_bullet_diameter " << m_bullet_diameter;

#ifndef BRAND_TACHUS
    m_is15Shoot = m_settings->value("15_shoot_match", 0).toInt() == 0 ? false : true;
#endif

    m_isSingleDecimal = m_settings->value("is_single_decimal", 0).toInt() == 0 ? false : true;
    m_match_meter = m_settings->value("distance", 10).toDouble();
    qDebug() << " distance " << getMatch_meter();
    QString matchRetain =  m_settings->value("match_file").toString(); //Match File: Clear/Retain
    if (matchRetain.compare("clear", Qt::CaseInsensitive) == 0) {
        m_isOverwriteOldSaveFile = true;
    }
    m_10or50mRange = m_settings->value("10or50mRange", 10).toInt();


    m_settings->endGroup();

//    Display_Settings:
//        A-pellet_transparent: yes/no
//        B-pellet_type: Normal/Bordered
//        C-zoom_button: Yes/No
//        D-group_mpi: Yes/No
//        E-notification_for_last_shot: Yes/No
    m_settings->beginGroup("Display_Settings");

    QString palletTransparent = m_settings->value("pellet_transparent", "no").toString();
    if (palletTransparent.compare("yes", Qt::CaseInsensitive) == 0)
        m_isPalletTRansparent = true;


    QString scoringsystem = m_settings->value("score_in_decimal","yes").toString();
    if (scoringsystem.compare("yes", Qt::CaseInsensitive) == 0)
        m_scoringSystem =true;

    QString zoomButton = m_settings->value("zoom_button",  "yes").toString();
    if (zoomButton.compare("yes", Qt::CaseInsensitive) == 0)
        m_showZoomButton = true;

    QString group_mpi = m_settings->value("group_mpi", "no").toString();
    if (group_mpi.compare("yes", Qt::CaseInsensitive) == 0)
        m_showGroupAndMPI = true;

    QString notification = m_settings->value("notification_for_last_shot", "yes").toString();
    if (notification.compare("yes", Qt::CaseInsensitive) == 0)
        m_notificationForLastShot = true;


    QString sighter_series = m_settings->value("sighter_series").toString();
   if (sighter_series.compare("clear", Qt::CaseInsensitive) == 0)
        m_sighter_series = true;

    m_settings->endGroup();

    m_settings->beginGroup("shot_count_and_timer");

    // Countdown clock is ON by default; config.ini can still turn it off
    // with timer=no. (Previously it was hidden unless the file said yes,
    // which made the app silently lose the clock without its config file.)
    QString timer = m_settings->value("timer", "yes").toString();
    if (timer.compare("yes", Qt::CaseInsensitive) == 0)
        m_timer = true;

    // ISSF 2026: combined preparation + sighting period is 15 minutes for all
    // events. Configurable for training via [shot_count_and_timer] prep_time.
    m_prepTimeMinutes = m_settings->value("prep_time", "15").toInt();


    shootCountAndTimeMap[10] = m_settings->value("ten_shoot", "15").toInt();
    shootCountAndTimeMap[20] = m_settings->value("twenty_shoot", "30").toInt();
    shootCountAndTimeMap[24] = m_settings->value("twentyfour_shoot", "35").toInt();
    shootCountAndTimeMap[30] = m_settings->value("thirty_shoot", "45").toInt();
    shootCountAndTimeMap[40] = m_settings->value("forty_shoot", "60").toInt();
    shootCountAndTimeMap[60]  = m_settings->value("sixty_shoot", "90").toInt();
    shootCountAndTimeMap[-1]  = m_settings->value("default", "90").toInt(); // -1 for free practise
    m_settings->endGroup();

//    // read server settings file path.
//    m_settings->beginGroup("Server_Settings");
//    m_serverSettingsFilePath = m_settings->value("serverFilePath").toString();
//    m_settings->endGroup();

//    // add watcher to the file
//    qDebug() << __FUNCTION__ << __LINE__ << m_serverSettingsFilePath;
//    if (!m_serverSettingsFilePath.isEmpty()) {
//        m_watcher = new QFileSystemWatcher(this);
//        connect(m_watcher, SIGNAL(fileChanged(const QString &)), this, SLOT(readServerSettings(const QString &)));
//        m_watcher->addPath(m_serverSettingsFilePath);
//    }
    // read Analytics Settings
    m_settings->beginGroup("analytics_settings");
    m_series_start_at = m_settings->value("series_start_at", "1").toInt();
    m_series_end_at = m_settings->value("series_end_at", "6").toInt();
    m_shot_interval = m_settings->value("shot_interval", "30").toInt();

    m_green_zone_start = m_settings->value("green_zone_start", "10.0").toDouble();
    m_green_zone_end  = m_settings->value("green_zone_end", "10.9").toDouble();
    m_yellow_zone_start = m_settings->value("yellow_zone_start", "9").toDouble();
    m_yellow_zone_end = m_settings->value("yellow_zone_end", "9.9").toDouble();
    m_red_zone_start = m_settings->value("red_zone_start", "8").toDouble(); // -1 for free practise
    m_red_zone_end = m_settings->value("red_zone_end", "8.9").toDouble();
    m_settings->endGroup();

    fetchUserHistoryData();
}

void AppSettings::setTachusWidget(TachusWidget *widget)
{
    tachusWidget = widget;
    tachusWidget->setMotorMovementTime(motor_movement_time, motor_movement_time_sighter);
    tachusWidget->setIsAppDemoMode(m_appMode);
    tachusWidget->setGame_distance(game_distance);
    tachusWidget->setServerPath(m_serverSettingsFilePath);
    tachusWidget->setMatch_distance_new(m_match_meter);

    QString serverSettingFile = m_serverSettingsFilePath.section("/",-1,-1);
    QString temp = m_serverSettingsFilePath;
    temp.remove(serverSettingFile);
    temp.append(getLaneString());
    temp.append(".txt");
    tachusWidget->setServerLaneFilePath(temp);
    LogFile::instance().appendToLogFile(QString("default mode -> %1").arg(m_appMode), LogType::interfaceLevel);
    //    if (m_isLogEnabled)
    //        tachusWidget->initialiseLogFile();

    // set Analytics Settings
    tachusWidget->setSeries_start_at(m_series_start_at);
    tachusWidget->setSeries_end_at(m_series_end_at);
    tachusWidget->setShot_interval(m_shot_interval);
    tachusWidget->setGreen_zone_start(m_green_zone_start);
    tachusWidget->setGreen_zone_end(m_green_zone_end);
    tachusWidget->setYellow_zone_start(m_yellow_zone_start);
    tachusWidget->setYellow_zone_end(m_yellow_zone_end);
    tachusWidget->setRed_zone_start(m_red_zone_start);
    tachusWidget->setRed_zone_end(m_red_zone_end);
}

bool AppSettings::getAppMode()
{
    return m_appMode;
}

QString AppSettings::getBrandName()
{
    return m_brandName;
}

void AppSettings::saveMatch(bool createNew)
{
    if (m_matchSavedFile == NULL || createNew) {
        if (m_isOverwriteOldSaveFile) {
            m_matchSavedFile = new QFile(QString("Match_%1.tch").arg(user_name));

        } else
            m_matchSavedFile = new QFile(QString("Match_%1.tch").arg(QDateTime::currentDateTime().toString("ddMMyyyy-hhmmss")));
    }

    if (m_matchSavedFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QXmlStreamWriter xmlWriter(m_matchSavedFile);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        xmlWriter.writeStartElement("root");
        xmlWriter.writeStartElement("Game_information");
        xmlWriter.writeTextElement("user_name", user_name );
        xmlWriter.writeTextElement("game_mode", QString::number(game_mode));
        xmlWriter.writeTextElement("game_event", QString::number(game_event));
        xmlWriter.writeTextElement("game_type", QString::number(game_is_sighter_mode));
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("GameData");
        if (tachusWidget != NULL)
        {
            for (int i=0; i<tachusWidget->getShootCount(); ++i)
            {
                xmlWriter.writeStartElement(QString("data_%1").arg(i));
                xmlWriter.writeTextElement("x_data", QString::number(tachusWidget->getXCord(i+1)));
                xmlWriter.writeTextElement("y_data", QString::number(tachusWidget->getYCord(i+1)));
                xmlWriter.writeTextElement("time", QString::number(tachusWidget->getTime(i+1)));
                xmlWriter.writeTextElement("time_stamp", tachusWidget->getTimeStamp(i+1));
                xmlWriter.writeEndElement();
            }
        }

        xmlWriter.writeEndElement();

        xmlWriter.writeEndDocument();
        xmlWriter.writeEndDocument();
        m_matchSavedFile->close();
    }
}

void AppSettings::autoSaveMatch()
{
    if (m_matchSavedFile == NULL) {
        if (m_isOverwriteOldSaveFile) {
            m_matchSavedFile = new QFile(QString("match_%1.tch").arg(user_name));

        } else
            m_matchSavedFile = new QFile(QString("test_match%1.tch").arg(QDateTime::currentDateTime().toString("ddMMyyyy-hhmmss")));
    }

    //    if (m_matchSavedFile->open(QIODevice::WriteOnly | QIODevice::Text))
    if (m_matchSavedFile->open(QIODevice::WriteOnly | QIODevice::Text) | QIODevice::Truncate) //auto save mode
    {
        QXmlStreamWriter xmlWriter(m_matchSavedFile);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        xmlWriter.writeStartElement("root");
        xmlWriter.writeStartElement("Game_information");
        xmlWriter.writeTextElement("user_name", user_name );
        xmlWriter.writeTextElement("game_mode", QString::number(game_mode));
        xmlWriter.writeTextElement("game_event", QString::number(game_event));
        xmlWriter.writeTextElement("game_type", QString::number(game_is_sighter_mode));
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("GameData");
        if (tachusWidget != NULL)
        {
            for (int i=0; i<tachusWidget->getShootCount(); ++i)
            {
                xmlWriter.writeStartElement(QString("data_%1").arg(i));
                xmlWriter.writeTextElement("x_data", QString::number(tachusWidget->getXCord(i+1)));
                xmlWriter.writeTextElement("y_data", QString::number(tachusWidget->getYCord(i+1)));
                xmlWriter.writeTextElement("time", QString::number(tachusWidget->getTime(i+1)));
                xmlWriter.writeTextElement("time_stamp", tachusWidget->getTimeStamp(i+1));
                xmlWriter.writeEndElement();
            }
        }

        xmlWriter.writeEndElement();

        xmlWriter.writeEndDocument();
        xmlWriter.writeEndDocument();
        m_matchSavedFile->close();
    }
}

void AppSettings::autoSaveMatchScore(int index, double xCor, double yCor)
{

}

bool AppSettings::isEulaAccepted()
{
    if (getBrandName() == "tachus")
    {
        QSettings regSettings("Tachus", "shootingApp");
        bool isEulaAccepted = regSettings.value("isEulaAccepted").toBool();
        return isEulaAccepted;
    } else {
        QSettings regSettings("Seta", "shootingApp");
        bool isEulaAccepted = regSettings.value("isEulaAccepted").toBool();
        return isEulaAccepted;
    }
}

void AppSettings::eulaAccepted()
{
    if (getBrandName() == "tachus")
    {
        QSettings regSettings("Tachus", "shootingApp");
        regSettings.setValue("isEulaAccepted", true);
    } else {
        QSettings regSettings("Seta", "shootingApp");
        regSettings.setValue("isEulaAccepted", true);
    }
}

QString AppSettings::getLoadFileLocation()
{
    if (getBrandName() == "tachus")
    {
        QSettings regSettings("Tachus", "shootingApp");
        return regSettings.value("loadFilePath").toString();
    } else {
        QSettings regSettings("Seta", "shootingApp");
        return regSettings.value("loadFilePath").toString();
    }
}

void AppSettings::setLoadFileLocation(QString path)
{
    if (getBrandName() == "tachus")
    {
        QSettings regSettings("Tachus", "shootingApp");
        regSettings.setValue("loadFilePath", path);
    } else {
        QSettings regSettings("Seta", "shootingApp");
        regSettings.setValue("loadFilePath", path);
    }
}

void AppSettings::setUsername(QString name)
{
    qDebug() << __FUNCTION__ << __LINE__ << name;
    if (!name.isEmpty()) {
        user_name = name;
        emit userNameChanged(user_name);
        if (tachusWidget)
            tachusWidget->setLaneName(getUserName());
    }
}

void AppSettings::setGameMode(int mode)
{
    game_mode = mode;
    tachusWidget->setGamemode(mode);
    if (game_mode == 0)
        setGame_mode_string("pistol");
    else
        setGame_mode_string("rifle");
}

void AppSettings::setGameEvent(int event)
{
    game_event = event;
}

void AppSettings::setGameSubMode(int mode)
{
    game_sub_mode = mode;
}

bool AppSettings::uploadGame()
{
    // Default to the folder the app saves matches into (next to the exe),
    // not the Windows home folder; remember the last-used folder after that.
    QString dirName = QCoreApplication::applicationDirPath();
    if (!getLoadFileLocation().isEmpty()) {
        dirName = getLoadFileLocation();
    }

    QString fileName = QFileDialog::getOpenFileName(tachusWidget, tr("Open File"),
                                                    dirName,
                                                    tr("File (*.tch)"));
    if (fileName.isEmpty())
        return false;

    QDomDocument xmlDocument;
    QFile f(fileName);
    if(!f.open(QIODevice::ReadOnly))
    {
        qDebug("Error While Reading the File");
        return false;
    }

    QFileInfo fInfo(f);
    qDebug() << fInfo.absoluteDir();
    setLoadFileLocation(fInfo.absoluteDir().absolutePath());
    xmlDocument.setContent(&f);
    f.close();
    qDebug("File was closed Successfully");

    //    <root>
    //    <Game_information>
    //        <user_name>SSBED</user_name>
    //        <game_mode>Pistol</game_mode>
    //        <game_event>20 Shots Match</game_event>
    //    </Game_information>
    //    <GameData>
    //        <data_0>
    //            <x_data>2.54639e-313</x_data>
    //            <y_data>2.54639e-313</y_data>
    //            <time>3</time>
    //        </data_0>
    //    </GameData>
    //    </root>

    QDomElement root = xmlDocument.documentElement();
    QDomElement gameInfoEle = root.firstChild().toElement();
    QString startTag = gameInfoEle.tagName();
    qDebug()<<"The ROOT tag is"<<startTag;

    // Get root names and attributes
    QDomElement userEle = gameInfoEle.firstChild().toElement();
    QString useData = userEle.firstChild().toText().data();
    QDomElement gameModeEle = userEle.nextSibling().toElement();
    QString gameModeData = gameModeEle.firstChild().toText().data();
    QDomElement gameEventEle = gameModeEle.nextSibling().toElement();
    QString gameEventData = gameEventEle.firstChild().toText().data();
    QDomElement gameTypeEle = gameEventEle.nextSibling().toElement();
    QString gameTypeData = gameTypeEle.firstChild().toText().data();
    qDebug() << "user name "<< useData << " gamemode " << gameModeData << " gameType " << gameTypeData;

    user_name = useData;
    game_mode = gameModeData.toInt();
    game_event = gameEventData.toInt();
    game_is_sighter_mode = gameTypeData.toInt();

    QDomElement gameDataEle = gameInfoEle.nextSibling().toElement();
    if(!gameDataEle.isNull())
    {
        x_valueList.clear();
        y_valueList.clear();
        timeList.clear();
        timeStampList.clear();

        if(gameDataEle.tagName()=="GameData")
        {
            QDomElement Component = gameDataEle.firstChild().toElement();

            while(!Component.isNull())
            {
                QString cmp = Component.tagName();
                //qDebug("Inside the Component WHILE Loop");
                if(Component.tagName().contains("data_"))
                {
                    QDomElement xEle = Component.firstChild().toElement();
                    QString xData = xEle.firstChild().toText().data();
                    x_valueList.append(xData.toDouble());
                    QDomElement yEle = xEle.nextSibling().toElement();
                    QString yData = yEle.firstChild().toText().data();
                    y_valueList.append(yData.toDouble());
                    QDomElement timeEle = yEle.nextSibling().toElement();
                    QString timeData = timeEle.firstChild().toText().data();
                    timeList.append(timeData.toDouble());
                    qDebug()<<"---------------------The data - "<<cmp << " x value " << xData << " y value " <<yData << " time " << timeData;
                    QDomElement timeStampEle = timeEle.nextSibling().toElement();
                    QString timeStampData = timeStampEle.firstChild().toText().data();
                    timeStampList.append(timeStampData);
                }

                Component = Component.nextSibling().toElement();
            }
        }
    }

    return true;
}

int AppSettings::getLoadedGameShotCount()
{
    if (x_valueList.count() == y_valueList.count())
        return x_valueList.count();

    return 0;
}

double AppSettings::getLoadedGameX(int index)
{
    if (x_valueList.count() > index)
        return x_valueList.at(index);

    return -1;
}
bool AppSettings::getScoringSystem() const
{
    return m_scoringSystem;
}

double AppSettings::getLoadedGameY(int index)
{
    if (y_valueList.count() > index)
        return y_valueList.at(index);

    return -1;
}

double AppSettings::getLoadedGameTime(int index)
{
    if (timeList.count() > index) {
        qDebug() << index<< "getLoadedGameTime " << timeList.at(index);
        qDebug() <<"timeList "<< timeList;
        return timeList.at(index);
    }

    return -1;
}

QString AppSettings::getLoadedGameTimeStamp(int index)
{
    if (timeStampList.count() > index) {
        qDebug() << index<< "getLoadedGameTime " << timeStampList.at(index);
        qDebug() <<"timeList "<< timeStampList;
        return timeStampList.at(index);
    }

    return "NNA";
}

void AppSettings::clearLoadedData()
{
    x_valueList.clear();
    y_valueList.clear();
    timeList.clear();
    timeStampList.clear();
}

int AppSettings::getTimeCount(int shootCount)
{
    // The 60-shot official match shares a single shot count across four ISSF
    // disciplines that each have a different official match time, so the
    // shot-count-keyed settings map cannot disambiguate them. Return the ISSF
    // 2026 official time per discipline (must match LoginPage getMatchTime()).
    if (shootCount == 60)
    {
        if (game_mode == 0)          // 10m Air Pistol
            return 75 * 60;
        if (m_10or50mRange == 10)    // 10m Air Rifle
            return 75 * 60;
        if (game_sub_mode == 1)      // 50m Rifle 3 Positions on EST (105 min is paper/outdoor)
            return 90 * 60;
        return 50 * 60;              // 50m Rifle Prone
    }

    if (shootCountAndTimeMap.contains(shootCount))
    {
        return shootCountAndTimeMap[shootCount]*60;
    }

    return 90 * 60;
}

void AppSettings::setGame_is_sighter_mode(int value)
{
    qDebug() << __FUNCTION__ << value;
    game_is_sighter_mode = value;

    //update xml file
    QDomDocument xmlDocument;
    if(!m_matchSavedFile->open(QIODevice::ReadWrite | QIODevice::Text))
    {
        qDebug("Error While Reading the File");
    }

    xmlDocument.setContent(m_matchSavedFile);
    qDebug("File was closed Successfully");

    //    <root>
    //    <Game_information>
    //        <user_name>SSBED</user_name>
    //        <game_mode>Pistol</game_mode>
    //        <game_event>20 Shots Match</game_event>
    //    </Game_information>
    //    <GameData>
    //        <data_0>
    //            <x_data>2.54639e-313</x_data>
    //            <y_data>2.54639e-313</y_data>
    //        </data_0>
    //    </GameData>
    //    </root>

    QDomElement root = xmlDocument.documentElement();
    QDomElement gameInfoEle = root.firstChild().toElement();
    QString startTag = gameInfoEle.tagName();
    qDebug()<<"The ROOT tag is"<<startTag;

    // Get root names and attributes
    QDomElement userEle = gameInfoEle.firstChild().toElement();
    QString useData = userEle.firstChild().toText().data();
    QDomElement gameModeEle = userEle.nextSibling().toElement();
    QString gameModeData = gameModeEle.firstChild().toText().data();
    QDomElement gameEventEle = gameModeEle.nextSibling().toElement();
    QString gameEventData = gameEventEle.firstChild().toText().data();
    QDomElement gameTypeEle = gameModeEle.nextSibling().toElement();
    QString gameTypeData = gameTypeEle.firstChild().toText().data();
    qDebug() <<"1 game type " << gameTypeData;
    gameTypeEle.firstChild().toText().setData(QString::number(value));
    gameTypeData = gameTypeEle.firstChild().toText().data();
    qDebug() <<"2 game type " << gameTypeData;

    m_matchSavedFile->resize(0);
    QTextStream stream(m_matchSavedFile);
    stream.setDevice(m_matchSavedFile);
    xmlDocument.save(stream, 1);
    m_matchSavedFile->close();
}

double AppSettings::getMotor_movement_time() const
{
    return motor_movement_time;
}

void AppSettings::saveMotorTimes(double matchSecs, double sighterSecs)
{
    motor_movement_time = matchSecs;
    motor_movement_time_sighter = sighterSecs;

    // Fresh QSettings with explicit group paths: m_settings' group state
    // after construction is unknown, and nesting would corrupt the keys.
    QSettings s(m_iniFileName, QSettings::IniFormat);
    s.setValue("App_Settings/motor_movement_time", matchSecs);
    s.setValue("App_Settings/motor_movement_time_sighter", sighterSecs);
    s.sync();

    if (tachusWidget)
        tachusWidget->setMotorMovementTime(matchSecs, sighterSecs);

    LogFile::instance().appendToLogFile(
        QString("motor feed times saved: match %1 s, sighter %2 s")
            .arg(matchSecs).arg(sighterSecs), LogType::interfaceLevel);
}

void AppSettings::setMotor_movement_time(double value, double sighterFeedTime)
{
    motor_movement_time = value;
    if (tachusWidget) {
        tachusWidget->setMotorMovementTime(value/100, sighterFeedTime/100);
        tachusWidget->intiateAutoMovementSighterSetup();
    }
}

int AppSettings::getGame_distance() const
{
    return game_distance;
}

void AppSettings::readServerSettings(const QString &path)
{
    qDebug() << __FUNCTION__ << path << __LINE__;

    bool isApply = false;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        QString line = in.readAll();
        file.close();
        qDebug() << __FUNCTION__ << line << __LINE__;

        int gametype =0 ;
        int matchModeIndex = 0 ;
        int sighterTime = 0;
        int matchTimer = 0;
        int sighterPf = 0;
        int matchPf = 0;
        QStringList dataSet = line.split(",");
        QStringList data = dataSet[0].split("=");
        if( data.count()>1)
        {
            gametype = data[1].toInt();
        }
        data = dataSet[1].split("=");
        if(data.count()>1)
        {
            QStringList matchList_ = {"10 shots match", "20 shots match", "30 shots match", "40 shots match", "60 shots match","Free pratice"};
            matchModeIndex = matchList_.indexOf(data[1]);
        }
        data = dataSet[2].split("=");
        if(data.count()>1)
        {
            sighterTime = data[1].toInt();
        }
        data = dataSet[3].split("=");
        if(data.count()>1)
        {
            matchTimer = data[1].toInt();
        }
        data = dataSet[4].split("=");
        if(data.count()>1)
        {
            sighterPf = data[1].toInt();
        }
        data = dataSet[5].split("=");
        if(data.count()>1)
        {
            matchPf = data[1].toInt();
        }
        emit tachusWidget->matchDetails(gametype,matchModeIndex,sighterTime,matchTimer,sighterPf,matchPf);

        if (line.contains("start")) {
            emit tachusWidget->startMatchFromServer();
        } else {
            // this is when apply is clicked, now read the lane_settings.ini file for port number
            isApply = true;
        }

        qDebug() <<__LINE__<< "Gametype is" << gametype << matchModeIndex << sighterTime << matchTimer << matchPf << sighterPf << matchPf ;
    }

    qDebug() <<__LINE__<<__FUNCTION__<<isApply;
    if (isApply) {
        //now read the lane_settings.ini file for port number
        QString dir = path.section("/",0,-2);
        QString laneSettingsFilePath = QString("%1/lane_settings.ini").arg(dir);
        QSettings settings( laneSettingsFilePath, QSettings::IniFormat );

        settings.beginGroup("lane_settings");
        setUsername(settings.value(getLaneString()).toString());
        qDebug() <<__LINE__<<__FUNCTION__<<user_name;
        settings.endGroup();

        settings.beginGroup("port_settings");
        setPortText(settings.value(getLaneString()).toString());
        qDebug() <<__LINE__<<__FUNCTION__<<m_portText;
        settings.endGroup();
    }
}

bool AppSettings::getIsPalletTRansparent() const
{
    return m_isPalletTRansparent;
}

void AppSettings::setIsPalletTRansparent(bool isPalletTRansparent)
{
    m_isPalletTRansparent = isPalletTRansparent;
}

bool AppSettings::isGPUVersion()
{
#ifdef OPEN_GL
        return false;
#endif
        return true;
}
void AppSettings::readControlFile(const QString &path)
{
    if (!tachusWidget->getIsServerNetworkEnabled())
        return;

    qDebug() << __FUNCTION__ << path << __LINE__;

    if (path == getSetaSettingsFilePath()) {
        uploadSetaServerSettingsCSV();
        qDebug() << __FUNCTION__ << path << __LINE__;
    } else if (path.endsWith(getStatusFileName())) {
        qDebug() << __FUNCTION__ << path << __LINE__;
        if (m_internalStatusFileModification)
            return;

        qDebug() << __FUNCTION__ << path << __LINE__;
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            QString line = in.readAll();
            file.close();

            // from harald
//            Statusfile like this
//            Status.csv
//            Act_mode;
//            1;

//            1= Home
//            2= Sighter
//            3= Match

            line = line.trimmed();
            QStringList dataset = line.split(";", Qt::SkipEmptyParts);
            if (dataset.contains("print")) {
                m_internalStatusFileModification = true;
                QFile file(path);
                line = line.trimmed();
                if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    QTextStream in(&file);
                    //QString line = in.readAll();
                    line.remove("print;1;");
                    in << line;
                    file.close();
                }
                qDebug() << __LINE__ << __FUNCTION__;
                emit printPDF();
                m_internalStatusFileModification = false;
            } else if (dataset.contains("Act_mode") && dataset.count() >= 2) {
                QString data = dataset.at(1);
                if (data.contains("3")) {
                    emit startMatch();
                } else if (data.contains("2")) {
                    emit startSighter();
                } else if (data.contains("1")) {
                    emit backHome();
                }
            }
        }
    }
}



int AppSettings::get10or50mRange() const
{
    return m_10or50mRange;
}

void AppSettings::set10or50mRange(int range)
{
    m_10or50mRange = range;
}


void AppSettings::serverDirectoryChanged(const QString &path)
{
//    qDebug() << __FUNCTION__ << __LINE__ << path;
//    if (path == getSetaServerPath()) {
//        QString controlFilePath = QString("%1/%2").arg(getSetaServerPath()).arg(getStatusFileName());
//        if (QFileInfo::exists(controlFilePath)) {
//            m_watcher->addPath(controlFilePath);
//            readControlFile(controlFilePath);
//        }
//    }
}

void AppSettings::readControlCSVFile(const QString &path)
{

}

double AppSettings::bullet_diameter() const
{
    return m_bullet_diameter;
}
void AppSettings::fetchUserHistoryData()
{
    QFile file("userHistory.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if(!line.isEmpty())
            m_userHistory.append(line);
    }

    file.close();
}

void AppSettings::updateUserHistoryData(QString name)
{
    if(!m_userHistory.contains(name)) {
        m_userHistory.append(name);
    }

    QFile file("userHistory.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return;

    QTextStream out(&file);
    out << m_userHistory.join("\n");

    file.close();
	}
double AppSettings::getMatch_meter() const
{
    return m_match_meter;
}

bool AppSettings::getNotificationForLastShot() const
{
    return m_notificationForLastShot;
}

bool AppSettings::getShowGroupAndMPI() const
{
    return m_showGroupAndMPI;
}

bool AppSettings::getShowZoomButton() const
{
    return m_showZoomButton;
}

bool AppSettings::getIsPalletTypeNormal() const
{
    return m_isPalletTypeNormal;
}


bool AppSettings::getSighter_series() const
{
    return m_sighter_series;
}

bool AppSettings::timer() const
{
    return m_timer;
}
void AppSettings::setMatch_meter(double match_meter)
{
    m_match_meter = match_meter;
}

bool AppSettings::getIsSingleDecimal() const
{
    return m_isSingleDecimal;
}

void AppSettings::setIsSingleDecimal(bool isSingleDecimal)
{
    m_isSingleDecimal = isSingleDecimal;
}

bool AppSettings::getIs15Shoot() const
{
    return m_is15Shoot;
}

QString AppSettings::getGame_mode_string() const
{
    return m_game_mode_string;
}

void AppSettings::setGame_mode_string(const QString &game_mode_string)
{
    m_game_mode_string = game_mode_string;

    QString shootdatFilePath = QString("%1/%2").arg(getSetaServerPath()).arg(getShootDataFileName());
    tachusWidget->setSetaLaneShootDataFilePath(shootdatFilePath);

    QString shootScoreDataFilePath = QString("%1/%2").arg(getSetaServerPath()).arg(getShootDataReportSummaryFileName());
    tachusWidget->setSetaLaneScoreSummaryFilePath(shootScoreDataFilePath);

    QString eachshootScoreDataFilePath = QString("%1/%2").arg(getSetaServerPath()).arg(getEachShootDataReportSummaryFileName());
    tachusWidget->setSetaLaneEachScoreDataFilePath(eachshootScoreDataFilePath);
}

QString AppSettings::getSetaSettingsFilePath() const
{
    return m_setaSettingsFilePath;
}

void AppSettings::setSetaSettingsFilePath(const QString &setaSettingsFilePath)
{
    m_setaSettingsFilePath = setaSettingsFilePath;
    uploadSetaServerSettingsCSV();
    updateStatusFeedbackFile();
}

QString AppSettings::getStatusFileName()
{
    return QString("%1_%2").arg(getLaneString()).arg(CONTROL_FILE);
}

QString AppSettings::getPrintPDFFilePath()
{
    if (getSetaServerPath().isEmpty())
        return QString();

    return QString("%1/%2_%3_%4").arg(getSetaServerPath()).arg(getLaneString())
            .arg(QDateTime::currentDateTime().toString("ddMMyyyy-hhmmss")).arg(REPORT_FILE);
}

QString AppSettings::getShootDataFileName()
{
    //lane1_shhotdata_<name>_<game_type>
    return QString("%1_%2_%3_%4.csv").arg(getLaneString()).arg(SHOOTDATA_FILE).arg(getUserName()).arg(getGame_mode_string());
}

QString AppSettings::getShootDataReportSummaryFileName()
{
    return QString("%1_%2.csv").arg(getLaneString()).arg(SHOOT_FILE);
}

QString AppSettings::getEachShootDataReportSummaryFileName()
{
    return QString("%1_%2.csv").arg(getLaneString()).arg(EACH_SHOOT_FILE);
}

QString AppSettings::getSetaServerPath() const
{
    return m_setaServerPath;
}

void AppSettings::setSetaServerPath(const QString &setaServerPath)
{
    m_setaServerPath = setaServerPath;
}

QString AppSettings::selectSetaSettingsFile()
{
    QString fileName = QFileDialog::getOpenFileName(tachusWidget, tr("Open File"),
                                                    "",
                                                    tr("File (*.csv)"));
    if (fileName.isEmpty())
        return QString();

    setSetaSettingsFilePathFromQML(fileName);
    return fileName;
}

void AppSettings::setSetaSettingsFilePathFromQML(QString filePath)
{
    if(filePath.isEmpty())
        return;

    // get the seta server path (dir)
    QString dir = filePath.section("/",0,-2);
    setSetaServerPath(dir);

    setSetaSettingsFilePath(filePath);

    addSetaServerPathToWatcher();
    qDebug() << __FUNCTION__ << __LINE__ << filePath << dir;
}

void AppSettings::addSetaServerPathToWatcher()
{
    if (!tachusWidget->getIsServerNetworkEnabled())
        return;

    QString controlFilePath = QString("%1/%2").arg(getSetaServerPath()).arg(getStatusFileName());
    qDebug() << __FUNCTION__ << controlFilePath;
    if (m_watcher == NULL) {
        m_watcher = new QFileSystemWatcher(this);
        connect(m_watcher, SIGNAL(fileChanged(const QString &)), this, SLOT(readControlFile(const QString &)));
        connect(m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(serverDirectoryChanged(const QString &)));
    }

    if (!QFile::exists(controlFilePath)) {
        QFile file(controlFilePath);
        if ( file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        {
            QTextStream stream( &file );
            stream << "";// ;
        }
    }
    m_watcher->addPath(controlFilePath);
    m_watcher->addPath(getSetaSettingsFilePath());
    m_watcher->addPath(getSetaServerPath());
}

QString AppSettings::checkForSetaServerSettingFile()
{
    return QString();
}

QString AppSettings::checkForSetaLaneConcrolFile()
{
    return QString();
}

void AppSettings::uploadSetaServerSettings()
{
    QSettings settings(getSetaSettingsFilePath(), QSettings::IniFormat );
    settings.beginGroup("lane_settings");
    setLaneNumber(settings.value("lane_number").toInt());
    setUsername(settings.value("name").toString());
    setPortText(settings.value("port").toString());
    settings.endGroup();
    settings.beginGroup("game_settings");
    int gameType = settings.value("game_type").toInt();
    QString match_mode = settings.value("match_mode").toString();
    QStringList matchList_ = {"10 shots match", "20 shots match", "30 shots match", "40 shots match", "60 shots match","Free pratice"};
    int matchModeIndex = matchList_.indexOf(match_mode);
    settings.endGroup();
    emit tachusWidget->matchDetailsSetaModification(gameType,matchModeIndex);
}

void AppSettings::uploadSetaServerSettingsCSV()
{
    QFile file(getSetaSettingsFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        QString line = in.readAll();
        file.close();
        qDebug() << __FUNCTION__ << line << __LINE__;

        // name;port;lane_number:game_type;match_mode
        //srinu;COM4;2;0;30
        QStringList dataSet = line.split(";");
        if (dataSet.count() >= 5) {
            setUsername(dataSet.at(0));
            setPortText(dataSet.at(1));
            setLaneNumber(QString(dataSet.at(2)).toInt());
            int gametype = QString(dataSet.at(3)).toInt() ;
            if (gametype == 0)
                setGame_mode_string("pistol");
            else
                setGame_mode_string("rifle");
            QString matchMode = dataSet.at(4);
            QStringList matchList_ = {"10", "20", "30", "40", "60","0"};
            int matchModeIndex = matchList_.indexOf(matchMode.trimmed());

            tachusWidget->setSetaServerPath(getSetaServerPath());
            tachusWidget->setSetaServerSettingPath(getSetaSettingsFilePath());
            QString controlFilePath = QString("%1/%2").arg(getSetaServerPath()).arg(getStatusFileName());
            tachusWidget->setSetaLaneStatusPath(controlFilePath);

            QString shootdatFilePath = QString("%1/%2").arg(getSetaServerPath()).arg(getShootDataFileName());
            tachusWidget->setSetaLaneShootDataFilePath(shootdatFilePath);
            emit tachusWidget->matchDetailsSetaModification(gametype,matchModeIndex);
        }
    }
}

void AppSettings::updateStatusFeedbackFile(int flag)
{
    if (!tachusWidget->getIsServerNetworkEnabled())
        return;

    QString feedbackFile = QString("%1_feedbackFile.csv").arg(getLaneString());
    QString feedbackFilePath = QString("%1/%2").arg(getSetaServerPath()).arg(feedbackFile);

    //if (QFile::exists(feedbackFilePath)) {
        QFile file(feedbackFilePath);
        if ( file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        {
            QTextStream stream( &file );
            stream << "Act_mode;"<<flag<<";" ;
            file.close();
        }
    //}
}

QString AppSettings::getPortText() const
{
    return m_portText;
}

void AppSettings::setPortText(const QString &portText)
{
    m_portText = portText;
    emit portNumberChanged(m_portText);
}

int AppSettings::getLaneNumber() const
{
    return m_laneNumber;
}

QString AppSettings::getLaneString()
{
    return QString("lane%1").arg(m_laneNumber);
}

void AppSettings::setLaneNumber(int laneNumber)
{
    m_laneNumber = laneNumber;
    emit laneNumberChanged(getLaneString());
}
