// ATHLETES, MATCH PLANS AND READINESS — milestone 4.
//
// The property under test throughout is that PLANNING and OBSERVATION stay
// apart. A plan records what the operator intends; it is never sent anywhere,
// so it may legitimately disagree with what a station reports, and that
// disagreement is information rather than something to reconcile away.

#include "test_support.h"

#include "rms/AthleteListModel.h"
#include "rms/AthleteRegistry.h"
#include "rms/LaneListModel.h"
#include "rms/MatchPlanService.h"
#include "rms/PlanLaneModel.h"
#include "rms/RangeConfigurationService.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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
    a.deviceIdentity = QStringLiteral("TechAim-EST/4100");
    return encode(a);
}

QByteArray statusFor(const QString& nodeId, const QString& bootId, quint64 seq,
                     ConnectionState conn = ConnectionState::TargetConnected,
                     const QString& programmeId
                         = QStringLiteral("issf.10m.air-rifle.qualification60"))
{
    NodeStatus s;
    s.nodeId = nodeId;
    s.bootId = bootId;
    s.sessionId = QStringLiteral("sess-%1").arg(nodeId);
    s.programmeId = programmeId;
    s.rulesetId = QStringLiteral("issf");
    s.targetStandardId = QStringLiteral("issf.10m.air-rifle");
    s.connection = conn;
    s.phase = MatchPhase::Match;
    s.shotsAccepted = 5;
    s.shotsExpected = 60;
    s.totalScore = 48.5;
    s.statusSeq = seq;
    return encode(s);
}

// The catalogue entry an operator would pick, passed exactly as the QML
// programme picker passes it (ids first; the label is a snapshot).
struct Programme {
    const char* id;      const char* ruleset;  const char* targetStandard;
    const char* discipline; int distance;      int shots;
    const char* type;    const char* label;
};
const Programme kAr60 = {
    "issf.10m.air-rifle.qualification60", "issf", "issf.10m.air-rifle",
    "AR10", 10, 60, "OFFICIAL", "10 m Air Rifle · Qualification 60" };
const Programme kAp20 = {
    "techaim.10m.air-pistol.match20", "techaim", "issf.10m.air-pistol",
    "AP10", 10, 20, "PRESET", "10 m Air Pistol · Match 20" };

bool applyProgramme(MatchPlanService& plans, const Programme& p)
{
    return plans.setProgramme(QString::fromLatin1(p.id), QString::fromLatin1(p.ruleset),
                              QString::fromLatin1(p.targetStandard),
                              QString::fromLatin1(p.discipline), p.distance, p.shots,
                              QString::fromLatin1(p.type), QString::fromLatin1(p.label));
}

// A ten-lane range with `online` stations assigned to lanes 1..online.
struct Rig {
    QTemporaryDir dir;
    RangeConfigurationService range;
    RangeMonitor monitor;
    AthleteRegistry athletes;
    MatchPlanService plans{&range, &monitor, &athletes};

    Rig()
    {
        range.setStorePath(dir.filePath(QStringLiteral("range.json")));
        athletes.setStorePath(dir.filePath(QStringLiteral("athletes.json")));
        plans.setStorePath(dir.filePath(QStringLiteral("plans.json")));
        plans.setClockForTesting([] { return qint64(1700000000000LL); });
        range.load();
        athletes.load();
        plans.load();
    }

    void buildRange(int lanes = 10, int assign = 6)
    {
        range.createFixedRange(QStringLiteral("Potchefstroom 50 m"),
                               QStringLiteral("50 m"), 1, lanes);
        for (int i = 1; i <= assign; ++i) {
            const QString id = nodeName(i);
            monitor.ingestDatagram(announceFor(id, QStringLiteral("boot-%1-a").arg(i)), 1000);
            monitor.ingestDatagram(statusFor(id, QStringLiteral("boot-%1-a").arg(i), 1), 1000);
            range.assignNodeToLane(id, i);
        }
    }
};

} // namespace

void run_match_plan_tests()
{
    std::printf("\n-- athletes --\n");
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("athletes.json"));
        QString idA, idB;
        {
            AthleteRegistry reg;
            reg.setStorePath(path);
            reg.load();
            check(reg.count() == 0, "a fresh RMS has an empty start list");

            idA = reg.addAthlete(QStringLiteral("Arnold Bailie"));
            check(!idA.isEmpty(), "an athlete can be added from a name alone");
            check(reg.count() == 1, "...and appears on the start list");
            check(reg.displayNameFor(idA) == QLatin1String("Arnold Bailie"),
                  "...under that name");

            // TWO PEOPLE MAY SHARE A NAME. The id is what an assignment refers
            // to, so this is allowed and is not even unusual on a club list.
            idB = reg.addAthlete(QStringLiteral("Arnold Bailie"));
            check(!idB.isEmpty() && idB != idA,
                  "a second athlete with the SAME display name is allowed");
            check(reg.count() == 2, "...and both are kept");

            check(reg.addAthlete(QString()).isEmpty(), "an athlete without a name is refused");
            check(reg.addAthlete(QStringLiteral("   ")).isEmpty(),
                  "a whitespace-only name is refused");

            check(reg.updateAthlete(idA, QStringLiteral("A. Bailie"),
                                    QStringLiteral("Potchefstroom"),
                                    QStringLiteral("RSA"), QString()),
                  "an athlete can be edited");
            check(reg.displayNameFor(idA) == QLatin1String("A. Bailie"),
                  "...and the edit takes effect");
        }

        // A new RMS process, reading only what is on disk.
        AthleteRegistry restarted;
        restarted.setStorePath(path);
        restarted.load();
        check(restarted.count() == 2, "the start list survives an RMS restart");
        check(restarted.displayNameFor(idA) == QLatin1String("A. Bailie"),
              "...including the edit");
        check(restarted.athleteAt(0).value(QStringLiteral("club")).toString()
                  == QLatin1String("Potchefstroom"),
              "...and the club");

        check(restarted.removeAthlete(idB), "an unused athlete can be removed");
        check(restarted.count() == 1, "...and is gone");
    }

    std::printf("\n-- match plan --\n");
    {
        Rig rig;
        check(rig.plans.createPlan(QStringLiteral("Morning Relay")).isEmpty(),
              "a match cannot be prepared before a range is configured");

        rig.buildRange();
        const QString planId = rig.plans.createPlan(QStringLiteral("Morning Relay"));
        check(!planId.isEmpty(), "a match plan is created against the configured range");
        check(rig.plans.hasPlan(), "...and becomes the plan being edited");
        check(rig.plans.planStatusLabel() == QLatin1String("DRAFT"),
              "...as a DRAFT");
        check(rig.plans.planId() != rig.range.range().rangeId,
              "the planId is its own identity");

        check(applyProgramme(rig.plans, kAr60), "a programme is selected");
        check(rig.plans.programmeId() == QLatin1String("issf.10m.air-rifle.qualification60"),
              "...by its STABLE id");
        check(rig.plans.programmeLabel() == QLatin1String("10 m Air Rifle · Qualification 60"),
              "...with the label kept only as a snapshot");
        check(rig.plans.current().programme.shotCount == 60,
              "the programme snapshot carries the course length");
        check(rig.plans.current().programme.isOfficial(),
              "...and whether it is an official course");
        check(rig.plans.current().programme.targetStandardId
                  == QLatin1String("issf.10m.air-rifle"),
              "...and the target standard");

        // planId must never be a node sessionId: one plan will eventually
        // enclose several node sessions.
        const TargetNodeRecord* n1 = rig.monitor.nodeById(nodeName(1));
        check(n1 && rig.plans.planId() != n1->sessionId,
              "a planId is not a node sessionId");
    }

    std::printf("\n-- lane selection --\n");
    {
        Rig rig;
        rig.buildRange(10, 6);
        rig.plans.createPlan(QStringLiteral("Morning Relay"));
        applyProgramme(rig.plans, kAr60);

        check(rig.plans.selectAllOnlineLanes() == 6,
              "SELECT ALL ONLINE picks up the six answering lanes");
        check(rig.plans.selectedLaneCount() == 6, "...and only those six");
        check(!rig.plans.isLaneSelected(9), "a lane with no device is not swept in");

        // An offline lane MAY be chosen deliberately - a tablet may be
        // switched on minutes before the start. It is never hidden.
        rig.monitor.evaluateLiveness(999999);
        check(rig.plans.selectLane(9, true),
              "an operator may deliberately include a lane that is not answering");
        check(rig.plans.selectedLaneCount() == 7, "...and it joins the plan");
        check(rig.plans.laneReadiness(9) == LaneReadiness::NoDevice,
              "...while being reported as not ready");

        check(rig.plans.selectLane(9, false), "a lane can be removed again");
        check(rig.plans.selectedLaneCount() == 6, "...leaving six");
        check(!rig.plans.selectLane(99, true), "a lane outside the range is refused");
    }

    std::printf("\n-- athlete assignment --\n");
    {
        Rig rig;
        rig.buildRange(10, 6);
        rig.plans.createPlan(QStringLiteral("Morning Relay"));
        applyProgramme(rig.plans, kAr60);
        rig.plans.selectAllOnlineLanes();

        const QString a1 = rig.athletes.addAthlete(QStringLiteral("Arnold Bailie"));
        const QString a2 = rig.athletes.addAthlete(QStringLiteral("Hennie Jacobs"));

        check(rig.plans.assignAthlete(a1, 1), "an athlete is assigned to a lane");
        check(rig.plans.athleteOnLane(1) == a1, "...and the lane records them");
        check(rig.plans.laneNumberForAthlete(a1) == 1, "...in both directions");

        QString rejection;
        QObject::connect(&rig.plans, &MatchPlanService::rejected,
                         [&rejection](const QString& r) { rejection = r; });

        // ONE ATHLETE, ONE LANE, within a plan.
        check(!rig.plans.assignAthlete(a1, 4),
              "the SAME athlete cannot also be put on another lane in the plan");
        check(rejection.contains(QLatin1String("lane 1")),
              "...and the refusal names the lane they are already on", rejection);
        check(rig.plans.athleteOnLane(4).isEmpty(), "...and lane 4 stays empty");
        check(rig.plans.laneNumberForAthlete(a1) == 1, "...and lane 1 keeps them");

        check(rig.plans.assignAthlete(a2, 4), "a different athlete may take lane 4");
        check(rig.plans.assignedAthleteCount() == 2, "two lanes are now assigned");

        // Moving is clearing then assigning - explicit, never implicit.
        check(rig.plans.clearAthlete(1), "a lane can be cleared");
        check(rig.plans.assignAthlete(a1, 5), "...and the athlete moved elsewhere");
        check(rig.plans.laneNumberForAthlete(a1) == 5, "...arriving on the new lane");
        check(rig.plans.athleteOnLane(1).isEmpty(), "...and leaving the old one empty");

        check(!rig.plans.assignAthlete(QStringLiteral("ath-nonexistent"), 2),
              "an athlete who is not on the start list is refused");
        check(!rig.plans.assignAthlete(a2, 9),
              "a lane that is not taking part is refused");

        // Deleting somebody who is on a lane would orphan that lane.
        QString athleteRejection;
        QObject::connect(&rig.athletes, &AthleteRegistry::rejected,
                         [&athleteRejection](const QString& r) { athleteRejection = r; });
        check(!rig.athletes.removeAthlete(a1),
              "an athlete on a plan lane cannot be deleted");
        check(athleteRejection.contains(QLatin1String("Morning Relay")),
              "...and the refusal names the match to clear first", athleteRejection);
        check(rig.plans.clearAthlete(5) && rig.athletes.removeAthlete(a1),
              "...and the deletion succeeds once the lane is cleared");
    }

    std::printf("\n-- readiness --\n");
    {
        Rig rig;
        rig.buildRange(10, 6);
        rig.plans.createPlan(QStringLiteral("Morning Relay"));
        applyProgramme(rig.plans, kAr60);
        rig.plans.selectAllOnlineLanes();
        for (int i = 1; i <= 6; ++i)
            rig.plans.assignAthlete(
                rig.athletes.addAthlete(QStringLiteral("Athlete %1").arg(i)), i);

        QVariantMap r = rig.plans.readiness();
        check(r.value(QStringLiteral("selectedLanes")).toInt() == 6, "six lanes selected");
        check(r.value(QStringLiteral("athletesAssigned")).toInt() == 6, "six athletes assigned");
        check(r.value(QStringLiteral("nodesOnline")).toInt() == 6, "six stations online");
        check(r.value(QStringLiteral("targetsConnected")).toInt() == 6, "six targets connected");
        check(r.value(QStringLiteral("planComplete")).toBool(), "the PLAN is complete");
        check(r.value(QStringLiteral("rangeReady")).toBool(), "the RANGE is ready");
        check(rig.plans.laneReadiness(1) == LaneReadiness::Ready, "lane 1 is ready");

        // The distinction the milestone exists to protect.
        check(r.value(QStringLiteral("targetMatchLoaded")).toBool() == false,
              "TARGET MATCH LOADED is FALSE - RMS cannot load a match onto a station");
        check(!r.value(QStringLiteral("targetMatchLoadedNote")).toString().isEmpty(),
              "...and says why");

        check(rig.plans.markReady(), "a complete plan can be marked ready");
        check(rig.plans.planStatusLabel() == QLatin1String("READY"), "...and is READY");
        check(rig.plans.readiness().value(QStringLiteral("rmsPlanReady")).toBool(),
              "RMS PLAN READY is true - which is not the same claim");

        // ── a station goes offline ──────────────────────────────────────
        rig.monitor.evaluateLiveness(999999);
        r = rig.plans.readiness();
        check(rig.plans.laneReadiness(1) == LaneReadiness::NodeOffline,
              "an offline station makes its lane NOT READY");
        check(!r.value(QStringLiteral("rangeReady")).toBool(),
              "...and the range is not ready");
        check(r.value(QStringLiteral("planComplete")).toBool(),
              "...but the PLAN is still complete - the operator did their part");
        check(r.value(QStringLiteral("issues")).toList().size() == 6,
              "...and every affected lane is listed as an issue");

        // ── a station is reachable but its target is not ────────────────
        rig.monitor.ingestDatagram(
            statusFor(nodeName(1), QStringLiteral("boot-1-a"), 5,
                      ConnectionState::TargetDisconnected), 1000000);
        check(rig.plans.laneReadiness(1) == LaneReadiness::TargetDisconnected,
              "a reachable station with a disconnected target is NOT READY - "
              "and is distinguished from being offline");

        // ── athlete missing ─────────────────────────────────────────────
        rig.monitor.ingestDatagram(statusFor(nodeName(2), QStringLiteral("boot-2-a"), 6),
                                   1000000);
        rig.plans.clearAthlete(2);
        check(rig.plans.laneReadiness(2) == LaneReadiness::NoAthlete,
              "a healthy lane with nobody on it is INCOMPLETE, not broken");
        check(!rig.plans.readiness().value(QStringLiteral("planComplete")).toBool(),
              "...and the plan is no longer complete");
        check(rig.plans.planStatusLabel() == QLatin1String("DRAFT"),
              "...and a change after review returns the plan to DRAFT");

        check(!rig.plans.markReady(),
              "a plan missing an athlete cannot be marked ready");
    }

    std::printf("\n-- planned vs observed --\n");
    {
        Rig rig;
        rig.buildRange(10, 3);
        rig.plans.createPlan(QStringLiteral("Morning Relay"));
        applyProgramme(rig.plans, kAr60);
        rig.plans.selectAllOnlineLanes();

        check(rig.plans.programmeMatchForLane(1) == ProgrammeMatch::Matches,
              "a station reporting the planned programme MATCHES the plan");

        // The station is set to something else. RMS did not put it there and
        // cannot change it: the disagreement is reported, not resolved.
        rig.monitor.ingestDatagram(
            statusFor(nodeName(2), QStringLiteral("boot-2-a"), 9,
                      ConnectionState::TargetConnected,
                      QStringLiteral("techaim.10m.air-pistol.free")), 2000);
        check(rig.plans.programmeMatchForLane(2) == ProgrammeMatch::Mismatch,
              "a station reporting a DIFFERENT programme is a mismatch");
        check(rig.plans.readiness().value(QStringLiteral("programmeMismatches")).toInt() == 1,
              "...and the review counts it");
        check(rig.plans.programmeId() == QLatin1String("issf.10m.air-rifle.qualification60"),
              "...and the PLAN is unchanged - RMS did not adopt the station's programme");
        const TargetNodeRecord* n2 = rig.monitor.nodeById(nodeName(2));
        check(n2 && n2->programmeId == QLatin1String("techaim.10m.air-pistol.free"),
              "...and the STATION is unchanged - RMS sent it nothing");

        rig.monitor.evaluateLiveness(999999);
        check(rig.plans.programmeMatchForLane(1) == ProgrammeMatch::Offline,
              "an offline station is OFFLINE, not a mismatch");
    }

    std::printf("\n-- persistence and RMS restart --\n");
    {
        QTemporaryDir dir;
        const QString rangePath = dir.filePath(QStringLiteral("range.json"));
        const QString athPath   = dir.filePath(QStringLiteral("athletes.json"));
        const QString planPath  = dir.filePath(QStringLiteral("plans.json"));
        const QVector<int> chosen = {1, 2, 4, 5, 8, 9};
        QString planId;
        QVector<QString> athleteIds;

        {
            RangeConfigurationService range;
            RangeMonitor monitor;
            AthleteRegistry athletes;
            MatchPlanService plans(&range, &monitor, &athletes);
            range.setStorePath(rangePath);
            athletes.setStorePath(athPath);
            plans.setStorePath(planPath);
            plans.setClockForTesting([] { return qint64(1700000000000LL); });
            range.load(); athletes.load(); plans.load();

            range.createFixedRange(QStringLiteral("Potchefstroom 50 m"),
                                   QStringLiteral("50 m"), 1, 10);
            for (int i = 1; i <= 10; ++i)
                range.assignNodeToLane(nodeName(i), i);

            planId = plans.createPlan(QStringLiteral("Morning Relay"));
            applyProgramme(plans, kAr60);
            for (int lane : chosen)
                plans.selectLane(lane, true);
            for (int i = 0; i < chosen.size(); ++i) {
                const QString id = athletes.addAthlete(
                    QStringLiteral("Athlete %1").arg(i + 1));
                athleteIds.append(id);
                plans.assignAthlete(id, chosen.at(i));
            }
            plans.markReady();
        }

        // A brand new RMS process.
        RangeConfigurationService range;
        RangeMonitor monitor;
        AthleteRegistry athletes;
        MatchPlanService plans(&range, &monitor, &athletes);
        range.setStorePath(rangePath);
        athletes.setStorePath(athPath);
        plans.setStorePath(planPath);
        range.load(); athletes.load(); plans.load();

        check(plans.hasPlan(), "an RMS restart reopens the plan it was editing");
        check(plans.planId() == planId, "...the same plan");
        check(plans.planName() == QLatin1String("Morning Relay"), "...by name");
        check(plans.planStatusLabel() == QLatin1String("READY"), "...with its status");
        check(plans.programmeId() == QLatin1String("issf.10m.air-rifle.qualification60"),
              "...and the same programme identity");
        check(plans.current().programme.shotCount == 60,
              "...and the whole programme snapshot");
        check(plans.selectedLaneCount() == 6, "the six chosen lanes are restored");
        bool sameLanes = true;
        for (int lane : chosen)
            if (!plans.isLaneSelected(lane))
                sameLanes = false;
        check(sameLanes, "...the SAME six physical lanes");
        check(plans.assignedAthleteCount() == 6, "...with six athletes on them");
        check(athletes.count() == 6, "the start list is restored too");
        bool sameAthletes = true;
        for (int i = 0; i < chosen.size(); ++i)
            if (plans.athleteOnLane(chosen.at(i)) != athleteIds.at(i))
                sameAthletes = false;
        check(sameAthletes, "...each on the lane they were given");

        // ONLINE/OFFLINE IS NEVER PERSISTED. Nothing has been heard yet, so
        // every lane is correctly not ready, however "ready" the plan was.
        const QVariantMap r = plans.readiness();
        check(r.value(QStringLiteral("nodesOnline")).toInt() == 0,
              "readiness is recomputed from live telemetry, not restored");
        check(!r.value(QStringLiteral("rangeReady")).toBool(),
              "...so a freshly started RMS does not claim the range is ready");
        check(r.value(QStringLiteral("planComplete")).toBool(),
              "...while the PLAN is still complete");

        // ── the station on lane 4 restarts ──────────────────────────────
        monitor.ingestDatagram(announceFor(nodeName(4), QStringLiteral("boot-4-b")), 5000);
        monitor.ingestDatagram(statusFor(nodeName(4), QStringLiteral("boot-4-b"), 1), 5000);
        check(plans.isLaneSelected(4), "a node restart leaves lane 4 in the plan");
        check(plans.athleteOnLane(4) == athleteIds.at(2),
              "...with the same athlete on it");
        check(plans.laneReadiness(4) == LaneReadiness::Ready,
              "...and it becomes ready again automatically");
    }

    std::printf("\n-- several plans --\n");
    {
        Rig rig;
        rig.buildRange(10, 6);
        const QString morning = rig.plans.createPlan(QStringLiteral("Morning Relay"));
        applyProgramme(rig.plans, kAr60);
        const QString afternoon = rig.plans.createPlan(QStringLiteral("Afternoon Relay"));
        applyProgramme(rig.plans, kAp20);

        check(rig.plans.planCount() == 2, "two plans are kept");
        check(rig.plans.planId() == afternoon, "the newest is the one being edited");
        check(rig.plans.openPlan(morning), "an earlier plan can be reopened");
        check(rig.plans.programmeId() == QLatin1String("issf.10m.air-rifle.qualification60"),
              "...with its own programme intact");
        check(rig.plans.openPlan(afternoon)
                  && rig.plans.programmeId() == QLatin1String("techaim.10m.air-pistol.match20"),
              "...and each plan keeps its own");

        check(rig.plans.deletePlan(afternoon), "a DRAFT plan can be deleted");
        check(rig.plans.planCount() == 1, "...and is gone");

        rig.plans.openPlan(morning);
        rig.plans.selectLane(1, true);
        rig.plans.assignAthlete(rig.athletes.addAthlete(QStringLiteral("A")), 1);
        rig.plans.markReady();
        check(!rig.plans.deletePlan(morning),
              "a plan the operator marked READY is not deleted outright");
        check(rig.plans.archivePlan(morning), "...it is archived instead");
        check(rig.plans.planSummary(morning).value(QStringLiteral("status")).toString()
                  == QLatin1String("ARCHIVED"), "...and says so");
    }

    std::printf("\n-- the models the UI binds to --\n");
    {
        Rig rig;
        rig.buildRange(10, 6);
        rig.plans.createPlan(QStringLiteral("Morning Relay"));
        applyProgramme(rig.plans, kAr60);
        rig.plans.selectAllOnlineLanes();
        const QString a1 = rig.athletes.addAthlete(QStringLiteral("Arnold Bailie"));
        rig.plans.assignAthlete(a1, 1);

        PlanLaneModel lanes(&rig.range, &rig.monitor, &rig.plans, &rig.athletes);
        check(lanes.rowCountProperty() == 10,
              "the plan lane list shows every PHYSICAL lane, not just the chosen ones");
        check(lanes.data(lanes.index(0, 0), PlanLaneModel::SelectedRole).toBool(),
              "lane 1 is selected");
        check(!lanes.data(lanes.index(8, 0), PlanLaneModel::SelectedRole).toBool(),
              "lane 9 is not");
        check(lanes.data(lanes.index(0, 0), PlanLaneModel::AthleteNameRole).toString()
                  == QLatin1String("Arnold Bailie"),
              "the assigned athlete shows by name");
        check(lanes.data(lanes.index(0, 0), PlanLaneModel::ReadyRole).toBool(),
              "lane 1 reads ready");
        check(lanes.data(lanes.index(1, 0), PlanLaneModel::ReadinessRole).toString()
                  == QLatin1String("NO ATHLETE"),
              "lane 2 says exactly what it is missing");

        AthleteListModel roster(&rig.athletes, &rig.plans);
        check(roster.rowCountProperty() == 1, "the start list model has the athlete");
        check(roster.data(roster.index(0, 0), AthleteListModel::AssignedLaneRole).toInt() == 1,
              "...and knows which lane they are on in this plan");

        // The Live Range keeps PLANNED and OBSERVED side by side.
        LaneListModel live(&rig.range, &rig.monitor);
        live.setPlanContext(&rig.plans, &rig.athletes);
        const QModelIndex l1 = live.index(0, 0);
        check(live.data(l1, LaneListModel::InPlanRole).toBool(),
              "the Live Range knows lane 1 is in the plan");
        check(live.data(l1, LaneListModel::PlannedAthleteRole).toString()
                  == QLatin1String("Arnold Bailie"),
              "...and who is planned for it");
        check(live.data(l1, LaneListModel::ProgrammeMatchRole).toString()
                  == QLatin1String("MATCHES PLAN"),
              "...and that the station agrees");
        check(!live.data(l1, LaneListModel::ProgrammeMismatchRole).toBool(),
              "...with no mismatch flagged");

        rig.monitor.ingestDatagram(
            statusFor(nodeName(1), QStringLiteral("boot-1-a"), 9,
                      ConnectionState::TargetConnected,
                      QStringLiteral("techaim.10m.air-pistol.free")), 3000);
        check(live.data(l1, LaneListModel::ProgrammeMismatchRole).toBool(),
              "a station that disagrees with the plan is flagged on the Live Range");
        check(live.data(l1, LaneListModel::ProgrammeLabelRole).toString()
                  == QStringLiteral("10 m Air Pistol · Free"),
              "...showing what the STATION reports");
        check(live.data(l1, LaneListModel::PlannedProgrammeRole).toString()
                  == QLatin1String("10 m Air Rifle · Qualification 60"),
              "...beside what the PLAN intends - neither overwriting the other");
    }

    std::printf("\n-- plan persistence format --\n");
    {
        Rig rig;
        rig.buildRange(10, 2);
        rig.plans.createPlan(QStringLiteral("Morning Relay"));
        applyProgramme(rig.plans, kAr60);
        rig.plans.selectLane(1, true);

        QFile f(rig.plans.configPath());
        check(f.open(QIODevice::ReadOnly), "the plan file can be read back");
        const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        check(o.value(QStringLiteral("schemaVersion")).toInt() == kMatchPlanSchemaVersion,
              "the plan document is versioned");
        const QJsonObject plan = o.value(QStringLiteral("plans")).toArray()
                                     .first().toObject();
        check(plan.value(QStringLiteral("status")).toString() == QLatin1String("DRAFT"),
              "the status is a stable token, not a translated word");
        check(plan.value(QStringLiteral("programme")).toObject()
                  .value(QStringLiteral("programmeId")).toString()
                  == QLatin1String("issf.10m.air-rifle.qualification60"),
              "the programme is persisted by id");
        check(!plan.contains(QStringLiteral("online"))
                  && !plan.contains(QStringLiteral("ready")),
              "no ONLINE/OFFLINE truth is persisted into the plan");
        check(!rig.plans.configPath().contains(QLatin1String("TechAim/TechAim")),
              "the plan file is NOT in the target application's namespace",
              rig.plans.configPath());
    }

    std::printf("\n-- the six-lane field-test flow --\n");
    {
        // The whole intended workflow, start to finish, with no development
        // hooks: configure, prepare, assign, review, save.
        Rig rig;
        rig.buildRange(10, 6);
        check(rig.range.isConfigured(), "1. the range is already configured");
        check(rig.monitor.nodeCount() == 6, "2. six stations are discovered");

        const QString planId = rig.plans.createPlan(QStringLiteral("Field test"));
        check(!planId.isEmpty(), "3. NEW MATCH creates a plan");
        check(applyProgramme(rig.plans, kAr60), "4. a programme is chosen");
        check(rig.plans.selectAllOnlineLanes() == 6, "5. all six online lanes are selected");

        const char* names[] = { "Arnold Bailie", "Hennie Jacobs", "Freek van Wyk",
                                "S. Nkosi", "M. Keller", "T. Adeyemi" };
        bool assigned = true;
        for (int i = 0; i < 6; ++i) {
            const QString id = rig.athletes.addAthlete(QString::fromLatin1(names[i]));
            if (id.isEmpty() || !rig.plans.assignAthlete(id, i + 1))
                assigned = false;
        }
        check(assigned, "6. six athletes are entered and assigned");

        const QVariantMap r = rig.plans.readiness();
        check(r.value(QStringLiteral("planComplete")).toBool(), "7. the review says complete");
        check(r.value(QStringLiteral("rangeReady")).toBool(), "...and the range is ready");
        check(rig.plans.markReady(), "8. the plan is saved as ready");

        // 9. Nothing was transmitted. Every station is exactly as it was.
        bool stationsUntouched = true;
        for (int i = 1; i <= 6; ++i) {
            const TargetNodeRecord* n = rig.monitor.nodeById(nodeName(i));
            if (!n || n->sessionId != QStringLiteral("sess-%1").arg(nodeName(i)))
                stationsUntouched = false;
        }
        check(stationsUntouched,
              "9. NOTHING was sent - every station's own state is untouched");
        check(!rig.plans.readiness().value(QStringLiteral("targetMatchLoaded")).toBool(),
              "...and RMS does not claim any target loaded the match");
    }
}
