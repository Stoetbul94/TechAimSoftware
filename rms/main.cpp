// ─────────────────────────────────────────────────────────────────────────────
// TECH AIM RANGE MANAGEMENT SYSTEM.
//
// RMS observes a range and configures its own view of it. It cannot touch a
// target. There is no command encoder, no transmitting socket and no control
// surface anywhere in the RMS tree; if this application is closed, killed or
// unplugged, every target node carries on with its match exactly as before.
//
// The ONLY thing RMS writes is its own range configuration — lanes, and which
// station stands on which lane. Assigning lane 4 tells RMS where a station is;
// it tells the station nothing.
//
//   TechAimRMS                     scripted simulated range (development)
//   TechAimRMS --live              observe real nodes on UDP 7755
//   TechAimRMS --dump [--seconds N]  headless: print the range as text
//
// Options:
//   --range-config <path>   use this range file instead of the installed one
//   --reset-range           development: forget the configured range at start
//
// Environment:
//   TECHAIM_RMS_TIMESCALE   speed up the simulated range (default 1.0)
//   TECHAIM_RMS_CAPTURE     PNG path; grabs the window once and writes it
//   TECHAIM_RMS_CAPTURE_MS  when to grab, ms after start (default 20000)
//   TECHAIM_RMS_CAPTURE_QUIT=1  exit after the grab
//   TECHAIM_RMS_PAGE        development: open on home|live|setup|displays
// ─────────────────────────────────────────────────────────────────────────────

#include "rms/LaneListModel.h"
#include "rms/RangeConfigurationService.h"
#include "rms/RangeListModel.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "rms/RmsUdpObserver.h"
#include "rms/UnassignedNodeModel.h"
#include "rms/dev/SimulatedRange.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

#include <cstdio>

using namespace ta::rms;

namespace {

double timescale()
{
    bool ok = false;
    const double v = qEnvironmentVariable("TECHAIM_RMS_TIMESCALE").toDouble(&ok);
    return (ok && v > 0.0) ? v : 1.0;
}

void printSimulatorBanner()
{
    std::fprintf(stderr,
        "\n"
        "  ============================================================\n"
        "   TECH AIM RMS - DEVELOPMENT SIMULATOR ACTIVE\n"
        "   The devices shown are NOT real targets. No physical range is\n"
        "   connected and no target node is being contacted.\n"
        "  ============================================================\n\n");
}

QString optionValue(const QStringList& args, const QString& name)
{
    const int at = args.indexOf(name);
    return (at >= 0 && at + 1 < args.size()) ? args.at(at + 1) : QString();
}

// Headless run: drive the simulated range and print what the LIVE RANGE page
// would show. Physical lanes, so an unassigned or silent lane still prints.
int runTextDump(int argc, char** argv, int seconds, const QString& configPath)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    RangeConfigurationService config;
    if (!configPath.isEmpty())
        config.setStorePath(configPath);
    config.load();

    RangeMonitor monitor;
    LaneListModel lanes(&config, &monitor);
    UnassignedNodeModel unassigned(&config, &monitor);
    dev::SimulatedRange sim;
    sim.configure(6);
    QObject::connect(&sim, &dev::SimulatedRange::datagramProduced,
                     [&](const QByteArray& d) {
                         monitor.ingestDatagram(d, sim.virtualNowMs());
                     });

    printSimulatorBanner();
    for (qint64 t = 0; t <= qint64(seconds) * 1000; t += 250) {
        sim.advanceTo(t);
        monitor.evaluateLiveness(t);
    }

    if (!config.isConfigured()) {
        std::printf("No range configured (%s).\n"
                    "Run the application once to create one, or pass --range-config.\n",
                    qPrintable(config.configPath()));
        return 0;
    }

    std::printf("TECH AIM RMS - %s (%s, %d physical lanes)\n",
                qPrintable(config.rangeName()), qPrintable(config.rangeModeLabel()),
                config.laneCount());
    std::printf("virtual t = %llds   online = %d   offline = %d   "
                "unassigned devices = %d\n\n",
                static_cast<long long>(seconds), lanes.onlineCount(),
                lanes.offlineCount(), unassigned.rowCountProperty());
    std::fputs(qPrintable(lanes.renderTextRange()), stdout);
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    QStringList args;
    for (int i = 1; i < argc; ++i)
        args << QString::fromLocal8Bit(argv[i]);

    const bool live = args.contains(QStringLiteral("--live"));
    const QString configPath = optionValue(args, QStringLiteral("--range-config"));

    if (args.contains(QStringLiteral("--dump"))) {
        int seconds = 45;
        const QString s = optionValue(args, QStringLiteral("--seconds"));
        if (!s.isEmpty())
            seconds = s.toInt();
        return runTextDump(argc, argv, seconds, configPath);
    }

    QGuiApplication app(argc, argv);
    // RMS'S OWN NAMESPACE. The range configuration must never land in the
    // target application's AppData: RMS is a separate product and may run on a
    // machine that has no node application installed at all.
    app.setOrganizationName(QStringLiteral("Tech Aim"));
    app.setApplicationName(QStringLiteral("Tech Aim RMS"));

    RangeConfigurationService rangeConfig;
    if (!configPath.isEmpty())
        rangeConfig.setStorePath(configPath);
    if (args.contains(QStringLiteral("--reset-range")))
        rangeConfig.forgetRangeForDevelopment();
    rangeConfig.load();
    std::fprintf(stderr, "RMS: range configuration %s\n",
                 qPrintable(rangeConfig.configPath()));

    RangeMonitor monitor;
    RangeListModel deviceModel(&monitor);                 // node-level diagnostics
    LaneListModel laneModel(&rangeConfig, &monitor);      // the physical range
    UnassignedNodeModel unassignedModel(&rangeConfig, &monitor);

    // ── the two mutually exclusive event sources ────────────────────────
    RmsUdpObserver observer;
    dev::SimulatedRange sim;
    QElapsedTimer wall;
    wall.start();

    if (live) {
        QObject::connect(&observer, &RmsUdpObserver::datagramReceived,
                         [&](const QByteArray& d) {
                             monitor.ingestDatagram(
                                 d, QDateTime::currentMSecsSinceEpoch());
                         });
        if (!observer.listen(kObservationPort)) {
            std::fprintf(stderr, "RMS: cannot observe UDP %u - %s\n",
                         unsigned(kObservationPort),
                         qPrintable(observer.lastError()));
        } else {
            std::fprintf(stderr, "RMS: observing UDP %u (receive only)\n",
                         unsigned(kObservationPort));
        }
    } else {
        printSimulatorBanner();
        sim.configure(6);
        QObject::connect(&sim, &dev::SimulatedRange::datagramProduced,
                         [&](const QByteArray& d) {
                             monitor.ingestDatagram(d, sim.virtualNowMs());
                         });
    }

    QTimer tick;
    const double scale = timescale();
    QObject::connect(&tick, &QTimer::timeout, [&]() {
        if (live) {
            monitor.evaluateLiveness(QDateTime::currentMSecsSinceEpoch());
        } else {
            const qint64 vt = qint64(wall.elapsed() * scale);
            sim.advanceTo(vt);
            monitor.evaluateLiveness(vt);
        }
    });
    tick.start(250);

    // DEVELOPMENT ONLY. Builds a demonstration range through the REAL
    // configuration service — the same calls the Create Range button and the
    // Range Setup assignment controls make — so a screenshot can be taken of
    // a populated range without a human clicking through it. It creates
    // nothing the UI could not create, and it is not reachable from the UI.
    const QString demoLanes = optionValue(args, QStringLiteral("--create-demo-range"));
    if (!demoLanes.isEmpty()) {
        const int assignCount = demoLanes.toInt();
        QTimer::singleShot(3000, &app, [&rangeConfig, &unassignedModel, assignCount]() {
            if (rangeConfig.isConfigured())
                return;
            rangeConfig.createFixedRange(QStringLiteral("Potchefstroom 50 m"),
                                         QStringLiteral("50 m"), 1, 10);
            const QStringList discovered = unassignedModel.nodeIds();
            for (int i = 0; i < discovered.size() && i < assignCount; ++i)
                rangeConfig.assignNodeToLane(discovered.at(i), i + 1);
            std::fprintf(stderr, "RMS: demo range created, %d of %lld devices assigned\n",
                         qMin(assignCount, int(discovered.size())),
                         static_cast<long long>(discovered.size()));
        });
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("RANGECONFIG"), &rangeConfig);
    engine.rootContext()->setContextProperty(QStringLiteral("LANES"), &laneModel);
    engine.rootContext()->setContextProperty(QStringLiteral("UNASSIGNED"), &unassignedModel);
    engine.rootContext()->setContextProperty(QStringLiteral("DEVICES"), &deviceModel);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_SIMULATED"), !live);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_READ_ONLY"), true);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_PROTOCOL_VERSION"),
                                             kProtocolVersion);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_OBSERVATION_PORT"),
                                             int(kObservationPort));
    engine.rootContext()->setContextProperty(
        QStringLiteral("RMS_INITIAL_PAGE"),
        qEnvironmentVariable("TECHAIM_RMS_PAGE"));
    engine.load(QUrl(QStringLiteral("qrc:/RmsMain.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    // Development capture hook: one screenshot of the real window, so visual
    // evidence does not depend on someone being at the machine.
    const QString capture = qEnvironmentVariable("TECHAIM_RMS_CAPTURE");
    if (!capture.isEmpty()) {
        int atMs = qEnvironmentVariable("TECHAIM_RMS_CAPTURE_MS").toInt();
        if (atMs <= 0)
            atMs = 20000;
        QTimer::singleShot(atMs, &app, [&engine, capture]() {
            auto* w = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0));
            if (w && w->grabWindow().save(capture))
                std::fprintf(stderr, "RMS: captured %s\n", qPrintable(capture));
            else
                std::fprintf(stderr, "RMS: capture FAILED\n");
            if (qEnvironmentVariableIntValue("TECHAIM_RMS_CAPTURE_QUIT") == 1)
                QGuiApplication::quit();
        });
    }

    return app.exec();
}
