// M1 — event catalogue + registry tests (steps 5, 9): construction,
// validation, type/payload mismatch, unknown type, unsupported version,
// registry completeness.

#include "reliability/events/EventRegistry.h"
#include "reliability/events/EventSerializer.h"
#include "test_support.h"

#include <QJsonObject>

using namespace ta::rel;

void run_event_tests()
{
    std::printf("--- event catalogue / registry tests ---\n");

    // registry completeness: every variant alternative has a row, ids and
    // versions agree, index mapping is exact
    {
        const auto& all = EventRegistry::allTypes();
        check(all.size() == static_cast<int>(std::variant_size_v<DomainEvent>),
              "registry has one row per DomainEvent alternative",
              QString::number(all.size()));
        bool indexesOk = true, lookupOk = true, versionOk = true;
        for (const EventMeta* m : all) {
            const EventMeta* byName =
                EventRegistry::metaForType(QString::fromLatin1(m->typeId));
            if (byName != m)
                lookupOk = false;
            if (m->variantIndex < 0
                || m->variantIndex >= static_cast<int>(std::variant_size_v<DomainEvent>))
                indexesOk = false;
            if (m->payloadVersion < 1)
                versionOk = false;
        }
        check(lookupOk, "registry lookup by type string returns the same row");
        check(indexesOk, "registry variant indexes are in range");
        check(versionOk, "registry payload versions are >= 1");
        check(EventRegistry::metaForType(QStringLiteral("NoSuchEvent")) == nullptr,
              "unknown type string yields no registry row");
    }

    // metaForEvent agrees with the held alternative
    {
        DomainEvent e = ShotAccepted{testjournal::shot(1, 104)};
        const EventMeta* m = EventRegistry::metaForEvent(e);
        check(m && QLatin1String(m->typeId) == QLatin1String("ShotAccepted"),
              "metaForEvent resolves the held alternative");
        check(m && m->durability == DurabilityClass::Sync,
              "official shots classified fsync (spec s12)");
        DomainEvent sighter = SighterAccepted{testjournal::shot(0, 99)};
        const EventMeta* sm = EventRegistry::metaForEvent(sighter);
        check(sm && sm->durability == DurabilityClass::Flush,
              "sighters classified flush-not-fsync (spec D2)");
    }

    // construction + validation
    {
        check(validateEvent(DomainEvent(
                  testjournal::sessionStarted(Discipline::Finals3P,
                                              QStringLiteral("FINAL 35"),
                                              QStringLiteral("A")))).ok,
              "valid SessionStarted validates");
        SessionStarted bad;   // empty sessionId
        check(!validateEvent(DomainEvent(bad)).ok,
              "SessionStarted without sessionId rejected");
        ShotAccepted shotSighterNumber{testjournal::shot(0, 104)};
        check(!validateEvent(DomainEvent(shotSighterNumber)).ok,
              "ShotAccepted with shotNumber 0 rejected");
        SighterAccepted sighterOfficialNumber{testjournal::shot(3, 104)};
        check(!validateEvent(DomainEvent(sighterOfficialNumber)).ok,
              "SighterAccepted with shotNumber != 0 rejected");
        ShotCore outOfRange = testjournal::shot(1, 110);
        check(!validateEvent(DomainEvent(ShotAccepted{outOfRange})).ok,
              "scoreTenths 110 rejected (max 109)");
        PositionChanged badPos;
        badPos.positionIndex = 3;
        check(!validateEvent(DomainEvent(badPos)).ok,
              "positionIndex 3 rejected");
        TimerStarted badTimer;
        badTimer.durationMs = 0;
        check(!validateEvent(DomainEvent(badTimer)).ok,
              "TimerStarted with zero duration rejected");
        check(eventTypeId(DomainEvent(SessionResumed{}))
                  == QLatin1String("SessionResumed"),
              "eventTypeId returns the stable identifier");
        check(eventPayloadVersion(DomainEvent(SessionResumed{})) == 1,
              "eventPayloadVersion is 1 for every M1 event");
    }

    // type/payload conflict is rejected at the envelope level
    {
        EventEnvelope env;
        env.sessionId = QString::fromLatin1(testjournal::kSid);
        env.wallTimestampIso = QString::fromLatin1(testjournal::kWall);
        env.seq = 1;
        env.eventType = QStringLiteral("ShotAccepted");
        env.payload = SessionResumed{};   // conflicting payload
        env.previousHash = QByteArray(32, '0');
        const ReliabilityResult r = env.validate();
        check(!r.ok && r.error.code == ReliabilityError::InvalidEvent,
              "envelope type/payload conflict rejected",
              r.error.technicalDetail);
    }

    // seq 0 must be the session header
    {
        EventEnvelope env;
        env.sessionId = QString::fromLatin1(testjournal::kSid);
        env.wallTimestampIso = QString::fromLatin1(testjournal::kWall);
        env.seq = 0;
        env.eventType = QStringLiteral("SessionResumed");
        env.payload = SessionResumed{};
        env.previousHash = QByteArray(32, '0');
        check(!env.validate().ok, "seq 0 with non-header payload rejected");
    }

    // unsupported payload version through deserialization
    {
        // a valid line, pv patched to 99 — must fail typed, never guessed
        MemoryJournalFile file;
        const QVector<EventEnvelope> envs = testjournal::writeScript(file,
            {testjournal::sessionStarted(Discipline::Prone50m,
                                         QStringLiteral("60"),
                                         QStringLiteral("A"))});
        check(envs.size() == 1, "script wrote header");
        QByteArray line = file.data;
        line.chop(1);   // LF
        QByteArray patched = line;
        patched.replace("\"pv\":1", "\"pv\":99");
        EventEnvelope out;
        const ReliabilityResult r =
            EventSerializer::deserializeEnvelope(patched, &out);
        check(!r.ok && r.error.code == ReliabilityError::UnsupportedEventVersion,
              "pv 99 yields typed UnsupportedEventVersion",
              QLatin1String(reliabilityErrorName(r.error.code)));
    }

    // unknown event type through deserialization
    {
        MemoryJournalFile file;
        testjournal::writeScript(file,
            {testjournal::sessionStarted(Discipline::Prone50m,
                                         QStringLiteral("60"),
                                         QStringLiteral("A"))});
        QByteArray line = file.data;
        line.chop(1);
        QByteArray patched = line;
        patched.replace("\"t\":\"SessionStarted\"", "\"t\":\"FutureEvent\"");
        EventEnvelope out;
        const ReliabilityResult r =
            EventSerializer::deserializeEnvelope(patched, &out);
        check(!r.ok && r.error.code == ReliabilityError::UnsupportedEventType,
              "unknown type yields typed UnsupportedEventType",
              QLatin1String(reliabilityErrorName(r.error.code)));
    }
}
