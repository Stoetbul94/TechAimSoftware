// M1 — deterministic serialization tests (step 7): exact bytes, stable
// ordering, Unicode, escaping, LF discipline, round-trip.

#include "reliability/events/EventSerializer.h"
#include "reliability/journal/HashChain.h"
#include "test_support.h"

using namespace ta::rel;

namespace {

EventEnvelope makeEnvelope(quint64 seq, const DomainEvent& payload,
                           const QByteArray& ph)
{
    EventEnvelope env;
    env.payloadVersion = eventPayloadVersion(payload);
    env.sessionId = QString::fromLatin1(testjournal::kSid);
    env.lane = QStringLiteral("L1");
    env.seq = seq;
    env.wallTimestampIso = QString::fromLatin1(testjournal::kWall);
    env.monotonicMs = static_cast<qint64>(seq) * 1000;
    env.eventType = QString::fromLatin1(eventTypeId(payload));
    env.payload = payload;
    if (seq == 0) {
        env.appVersion = QStringLiteral("4.0.0-test");
        env.deviceId = QStringLiteral("TEST-DEVICE");
    }
    env.previousHash = ph;
    return env;
}

} // namespace

void run_serializer_tests()
{
    std::printf("--- deterministic serializer tests ---\n");

    // exact golden bytes of a simple envelope core (frozen layout)
    {
        PositionChanged pos;
        pos.positionIndex = 2;
        EventEnvelope env = makeEnvelope(5, pos, QByteArray(32, 'a'));
        QByteArray core;
        const ReliabilityResult r =
            EventSerializer::serializeCoreWithoutCurrentHash(env, &core);
        check(r.ok, "core serialization succeeds", r.error.technicalDetail);
        const QByteArray expected =
            "{\"sv\":1,\"pv\":1,\"sid\":\"00000000-0000-4000-8000-0000000000a1\","
            "\"lane\":\"L1\",\"seq\":5,\"tw\":\"2026-07-17T10:00:00.000\","
            "\"tm\":5000,\"t\":\"PositionChanged\",\"p\":{\"positionIndex\":2},"
            "\"ph\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}";
        check(core == expected, "core bytes match the frozen golden layout",
              QString::fromUtf8(core));

        env.currentHash = HashChain::computeLineHash(env.previousHash, core);
        QByteArray full;
        EventSerializer::serializeCompleteEnvelope(env, &full);
        check(full.startsWith(core.left(core.size() - 1))
                  && full.endsWith("\"}")
                  && full.contains(",\"h\":\""),
              "complete envelope = core + h field");
        check(!full.contains('\n'), "serialized line contains no newline");
        // serialization is repeatable byte-for-byte
        QByteArray core2;
        EventSerializer::serializeCoreWithoutCurrentHash(env, &core2);
        check(core == core2, "serialization is deterministic across calls");
    }

    // stable ordering: same envelope serialized twice from a parsed copy
    {
        MemoryJournalFile file;
        testjournal::writeScript(file,
            {testjournal::sessionStarted(Discipline::ThreePositions50m,
                                         QStringLiteral("120"),
                                         QStringLiteral("Athlete")),
             SightingStarted{1}});
        QByteArray firstLine = file.data.left(file.data.indexOf('\n'));
        EventEnvelope parsed;
        const ReliabilityResult r =
            EventSerializer::deserializeEnvelope(firstLine, &parsed);
        check(r.ok, "written header deserializes", r.error.technicalDetail);
        parsed.currentHash.clear();
        QByteArray reserialized;
        EventSerializer::serializeCoreWithoutCurrentHash(parsed, &reserialized);
        QByteArray originalCore = firstLine;
        // strip ',"h":"..."' suffix by re-deriving: core + hash == line
        const QByteArray expectedFull = [&] {
            EventEnvelope tmp = parsed;
            tmp.currentHash =
                HashChain::computeLineHash(tmp.previousHash, reserialized);
            QByteArray full;
            EventSerializer::serializeCompleteEnvelope(tmp, &full);
            return full;
        }();
        check(expectedFull == originalCore,
              "parse -> reserialize reproduces the exact written bytes");
    }

    // Unicode athlete names survive byte-exactly (raw UTF-8, no \u)
    {
        SessionStarted started = testjournal::sessionStarted(
            Discipline::AirRifle10m, QStringLiteral("60"),
            QString::fromUtf8("Zoë Müller 佐藤"));
        EventEnvelope env = makeEnvelope(0, started, QByteArray(32, '0'));
        QByteArray core;
        const ReliabilityResult r =
            EventSerializer::serializeCoreWithoutCurrentHash(env, &core);
        check(r.ok && core.contains(QString::fromUtf8("Zoë Müller 佐藤").toUtf8()),
              "unicode athlete name embedded as raw UTF-8");
        env.currentHash = HashChain::computeLineHash(env.previousHash, core);
        QByteArray full;
        EventSerializer::serializeCompleteEnvelope(env, &full);
        EventEnvelope back;
        const ReliabilityResult rr = EventSerializer::deserializeEnvelope(full, &back);
        const auto* p = std::get_if<SessionStarted>(&back.payload);
        check(rr.ok && p && p->athlete == started.athlete,
              "unicode round-trips exactly");
    }

    // escaping: quotes, backslashes, control characters
    {
        AthleteAssigned a;
        a.athlete = QStringLiteral("He said \"hi\" \\ tab\there\nnl");
        EventEnvelope env = makeEnvelope(3, a, QByteArray(32, 'b'));
        QByteArray core;
        const ReliabilityResult r =
            EventSerializer::serializeCoreWithoutCurrentHash(env, &core);
        check(r.ok && core.contains("\\\"hi\\\"") && core.contains("\\\\")
                  && core.contains("\\t") && core.contains("\\n"),
              "quotes/backslash/control characters escaped",
              QString::fromUtf8(core));
        env.currentHash = HashChain::computeLineHash(env.previousHash, core);
        QByteArray full;
        EventSerializer::serializeCompleteEnvelope(env, &full);
        EventEnvelope back;
        const ReliabilityResult rr = EventSerializer::deserializeEnvelope(full, &back);
        const auto* p = std::get_if<AthleteAssigned>(&back.payload);
        check(rr.ok && p && p->athlete == a.athlete,
              "escaped strings round-trip exactly");
    }

    // every event type round-trips through serialize -> deserialize
    {
        QVector<DomainEvent> catalogue = {
            testjournal::sessionStarted(Discipline::Finals3P,
                                        QStringLiteral("FINAL 35"),
                                        QStringLiteral("A")),
            SessionConfigured{Discipline::Prone50m, QStringLiteral("60"), {}},
            AthleteAssigned{QStringLiteral("A"), QStringLiteral("L2"),
                            QStringLiteral("T2")},
            PreparationStarted{1}, SightingStarted{1}, OfficialMatchStarted{2},
            ShotAccepted{testjournal::shot(4, 104)},
            SighterAccepted{testjournal::shot(0, 96)},
            StageCompleted{2, 1}, PositionChanged{1},
            TimerStarted{TimerId::Match, 900000},
            TimerPaused{TimerId::Match, 5000},
            TimerResumed{TimerId::Match, 9000},
            TimerExpired{TimerId::Match},
            MatchCompleted{104, 1},
            SessionSuspended{SuspendReason::UserHide},
            SessionResumed{},
            StateSnapshot{1, 7, 1, 1, 104, QByteArrayLiteral("{\"x\":1}")},
            ShotInvalidated{7, QStringLiteral("rule 7.11"), Authority::Jury},
            ShotRescored{7, 103, true, 10, -20, QStringLiteral("jury"),
                         Authority::Jury},
            SeriesAdjusted{2, -10, QStringLiteral("jury"), Authority::Jury},
            CrossShotRecorded{testjournal::shot(0, 87), QStringLiteral("L3")},
            EquipmentMalfunctionRecorded{QStringLiteral("broken sling"), 900000},
            PenaltyIssued{-20, QStringLiteral("7.4.5"), Authority::Jury},
            RecoveryStarted{41}, RecoveryCompleted{41, true},
            SessionClosed{CloseReason::Clean},
            // M2 additions
            StageEntered{5}, StageStatusChanged{5, 2}, TargetModeChanged{1},
            WindowOpened{4}, WindowClosed{4},
            CommandIssued{7, 3, QStringLiteral("START"), 7,
                          QStringLiteral("start"), 1000, 1000},
            ShotRejected{QStringLiteral("WindowClosed"), 99, 10, -20, false},
            MissingShotRecorded{14, 5, QStringLiteral("TimeExpired")},
            PersistenceDegraded{3}, PersistenceRestored{0},
            AuxEventsDropped{50, 52, 3}, CleanShutdown{},
            // M3 Phase A — generic EST incident + Jury-decision events
            EstIncidentRaised{QStringLiteral("inc-1"),
                              IncidentType::TargetNetworkFailure,
                              IncidentScope::RelayWide, QStringLiteral("FP3"),
                              QStringLiteral("R1"),
                              QStringLiteral("2026-07-19T10:00:00.000Z"),
                              QStringLiteral("network dropped")},
            TimeCreditGranted{QStringLiteral("inc-1"), 240000, Authority::Jury,
                              QStringLiteral("JuryChair"),
                              QStringLiteral("6.11.2 over-3-min")},
            RecoveryPhaseEntered{QStringLiteral("inc-1"),
                                 RecoveryPhaseKind::Preparation, 300000,
                                 Authority::Jury, QStringLiteral("JuryChair")},
            EstIncidentResolved{QStringLiteral("inc-1"), IncidentStatus::Resolved,
                                250000, 240000,
                                QStringLiteral("2026-07-19T10:04:00.000Z"), true,
                                QStringLiteral("T3"), QStringLiteral("T9"), true,
                                QStringLiteral("jury note"),
                                QStringLiteral("RO note"),
                                QStringLiteral("IR-2026-07")},
            // Phase E — Jury decision + live target reassignment
            EstDecisionRecorded{QStringLiteral("inc-1"),
                                EstDecisionKind::NoAllowance, -1,
                                Authority::Jury, QStringLiteral("Chair"),
                                QStringLiteral("under 3 minutes")},
            TargetReassigned{QStringLiteral("inc-1"), QStringLiteral("T3"),
                             QStringLiteral("T9"), Authority::Jury,
                             QStringLiteral("Chair"),
                             QStringLiteral("continuous fault")},
            // Training Lab (T1)
            TrainingSessionStarted{QStringLiteral("technical_blocks"),
                                   Discipline::AirRifle10m, 5, 6, 0,
                                   QStringLiteral("Trigger"), 0},
            TrainingBlockStarted{1, 0},
            TrainingShotAccepted{ShotCore{1, 1, 1, 0, 120, -35, 104, 0, 4200, 0, 0, 7, true},
                                 1, 1, 0, 0, 0, false},
            TrainingBlockCompleted{1, 6},
            TrainingNoteSaved{1, QStringLiteral("felt steady")},
            TrainingCompleted{5},
            TrainingSighterAccepted{ShotCore{0, 0, 0, 0, 90, 12, 98, 0, 3000, 0, 0, 5, true}, 0, 1},
            TrainingSighterPhaseStarted{1, 3},
            CallDiagnoseSessionStarted{QStringLiteral("call_and_diagnose"), 20,
                                       QStringLiteral("Trigger"), 0, false},
            CallDiagnoseStarted{0},
            CallDiagnoseShotReceived{ShotCore{3, 3, 0, 0, 120, 34, 104, 0, 4000, 0, 0, 42, true}, 3, 0},
            CallDiagnoseCallRecorded{3, 0, 250, -180, 4200},
            CallDiagnoseNoteSaved{3, 0, QStringLiteral("felt high")},
            CallDiagnoseCompleted{20, QStringLiteral("good session")},
        };
        check(catalogue.size() == static_cast<int>(std::variant_size_v<DomainEvent>),
              "round-trip covers every event type",
              QString::number(catalogue.size()));
        bool all = true;
        QString failedType;
        for (int i = 0; i < catalogue.size(); ++i) {
            // seq 1+ except the header event (SessionStarted must sit at 0)
            const bool isHeader =
                std::holds_alternative<SessionStarted>(catalogue[i]);
            EventEnvelope env = makeEnvelope(isHeader ? 0 : quint64(i + 1),
                                             catalogue[i], QByteArray(32, 'c'));
            if (isHeader)
                env.previousHash = QByteArray(32, '0');
            QByteArray core;
            ReliabilityResult r =
                EventSerializer::serializeCoreWithoutCurrentHash(env, &core);
            if (!r.ok) { all = false; failedType = env.eventType + ": " + r.error.technicalDetail; break; }
            env.currentHash = HashChain::computeLineHash(env.previousHash, core);
            QByteArray full;
            EventSerializer::serializeCompleteEnvelope(env, &full);
            EventEnvelope back;
            r = EventSerializer::deserializeEnvelope(full, &back);
            if (!r.ok) { all = false; failedType = env.eventType + ": " + r.error.technicalDetail; break; }
            if (back.seq != env.seq || back.eventType != env.eventType
                || back.currentHash != env.currentHash) {
                all = false;
                failedType = env.eventType + QStringLiteral(" (value mismatch)");
                break;
            }
            QByteArray again;
            r = EventSerializer::serializeCompleteEnvelope(back, &again);
            if (!r.ok || again != full) {
                all = false;
                failedType = env.eventType + QStringLiteral(" (byte drift)");
                break;
            }
        }
        check(all, "all event types round-trip byte-identically", failedType);
    }

    // malformed JSON / missing field / wrong type are typed
    {
        EventEnvelope out;
        ReliabilityResult r =
            EventSerializer::deserializeEnvelope("{not json", &out);
        check(!r.ok && r.error.code == ReliabilityError::InvalidJson,
              "malformed JSON -> InvalidJson");
        r = EventSerializer::deserializeEnvelope("[1,2]", &out);
        check(!r.ok && r.error.code == ReliabilityError::InvalidJson,
              "non-object JSON -> InvalidJson");
        MemoryJournalFile file;
        testjournal::writeScript(file,
            {testjournal::sessionStarted(Discipline::Prone50m,
                                         QStringLiteral("60"),
                                         QStringLiteral("A"))});
        QByteArray line = file.data;
        line.chop(1);
        QByteArray noSeq = line;
        noSeq.replace("\"seq\":0,", "");
        r = EventSerializer::deserializeEnvelope(noSeq, &out);
        check(!r.ok && r.error.code == ReliabilityError::MissingField,
              "missing seq -> MissingField",
              QLatin1String(reliabilityErrorName(r.error.code)));
        QByteArray strSeq = line;
        strSeq.replace("\"seq\":0,", "\"seq\":\"0\",");
        r = EventSerializer::deserializeEnvelope(strSeq, &out);
        check(!r.ok && r.error.code == ReliabilityError::InvalidFieldType,
              "string seq -> InvalidFieldType");
        QByteArray fracTm = line;
        fracTm.replace("\"tm\":0,", "\"tm\":0.5,");
        r = EventSerializer::deserializeEnvelope(fracTm, &out);
        check(!r.ok && r.error.code == ReliabilityError::InvalidFieldType,
              "fractional tm -> InvalidFieldType (integers only)");
    }
}
