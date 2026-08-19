// COMPETITION STATUS — a third axis, and the invariant that RMS never decides
// it.
//
// Elimination in a 3P final is determined by Finals3PController on the target
// node against the ISSF rules. RMS is a display. The tests below are the
// enforcement of that: every plausible-looking heuristic RMS might be tempted
// to use is exercised, and each one must leave the competition status exactly
// where it was — UNKNOWN.

#include "test_support.h"

#include "rms/CompetitionState.h"
#include "rms/LaneListModel.h"
#include "rms/RangeConfigurationService.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"

#include <QTemporaryDir>

#include <cstdio>

using namespace ta::rms;

namespace {

QString nodeName(int n)
{
    return QStringLiteral("TA-NODE-%1").arg(n, 3, 10, QLatin1Char('0'));
}

QByteArray announceFor(const QString& nodeId, const QString& bootId)
{
    NodeAnnounce a;
    a.nodeId = nodeId;
    a.bootId = bootId;
    return encode(a);
}

QByteArray statusFor(const QString& nodeId, quint64 seq, int shots, double total,
                     ConnectionState conn = ConnectionState::TargetConnected,
                     MatchPhase phase = MatchPhase::Match)
{
    NodeStatus s;
    s.nodeId = nodeId;
    s.bootId = QStringLiteral("boot-a");
    s.sessionId = QStringLiteral("sess-%1").arg(nodeId);
    s.programmeId = QStringLiteral("issf.10m.air-rifle.qualification60");
    s.rulesetId = QStringLiteral("issf");
    s.athleteName = QStringLiteral("A. Bailie");
    s.connection = conn;
    s.phase = phase;
    s.shotsAccepted = shots;
    s.shotsExpected = 60;
    s.totalScore = total;
    s.statusSeq = seq;
    return encode(s);
}

struct Rig {
    QTemporaryDir dir;
    RangeConfigurationService range;
    RangeMonitor monitor;
    LaneListModel lanes{&range, &monitor};

    Rig()
    {
        range.setStorePath(dir.filePath(QStringLiteral("range.json")));
        range.load();
        range.createFixedRange(QStringLiteral("Finals bay"), QStringLiteral("50 m"), 1, 8);
        for (int i = 1; i <= 8; ++i) {
            monitor.ingestDatagram(announceFor(nodeName(i), QStringLiteral("boot-a")), 1000);
            monitor.ingestDatagram(statusFor(nodeName(i), 1, 5, 48.5), 1000);
            range.assignNodeToLane(nodeName(i), i);
        }
    }

    QVariant laneRole(int laneNumber, int role) const
    {
        const int row = range.range().indexOfLaneNumber(laneNumber);
        return row < 0 ? QVariant() : lanes.data(lanes.index(row, 0), role);
    }
    const TargetNodeRecord* rec(int n) const { return monitor.nodeById(nodeName(n)); }
};

} // namespace

void run_competition_state_tests()
{
    std::printf("\n-- competition status: tokens --\n");
    {
        check(toString(CompetitionStatus::Eliminated) == QLatin1String("ELIMINATED"),
              "ELIMINATED is a stable token");
        check(toString(CompetitionStatus::Active) == QLatin1String("ACTIVE"),
              "ACTIVE is a stable token");
        check(toString(CompetitionStatus::Waiting) == QLatin1String("WAITING"),
              "WAITING is a stable token");
        check(toString(CompetitionStatus::Finished) == QLatin1String("FINISHED"),
              "FINISHED is a stable token");

        bool ok = false;
        check(competitionStatusFromString(QStringLiteral("ELIMINATED"), &ok)
                  == CompetitionStatus::Eliminated && ok,
              "the future v2 decoder reads ELIMINATED back");
        competitionStatusFromString(QStringLiteral("Eliminated"), &ok);
        check(!ok, "tokens are case-sensitive constants, not prose");
        competitionStatusFromString(QStringLiteral("ausgeschieden"), &ok);
        check(!ok, "a TRANSLATED word is not understood - and is not guessed at");
        check(competitionStatusFromString(QStringLiteral("ausgeschieden"))
                  == CompetitionStatus::Unknown,
              "...it resolves to UNKNOWN, never to ELIMINATED");

        CompetitionState s;
        check(s.status == CompetitionStatus::Unknown, "the default status is UNKNOWN");
        check(s.source == CompetitionState::Source::NotReported,
              "...from no source");
        check(!s.isReported() && !s.isTerminal() && !s.isEliminated(),
              "...and is neither reported nor terminal");
        // QStringLiteral, not QLatin1String: the em dash is multi-byte UTF-8
        // and Latin-1 would compare it as two characters.
        check(s.scoreLabel() == QStringLiteral("—"),
              "an unreported final score is a dash, not a zero");

        s.rank = 1;  check(s.rankLabel() == QLatin1String("1st"), "1st");
        s.rank = 2;  check(s.rankLabel() == QLatin1String("2nd"), "2nd");
        s.rank = 3;  check(s.rankLabel() == QLatin1String("3rd"), "3rd");
        s.rank = 8;  check(s.rankLabel() == QLatin1String("8th"), "8th");
        s.rank = 11; check(s.rankLabel() == QLatin1String("11th"), "11th, not 11st");
        s.rank = 0;  check(s.rankLabel().isEmpty(), "an unreported rank has no label");
    }

    std::printf("\n-- RMS NEVER INFERS ELIMINATION --\n");
    {
        Rig rig;
        check(rig.rec(1)->competition.status == CompetitionStatus::Unknown,
              "a station reporting normally has UNKNOWN competition status");
        check(!rig.rec(1)->competition.isReported(),
              "...because protocol v1 carries no such field");

        // Every heuristic RMS might be tempted to use, one at a time.

        // 1. Score alone. A very low total is not an elimination.
        rig.monitor.ingestDatagram(statusFor(nodeName(1), 2, 60, 1.0), 2000);
        check(rig.rec(1)->competition.status == CompetitionStatus::Unknown,
              "a LOW SCORE does not make an athlete eliminated");

        // 2. Shot count / course completion. Finishing the course is not it.
        rig.monitor.ingestDatagram(statusFor(nodeName(2), 2, 60, 621.4), 2000);
        check(rig.rec(2)->competition.status == CompetitionStatus::Unknown,
              "COMPLETING THE COURSE does not make an athlete eliminated");

        // 3. The node's own COMPLETE phase. Still a match phase, not a
        //    competition placing.
        rig.monitor.ingestDatagram(
            statusFor(nodeName(3), 2, 60, 600.0, ConnectionState::TargetConnected,
                      MatchPhase::Complete), 2000);
        check(rig.rec(3)->competition.status == CompetitionStatus::Unknown,
              "a COMPLETE match phase does not make an athlete eliminated");
        check(rig.laneRole(3, LaneListModel::PhaseRole).toString()
                  == QLatin1String("COMPLETE"),
              "...the phase is still reported as itself");

        // 4. Rank. RMS holds no ranking and must not build one to decide this.
        //    Lane 4 has the worst total on the range; it changes nothing.
        rig.monitor.ingestDatagram(statusFor(nodeName(4), 2, 10, 0.5), 2000);
        check(rig.rec(4)->competition.status == CompetitionStatus::Unknown,
              "being LAST ON THE RANGE does not make an athlete eliminated");

        // 5. Another athlete disappearing.
        rig.monitor.evaluateLiveness(999999);
        check(rig.rec(1)->competition.status == CompetitionStatus::Unknown,
              "OTHER ATHLETES GOING OFFLINE does not eliminate anybody");
        check(rig.rec(5)->competition.status == CompetitionStatus::Unknown,
              "...including the ones that went offline themselves");

        // 6. Going offline is a node fault, never a competition outcome.
        check(rig.rec(5)->isOffline(), "lane 5's station is offline");
        check(!rig.rec(5)->competition.isEliminated(),
              "an OFFLINE station is not an eliminated athlete");

        // And the whole range, after all of that.
        int inferred = 0;
        for (int i = 1; i <= 8; ++i)
            if (rig.rec(i)->competition.status != CompetitionStatus::Unknown)
                ++inferred;
        check(inferred == 0,
              "after every heuristic above, RMS has inferred NOTHING",
              QString::number(inferred));
    }

    std::printf("\n-- the three statuses are independent --\n");
    {
        Rig rig;
        CompetitionState eliminated;
        eliminated.status = CompetitionStatus::Eliminated;
        eliminated.rank = 8;
        eliminated.finalScore = 402.7;
        eliminated.finalScoreReported = true;
        eliminated.eliminatedAtStage = QStringLiteral("STANDING");
        rig.monitor.injectDevelopmentCompetitionState(nodeName(6), eliminated);

        check(rig.rec(6)->competition.isEliminated(), "lane 6's athlete is eliminated");
        // The point of the whole exercise: the station is fine.
        check(!rig.rec(6)->isOffline(), "...and the station is STILL ONLINE");
        check(rig.rec(6)->connection == ConnectionState::TargetConnected,
              "...with its target STILL CONNECTED");
        check(rig.laneRole(6, LaneListModel::ConnectionRole).toString()
                  == QLatin1String("TARGET_CONNECTED"),
              "...and the lane reports target health unchanged");
        check(rig.laneRole(6, LaneListModel::OnlineRole).toBool(),
              "...and reports node health unchanged");
        check(rig.laneRole(6, LaneListModel::CompetitionStatusRole).toString()
                  == QLatin1String("ELIMINATED"),
              "...while the COMPETITION status reads ELIMINATED");

        // The lane stays on the range. Removing it would lose an athlete the
        // officials still have to account for.
        check(rig.lanes.rowCountProperty() == 8,
              "an eliminated athlete's lane is NOT removed from the range");
        check(rig.lanes.onlineCount() == 8,
              "...and still counts as an online lane");

        check(rig.laneRole(6, LaneListModel::EliminatedRole).toBool(),
              "the lane exposes the elimination to a display");
        check(rig.laneRole(6, LaneListModel::CompetitionTerminalRole).toBool(),
              "...as a TERMINAL state, so a display stops inviting them to shoot");
        check(rig.laneRole(6, LaneListModel::FinalRankLabelRole).toString()
                  == QLatin1String("8th"), "...with the final rank");
        check(rig.laneRole(6, LaneListModel::FinalScoreLabelRole).toString()
                  == QLatin1String("402.7"), "...and the node's final score");

        // Anything injected is marked, so a demonstration can never be mistaken
        // for a real elimination.
        check(rig.rec(6)->competition.isSimulated(),
              "an injected state is tagged as a DEVELOPMENT injection");
        check(rig.laneRole(6, LaneListModel::CompetitionSimulatedRole).toBool(),
              "...and every surface can label it SIMULATED");

        // Neighbouring lanes are untouched.
        check(rig.rec(5)->competition.status == CompetitionStatus::Unknown,
              "injecting one lane does not touch another");

        const QVariantMap detail =
            rig.lanes.laneDetail(rig.range.range().indexOfLaneNumber(6));
        check(detail.value(QStringLiteral("competitionStatus")).toString()
                  == QLatin1String("ELIMINATED"),
              "the lane detail carries the competition status");
        check(detail.value(QStringLiteral("eliminatedAtStage")).toString()
                  == QLatin1String("STANDING"),
              "...and where the final was when it happened");
        check(detail.value(QStringLiteral("connection")).toString()
                  == QLatin1String("TARGET_CONNECTED"),
              "...beside the unchanged target status");
    }

    std::printf("\n-- terminal states other than elimination --\n");
    {
        Rig rig;
        CompetitionState finished;
        finished.status = CompetitionStatus::Finished;
        finished.rank = 1;
        finished.finalScore = 463.2;
        finished.finalScoreReported = true;
        rig.monitor.injectDevelopmentCompetitionState(nodeName(1), finished);

        check(rig.laneRole(1, LaneListModel::CompetitionTerminalRole).toBool(),
              "FINISHED is terminal too - a display must handle more than elimination");
        check(!rig.laneRole(1, LaneListModel::EliminatedRole).toBool(),
              "...but it is NOT an elimination");
        check(rig.laneRole(1, LaneListModel::FinalRankLabelRole).toString()
                  == QLatin1String("1st"), "...and the winner is 1st");

        CompetitionState waiting;
        waiting.status = CompetitionStatus::Waiting;
        rig.monitor.injectDevelopmentCompetitionState(nodeName(2), waiting);
        check(!rig.laneRole(2, LaneListModel::CompetitionTerminalRole).toBool(),
              "WAITING is not terminal - the athlete shoots again");
    }

    std::printf("\n-- injection cannot come from the wire --\n");
    {
        Rig rig;
        // A datagram that names the field it does not have. v1 ignores unknown
        // fields, so this must change nothing at all.
        QByteArray forged = statusFor(nodeName(7), 5, 20, 200.0);
        forged.insert(1, "\"competitionStatus\":\"ELIMINATED\",\"rank\":8,");
        const IngestOutcome out = rig.monitor.ingestDatagram(forged, 3000);

        check(out.accepted, "the datagram is accepted - unknown fields are ignored");
        check(rig.rec(7)->competition.status == CompetitionStatus::Unknown,
              "a v1 datagram CANNOT set a competition status, however it is dressed");
        check(rig.rec(7)->shotsAcceptedByNode == 20,
              "...while the fields v1 does define are applied normally");
        check(!rig.rec(7)->competition.isReported(),
              "...and the status is still recorded as not reported");
    }
}
