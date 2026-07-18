// M3 Phase A — generic EST incident + Jury-decision events.
//
// Covers: reducer lifecycle (raise → credit → recovery phases → resolve),
// the NO-AUTOMATIC-ALLOWANCE guarantee (a Jury credit never mutates the
// competition timer or totals), recovery-sighter separation (official count /
// total / next-shot number unaffected), survival through journal→replay and
// through snapshot serialize→deserialize, reducer rejections, and v1→v2
// backward compatibility. See docs/issf-rules/est-malfunctions.md.

#include "reliability/reducer/SessionReducer.h"
#include "reliability/reducer/SessionState.h"
#include "reliability/replay/ReplayEngine.h"
#include "test_support.h"

using namespace ta::rel;

namespace {

EventEnvelope env(quint64 seq, const DomainEvent& payload, qint64 tm = 0)
{
    EventEnvelope e;
    e.payloadVersion = eventPayloadVersion(payload);
    e.sessionId = QString::fromLatin1(testjournal::kSid);
    e.lane = QStringLiteral("L1");
    e.seq = seq;
    e.wallTimestampIso = QString::fromLatin1(testjournal::kWall);
    e.monotonicMs = tm;
    e.eventType = QString::fromLatin1(eventTypeId(payload));
    e.payload = payload;
    e.previousHash = QByteArray(32, '0');
    e.currentHash = QByteArray(32, '0');
    return e;
}

// Fold a script from scratch; each step must apply. `ok` reports overall.
SessionState fold(const QVector<DomainEvent>& events, bool* ok = nullptr)
{
    SessionState s;
    quint64 seq = 0;
    bool good = true;
    for (const DomainEvent& e : events) {
        const ReduceResult r = SessionReducer::apply(s, env(seq, e));
        if (!r.ok) { good = false; break; }
        s = r.state;
        ++seq;
    }
    if (ok) *ok = good;
    return s;
}

const char* kUtcStart = "2026-07-19T10:00:00.000Z";
const char* kUtcRestored = "2026-07-19T10:04:10.000Z";

// A full qualification lifecycle with a relay-wide EST incident: original
// sighter, two officials, incident raised, Jury credit, 5-min prep, recovery
// sighting (one recovery sighter), official resume, third official, resolve.
QVector<DomainEvent> incidentScript()
{
    return {
        DomainEvent(testjournal::sessionStarted(Discipline::AirRifle10m,
                                                QStringLiteral("60"),
                                                QStringLiteral("A"))),
        DomainEvent(PreparationStarted{0}),
        DomainEvent(SightingStarted{0}),
        DomainEvent(SighterAccepted{testjournal::shot(0, 100)}),   // original
        DomainEvent(OfficialMatchStarted{1}),
        DomainEvent(TimerStarted{TimerId::Match, 4500000}),        // 75 min
        DomainEvent(ShotAccepted{testjournal::shot(1, 104)}),
        DomainEvent(ShotAccepted{testjournal::shot(2, 103)}),
        DomainEvent(EstIncidentRaised{QStringLiteral("inc-1"),
                    IncidentType::ScoringComputerFailure,
                    IncidentScope::RelayWide, QStringLiteral("FP1"),
                    QStringLiteral("R1"),
                    QString::fromLatin1(kUtcStart),
                    QStringLiteral("central computer down")}),
        DomainEvent(TimeCreditGranted{QStringLiteral("inc-1"), 240000,
                    Authority::Jury, QStringLiteral("Chair"),
                    QStringLiteral("6.11.2 over-3-min")}),
        DomainEvent(RecoveryPhaseEntered{QStringLiteral("inc-1"),
                    RecoveryPhaseKind::Preparation, 300000, Authority::Jury,
                    QStringLiteral("Chair")}),
        DomainEvent(RecoveryPhaseEntered{QStringLiteral("inc-1"),
                    RecoveryPhaseKind::Sighting, 0, Authority::Jury,
                    QStringLiteral("Chair")}),
        DomainEvent(SighterAccepted{testjournal::shot(0, 90)}),    // recovery
        DomainEvent(RecoveryPhaseEntered{QStringLiteral("inc-1"),
                    RecoveryPhaseKind::OfficialResume, 0, Authority::Jury,
                    QStringLiteral("Chair")}),
        DomainEvent(ShotAccepted{testjournal::shot(3, 102)}),      // resumes
        DomainEvent(EstIncidentResolved{QStringLiteral("inc-1"),
                    IncidentStatus::Resolved, 250000, 240000,
                    QString::fromLatin1(kUtcRestored), false,
                    QString(), QString(), true, QStringLiteral("jury note"),
                    QStringLiteral("RO note"), QStringLiteral("IR-2026-1")}),
    };
}

} // namespace

void run_incident_tests()
{
    std::printf("--- EST incident domain tests (M3 Phase A) ---\n");

    // 1. Lifecycle folds and the incident record is fully populated.
    {
        bool ok = false;
        const SessionState s = fold(incidentScript(), &ok);
        check(ok, "incident lifecycle folds cleanly");
        check(s.estIncidents.size() == 1, "one incident recorded",
              QString::number(s.estIncidents.size()));
        if (s.estIncidents.size() == 1) {
            const EstIncidentRecord& i = s.estIncidents.first();
            check(i.incidentId == QLatin1String("inc-1"), "incident id");
            check(i.scope == static_cast<quint8>(IncidentScope::RelayWide),
                  "scope relay-wide");
            check(i.incidentType
                      == static_cast<quint8>(IncidentType::ScoringComputerFailure),
                  "incident type");
            check(i.status == static_cast<quint8>(IncidentStatus::Resolved),
                  "status resolved");
            check(i.timeCreditMs == 240000, "credit recorded",
                  QString::number(i.timeCreditMs));
            check(i.preparationGranted, "prep granted");
            check(i.sightingGranted, "recovery sighting granted");
            check(i.officialResumeAuthorised, "official resume authorised");
            check(i.officiallyAcceptedDurationMs == 240000,
                  "accepted duration");
            check(i.calculatedDurationMs == 250000, "calculated duration");
            check(!i.targetMoved, "no target move");
            check(i.backupScoreReviewed, "backup reviewed");
            check(i.interruptionStartUtc == QLatin1String(kUtcStart),
                  "interruption start UTC persisted");
            check(i.authorisedBy == QLatin1String("Chair"), "authorising official");
        }
    }

    // 2. NO AUTOMATIC ALLOWANCE — a Jury credit changes neither the
    //    competition timer nor the official total (est-malfunctions §5).
    {
        QVector<DomainEvent> upToRaise = incidentScript().mid(0, 9);   // ..raise
        QVector<DomainEvent> upToCredit = incidentScript().mid(0, 10); // ..credit
        bool a = false, b = false;
        const SessionState before = fold(upToRaise, &a);
        const SessionState after = fold(upToCredit, &b);
        check(a && b, "pre/post-credit folds apply");
        check(before.timer == after.timer,
              "TimeCreditGranted does not mutate the competition timer");
        check(before.totalTenths == after.totalTenths,
              "TimeCreditGranted does not mutate the official total");
        check(after.timer.active && after.timer.durationMs == 4500000,
              "match timer still the original 75-min duration");
    }

    // 3. Recovery sighter is separated from officials — official count,
    //    total and next-shot number are unaffected by the recovery sighter.
    {
        bool ok = false;
        const SessionState s = fold(incidentScript(), &ok);
        check(s.officials.size() == 3, "three official shots (recovery sighter "
              "excluded)", QString::number(s.officials.size()));
        check(s.sighters.size() == 2, "two sighters (original + recovery)",
              QString::number(s.sighters.size()));
        check(s.totalTenths == 104 + 103 + 102, "official total excludes sighters",
              QString::number(s.totalTenths));
        qint16 maxOfficial = 0;
        for (const StateShotRecord& r : s.officials)
            maxOfficial = qMax(maxOfficial, r.shot.shotNumber);
        check(maxOfficial == 3, "next official shot number is 4 (unbumped by "
              "recovery sighter)", QString::number(maxOfficial));
    }

    // 4. Survives journal → replay (real hashes, validate + fold).
    {
        MemoryJournalFile file;
        testjournal::writeScript(file, incidentScript());
        const ReplayResult rr = ReplayEngine::replayBytes(
            file.data, QString::fromLatin1(testjournal::kSid));
        check(rr.ok, "incident journal replays", rr.error.technicalDetail);
        check(rr.state.estIncidents.size() == 1,
              "replayed state carries the incident");
        bool foldOk = false;
        const SessionState direct = fold(incidentScript(), &foldOk);
        check(foldOk && rr.state.estIncidents == direct.estIncidents,
              "replayed incident equals folded incident");
    }

    // 5. Survives snapshot serialize → deserialize (state v2 round-trip) and
    //    a StateSnapshot event folds (embedded-bytes == fold check passes).
    {
        bool ok = false;
        const SessionState s = fold(incidentScript(), &ok);
        const QByteArray js = serializeSessionState(s);
        SessionState back;
        const ReliabilityResult dr = deserializeSessionState(js, &back);
        check(dr.ok, "state with incidents deserializes", dr.error.technicalDetail);
        check(back == s, "state round-trips byte-for-byte through v2 snapshot");

        // Folding a StateSnapshot built from the state must apply: its handler
        // re-deserializes the embedded bytes and requires captured == folded.
        QVector<DomainEvent> withSnap = incidentScript();
        withSnap.append(DomainEvent(buildStateSnapshot(s)));
        bool snapOk = false;
        fold(withSnap, &snapOk);
        check(snapOk, "StateSnapshot embedding incidents folds cleanly");
    }

    // 6. Reducer rejections — duplicate id, unknown-incident references,
    //    double-resolve.
    {
        const DomainEvent start =
            testjournal::sessionStarted(Discipline::AirPistol10m,
                                        QStringLiteral("60"),
                                        QStringLiteral("A"));
        const DomainEvent raise = EstIncidentRaised{
            QStringLiteral("x1"), IncidentType::PowerFailure,
            IncidentScope::IndividualTarget, QString(), QString(),
            QString::fromLatin1(kUtcStart), QString()};

        bool dup = false;
        fold({start, raise, raise}, &dup);
        check(!dup, "duplicate incidentId rejected");

        bool unknownCredit = false;
        fold({start, raise, DomainEvent(TimeCreditGranted{
                  QStringLiteral("nope"), 1000, Authority::Jury,
                  QStringLiteral("Chair"), QString()})}, &unknownCredit);
        check(!unknownCredit, "credit for unknown incident rejected");

        bool unknownResolve = false;
        fold({start, DomainEvent(EstIncidentResolved{
                  QStringLiteral("nope"), IncidentStatus::Resolved, -1, 0,
                  QString(), false, QString(), QString(), false, QString(),
                  QString(), QString()})}, &unknownResolve);
        check(!unknownResolve, "resolve of unknown incident rejected");

        const DomainEvent resolve = EstIncidentResolved{
            QStringLiteral("x1"), IncidentStatus::Resolved, -1, 0, QString(),
            false, QString(), QString(), false, QString(), QString(), QString()};
        bool doubleResolve = false;
        fold({start, raise, resolve, resolve}, &doubleResolve);
        check(!doubleResolve, "double-resolve rejected");
    }

    // 7. Backward compatibility — a v1 state snapshot (no estIncidents key,
    //    stateVersion 1) deserializes to an empty incident vector.
    {
        bool ok = false;
        const SessionState s = fold({testjournal::sessionStarted(
            Discipline::Prone50m, QStringLiteral("60"),
            QStringLiteral("A"))}, &ok);
        QByteArray v1 = serializeSessionState(s);
        // strip the (empty) estIncidents array and pin the version to 1
        v1.replace(",\"estIncidents\":[]", "");
        v1.replace(QStringLiteral("\"stateVersion\":%1")
                       .arg(kSessionStateVersion).toUtf8(),
                   "\"stateVersion\":1");
        SessionState back;
        const ReliabilityResult dr = deserializeSessionState(v1, &back);
        check(dr.ok, "v1 snapshot (no estIncidents) still deserializes",
              dr.error.technicalDetail);
        check(back.estIncidents.isEmpty(),
              "absent estIncidents yields an empty vector");
    }
}
