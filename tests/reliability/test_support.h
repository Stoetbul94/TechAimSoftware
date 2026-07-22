#ifndef TA_REL_TEST_SUPPORT_H
#define TA_REL_TEST_SUPPORT_H

// Shared support for the reliability console harness (finals-harness
// pattern): one binary, one check counter, per-module run functions.

#include "reliability/journal/JournalWriter.h"

#include <QByteArray>
#include <QString>

#include <cstdio>

extern int g_checks;
extern int g_failures;

void check(bool ok, const char* name, const QString& detail = QString());

// ── fault-injection double (M1 step 17) ───────────────────────────────
// In-memory IJournalFile with injectable faults: open/write/flush/sync
// failure and short (partial) writes. Test-only by construction — it is
// dependency-injected into JournalWriter.
class MemoryJournalFile : public ta::rel::IJournalFile {
public:
    QByteArray data;
    bool opened = false;
    bool failOpen = false;
    bool failWrite = false;        // write() returns -1, nothing stored
    bool failFlush = false;
    bool failSync = false;
    qint64 partialWriteLimit = -1; // >=0: store only this many bytes once
    int writeCount = 0, flushCount = 0, syncCount = 0;

    bool open() override
    {
        if (failOpen)
            return false;
        opened = true;
        return true;
    }
    bool isOpen() const override { return opened; }
    qint64 write(const QByteArray& bytes) override
    {
        ++writeCount;
        if (failWrite)
            return -1;
        if (partialWriteLimit >= 0) {
            const qint64 n = qMin<qint64>(partialWriteLimit, bytes.size());
            data += bytes.left(static_cast<int>(n));
            partialWriteLimit = -1;   // one-shot fault
            return n;
        }
        data += bytes;
        return bytes.size();
    }
    bool flush() override
    {
        ++flushCount;
        return !failFlush;
    }
    bool sync() override
    {
        ++syncCount;
        return !failSync;
    }
    QString path() const override { return QStringLiteral("<memory>"); }
    QString lastErrorDetail() const override
    {
        return QStringLiteral("injected fault");
    }
};

// ── deterministic journal script helpers (shared by several modules) ──
namespace testjournal {

inline const char* kSid = "00000000-0000-4000-8000-0000000000a1";
inline const char* kWall = "2026-07-17T10:00:00.000";

ta::rel::JournalIdentity identity();
ta::rel::SessionStarted sessionStarted(ta::rel::Discipline discipline,
                                       const QString& matchType,
                                       const QString& athlete);
ta::rel::ShotCore shot(qint16 number, qint16 scoreTenths,
                       qint32 xHundredthMm = 0, qint32 yHundredthMm = 0,
                       qint16 stageId = 1);

// Append a script of events through a fresh writer into `file`; every
// append must succeed (checked). Returns the envelopes.
QVector<ta::rel::EventEnvelope> writeScript(
    MemoryJournalFile& file, const QVector<ta::rel::DomainEvent>& events);

} // namespace testjournal

// per-module entry points (defined one per tst_*.cpp)
void run_storagepaths_tests();
void run_fixedpoint_tests();
void run_event_tests();
void run_serializer_tests();
void run_hashchain_tests();
void run_writer_tests();
void run_reader_tests();
void run_validator_tests();
void run_reducer_tests();
void run_incident_tests();
void run_qualification_tests();
void run_snapshot_tests();
void run_store_tests();
void run_recovery_tests();
void run_operatingmode_tests();
void run_fixture_tests(bool regenerate);

#endif // TA_REL_TEST_SUPPORT_H
