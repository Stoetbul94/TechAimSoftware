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
//   TechAimRMS                     LIVE: observe real nodes on UDP 7755
//   TechAimRMS --live              LIVE, said explicitly
//   TechAimRMS --demo-range        FIELD-TEST DEMONSTRATION: a scripted range
//                                  with no hardware, in its own profile
//   TechAimRMS --simulate          the development simulator scenario
//   TechAimRMS --dump [--seconds N]  headless: print the range as text
//
// LIVE IS THE DEFAULT, deliberately. A shipped executable that started a
// simulator when it was double-clicked would eventually be believed. The three
// modes are mutually exclusive: LIVE binds the observation socket and builds
// no simulator; the other two build a simulator and bind no socket.
//
// Options:
//   --range-config <path>   use this range file instead of the installed one
//   --reset-range           development: forget the configured range at start
//   --reset-demo            with --demo-range: wipe the demo profile first, so
//                           the demonstration restarts from its known state.
//                           It cannot touch the real range - the demo profile
//                           is a separate directory and LIVE never opens it.
//   --seed-demo-plan <stage>  development: drive the REAL planning services to
//                           a named wizard stage so it can be captured
//   --simulate-elimination <lane>:<rank>:<score>
//   --simulate-competition <lane>:<STATUS>:<rank>:<score>
//                           development: inject a terminal COMPETITION state
//                           so the display can be shown handling one. NOT
//                           telemetry - protocol v1 carries no such field, and
//                           every surface labels the value SIMULATED.
//
// Environment:
//   TECHAIM_RMS_TIMESCALE   speed up the simulated range (default 1.0)
//   TECHAIM_RMS_CAPTURE     PNG path; grabs the window once and writes it
//   TECHAIM_RMS_CAPTURE_MS  when to grab, ms after start (default 20000)
//   TECHAIM_RMS_CAPTURE_QUIT=1  exit after the grab
//   TECHAIM_RMS_PAGE        development: open on home|live|setup|displays|newmatch
//   TECHAIM_RMS_STEP        development: New Match wizard step to open on
//   TECHAIM_RMS_LANE        development: Live Range lane to open selected
//   TECHAIM_RMS_SIZE        development: main window size, e.g. 1366x768
//   TECHAIM_RMS_FT_START=1  development: start the field-test log
//   TECHAIM_RMS_FT_EXPORT_MS  development: export the bundle at this time
//   TECHAIM_RMS_DISPLAY_LANE / _MODE / _ROTATE / _FULLSCREEN / _FILTER
//                           development: drive the REAL DisplayController into
//                           a named state for a capture
// ─────────────────────────────────────────────────────────────────────────────

#include "rms/AthleteListModel.h"
#include "rms/FieldTestRecorder.h"
#include "rms/FieldTestService.h"
#include "rms/NetworkDiagnostics.h"
#include "rms/StationCode.h"
#include "rms/CompetitionState.h"
#include "rms/DisplayController.h"
#include "rms/DisplayLaneModel.h"
#include "rms/TargetGeometry.h"
#include "rms/AthleteRegistry.h"
#include "rms/LaneListModel.h"
#include "rms/MatchPlanService.h"
#include "rms/PlanLaneModel.h"
#include "rms/RangeConfigurationService.h"
#include "rms/RangeListModel.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "rms/RmsUdpObserver.h"
#include "rms/UnassignedNodeModel.h"
#include "rms/dev/SimulatedRange.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
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

// The three mutually exclusive event sources. There is no combination of
// flags that runs two of them: LIVE binds the observation socket and never
// constructs a simulator; the other two construct a simulator and never bind
// a socket. The mode is printed at startup and shown in the window header, so
// a screen can never be mistaken for the wrong one.
enum class RunMode {
    Live,          // observe real target nodes on UDP 7755
    DevSimulator,  // the harness scenario, for development
    FieldTestDemo  // the scripted demonstration a human clicks through
};

RunMode runModeFrom(const QStringList& args)
{
    // LIVE is the DEFAULT. A shipped executable that starts a simulator when
    // double-clicked would eventually be believed, so demonstration has to be
    // asked for by name.
    if (args.contains(QStringLiteral("--live")))
        return RunMode::Live;
    if (args.contains(QStringLiteral("--demo-range")))
        return RunMode::FieldTestDemo;
    if (args.contains(QStringLiteral("--simulate")))
        return RunMode::DevSimulator;
    return RunMode::Live;
}

const char* runModeName(RunMode m)
{
    switch (m) {
    case RunMode::Live:          return "LIVE";
    case RunMode::DevSimulator:  return "SIMULATOR";
    case RunMode::FieldTestDemo: return "DEMO";
    }
    return "LIVE";
}

void printModeBanner(RunMode mode)
{
    if (mode == RunMode::Live) {
        std::fprintf(stderr,
            "\n"
            "  ============================================================\n"
            "   TECH AIM RMS - LIVE OBSERVATION\n"
            "   Listening for real target nodes on UDP 7755. RMS receives\n"
            "   only; it cannot start, stop or otherwise control a target.\n"
            "  ============================================================\n\n");
        return;
    }
    std::fprintf(stderr,
        "\n"
        "  ============================================================\n"
        "   TECH AIM RMS - %s RANGE ACTIVE (NOT REAL TARGETS)\n"
        "   Every device, athlete, shot and score on screen is generated\n"
        "   locally. No physical range is connected, no target node is\n"
        "   being contacted, and nothing here is competition data.\n"
        "  ============================================================\n\n",
        mode == RunMode::FieldTestDemo ? "FIELD-TEST DEMONSTRATION" : "DEVELOPMENT SIMULATOR");
}

// Where a demonstration keeps its files. NOT the installed range: a field
// test must never be able to damage the range a user has actually configured,
// so the demo profile is a separate directory inside RMS's own namespace and
// LIVE mode never opens it.
QString demoProfileDir()
{
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(base).filePath(QStringLiteral("field-test-demo"));
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

    // RMS'S OWN NAMESPACE, established BEFORE any path is computed. These are
    // static, and QStandardPaths reads them: setting them after the first
    // writableLocation() call puts RMS's files in a bare AppData folder that
    // belongs to no product.
    QCoreApplication::setOrganizationName(QStringLiteral("Tech Aim"));
    QCoreApplication::setApplicationName(QStringLiteral("Tech Aim RMS"));

    const RunMode mode = runModeFrom(args);
    const bool live = (mode == RunMode::Live);
    QString configPath = optionValue(args, QStringLiteral("--range-config"));

    // A demonstration works in its OWN profile directory unless the caller
    // named one. Whatever range the user has really configured is not opened,
    // not read and not written by a demo run.
    if (mode == RunMode::FieldTestDemo && configPath.isEmpty()) {
        const QString dir = demoProfileDir();
        if (args.contains(QStringLiteral("--reset-demo"))) {
            QDir(dir).removeRecursively();
            std::fprintf(stderr, "RMS: demo profile reset (%s)\n", qPrintable(dir));
        }
        QDir().mkpath(dir);
        configPath = QDir(dir).filePath(QStringLiteral("range.json"));
    }

    if (args.contains(QStringLiteral("--dump"))) {
        int seconds = 45;
        const QString s = optionValue(args, QStringLiteral("--seconds"));
        if (!s.isEmpty())
            seconds = s.toInt();
        return runTextDump(argc, argv, seconds, configPath);
    }

    QGuiApplication app(argc, argv);
#ifdef RMS_VERSION_STR
    app.setApplicationVersion(QStringLiteral(RMS_VERSION_STR));
    std::fprintf(stderr, "RMS: %s (%s)  mode=%s\n",
                 RMS_VERSION_STR, RMS_GIT_SHA, runModeName(mode));
#endif

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

    // ── competition preparation ──────────────────────────────────────────
    // RMS's own start list and match plans. These write RMS data files and
    // nothing else: preparing a match records an intention, and no station is
    // told, asked or configured by any of it.
    AthleteRegistry athletes;
    MatchPlanService plans(&rangeConfig, &monitor, &athletes);
    if (!configPath.isEmpty()) {
        // --range-config also relocates the sibling documents, so a scratch
        // configuration never mixes with the installed one.
        const QString dir = QFileInfo(configPath).absolutePath();
        athletes.setStorePath(QDir(dir).filePath(QStringLiteral("athletes.json")));
        plans.setStorePath(QDir(dir).filePath(QStringLiteral("plans.json")));
    }
    athletes.load();
    plans.load();
    // The Live Range shows the plan's intention BESIDE the observation; it
    // never merges one into the other.
    laneModel.setPlanContext(&plans, &athletes);
    AthleteListModel athleteModel(&athletes, &plans);
    PlanLaneModel planLaneModel(&rangeConfig, &monitor, &plans, &athletes);

    // ── target display ───────────────────────────────────────────────────
    // Presentation state only: which lanes are on screen, which one is large,
    // whether it rotates. All RMS-local — choosing what to look at sends
    // nothing and no station can tell the difference.
    DisplayController display(&rangeConfig, &plans);
    DisplayLaneModel displayLanes(&rangeConfig, &monitor, &plans, &athletes, &display);
    TargetGeometryBridge targetGeometry;

    // ── field-test instrumentation ───────────────────────────────────────
    // Observation of the observation. None of this transmits, and none of it
    // changes what RMS believes: it watches the monitor, remembers the
    // transitions worth remembering, and writes RMS's own disk.
    NetworkDiagnostics network;
    network.setMode(QString::fromLatin1(runModeName(mode)), live);
    FieldTestRecorder fieldLog;
    FieldTestService fieldTest(&monitor, &rangeConfig, &plans, &fieldLog, &network);
    fieldTest.setMode(QString::fromLatin1(runModeName(mode)), live);

    // ── the two mutually exclusive event sources ────────────────────────
    printModeBanner(mode);

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
            // NOT a silent empty range. An operator who cannot tell a dead
            // socket from quiet tablets will spend the morning blaming the
            // tablets.
            network.setListenerState(false, int(kObservationPort), observer.lastError());
            std::fprintf(stderr, "RMS: cannot observe UDP %u - %s\n",
                         unsigned(kObservationPort),
                         qPrintable(observer.lastError()));
        } else {
            network.setListenerState(true, int(kObservationPort), QString());
            std::fprintf(stderr, "RMS: observing UDP %u (receive only)\n",
                         unsigned(kObservationPort));
        }
    } else {
        sim.configure(6, mode == RunMode::FieldTestDemo
                             ? dev::SimulatedRange::Scenario::FieldTestDemo
                             : dev::SimulatedRange::Scenario::Development);
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
        // Same cadence as liveness: the service compares state and records
        // only what actually changed, so this cannot flood the timeline.
        fieldTest.poll();
    });
    tick.start(250);

    // ── the field-test demonstration ─────────────────────────────────────
    //
    // One flag, --demo-range, produces a range a human can click through with
    // no hardware: six lanes, six athletes, a plan, live shot traffic, and the
    // states worth looking at.
    //
    // It drives the SAME services the buttons drive - createFixedRange,
    // assignNodeToLane, createPlan, setProgramme, assignAthlete, markReady -
    // so it creates nothing the UI could not create. It is a script, not an
    // inference: every event below fires at a fixed point on the simulator's
    // own clock. Nothing here reads a score, a rank or a shot count to decide
    // that an athlete is finished or eliminated, because deciding that is the
    // target node's job and never RMS's.
    //
    // Timeline (simulated seconds; TECHAIM_RMS_TIMESCALE scales it):
    //     3   stations appear, the range and the plan are created
    //     3+  every lane is shooting
    //    14   lane 3 loses the network        -> OFFLINE, and stays offline
    //    14   lane 4 loses the network        -> OFFLINE, last known data
    //    34   lane 4 returns                  -> unseen-shot warning on lane 4
    //    40   the script declares lane 5 FINISHED    (SIMULATED)
    //    50   the script declares lane 6 ELIMINATED  (SIMULATED)
    //    93   the 60-shot lanes complete; the range then holds its state
    QTimer demoScript;
    if (mode == RunMode::FieldTestDemo) {
        auto* seeded = new bool(false);
        auto* finishedDone = new bool(false);
        auto* eliminatedDone = new bool(false);
        QObject::connect(&demoScript, &QTimer::timeout,
                         [&sim, &rangeConfig, &unassignedModel, &plans, &athletes,
                          &monitor, seeded, finishedDone, eliminatedDone]() {
            const qint64 vt = sim.virtualNowMs();

            if (!*seeded && unassignedModel.rowCountProperty() >= 6) {
                *seeded = true;
                if (!rangeConfig.isConfigured()) {
                    rangeConfig.createFixedRange(QStringLiteral("Tech Aim Demo Range"),
                                                 QStringLiteral("10 m"), 1, 6);
                    const QStringList discovered = unassignedModel.nodeIds();
                    for (int i = 0; i < discovered.size() && i < 6; ++i)
                        rangeConfig.assignNodeToLane(discovered.at(i), i + 1);
                }
                if (!plans.hasPlan()) {
                    plans.createPlan(QStringLiteral("Demo Relay 1"));
                    plans.setProgramme(QStringLiteral("issf.10m.air-rifle.qualification60"),
                                       QStringLiteral("issf"),
                                       QStringLiteral("issf.10m.air-rifle"),
                                       QStringLiteral("AR10"), 10, 60,
                                       QStringLiteral("OFFICIAL"),
                                       QStringLiteral("10M AIR RIFLE · MATCH-60"));
                    plans.selectAllOnlineLanes();
                    // Invented names. No real athlete, club or federation
                    // record belongs in a demonstration package.
                    const char* names[] = { "A. Bailie", "M. Keller", "S. Nkosi",
                                            "J. Bergmann", "P. Rossouw", "T. Adeyemi" };
                    for (int i = 0; i < 6; ++i)
                        plans.assignAthlete(athletes.addAthlete(QString::fromUtf8(names[i])),
                                            i + 1);
                    plans.markReady();
                }
                std::fprintf(stderr, "RMS: DEMO range and plan seeded\n");
            }

            // The two terminal states. Protocol v1 carries no competition
            // status, so these can only arrive as a development injection -
            // which is exactly why every surface stamps them SIMULATED.
            auto declare = [&](int lane, const char* status, int rank, double score) {
                const QString nodeId = rangeConfig.nodeForLaneNumber(lane);
                if (nodeId.isEmpty())
                    return false;
                CompetitionState st;
                st.status = competitionStatusFromString(QString::fromLatin1(status));
                st.rank = rank;
                st.finalScore = score;
                st.finalScoreReported = true;
                if (st.status == CompetitionStatus::Eliminated) {
                    st.finalsStage = QStringLiteral("STANDING");
                    st.eliminatedAtStage = QStringLiteral("STANDING");
                }
                monitor.injectDevelopmentCompetitionState(nodeId, st);
                // The simulated station stops shooting but keeps answering:
                // an athlete leaving the competition does not break their
                // target, and the lane must not start looking like a fault.
                sim.concludeLane(lane);
                std::fprintf(stderr,
                    "RMS: DEMO script declares lane %d %s - SIMULATED, not telemetry\n",
                    lane, status);
                return true;
            };

            if (!*finishedDone && vt >= 40000)
                *finishedDone = declare(5, "FINISHED", 3, 208.4);
            if (!*eliminatedDone && vt >= 50000)
                *eliminatedDone = declare(6, "ELIMINATED", 8, 402.7);
        });
        demoScript.start(250);
    }

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

    // DEVELOPMENT ONLY. Drives the REAL planning services — the identical
    // invokables the New Match buttons call — so a screenshot can be taken of a
    // populated wizard step without a human clicking through it. It creates
    // nothing the UI could not create, and it is not reachable from the UI.
    const QString demoPlan = optionValue(args, QStringLiteral("--seed-demo-plan"));
    if (!demoPlan.isEmpty()) {
        QTimer::singleShot(3500, &app, [&plans, &athletes, &planLaneModel, demoPlan]() {
            if (plans.hasPlan())
                return;
            plans.createPlan(QStringLiteral("Morning Relay"));
            if (demoPlan == QLatin1String("named"))
                return;

            // The 50 m course, so the simulated 10 m stations disagree with it
            // and the planned-vs-observed warning has something real to report.
            const bool fiftyMetre = (demoPlan == QLatin1String("mismatch"));
            if (fiftyMetre)
                plans.setProgramme(QStringLiteral("issf.50m.rifle.qualification60"),
                                   QStringLiteral("issf"), QStringLiteral("issf.50m.rifle"),
                                   QStringLiteral("RIFLE50"), 50, 60,
                                   QStringLiteral("OFFICIAL"),
                                   QStringLiteral("50 Meter RIFLE · MATCH-60"));
            else
                plans.setProgramme(QStringLiteral("issf.10m.air-rifle.qualification60"),
                                   QStringLiteral("issf"), QStringLiteral("issf.10m.air-rifle"),
                                   QStringLiteral("AR10"), 10, 60,
                                   QStringLiteral("OFFICIAL"),
                                   QStringLiteral("10M AIR RIFLE · MATCH-60"));
            if (demoPlan == QLatin1String("programme"))
                return;

            plans.selectAllOnlineLanes();
            if (demoPlan == QLatin1String("lanes"))
                return;

            const char* names[] = { "Arnold Bailie", "Hennie Jacobs", "Freek van Wyk",
                                    "S. Nkosi", "M. Keller", "T. Adeyemi" };
            const int assign = (demoPlan == QLatin1String("athletes")) ? 4 : 6;
            for (int i = 0; i < 6; ++i) {
                const QString id = athletes.addAthlete(QString::fromLatin1(names[i]));
                if (i < assign)
                    plans.assignAthlete(id, i + 1);
            }
            if (demoPlan == QLatin1String("ready") || demoPlan == QLatin1String("mismatch"))
                plans.markReady();
            Q_UNUSED(planLaneModel);
        });
    }

    // DEVELOPMENT ONLY. Injects a terminal COMPETITION state onto a lane so the
    // display can be shown handling one.
    //
    // It is not telemetry and does not pretend to be: protocol v1 carries no
    // competition status, no real station has reported this, and the value is
    // tagged DevelopmentInjection so every surface labels it SIMULATED. RMS
    // still never infers elimination — this is an explicit operator-free
    // override for evidence, and the only other way the field can ever move is
    // a deliberate protocol revision.
    // "<lane>:<rank>:<finalScore>" for elimination, or, for any terminal state,
    // --simulate-competition "<lane>:<STATUS>:<rank>:<finalScore>".
    const QString simEliminate = optionValue(args, QStringLiteral("--simulate-elimination"));
    const QString simTerminal = optionValue(args, QStringLiteral("--simulate-competition"));
    if (!simEliminate.isEmpty() || !simTerminal.isEmpty()) {
        QTimer::singleShot(4500, &app, [&rangeConfig, &monitor, simEliminate, simTerminal]() {
            QStringList parts = (simTerminal.isEmpty() ? simEliminate : simTerminal)
                                    .split(QLatin1Char(':'));
            const int laneNumber = parts.takeFirst().toInt();
            QString statusText = QStringLiteral("ELIMINATED");
            if (!simTerminal.isEmpty() && !parts.isEmpty())
                statusText = parts.takeFirst().toUpper();
            const QString nodeId = rangeConfig.nodeForLaneNumber(laneNumber);
            if (nodeId.isEmpty()) {
                std::fprintf(stderr, "RMS: no device on lane %d to simulate\n", laneNumber);
                return;
            }
            CompetitionState state;
            state.status = competitionStatusFromString(statusText);
            state.rank = parts.value(0).toInt();
            state.finalScore = parts.value(1).toDouble();
            state.finalScoreReported = parts.size() > 1;
            if (state.status == CompetitionStatus::Eliminated) {
                state.finalsStage = QStringLiteral("STANDING");
                state.eliminatedAtStage = QStringLiteral("STANDING");
            }
            monitor.injectDevelopmentCompetitionState(nodeId, state);
            std::fprintf(stderr,
                "RMS: SIMULATED %s injected on lane %d (%s) - not telemetry\n",
                qPrintable(statusText), laneNumber, qPrintable(nodeId));
        });
    }

    // DEVELOPMENT ONLY. Drives the SAME field-test methods the buttons call,
    // so a bundle can be produced without a human clicking. The buttons stay
    // genuinely wired; this replaces the click, not the wiring.
    {
        const bool ftStart = qEnvironmentVariableIntValue("TECHAIM_RMS_FT_START") == 1;
        const int ftExportMs = qEnvironmentVariableIntValue("TECHAIM_RMS_FT_EXPORT_MS");
        if (ftStart) {
            QTimer::singleShot(5000, &app, [&fieldLog, &fieldTest, mode]() {
                fieldLog.start(QStringLiteral("Instrumentation self-test"),
                               QStringLiteral("Development"),
                               QStringLiteral("automated"),
                               QStringLiteral("produced by a development flag"),
                               QString::fromLatin1(runModeName(mode)));
                fieldTest.noteLogStarted();
            });
        }
        if (ftExportMs > 0) {
            QTimer::singleShot(ftExportMs, &app, [&fieldTest, &fieldLog]() {
                const QString dir = fieldTest.exportFieldTest();
                std::fprintf(stderr, "RMS: field-test bundle %s\n",
                             dir.isEmpty() ? "FAILED" : qPrintable(dir));
                fieldLog.stop();
            });
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("RANGECONFIG"), &rangeConfig);
    engine.rootContext()->setContextProperty(QStringLiteral("LANES"), &laneModel);
    engine.rootContext()->setContextProperty(QStringLiteral("UNASSIGNED"), &unassignedModel);
    engine.rootContext()->setContextProperty(QStringLiteral("DEVICES"), &deviceModel);
    engine.rootContext()->setContextProperty(QStringLiteral("ATHLETES"), &athletes);
    engine.rootContext()->setContextProperty(QStringLiteral("ATHLETEMODEL"), &athleteModel);
    engine.rootContext()->setContextProperty(QStringLiteral("PLANS"), &plans);
    engine.rootContext()->setContextProperty(QStringLiteral("PLANLANES"), &planLaneModel);
    engine.rootContext()->setContextProperty(QStringLiteral("DISPLAY"), &display);
    engine.rootContext()->setContextProperty(QStringLiteral("DISPLAYLANES"), &displayLanes);
    engine.rootContext()->setContextProperty(QStringLiteral("TARGETGEO"), &targetGeometry);
    engine.rootContext()->setContextProperty(QStringLiteral("FIELDTEST"), &fieldTest);
    engine.rootContext()->setContextProperty(QStringLiteral("FIELDLOG"), &fieldLog);
    engine.rootContext()->setContextProperty(QStringLiteral("NETDIAG"), &network);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_SIMULATED"), !live);
    // DEVELOPMENT ONLY. Prints the scale and the last shot's millimetres onto
    // the face so a qualification screenshot states its own geometry instead of
    // being measured by eye. Never on in a normal display.
    engine.rootContext()->setContextProperty(
        QStringLiteral("RMS_GEOMETRY_OVERLAY"),
        qEnvironmentVariableIntValue("TECHAIM_RMS_GEOMETRY_OVERLAY") == 1);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_MODE"),
                                             QString::fromLatin1(runModeName(mode)));
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_READ_ONLY"), true);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_PROTOCOL_VERSION"),
                                             kProtocolVersion);
    engine.rootContext()->setContextProperty(QStringLiteral("RMS_OBSERVATION_PORT"),
                                             int(kObservationPort));
    engine.rootContext()->setContextProperty(
        QStringLiteral("RMS_INITIAL_PAGE"),
        qEnvironmentVariable("TECHAIM_RMS_PAGE"));
    engine.rootContext()->setContextProperty(
        QStringLiteral("RMS_INITIAL_STEP"),
        qEnvironmentVariable("TECHAIM_RMS_STEP").toInt());
    engine.rootContext()->setContextProperty(
        QStringLiteral("RMS_INITIAL_LANE"),
        qEnvironmentVariable("TECHAIM_RMS_LANE").toInt());
    // DEVELOPMENT ONLY. Puts the display into a named state by calling the
    // SAME DisplayController methods the buttons call, so a capture can be
    // taken without a human clicking. The controls themselves are genuinely
    // wired; this replaces the click, not the wiring.
    {
        const QString wantLane = qEnvironmentVariable("TECHAIM_RMS_DISPLAY_LANE");
        const QString wantMode = qEnvironmentVariable("TECHAIM_RMS_DISPLAY_MODE");
        const bool wantRotate = qEnvironmentVariableIntValue("TECHAIM_RMS_DISPLAY_ROTATE") == 1;
        const bool wantFull = qEnvironmentVariableIntValue("TECHAIM_RMS_DISPLAY_FULLSCREEN") == 1;
        const QString wantFilter = qEnvironmentVariable("TECHAIM_RMS_DISPLAY_FILTER");
        const int wantNext = qEnvironmentVariableIntValue("TECHAIM_RMS_DISPLAY_NEXT");
        const int wantPrev = qEnvironmentVariableIntValue("TECHAIM_RMS_DISPLAY_PREVIOUS");
        if (!wantLane.isEmpty() || !wantMode.isEmpty() || wantRotate || wantFull
            || !wantFilter.isEmpty() || wantNext > 0 || wantPrev > 0) {
            QTimer::singleShot(4200, &app, [&display, wantLane, wantMode, wantRotate,
                                            wantFull, wantFilter, wantNext, wantPrev]() {
                if (!wantFilter.isEmpty())
                    display.setLaneFilterLabel(wantFilter);
                if (!wantLane.isEmpty())
                    display.selectLane(wantLane.toInt());
                for (int i = 0; i < wantNext; ++i)
                    display.next();
                for (int i = 0; i < wantPrev; ++i)
                    display.previous();
                if (wantMode == QLatin1String("all"))
                    display.showAllTargets();
                if (wantRotate)
                    display.setRotating(true);
                if (wantFull)
                    display.setFullScreen(true);
            });
        }
    }

    engine.load(QUrl(QStringLiteral("qrc:/RmsMain.qml")));

    // DEVELOPMENT ONLY. Sizes the main window so a layout can be evidenced at
    // a named resolution without a human dragging the frame.
    {
        const QString size = qEnvironmentVariable("TECHAIM_RMS_SIZE");
        const QStringList wh = size.split(QLatin1Char('x'));
        if (wh.size() == 2) {
            if (auto* w = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0))) {
                w->setWidth(wh.at(0).toInt());
                w->setHeight(wh.at(1).toInt());
            }
        }
    }
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
            // A full-screen display is its own window; grabbing the main one
            // would capture whatever is behind it.
            if (w) {
                if (auto* fs = w->findChild<QQuickWindow*>(
                        QStringLiteral("rmsFullScreenWindow"))) {
                    if (fs->isVisible())
                        w = fs;
                }
            }
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
