#include "logfile.h"
#include <QTextStream>
#include <QDebug>

QFile* LogFile::m_file = NULL;

LogFile::LogFile(QObject *parent) //: QObject(parent)
{

}

void LogFile::writeToLogFile(const QString &data)
{
    if (m_file->isOpen())
    {
        m_previousData.append(data);
        return;
    }

    if (m_file->open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream outputStream(m_file);
        outputStream << data;
        m_previousData.clear();
        m_file->close();
    }
}

void LogFile::appendToLogFile(QString string, LogType type)
{
    qDebug() << string;
    if (m_file == NULL)
        return;

    QString dateTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString logString;
    if (type == LogType::UXLevel) {
        logString = QString("QML %1 -> %2").arg(dateTime).arg(string);
    } else if (type = LogType::BackendLevel) {
        logString = QString("Backend %1 -> %2").arg(dateTime).arg(string);
    }else if (type = LogType::interfaceLevel) {
        logString = QString("interface %1 -> %2").arg(dateTime).arg(string);
    }

    logString.append("\n");

    writeToLogFile(logString);
}
