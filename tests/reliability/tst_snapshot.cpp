// M1 — snapshot tests (step 15): serialize/deserialize the full state,
// state equality after round-trip, integrity metadata, sequence rules,
// reducer verification, hash-chain participation.

#include "reliability/journal/JournalValidator.h"
#include "reliability/reducer/SessionReducer.h"
#include "test_support.h"

using namespace ta::rel;

namespace {

EventEnvelope env(quint64 seq, const DomainEvent& payload)
{
    EventEnvelope e;
    e.payloadVersion = eventPayloadVersion(payload);
    e.sessionId = QString::fromLatin1(testjournal::kSid);
    e.lane = QStringLiteral("L1");
    e.seq = seq;
    e.wallTimestampIso = QString::fromLatin1(testjournal::kWall);
    e.monotonicMs = static_cast<qint64>(seq) * 1000;
    e.eventType = QString::fromLatin1(eventTypeId(payload));
    e.payload = payload;
    e.previousHash = QByteArray(32, '0');
    e.currentHash = QByteArray(32, '0');
    return e;
}

SessionState richState()
{
    SessionState s;
    quint64 seq = 0;
    const QVector<DomainEvent> script = {
        DomainEvent(testjournal::sessionStarted(Discipline::Finals3P,
                                                QStringLiteral("FINAL 35"),
                                                QString::fromUtf8("Zoë 佐藤"))),
        DomainEvent(PreparationStarted{0}),
        DomainEvent(SightingStarted{0}),
        DomainEvent(SighterAccepted{testjournal::shot(0, 101)}),
        DomainEvent(OfficialMatchStarted{1}),
        DomainEvent(TimerStarted{TimerId::Stage, 300000}),
        DomainEvent(ShotAccepted{testjournal::shot(1, 104)}),
        DomainEvent(ShotAccepted{testjournal::shot(2, 99)}),
        DomainEvent(StageCompleted{1, 2}),
        DomainEvent(PositionChanged{1}),
        DomainEvent(ShotRescored{6, 105, true, 42, -17,
                                 QStringLiteral("jury"), Authority::Jury}),
        DomainEvent(PenaltyIssued{-20, QStringLiteral("7.4.5"),
                                  Authority::Operator}),
        DomainEvent(EquipmentMalfunctionRecorded{QStringLiteral("note"), -1}),
    };
    for (const DomainEvent& e : script) {
        const ReduceResult r = SessionReducer::apply(s, env(seq, e));
        check(r.ok, "rich state fold step", r.error.technicalDetail);
        if (!r.ok)
            break;
        s = r.state;
        ++seq;
    }
    return s;
}

} // namespace

void run_snapshot_tests()
{
    std::printf("--- state snapshot tests ---\n");

    const SessionState state = richState();

    // serialize -> deserialize -> exact equality
    {
        const QByteArray json = serializeSessionState(state);
        check(!json.isEmpty() && json.startsWith('{') && json.endsWith('}'),
              "state serializes to a compact object");
        check(serializeSessionState(state) == json,
              "state serialization is deterministic");
        SessionState back;
        const ReliabilityResult r = deserializeSessionState(json, &back);
        check(r.ok, "state deserializes", r.error.technicalDetail);
        check(back == state, "state round-trip preserves exact equality");
        check(serializeSessionState(back) == json,
              "state round-trip preserves exact bytes");
    }

    // buildStateSnapshot metadata
    {
        const StateSnapshot snap = buildStateSnapshot(state);
        check(snap.lastAppliedSeq == state.lastSeq
                  && snap.officialCount == state.officials.size()
                  && snap.sighterCount == state.sighters.size()
                  && snap.totalTenths == state.totalTenths,
              "snapshot integrity metadata mirrors the state");
        check(snap.stateVersion == kSessionStateVersion,
              "snapshot carries the state version");
        check(validateEvent(DomainEvent(snap)).ok, "snapshot payload validates");
    }

    // reducer verifies a coherent snapshot and rejects drifted metadata
    {
        const StateSnapshot snap = buildStateSnapshot(state);
        const ReduceResult ok = SessionReducer::apply(
            state, env(state.lastSeq + 1, DomainEvent(snap)));
        check(ok.ok && ok.state.lastSeq == state.lastSeq + 1,
              "coherent snapshot accepted; only lastSeq advances",
              ok.error.technicalDetail);
        StateSnapshot drifted = snap;
        drifted.totalTenths += 1;
        const ReduceResult bad = SessionReducer::apply(
            state, env(state.lastSeq + 1, DomainEvent(drifted)));
        check(!bad.ok && bad.error.code == ReliabilityError::ReducerRejected,
              "drifted snapshot metadata rejected");
        StateSnapshot foreign = snap;
        SessionState other = state;
        other.athlete = QStringLiteral("Somebody Else");
        foreign.stateJson = serializeSessionState(other);
        const ReduceResult mismatch = SessionReducer::apply(
            state, env(state.lastSeq + 1, DomainEvent(foreign)));
        check(!mismatch.ok,
              "snapshot whose embedded state differs from the fold rejected");
    }

    // malformed / newer-version embedded state is typed
    {
        SessionState out;
        const ReliabilityResult bad =
            deserializeSessionState(QByteArrayLiteral("{"), &out);
        check(!bad.ok && bad.error.code == ReliabilityError::InvalidJson,
              "malformed state json rejected typed");
        QByteArray newer = serializeSessionState(state);
        newer.replace("\"stateVersion\":1", "\"stateVersion\":99");
        const ReliabilityResult tooNew = deserializeSessionState(newer, &out);
        check(!tooNew.ok && tooNew.error.code == ReliabilityError::SchemaTooNew,
              "newer stateVersion rejected typed",
              QLatin1String(reliabilityErrorName(tooNew.error.code)));
    }

    // snapshots participate in the hash chain like any event
    {
        MemoryJournalFile file;
        SessionState s;
        JournalWriter writer(testjournal::identity(), &file);
        quint64 seq = 0;
        const QVector<DomainEvent> script = {
            DomainEvent(testjournal::sessionStarted(Discipline::Training,
                                                    QStringLiteral("TRAIN"),
                                                    QStringLiteral("A"))),
            DomainEvent(SightingStarted{0}),
            DomainEvent(SighterAccepted{testjournal::shot(0, 88)}),
        };
        for (const DomainEvent& e : script) {
            const AppendOutcome out =
                writer.append(e, QString::fromLatin1(testjournal::kWall),
                              static_cast<qint64>(seq) * 1000);
            check(out.ok, "snapshot-chain script append",
                  out.error.technicalDetail);
            const ReduceResult r = SessionReducer::apply(s, out.envelope);
            check(r.ok, "snapshot-chain script reduce", r.error.technicalDetail);
            s = r.state;
            ++seq;
        }
        const AppendOutcome snapOut = writer.append(
            DomainEvent(buildStateSnapshot(s)),
            QString::fromLatin1(testjournal::kWall),
            static_cast<qint64>(seq) * 1000);
        check(snapOut.ok, "snapshot appended to the journal",
              snapOut.error.technicalDetail);
        const ValidationReport rep = JournalValidator::validateBytes(file.data);
        check(rep.classification == JournalClassification::Clean
                  && rep.validPrefixCount == 4,
              "journal with snapshot validates Clean (chain participation)",
              rep.error.technicalDetail);
        // tampering INSIDE the embedded state breaks the line hash
        QByteArray tampered = file.data;
        tampered.replace(QByteArrayLiteral("\\\"athlete\\\":\\\"A\\\""),
                         QByteArrayLiteral("\\\"athlete\\\":\\\"B\\\""));
        check(tampered != file.data, "embedded-state tamper target found");
        const ValidationReport tampRep =
            JournalValidator::validateBytes(tampered);
        check(tampRep.classification != JournalClassification::Clean,
              "tampering inside the snapshot state detected by the chain",
              QLatin1String(journalClassificationName(tampRep.classification)));
    }
}
