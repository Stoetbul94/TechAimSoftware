// ─────────────────────────────────────────────────────────────────────────────
// DEVELOPMENT TOOL — N target nodes on one machine, driving the REAL path.
//
// For each node it builds a real QualificationController (which owns a real
// SessionStore), a real NodeTelemetryService and a real UdpTelemetrySink, then
// calls exactly the invokables ShootingPage.qml calls. A shot therefore has to
// be classified, scored, accepted by the reducer and durably journalled before
// telemetry can see it — the same gate the application enforces.
//
// It exists because the application takes a single-instance lock, so six real
// GUI instances cannot run side by side on one machine. Milestone 2 §17 allows
// exactly this substitution.
//
//   rmsnode --nodes 6 --localhost --shots 20
//
// It is DEVELOPMENT-ONLY: not built by Seta.pro, not shipped, and it writes
// its journals and node identities under a scratch directory it creates.
// ─────────────────────────────────────────────────────────────────────────────

#include "qualification/QualificationController.h"
#include "reliability/storage/StoragePaths.h"
#include "telemetry/NodeIdentity.h"
#include "telemetry/NodeTelemetryService.h"
#include "telemetry/UdpTelemetrySink.h"
#include "rms/RmsProtocol.h"

#include <QCoreApplication>
#include <QDir>
#include <QHostAddress>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>

#include <cstdio>
#include <memory>

using namespace ta::telemetry;

namespace {

struct LanePlan {
    const char* lane;
    const char* athlete;
    const char* disciplineId;      // QualificationController's stable ids
    const char* programmeId;       // CompetitionCatalogue's stable ids
    const char* rulesetId;
    const char* targetStandardId;
    int         officialShots;
};

const LanePlan kPlan[] = {
    { "Lane 1", "A. Bailie",   "AR10",    "issf.10m.air-rifle.qualification60",
      "issf",    "issf.10m.air-rifle",  60 },
    { "Lane 2", "M. Keller",   "AP10",    "issf.10m.air-pistol.qualification60",
      "issf",    "issf.10m.air-pistol", 60 },
    { "Lane 3", "S. Nkosi",    "AR10",    "techaim.10m.air-rifle.match40",
      "techaim", "issf.10m.air-rifle",  40 },
    { "Lane 4", "J. Bergmann", "PRONE50", "issf.50m.rifle.qualification60",
      "issf",    "issf.50m.rifle",      60 },
    { "Lane 5", "P. Rossouw",  "AP10",    "techaim.10m.air-pistol.match20",
      "techaim", "issf.10m.air-pistol", 20 },
    { "Lane 6", "T. Adeyemi",  "AR10",    "techaim.10m.air-rifle.match30",
      "techaim", "issf.10m.air-rifle",  30 }
};

// One simulated station: real controller, real store, real publisher.
struct Node {
    std::unique_ptr<QualificationController> controller;
    std::unique_ptr<UdpTelemetrySink>        sink;
    std::unique_ptr<NodeTelemetryService>    telemetry;
    int  fired = 0;
    int  target = 0;
    bool restarted = false;
};

quint32 g_rand = 20260819u;
double nextScore()
{
    g_rand = 1664525u * g_rand + 1013904223u;
    return 8.0 + (int((g_rand >> 16) % 30u) / 10.0);
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    // The reliability layer resolves its root from the application identity and
    // must be initialised before any journal write — exactly as production
    // main() does. Its own namespace, so a harness run never writes into the
    // real installation's Sessions directory.
    QCoreApplication::setOrganizationName(QStringLiteral("TechAim"));
    QCoreApplication::setApplicationName(QStringLiteral("TechAimRmsNodeHarness"));
    const ta::rel::StorageResult storage = ta::rel::StoragePaths::initialize();
    if (!storage.ok) {
        std::fprintf(stderr, "  storage unavailable: %s\n",
                     qPrintable(storage.technicalDetail));
        return 2;
    }

    const QStringList args = app.arguments();
    int nodeCount = 6;
    int shotsPerNode = 20;
    bool localhost = false;
    for (int i = 1; i < args.size(); ++i) {
        if (args.at(i) == QLatin1String("--nodes") && i + 1 < args.size())
            nodeCount = args.at(i + 1).toInt();
        else if (args.at(i) == QLatin1String("--shots") && i + 1 < args.size())
            shotsPerNode = args.at(i + 1).toInt();
        else if (args.at(i) == QLatin1String("--localhost"))
            localhost = true;
    }
    nodeCount = qBound(1, nodeCount, 6);

    const QString root =
        QDir::temp().filePath(QStringLiteral("techaim-rmsnode"));
    QDir().mkpath(root);

    std::fprintf(stderr,
        "\n  ============================================================\n"
        "   TECH AIM RMS NODE HARNESS - DEVELOPMENT TOOL\n"
        "   %d simulated stations, each running the REAL controller,\n"
        "   the REAL SessionStore and the REAL telemetry publisher.\n"
        "   Node identities under: %s\n"
        "  ============================================================\n\n",
        nodeCount, qPrintable(root));

    QVector<Node*> nodes;
    for (int i = 0; i < nodeCount; ++i) {
        Node* n = new Node;
        n->target = shotsPerNode > 0 ? qMin(shotsPerNode, kPlan[i].officialShots)
                                     : kPlan[i].officialShots;

        n->sink = std::make_unique<UdpTelemetrySink>();
        if (localhost)
            n->sink->setDestination(QHostAddress::LocalHost, ta::rms::kObservationPort);

        const QString ini = QDir(root).filePath(QStringLiteral("node%1.ini").arg(i + 1));
        n->telemetry = std::make_unique<NodeTelemetryService>(
            NodeIdentity::forSettingsFile(ini), n->sink.get());
        n->telemetry->setAppVersion(QStringLiteral("0.9.0-node-harness"));
        n->telemetry->setProductIdentity(QStringLiteral("Tech Aim"));
        n->telemetry->setDeviceIdentity(QStringLiteral("TechAim-EST/%1").arg(4100 + i));
        n->telemetry->setLaneHint(QString::fromLatin1(kPlan[i].lane));
        n->telemetry->setTargetConnected(true);
        n->telemetry->setProgramme(QString::fromLatin1(kPlan[i].programmeId),
                                   QString::fromLatin1(kPlan[i].rulesetId),
                                   QString::fromLatin1(kPlan[i].targetStandardId));

        n->controller = std::make_unique<QualificationController>();
        // Demo running mode: the controller's F10 input-source gate accepts a
        // shot flagged `simulated` only in Demo. This is the application's own
        // production demo path, not a bypass.
        n->controller->setOperatingMode(1);
        n->telemetry->attachStore(n->controller->store());
        n->telemetry->start();

        const bool started = n->controller->startSession(
            QString::fromLatin1(kPlan[i].disciplineId),
            QString::number(kPlan[i].officialShots),
            QString::fromLatin1(kPlan[i].athlete),
            kPlan[i].officialShots,
            45LL * 60 * 1000, 15LL * 60 * 1000, -1,
            QString::fromLatin1(kPlan[i].lane), QStringLiteral("T%1").arg(i + 1));
        if (!started) {
            std::fprintf(stderr, "  node %d: startSession FAILED\n", i + 1);
            delete n;
            continue;
        }
        n->controller->beginPreparation();
        n->controller->beginSighting();

        std::fprintf(stderr, "  node %d  %-8s  %-10s  %s\n", i + 1,
                     qPrintable(n->telemetry->nodeId().right(8)),
                     kPlan[i].lane, kPlan[i].programmeId);
        nodes.append(n);
    }

    // A couple of sighters each, then the official match — the same order the
    // application's own phase transitions take.
    QTimer::singleShot(1500, [&nodes] {
        for (Node* n : nodes) {
            n->controller->submitSighter(1.2, -0.8, 9.4, 900, 45.0, true);
            n->controller->submitSighter(-0.6, 1.1, 10.1, 901, 120.0, true);
            n->controller->beginOfficialMatch();
        }
    });

    // One official shot per node per tick.
    QTimer shotTimer;
    QObject::connect(&shotTimer, &QTimer::timeout, [&nodes] {
        for (int i = 0; i < nodes.size(); ++i) {
            Node* n = nodes.at(i);
            if (n->fired >= n->target)
                continue;
            const double score = nextScore();
            const double x = ((int(score * 10) % 21) - 10) / 2.0;
            const double y = ((int(score * 10) % 19) - 9) / 2.0;
            // EXACTLY the call ShootingPage.qml makes.
            if (n->controller->submitOfficial(x, y, score, 1000 + n->fired, 45.0, true))
                ++n->fired;
        }
    });
    shotTimer.start(1200);

    // Node 5 restarts mid-match: same station, new process. Its identity file
    // is unchanged, so the nodeId is the same and only the bootId moves.
    if (nodes.size() >= 5) {
        QTimer::singleShot(26000, [&nodes, root] {
            Node* n = nodes.at(4);
            if (n->restarted)
                return;
            n->restarted = true;
            const QString ini = QDir(root).filePath(QStringLiteral("node5.ini"));
            const QString before = n->telemetry->nodeId();
            n->telemetry->stop();
            n->telemetry = std::make_unique<NodeTelemetryService>(
                NodeIdentity::forSettingsFile(ini), n->sink.get());
            n->telemetry->setAppVersion(QStringLiteral("0.9.0-node-harness"));
            n->telemetry->setProductIdentity(QStringLiteral("Tech Aim"));
            n->telemetry->setDeviceIdentity(QStringLiteral("TechAim-EST/4104"));
            n->telemetry->setLaneHint(QStringLiteral("Lane 5"));
            n->telemetry->setTargetConnected(true);
            n->telemetry->setProgramme(QStringLiteral("techaim.10m.air-pistol.match20"),
                                       QStringLiteral("techaim"),
                                       QStringLiteral("issf.10m.air-pistol"));
            n->telemetry->attachStore(n->controller->store());
            n->telemetry->start();
            std::fprintf(stderr, "  node 5 RESTARTED: nodeId %s (unchanged), new bootId %s\n",
                         qPrintable(before == n->telemetry->nodeId()
                                        ? QStringLiteral("same") : QStringLiteral("CHANGED")),
                         qPrintable(n->telemetry->bootId()));
        });
    }

    // Node 3 goes quiet, then comes back. Its match keeps running throughout —
    // only the broadcast stops, which is the whole point.
    if (nodes.size() >= 3) {
        QTimer::singleShot(14000, [&nodes] {
            nodes.at(2)->telemetry->stop();
            std::fprintf(stderr, "  node 3 telemetry SILENT (its match continues)\n");
        });
        QTimer::singleShot(34000, [&nodes] {
            nodes.at(2)->telemetry->start();
            std::fprintf(stderr, "  node 3 telemetry RESUMED\n");
        });
    }

    QTimer::singleShot(75000, &app, [&nodes] {
        std::fprintf(stderr, "\n  --- node harness summary ---\n");
        for (int i = 0; i < nodes.size(); ++i) {
            const Node* n = nodes.at(i);
            std::fprintf(stderr,
                "  node %d  accepted=%d  published=%d  announces=%d  status=%d  "
                "dropped=%d  sendFail=%d\n",
                i + 1, n->controller->officialShotCount(),
                n->telemetry->shotsPublished(), n->telemetry->announcesPublished(),
                n->telemetry->statusPublished(), n->telemetry->droppedCount(),
                n->telemetry->sendFailures());
        }
        QCoreApplication::quit();
    });

    return app.exec();
}
