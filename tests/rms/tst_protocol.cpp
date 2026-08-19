// Wire protocol: round-trip, version gating, rejection, and the rules that
// keep programme identity stable.

#include "test_support.h"

#include "rms/ProgrammeDisplay.h"
#include "rms/RmsProtocol.h"

#include <cstdio>

using namespace ta::rms;

namespace {

NodeStatus sampleStatus()
{
    NodeStatus s;
    s.nodeId           = QStringLiteral("TA-NODE-001");
    s.bootId           = QStringLiteral("boot-1-a");
    s.laneId           = QStringLiteral("Lane 1");
    s.sessionId        = QStringLiteral("sess-1");
    s.programmeId      = QStringLiteral("issf.10m.air-rifle.qualification60");
    s.rulesetId        = QStringLiteral("issf");
    s.targetStandardId = QStringLiteral("issf.10m.air-rifle");
    s.athleteName      = QStringLiteral("A. Bailie");
    s.position         = QStringLiteral("STANDING");
    s.connection       = ConnectionState::TargetConnected;
    s.phase            = MatchPhase::Match;
    s.shotsAccepted    = 17;
    s.shotsExpected    = 60;
    s.totalScore       = 164.2;
    s.health           = QStringLiteral("OK");
    s.statusSeq        = 9;
    s.timestampUtcMs   = 1700000000000LL;
    return s;
}

AcceptedShot sampleShot()
{
    AcceptedShot sh;
    sh.eventId      = QStringLiteral("evt-1");
    sh.nodeId       = QStringLiteral("TA-NODE-001");
    sh.bootId       = QStringLiteral("boot-1-a");
    sh.laneId       = QStringLiteral("Lane 1");
    sh.sessionId    = QStringLiteral("sess-1");
    sh.programmeId  = QStringLiteral("issf.10m.air-rifle.qualification60");
    sh.position     = QStringLiteral("STANDING");
    sh.shotSequence = 17;
    sh.rawXMm       = -1.25;
    sh.rawYMm       = 2.5;
    sh.authoritativeScore = 10.4;
    sh.integerScore = 10;
    sh.innerTen     = false;
    sh.timestampUtcMs = 1700000000000LL;
    sh.acquisitionStatus = QStringLiteral("ACCEPTED");
    return sh;
}

} // namespace

void run_protocol_tests()
{
    std::printf("\n-- protocol --\n");

    // ── announce round-trip ────────────────────────────────────────────
    {
        NodeAnnounce a;
        a.nodeId = QStringLiteral("TA-NODE-042");
        a.bootId = QStringLiteral("boot-x");
        a.laneId = QStringLiteral("Lane 4");
        a.deviceIdentity = QStringLiteral("TechAim-EST/4103");
        a.appVersion = QStringLiteral("0.9.0");
        a.productIdentity = QStringLiteral("Tech Aim");
        const DecodedMessage d = decode(encode(a));
        check(d.type == MessageType::NodeAnnounce, "announce round-trips", d.rejectReason);
        check(d.announce.nodeId == a.nodeId, "announce keeps nodeId");
        check(d.announce.bootId == a.bootId, "announce keeps bootId");
        check(d.announce.deviceIdentity == a.deviceIdentity, "announce keeps device identity");
    }

    // ── status round-trip ──────────────────────────────────────────────
    {
        const NodeStatus s = sampleStatus();
        const DecodedMessage d = decode(encode(s));
        check(d.type == MessageType::NodeStatus, "status round-trips", d.rejectReason);
        check(d.status.programmeId == s.programmeId, "status keeps programmeId");
        check(d.status.rulesetId == s.rulesetId, "status keeps rulesetId");
        check(d.status.targetStandardId == s.targetStandardId, "status keeps targetStandardId");
        check(d.status.connection == ConnectionState::TargetConnected,
              "status keeps connection state");
        check(d.status.phase == MatchPhase::Match, "status keeps match phase");
        check(d.status.shotsAccepted == 17 && d.status.shotsExpected == 60,
              "status keeps node shot counts");
        check(qAbs(d.status.totalScore - 164.2) < 1e-9, "status keeps node total score");
        check(d.status.statusSeq == 9u, "status keeps statusSeq");
    }

    // ── accepted shot round-trip ───────────────────────────────────────
    {
        const AcceptedShot sh = sampleShot();
        const DecodedMessage d = decode(encode(sh));
        check(d.type == MessageType::AcceptedShot, "accepted shot round-trips", d.rejectReason);
        check(d.shot.eventId == sh.eventId, "shot keeps eventId");
        check(d.shot.shotSequence == 17, "shot keeps shotSequence");
        check(qAbs(d.shot.authoritativeScore - 10.4) < 1e-9,
              "shot keeps the NODE's authoritative score verbatim");
        check(d.shot.integerScore == 10, "shot keeps integer score");
        check(qAbs(d.shot.rawXMm + 1.25) < 1e-9 && qAbs(d.shot.rawYMm - 2.5) < 1e-9,
              "shot keeps raw coordinates (diagnostics only)");
    }

    // ── version gating ─────────────────────────────────────────────────
    {
        QByteArray future = encode(sampleStatus());
        future.replace("\"protocolVersion\":1", "\"protocolVersion\":2");
        const DecodedMessage d = decode(future);
        check(d.type == MessageType::Unknown, "a newer protocolVersion is REJECTED, never guessed");
        check(d.rejectReason.contains(QStringLiteral("unsupported")),
              "rejection says why", d.rejectReason);
    }
    {
        const DecodedMessage d = decode(QByteArray("{\"type\":\"node.status\",\"nodeId\":\"n\"}"));
        check(d.type == MessageType::Unknown, "a message without protocolVersion is rejected");
    }

    // ── forward compatibility ──────────────────────────────────────────
    {
        QByteArray extended = encode(sampleStatus());
        extended.insert(1, "\"aFieldFromAFutureBuild\":{\"nested\":true},");
        const DecodedMessage d = decode(extended);
        check(d.type == MessageType::NodeStatus,
              "unknown FIELDS are ignored (forward compatible)", d.rejectReason);
        check(d.status.shotsAccepted == 17, "ignoring unknown fields does not disturb known ones");
    }

    // ── malformed and incomplete input ─────────────────────────────────
    {
        check(decode(QByteArray("not json at all")).type == MessageType::Unknown,
              "malformed JSON is rejected");
        check(decode(QByteArray()).type == MessageType::Unknown,
              "an empty datagram is rejected");
        check(decode(QByteArray("{\"protocolVersion\":1,\"type\":\"node.status\"}")).type
                  == MessageType::Unknown,
              "a status without nodeId is rejected");
        check(decode(QByteArray("{\"protocolVersion\":1,\"type\":\"node.command\","
                                "\"nodeId\":\"n\"}")).type == MessageType::Unknown,
              "an unknown message TYPE is rejected - there is no command type to decode");

        AcceptedShot bad = sampleShot();
        bad.shotSequence = 0;
        check(decode(encode(bad)).type == MessageType::Unknown,
              "a shot with a non-positive shotSequence is rejected");
        bad = sampleShot();
        bad.eventId.clear();
        check(decode(encode(bad)).type == MessageType::Unknown,
              "a shot without an eventId is rejected - dedup would be impossible");
        bad = sampleShot();
        bad.sessionId.clear();
        check(decode(encode(bad)).type == MessageType::Unknown,
              "a shot without a sessionId is rejected");
    }

    // ── OFFLINE is an RMS conclusion, not a node claim ─────────────────
    {
        NodeStatus s = sampleStatus();
        s.connection = ConnectionState::Offline;
        const DecodedMessage d = decode(encode(s));
        check(d.type == MessageType::NodeStatus, "status carrying OFFLINE still parses");
        check(d.status.connection == ConnectionState::Unknown,
              "a node cannot declare itself OFFLINE - RMS derives that from silence");
    }

    // ── enum tokens are stable wire constants ──────────────────────────
    {
        check(toString(MatchPhase::PositionChange) == QLatin1String("POSITION_CHANGE"),
              "phase token POSITION_CHANGE is stable");
        check(toString(MatchPhase::RecoveryRequired) == QLatin1String("RECOVERY_REQUIRED"),
              "phase token RECOVERY_REQUIRED is stable");
        check(matchPhaseFromString(QStringLiteral("SIGHTING")) == MatchPhase::Sighting,
              "phase token SIGHTING round-trips");
        check(matchPhaseFromString(QStringLiteral("Match")) == MatchPhase::Unknown,
              "phase tokens are case-sensitive constants, not prose");
    }

    // ── programme identity ─────────────────────────────────────────────
    {
        check(ProgrammeDisplay::describe(QStringLiteral("issf.10m.air-rifle.qualification60"))
                  == QStringLiteral("10 m Air Rifle · Qualification 60"),
              "programme label is DERIVED from the stable id");
        check(ProgrammeDisplay::describe(QStringLiteral("techaim.50m.pistol.match40.p15"))
                  == QStringLiteral("50 m Pistol · Match 40 (15-shot)"),
              "the 15-shot variant marker is rendered, not dropped");
        check(ProgrammeDisplay::describe(QStringLiteral("some.unknown.identity.form.here"))
                  == QStringLiteral("some.unknown.identity.form.here"),
              "an unrecognised id is shown VERBATIM, never given an invented name");
        check(ProgrammeDisplay::describe(QString()).isEmpty(),
              "an empty programmeId produces no label");
        check(ProgrammeDisplay::shortDescribe(
                  QStringLiteral("issf.50m.rifle.qualification60"))
                  == QStringLiteral("50 m Rifle"),
              "the short label drops the course, not the discipline");

        // The load-bearing rule: OFFICIAL is decided by rulesetId. A Tech Aim
        // preset shoots on an ISSF target under ISSF scoring, and still is not
        // an ISSF event.
        check(ProgrammeDisplay::isOfficialProgramme(QStringLiteral("issf")),
              "rulesetId 'issf' is an official competition course");
        check(!ProgrammeDisplay::isOfficialProgramme(QStringLiteral("techaim")),
              "rulesetId 'techaim' is a PRESET, not an official ISSF event");
        check(!ProgrammeDisplay::isOfficialProgramme(QString()),
              "an unset rulesetId is never treated as official");

        // Two programmes on the SAME ISSF target standard, one official and
        // one not — proof that target standard and competition authority are
        // separate axes.
        NodeStatus a = sampleStatus();
        NodeStatus b = sampleStatus();
        b.programmeId = QStringLiteral("techaim.10m.air-rifle.match40");
        b.rulesetId   = QStringLiteral("techaim");
        check(decode(encode(a)).status.targetStandardId
                  == decode(encode(b)).status.targetStandardId,
              "official and preset programmes can share one target standard");
        check(ProgrammeDisplay::isOfficialProgramme(decode(encode(a)).status.rulesetId)
                  != ProgrammeDisplay::isOfficialProgramme(decode(encode(b)).status.rulesetId),
              "...while differing in competition authority");
    }
}
