// LOG-DEFECT-007 — the next field evidence has to be trustworthy.
//
// The 2026-08-23 logs were not. Two defects, both silent:
//
//   appendToLogFile() chose its label with `=` instead of `==`, so the first
//   non-UX branch always matched: every interface and error entry was written
//   as "Backend", AND the caller's LogType argument was overwritten on the way
//   through. Reading those logs there was no way to tell an interface event
//   from a backend one - the categories were a fiction.
//
//   writeToLogFile() appended into a member QString and returned whenever it
//   found the handle already open. Nothing ever wrote that string out. Under
//   the 100 ms poll plus the motor and flush threads, entries disappeared with
//   no gap, no marker and no error - the log simply did not contain what the
//   application did.
//
// These run the REAL LogFile against a temporary file, not a copy of its logic.
#include "test_support.h"
#include "logfile.h"

#include <QFile>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>

namespace {

QStringList readBack(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QStringList();
    QStringList out;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (!line.isEmpty()) out.append(line);
    }
    return out;
}

// One writer thread, so the concurrency case exercises the same path the
// acquisition poll, the motor and the flush all go through.
class Writer : public QThread
{
public:
    Writer(const QString& tag, int count, LogType type)
        : m_tag(tag), m_count(count), m_type(type) {}
protected:
    void run() override
    {
        for (int i = 0; i < m_count; ++i)
            LogFile::instance().appendToLogFile(
                QStringLiteral("%1 %2").arg(m_tag).arg(i), m_type);
    }
private:
    QString m_tag;
    int m_count;
    LogType m_type;
};

} // namespace

void run_logging_integrity_tests()
{
    QTemporaryDir dir;
    check(dir.isValid(), "LOG. a temporary directory for the log fixture");
    if (!dir.isValid()) return;

    const QString path = dir.filePath(QStringLiteral("tachus_log_fixture.log"));
    // LogFile::instance() only creates its own file when m_file is null, so the
    // fixture takes ownership of the destination before the first entry.
    LogFile::m_file = new QFile(path);

    // -- 1. every category writes its OWN label ------------------------------
    LogFile::instance().appendToLogFile(QStringLiteral("ux-entry"), LogType::UXLevel);
    LogFile::instance().appendToLogFile(QStringLiteral("backend-entry"), LogType::BackendLevel);
    LogFile::instance().appendToLogFile(QStringLiteral("interface-entry"), LogType::interfaceLevel);
    LogFile::instance().appendToLogFile(QStringLiteral("error-entry"), LogType::ErrorLevel);

    QStringList lines = readBack(path);
    check(lines.size() == 4, "LOG. four entries in, four entries out",
          QStringLiteral("got %1").arg(lines.size()));
    if (lines.size() == 4) {
        check(lines[0].startsWith(QStringLiteral("QML ")),
              "LOG. a UX entry is labelled QML", lines[0]);
        check(lines[1].startsWith(QStringLiteral("Backend ")),
              "LOG. a backend entry is labelled Backend", lines[1]);
        check(lines[2].startsWith(QStringLiteral("interface ")),
              "LOG. an interface entry is labelled interface - the `=` bug is gone",
              lines[2]);
        check(lines[3].startsWith(QStringLiteral("ERROR ")),
              "LOG. an error entry is labelled ERROR - it had no branch at all before",
              lines[3]);
        // The label was the only thing distinguishing them, so prove they differ.
        check(lines[1].section(QLatin1Char(' '), 0, 0)
              != lines[2].section(QLatin1Char(' '), 0, 0),
              "LOG. backend and interface are no longer the same label");
    }

    // -- 2. a timestamp that can be correlated with a journal ----------------
    if (!lines.isEmpty()) {
        const QString stamp = lines[0].section(QLatin1Char(' '), 1, 1);
        check(stamp.size() == 12 && stamp.count(QLatin1Char(':')) == 2
              && stamp.contains(QLatin1Char('.')),
              "LOG. the timestamp is hh:mm:ss.zzz - millisecond correlation is possible",
              stamp);
    }

    // -- 3. nothing is dropped when several threads log at once --------------
    // The field build discarded entries silently whenever it found the handle
    // open, which is exactly what concurrent writers produce.
    const int perThread = 400;
    Writer a(QStringLiteral("poll"), perThread, LogType::BackendLevel);
    Writer b(QStringLiteral("motor"), perThread, LogType::interfaceLevel);
    Writer c(QStringLiteral("flush"), perThread, LogType::ErrorLevel);
    a.start(); b.start(); c.start();
    a.wait(); b.wait(); c.wait();

    lines = readBack(path);
    const int expected = 4 + 3 * perThread;
    check(lines.size() == expected,
          "LOG. 1200 concurrent entries from three threads all reach the file",
          QStringLiteral("expected %1 got %2").arg(expected).arg(lines.size()));

    int poll = 0, motor = 0, flush = 0, malformed = 0;
    for (const QString& l : lines) {
        if (l.contains(QStringLiteral("poll "))) ++poll;
        else if (l.contains(QStringLiteral("motor "))) ++motor;
        else if (l.contains(QStringLiteral("flush "))) ++flush;
        // A line interleaved by an unsynchronised write carries two arrows.
        if (l.count(QStringLiteral(" -> ")) != 1) ++malformed;
    }
    check(poll == perThread && motor == perThread && flush == perThread,
          "LOG. and each thread's entries are all present, none lost",
          QStringLiteral("poll=%1 motor=%2 flush=%3").arg(poll).arg(motor).arg(flush));
    check(malformed == 0,
          "LOG. no line is interleaved with another - the writes are serialized",
          QStringLiteral("malformed=%1").arg(malformed));

    // -- 4. a null handle is refused, not crashed through --------------------
    QFile* saved = LogFile::m_file;
    LogFile::m_file = nullptr;
    LogFile::instance();                    // creates its own handle
    delete LogFile::m_file;
    LogFile::m_file = nullptr;
    LogFile::instance().appendToLogFile(QStringLiteral("no handle"), LogType::BackendLevel);
    check(true, "LOG. logging with no file handle does not crash");
    LogFile::m_file = saved;
}
