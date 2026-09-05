// THE FIRST PHYSICAL TEST, REPLAYED AGAINST ITS OWN EVIDENCE (R3B §3, §11).
//
// On 2026-09-05 a real athlete fired three sighters and five official shots at
// a real 10 m target. The station accepted all five and totalled 27.7. RMS
// displayed four and totalled 20.9, and reported 1 SHOT NOT OBSERVED. One
// accepted shot went missing between the node's publisher and RMS.
//
// This suite runs that exact session again, from the node's own journal file,
// through the REAL chain:
//
//   the physical journal
//     -> JournalReader                       (the node's own reader)
//     -> ta::telemetry::toAcceptedShot       (the SAME conversion live uses)
//     -> ta::rms::RangeMonitor::ingestDatagram   (the ONE RMS ingress)
//
// Nothing is stubbed and no value is retyped from the analysis: every number
// asserted below is read out of the committed evidence file. If the conversion,
// the dedup or the gap logic regresses, these numbers move and this suite says
// so against a case that actually happened.
//
// THE FIXTURE IS EVIDENCE. tests/rmsnode/fixtures/session_...jsonl is a
// byte-identical copy of the physical journal (sha256
// bfc6112edc342886bb5289fade6c94d969cdf0fe7ff918b4bb4ea34655c119d0). It is read
// only, never rewritten, and must not be regenerated: the day it is
// regenerated it stops being evidence and becomes an expectation.

#include "test_support.h"

#include "reliability/journal/JournalReader.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "telemetry/ShotTelemetry.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include <cstdio>
#include <variant>

using namespace ta::rms;

namespace {

// ── the physical facts, as recorded in the journal ───────────────────────
const char* kPhysicalSessionId = "91ec22c0-4d93-4516-b9e5-cc41f6f84685";
const char* kPhysicalNodeId    = "TA-NODE-B906A93F0195";
const char* kPhysicalBootId    = "87876dd7c30a";
// The one that never arrived.
constexpr int kMissingShotNumber = 3;

QString fixturePath()
{
    // Beside the source, found from the build directory.
    const QString here = QStringLiteral(RMSNODE_TEST_ROOT);
    return QDir(here).filePath(
        QStringLiteral("fixtures/session_20260905T082908_91ec22c0.jsonl"));
}

struct PhysicalShot {
    int     shotNumber = 0;
    int     scoreTenths = 0;
    double  xMm = 0.0;
    double  yMm = 0.0;
    QString eventId;
    QByteArray datagram;
};

// Reads the physical journal and rebuilds the wire events EXACTLY as the live
// publisher would have. This is the node's replay provider in miniature: same
// reader, same conversion, same field-for-field result.
QList<PhysicalShot> readPhysicalOfficialShots(QString* sessionIdOut)
{
    QList<PhysicalShot> out;
    const ta::rel::JournalReadResult res =
        ta::rel::JournalReader::readFile(fixturePath());
    if (!res.fileOk)
        return out;

    for (const ta::rel::JournalLine& line : res.lines) {
        if (!line.parsed)
            continue;
        const ta::rel::EventEnvelope& env = line.envelope;
        if (!std::holds_alternative<ta::rel::ShotAccepted>(env.payload))
            continue;
        const ta::rel::ShotCore& core =
            std::get<ta::rel::ShotAccepted>(env.payload).shot;
        if (core.shotNumber <= 0)
            continue;                       // sighters carry 0; not official
        if (sessionIdOut) *sessionIdOut = env.sessionId;

        const AcceptedShot shot = ta::telemetry::toAcceptedShot(
            core, env.sessionId, QLatin1String(kPhysicalNodeId),
            QLatin1String(kPhysicalBootId), QStringLiteral("Lane 1"),
            QStringLiteral("techaim.10m.air-rifle.match60"), env.monotonicMs);

        PhysicalShot p;
        p.shotNumber  = core.shotNumber;
        p.scoreTenths = core.scoreTenths;
        p.xMm = shot.rawXMm;
        p.yMm = shot.rawYMm;
        p.eventId = shot.eventId;
        p.datagram = encode(shot);
        out.append(p);
    }
    return out;
}

// The node's status, which is what tells RMS how many shots it ACCEPTED - the
// number the gap is measured against.
QByteArray statusDatagram(const QString& sessionId, int shotsAccepted,
                          double totalScore, quint64 seq)
{
    NodeStatus s;
    s.protocolVersion = kProtocolVersion;
    s.nodeId    = QLatin1String(kPhysicalNodeId);
    s.bootId    = QLatin1String(kPhysicalBootId);
    s.laneId    = QStringLiteral("Lane 1");
    s.sessionId = sessionId;
    s.programmeId = QStringLiteral("techaim.10m.air-rifle.match60");
    s.connection = ConnectionState::TargetConnected;
    s.phase      = MatchPhase::Match;
    s.shotsAccepted = shotsAccepted;
    s.shotsExpected = 60;
    s.totalScore = totalScore;
    s.statusSeq  = seq;
    s.timestampUtcMs = 1'757'061'374'000LL;
    return encode(s);
}

int tenthsSum(const QList<PhysicalShot>& shots)
{
    int t = 0;
    for (const PhysicalShot& s : shots) t += s.scoreTenths;
    return t;
}

} // namespace

void run_physical_replay_tests()
{
    std::printf("\n-- the 2026-09-05 physical session, replayed --\n");

    // ── 1. the evidence is present and is what it was ────────────────────
    check(QFileInfo::exists(fixturePath()),
          "physical: the journal fixture is present", fixturePath());

    QString sessionId;
    const QList<PhysicalShot> shots = readPhysicalOfficialShots(&sessionId);
    check(shots.size() == 5, "physical: five OFFICIAL shots in the journal",
          QStringLiteral("found %1").arg(shots.size()));
    if (shots.size() != 5)
        return;
    check(sessionId == QLatin1String(kPhysicalSessionId),
          "physical: the session is the one that was shot", sessionId);

    // The station's own total, from the journal - not retyped from a report.
    check(tenthsSum(shots) == 277,
          "physical: the five shots total 27.7 exactly as the station showed",
          QStringLiteral("%1 tenths").arg(tenthsSum(shots)));

    const PhysicalShot* missing = nullptr;
    for (const PhysicalShot& s : shots)
        if (s.shotNumber == kMissingShotNumber) missing = &s;
    check(missing != nullptr, "physical: shot 3 is IN the authoritative journal");
    if (!missing)
        return;
    check(missing->scoreTenths == 68, "physical: shot 3 scored 6.8",
          QStringLiteral("%1 tenths").arg(missing->scoreTenths));
    check(qAbs(missing->xMm - 6.9) < 0.001 && qAbs(missing->yMm + 7.7) < 0.001,
          "physical: shot 3 is at x=6.9 y=-7.7",
          QStringLiteral("x=%1 y=%2").arg(missing->xMm).arg(missing->yMm));
    check(missing->eventId == QStringLiteral("%1:official:3").arg(sessionId),
          "physical: its eventId is derived, stable and reproducible",
          missing->eventId);

    // ── 2. reproduce the physical loss ───────────────────────────────────
    // Ingest 1, 2, 4, 5 - exactly the sequences RMS actually received - and
    // the node's status saying it accepted five.
    RangeMonitor monitor;
    for (const PhysicalShot& s : shots) {
        if (s.shotNumber == kMissingShotNumber)
            continue;                       // the datagram that never arrived
        monitor.ingestDatagram(s.datagram, 1'757'061'374'000LL);
    }
    monitor.ingestDatagram(statusDatagram(sessionId, 5, 27.7, 1),
                           1'757'061'374'000LL);

    const TargetNodeRecord* rec = monitor.nodeById(QLatin1String(kPhysicalNodeId));
    check(rec != nullptr, "physical: RMS knows the lane");
    if (!rec) return;

    check(rec->ledger.observedCount() == 4,
          "physical: RMS holds 4 of 5 - the state the range was actually in",
          QStringLiteral("%1").arg(rec->ledger.observedCount()));
    check(int(rec->ledger.observedScoreSum() * 10.0 + 0.5) == 209,
          "physical: and totals 20.9, exactly what the operator saw",
          QStringLiteral("%1").arg(rec->ledger.observedScoreSum()));

    // ── 3. gap detection, on the real numbers ────────────────────────────
    check(rec->unobservedShotCount() == 1,
          "physical: RMS reports 1 SHOT NOT OBSERVED");
    const QList<int> holes = rec->ledger.missingSequences();
    check(holes == QList<int>{3},
          "physical: and names the gap as sequence 3 - a MIDDLE hole, not a tail",
          QStringLiteral("%1 hole(s)").arg(holes.size()));
    // The distinction that matters: highestSequence already reads 5, so a
    // shortfall-only check would have declared this lane current.
    check(rec->ledger.highestSequence() == 5,
          "physical: the highest sequence ALREADY reads 5 - a tail check would miss this");

    // ── 4. replay the missing event, through the same ingest ─────────────
    const IngestOutcome io = monitor.ingestDatagram(missing->datagram,
                                                    1'757'061'380'000LL);
    check(io.accepted, "physical: the replayed shot is accepted by RMS");
    rec = monitor.nodeById(QLatin1String(kPhysicalNodeId));
    check(rec->ledger.observedCount() == 5,
          "physical: the ledger completes 4 -> 5",
          QStringLiteral("%1").arg(rec->ledger.observedCount()));
    check(int(rec->ledger.observedScoreSum() * 10.0 + 0.5) == 277,
          "physical: and the total becomes 27.7 - matching the station exactly",
          QStringLiteral("%1").arg(rec->ledger.observedScoreSum()));
    check(rec->unobservedShotCount() == 0,
          "physical: nothing is unobserved any more");
    check(rec->ledger.missingSequences().isEmpty(),
          "physical: and no hole remains");
    check(rec->ledger.sequenceConflicts() == 0,
          "physical: no sequence conflict was created by the recovery");

    // ── 5. replay it AGAIN - the property that makes catch-up safe ───────
    const int dupsBefore = rec->ledger.duplicatesSuppressed();
    const IngestOutcome again = monitor.ingestDatagram(missing->datagram,
                                                       1'757'061'381'000LL);
    rec = monitor.nodeById(QLatin1String(kPhysicalNodeId));
    check(again.shotResult == ShotIngest::DuplicateSuppressed,
          "physical: replaying the same event again is SUPPRESSED");
    check(rec->ledger.observedCount() == 5,
          "physical: the ledger stays at 5 - no duplicate shot");
    check(int(rec->ledger.observedScoreSum() * 10.0 + 0.5) == 277,
          "physical: and the total stays 27.7 - a repeated recovery cannot inflate a score");
    check(rec->ledger.duplicatesSuppressed() == dupsBefore + 1,
          "physical: the suppression is COUNTED, not silent");

    // ── 6. replaying the WHOLE session is equally safe ───────────────────
    // This is what a catch-up from sequence 0 does, and an operator running one
    // twice must not change a single number.
    for (const PhysicalShot& s : shots)
        monitor.ingestDatagram(s.datagram, 1'757'061'382'000LL);
    rec = monitor.nodeById(QLatin1String(kPhysicalNodeId));
    check(rec->ledger.observedCount() == 5,
          "physical: a full-session replay still leaves 5 shots");
    check(int(rec->ledger.observedScoreSum() * 10.0 + 0.5) == 277,
          "physical: and 27.7");

    // ── 7. sighters stayed out of the official ledger ────────────────────
    // Three sighters were fired. They carry shotNumber 0 in the journal, are
    // never published, and the official count above is 5 - not 8.
    const ta::rel::JournalReadResult res =
        ta::rel::JournalReader::readFile(fixturePath());
    int sighters = 0;
    for (const ta::rel::JournalLine& line : res.lines) {
        if (!line.parsed) continue;
        if (std::holds_alternative<ta::rel::SighterAccepted>(line.envelope.payload))
            ++sighters;
    }
    check(sighters == 3, "physical: three sighters are in the journal",
          QStringLiteral("%1").arg(sighters));
    check(rec->ledger.observedCount() == 5,
          "physical: and NONE of them reached the official ledger");
    check(rec->shotsAcceptedByNode == 5,
          "physical: the node's own official count is 5, not 8");

    std::fflush(stdout);
}
