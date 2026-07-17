#include "test_support.h"

int g_checks = 0;
int g_failures = 0;

void check(bool ok, const char* name, const QString& detail)
{
    ++g_checks;
    std::printf(ok ? "PASS  %s\n" : "FAIL  %s  %s\n", name,
                ok ? "" : qUtf8Printable(detail));
    if (!ok)
        ++g_failures;
    std::fflush(stdout);
}

namespace testjournal {

using namespace ta::rel;

JournalIdentity identity()
{
    JournalIdentity id;
    id.sessionId = QString::fromLatin1(kSid);
    id.lane = QStringLiteral("L1");
    id.appVersion = QStringLiteral("4.0.0-test");
    id.deviceId = QStringLiteral("TEST-DEVICE");
    return id;
}

SessionStarted sessionStarted(Discipline discipline, const QString& matchType,
                              const QString& athlete)
{
    SessionStarted e;
    e.sessionId = QString::fromLatin1(kSid);
    e.schemaVersion = 1;
    e.appVersion = QStringLiteral("4.0.0-test");
    e.createdAtIso = QString::fromLatin1(kWall);
    e.athlete = athlete;
    e.lane = QStringLiteral("L1");
    e.targetId = QStringLiteral("T1");
    e.discipline = discipline;
    e.matchType = matchType;
    e.config.officialShots = 60;
    e.config.seriesSize = 10;
    e.config.matchMs = 3000000;
    e.config.perPositionShots = 20;
    e.config.sighterLimit = -1;
    e.deviceId = QStringLiteral("TEST-DEVICE");
    return e;
}

ShotCore shot(qint16 number, qint16 scoreTenths, qint32 xHundredthMm,
              qint32 yHundredthMm, qint16 stageId)
{
    ShotCore s;
    s.shotNumber = number;
    s.withinStage = number;
    s.stageId = stageId;
    s.seriesIndex = 0;
    s.xHundredthMm = xHundredthMm;
    s.yHundredthMm = yHundredthMm;
    s.scoreTenths = scoreTenths;
    s.directionCentiDeg = 4500;
    s.splitMs = 8000;
    s.windowId = 1;
    s.targetMode = 1;
    s.externalId = 1000 + number;
    s.simulated = true;
    return s;
}

QVector<EventEnvelope> writeScript(MemoryJournalFile& file,
                                   const QVector<DomainEvent>& events)
{
    QVector<EventEnvelope> envelopes;
    JournalWriter writer(identity(), &file);
    qint64 tm = 0;
    for (const DomainEvent& e : events) {
        const AppendOutcome out =
            writer.append(e, QString::fromLatin1(kWall), tm);
        check(out.ok, "test script append ok", out.error.technicalDetail);
        if (!out.ok)
            break;
        envelopes.append(out.envelope);
        tm += 1000;
    }
    return envelopes;
}

} // namespace testjournal
