// ─────────────────────────────────────────────────────────────────────────────
// TECH AIM RANGE MANAGEMENT SYSTEM — milestone 1, READ-ONLY OBSERVER.
//
// This binary can watch a range. It cannot touch one. There is no command
// encoder, no transmitting socket and no control surface anywhere in the RMS
// tree; if this application is closed, killed or unplugged, every target node
// carries on with its match exactly as before, because nothing here was ever
// part of the node's control loop.
//
//   TechAimRMS                     scripted simulated range (development)
//   TechAimRMS --live              observe real nodes on UDP 7755
//   TechAimRMS --dump [--seconds N]  headless: print the dashboard as text
//
// Environment:
//   TECHAIM_RMS_TIMESCALE   speed up the simulated range (default 1.0)
//   TECHAIM_RMS_CAPTURE     PNG path; grabs the window once and writes it
//   TECHAIM_RMS_CAPTURE_MS  when to grab, ms after start (default 20000)
//   TECHAIM_RMS_CAPTURE_QUIT=1  exit after the grab
// ─────────────────────────────────────────────────────────────────────────────

#include "rms/RangeListModel.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "rms/RmsUdpObserver.h"
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
        "   The lanes shown are NOT real targets. No physical range is\n"
        "   connected and no target node is being contacted.\n"
        "  ============================================================\n\n");
}

// Headless run: drive the simulated range for N seconds of virtual time and
// print the dashboard the UI would show. This is the harness-friendly way to
// see the whole product without a display.
int runTextDump(int argc, char** argv, int seconds)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    RangeMonitor monitor;
    RangeListModel model(&monitor);
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

    std::printf("TECH AIM RMS - RANGE DASHBOARD (read-only observer)\n");
    std::printf("virtual t = %llds   nodes = %d   accepted = %d   rejected = %d\n\n",
                static_cast<long long>(seconds), model.rowCountProperty(),
                monitor.acceptedDatagrams(), monitor.rejectedDatagrams());
    std::fputs(qPrintable(model.renderTextDashboard()), stdout);
    std::printf("\n");
    for (int i = 0; i < model.rowCountProperty(); ++i) {
        const QVariantMap d = model.nodeDetail(i);
        std::printf("%s  observed=%d  unobserved=%d  duplicates=%d  "
                    "out-of-order=%d  gaps=[%s]  restarts=%d  offline-episodes=%d\n",
                    qPrintable(d.value("laneLabel").toString()),
                    d.value("observedShots").toInt(),
                    d.value("unobserved").toInt(),
                    d.value("duplicatesSuppressed").toInt(),
                    d.value("outOfOrder").toInt(),
                    qPrintable(d.value("gapList").toString()),
                    d.value("nodeRestarts").toInt(),
                    d.value("offlineEpisodes").toInt());
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    QStringList args;
    for (int i = 1; i < argc; ++i)
        args << QString::fromLocal8Bit(argv[i]);

    const bool live = args.contains(QStringLiteral("--live"));

    if (args.contains(QStringLiteral("--dump"))) {
        int seconds = 45;
        const int at = args.indexOf(QStringLiteral("--seconds"));
        if (at >= 0 && at + 1 < args.size())
            seconds = args.at(at + 1).toInt();
        return runTextDump(argc, argv, seconds);
    }

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Tech Aim RMS"));
    app.setOrganizationName(QStringLiteral("Tech Aim"));

    RangeMonitor monitor;
    RangeListModel model(&monitor);

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

    // One timer drives both modes: it advances the simulated range (when
    // simulating) and ages out silent nodes (always).
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

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("RANGE"), &model);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_SIMULATED"), !live);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_READ_ONLY"), true);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_PROTOCOL_VERSION"),
                                             kProtocolVersion);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_OBSERVATION_PORT"),
                                             int(kObservationPort));
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
