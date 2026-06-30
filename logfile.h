#ifndef LOGFILE_H
#define LOGFILE_H

#include <QObject>
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <QDebug>

enum LogType
{
    UXLevel = 0,
    BackendLevel,
    interfaceLevel,
    ErrorLevel
};

class LogFile //: public QObject
{
    //Q_OBJECT
public:
    static QFile* m_file;
    static LogFile& instance()
    {
        static LogFile staticLog;
        QString filePath = QString("%1/tachus_log%2.log").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                .arg(QDateTime::currentDateTime().toString("ddMMyyyy-hhmmss"));
        if (m_file == NULL)
            m_file = new QFile(filePath);
//        qDebug() << __FUNCTION__ << filePath;
        return staticLog;
    }
    void writeToLogFile(const QString& data);
    void createLogFile();
    void appendToLogFile(QString string, LogType type);

//signals:

//public slots:

private:
    explicit LogFile(QObject *parent = nullptr);
    QString m_previousData;
};

#endif // LOGFILE_H
