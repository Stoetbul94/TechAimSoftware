// TARGET DISPLAY — geometry, coordinate mapping, lane traversal, rotation and
// what the display refuses to invent.
//
// The coordinate transform is tested rather than eyeballed: a sign error in it
// would put every shot in the wrong quadrant of every target on the range, and
// would look entirely plausible while doing so.

#include "test_support.h"

#include "rms/AthleteRegistry.h"
#include "rms/DisplayController.h"
#include "rms/DisplayLaneModel.h"
#include "rms/MatchPlanService.h"
#include "rms/RangeConfigurationService.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "rms/TargetGeometry.h"

#include <QMetaProperty>
#include <QTemporaryDir>

#include <cmath>
#include <cstdio>

using namespace ta::rms;

namespace {

QString nodeName(int n)
{
    return QStringLiteral("TA-NODE-%1").arg(n, 3, 10, QLatin1Char('0'));
}

QByteArray announceFor(const QString& nodeId)
{
    NodeAnnounce a;
    a.nodeId = nodeId;
    a.bootId = QStringLiteral("boot-a");
    return encode(a);
}

QByteArray statusFor(const QString& nodeId, quint64 seq, int accepted, double total,
                     const QString& standard = QStringLiteral("issf.10m.air-rifle"),
                     ConnectionState conn = ConnectionState::TargetConnected)
{
    NodeStatus s;
    s.nodeId = nodeId;
    s.bootId = QStringLiteral("boot-a");
    s.sessionId = QStringLiteral("sess-%1").arg(nodeId);
    s.programmeId = QStringLiteral("issf.10m.air-rifle.qualification60");
    s.rulesetId = QStringLiteral("issf");
    s.targetStandardId = standard;
    s.athleteName = QStringLiteral("Observed Name");
    s.connection = conn;
    s.phase = MatchPhase::Match;
    s.shotsAccepted = accepted;
    s.shotsExpected = 60;
    s.totalScore = total;
    s.statusSeq = seq;
    return encode(s);
}

QByteArray shotFor(const QString& nodeId, int seq, double xMm, double yMm, double score)
{
    AcceptedShot sh;
    sh.eventId = QStringLiteral("%1-evt-%2").arg(nodeId).arg(seq);
    sh.nodeId = nodeId;
    sh.bootId = QStringLiteral("boot-a");
    sh.sessionId = QStringLiteral("sess-%1").arg(nodeId);
    sh.shotSequence = seq;
    sh.rawXMm = xMm;
    sh.rawYMm = yMm;
    sh.authoritativeScore = score;
    sh.acquisitionStatus = QStringLiteral("ACCEPTED");
    return encode(sh);
}

struct Rig {
    QTemporaryDir dir;
    RangeConfigurationService range;
    RangeMonitor monitor;
    AthleteRegistry athletes;
    MatchPlanService plans{&range, &monitor, &athletes};
    DisplayController display{&range, &plans};
    DisplayLaneModel model{&range, &monitor, &plans, &athletes, &display};

    explicit Rig(int lanes = 10, int assigned = 6)
    {
        range.setStorePath(dir.filePath(QStringLiteral("range.json")));
        athletes.setStorePath(dir.filePath(QStringLiteral("athletes.json")));
        plans.setStorePath(dir.filePath(QStringLiteral("plans.json")));
        plans.setClockForTesting([] { return qint64(1700000000000LL); });
        range.load(); athletes.load(); plans.load();

        range.createFixedRange(QStringLiteral("Potchefstroom"),
                               QStringLiteral("10 m"), 1, lanes);
        for (int i = 1; i <= assigned; ++i) {
            monitor.ingestDatagram(announceFor(nodeName(i)), 1000);
            monitor.ingestDatagram(statusFor(nodeName(i), 1, 0, 0.0), 1000);
            range.assignNodeToLane(nodeName(i), i);
        }
    }

    void planLanes(const QVector<int>& lanes)
    {
        plans.createPlan(QStringLiteral("Morning Relay"));
        plans.setProgramme(QStringLiteral("issf.10m.air-rifle.qualification60"),
                           QStringLiteral("issf"), QStringLiteral("issf.10m.air-rifle"),
                           QStringLiteral("AR10"), 10, 60, QStringLiteral("OFFICIAL"),
                           QStringLiteral("10M AIR RIFLE MATCH-60"));
        for (int n : lanes)
            plans.selectLane(n, true);
    }
};

} // namespace

void run_target_display_tests()
{
    std::printf("\n-- target geometry --\n");
    {
        // Only the standards CompetitionCatalogue.qml actually names.
        check(TargetGeometry::isSupported(QStringLiteral("issf.10m.air-rifle")),
              "10 m air rifle is a supported target standard");
        check(TargetGeometry::isSupported(QStringLiteral("issf.10m.air-pistol")),
              "10 m air pistol is supported");
        check(TargetGeometry::isSupported(QStringLiteral("issf.50m.rifle")),
              "50 m rifle is supported");
        check(TargetGeometry::isSupported(QStringLiteral("issf.50m.pistol")),
              "50 m pistol is supported");
        check(TargetGeometry::supportedStandards().size() == 4,
              "...and nothing else is claimed");

        // An unknown standard must NOT quietly become a different target.
        const TargetSpec unknown =
            TargetGeometry::specFor(QStringLiteral("issf.300m.something"));
        check(!unknown.supported, "an unrecognised standard is UNSUPPORTED");
        check(unknown.faceRadiusMm() <= 0.0,
              "...and has no face, so nothing can be drawn on the wrong target");
        const TargetSpec none = TargetGeometry::specFor(QString());
        check(!none.supported, "no standard at all is also unsupported");

        // Ring radii mirror the foundation renderer: r(k) = ten + (10-k)*step.
        const TargetSpec ar = TargetGeometry::specFor(QStringLiteral("issf.10m.air-rifle"));
        check(qAbs(ar.ringRadiusMm(10) - 0.25) < 1e-9,
              "10 m air rifle ten ring is 0.25 mm radius");
        check(qAbs(ar.ringRadiusMm(9) - 2.75) < 1e-9, "...the 9 ring one step out");
        check(qAbs(ar.faceRadiusMm() - (0.25 + 6 * 2.5)) < 1e-9,
              "...and the face edge is the 4 ring");

        const TargetSpec ap = TargetGeometry::specFor(QStringLiteral("issf.10m.air-pistol"));
        check(qAbs(ap.ringRadiusMm(10) - 5.75) < 1e-9,
              "10 m air pistol ten ring is 5.75 mm radius");
        const TargetSpec r50 = TargetGeometry::specFor(QStringLiteral("issf.50m.rifle"));
        check(qAbs(r50.ringRadiusMm(10) - 5.2) < 1e-9,
              "50 m rifle ten ring is the 5.2 mm the foundation renders");
        check(ap.faceRadiusMm() > ar.faceRadiusMm(),
              "a pistol face is larger than an air rifle face");
    }

    std::printf("\n-- coordinate mapping --\n");
    {
        const TargetSpec spec = TargetGeometry::specFor(QStringLiteral("issf.10m.air-pistol"));
        const double face = spec.faceRadiusMm();

        // Centre.
        const QPointF c = TargetGeometry::normalise(spec, 0.0, 0.0);
        check(qAbs(c.x()) < 1e-9 && qAbs(c.y()) < 1e-9, "a centre shot normalises to (0,0)");

        // POSITIVE X is to the RIGHT.
        const QPointF right = TargetGeometry::normalise(spec, face / 2.0, 0.0);
        check(right.x() > 0.0 && qAbs(right.x() - 0.5) < 1e-9,
              "positive x is half way to the right edge");
        check(qAbs(right.y()) < 1e-9, "...and does not move vertically");

        // NEGATIVE X is to the LEFT.
        const QPointF left = TargetGeometry::normalise(spec, -face / 2.0, 0.0);
        check(left.x() < 0.0 && qAbs(left.x() + 0.5) < 1e-9, "negative x is to the left");

        // POSITIVE Y is UP in the telemetry, and screens count downwards, so
        // the normalised y must be NEGATIVE. Getting this backwards would
        // mirror every target on the range and look completely convincing.
        const QPointF up = TargetGeometry::normalise(spec, 0.0, face / 2.0);
        check(up.y() < 0.0,
              "positive y (up-range) maps to NEGATIVE screen y - a high shot draws HIGH");
        check(qAbs(up.y() + 0.5) < 1e-9, "...half way to the top edge");
        check(qAbs(up.x()) < 1e-9, "...and does not move horizontally");

        const QPointF down = TargetGeometry::normalise(spec, 0.0, -face / 2.0);
        check(down.y() > 0.0, "negative y draws LOW");

        // Top-left and bottom-right quadrants.
        const QPointF topLeft = TargetGeometry::normalise(spec, -face / 3.0, face / 3.0);
        check(topLeft.x() < 0.0 && topLeft.y() < 0.0,
              "a high-left shot lands top-left on screen");
        const QPointF bottomRight = TargetGeometry::normalise(spec, face / 3.0, -face / 3.0);
        check(bottomRight.x() > 0.0 && bottomRight.y() > 0.0,
              "a low-right shot lands bottom-right on screen");

        // Ring edges.
        const QPointF tenEdge = TargetGeometry::normalise(spec, spec.ringRadiusMm(10), 0.0);
        check(qAbs(tenEdge.x() - spec.ringRadiusMm(10) / face) < 1e-9,
              "the ten-ring edge normalises to its true fraction of the face");
        const QPointF faceEdge = TargetGeometry::normalise(spec, face, 0.0);
        check(qAbs(faceEdge.x() - 1.0) < 1e-9, "the face edge normalises to exactly 1.0");

        // Off the face: flagged, held at the edge, never dropped.
        check(TargetGeometry::isWithinFace(spec, face * 0.99, 0.0), "a shot on the face is on it");
        check(!TargetGeometry::isWithinFace(spec, face * 1.5, 0.0),
              "a shot beyond the face is flagged as off it");
        const QPointF wild = TargetGeometry::normaliseClamped(spec, face * 3.0, 0.0);
        check(qAbs(std::hypot(wild.x(), wild.y()) - 1.0) < 1e-9,
              "a wild shot is HELD AT THE EDGE, not discarded - an operator must see it");
        check(wild.x() > 0.0, "...on the side it actually landed");
        const QPointF inside = TargetGeometry::normaliseClamped(spec, 1.0, 1.0);
        check(qAbs(inside.x() - 1.0 / face) < 1e-9, "a shot inside the face is not moved");

        // An unsupported standard cannot place anything.
        const TargetSpec bad = TargetGeometry::specFor(QStringLiteral("nope"));
        const QPointF nowhere = TargetGeometry::normalise(bad, 5.0, 5.0);
        check(qAbs(nowhere.x()) < 1e-9 && qAbs(nowhere.y()) < 1e-9,
              "an unsupported standard places nothing rather than guessing");
    }

    std::printf("\n-- the same shot at every scale --\n");
    {
        const TargetSpec spec = TargetGeometry::specFor(QStringLiteral("issf.50m.rifle"));
        const QPointF n = TargetGeometry::normalise(spec, 12.0, -7.0);

        // A small card and a full-screen view must place the shot at the same
        // FRACTION of the face; only pixels differ.
        const QPointF small = TargetGeometry::toView(n, 120.0);
        const QPointF large = TargetGeometry::toView(n, 900.0);
        const double smallFrac = (small.x() - 60.0) / 60.0;
        const double largeFrac = (large.x() - 450.0) / 450.0;
        check(qAbs(smallFrac - largeFrac) < 1e-9,
              "the same shot sits at the same fraction of the face at both scales");
        check(qAbs(smallFrac - n.x()) < 1e-9, "...which is its normalised x");

        const QPointF centred = TargetGeometry::toView(QPointF(0, 0), 300.0);
        check(qAbs(centred.x() - 150.0) < 1e-9 && qAbs(centred.y() - 150.0) < 1e-9,
              "a centre shot draws at the centre of the view");
        const QPointF withMargin = TargetGeometry::toView(QPointF(1, 0), 200.0, 10.0);
        check(qAbs(withMargin.x() - 190.0) < 1e-9,
              "a margin insets the face without moving the centre");
    }

    std::printf("\n-- lane traversal --\n");
    {
        Rig rig(10, 6);
        check(rig.display.laneOrderNumbers().size() == 10,
              "with no plan the display walks all ten physical lanes");
        check(rig.display.laneFilterLabel() == QLatin1String("ALL_PHYSICAL"),
              "...under the ALL_PHYSICAL filter");

        rig.display.selectLane(1);
        check(rig.display.selectedLane() == 1, "lane 1 is selected");
        check(rig.display.modeLabel() == QLatin1String("SINGLE_TARGET"),
              "...and the display shows a single target");

        rig.display.next();
        check(rig.display.selectedLane() == 2, "NEXT moves to lane 2");
        rig.display.previous();
        check(rig.display.selectedLane() == 1, "PREVIOUS moves back to lane 1");
        rig.display.previous();
        check(rig.display.selectedLane() == 10,
              "PREVIOUS from the first lane WRAPS to the last");
        rig.display.next();
        check(rig.display.selectedLane() == 1, "NEXT from the last wraps to the first");

        // ── the participating set ───────────────────────────────────────
        rig.planLanes({1, 2, 4, 5, 8, 9});
        rig.display.setLaneFilterLabel(QStringLiteral("PARTICIPATING"));
        const QVector<int> order = rig.display.laneOrderNumbers();
        check(order == QVector<int>({1, 2, 4, 5, 8, 9}),
              "the participating filter walks only the match lanes, in lane order");

        rig.display.selectLane(2);
        rig.display.next();
        check(rig.display.selectedLane() == 4,
              "NEXT SKIPS lane 3 - it is not in the match");
        rig.display.next();
        check(rig.display.selectedLane() == 5, "...then lane 5");
        rig.display.selectLane(9);
        rig.display.next();
        check(rig.display.selectedLane() == 1, "...and wraps from 9 back to 1");
        rig.display.previous();
        check(rig.display.selectedLane() == 9, "...and back again");

        // Selection must survive a filter change that drops the lane.
        rig.display.selectLane(9);
        rig.display.setLaneFilterLabel(QStringLiteral("ALL_PHYSICAL"));
        check(rig.display.selectedLane() == 9,
              "a lane in both sets stays selected across a filter change");
        rig.display.selectLane(3);
        rig.display.setLaneFilterLabel(QStringLiteral("PARTICIPATING"));
        check(rig.display.isLaneInOrder(rig.display.selectedLane()),
              "a lane dropped by the filter is replaced by one in the set");
        check(rig.display.selectedLane() == 1, "...the first participating lane");

        // Going to the overview keeps the selection.
        rig.display.selectLane(5);
        rig.display.showAllTargets();
        check(rig.display.showingAll(), "ALL TARGETS is showing");
        check(rig.display.selectedLane() == 5,
              "...and the lane the operator was watching is still selected");

        // A lane outside the set cannot be selected by mistake.
        rig.display.selectLane(3);
        check(rig.display.selectedLane() == 5, "a lane outside the set is not selected");
    }

    std::printf("\n-- range reconfiguration --\n");
    {
        Rig rig(10, 6);
        rig.display.setLaneFilterLabel(QStringLiteral("ALL_PHYSICAL"));
        rig.display.selectLane(9);

        // The range shrinks under the display.
        rig.range.createFixedRange(QStringLiteral("Small bay"), QStringLiteral("10 m"), 1, 4);
        check(rig.display.laneOrderNumbers().size() == 4,
              "the display follows the range definition");
        check(rig.display.selectedLane() == 1,
              "...and a selection that no longer exists is replaced, not left dangling");
        check(rig.model.rowCountProperty() == 4, "the display model follows too");
    }

    std::printf("\n-- auto rotation --\n");
    {
        Rig rig(10, 6);
        rig.planLanes({1, 2, 4, 5, 8, 9});
        rig.display.setLaneFilterLabel(QStringLiteral("PARTICIPATING"));

        qint64 now = 100000;
        rig.display.setClockForTesting([&now] { return now; });
        rig.display.selectLane(1);
        rig.display.setRotationIntervalMs(10000);
        check(rig.display.rotationIntervalMs() == 10000, "the interval is ten seconds");

        rig.display.setRotating(true);
        check(rig.display.isRotating(), "rotation starts");
        check(rig.display.modeLabel() == QLatin1String("ROTATE_TARGETS"),
              "...and the mode says so");
        check(rig.display.selectedLane() == 1, "...from the lane already selected");

        now += 9000;
        rig.display.tickRotation(now);
        check(rig.display.selectedLane() == 1, "it does not advance early");

        now += 2000;
        rig.display.tickRotation(now);
        check(rig.display.selectedLane() == 2, "it advances when the interval elapses");
        now += 11000; rig.display.tickRotation(now);
        check(rig.display.selectedLane() == 4,
              "...along the SAME ordered set previous/next walks - lane 3 is skipped");
        now += 11000; rig.display.tickRotation(now);
        now += 11000; rig.display.tickRotation(now);
        now += 11000; rig.display.tickRotation(now);
        check(rig.display.selectedLane() == 9, "...through to the last participating lane");
        now += 11000; rig.display.tickRotation(now);
        check(rig.display.selectedLane() == 1, "...and wraps");

        // ── rotation never fights the operator ──────────────────────────
        rig.display.selectLane(5);
        check(!rig.display.isRotating(),
              "choosing a lane STOPS rotation - nothing keeps moving behind the operator");
        check(rig.display.selectedLane() == 5, "...and stays where they put it");
        now += 60000;
        rig.display.tickRotation(now);
        check(rig.display.selectedLane() == 5, "...however long they leave it");

        rig.display.setRotating(true);
        rig.display.next();
        check(!rig.display.isRotating(), "pressing NEXT stops rotation too");

        rig.display.setRotating(true);
        rig.display.showAllTargets();
        check(!rig.display.isRotating(), "returning to ALL TARGETS stops rotation");

        rig.display.setRotating(true);
        rig.display.leaveDisplays();
        check(!rig.display.isRotating(),
              "leaving the display page stops rotation - no hidden timer survives it");
        check(!rig.display.isFullScreen(), "...and full screen is left too");

        // Bounds.
        rig.display.setRotationIntervalMs(50);
        check(rig.display.rotationIntervalMs() >= 2000,
              "an unreadably short interval is clamped");
        rig.display.setRotationIntervalMs(99999999);
        check(rig.display.rotationIntervalMs() <= 600000, "an absurd one is clamped too");

        // Nothing to rotate through.
        Rig empty(10, 0);
        empty.display.setLaneFilterLabel(QStringLiteral("PARTICIPATING"));
        empty.display.setRotating(true);
        check(!empty.display.isRotating(), "rotation refuses to start with no lanes");
        check(!empty.display.emptyReason().isEmpty(),
              "...and the page says why it is empty", empty.display.emptyReason());
    }

    std::printf("\n-- what the display draws --\n");
    {
        Rig rig(10, 6);
        rig.monitor.ingestDatagram(statusFor(nodeName(1), 2, 3, 29.6), 2000);
        rig.monitor.ingestDatagram(shotFor(nodeName(1), 1, 0.0, 0.0, 10.9), 2000);
        rig.monitor.ingestDatagram(shotFor(nodeName(1), 2, 2.0, -1.0, 9.4), 2000);
        rig.monitor.ingestDatagram(shotFor(nodeName(1), 3, -1.5, 3.0, 9.3), 2000);

        const QVariantMap lane = rig.model.laneByNumber(1);
        check(lane.value(QStringLiteral("targetStandardId")).toString()
                  == QLatin1String("issf.10m.air-rifle"),
              "the lane carries the target standard the station reported");
        check(lane.value(QStringLiteral("targetSupported")).toBool(),
              "...and it is one RMS can draw");

        const QVariantList shots = lane.value(QStringLiteral("shots")).toList();
        check(shots.size() == 3, "three observed shots are offered to the renderer");
        const QVariantMap first = shots.first().toMap();
        check(qAbs(first.value(QStringLiteral("x")).toDouble()) < 1e-9
                  && qAbs(first.value(QStringLiteral("y")).toDouble()) < 1e-9,
              "the centre shot is at the centre");
        check(first.value(QStringLiteral("score")).toString() == QLatin1String("10.9"),
              "the shot carries the NODE's score, not one derived from its position");
        const QVariantMap third = shots.at(2).toMap();
        check(third.value(QStringLiteral("y")).toDouble() < 0.0,
              "a high shot (positive y) is drawn high");
        check(third.value(QStringLiteral("x")).toDouble() < 0.0, "...and left");
        check(third.value(QStringLiteral("last")).toBool(),
              "the newest shot is marked as the last one");
        check(!first.value(QStringLiteral("last")).toBool(), "...and older ones are not");

        check(lane.value(QStringLiteral("lastShotScore")).toString() == QLatin1String("9.3"),
              "the last-shot score is the node's");
        check(lane.value(QStringLiteral("nodeTotalLabel")).toString()
                  == QLatin1String("29.6"),
              "the authoritative total is the node's running total");

        // An unsupported standard draws nothing rather than the wrong face.
        rig.monitor.ingestDatagram(
            statusFor(nodeName(2), 2, 1, 10.0, QStringLiteral("issf.300m.standard")), 2000);
        const QVariantMap odd = rig.model.laneByNumber(2);
        check(!odd.value(QStringLiteral("targetSupported")).toBool(),
              "an unrecognised standard is reported as unsupported");
        check(odd.value(QStringLiteral("targetStandardName")).toString()
                  == QLatin1String("issf.300m.standard"),
              "...naming what was actually reported");
    }

    std::printf("\n-- shot history is bounded --\n");
    {
        Rig rig(2, 1);
        for (int i = 1; i <= 60; ++i)
            rig.monitor.ingestDatagram(shotFor(nodeName(1), i, 0.5 * i, 0.0, 9.0), 3000);
        rig.monitor.ingestDatagram(statusFor(nodeName(1), 2, 60, 540.0), 3000);

        const QVariantMap lane = rig.model.laneByNumber(1);
        check(lane.value(QStringLiteral("observedShotCount")).toInt() == 60,
              "RMS holds every observed shot");
        const QVariantList shots = lane.value(QStringLiteral("shots")).toList();
        check(shots.size() == DisplayLaneModel::kVisibleShots,
              "...but the face draws a bounded number of them",
              QString::number(shots.size()));
        check(shots.last().toMap().value(QStringLiteral("sequence")).toInt() == 60,
              "...ending at the most recent shot");
        check(shots.first().toMap().value(QStringLiteral("sequence")).toInt() == 31,
              "...and starting the bound back from it");
        check(shots.last().toMap().value(QStringLiteral("last")).toBool(),
              "the most recent is still flagged as last");
    }

    std::printf("\n-- unseen shots are never invented --\n");
    {
        Rig rig(2, 1);
        // The node accepted 30; RMS received 18 - the rest were lost while it
        // could not hear the station.
        for (int i = 1; i <= 18; ++i)
            rig.monitor.ingestDatagram(shotFor(nodeName(1), i, 1.0, 1.0, 9.5), 4000);
        rig.monitor.ingestDatagram(statusFor(nodeName(1), 2, 30, 283.7), 4000);

        const QVariantMap lane = rig.model.laneByNumber(1);
        check(lane.value(QStringLiteral("shotsAccepted")).toInt() == 30,
              "the node says it accepted 30");
        check(lane.value(QStringLiteral("observedShotCount")).toInt() == 18,
              "RMS observed 18");
        check(lane.value(QStringLiteral("unseenShotCount")).toInt() == 12,
              "the display reports 12 unseen");
        check(lane.value(QStringLiteral("shots")).toList().size() == 18,
              "the face draws ONLY the 18 impacts RMS actually has");
        check(lane.value(QStringLiteral("observedTotalLabel")).toString()
                  == QLatin1String("171.0"),
              "the OBSERVED sum is the sum of what RMS saw");
        check(lane.value(QStringLiteral("nodeTotalLabel")).toString()
                  == QLatin1String("283.7"),
              "...and is kept apart from the node's AUTHORITATIVE total");
        check(lane.value(QStringLiteral("observedTotalLabel")).toString()
                  != lane.value(QStringLiteral("nodeTotalLabel")).toString(),
              "...which differ, so the partial sum can never pass as the result");
    }

    std::printf("\n-- offline, restart and reconnection --\n");
    {
        Rig rig(10, 6);
        rig.monitor.ingestDatagram(shotFor(nodeName(3), 1, 1.0, 1.0, 10.1), 5000);
        rig.monitor.ingestDatagram(statusFor(nodeName(3), 2, 1, 10.1), 5000);
        rig.display.selectLane(3);

        rig.monitor.evaluateLiveness(999999);
        const QVariantMap offline = rig.model.laneByNumber(3);
        check(rig.model.rowCountProperty() == 10,
              "an offline station does not remove its tile from the display");
        check(!offline.value(QStringLiteral("online")).toBool(), "the lane reads offline");
        check(offline.value(QStringLiteral("statusText")).toString()
                  .contains(QLatin1String("last known")),
              "...and says the data on it is stale",
              offline.value(QStringLiteral("statusText")).toString());
        check(offline.value(QStringLiteral("shots")).toList().size() == 1,
              "...while the last known impact stays on the face");
        check(rig.display.selectedLane() == 3, "...and the lane stays selected");

        // Same station, new boot: the same tile, not a second one.
        NodeStatus back;
        back.nodeId = nodeName(3);
        back.bootId = QStringLiteral("boot-b");
        back.connection = ConnectionState::TargetConnected;
        back.targetStandardId = QStringLiteral("issf.10m.air-rifle");
        back.shotsAccepted = 4;
        back.totalScore = 39.9;
        back.statusSeq = 1;
        rig.monitor.ingestDatagram(encode(back), 1000000);
        check(rig.model.rowCountProperty() == 10,
              "a node restart creates no extra display tile");
        check(rig.model.laneByNumber(3).value(QStringLiteral("online")).toBool(),
              "...the same lane is live again");
        check(rig.display.selectedLane() == 3, "...and the selection is unchanged");
    }

    std::printf("\n-- terminal competition states on the display --\n");
    {
        Rig rig(10, 6);
        CompetitionState elim;
        elim.status = CompetitionStatus::Eliminated;
        elim.rank = 8;
        elim.finalScore = 402.7;
        elim.finalScoreReported = true;
        rig.monitor.injectDevelopmentCompetitionState(nodeName(4), elim);

        CompetitionState done;
        done.status = CompetitionStatus::Finished;
        done.rank = 1;
        done.finalScore = 463.2;
        done.finalScoreReported = true;
        rig.monitor.injectDevelopmentCompetitionState(nodeName(5), done);

        const QVariantMap out = rig.model.laneByNumber(4);
        const QVariantMap fin = rig.model.laneByNumber(5);

        check(rig.model.rowCountProperty() == 10,
              "an eliminated athlete's lane REMAINS on the display");
        check(out.value(QStringLiteral("eliminated")).toBool(), "lane 4 reads ELIMINATED");
        check(out.value(QStringLiteral("online")).toBool(),
              "...while its station is still ONLINE");
        check(out.value(QStringLiteral("connection")).toString()
                  == QLatin1String("TARGET_CONNECTED"),
              "...and its target still CONNECTED - separate axes");
        check(out.value(QStringLiteral("finalRankLabel")).toString() == QLatin1String("8th"),
              "...with the final rank");
        check(out.value(QStringLiteral("competitionSimulated")).toBool(),
              "...and marked SIMULATED, because no station reported it");

        // FINISHED and ELIMINATED must not collapse into one label.
        check(fin.value(QStringLiteral("finished")).toBool(), "lane 5 reads FINISHED");
        check(!fin.value(QStringLiteral("eliminated")).toBool(),
              "...which is NOT an elimination");
        check(fin.value(QStringLiteral("competitionStatus")).toString()
                  != out.value(QStringLiteral("competitionStatus")).toString(),
              "...and the two states are labelled differently");
        check(fin.value(QStringLiteral("competitionTerminal")).toBool()
                  && out.value(QStringLiteral("competitionTerminal")).toBool(),
              "...though both are terminal, so neither invites more shooting");

        // Everything else is untouched.
        check(rig.model.laneByNumber(1).value(QStringLiteral("competitionStatus")).toString()
                  == QLatin1String("UNKNOWN"),
              "a station that reported nothing is still UNKNOWN, not eliminated");
        check(!rig.model.laneByNumber(1).value(QStringLiteral("eliminated")).toBool(),
              "...and not eliminated by a neighbour's state");
    }

    std::printf("\n-- planned and observed stay apart on the display --\n");
    {
        Rig rig(10, 6);
        rig.planLanes({1, 2});
        const QString a = rig.athletes.addAthlete(QStringLiteral("Arnold Bailie"));
        rig.plans.assignAthlete(a, 1);

        const QVariantMap lane = rig.model.laneByNumber(1);
        // The station reports "Observed Name"; the plan says Arnold Bailie.
        check(lane.value(QStringLiteral("plannedAthlete")).toString()
                  == QLatin1String("Arnold Bailie"), "the plan's athlete is kept");
        check(lane.value(QStringLiteral("observedAthlete")).toString()
                  == QLatin1String("Observed Name"), "the station's is kept too");
        check(lane.value(QStringLiteral("athlete")).toString()
                  == QLatin1String("Arnold Bailie"),
              "the display title prefers the PLAN when a match is open");
        check(lane.value(QStringLiteral("athleteMismatch")).toBool(),
              "...and the disagreement is flagged rather than hidden");

        // With no plan lane, the station's own text is all there is.
        const QVariantMap unplanned = rig.model.laneByNumber(6);
        check(unplanned.value(QStringLiteral("athlete")).toString()
                  == QLatin1String("Observed Name"),
              "a lane outside the plan shows what the station reports");
        check(!unplanned.value(QStringLiteral("athleteMismatch")).toBool(),
              "...with nothing to disagree with");
    }

    std::printf("\n-- empty states --\n");
    {
        QTemporaryDir dir;
        RangeConfigurationService range;
        RangeMonitor monitor;
        AthleteRegistry athletes;
        MatchPlanService plans(&range, &monitor, &athletes);
        range.setStorePath(dir.filePath(QStringLiteral("range.json")));
        athletes.setStorePath(dir.filePath(QStringLiteral("athletes.json")));
        plans.setStorePath(dir.filePath(QStringLiteral("plans.json")));
        range.load(); athletes.load(); plans.load();
        DisplayController display(&range, &plans);
        DisplayLaneModel model(&range, &monitor, &plans, &athletes, &display);

        check(model.rowCountProperty() == 0, "no range configured means no tiles");
        check(display.emptyReason().contains(QLatin1String("No range")),
              "...and the page says so", display.emptyReason());
        check(display.selectedLane() == -1, "...with nothing selected");

        range.createFixedRange(QStringLiteral("Bay"), QStringLiteral("10 m"), 1, 4);
        check(model.rowCountProperty() == 4,
              "a configured range shows its lanes with no stations at all");
        check(display.emptyReason().isEmpty(), "...and is not an empty state");
        const QVariantMap lane = model.laneByNumber(1);
        check(!lane.value(QStringLiteral("hasDevice")).toBool(), "the lane has no device");
        check(lane.value(QStringLiteral("shots")).toList().isEmpty(),
              "...and nothing is drawn on it");
        // QStringLiteral: the em dash is multi-byte UTF-8 and Latin-1
        // would compare it as two characters.
        check(lane.value(QStringLiteral("nodeTotalLabel")).toString() == QStringLiteral("—"),
              "...and its score is a dash, not a zero");
    }

    std::printf("\n-- the lane order is bindable --\n");
    //
    // A Q_INVOKABLE cannot drive a live QML model: nothing tells the binding
    // to re-read it, so a lane strip bound to the invokable is populated once
    // — with the empty pre-configuration order — and never again. That is
    // exactly the defect this asserts against; the ordered set must be
    // reachable as a NOTIFYING property.
    {
        QTemporaryDir dir;
        RangeConfigurationService range;
        RangeMonitor monitor;
        AthleteRegistry athletes;
        MatchPlanService plans(&range, &monitor, &athletes);
        range.setStorePath(dir.filePath(QStringLiteral("range.json")));
        athletes.setStorePath(dir.filePath(QStringLiteral("athletes.json")));
        plans.setStorePath(dir.filePath(QStringLiteral("plans.json")));
        DisplayController display(&range, &plans);

        const QMetaObject* mo = display.metaObject();
        const int idx = mo->indexOfProperty("laneOrderList");
        check(idx >= 0, "the ordered lane set is exposed as a property");
        if (idx >= 0) {
            const QMetaProperty p = mo->property(idx);
            check(p.hasNotifySignal(),
                  "...and it notifies, so a bound model re-reads it");
            check(!p.isWritable(),
                  "...and it is read-only, because a display never sets the order");

            check(p.read(&display).toList().isEmpty(),
                  "before a range exists the order is empty");
            range.createFixedRange(QStringLiteral("Bay"), QStringLiteral("10 m"), 1, 3);
            check(p.read(&display).toList().size() == 3,
                  "...and configuring the range fills it through the same property");
        }
    }
}
