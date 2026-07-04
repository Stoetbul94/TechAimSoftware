#include "tachuswidget.h"
#include "ui_tachuswidget.h"
#include "sender.h"

#include <QThread>
#include <QDate>
#include <cmath>
#include <cfloat>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QSerialPortInfo>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QCryptographicHash>
#include <QtConcurrent/QtConcurrent>

#define USER_DETAILS "userDetails_seta.txt"

#define ENCRYPTION_DEFAULT 3
#define DATA_DELIMITER "&*&"
#define SERIES_SHOOT_COUNT 10

TachusWidget::TachusWidget(MainWindow *mainwindow, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TachusWidget)
{
    ui->setupUi(this);
    m_mainWindow = mainwindow;

    // for initial connection, do dummy read
    //on_pushButton_clicked();

    m_timer = new QTimer(this);
    //connect(m_timer, SIGNAL(timeout()), this, SLOT(checkForNewShots()));
    //    QDate date = QDate::currentDate();
    //    if (date.dayOfYear() > 150 && date.dayOfYear() < 360 && date.year() == 2019)
    connect(m_timer, SIGNAL(timeout()), this, SLOT(on_pushButton_2_clicked()));
    m_timer->start(100);
    m_motorThread = new MotorThread(m_mainWindow, this);
    m_flushCount = new WorkerThread(m_mainWindow, this);
    changeSighterMode(true); // as the app start with sighter mode

    //connect(this, SIGNAL(shootCountChanged(int)), this, SLOT(broadCastNewShoot(int)));
    //connect(this, SIGNAL(shootCountChanged(int)), this, SLOT(updateShootData(int)));
    connect(this, SIGNAL(shootCountChanged(int)), this, SLOT(updateSetaShootData(int)));
}

TachusWidget::~TachusWidget()
{
    delete ui;
}

void TachusWidget::initialiseConnection()
{
    //    if (!m_mainWindow->isModBusConnected())
    //        m_mainWindow->changedConnect(true);

    //    uint8_t dest[1024]; //setup memory for data
    //    uint16_t * dest16 = (uint16_t *) dest;
    //    memset(dest, 0, 1024);

    //    m_mainWindow->modbusReadRegistry(8192, 2, dest16);
    //    m_currentShootsCount = dest16[1];
    //    m_mainWindow->getTextEdit()->setPlainText(QString("shoots count ******* %1").arg(m_currentShootsCount));
}

bool TachusWidget::isModBusConnected()
{
    bool isConnected = m_mainWindow->isModBusConnected();
    LogFile::instance().appendToLogFile(QString("Checking port connection status -> %1").arg(isConnected), LogType::interfaceLevel);
    return isConnected;
}

bool TachusWidget::isHardwareConnected()
{
//    To check Microphones, we use below commands:
//    Input: 01 03 10 00 00 03 (010B)
//    Output:
//    01 03 06 09B0 09B0 00C8 (63 DB)

    LogFile::instance().appendToLogFile(QString("isHardwareConnected "), LogType::interfaceLevel);
    uint8_t dest[1024]; //setup memory for data
    uint16_t * dest16 = (uint16_t *) dest;
    memset(dest, 0, 1024);

    m_mainWindow->modbusReadRegistry(4096, 2, dest16);
    int data_1 = dest16[1];
    int data_0 = dest16[0];
    LogFile::instance().appendToLogFile(QString("isHardwareConnected data[0] = %1, data[1] = %2").arg(data_0).arg(data_1), LogType::interfaceLevel);

    return (data_1 == 0 && data_0 == 0) ? false : true;
}

bool TachusWidget::isMasterSystemConnected()
{
    return m_isMasterConnected;
}

bool TachusWidget::connectedModbus(QString portName)
{
    // A leading/trailing space in the port field ("` COM7`") makes the serial
    // open fail AND tears down whatever connection was already up — trim
    // before anything else touches the name.
    portName = portName.trimmed();

    if (portName.isEmpty() && m_lastManuallyConnectedPort != "") {
        portName = m_lastManuallyConnectedPort;
    }

    if (m_mainWindow == NULL)
        return false;

    // Already connected on the requested port (or no specific port asked):
    // keep the live connection instead of tearing it down and reopening.
    if (m_mainWindow->isModBusConnected()
            && (portName.isEmpty()
                || QString::compare(portName, m_lastManuallyConnectedPort, Qt::CaseInsensitive) == 0)) {
        LogFile::instance().appendToLogFile(
            QString("already connected on %1 - keeping connection").arg(m_lastManuallyConnectedPort),
            LogType::interfaceLevel);
        return true;
    }

    if (!portName.isEmpty())
        m_lastManuallyConnectedPort = portName;

    if (portName.isEmpty())
        LogFile::instance().appendToLogFile(QString("connect with port number -> Auto Connect"), LogType::interfaceLevel);
    else
        LogFile::instance().appendToLogFile(QString("connect with port number manually -> %1").arg(portName), LogType::interfaceLevel);

    //if (!m_mainWindow->isModBusConnected()) {
        m_mainWindow->tachusReconfigurePortNumber();
        m_mainWindow->changedConnect(true, portName);
    //}

    if (m_mainWindow->isModBusConnected()) {
        clearShootCount();
        intiateAutoMovementSetup();
        return true;
    }

    // Auto-detect fallback: if the configured port would not even open, try
    // every serial port on the machine and keep the first that opens. (A
    // deeper probe via isHardwareConnected() is NOT safe here: the register
    // read blocks indefinitely on an open port with no responding target.)
    if (portName.isEmpty()) {
        const auto ports = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &info : ports) {
            const QString candidate = info.portName();
            if (candidate.isEmpty())
                continue;
            LogFile::instance().appendToLogFile(
                QString("auto-detect: trying %1").arg(candidate), LogType::interfaceLevel);
            m_mainWindow->changedConnect(true, candidate);
            if (m_mainWindow->isModBusConnected()) {
                LogFile::instance().appendToLogFile(
                    QString("auto-detect: connected on %1").arg(candidate), LogType::interfaceLevel);
                m_lastManuallyConnectedPort = candidate;
                clearShootCount();
                intiateAutoMovementSetup();
                return true;
            }
        }
        LogFile::instance().appendToLogFile(
            QString("auto-detect: no openable serial port found"), LogType::interfaceLevel);
    }

    return false;
}

int TachusWidget::validateLicence(QString mail)
{
    if (mail.isEmpty())
        return 0;

    QFile file("licence.txt");
    QFileInfo fInfo(file);

    //1. check for licence file if not exist return false
    if (!fInfo.exists() || !file.open(QIODevice::ReadOnly))
        return 1;

    //2. check for verified word on last line
    //a. if present, already verified
    //b. if not check for

    QTextStream stream(&file);
    int counter = 0;
    qDebug() << __LINE__;
    while(!stream.atEnd()) {
        QString line = stream.readLine();
        QString code;
        if (counter == 0) {
            code = getDencryptedText(line, "",true);
            line = code;
        } else {
            line = getDencryptedText(line, code, false);
        }
        qDebug() << line << __LINE__;
        QString date = QString("current_date %1").arg(QDate::currentDate().toString("dd/MM/yyyy"));
        if (line.contains("user_mail ") && line.endsWith(mail)) {
            file.close();
            licValidated();
            return 3; // verification success
        } else if (line.contains("current_date ") && !line.endsWith(date)) {
            file.close();
            return 2;
        }

        qDebug() << line;
        counter++;
    }


    file.close();
    return 0;
}

bool TachusWidget::disconnectModbus()
{
    LogFile::instance().appendToLogFile(QString("disconnect port"), LogType::interfaceLevel);
    if (m_mainWindow == NULL)
        return false;

    if (m_mainWindow->isModBusConnected())
        m_mainWindow->changedConnect(false);

    return m_mainWindow->isModBusConnected();
}

void TachusWidget::on_pushButton_clicked()
{
    if (m_mainWindow == NULL)
        return;

    int from = ui->spinBox->value();
    int to = ui->spinBox_2->value();
    //    if (from <= 0 || to <= 0 || from >= to)
    //        return;
    // just to allow initial connection


    if (!m_mainWindow->isModBusConnected())
        m_mainWindow->changedConnect(true);

    if (m_mainWindow->isModBusConnected())
    {
        LogFile::instance().appendToLogFile(QString("Collect data for shoots from %1 to %2").arg(from).arg(to), LogType::interfaceLevel);
        int baseNumber = 16376;

        for (int i=from; i<=to; ++i)
        {
            int newAddress = baseNumber +(8*i);
            m_mainWindow->setSBStartAddValue(newAddress, 0);
            m_mainWindow->request();
        }
    }
}

void TachusWidget::on_pushButton_2_clicked() // read old data
{
//    LogFile::instance().appendToLogFile(QString("isAppDemoMode %1").arg(isAppDemoMode), LogType::interfaceLevel);
//    qDebug() << __FUNCTION__ << __LINE__ << isAppDemoMode;
    if (!isAppDemoMode) // 0 for demo and 1 for live
        return;

    if (m_onLoginPage)
        return;


    if (!m_hardwareCheckDisabled) {
        if (m_hardwareDisconnected && !isHardwareConnected()) {
            return;
        }
    }

    if (m_mainWindow && m_mainWindow->isModBusConnected())
    {
        int from = m_currentShootsCount;
        checkForNewShots();
        int to = m_currentShootsCount;

        if (from < to && to-from != 10)
        {
            LogFile::instance().appendToLogFile(QString("Collecting data for shoots from %1 to %2").arg(from).arg(to), LogType::interfaceLevel);
            int baseNumber = 16376;
            for (int i=from+1; i<=to; ++i)
            {
                uint8_t dest[1024]; //setup memory for data
                uint16_t * dest16 = (uint16_t *) dest;
                memset(dest, 0, 1024);

                int newAddress = baseNumber +(8*i);
                m_mainWindow->modbusReadRegistry(newAddress, 2, dest16);
                int x = dest16[1];
                int y = dest16[0];
                double decimalDevider = m_isSingleDecimal ? 10.0 : 100.0;
                double xReal = x < 255 ? x/decimalDevider : getRealValue(x)/decimalDevider;
                double yReal = y < 255 ? y/decimalDevider : getRealValue(y)/decimalDevider;
                if (getGame_range() == 10) {
//                    double ratio = (double (getGame_distance()))/getGame_range();
                    double ratio = (double (getMatch_distance_new()))/getGame_range();
                    double modifiedX = xReal/ratio;
                    double modifiedY = yReal/ratio;
                    m_xCordList.append(modifiedX);
                    m_yCordList.append(modifiedY);

                    LogFile::instance().appendToLogFile(QString("Hardware value X %1 and computed %2").arg(x).arg(x < 255 ? x : getRealValue(x)), LogType::interfaceLevel);
                    LogFile::instance().appendToLogFile(QString("Hardware value Y %1 and computed %2").arg(y).arg(y < 255 ? y : getRealValue(y)), LogType::interfaceLevel);
                    LogFile::instance().appendToLogFile(QString("original X %1 and modified X %2 for shoot %3").arg(xReal).arg(modifiedX).arg(getShootCount() - to + i), LogType::interfaceLevel);
                    LogFile::instance().appendToLogFile(QString("original Y %1 and modified Y %2 for shoot %3").arg(yReal).arg(modifiedY).arg(getShootCount() - to + i), LogType::interfaceLevel);
                } else {
                    m_xCordList.append(xReal);
                    m_yCordList.append(yReal);
                }
                if (isSighterMode) {
                    m_xCordList_sighterMode.append(xReal);
                    m_yCordList_sighterMode.append(yReal);
                } else {
                    m_xCordList_gameMode.append(xReal);
                    m_yCordList_gameMode.append(yReal);
                }
                LogFile::instance().appendToLogFile(QString("check x cor %1 and y cor %2 for shoot %3").arg(xReal).arg(yReal).arg(getShootCount() - to + i), LogType::interfaceLevel);
                emit shootCountChanged(getShootCount() - to + i);
            }

            if (m_flushCount && m_currentShootsCount == FLUSH_SHOOT_COUNT) {
//                QThread::msleep(2600);
//                clearShootCount();
                LogFile::instance().appendToLogFile(QString("Reset shoot is called, old totol shoot count %1").arg(m_oldResetCount), LogType::interfaceLevel);
                m_oldResetCount = m_oldResetCount + m_currentShootsCount;
                LogFile::instance().appendToLogFile(QString("Reset shoot is called, Current shoot count %1").arg(m_currentShootsCount), LogType::interfaceLevel);

                m_flushStarted = true;
                m_flushCount->start();

                m_currentShootsCount = 0;
                LogFile::instance().appendToLogFile(QString("Reset done, Current shoot count %1").arg(m_currentShootsCount), LogType::interfaceLevel);
            }
        }
    }
}

bool TachusWidget::isValidLicence()
{
    //return true;

    QDate date = QDate::currentDate();

    QFile file("licence.txt");
    QFileInfo fInfo(file);
    QFileInfo cfInfo("config.ini");
    if (!cfInfo.exists())
        return true;

    QDateTime cDate = cfInfo.lastModified();
    qDebug() << "isValidLicence ------------" <<date.dayOfYear() <<fInfo.exists();

    if (date.dayOfYear() > 200 && date.year() >= 2024 && cDate.date().dayOfYear() > 2024)
        return false;

    return true;
    //1. check for licence file if not exist return false
    if (!fInfo.exists() || !file.open(QIODevice::ReadOnly))
        return false;

    //2. check for verified word on last line
    //a. if present, already verified
    //b. if not check for

        QString testEncry = getEncryptedText("srinz", true);
        qDebug() << testEncry << " encrypted text ";
        qDebug() << getDencryptedText(testEncry, "", true);
    QTextStream stream(&file);
    int counter = 0;
    while(!stream.atEnd()) {
        QString line = stream.readLine();
        qDebug() << line;
        QString code;
        if (counter == 0) {
            code = getDencryptedText(line, "",true);
            line = code;
        } else {
            line = getDencryptedText(line, code, false);
        }

        if (line == "valid")
            return true;
        qDebug() << line;
        counter++;
    }


    file.close();
    return false;
}

void TachusWidget::uxShoot(double xCor, double yCor)
{
    LogFile::instance().appendToLogFile(QString("Ux shoot call with xCor %1 and yCor %2").arg(xCor).arg(yCor), LogType::interfaceLevel);
    if (getGame_range() == 10) {
        double ratio = (double (getMatch_distance_new()))/getGame_range();
        double modifiedX = xCor/ratio;
        double modifiedY = yCor/ratio;
        m_xCordList.append(modifiedX);
        m_yCordList.append(modifiedY);
        LogFile::instance().appendToLogFile(QString("ratio %1").arg(ratio), LogType::interfaceLevel);
        LogFile::instance().appendToLogFile(QString("original X %1 and modified X %2").arg(xCor).arg(modifiedX), LogType::interfaceLevel);
        LogFile::instance().appendToLogFile(QString("original Y %1 and modified Y %2").arg(yCor).arg(modifiedY), LogType::interfaceLevel);
    } else {
        m_xCordList.append(xCor);
        m_yCordList.append(yCor);
    }

    if (isSighterMode) {
        m_xCordList_sighterMode.append(xCor);
        m_yCordList_sighterMode.append(yCor);
    } else {
        m_xCordList_gameMode.append(xCor);
        m_yCordList_gameMode.append(yCor);
    }
    LogFile::instance().appendToLogFile(QString("ux shoot count %1").arg(m_oldResetCount), LogType::interfaceLevel);
    m_oldResetCount++;
    emit shootCountChanged(m_oldResetCount);
    //    emit hardwareDisconnected();
}

double TachusWidget::getTime(int index)
{
    if (m_timeConsumedList.isEmpty() || index == 0)
        return -1;

    if (m_timeConsumedList.count()>= index) {
        return m_timeConsumedList.at(index-1).toDouble();
    }

    qDebug() << __FUNCTION__ << index;
    return -1;
}

QString TachusWidget::getTimeStamp(int index)
{
    if (m_timeStampList.isEmpty() || index == 0)
        return QString();

    if (m_timeStampList.count()>= index) {
        return m_timeStampList.at(index-1);
    }

    qDebug() << __FUNCTION__ << index;
    return QString();
}

double TachusWidget::getXCord(int index)
{
    //qDebug() << __FUNCTION__ << index << m_xCordList.count();
    if (m_xCordList.isEmpty() || index == 0)
        return -1;

    if (m_xCordList.count()>= index) {
        return m_xCordList.at(index-1);
    }

    qDebug() << __FUNCTION__ << index;
    return -1;
}

double TachusWidget:: getXMPI(int series)
{
    double mpi = 0;
    if (series == -1 || series == 0) // for all series
    {
        for(int i=0; i<m_xCordList.count(); ++i ) {
            mpi += m_xCordList.at(i);
//            qDebug() << "mpi " << mpi << " xcor " << m_xCordList.at(i) << " index " << i;
        }

//        qDebug() << " mpi " << mpi << " count " << m_xCordList.count();
        mpi = mpi/m_xCordList.count();
//        qDebug() << " mpi " << mpi;
    } else {
        int startIndex = ((series-1)*10);
        int limit = (m_xCordList.count() - (series-1)*10) > 10 ? ((series-1)*10)+10 : m_xCordList.count();
        for(int i=startIndex; i<limit; ++i ) {
            mpi += m_xCordList.at(i);
        }

        mpi = mpi/(limit-startIndex);
    }

    QString mpiString = QString::number(mpi, 'f', 2);
    qDebug() <<__LINE__<< __FUNCTION__ << mpiString;

    return mpiString.toDouble();
}

double TachusWidget::getGroup(int pageIndex, bool withPalletOffset)
{
    double maxDistance = 0;
    double maxx1 = 0;
    double maxx2 = 0;
    double maxx3 = 0;
    double maxy1 = 0;
    double maxy2 = 0;
    double maxy3 = 0;

    double x1 = 0;
    double x2 = 0;
    double y1 = 0;
    double y2 = 0;

    int max1Index = 0;
    int max2Index = 0;
    int max3Index = 0;

    if (pageIndex == -1) // for all series
    {
        pageIndex = 0;

        qDebug() << __FUNCTION__ << __LINE__ <<pageIndex << m_xCordList.count();
        qDebug() << __FUNCTION__ << __LINE__ <<pageIndex << m_xCordList.count();
        int limit = m_xCordList.count();
        qDebug() << __FUNCTION__ << __LINE__ << limit;
        if (limit < 2 || (m_xCordList.count() - (pageIndex)*10 < 2))
            return 0;
        for(int i=((pageIndex)*10); i<limit-1; ++i ) {
            qDebug() << __FUNCTION__ << __LINE__ << m_xCordList.count() << m_yCordList.count() << i;
            x1 = m_xCordList.at(i);
            y1 = m_yCordList.at(i);
            for(int j=i+1; j<limit; ++j ) {
                qDebug() << __FUNCTION__ << j;
                x2 = m_xCordList.at(j);
                y2 = m_yCordList.at(j);

                double distance = sqrt(((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1)));
                if (distance > maxDistance) {
                    maxDistance = distance;
                    maxx1 = x1;
                    maxx2 = x2;
                    maxy1 = y1;
                    maxy2 = y2;
                    max1Index = i;
                    max2Index = j;
                }
            }
        }


        m_xGroup = (maxx1+maxx2)/2;
        m_yGroup = (maxy1+maxy2)/2;

        qDebug() <<__LINE__<< __FUNCTION__ << maxDistance;

        // recalculate the distance from each shoot
        x2 = m_xGroup;
        y2 = m_yGroup;
        int counter = 0;
        bool anyExterior = false;
        for(int i=((pageIndex)*10); i<limit; ++i ) {
            x1 = m_xCordList.at(i);
            y1 = m_yCordList.at(i);

            double distance = sqrt(((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1)))*2;
            qDebug() << __FUNCTION__ << __LINE__ << distance << maxDistance << i;
            if (distance > maxDistance) {
                maxDistance = distance;
                maxx3 = x1;
                maxy3 = y1;
                max3Index = i;
                anyExterior = true;
            }

            counter++;
        }

        if (counter > 2 && anyExterior) {
            pdd center = findCircumCenter(make_pair(maxx1, maxy1), make_pair(maxx2, maxy2), make_pair(maxx3, maxy3));
            if (center.first == FLT_MAX && center.second == FLT_MAX) {

            } else {
                maxDistance = 0;
                qDebug() << "max1Index "<<max1Index<<" max2Index "<<max2Index<<" max3Index "<<max3Index;
                double distance = sqrt(((center.first-maxx1)*(center.first-maxx1))+((center.second-maxy1)*(center.second-maxy1)))*2;
                qDebug() << __FUNCTION__ << __LINE__ << center.first <<center.second << distance;
                if (distance > maxDistance) {
                    maxDistance = distance;
                    m_xGroup = center.first;
                    m_yGroup = center.second;
                }
            }
        }

    } else {
        qDebug() << __FUNCTION__ << __LINE__ <<pageIndex << m_xCordList.count();
        int limit = (m_xCordList.count() - (pageIndex)*10) > 10 ? ((pageIndex)*10)+10 : m_xCordList.count();
        qDebug() << __FUNCTION__ << __LINE__ << limit;
        if (limit < 2 || (m_xCordList.count() - (pageIndex)*10 < 2))
            return 0;
        for(int i=((pageIndex)*10); i<limit-1; ++i ) {
            qDebug() << __FUNCTION__ << __LINE__ << m_xCordList.count() << m_yCordList.count() << i;
            x1 = m_xCordList.at(i);
            y1 = m_yCordList.at(i);
            for(int j=i+1; j<limit; ++j ) {
                qDebug() << __FUNCTION__ << j;
                x2 = m_xCordList.at(j);
                y2 = m_yCordList.at(j);

                double distance = sqrt(((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1)));
                if (distance > maxDistance) {
                    maxDistance = distance;
                    maxx1 = x1;
                    maxx2 = x2;
                    maxy1 = y1;
                    maxy2 = y2;
                    max1Index = i;
                    max2Index = j;
                }
            }
        }


        m_xGroup = (maxx1+maxx2)/2;
        m_yGroup = (maxy1+maxy2)/2;

        qDebug() <<__LINE__<< __FUNCTION__ << maxDistance;

        // recalculate the distance from each shoot
        x2 = m_xGroup;
        y2 = m_yGroup;
        int counter = 0;
        bool anyExterior = false;
        for(int i=((pageIndex)*10); i<limit; ++i ) {
            x1 = m_xCordList.at(i);
            y1 = m_yCordList.at(i);

            double distance = sqrt(((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1)))*2;
            qDebug() << __FUNCTION__ << __LINE__ << distance << maxDistance << i;
            if (distance > maxDistance) {
                maxDistance = distance;
                maxx3 = x1;
                maxy3 = y1;
                max3Index = i;
                anyExterior = true;
            }

            counter++;
        }

        if (counter > 2 && anyExterior) {
            pdd center = findCircumCenter(make_pair(maxx1, maxy1), make_pair(maxx2, maxy2), make_pair(maxx3, maxy3));
            if (center.first == FLT_MAX && center.second == FLT_MAX) {

            } else {
                maxDistance = 0;
                qDebug() << "max1Index "<<max1Index<<" max2Index "<<max2Index<<" max3Index "<<max3Index;
                double distance = sqrt(((center.first-maxx1)*(center.first-maxx1))+((center.second-maxy1)*(center.second-maxy1)))*2;
                qDebug() << __FUNCTION__ << __LINE__ << center.first <<center.second << distance;
                if (distance > maxDistance) {
                    maxDistance = distance;
                    m_xGroup = center.first;
                    m_yGroup = center.second;
                }
            }
        }
    }

    if (withPalletOffset)
        maxDistance = m_game_range == 10 ? maxDistance + 4.5 : maxDistance + 5.6;

    QString distanceString = QString::number(maxDistance, 'f', 2);
    qDebug() <<__LINE__<< __FUNCTION__ << distanceString;
    return distanceString.toDouble();
}

double TachusWidget::getGroupFromList(QList<double> xList, QList<double> yList)
{
    qDebug() << __FUNCTION__ << __LINE__;
    double maxDistance = 0;
    double maxx1 = 0;
    double maxx2 = 0;
    double maxx3 = 0;
    double maxy1 = 0;
    double maxy2 = 0;
    double maxy3 = 0;

    double x1 = 0;
    double x2 = 0;
    double y1 = 0;
    double y2 = 0;

    int max1Index = 0;
    int max2Index = 0;
    int max3Index = 0;

   {
        int count = xList.count();
        for(int i=0; i<count-1; ++i ) {
            qDebug() << __FUNCTION__ << __LINE__ << m_xCordList.count() << m_yCordList.count() << i;
            x1 = xList.at(i);
            y1 = yList.at(i);
            for(int j=i+1; j<count; ++j ) {
                qDebug() << __FUNCTION__ << j;
                x2 = xList.at(j);
                y2 = yList.at(j);

                double distance = sqrt(((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1)));
                if (distance > maxDistance) {
                    maxDistance = distance;
                    maxx1 = x1;
                    maxx2 = x2;
                    maxy1 = y1;
                    maxy2 = y2;
                    max1Index = i;
                    max2Index = j;
                }
            }
        }

        m_xGroup = (maxx1+maxx2)/2;
        m_yGroup = (maxy1+maxy2)/2;

        qDebug() <<__LINE__<< __FUNCTION__ << maxDistance;

        // recalculate the distance from each shoot
        x2 = m_xGroup;
        y2 = m_yGroup;
        int counter = 0;
        bool anyExterior = false;
        for(int i=0; i<count; ++i ) {
            x1 = xList.at(i);
            y1 = yList.at(i);

            double distance = sqrt(((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1)))*2;
            qDebug() << __FUNCTION__ << __LINE__ << distance << maxDistance << i;
            if (distance > maxDistance) {
                maxDistance = distance;
                maxx3 = x1;
                maxy3 = y1;
                max3Index = i;
                anyExterior = true;
            }

            counter++;
        }

        if (counter > 2 && anyExterior) {
            pdd center = findCircumCenter(make_pair(maxx1, maxy1), make_pair(maxx2, maxy2), make_pair(maxx3, maxy3));
            if (center.first == FLT_MAX && center.second == FLT_MAX) {

            } else {
                maxDistance = 0;
                qDebug() << "max1Index "<<max1Index<<" max2Index "<<max2Index<<" max3Index "<<max3Index;
                double distance = sqrt(((center.first-maxx1)*(center.first-maxx1))+((center.second-maxy1)*(center.second-maxy1)))*2;
                qDebug() << __FUNCTION__ << __LINE__ << center.first <<center.second << distance;
                if (distance > maxDistance) {
                    maxDistance = distance;
                    m_xGroup = center.first;
                    m_yGroup = center.second;
                }
            }
        }
    }


    qDebug() <<__LINE__<< __FUNCTION__ << maxDistance << " xcenter "<<m_xGroup<<" y center "<<m_yGroup;
    return maxDistance;
}

double TachusWidget::getGroup_1(int pageIndex)
{
    if (m_xCordList.count() <= 2) {
        return getGroup(pageIndex);
    }

    double minDistance = FLT_MAX;
    pdd centerPoint;

    double x1 = 0;
    double x2 = 0;
    double x3 = 0;
    double y1 = 0;
    double y2 = 0;
    double y3 = 0;

    if (pageIndex == -1) // for all series
    {
        qDebug() << __FUNCTION__;
    } else {
        qDebug() << __FUNCTION__ << __LINE__;
        int limit = (m_xCordList.count() - (pageIndex)*10) > 10 ? ((pageIndex-1)*10)+10 : m_xCordList.count();
        qDebug() << __FUNCTION__ << __LINE__ << limit;
        if (limit < 2)
            return 0;
        for(int i=((pageIndex)*10); i<limit-2; ++i ) {
            qDebug() << __FUNCTION__ << __LINE__ << m_xCordList.count() << m_yCordList.count() << i;
            x1 = m_xCordList.at(i);
            y1 = m_yCordList.at(i);

            for(int j=i+1; j<limit-1; ++j ) {
                x2 = m_xCordList.at(j);
                y2 = m_yCordList.at(j);
                for(int k=j+1; k<limit; ++k ) {
                    x3 = m_xCordList.at(k);
                    y3 = m_yCordList.at(k);

                    pdd center = findCircumCenter(make_pair(x1, y1), make_pair(x2, y2), make_pair(x3, y3));
                    if (center.first == FLT_MAX && center.second == FLT_MAX) {

                    } else {
                        double distance = sqrt(((center.first-x1)*(center.first-x1))+((center.second-y1)*(center.second-y1)))*2;
                        qDebug() << __FUNCTION__ << __LINE__ << center.first <<center.second << distance << i;
                        if (distance < minDistance && inBoundAllPoints(center, distance, ((pageIndex)*10), limit)) {
                            minDistance = distance;
                            centerPoint = center;
                        }
                    }
                }
            }
        }
    }

    m_xGroup = centerPoint.first;
    m_yGroup = centerPoint.second;

    qDebug() <<__LINE__<< __FUNCTION__ << minDistance;
    return minDistance;
}

double TachusWidget::getXGroup()
{
    qDebug() << __FUNCTION__ << m_xGroup;
    return m_xGroup;
}

double TachusWidget::getYGroup()
{
    qDebug() << __FUNCTION__ << m_yGroup;
    return m_yGroup;
}

///////////////////////// https://www.geeksforgeeks.org/program-find-circumcenter-triangle-2/

// Function to find the line given two points
void TachusWidget::lineFromPoints(pdd P, pdd Q, double &a,
                        double &b, double &c)
{
    a = Q.second - P.second;
    b = P.first - Q.first;
    c = a*(P.first)+ b*(P.second);
}

// Function which converts the input line to its
// perpendicular bisector. It also inputs the points
// whose mid-point lies on the bisector
void TachusWidget::perpendicularBisectorFromLine(pdd P, pdd Q,
                 double &a, double &b, double &c)
{
    pdd mid_point = make_pair((P.first + Q.first)/2,
                            (P.second + Q.second)/2);

    // c = -bx + ay
    c = -b*(mid_point.first) + a*(mid_point.second);

    double temp = a;
    a = -b;
    b = temp;
}

// Returns the intersection point of two lines
pdd TachusWidget::lineLineIntersection(double a1, double b1, double c1,
                         double a2, double b2, double c2)
{
    double determinant = a1*b2 - a2*b1;
    if (determinant == 0)
    {
        // The lines are parallel. This is simplified
        // by returning a pair of FLT_MAX
        return make_pair(FLT_MAX, FLT_MAX);
    }

    else
    {
        double x = (b2*c1 - b1*c2)/determinant;
        double y = (a1*c2 - a2*c1)/determinant;
        return make_pair(x, y);
    }
}

pair<double, double> TachusWidget::findCircumCenter(pdd P, pdd Q, pdd R)
{
    // Line PQ is represented as ax + by = c
    double a, b, c;
    lineFromPoints(P, Q, a, b, c);

    // Line QR is represented as ex + fy = g
    double e, f, g;
    lineFromPoints(Q, R, e, f, g);

    // Converting lines PQ and QR to perpendicular
    // vbisectors. After this, L = ax + by = c
    // M = ex + fy = g
    perpendicularBisectorFromLine(P, Q, a, b, c);
    perpendicularBisectorFromLine(Q, R, e, f, g);

    // The point of intersection of L and M gives
    // the circumcenter
    pdd circumcenter =
           lineLineIntersection(a, b, c, e, f, g);

    if (circumcenter.first == FLT_MAX &&
        circumcenter.second == FLT_MAX)
    {
        qDebug() << "The two perpendicular bisectors "
                "found come parallel";
        qDebug() << "Thus, the given points do not form "
                "a triangle and are collinear";
    }

    else
    {
        qDebug() << "The circumcenter of the triangle PQR is: ";
        qDebug() << "(" << circumcenter.first << ", "
             << circumcenter.second  << ")";
    }

    return circumcenter;
}

bool TachusWidget::inBoundAllPoints(pair<double, double> center, double dia, int startIndex, int endIndex)
{
    for (int i = startIndex; i<endIndex; ++i) {
        double distance = sqrt((center.first-m_xCordList.at(i)*(center.first-m_xCordList.at(i)))+((center.second-m_yCordList.at(i))*(center.second-m_yCordList.at(i))))*2;
        if (distance > dia)
            return false;
    }

    return true;
}
///////////////////////// https://www.geeksforgeeks.org/program-find-circumcenter-triangle-2/

double TachusWidget::getXMPIForShoot(int series, int shootNumber)
{
    if (shootNumber >= 0 && m_xCordList.count() > shootNumber && series >= 1)
    {
        int index = ((series - 1)*10) + shootNumber;
        return m_xCordList.at(index);
    }

    return -1;
}

double TachusWidget::getYCord(int index)
{
    qDebug() << __FUNCTION__ << index;
    if (m_yCordList.isEmpty() || index == 0)
        return -1;


    ui->listWidget_2->addItem(QString("get y cord %1 index count %2").arg(index).arg(m_yCordList.count()));
    if (m_yCordList.count()>= index) {
        ui->listWidget_2->addItem(QString("get y cord value %1").arg(m_yCordList.at(index-1)));
        return m_yCordList.at(index-1);
    }

    qDebug() << __FUNCTION__ << index;
    return -1;
}

double TachusWidget::getYMPI(int series)
{
    qDebug() << __FUNCTION__;
    double mpi = 0;
    if (series == -1 || series == 0) // for all series
    {
        for(int i=0; i<m_yCordList.count(); ++i ) {
            mpi += m_yCordList.at(i);
//            qDebug() << "mpi " << mpi << " ycor " << m_yCordList.at(i) << " index " << i;
        }

//        qDebug() << "mpi " << mpi <<" count "<< m_yCordList.count();
        mpi = mpi/m_yCordList.count();
//        qDebug() << "mpi " << mpi;
    } else {
        int startIndex = ((series-1)*10);
        int limit = (m_yCordList.count() - (series-1)*10) > 10 ? ((series-1)*10)+10 : m_yCordList.count();
        for(int i=startIndex; i<limit; ++i ) {
            mpi += m_yCordList.at(i);
        }

        mpi = mpi/(limit-startIndex);
    }

    QString mpiString = QString::number(mpi, 'f', 2);
    qDebug() <<__LINE__<< __FUNCTION__ << mpiString;

    return mpiString.toDouble();
}

double TachusWidget::getYMPIForShoot(int series, int shootNumber)
{
    if (shootNumber >= 0 && m_yCordList.count() > shootNumber && series >= 1)
    {
        int index = ((series - 1)*10) + shootNumber;
        return m_yCordList.at(index);
    }

    return -1;
}

double TachusWidget::getTeiler(int series)
{
    double teiler = 0;
    if (series == -1 || series == 0) // for all series
    {
        for(int i=0; i<m_yCordList.count(); ++i ) {
            double xCor = m_xCordList.at(i);
            double yCor = m_yCordList.at(i);
            teiler += (sqrt((xCor*xCor)+(yCor*yCor)))*100;
        }

        teiler = teiler/m_yCordList.count();
    } else {
        int limit = (m_yCordList.count() - (series-1)*10) > 10 ? ((series-1)*10)+10 : m_yCordList.count();
        for(int i=((series-1)*10); i<limit; ++i ) {
            double xCor = m_xCordList.at(i);
            double yCor = m_yCordList.at(i);
            teiler += (sqrt((xCor*xCor)+(yCor*yCor)))*100;
        }

        teiler = teiler/m_yCordList.count();
    }
    return getFormatedValueFoeTwoDecimal(teiler);
}

double TachusWidget::getTeilerForShoot(int series, int shootNumber)
{
    if (shootNumber >= 0 && m_yCordList.count() > shootNumber && series >= 1)
    {
        int index = ((series - 1)*10) + shootNumber;

        float xCor = m_xCordList.at(index);
        float yCor = m_yCordList.at(index);
        float sqrtValue = sqrt(((xCor*xCor)+(yCor*yCor)));
        double final = sqrtValue*100;

//        LogFile::instance().appendToLogFile(QString("Teiler sqrt %1").arg(sqrtValue), LogType::interfaceLevel);
//        LogFile::instance().appendToLogFile(QString("Teiler sqrt*100 %1").arg(final), LogType::interfaceLevel);

        return getFormatedValueFoeTwoDecimal(final);
    }

    return -1;
}

double TachusWidget::getTeilerForShootOfMatch(int shootNumber)
{
    // QML delegate bindings re-evaluate with negative indices while the score
    // list is being cleared (currentPageIndex briefly -1 → shootNumber -10),
    // so reject the whole negative range, not just -1 — .at(-10) on an empty
    // QList is a native crash.
    if (shootNumber < 0 || m_xCordList.count() <= shootNumber)
        return -1;
    qDebug() << shootNumber << " " << __FUNCTION__;
//    int series = shootNumber/m_shotPerSeries+1;
//    int index = (shootNumber)%m_shotPerSeries+1;

//    qDebug() << shootNumber << " " << __FUNCTION__<<" " << series << " " << index;
//    return getTeilerForShoot(series, index) == -1 ? 0 : getTeilerForShoot(series, index);

    float xCor = m_xCordList.at(shootNumber);
    float yCor = m_yCordList.at(shootNumber);
    float sqrtValue = sqrt(((xCor*xCor)+(yCor*yCor)));
    double final = sqrtValue*100;

    return getFormatedValueFoeTwoDecimal(final);
}

double TachusWidget::getScore(int index)
{
    double value = 0;
    qDebug() <<__LINE__<< "index "<<index << " value " <<value;

    if (isSighterMode && index <= m_scoreList_sighterMode.count()) {
        value = m_scoreList_sighterMode.at(index-1);
    } else if (index <= m_scoreList_gameMode.count()){
        value = m_scoreList_gameMode.at(index-1);
    }

    qDebug() << "index "<<index << " value " <<value;
    return value;
}

void TachusWidget::setScore(double value)
{
    value = getFormatedSCore(value);
    if (isSighterMode) {
        m_scoreList_sighterMode.append(value);
        qDebug() << "sigter count " << m_scoreList_sighterMode.count() << " value " <<value;
    } else {
        m_scoreList_gameMode.append(value);
        qDebug() << "shout count " << m_scoreList_gameMode.count() << " value " <<value;
    }
}

void TachusWidget::initiateMotorMovement()
{
    if (m_motorThread && m_mainWindow->isModBusConnected())
        m_motorThread->start();
}

void TachusWidget::intiateAutoMovementSetup()
{
    //return; // for manual motor movement && change autoMotorMovementMode in centerPane.qml to false
    LogFile::instance().appendToLogFile(QString("intiateAutoMovementSetup"), LogType::interfaceLevel);

    if (m_hardwareCheckDisabled) return; // no physical target attached — skip motor setup writes

    //    1-String to Activate Auto Paper Mode:
    //    01 06 2004 0100 01  // 2004 hex to decimal is 8196 and 0100 hex to deci is 256
    //    2-String to Set up Timer:
    //    01 06 2005 0050 01  // 2005 hex to deci is 8197 and 0250 (as we required 2500 ms) hex to deci is 592
    //    Timer: 500 ms =10ms* 0050
    //    3-String to Setup Radius Area:
    //    01 06 2006 1000 01  // 2006 hex to deci is 8198 and 1000 hex to deci is 4096
    //    Radius: 100 mm = 1/10 mm * 1000
    //start motor
    m_mainWindow->modbusWriteSingleRegister(8196, 256);
    //m_mainWindow->modbusWriteSingleRegister(8197, 293); // replace 250 (hex to deci 592) with 125 (hex to deci 293)
    QString motorDurationString = QString::number(m_motor_movement_duration*100);
    bool ok;
    uint motorDurationInt = motorDurationString.toUInt();
    LogFile::instance().appendToLogFile(QString("intiateAutoMovementSetup with duration %1").arg(motorDurationInt), LogType::interfaceLevel);
    m_mainWindow->modbusWriteSingleRegister(8197, motorDurationInt);
    m_mainWindow->modbusWriteSingleRegister(8198, 4096);
}

void TachusWidget::intiateAutoMovementSighterSetup()
{
    //return; // for manual motor movement && change autoMotorMovementMode in centerPane.qml to false
    LogFile::instance().appendToLogFile(QString("intiateAutoMovementSetup"), LogType::interfaceLevel);

    if (m_hardwareCheckDisabled) return; // no physical target attached — skip motor setup writes

    //    1-String to Activate Auto Paper Mode:
    //    01 06 2004 0100 01  // 2004 hex to decimal is 8196 and 0100 hex to deci is 256
    //    2-String to Set up Timer:
    //    01 06 2005 0050 01  // 2005 hex to deci is 8197 and 0250 (as we required 2500 ms) hex to deci is 592
    //    Timer: 500 ms =10ms* 0050
    //    3-String to Setup Radius Area:
    //    01 06 2006 1000 01  // 2006 hex to deci is 8198 and 1000 hex to deci is 4096
    //    Radius: 100 mm = 1/10 mm * 1000
    //start motor
    m_mainWindow->modbusWriteSingleRegister(8196, 256);
    //m_mainWindow->modbusWriteSingleRegister(8197, 293); // replace 250 (hex to deci 592) with 125 (hex to deci 293)
    QString motorDurationString = QString::number(m_motor_movement_duration_sighter*100);
    bool ok;
    uint motorDurationInt = motorDurationString.toUInt();
    LogFile::instance().appendToLogFile(QString("intiateAutoMovementSetup with duration %1").arg(motorDurationInt), LogType::interfaceLevel);
    m_mainWindow->modbusWriteSingleRegister(8197, motorDurationInt);
    m_mainWindow->modbusWriteSingleRegister(8198, 4096);
}
bool TachusWidget::checkAutoFeedMode(bool showPopup)
{
    return true;
    LogFile::instance().appendToLogFile(QString("checkAutoFeedMode "), LogType::interfaceLevel);
    //    1-String to Activate Auto Paper Mode:
    //    01 06 2004 0100 01  // 2004 hex to decimal is 8196 and 0100 hex to deci is 256
    // 1-If Automatic Paper Feed is on or not:
    // I/P: 01 03 20 06 00 01
    uint8_t dest[1024]; //setup memory for data
    uint16_t * dest16 = (uint16_t *) dest;
    memset(dest, 0, 1024);

    m_mainWindow->modbusReadRegistry(8198, 2, dest16);
    int data_1 = dest16[1];
    int data_0 = dest16[0];
    LogFile::instance().appendToLogFile(QString("checkAutoFeedMode data[0] = %1, data[1] = %2").arg(data_0).arg(data_1), LogType::interfaceLevel);
    if (data_1 == 0 && data_0 == 0 && m_currentShootsCount <= 1 && showPopup) {
        showMessage("Connection Error, Please reconnect");
    }

    return (data_1 == 0 && data_0 == 0) ? false : true;
}

void TachusWidget::showMessage(QString string)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Error !");
    msgBox.setText(string);
    msgBox.exec();
}

void TachusWidget::changeSighterMode(bool flag)
{
    //    if (!flag) {
    //        // in game more, as interchangeable is not possible
    //        // we can clean up sighter mode data

    //        m_currentShootsCount = 0;
    //        m_oldResetCount = 0;
    //        m_xCordList.clear();
    //        m_yCordList.clear();
    //    }

    //    return; //TODO - srinivas still need to work on it
    if (isSighterMode == flag)
        return;

    removeSetaLaneShootDataFile();

    if (flag) {
        QList<double> temp_XCorList = m_xCordList;
        QList<double> temp_YCorList = m_yCordList;
        int m_currentShootsCount_temp = m_currentShootsCount;
        int m_oldResetCount_temp = m_oldResetCount;

        m_xCordList = m_xCordList_sighterMode;
        m_yCordList = m_yCordList_sighterMode;
        m_currentShootsCount = m_currentShootsCount_sighter;
        m_oldResetCount = m_oldResetCount_sighter;

        m_xCordList_gameMode = temp_XCorList;
        m_yCordList_gameMode = temp_YCorList;
        m_currentShootsCount_game = m_currentShootsCount_temp;
        m_oldResetCount_game = m_oldResetCount_temp;

        //time stamp
        m_timeConsumedList = m_timeConsumedList_sighterMode;
        m_timeStampList = m_timeStampList_sighterMode;
        m_shotsRotation = m_shotsRotation_sighterMode;
    } else {
        QList<double> temp_XCorList = m_xCordList;
        QList<double> temp_YCorList = m_yCordList;
        int m_currentShootsCount_temp = m_currentShootsCount;
        int m_oldResetCount_temp = m_oldResetCount;

        m_xCordList = m_xCordList_gameMode;
        m_yCordList = m_yCordList_gameMode;
        m_currentShootsCount = m_currentShootsCount_game;
        m_oldResetCount = m_oldResetCount_game;

        m_xCordList_sighterMode = temp_XCorList;
        m_yCordList_sighterMode = temp_YCorList;
        m_currentShootsCount_sighter = m_currentShootsCount_temp;
        m_oldResetCount_sighter = m_oldResetCount_temp;

        //time stamp
        m_timeConsumedList = m_timeConsumedList_gameMode;
        m_timeStampList = m_timeStampList_gameMode;
        m_shotsRotation = m_shotsRotation_gameMode;

        // reset live/game mode
        resetShootinCount();
        LogFile::instance().appendToLogFile("changeSighterMode: lists reset, leaving sighter mode", LogType::BackendLevel);
    }

    isSighterMode = flag;
}

void TachusWidget::appendToLogFile(QString string, LogType type)
{
    // this call is basically from QML file
    LogFile::instance().appendToLogFile(string, type);
}

void TachusWidget::connectToMaster(QString laneName)
{
    QString localhostname =  QHostInfo::localHostName();
    QString localhostIP;
    QList<QHostAddress> hostList = QHostInfo::fromName(localhostname).addresses();
    foreach (const QHostAddress& address, hostList) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address.isLoopback() == false) {
            localhostIP = address.toString();
        }
    }
    QString localMacAddress;
    QString localNetmask;
    foreach (const QNetworkInterface& networkInterface, QNetworkInterface::allInterfaces()) {
        foreach (const QNetworkAddressEntry& entry, networkInterface.addressEntries()) {
            if (entry.ip().toString() == localhostIP) {
                localMacAddress = networkInterface.hardwareAddress();
                localNetmask = entry.netmask().toString();
                break;
            }
        }
    }

    QList<QHostAddress> list = QHostInfo::fromName(QHostInfo::localHostName()).addresses();
    for (int var = 0; var < list.size(); ++var) {
        qDebug() << list[var].toString();
    }

    LogFile::instance().appendToLogFile(QString("Lane name: %1").arg(laneName), LogType::interfaceLevel);
    LogFile::instance().appendToLogFile(QString("Localhost name:  %1").arg(localhostname), LogType::interfaceLevel);
    LogFile::instance().appendToLogFile(QString("IP =  %1").arg(localhostIP), LogType::interfaceLevel);
    LogFile::instance().appendToLogFile(QString("MAC =  %1").arg(localMacAddress), LogType::interfaceLevel);
    LogFile::instance().appendToLogFile(QString("Netmask =  %1").arg(localNetmask), LogType::interfaceLevel);

    QString data = QString("laneName-%1%2systemName-%3%4 ip-%5%6 mac-%7%8 netmask-%9%10 gamemode-%11")
            .arg(laneName).arg(DATA_DELIMITER)
            .arg(localhostname).arg(DATA_DELIMITER)
            .arg(localhostIP).arg(DATA_DELIMITER)
            .arg(localMacAddress).arg(DATA_DELIMITER)
            .arg(localNetmask).arg(DATA_DELIMITER)
            .arg(getGamemode());

    m_ipAddress = localhostIP;
    Sender sender;
    sender.broadcastDatagram(data);

    // need to assign after verification
    m_laneName = laneName;
}

void TachusWidget::startTCP()
{
    m_tcpServer = new QTcpServer();
}

void TachusWidget::stopTCP()
{
    delete m_tcpServer;
    if (m_tcpServer != nullptr)
        m_tcpServer = nullptr;
}

void TachusWidget::on_pushButton_3_clicked()
{
    // no action
}

void TachusWidget::checkForNewShots(bool motorAutoMode)
{
//    LogFile::instance().appendToLogFile("Checking for new shoots", LogType::BackendLevel);
    uint8_t dest[1024]; //setup memory for data
    uint16_t * dest16 = (uint16_t *) dest;
    memset(dest, 0, 1024);

    m_mainWindow->modbusReadRegistry(8192, 2, dest16);
    int newShotsCount = dest16[1];
    //motorAutoMode = false;
    //LogFile::instance().appendToLogFile(QString("Current shoot count %1 while old shoot count was %2").arg(newShotsCount).arg(m_currentShootsCount), LogType::BackendLevel);
    if (newShotsCount == 0 && !m_hardwareCheckDisabled) {
        // check for hardware disconnection
        if (!isHardwareConnected()) {
            emit hardwareDisconnected();
            m_hardwareDisconnected = true;
            return;
        }
    }
    if (newShotsCount > m_currentShootsCount && newShotsCount - m_currentShootsCount < 2)
    {
        LogFile::instance().appendToLogFile(QString("Current shoot count %1 while old shoot count was %2").arg(newShotsCount).arg(m_currentShootsCount), LogType::BackendLevel);
        m_currentShootsCount = newShotsCount;
    }
}

int TachusWidget::getRealValue(int value)
{
    int decimal = value;
    bool ok;
    QString hex = QString::number(decimal, 16);
    int hexNum = hex.toInt(&ok, 16);
    QString binary = QString::number(hexNum, 2);
    QString binaryRightEight = binary.right(8);
    //qDebug() << "-----------------------***" << binaryRightEight;
    QString binary2C;
    if (binary.at(0) == QChar('1') && binary.length() == 16)
    {
        for (int i=0; i<16; i++)
        {
            if (binary.at(i) == QChar('1'))
                binary2C.append("0");
            else
                binary2C.append("1");
        }
        int realValue = binary2C.toInt(&ok, 2);
        return (realValue+1)*(-1);
    } else
        return decimal;
}

void TachusWidget::broadCastNewShoot(int count)
{
//    if (!isMasterSystemConnected())
//        return;

    qDebug() << __FUNCTION__ << count;
    if (count > getCurrentMatchTotalShotsCount())
        return;

    QString data = QString("shootdata %1 %2 %3 %4 %5 %6")
            .arg(m_laneName)
            .arg(count)
            .arg(getXCord(count))
            .arg(getYCord(count))
            .arg(getScore(count))
            .arg(isSighterMode);
    LogFile::instance().appendToLogFile(QString("Broadcasting %1").arg(data), LogType::interfaceLevel);
    Sender sender;
    sender.broadcastDatagram(data);
}

void TachusWidget::updateShootData(int count)
{
    if (!getIsServerNetworkEnabled())
        return;

    QString data = QString("shootdata %1 %2 %3 %4 %5 %6 \n")
            .arg(m_laneName)
            .arg(count)
            .arg(getXCord(count))
            .arg(getYCord(count))
            .arg(getScore(count))
            .arg(isSighterMode);

    qDebug() << __FUNCTION__ << count;
    QFile outputFile(m_serverLaneFilePath);
    if (outputFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream outStream(&outputFile);
        outStream << data;
        outputFile.close();
    }
}

void TachusWidget::updateSetaShootData(int count)
{
    LogFile::instance().appendToLogFile(QString("update shootdat file -> count %1\n").arg(count), LogType::interfaceLevel);
    LogFile::instance().appendToLogFile(QString("update shootdat file -> match total shoot %1\n").arg(m_currentMatchTotalShotsCount), LogType::interfaceLevel);
    if (!getIsServerNetworkEnabled())
        return;

    if (count > m_currentMatchTotalShotsCount)
        return;

    QString data = QString("shootdata,%1,%2,%3,%4,%5 \n")
            .arg(count)
            .arg(getXCord(count))
            .arg(getYCord(count))
            .arg(getScore(count))
            .arg(isSighterMode);

    qDebug() << __FUNCTION__ << count << getSetaLaneShootDataFilePath();
    QFile outputFile(getSetaLaneShootDataFilePath());
    if (outputFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream outStream(&outputFile);
        outStream << data;
        outputFile.close();
    }
}

void TachusWidget::clearTimeStampAndTimeConsumed()
{
    m_timeConsumedList.clear();
    m_timeConsumedList_gameMode.clear();
    m_timeConsumedList_sighterMode.clear();
    m_timeStampList.clear();
    m_timeStampList_gameMode.clear();
    m_timeStampList_sighterMode.clear();
}
void TachusWidget::clearShotDirection()
{
    m_shotsRotation.clear();
    m_shotsRotation_gameMode.clear();
    m_shotsRotation_sighterMode.clear();
}

void TachusWidget::appendTimeConsumed(QString data)
{
    m_timeConsumedList.append(data);
    if (isSighterMode) {
        m_timeConsumedList_sighterMode.append(data);
    } else {
        m_timeConsumedList_gameMode.append(data);
    }
}

void TachusWidget::appendTimeStamp(QString data)
{
    m_timeStampList.append(data);
    if (isSighterMode) {
        m_timeStampList_sighterMode.append(data);
    } else {
        m_timeStampList_gameMode.append(data);
    }
}

void TachusWidget::appendShotDirection(int direction)
{
    m_shotsRotation.append(direction);
    if (isSighterMode) {
        m_shotsRotation_sighterMode.append(direction);
    } else {
        m_shotsRotation_gameMode.append(direction);
    }
}

void TachusWidget::clearShootCount()
{
    LogFile::instance().appendToLogFile(QString("Reset shoot is called, old totol shoot count %1").arg(m_oldResetCount), LogType::interfaceLevel);
    m_oldResetCount = m_oldResetCount + m_currentShootsCount;
    LogFile::instance().appendToLogFile(QString("Reset shoot is called, Current shoot count %1").arg(m_currentShootsCount), LogType::interfaceLevel);
    // reset the hardware counter register (2001 Hex = 8193 decimal)
    // skip if hardware check is disabled (no physical target attached)
    if (!m_hardwareCheckDisabled) {
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (m_mainWindow->modbusWriteSingleRegister(8193, 0) != -1)
                break;
            LogFile::instance().appendToLogFile(
                QString("clearShootCount: hw counter reset failed (attempt %1)").arg(attempt + 1),
                LogType::BackendLevel);
        }
    }
    //checkForNewShots();
    m_currentShootsCount = 0;
    LogFile::instance().appendToLogFile(QString("Reset done, Current shoot count %1").arg(m_currentShootsCount), LogType::interfaceLevel);
}

QString TachusWidget::getEncryptedText(QString data, bool onlyDefault)
{
    QString result;

    if (onlyDefault) {
        //result.append(data.toUtf8());
        for (int i=0; i<data.size(); ++i) {
            result.append(QChar(data.at(i).unicode()+ENCRYPTION_DEFAULT));
        }
    } else {
//        for (int i=0; i<data.size(); ++i) {
//            int curIndex = i%m_encryption_text.size();
//            result.append(data.at(i).toLatin1()+m_encryption_text.at(curIndex).toLatin1());
//        }
    }

    return result;
}

QString TachusWidget::getDencryptedText(QString data, QString encryptionText, bool onlyDefault)
{
    QString result;

    //if (onlyDefault) {
        for (int i=0; i<data.size(); ++i) {
            result.append(QChar(data.at(i).unicode()-ENCRYPTION_DEFAULT));
        }
//    } else {
//        for (int i=0; i<data.size(); ++i) {
//            int curIndex = i%encryptionText.size();
//            result.append(data.at(i).unicode()-encryptionText.at(curIndex).unicode());
//        }
//    }

        return result;
}

void TachusWidget::licValidated()
{
    QFile file("licence.txt");
    QFileInfo fInfo(file);

    if (!fInfo.exists() || !file.open(QIODevice::Append))
        return;

    QTextStream stream(&file);
    stream << Qt::endl;
    stream << getEncryptedText("valid", true);
    file.close();
}

bool TachusWidget::getIsServerNetworkEnabled() const
{
    // Sharing needs a destination: with no share folder configured the
    // composed lane-file paths degenerate to "/<file>" on the drive root,
    // where antivirus write-blocking can stall the UI thread indefinitely.
    // Treat "no folder" as sharing disabled. This gates every share write
    // in both TachusWidget and AppSettings.
    return m_isServerNetworkEnabled && !m_setaServerPath.isEmpty();
}

void TachusWidget::setIsServerNetworkEnabled(bool isServerNetworkEnabled)
{
    m_isServerNetworkEnabled = isServerNetworkEnabled;
}

QString TachusWidget::getSetaLaneEachScoreDataFilePath() const
{
    return m_setaLaneEachScoreDataFilePath;
}

void TachusWidget::setSetaLaneEachScoreDataFilePath(const QString &setaLaneEachScoreDataFilePath)
{
    m_setaLaneEachScoreDataFilePath = setaLaneEachScoreDataFilePath;
}

double TachusWidget::getRed_zone_start() const
{
    return m_red_zone_start;
}

void TachusWidget::setRed_zone_start(double red_zone_start)
{
    m_red_zone_start = red_zone_start;
}

double TachusWidget::getRed_zone_end() const
{
    return m_red_zone_end;
}

void TachusWidget::setRed_zone_end(double red_zone_end)
{
    m_red_zone_end = red_zone_end;
}

double TachusWidget::getYellow_zone_end() const
{
    return m_yellow_zone_end;
}

void TachusWidget::setYellow_zone_end(double yellow_zone_end)
{
    m_yellow_zone_end = yellow_zone_end;
}

double TachusWidget::getYellow_zone_start() const
{
    return m_yellow_zone_start;
}

void TachusWidget::setYellow_zone_start(double yellow_zone_start)
{
    m_yellow_zone_start = yellow_zone_start;
}

double TachusWidget::getGreen_zone_end() const
{
    return m_green_zone_end;
}

void TachusWidget::setGreen_zone_end(double green_zone_end)
{
    m_green_zone_end = green_zone_end;
}

double TachusWidget::getGreen_zone_start() const
{
    return m_green_zone_start;
}

void TachusWidget::setGreen_zone_start(double green_zone_start)
{
    m_green_zone_start = green_zone_start;
}

int TachusWidget::getShot_interval() const
{
    return m_shot_interval;
}

void TachusWidget::setShot_interval(int shot_interval)
{
    m_shot_interval = shot_interval;
}
int TachusWidget::getSeries_end_at() const
{
    return m_series_end_at;
}

void TachusWidget::setSeries_end_at(int series_end_at)
{
    m_series_end_at = series_end_at;
}

int TachusWidget::getSeries_start_at() const
{
    return m_series_start_at;
}

void TachusWidget::setSeries_start_at(int series_start_at)
{
    m_series_start_at = series_start_at;
}

bool TachusWidget::getIsAppDemoMode() const
{
    return isAppDemoMode;
}

int TachusWidget::getShotPerSeries() const
{
    return m_shotPerSeries;
}

void TachusWidget::setShotPerSeries(int shotPerSeries)
{
    m_shotPerSeries = shotPerSeries;
}

double TachusWidget::getMatch_distance_new() const
{
    return m_match_distance_new;
}

void TachusWidget::setMatch_distance_new(double match_distance_new)
{
    m_match_distance_new = match_distance_new;
}

QStringList TachusWidget::getPDFString()
{
    QStringList result;
    result.append("Sr No.###Score###X(mm)###Y(mm)###Teiler###Time Stamp(s)");
    result.append("--");

    QString deliminater = "###";
    int seriesIndex = 1;
    qDebug() << __LINE__ << m_scoreList_gameMode.count();
    for(int i=1; i<=m_scoreList_gameMode.count(); ++i) {
        QString data;
        data.append(QString::number(i));
        data.append(deliminater);
        data.append(QString::number(getScore(i)));
        //data.append(deliminater);
        data.append(QString("direction%1").arg(m_shotsRotation.at(i-1)));
        data.append(deliminater);
        data.append(QString::number(getXCord(i)));
        data.append(deliminater);
        data.append(QString::number(getYCord(i)));
        data.append(deliminater);
        data.append(QString::number(getTeilerForShoot(seriesIndex, (i-1)%m_shotPerSeries)));
        data.append(deliminater);
        data.append(m_timeConsumedList[i-1]);

        result.append(data);
        if ((i)%m_shotPerSeries == 0) {
            result.append("--");
            result.append(QString("Series No.: %1    Series Total: %2(%3)  Group: %4mm   MPI: %5,%6").
                          arg(seriesIndex).
                          arg(m_seriesScoreWD.value(seriesIndex, 0)).
                          arg(m_seriesScore.value(seriesIndex, 0)).
                          arg(getGroup(seriesIndex-1)).arg(QString::number(getXMPI(seriesIndex), 'f', 2)).
                          arg(QString::number(getYMPI(seriesIndex), 'f', 2)));
            result.append("--");
            seriesIndex++;
//            if (seriesIndex == 5 /*|| seriesIndex == 5*/) {
//                result.append("newpage");
//                result.append("Sr No.###Score###X(mm)###Y(mm)###Teiler###Time Stamp");
//                result.append("--");
//            }
        } else if (i==m_scoreList_gameMode.count()) {
            result.append("--");
            result.append(QString("Series No.: %1    Series Total: %2(%3)  Group: %4mm   MPI: %5,%6").
                          arg(seriesIndex).
                          arg(m_seriesScoreWD.value(seriesIndex, 0)).
                          arg(m_seriesScore.value(seriesIndex, 0)).
                          arg(getGroup(seriesIndex-1)).arg(QString::number(getXMPI(seriesIndex), 'f', 2)).
                          arg(QString::number(getYMPI(seriesIndex), 'f', 2)));
            result.append("--");
            seriesIndex++;
        }
    }

    return result;
}

QStringList TachusWidget::getSeriesComparisionData()
{
    QStringList result;
    int diff = getSeries_end_at() - getSeries_start_at() + 1;
    if (diff < 2 || diff == 6)
    {
        result.append("Shot No.###Series 1###Series 2###Series 3###Series 4###Series 5###Series 6");
        result.append("--");

        QString deliminater = "###";
        int seriesIndex = 1;
        for(int i=1; i<=m_shotPerSeries; ++i) {
            QString data;
            data.append(QString::number(i));
            data.append(deliminater);
            for (int j=1; j<=6; ++ j) {
                // check validity of shoots
                int currentShotIndex = (j-1)*m_shotPerSeries+i;
                if (m_scoreList_gameMode.count() >= currentShotIndex) {
                    data.append(QString::number(getScore(currentShotIndex)));
                    //data.append(deliminater);
                    data.append(QString("direction%1").arg(m_shotsRotation.at(currentShotIndex-1)));
                    if (j != 6)
                        data.append(deliminater);
                } else {
                    data.append("");
                    if (j != 6)
                        data.append(deliminater);
                }
            }

            result.append(data);
        }
        result.append("--");

        qDebug() << __LINE__ << result;
        return result;
    } else {

    }
}

QStringList TachusWidget::getShotIntervalData()
{
    QStringList result;
    result.append("Interval (In Seconds)###Number of Shots###Average Score");
    result.append("--");

    QMap<int, QList<int> > intervalShotIndexList;
    QList<int> tempList;
    intervalShotIndexList[20] = tempList;
    intervalShotIndexList[40] = tempList;
    intervalShotIndexList[60] = tempList;
    intervalShotIndexList[80] = tempList;
    intervalShotIndexList[100] = tempList;
    intervalShotIndexList[120] = tempList;
    for (int i=0; i < m_timeConsumedList.count(); ++i) {
        double curTimeConsumed = QString(m_timeConsumedList.at(i)).toDouble();
        if (curTimeConsumed <= 20) {
            tempList = intervalShotIndexList[20];
            tempList.append(i);
            intervalShotIndexList[20] = tempList;
        } else if (curTimeConsumed <= 40) {
            tempList = intervalShotIndexList[40];
            tempList.append(i);
            intervalShotIndexList[40] = tempList;
        } else if (curTimeConsumed <= 60) {
            tempList = intervalShotIndexList[60];
            tempList.append(i);
            intervalShotIndexList[60] = tempList;
        } else if (curTimeConsumed <= 80) {
            tempList = intervalShotIndexList[80];
            tempList.append(i);
            intervalShotIndexList[80] = tempList;
        } else if (curTimeConsumed <= 100) {
            tempList = intervalShotIndexList[100];
            tempList.append(i);
            intervalShotIndexList[100] = tempList;
        } else {
            tempList = intervalShotIndexList[120];
            tempList.append(i);
            intervalShotIndexList[120] = tempList;
        }
    }
    QString deliminater = "###";
    QMap<int, QList<int> >::iterator iter = intervalShotIndexList.begin();
    for (; iter !=intervalShotIndexList.end(); ++iter) {
        QString data;
        QList<int> indexList = iter.value();
        int keyValue = iter.key();
        if (keyValue == 20)
            data.append("0-20");
        else if (keyValue == 40)
            data.append("21-40");
        else if (keyValue == 60)
            data.append("41-60");
        else if (keyValue == 80)
            data.append("61-80");
        else if (keyValue == 100)
            data.append("81-100");
        else
            data.append("100+");

        data.append(deliminater);
        double sum = 0;
        for (int j=0; j<indexList.count(); ++j) {
            int index = indexList.at(j);
            sum += getScore(index+1);
        }

        data.append(QString::number(indexList.count()));
        data.append(deliminater);
        double avg = sum == 0 ? 0 : sum/indexList.count();
        data.append(QString::number(avg, 'f', 2));

        result.append(data);
        qDebug() << __LINE__ << indexList;
        qDebug() << intervalShotIndexList;
    }
//    for(int i=1; i<=m_shotPerSeries; ++i) {
//        QString data;
//        data.append(QString::number(i));
//        data.append(deliminater);
//        for (int j=1; j<=6; ++ j) {
//            // check validity of shoots
//            int currentShotIndex = (j-1)*m_shotPerSeries+i;
//            if (m_scoreList_gameMode.count() >= currentShotIndex) {
//                data.append(QString::number(getScore(currentShotIndex)));
//                //data.append(deliminater);
//                data.append(QString("direction%1").arg(m_shotsRotation.at(currentShotIndex-1)));
//                if (j != 6)
//                    data.append(deliminater);
//            } else
//                break;
//        }

//        result.append(data);
//    }
    result.append("--");

    qDebug() << __LINE__ << result;
    return result;

}

QStringList TachusWidget::getTimeSeriesData()
{
    QStringList result;
    result.append("Series Number###Series (Total)###Total Time/Series (In Secs)");
    result.append("--");

    QMap<int, QList<int> > intervalShotIndexList;
    QList<int> tempList;
    intervalShotIndexList[1] = tempList;
    intervalShotIndexList[2] = tempList;
    intervalShotIndexList[3] = tempList;
    intervalShotIndexList[4] = tempList;
    intervalShotIndexList[5] = tempList;
    intervalShotIndexList[6] = tempList;
    for (int i=0; i < m_timeConsumedList.count(); ++i) {
        double curTimeConsumed = QString(m_timeConsumedList.at(i)).toDouble();
        if (i < m_shotPerSeries*1 - 1) {
            tempList = intervalShotIndexList[1];
            tempList.append(curTimeConsumed);
            intervalShotIndexList[1] = tempList;
        } else if (i < m_shotPerSeries*2 - 1) {
            tempList = intervalShotIndexList[2];
            tempList.append(curTimeConsumed);
            intervalShotIndexList[2] = tempList;
        } else if (i < m_shotPerSeries*3 - 1) {
            tempList = intervalShotIndexList[3];
            tempList.append(curTimeConsumed);
            intervalShotIndexList[3] = tempList;
        } else if (i < m_shotPerSeries*4 - 1) {
            tempList = intervalShotIndexList[4];
            tempList.append(curTimeConsumed);
            intervalShotIndexList[4] = tempList;
        } else if (i < m_shotPerSeries*5 - 1) {
            tempList = intervalShotIndexList[5];
            tempList.append(curTimeConsumed);
            intervalShotIndexList[5] = tempList;
        } else {
            tempList = intervalShotIndexList[6];
            tempList.append(curTimeConsumed);
            intervalShotIndexList[6] = tempList;
        }
    }
    QString deliminater = "###";
    QMap<int, QList<int> >::iterator iter = intervalShotIndexList.begin();
    for (; iter !=intervalShotIndexList.end(); ++iter) {
        QString data;
        QList<int> timeConsumedList = iter.value();
        int keyValue = iter.key();
        data.append(QString::number(keyValue));
        data.append(deliminater);
        double sum = 0;
        for (int j=0; j<timeConsumedList.count(); ++j) {
            sum += timeConsumedList.at(j);
        }

        data.append(QString::number(m_seriesScoreWD.value(keyValue, 0)));
        data.append(deliminater);
        double avg = sum == 0 ? 0 : sum;
        data.append(QString::number(avg, 'f', 1));

        result.append(data);
    }
//    for(int i=1; i<=m_shotPerSeries; ++i) {
//        QString data;
//        data.append(QString::number(i));
//        data.append(deliminater);
//        for (int j=1; j<=6; ++ j) {
//            // check validity of shoots
//            int currentShotIndex = (j-1)*m_shotPerSeries+i;
//            if (m_scoreList_gameMode.count() >= currentShotIndex) {
//                data.append(QString::number(getScore(currentShotIndex)));
//                //data.append(deliminater);
//                data.append(QString("direction%1").arg(m_shotsRotation.at(currentShotIndex-1)));
//                if (j != 6)
//                    data.append(deliminater);
//            } else
//                break;
//        }

//        result.append(data);
//    }
    result.append("--");

    qDebug() << __LINE__ << result;
    return result;
}

QStringList TachusWidget::getZoneTableData()
{
    QStringList result;
    result.append("Shot No.###Score###Shot No.###Score###Shot No.###Score");
    result.append("--");

    QList<int> tempList1;
    QList<int> tempList2;
    QList<int> tempList3;
    for (int i=0; i < m_scoreList_gameMode.count(); ++i) {
        double curScore = getScore(i+1);
        if (curScore >= 10.0) {
            tempList1.append(i);
        } else if (curScore >= 9.0) {
            tempList2.append(i);
        } else {
            tempList3.append(i);
        }
    }
    QString deliminater = "###";
    for (int i=0; i < m_scoreList_gameMode.count(); ++i) {
        QString data;
        bool allBlank = true;
        if (tempList1.count() > i) {
            int curIndex = tempList1.at(i)+1;
            data.append(QString::number(curIndex));
            data.append(deliminater);
            data.append(QString::number(getScore(curIndex)));
            data.append(deliminater);
            allBlank = false;
        } else {
            data.append(" ");
            data.append(deliminater);
            data.append(" ");
            data.append(deliminater);
        }
        if (tempList2.count() > i) {
            int curIndex = tempList2.at(i)+1;
            data.append(QString::number(curIndex));
            data.append(deliminater);
            data.append(QString::number(getScore(curIndex)));
            data.append(deliminater);
            allBlank = false;
        } else {
            data.append(" ");
            data.append(deliminater);
            data.append(" ");
            data.append(deliminater);
        }
        if (tempList3.count() > i) {
            int curIndex = tempList3.at(i)+1;
            data.append(QString::number(curIndex));
            data.append(deliminater);
            data.append(QString::number(getScore(curIndex)));
            allBlank = false;
        } else {
            data.append(" ");
            data.append(deliminater);
            data.append(" ");
        }

        result.append(data);
        if (allBlank)
            break;
    }

//    result.append("--");

    qDebug() << __LINE__ << result;
    return result;
}
void TachusWidget::setTotalScoreWD(double totalScoreWD)
{
    m_totalScoreWD = totalScoreWD;
}

void TachusWidget::updateSeriesScore(int index, int value)
{
    m_seriesScore[index] = value;
}

void TachusWidget::updateSeriesScoreWD(int index, double value)
{
    m_seriesScoreWD[index] = value;
}

void TachusWidget::setTotalScoreWOD(int totalScoreWOD)
{
    m_totalScoreWOD = totalScoreWOD;
}

QString TachusWidget::getSetaLaneScoreSummaryFilePath() const
{
    return m_setaLaneScoreSummaryFilePath;
}

void TachusWidget::setSetaLaneScoreSummaryFilePath(const QString &setaLaneScoreSummaryFilePath)
{
    m_setaLaneScoreSummaryFilePath = setaLaneScoreSummaryFilePath;
}

bool TachusWidget::getIsSingleDecimal() const
{
    return m_isSingleDecimal;
}

void TachusWidget::setIsSingleDecimal(bool isSingleDecimal)
{
    m_isSingleDecimal = isSingleDecimal;
}

QString TachusWidget::getSetaLaneShootDataFilePath() const
{
    return m_setaLaneShootDataFilePath;
}

void TachusWidget::setSetaLaneShootDataFilePath(const QString &setaLaneShootDataFilePath)
{
    LogFile::instance().appendToLogFile(QString("setSetaLaneShootDataFilePath %1").arg(setaLaneShootDataFilePath), LogType::interfaceLevel);
//    QFile::remove(setaLaneShootDataFilePath);
    m_setaLaneShootDataFilePath = setaLaneShootDataFilePath;
    removeAllShootdatForThisLane();
}

void TachusWidget::removeSetaLaneShootDataFile()
{
    LogFile::instance().appendToLogFile(QString("removeSetaLaneShootDataFile %1"), LogType::interfaceLevel);
    removeAllShootdatForThisLane();
//    QFile::remove(getSetaLaneShootDataFilePath());
}

void TachusWidget::removeAllShootdatForThisLane()
{
    if (m_setaServerPath.isEmpty())
        return;

    LogFile::instance().appendToLogFile(QString("removeAllShootdatForThisLane %1").arg(m_setaServerPath), LogType::interfaceLevel);
    QDir dir(m_setaServerPath);

    QString filePath = getSetaLaneShootDataFilePath();
    QString fileName = filePath.section("/", -1);
    fileName = fileName.section("_", 0, 1);
    QStringList namefilter;
    namefilter << QString("%1*").arg(fileName);
    QStringList files = dir.entryList(namefilter, QDir::Files);
    for (int i=0; i<files.count(); ++i) {
        dir.remove(files.at(i));
    }
}

QString TachusWidget::getSetaLaneStatusPath() const
{
    return m_setaLaneStatusPath;
}

void TachusWidget::setSetaLaneStatusPath(const QString &setaLaneStatusPath)
{
    m_setaLaneStatusPath = setaLaneStatusPath;
}

QString TachusWidget::getSetaServerSettingPath() const
{
    return m_setaServerSettingPath;
}

void TachusWidget::setSetaServerSettingPath(const QString &setaServerSettingPath)
{
    m_setaServerSettingPath = setaServerSettingPath;
}

QString TachusWidget::getSetaServerPath() const
{
    return m_setaServerPath;
}

void TachusWidget::setSetaServerPath(const QString &setaServerPath)
{
    m_setaServerPath = setaServerPath;
}

QString TachusWidget::getLaneName() const
{
    return m_laneName;
}

void TachusWidget::setLaneName(const QString &laneName)
{
    qDebug() <<__FUNCTION__<<__LINE__<< laneName;
    m_laneName = laneName;
}

QString TachusWidget::getServerLaneFilePath() const
{
    return m_serverLaneFilePath;
}

void TachusWidget::setServerLaneFilePath(const QString &serverLaneFilePath)
{
    m_serverLaneFilePath = serverLaneFilePath;
}

QString TachusWidget::getServerPath() const
{
    return m_serverSettingsFilePath;
}

void TachusWidget::setServerPath(const QString &serverPath)
{
    m_serverSettingsFilePath = serverPath;
}

int TachusWidget::getGame_range() const
{
    return m_game_range;
}

void TachusWidget::setGame_range(int game_range)
{
    m_game_range = game_range;
}

double TachusWidget::getFormatedSCore(double value)
{
//    if (!getIsSingleDecimal())
//        return value;
    //qDebug() <<__LINE__<< __FUNCTION__ << value;
    QString valueString = QString::number(value);
    //qDebug() <<__LINE__<< __FUNCTION__ << valueString;
    if (!valueString.contains("."))
        return value;

//    qDebug() << __LINE__ << __FUNCTION__ << value;
    QStringList splitString = valueString.split('.');
    if (splitString.count() != 2)
        return value;

//    qDebug() << __LINE__ << __FUNCTION__ << value;
    QString firstChar = QString(splitString.at(1)).at(0);
    QString resultValueString = QString("%1.%2").arg(splitString.at(0)).arg(firstChar);
    qDebug() << __LINE__ << __FUNCTION__ << resultValueString;

    return resultValueString.toDouble();
}

double TachusWidget::getFormatedValueFoeTwoDecimal(double value)
{
    return value;
//    value = value + 0.00001;
//    qDebug() << __LINE__ << __FUNCTION__ << value;
    QString valueString = QString::number(value);
    if (!valueString.contains("."))
        return value;
//    qDebug() << __LINE__ << __FUNCTION__ << value;

    QStringList splitString = valueString.split('.');
    if (splitString.count() != 2)
        return value;
//    qDebug() << __LINE__ << __FUNCTION__ << value;

    QString afterDecimal = splitString.at(1);
    if (afterDecimal.length() < 2)
        return value;

//    qDebug() << __LINE__ << __FUNCTION__ << value;
    QString firstChar = QString(afterDecimal).at(0);
    QString secondChar = QString(afterDecimal).at(1);
    QString resultValueString = QString("%1.%2%3").arg(splitString.at(0)).arg(firstChar).arg(secondChar);
    qDebug() << __LINE__ << __FUNCTION__ << resultValueString;

    return resultValueString.toDouble();
}

int TachusWidget::getGame_distance() const
{
    return m_game_distance;
}

void TachusWidget::setGame_distance(int game_distance)
{
    m_game_distance = game_distance;
}

int TachusWidget::getCurrentMatchTotalShotsCount() const
{
    return m_currentMatchTotalShotsCount;
}

void TachusWidget::setCurrentMatchTotalShotsCount(int currentMatchTotalShotsCount)
{
    m_currentMatchTotalShotsCount = currentMatchTotalShotsCount;
}

void TachusWidget::saveNameAndPort(QString name, QString port, QString networkPath)
{
    QString appPath = QCoreApplication::applicationDirPath();
    QString filePath = QString("%1/%2").arg(appPath).arg(USER_DETAILS);
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly))
        return;

    name = name.isEmpty() ? " " : name;
    port = port.isEmpty() ? " " : port;
    networkPath = networkPath.isEmpty() ? " " : networkPath;

    QTextStream stream(&file);
    stream << name;
    stream << DATA_DELIMITER;
    stream << port;
    stream << DATA_DELIMITER;
    stream << networkPath;
    file.close();
}

QString TachusWidget::getUserName()
{
    QString appPath = QCoreApplication::applicationDirPath();
    QString filePath = QString("%1/%2").arg(appPath).arg(USER_DETAILS);
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QString data = file.readAll();
    QStringList dataList = data.split(DATA_DELIMITER);
    if (dataList.count()) {
        return dataList.at(0);
    }

    return QString();
}

QString TachusWidget::getPortNumber()
{
    QString appPath = QCoreApplication::applicationDirPath();
    QString filePath = QString("%1/%2").arg(appPath).arg(USER_DETAILS);
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QString data = file.readAll();
    QStringList dataList = data.split(DATA_DELIMITER);
    if (dataList.count() >= 2) {
        return dataList.at(1);
    }

    return QString();
}

QString TachusWidget::getNetworkPath()
{
    QString appPath = QCoreApplication::applicationDirPath();
    QString filePath = QString("%1/%2").arg(appPath).arg(USER_DETAILS);
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QString data = file.readAll();
    QStringList dataList = data.split(DATA_DELIMITER);
    if (dataList.count() == 3) {
        QString path = dataList.at(2);
        path = path.simplified();
        return path;
    }

    return QString();
}

void TachusWidget::updateSetaShootSummaryData()
{
    if (!getIsServerNetworkEnabled())
        return;

    qDebug() << __FUNCTION__ << getSetaLaneScoreSummaryFilePath();
    QFile outputFile(getSetaLaneScoreSummaryFilePath());
    if (outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qDebug() << __FUNCTION__ << __LINE__;
        QTextStream outStream(&outputFile);
        outStream << QString("Name,%1").arg(m_laneName) <<"\n";
        outStream << QString("score1,%1").arg(m_totalScoreWOD)<<"\n";
        outStream << QString("score2,%1").arg(m_totalScoreWD)<<"\n";
        outStream << QString("series1,%1").arg(m_seriesScore.value(1, 0))<<"\n";
        outStream << QString("series2,%1").arg(m_seriesScore.value(2, 0))<<"\n";
        outStream << QString("series3,%1").arg(m_seriesScore.value(3, 0))<<"\n";
        outStream << QString("series4,%1").arg(m_seriesScore.value(4, 0))<<"\n";
        outStream << QString("series5,%1").arg(m_seriesScore.value(5, 0))<<"\n";
        outStream << QString("series6,%1").arg(m_seriesScore.value(6, 0))<<"\n";
        outStream << QString("Dseries1,%1").arg(m_seriesScoreWD.value(1, 0))<<"\n";
        outStream << QString("Dseries2,%1").arg(m_seriesScoreWD.value(2, 0))<<"\n";
        outStream << QString("Dseries3,%1").arg(m_seriesScoreWD.value(3, 0))<<"\n";
        outStream << QString("Dseries4,%1").arg(m_seriesScoreWD.value(4, 0))<<"\n";
        outStream << QString("Dseries5,%1").arg(m_seriesScoreWD.value(5, 0))<<"\n";
        outStream << QString("Dseries6,%1").arg(m_seriesScoreWD.value(6, 0))<<"\n";
        outputFile.close();
    }

    updateSetaEachShootData();
}

void TachusWidget::updateSetaEachShootData()
{
    if (!getIsServerNetworkEnabled())
        return;

    qDebug() << __FUNCTION__ << getSetaLaneEachScoreDataFilePath();
    QFile outputFile(getSetaLaneEachScoreDataFilePath());
    if (outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qDebug() << __FUNCTION__ << __LINE__;
        QTextStream outStream(&outputFile);
        for (int i=0; i<m_xCordList.count(); ++i) {
            QString score = "";
            if (isSighterMode) {
                if (!m_scoreList_sighterMode.isEmpty()) {
                    score = getGermanDecimalNumber(QString::number(m_scoreList_sighterMode.at(i)));
                }
            } else {
                score = getGermanDecimalNumber(QString::number(m_scoreList_gameMode.at(i)));
            }

            QString xCor = getGermanDecimalNumber(QString::number(m_xCordList.at(i)));
            QString yCor = getGermanDecimalNumber(QString::number(m_yCordList.at(i)));
            outStream << QString("shot%1;%2;%3;%4").arg(i+1).arg(xCor).arg(yCor).arg(score)<<"\n";
        }
    }
    outputFile.close();
}

int TachusWidget::getGamemode() const
{
    return m_gamemode;
}

void TachusWidget::setGamemode(int gamemode)
{
    m_gamemode = gamemode;
}

bool TachusWidget::getOnLoginPage() const
{
    return m_onLoginPage;
}

QString TachusWidget::getGermanDecimalNumber(QString data)
{
    if (data.contains(".")) {
        data.replace(".",",");
    }

    return data;
}

void TachusWidget::setOnLoginPage(bool onLoginPage)
{
    m_onLoginPage = onLoginPage;
}

void TachusWidget::setIsAppDemoMode(bool value)
{
    isAppDemoMode = value;
}

void TachusWidget::attemptReconnection()
{
    LogFile::instance().appendToLogFile(QString("attemptReconnection"), LogType::interfaceLevel);
    bool modBusConnected = false;
    bool hardwareConnected = false;
    bool autoFeedOn = false;
    while(1) { //unless the hardware get reconnected
        if (!modBusConnected) {
            modBusConnected = connectedModbus();
            LogFile::instance().appendToLogFile(QString("Modbus connection status -> %1").arg(modBusConnected), LogType::interfaceLevel);
        }
        if (!hardwareConnected) {
            hardwareConnected = isHardwareConnected();
            LogFile::instance().appendToLogFile(QString("Hardware connection status -> %1").arg(hardwareConnected), LogType::interfaceLevel);
        }
        if (!autoFeedOn && hardwareConnected) {
            intiateAutoMovementSetup();
            autoFeedOn = checkAutoFeedMode(false);
            LogFile::instance().appendToLogFile(QString("Auto feed status -> %1").arg(autoFeedOn), LogType::interfaceLevel);
        }
        if (modBusConnected && hardwareConnected && autoFeedOn) {
            emit hardwareReconnected();
            m_hardwareDisconnected = false;
            return;
        }
    }
}

void TachusWidget::setIsMasterConnected(bool isMasterConnected)
{
    m_isMasterConnected = isMasterConnected;
    emit masterConnectionChanged(isMasterConnected);
}

QString TachusWidget::getIpAddress() const
{
    return m_ipAddress;
}
