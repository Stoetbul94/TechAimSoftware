#include "logfile.h"
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QDebug>

QFile* LogFile::m_file = NULL;

// LOG-DEFECT-007. The field log has to be trustworthy before the next physical
// test, because the last one was diagnosed from it.
//
// Two defects were fixed here, both of which cost evidence on 2026-08-23:
//
//   1. appendToLogFile() chose its label with `=` instead of `==`:
//
//          } else if (type = LogType::BackendLevel) {
//
//      An assignment, always true, so EVERY non-QML entry was written with the
//      "Backend" prefix - interface and error entries included - and LogType
//      was silently overwritten in the caller's copy. Every "Backend" line in
//      the four-tablet evidence has to be read with that in mind.
//
//   2. writeToLogFile() dropped entries. It opened and closed the file for
//      every line, and if it ever found the handle already open it appended to
//      a member string and returned - a buffer that was only ever cleared, and
//      never written. Under a 100 ms acquisition poll writing from one thread
//      while the GUI wrote from another, that is a silent, unbounded loss of
//      exactly the lines an incident is reconstructed from.
//
// The file is now opened once, written under a mutex and flushed per line, so
// a log entry that was produced is a log entry that is on disk.
static QMutex& logMutex()
{
    static QMutex m;
    return m;
}

LogFile::LogFile(QObject *parent) //: QObject(parent)
{
    Q_UNUSED(parent);
}

void LogFile::writeToLogFile(const QString &data)
{
    QMutexLocker lock(&logMutex());
    if (!m_file)
        return;

    if (!m_file->isOpen()) {
        if (!m_file->open(QIODevice::Append | QIODevice::Text))
            return;                 // nothing can be done, and nothing is lost
                                    // silently: the file simply never opened
    }

    // Flushed per line. A log that is a few milliseconds behind is useless for
    // reconstructing the last second before a crash.
    QTextStream out(m_file);
    out << data;
    out.flush();
    m_file->flush();
}

void LogFile::appendToLogFile(QString string, LogType type)
{
    if (m_file == NULL)
        return;

    const QString dateTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString logString;
    switch (type) {
    case LogType::UXLevel:
        logString = QStringLiteral("QML %1 -> %2").arg(dateTime, string);
        break;
    case LogType::BackendLevel:
        logString = QStringLiteral("Backend %1 -> %2").arg(dateTime, string);
        break;
    case LogType::interfaceLevel:
        logString = QStringLiteral("interface %1 -> %2").arg(dateTime, string);
        break;
    case LogType::ErrorLevel:
        logString = QStringLiteral("ERROR %1 -> %2").arg(dateTime, string);
        break;
    }

    logString.append(QLatin1Char('\n'));
    writeToLogFile(logString);
}
