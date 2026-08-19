// RANGE DEFINITION AND LANE CONFIGURATION — milestone 3.
//
// The property under test throughout is the separation the milestone exists
// for: the range is CONFIGURATION and the nodes are OBSERVATION. A ten-lane
// range has ten lanes with two stations on, with ten on, and with none on.

#include "test_support.h"

#include "rms/LaneListModel.h"
#include "rms/RangeConfigurationService.h"
#include "rms/RangeMonitor.h"
#include "rms/RangeStore.h"
#include "rms/RmsProtocol.h"
#include "rms/UnassignedNodeModel.h"

#include <QDir>
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
    a.appVersion = QStringLiteral("0.9.0");
    a.productIdentity = QStringLiteral("Tech Aim");
    return encode(a);
}

QByteArray statusFor(const QString& nodeId, const QString& bootId,
                     quint64 seq, int shots = 0, double total = 0.0)
{
    NodeStatus s;
    s.nodeId = nodeId;
    s.bootId = bootId;
    s.sessionId = QStringLiteral("sess-%1").arg(nodeId);
    s.programmeId = QStringLiteral("issf.10m.air-rifle.qualification60");
    s.rulesetId = QStringLiteral("issf");
    s.targetStandardId = QStringLiteral("issf.10m.air-rifle");
    s.athleteName = QStringLiteral("A. Bailie");
    s.connection = ConnectionState::TargetConnected;
    s.phase = MatchPhase::Match;
    s.shotsAccepted = shots;
    s.shotsExpected = 60;
    s.totalScore = total;
    s.statusSeq = seq;
    return encode(s);
}

// A rig with a scratch config file, so no test touches a real installation.
struct Rig {
    QTemporaryDir dir;
    RangeConfigurationService config;
    RangeMonitor monitor;
    LaneListModel lanes{&config, &monitor};
    UnassignedNodeModel unassigned{&config, &monitor};

    Rig()
    {
        config.setStorePath(dir.filePath(QStringLiteral("range.json")));
        config.load();
    }

    // Bring `count` stations online, starting at node 1.
    void bringOnline(int count, qint64 nowMs = 1000,
                     const QString& bootSuffix = QStringLiteral("a"))
    {
        for (int i = 1; i <= count; ++i) {
            const QString id = nodeName(i);
            const QString boot = QStringLiteral("boot-%1-%2").arg(i).arg(bootSuffix);
            monitor.ingestDatagram(announceFor(id, boot), nowMs);
            monitor.ingestDatagram(statusFor(id, boot, 1, 5, 48.5), nowMs);
        }
    }

    int laneRowOf(int laneNumber) const
    {
        return config.range().indexOfLaneNumber(laneNumber);
    }
    QVariant laneRole(int laneNumber, int role) const
    {
        const int row = laneRowOf(laneNumber);
        return row < 0 ? QVariant()
                       : lanes.data(lanes.index(row, 0), role);
    }
};

} // namespace

void run_range_config_tests()
{
    std::printf("\n-- range definition --\n");
    {
        Rig rig;
        check(!rig.config.isConfigured(),
              "a machine that has never been set up has no range");
        check(rig.lanes.rowCountProperty() == 0, "...and no lanes to show");

        check(rig.config.createFixedRange(QStringLiteral("Potchefstroom 50 m"),
                                          QStringLiteral("50 m"), 1, 10),
              "a fixed range is created");
        check(rig.config.isConfigured(), "...and the range is configured");
        check(rig.config.laneCount() == 10, "...with ten physical lanes");
        check(rig.config.firstLaneNumber() == 1 && rig.config.lastLaneNumber() == 10,
              "...numbered 1 to 10");
        check(rig.config.rangeModeLabel() == QLatin1String("Fixed"),
              "...as a FIXED range");
        check(rig.config.rangeName() == QLatin1String("Potchefstroom 50 m"),
              "...under its own name");
        check(QFile::exists(rig.config.configPath()),
              "...and it was written to disk", rig.config.configPath());

        // Lane numbering that does not start at 1 is a real range layout.
        Rig other;
        other.config.createFixedRange(QStringLiteral("Bay B"), QStringLiteral("10 m"),
                                      11, 20);
        check(other.config.laneCount() == 10 && other.config.firstLaneNumber() == 11,
              "a range may be numbered 11-20");

        check(!other.config.createFixedRange(QString(), QStringLiteral("10 m"), 1, 10),
              "a range without a name is refused");
        check(!other.config.createFixedRange(QStringLiteral("X"),
                                             QStringLiteral("10 m"), 0, 10),
              "lane numbering below 1 is refused");
    }

    std::printf("\n-- a ten-lane range with six stations on --\n");
    {
        Rig rig;
        rig.config.createFixedRange(QStringLiteral("Potchefstroom 50 m"),
                                    QStringLiteral("50 m"), 1, 10);
        rig.bringOnline(6);
        for (int i = 1; i <= 6; ++i)
            rig.config.assignNodeToLane(nodeName(i), i);

        check(rig.lanes.rowCountProperty() == 10,
              "TEN physical lanes are shown with SIX stations online");
        check(rig.lanes.onlineCount() == 6, "six are online");
        check(rig.lanes.offlineCount() == 4, "four are not");
        check(rig.lanes.unassignedLaneCount() == 4, "four lanes have no device");

        check(rig.laneRole(3, LaneListModel::OnlineRole).toBool(),
              "lane 3 is online");
        check(rig.laneRole(10, LaneListModel::OnlineRole).toBool() == false,
              "lane 10 is offline");
        check(rig.laneRole(10, LaneListModel::ConnectionRole).toString()
                  == QLatin1String("NO DEVICE"),
              "a lane with nothing assigned says NO DEVICE, not OFFLINE");
        check(rig.laneRole(10, LaneListModel::LaneLabelRole).toString()
                  == QLatin1String("Lane 10"),
              "the empty lane is still named and still present");
        check(rig.laneRole(1, LaneListModel::ProgrammeLabelRole).toString()
                  == QStringLiteral("10 m Air Rifle · Qualification 60"),
              "an online lane carries the observed programme");
        check(rig.laneRole(1, LaneListModel::ScoreLabelRole).toString()
                  == QLatin1String("48.5"),
              "...and the node's own total, formatted only");
    }

    std::printf("\n-- persistent lane mapping --\n");
    {
        Rig rig;
        rig.config.createFixedRange(QStringLiteral("Club"), QStringLiteral("10 m"), 1, 8);
        rig.bringOnline(2);
        check(rig.config.assignNodeToLane(nodeName(1), 4), "a station is assigned lane 4");
        check(rig.config.laneNumberForNode(nodeName(1)) == 4, "the mapping is recorded");
        check(rig.config.nodeForLaneNumber(4) == nodeName(1), "...in both directions");

        // ── restart: same nodeId, new bootId ────────────────────────────
        rig.monitor.ingestDatagram(announceFor(nodeName(1), QStringLiteral("boot-1-b")),
                                   5000);
        rig.monitor.ingestDatagram(statusFor(nodeName(1), QStringLiteral("boot-1-b"),
                                             1, 9, 88.1), 5100);
        check(rig.config.laneNumberForNode(nodeName(1)) == 4,
              "after a NODE RESTART the station is still on lane 4");
        check(rig.config.laneCount() == 8, "...and no extra lane was created");
        check(rig.monitor.nodeCount() == 2, "...and the device was not duplicated");
        check(rig.laneRole(4, LaneListModel::OnlineRole).toBool(),
              "...and lane 4 is online again with no operator action");
        check(rig.laneRole(4, LaneListModel::ShotsLabelRole).toString()
                  == QLatin1String("9/60"),
              "...showing the node's post-restart state");

        // ── the station goes quiet ──────────────────────────────────────
        rig.monitor.evaluateLiveness(999999);
        check(rig.lanes.rowCountProperty() == 8, "an offline station does not remove a lane");
        check(rig.laneRole(4, LaneListModel::OnlineRole).toBool() == false,
              "lane 4 reads OFFLINE");
        check(rig.laneRole(4, LaneListModel::ConnectionRole).toString()
                  == QLatin1String("OFFLINE"),
              "...as OFFLINE, not NO DEVICE - the assignment is still there");
        check(rig.config.nodeForLaneNumber(4) == nodeName(1),
              "...and the assignment survives the outage");
        check(rig.laneRole(4, LaneListModel::StatusTextRole).toString()
                  == QLatin1String("Device offline"),
              "...and the lane says why");

        // ── and comes back, on a new boot ───────────────────────────────
        rig.monitor.ingestDatagram(statusFor(nodeName(1), QStringLiteral("boot-1-c"),
                                             1, 21, 201.4), 1000000);
        check(rig.laneRole(4, LaneListModel::OnlineRole).toBool(),
              "the returning station is back on LANE 4 automatically");
        check(rig.config.laneNumberForNode(nodeName(1)) == 4,
              "...because the mapping is by nodeId, which did not change");
        check(rig.laneRole(4, LaneListModel::ScoreLabelRole).toString()
                  == QLatin1String("201.4"),
              "...and its authoritative total follows it back");
    }

    std::printf("\n-- unassigned devices --\n");
    {
        Rig rig;
        rig.config.createFixedRange(QStringLiteral("Club"), QStringLiteral("10 m"), 1, 8);
        rig.bringOnline(3);
        check(rig.unassigned.rowCountProperty() == 3,
              "a discovered station with no lane is UNASSIGNED");
        check(rig.lanes.rowCountProperty() == 8,
              "...and discovery did NOT invent a physical lane");
        check(rig.lanes.onlineCount() == 0,
              "...so no lane is online until an operator assigns one");

        check(rig.config.assignNodeToLane(nodeName(2), 7), "a device is assigned lane 7");
        check(rig.unassigned.rowCountProperty() == 2, "...and leaves the unassigned list");
        check(rig.lanes.onlineCount() == 1, "...and lane 7 comes alive");
        check(rig.unassigned.nodeIds().contains(nodeName(2)) == false,
              "an assigned device is not offered for assignment again");
    }

    std::printf("\n-- assignment rules --\n");
    {
        Rig rig;
        rig.config.createFixedRange(QStringLiteral("Club"), QStringLiteral("10 m"), 1, 8);
        rig.bringOnline(3);
        rig.config.assignNodeToLane(nodeName(1), 2);
        rig.config.assignNodeToLane(nodeName(2), 3);

        QString rejection;
        QObject::connect(&rig.config, &RangeConfigurationService::assignmentRejected,
                         [&rejection](const QString& r) { rejection = r; });

        check(!rig.config.assignNodeToLane(nodeName(3), 2),
              "a second device cannot be put on an occupied lane");
        check(!rejection.isEmpty(), "...and the refusal explains itself", rejection);
        check(rig.config.nodeForLaneNumber(2) == nodeName(1),
              "...and the lane keeps the device it had");
        check(!rig.config.isNodeAssigned(nodeName(3)),
              "...and the refused device stays unassigned");

        check(!rig.config.assignNodeToLane(nodeName(3), 99),
              "a lane that is not part of the range is refused");
        check(!rig.config.assignNodeToLane(QString(), 5),
              "an empty device id is refused");

        // ── a move is atomic ────────────────────────────────────────────
        check(rig.config.assignNodeToLane(nodeName(1), 7),
              "a device moves from lane 2 to lane 7");
        check(rig.config.nodeForLaneNumber(7) == nodeName(1), "...arriving on lane 7");
        check(rig.config.nodeForLaneNumber(2).isEmpty(),
              "...and lane 2 is cleared in the SAME change - never on two lanes");
        check(rig.config.laneNumberForNode(nodeName(1)) == 7,
              "...and the reverse mapping moved with it");

        check(rig.config.assignNodeToLane(nodeName(1), 7),
              "re-assigning a device to the lane it is already on succeeds quietly");

        check(rig.config.clearLane(7), "a lane can be cleared");
        check(rig.config.nodeForLaneNumber(7).isEmpty(), "...and is then empty");
        check(!rig.config.isNodeAssigned(nodeName(1)),
              "...returning the device to the unassigned list");
    }

    std::printf("\n-- RMS restart --\n");
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("range.json"));
        QString laneId4;
        {
            RangeConfigurationService config;
            config.setStorePath(path);
            config.load();
            config.createFixedRange(QStringLiteral("Potchefstroom 50 m"),
                                    QStringLiteral("50 m"), 1, 10);
            config.assignNodeToLane(nodeName(1), 4);
            config.assignNodeToLane(nodeName(2), 7);
            laneId4 = config.laneAt(config.range().indexOfLaneNumber(4))
                          .value(QStringLiteral("laneId")).toString();
        }

        // A brand new RMS process, reading only what is on disk.
        RangeConfigurationService restarted;
        restarted.setStorePath(path);
        restarted.load();
        check(restarted.isConfigured(), "an RMS restart finds its range");
        check(restarted.rangeName() == QLatin1String("Potchefstroom 50 m"),
              "...by name");
        check(restarted.laneCount() == 10, "...with all ten lanes");
        check(restarted.nodeForLaneNumber(4) == nodeName(1),
              "...and lane 4 still knows its device");
        check(restarted.nodeForLaneNumber(7) == nodeName(2),
              "...and so does lane 7");
        check(restarted.laneAt(restarted.range().indexOfLaneNumber(4))
                  .value(QStringLiteral("laneId")).toString() == laneId4,
              "...and the lane identity itself survived, not just its number");

        // Nothing observed yet after a restart: every lane offline, none lost.
        RangeMonitor monitor;
        LaneListModel lanes(&restarted, &monitor);
        check(lanes.rowCountProperty() == 10,
              "a freshly started RMS shows the whole range before hearing anything");
        check(lanes.onlineCount() == 0, "...with nothing online yet");
        check(lanes.data(lanes.index(lanes.rowCountProperty() > 3 ? 3 : 0, 0),
                         LaneListModel::ConnectionRole).toString()
                  == QLatin1String("OFFLINE"),
              "...and an assigned-but-unheard lane reads OFFLINE");
    }

    std::printf("\n-- temporary range --\n");
    {
        Rig rig;
        rig.bringOnline(6);
        check(rig.unassigned.rowCountProperty() == 6, "six devices are discovered");

        check(rig.config.createTemporaryRange(QStringLiteral("Training bay"),
                                              QStringLiteral("10 m"),
                                              rig.unassigned.nodeIds(), 1),
              "a temporary range is built from what was discovered");
        check(rig.config.laneCount() == 6, "...as six lanes");
        check(rig.config.rangeModeLabel() == QLatin1String("Temporary"),
              "...marked TEMPORARY, not fixed");
        check(rig.config.assignedLaneCount() == 6, "...with every lane already assigned");
        check(rig.unassigned.rowCountProperty() == 0, "...leaving nothing unassigned");
        check(rig.lanes.onlineCount() == 6, "...and all six lanes live");

        // Still editable afterwards — the discovered order is a starting
        // point, not a claim about where the stations physically stand.
        check(rig.config.clearLane(1), "a temporary mapping can be cleared");
        check(rig.config.assignNodeToLane(rig.config.nodeForLaneNumber(2), 1),
              "...and re-pointed");

        Rig empty;
        check(!empty.config.createTemporaryRange(QStringLiteral("X"),
                                                 QStringLiteral("10 m"),
                                                 QStringList(), 1),
              "a temporary range cannot be built from no devices");
    }

    std::printf("\n-- identity is the nodeId, not the address --\n");
    {
        Rig rig;
        rig.config.createFixedRange(QStringLiteral("Club"), QStringLiteral("10 m"), 1, 6);
        rig.bringOnline(1);
        rig.config.assignNodeToLane(nodeName(1), 5);

        // Everything about the station's addressing changes: new boot, new
        // session, and — as far as RMS is concerned — a new source address,
        // which it never recorded in the first place.
        rig.monitor.ingestDatagram(announceFor(nodeName(1), QStringLiteral("boot-1-z")),
                                   9000);
        rig.monitor.ingestDatagram(statusFor(nodeName(1), QStringLiteral("boot-1-z"),
                                             1, 3, 29.7), 9100);
        check(rig.config.laneNumberForNode(nodeName(1)) == 5,
              "a new boot and a new address leave the lane identity unchanged");
        check(rig.config.laneCount() == 6, "...and the range unchanged");

        const QVariantMap detail =
            rig.lanes.laneDetail(rig.config.range().indexOfLaneNumber(5));
        check(detail.value(QStringLiteral("bootId")).toString()
                  == QLatin1String("boot-1-z"),
              "the diagnostics show the CURRENT boot");
        check(detail.value(QStringLiteral("assignedNodeId")).toString() == nodeName(1),
              "...against the unchanged node identity");
        check(detail.contains(QStringLiteral("duplicatesSuppressed"))
                  && detail.contains(QStringLiteral("gapCount"))
                  && detail.contains(QStringLiteral("offlineEpisodes")),
              "milestone 1's engineering detail is preserved in lane diagnostics");
    }

    std::printf("\n-- persistence format --\n");
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("range.json"));
        RangeConfigurationService config;
        config.setStorePath(path);
        config.load();
        config.createFixedRange(QStringLiteral("Club"), QStringLiteral("10 m"), 1, 4);

        QFile f(path);
        check(f.open(QIODevice::ReadOnly), "the range file can be read back");
        const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        check(o.value(QStringLiteral("schemaVersion")).toInt() == kRangeSchemaVersion,
              "the document is versioned");
        check(o.value(QStringLiteral("mode")).toString()
                  == QLatin1String("FIXED_RANGE"),
              "the mode is a stable token, not a translated word");
        check(o.value(QStringLiteral("lanes")).toArray().size() == 4,
              "every lane is persisted");
        check(!config.configPath().contains(QLatin1String("TechAim/TechAim")),
              "the range file is NOT in the target application's namespace",
              config.configPath());

        // Unknown fields from a future build must not stop an older one.
        QJsonObject extended = o;
        extended[QStringLiteral("aFieldFromAFutureBuild")] = true;
        QFile w(path);
        w.open(QIODevice::WriteOnly | QIODevice::Truncate);
        w.write(QJsonDocument(extended).toJson());
        w.close();
        RangeConfigurationService forward;
        forward.setStorePath(path);
        forward.load();
        check(forward.isConfigured() && forward.laneCount() == 4,
              "unknown fields are ignored - the format is forward compatible");

        // A document from a NEWER RMS is refused, and refusing also blocks
        // writing: otherwise this build would offer first-run setup and the
        // operator would overwrite a configuration it simply could not read.
        QJsonObject future = o;
        future[QStringLiteral("schemaVersion")] = kRangeSchemaVersion + 1;
        QFile w2(path);
        w2.open(QIODevice::WriteOnly | QIODevice::Truncate);
        w2.write(QJsonDocument(future).toJson());
        w2.close();
        RangeConfigurationService newer;
        newer.setStorePath(path);
        newer.load();
        check(!newer.isConfigured(), "a range file from a newer RMS is not loaded");
        check(newer.isConfigLocked(), "...and is reported as locked");
        check(!newer.lastError().isEmpty(), "...with an explanation",
              newer.lastError());
        check(!newer.createFixedRange(QStringLiteral("Replacement"),
                                      QStringLiteral("10 m"), 1, 4),
              "...and CANNOT be overwritten by this build");

        QFile check2(path);
        check2.open(QIODevice::ReadOnly);
        const QJsonObject still = QJsonDocument::fromJson(check2.readAll()).object();
        check2.close();
        check(still.value(QStringLiteral("schemaVersion")).toInt()
                  == kRangeSchemaVersion + 1,
              "...so the newer configuration is still intact on disk");
    }

    std::printf("\n-- the range view, rendered --\n");
    {
        Rig rig;
        rig.config.createFixedRange(QStringLiteral("Potchefstroom 50 m"),
                                    QStringLiteral("50 m"), 1, 10);
        rig.bringOnline(6);
        for (int i = 1; i <= 6; ++i)
            rig.config.assignNodeToLane(nodeName(i), i);

        const QString rendered = rig.lanes.renderTextRange();
        std::printf("\n     [%s - %d physical lanes, %d online]\n",
                    qPrintable(rig.config.rangeName()),
                    rig.config.laneCount(), rig.lanes.onlineCount());
        const QStringList lines = rendered.split(QLatin1Char('\n'));
        for (const QString& l : lines)
            if (!l.isEmpty())
                std::printf("     %s\n", qPrintable(l));
        std::printf("\n");
        std::fflush(stdout);

        check(rendered.contains(QLatin1String("Lane 10")),
              "the rendered range includes the lane with no device");
        check(rendered.contains(QLatin1String("NO DEVICE")),
              "...and says so");
        check(rendered.count(QLatin1String("TARGET_CONNECTED")) == 6,
              "...alongside the six that are live");
    }
}
