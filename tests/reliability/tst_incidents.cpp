// M3 Phase A — generic EST incident + Jury-decision events.
//
// Covers: reducer lifecycle (raise → credit → recovery phases → resolve),
// the NO-AUTOMATIC-ALLOWANCE guarantee (a Jury credit never mutates the
// competition timer or totals), recovery-sighter separation (official count /
// total / next-shot number unaffected), survival through journal→replay and
// through snapshot serialize→deserialize, reducer rejections, and v1→v2
// backward compatibility. See docs/issf-rules/est-malfunctions.md.

#include "incident/EstIncidentController.h"
#include "qualification/QualificationController.h"
#include "reliability/journal/JournalValidator.h"
#include "reliability/reducer/SessionReducer.h"
#include "reliability/reducer/SessionState.h"
#include "reliability/replay/ReplayEngine.h"
#include "test_support.h"

// Phase E workflow block (defined below, invoked from run_incident_tests).
void run_incident_workflow_tests();

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
        // Normalize the envelope-clock anchor (Phase E: raisedAtMonoMs is
        // stamped from tm, which differs between the journal path and this
        // tm=0 direct fold) — everything else must match exactly.
        QVector<EstIncidentRecord> a = rr.state.estIncidents;
        QVector<EstIncidentRecord> b = direct.estIncidents;
        for (EstIncidentRecord& r : a) r.raisedAtMonoMs = 0;
        for (EstIncidentRecord& r : b) r.raisedAtMonoMs = 0;
        check(foldOk && a == b, "replayed incident equals folded incident");
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

    // ── Phase E — full incident/Jury workflow through the service ─────────
    run_incident_workflow_tests();
}

// Phase E: exercise EstIncidentController end-to-end against a real
// SessionStore (in-memory journal + ManualClock): raise → clock freeze →
// decisions → recovery phases → resume gate → resolve, plus replay
// determinism, the explicit no-allowance ruling, double-credit rejection,
// boundary keys (no auto-decision at exactly 3:00/5:00), and the official
// resume gate at the qualification controller.
void run_incident_workflow_tests()
{
    std::printf("--- EST incident workflow tests (Phase E) ---\n");

    // Harness: an AR10-style qualification store with a live 75-min match
    // clock, driven through QualificationController + EstIncidentController.
    MemoryJournalFile file;
    ManualClock clock;
    QualificationController qc;
    qc.storeForTesting()->setClockForTesting(&clock);
    qc.storeForTesting()->setJournalFileForTesting(&file);
    qc.startSession(QStringLiteral("AR10"), QStringLiteral("60"),
                    QStringLiteral("A"), 60, 4500000, 900000, -1,
                    QString(), QString());
    qc.beginPreparation();
    qc.beginSighting();
    clock.advance(10000);
    qc.submitSighter(0, 0, 10.5, 1, 0, true);        // original sighter
    qc.beginOfficialMatch();                          // anchors 75-min clock
    clock.advance(60000);
    qc.submitOfficial(0, 0, 10.9, 11, 0, true);
    clock.advance(30000);
    qc.submitOfficial(0, 0, 10.3, 12, 0, true);

    EstIncidentController inc;
    inc.setStoreProvider([&qc]() { return qc.storeForTesting(); });

    // 1) Raise (individual target) — clock freezes at the raise instant.
    clock.advance(15000);
    check(inc.raiseIncident(0 /*target-not-registering*/, 0 /*individual*/,
                            QStringLiteral("1"), QString(),
                            QStringLiteral("no registration")),
          "raise individual-target incident");
    check(inc.hasOpenIncident(), "incident is open");
    const SessionState& st = qc.storeForTesting()->state();
    check(st.timer.paused, "competition clock frozen (TimerPaused journalled)");
    const qint64 frozenRemaining = inc.remainingCompetitionMs();
    // elapsed running = 60000+30000+15000 = 105000 → remaining 4'395'000.
    check(frozenRemaining == 4500000 - 105000,
          "frozen remaining derived pause-aware",
          QString::number(frozenRemaining));
    clock.advance(120000);   // 2 minutes of interruption tick by...
    check(inc.remainingCompetitionMs() == frozenRemaining,
          "interruption time does not consume competition time");

    // 2) Official shots are blocked at the controller while unresolved.
    check(!qc.submitOfficial(0, 0, 10.0, 13, 0, true),
          "official shot refused while incident unresolved");
    check(qc.officialShotCount() == 2, "official count unchanged while blocked");

    // 3) No automatic allowance: nothing was granted by raising alone.
    {
        const QVariantMap m = inc.activeIncident();
        check(m.value(QStringLiteral("creditDecision")).toInt() == 0,
              "allowance decision pending (never automatic)");
        check(m.value(QStringLiteral("timeCreditMs")).toLongLong() == 0,
              "no automatic time credit");
        check(m.value(QStringLiteral("statusKey")).toString()
                  == QLatin1String("awaiting-jury-decision"),
              "status: awaiting Jury decision");
    }

    // 4) Jury confirms the official duration (4 minutes) — persists.
    check(inc.recordAcceptedDuration(240000, QStringLiteral("Chair"),
                                     QStringLiteral("stopwatch")),
          "officially accepted duration recorded");
    check(inc.activeIncident().value(QStringLiteral("acceptedDurationMs"))
              .toLongLong() == 240000,
          "accepted duration persists");
    check(inc.activeIncident().value(QStringLiteral("boundaryKey")).toString()
              == QLatin1String("over-3"),
          "4:00 classified as over-3 guidance");

    // 5) Authorised time credit — applied exactly once.
    check(inc.grantTimeCredit(240000, QStringLiteral("Chair"),
                              QStringLiteral("6.11.2 over-3-min")),
          "Jury time credit granted");
    check(!inc.grantTimeCredit(240000, QStringLiteral("Chair"),
                               QStringLiteral("again")),
          "second credit for the same incident refused");
    check(inc.remainingCompetitionMs() == frozenRemaining + 240000,
          "remaining = frozen + authorised credit (single application)",
          QString::number(inc.remainingCompetitionMs()));

    // 6) Recovery preparation + recovery sighting (authorised phases).
    check(inc.beginRecoveryPreparation(QStringLiteral("Chair")),
          "recovery preparation authorised");
    check(inc.beginRecoverySighting(QStringLiteral("Chair")),
          "recovery sighting authorised");
    // Recovery sighter: journalled, excluded from officials, tagged by seq.
    const int officialsBefore = qc.officialShotCount();
    const double totalBefore = qc.totalDecimal();
    check(qc.submitSighter(0, 0, 9.9, 20, 0, true),
          "recovery sighter accepted while blocked");
    check(qc.officialShotCount() == officialsBefore
              && qFuzzyCompare(qc.totalDecimal() + 1.0, totalBefore + 1.0),
          "recovery sighter excluded from official count/total");
    {
        const SessionState& s2 = qc.storeForTesting()->state();
        const EstIncidentRecord& r = s2.estIncidents.first();
        check(r.sightingGrantedSeq > 0
                  && s2.sighters.last().seq > r.sightingGrantedSeq,
              "recovery sighter deterministically tagged (seq bracket)");
        check(s2.sighters.first().seq < r.raisedSeq,
              "original sighter stays outside the recovery bracket");
    }

    // 7) Official resume gate: still blocked, then authorised → unblocked and
    //    the clock resumes from frozen+credit.
    check(!qc.submitOfficial(0, 0, 10.0, 21, 0, true),
          "official still refused before resume authorisation");
    clock.advance(300000);   // prep+sighting wall time
    check(inc.authoriseOfficialResume(QStringLiteral("Chair")),
          "official resume authorised (journalled)");
    check(!qc.storeForTesting()->state().timer.paused,
          "competition clock resumed (TimerResumed journalled)");
    check(inc.remainingCompetitionMs() == frozenRemaining + 240000,
          "remaining unchanged at the resume instant",
          QString::number(inc.remainingCompetitionMs()));
    check(qc.submitOfficial(0, 0, 10.6, 22, 0, true),
          "official accepted after authorisation");
    check(qc.officialShotCount() == 3
              && qc.storeForTesting()->state().officials.last().shot.shotNumber == 3,
          "resumed official continues numbering (shot 3)");

    // 8) Target reassignment + backup review persist on the record.
    check(inc.reassignTarget(QStringLiteral("T1"), QStringLiteral("T7"),
                             QStringLiteral("RO Smith"),
                             QStringLiteral("continuous fault")),
          "target reassignment recorded");
    check(inc.recordBackupReview(1, QStringLiteral("Chair"),
                                 QStringLiteral("printer roll matches")),
          "backup review (accepted) recorded");
    {
        const QVariantMap m = inc.activeIncident();
        check(m.value(QStringLiteral("targetMoved")).toBool()
                  && m.value(QStringLiteral("reserveTarget")).toString()
                         == QLatin1String("T7"),
              "reassignment persists (original/reserve)");
        check(m.value(QStringLiteral("backupReview")).toInt() == 1,
              "backup decision persists");
    }

    // 9) Resolve; actions on a closed incident are refused.
    check(inc.resolveIncident(QStringLiteral("jury note"),
                              QStringLiteral("ro note"),
                              QStringLiteral("IR-1")),
          "incident resolved");
    check(!inc.hasOpenIncident(), "no open incident after resolution");
    check(!inc.recordNoAllowance(QStringLiteral("Chair"), QString()),
          "decision after resolution refused");
    check(qc.submitOfficial(0, 0, 10.0, 23, 0, true),
          "officials keep flowing after resolution");

    // 10) The full workflow replays deterministically: same incident record,
    //     same adjusted remaining, no duplicate shots.
    {
        const ValidationReport rep = JournalValidator::validateBytes(file.data);
        check(rep.classification == JournalClassification::Clean,
              "workflow journal validates Clean (hash chain intact)");
        const ReplayResult rr = ReplayEngine::replay(rep.validEnvelopes);
        check(rr.ok, "workflow journal replays", rr.error.technicalDetail);
        check(rr.state.estIncidents.size() == 1
                  && rr.state.estIncidents == qc.storeForTesting()->state().estIncidents,
              "replayed incident record equals live record");
        const qint64 lastMono = rep.validEnvelopes.last().monotonicMs;
        check(EstIncidentController::remainingCompetitionMsFor(rr.state, lastMono)
                  == EstIncidentController::remainingCompetitionMsFor(
                         qc.storeForTesting()->state(), lastMono),
              "adjusted remaining clock reproduced by replay");
        check(rr.state.officials.size() == 4,
              "no duplicate officials after replay");
    }

    // 11) Explicit no-allowance ruling (second scenario, relay-wide) — a
    //     journalled tri-state distinct from decision-pending; contradictory
    //     rulings and unknown incidents are refused.
    {
        MemoryJournalFile f2; ManualClock c2;
        QualificationController q2;
        q2.storeForTesting()->setClockForTesting(&c2);
        q2.storeForTesting()->setJournalFileForTesting(&f2);
        q2.startSession(QStringLiteral("AP10"), QStringLiteral("60"),
                        QStringLiteral("A"), 60, 4500000, 900000, -1,
                        QString(), QString());
        q2.beginPreparation(); q2.beginSighting(); q2.beginOfficialMatch();
        q2.submitOfficial(0, 0, 10, 1, 0, true);
        EstIncidentController i2;
        i2.setStoreProvider([&q2]() { return q2.storeForTesting(); });
        check(i2.raiseIncident(3 /*network*/, 2 /*relay-wide*/, QString(),
                               QStringLiteral("R1"), QStringLiteral("network")),
              "raise relay-wide incident");
        check(i2.recordNoAllowance(QStringLiteral("Chair"),
                                   QStringLiteral("under 3 minutes")),
              "explicit no-allowance ruling journalled");
        {
            const QVariantMap m = i2.activeIncident();
            check(m.value(QStringLiteral("creditDecision")).toInt() == 1,
                  "tri-state: none-granted (distinct from pending)");
        }
        check(!i2.grantTimeCredit(1000, QStringLiteral("Chair"), QString()),
              "credit after no-allowance ruling refused");
        check(i2.remainingCompetitionMs()
                  == EstIncidentController::remainingCompetitionMsFor(
                         q2.storeForTesting()->state(), c2.nowMs()),
              "no timer credit added by the no-allowance path");
        check(i2.authoriseOfficialResume(QStringLiteral("Chair")),
              "resume authorised after no-allowance ruling");
        check(q2.submitOfficial(0, 0, 9, 2, 0, true),
              "AP10 integer official resumes (no decimal leakage)");
        check(qRound(q2.totalDecimal() * 10) % 10 == 0,
              "AP10 total stays integer through the incident");
        // The ruling survives crash/replay.
        const ReplayResult rr = ReplayEngine::replayBytes(f2.data);
        check(rr.ok && !rr.state.estIncidents.isEmpty()
                  && rr.state.estIncidents.first().creditDecision == 1,
              "no-allowance ruling survives replay");
    }

    // 12) Boundary guidance — exactly 3:00 and exactly 5:00 are boundary
    //     cases requiring an authorised decision; never auto-classified.
    check(EstIncidentController::boundaryKey(179000) == QLatin1String("under-3"),
          "2:59 → under-3 guidance");
    check(EstIncidentController::boundaryKey(180000)
              == QLatin1String("exactly-3-boundary"),
          "exactly 3:00 → boundary (manual decision, no auto-award/deny)");
    check(EstIncidentController::boundaryKey(181000) == QLatin1String("over-3"),
          "3:01 → over-3 guidance");
    check(EstIncidentController::boundaryKey(299000) == QLatin1String("over-3"),
          "4:59 → over-3 guidance");
    check(EstIncidentController::boundaryKey(300000)
              == QLatin1String("exactly-5-boundary"),
          "exactly 5:00 → boundary (manual decision, no auto-award/deny)");
    check(EstIncidentController::boundaryKey(301000) == QLatin1String("over-5"),
          "5:01 → over-5 guidance");
    // A boundary duration on a live incident stays decision-pending: recording
    // exactly 3:00 as the accepted duration must not create any allowance.
    {
        MemoryJournalFile f3; ManualClock c3;
        QualificationController q3;
        q3.storeForTesting()->setClockForTesting(&c3);
        q3.storeForTesting()->setJournalFileForTesting(&f3);
        q3.startSession(QStringLiteral("PRONE50"), QStringLiteral("60"),
                        QStringLiteral("A"), 60, 3000000, 900000, -1,
                        QString(), QString());
        q3.beginPreparation(); q3.beginSighting(); q3.beginOfficialMatch();
        EstIncidentController i3;
        i3.setStoreProvider([&q3]() { return q3.storeForTesting(); });
        i3.raiseIncident(1, 0, QStringLiteral("1"), QString(),
                         QStringLiteral("fault"));
        i3.recordAcceptedDuration(180000, QStringLiteral("Chair"),
                                  QStringLiteral("exact"));
        const QVariantMap m = i3.activeIncident();
        check(m.value(QStringLiteral("boundaryKey")).toString()
                  == QLatin1String("exactly-3-boundary"),
              "recorded exactly-3:00 surfaces as a boundary case");
        check(m.value(QStringLiteral("creditDecision")).toInt() == 0
                  && m.value(QStringLiteral("timeCreditMs")).toLongLong() == 0,
              "no automatic decision at the exact boundary");
    }
}
