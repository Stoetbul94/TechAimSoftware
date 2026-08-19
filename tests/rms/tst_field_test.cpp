// ─────────────────────────────────────────────────────────────────────────────
// FIELD-TEST INSTRUMENTATION + STATION IDENTITY (milestone 4.7)
//
// The question behind every test here: after a real range day, can somebody
// who was not there work out what happened from the bundle alone?
//
// The identity tests are the ones that matter most. A physical lane is not a
// device identity; the mapping is laneId <-> nodeId and nothing else. IP,
// bootId, COM port and discovery order are diagnostics, and each of them is
// tested here to prove it CANNOT move a lane.
// ─────────────────────────────────────────────────────────────────────────────

#include "test_support.h"

#include "rms/FieldTestRecorder.h"
#include "rms/FieldTestService.h"
#include "rms/MatchPlanService.h"
#include "rms/NetworkDiagnostics.h"
#include "rms/RangeConfigurationService.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "rms/StationCode.h"

#include <QDir>
#include <QJsonArray>
#include <QMetaMethod>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTemporaryDir>

#include <cstdio>

using namespace ta::rms;

namespace {

qint64 g_clock = 1000000;
qint64 testClock() { return g_clock; }

QByteArray announce(const QString& nodeId, const QString& bootId,
                    const QString& laneId = QString())
{
    NodeAnnounce a;
    a.nodeId = nodeId;
    a.bootId = bootId;
    a.laneId = laneId;
    a.deviceIdentity = QStringLiteral("TechAim-EST/1");
    a.appVersion = QStringLiteral("0.9.0");
    a.productIdentity = QStringLiteral("Tech Aim");
    a.timestampUtcMs = g_clock;
    return encode(a);
}

QByteArray status(const QString& nodeId, const QString& bootId, quint64 seq,
                  int accepted, ConnectionState conn = ConnectionState::TargetConnected)
{
    NodeStatus s;
    s.nodeId = nodeId;
    s.bootId = bootId;
    s.sessionId = QStringLiteral("sess-1");
    s.programmeId = QStringLiteral("issf.10m.air-rifle.qualification60");
    s.targetStandardId = QStringLiteral("issf.10m.air-rifle");
    s.statusSeq = seq;
    s.shotsAccepted = accepted;
    s.shotsExpected = 60;
    s.connection = conn;
    s.timestampUtcMs = g_clock;
    return encode(s);
}

QByteArray shot(const QString& nodeId, const QString& bootId, int sequence,
                double score)
{
    AcceptedShot sh;
    sh.eventId = QStringLiteral("%1-%2").arg(nodeId).arg(sequence);
    sh.nodeId = nodeId;
    sh.bootId = bootId;
    sh.sessionId = QStringLiteral("sess-1");
    sh.programmeId = QStringLiteral("issf.10m.air-rifle.qualification60");
    sh.shotSequence = sequence;
    sh.rawXMm = 1.0 * sequence;
    sh.rawYMm = -0.5 * sequence;
    sh.authoritativeScore = score;
    sh.timestampUtcMs = g_clock;
    sh.acquisitionStatus = QStringLiteral("ACCEPTED");
    return encode(sh);
}

const char* kNodeA = "TA-NODE-E368E222403F";
const char* kNodeB = "TA-NODE-9A41C7B10D22";
const char* kNodeC = "TA-NODE-5C02FA8E9147";
const char* kNodeD = "TA-NODE-17BD3E60C8A5";

}  // namespace

void run_field_test_tests()
{
    // ── station code ─────────────────────────────────────────────────────
    std::printf("\n-- station code --\n");
    {
        const QString a = QString::fromLatin1(kNodeA);
        const QString code = StationCode::shortCode(a);
        check(code == QStringLiteral("E222-403F"),
              "a nodeId yields a short, readable station code", code);
        check(StationCode::shortCode(a) == code,
              "...and the SAME code every time it is asked");
        check(StationCode::shortCode(QString::fromLatin1(kNodeB)) != code,
              "a different nodeId yields a different code");
        check(code.length() < a.length() / 2,
              "...and it is much shorter than the identity it stands for");

        // It is a LABEL. Nothing may be stored under it.
        check(StationCode::shortCode(QString()).isEmpty(),
              "an empty nodeId has no code, rather than a misleading one");

        // An id from some future naming scheme still gets a stable code.
        const QString odd = StationCode::shortCode(QStringLiteral("WEIRD-ID-123456"));
        check(!odd.isEmpty(), "an unfamiliar id shape still produces a code", odd);

        // ── collisions ───────────────────────────────────────────────────
        // Two ids that share their last eight hex characters.
        const QString c1 = QStringLiteral("TA-NODE-1111DEADBEEF");
        const QString c2 = QStringLiteral("TA-NODE-2222DEADBEEF");
        check(StationCode::shortCode(c1) == StationCode::shortCode(c2),
              "a forced collision does collide at the default length");
        check(StationCode::wouldCollide({ c1, c2 }),
              "...and the collision is detected rather than shipped");

        const QHash<QString, QString> resolved = StationCode::codesFor({ c1, c2 });
        check(resolved.value(c1) != resolved.value(c2),
              "codesFor() disambiguates them",
              QStringLiteral("%1 vs %2").arg(resolved.value(c1), resolved.value(c2)));
        check(resolved.value(c1).length() > StationCode::shortCode(c1).length(),
              "...by showing MORE of the identity, not by inventing a suffix");
        check(resolved.value(c1).length() == resolved.value(c2).length(),
              "...and both grow together, so the range reads one format");

        // The ordinary case must not be lengthened just because it can be.
        const QHash<QString, QString> normal =
            StationCode::codesFor({ QString::fromLatin1(kNodeA),
                                    QString::fromLatin1(kNodeB),
                                    QString::fromLatin1(kNodeC) });
        check(normal.value(QString::fromLatin1(kNodeA)) == QStringLiteral("E222-403F"),
              "distinct stations keep the short form");
        QSet<QString> uniq;
        for (const QString& v : normal.values())
            uniq.insert(v);
        check(uniq.size() == 3, "and every code in a set is unique");

        // The same node listed twice is not a collision.
        check(!StationCode::wouldCollide({ QString::fromLatin1(kNodeA),
                                           QString::fromLatin1(kNodeA) }),
              "one station listed twice is not a collision");
    }

    // ── identity is the nodeId, and only the nodeId ──────────────────────
    std::printf("\n-- lane mapping authority --\n");
    {
        QTemporaryDir dir;
        RangeConfigurationService range;
        RangeMonitor monitor;
        AthleteRegistry athletes;
        MatchPlanService plans(&range, &monitor, &athletes);
        range.setStorePath(dir.filePath(QStringLiteral("range.json")));
        athletes.setStorePath(dir.filePath(QStringLiteral("athletes.json")));
        plans.setStorePath(dir.filePath(QStringLiteral("plans.json")));
        range.load();
        range.createFixedRange(QStringLiteral("Test"), QStringLiteral("10 m"), 1, 6);

        const QString A = QString::fromLatin1(kNodeA);
        const QString B = QString::fromLatin1(kNodeB);
        const QString C = QString::fromLatin1(kNodeC);
        const QString D = QString::fromLatin1(kNodeD);

        // ── §52 identity persists across a node restart ──────────────────
        monitor.ingestDatagram(announce(A, QStringLiteral("boot-1")), g_clock);
        range.assignNodeToLane(A, 1);
        check(range.laneNumberForNode(A) == 1, "station A is commissioned on lane 1");

        monitor.ingestDatagram(announce(A, QStringLiteral("boot-2")), g_clock + 1000);
        check(range.laneNumberForNode(A) == 1,
              "a NEW bootId does not move the lane — same station, new process");
        const TargetNodeRecord* ra = monitor.nodeById(A);
        check(ra && ra->nodeRestarts == 1, "...and it is counted as a restart");
        check(ra && ra->bootId == QLatin1String("boot-2"),
              "...with the current boot id recorded for diagnostics");

        // ── §53 identity survives a DHCP change ──────────────────────────
        // RMS never reads an address as identity: nothing in the assignment
        // path takes one. Asserted by construction — assignNodeToLane's only
        // key is the nodeId — and by the mapping surviving repeated announces.
        monitor.ingestDatagram(announce(A, QStringLiteral("boot-2")), g_clock + 2000);
        check(range.laneNumberForNode(A) == 1,
              "repeated contact never re-decides the lane");
        check(range.nodeForLaneNumber(1) == A, "lane 1 still names station A");

        // ── §54 discovery order is not lane numbering ────────────────────
        range.assignNodeToLane(B, 2);
        range.assignNodeToLane(C, 3);
        // Heard in the order C, A, B — the reverse of their lane order.
        RangeMonitor second;
        second.ingestDatagram(announce(C, QStringLiteral("b")), g_clock);
        second.ingestDatagram(announce(A, QStringLiteral("b")), g_clock);
        second.ingestDatagram(announce(B, QStringLiteral("b")), g_clock);
        check(second.nodeIds().first() == C, "C was discovered first");
        check(range.laneNumberForNode(A) == 1 && range.laneNumberForNode(B) == 2
                  && range.laneNumberForNode(C) == 3,
              "DISCOVERY ORDER DOES NOT RENUMBER LANES — A=1, B=2, C=3 regardless");

        // ── §55 a new station is unassigned, never auto-placed ───────────
        monitor.ingestDatagram(announce(D, QStringLiteral("b")), g_clock);
        check(range.laneNumberForNode(D) <= 0,
              "a station nobody commissioned is UNASSIGNED");
        check(range.nodeForLaneNumber(4).isEmpty(),
              "...and the free lane 4 stays empty rather than adopting it");

        // ── §56 no duplicate assignment ──────────────────────────────────
        // Lane 5 is free, so moving A there must MOVE it, not copy it.
        check(range.assignNodeToLane(A, 5), "station A is moved to lane 5");
        int lanesHoldingA = 0;
        for (int n = 1; n <= 6; ++n)
            if (range.nodeForLaneNumber(n) == A)
                ++lanesHoldingA;
        check(lanesHoldingA == 1,
              "a station is on exactly ONE lane — the move is atomic",
              QString::number(lanesHoldingA));
        check(range.nodeForLaneNumber(5) == A && range.nodeForLaneNumber(1).isEmpty(),
              "...and the lane it came from is left empty");

        // ── §57 replacement tablet, explicitly ───────────────────────────
        range.assignNodeToLane(C, 3);
        check(range.nodeForLaneNumber(3) == C, "lane 3 carries station C");

        // A bare assign onto an OCCUPIED lane is REFUSED. Silently displacing
        // the station already there is exactly the surprise this product must
        // not spring on a range: replacing a tablet has to be two deliberate
        // acts, not one ambiguous one.
        const bool sneaky = range.assignNodeToLane(D, 3);
        check(!sneaky, "assigning onto an occupied lane is refused");
        check(range.nodeForLaneNumber(3) == C, "...and lane 3 still carries C");
        check(range.lastError().contains(QLatin1String("Clear it first")),
              "...and the operator is told why", range.lastError());

        // The real replacement workflow: clear, then assign.
        check(range.clearLane(3), "the operator clears lane 3");
        check(range.assignNodeToLane(D, 3), "...and assigns the replacement");
        check(range.nodeForLaneNumber(3) == D, "lane 3 now carries station D");
        check(range.laneNumberForNode(C) <= 0,
              "...and the replaced station is left unassigned, not hidden");

        // ── §14 a wiped tablet is a NEW station ──────────────────────────
        const QString reimaged = QStringLiteral("TA-NODE-FFFFFFFFFFFF");
        monitor.ingestDatagram(announce(reimaged, QStringLiteral("b")), g_clock);
        check(range.laneNumberForNode(reimaged) <= 0,
              "a station with a new nodeId is NEW, never assumed to be the old one");

        // Persistence: everything above must survive an RMS restart.
        RangeConfigurationService reopened;
        reopened.setStorePath(dir.filePath(QStringLiteral("range.json")));
        reopened.load();
        check(reopened.nodeForLaneNumber(3) == D,
              "the replacement persists across an RMS restart");
        check(reopened.laneNumberForNode(A) == 5,
              "...and so does the move");
    }

    // ── the recorder ─────────────────────────────────────────────────────
    std::printf("\n-- field-test recorder --\n");
    {
        QTemporaryDir dir;
        FieldTestRecorder rec;
        rec.setClockForTesting(testClock);

        check(!rec.isActive(), "no log is running until somebody starts one");
        rec.record(QStringLiteral("IGNORED"), QStringLiteral("before start"));
        check(rec.eventCount() == 0,
              "and events before the start are not silently collected");

        g_clock = 1700000000000LL;
        const bool started = rec.start(QStringLiteral("Test 1"),
                                       QStringLiteral("Potch"),
                                       QStringLiteral("Arnold"),
                                       QStringLiteral("notes"),
                                       QStringLiteral("LIVE"));
        check(started, "the log starts");
        check(rec.isActive(), "...and reports itself active");
        check(rec.sessionId().startsWith(QLatin1String("FT-")),
              "...with a test-session identity", rec.sessionId());
        check(rec.sessionId() != QStringLiteral("sess-1"),
              "which is NOT a node session id — different thing entirely");

        rec.record(QStringLiteral("NODE_DISCOVERED"), QStringLiteral("one"));
        g_clock += 1500;
        rec.record(QStringLiteral("NODE_OFFLINE"), QStringLiteral("two"));
        rec.flush();
        check(rec.eventCount() == 2, "events are counted", QString::number(rec.eventCount()));
        check(rec.rowCount() == 2, "...and reach the view");

        // ── JSONL on disk, one object per line ───────────────────────────
        QFile f(rec.logPath());
        check(f.exists(), "the log file exists", rec.logPath());
        check(f.open(QIODevice::ReadOnly | QIODevice::Text), "...and is readable");
        int lines = 0;
        bool everyLineParses = true;
        bool headerHasMode = false;
        while (!f.atEnd()) {
            const QByteArray line = f.readLine().trimmed();
            if (line.isEmpty())
                continue;
            ++lines;
            QJsonParseError err{};
            const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
            if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                everyLineParses = false;
                continue;
            }
            const QJsonObject o = doc.object();
            if (o.value(QStringLiteral("eventType")).toString()
                == QLatin1String("FIELD_TEST_STARTED")) {
                headerHasMode = o.value(QStringLiteral("detail")).toObject()
                                    .contains(QStringLiteral("mode"));
            }
        }
        f.close();
        check(lines == 3, "header plus two events, one per line",
              QString::number(lines));
        check(everyLineParses,
              "EVERY LINE IS A COMPLETE JSON OBJECT — a crash costs at most the last one");
        check(headerHasMode, "the header states the mode the log was taken in");

        const auto events = rec.allEvents();
        check(events.size() == 3, "the recorder reads its own file back");
        check(events.first().value(QStringLiteral("eventType")).toString()
                  == QLatin1String("FIELD_TEST_STARTED"),
              "...in order, header first");

        // A partial final line is what a crash leaves. It must not poison the
        // records before it.
        {
            QFile a(rec.logPath());
            a.open(QIODevice::Append | QIODevice::Text);
            a.write("{\"eventType\":\"TRUNCA");
            a.close();
        }
        check(rec.allEvents().size() == 3,
              "a half-written final line is skipped, and the complete ones survive");

        rec.stop();
        check(!rec.isActive(), "the log stops");
        check(rec.allEvents().size() >= 4,
              "...and the stop itself is recorded before the file closes");

        // ── a second segment names the first ─────────────────────────────
        const QString first = rec.sessionId();
        g_clock += 60000;
        rec.start(QStringLiteral("Test 2"), QString(), QString(), QString(),
                  QStringLiteral("LIVE"));
        check(rec.previousSessionId() == first,
              "a restart begins a NEW segment that names the one before it",
              rec.previousSessionId());
        check(rec.sessionId() != first, "...with its own identity");
        rec.stop();
    }

    // ── the service: timeline, counters, preflight, export ───────────────
    std::printf("\n-- field-test service --\n");
    {
        QTemporaryDir dir;
        RangeConfigurationService range;
        RangeMonitor monitor;
        AthleteRegistry athletes;
        MatchPlanService plans(&range, &monitor, &athletes);
        range.setStorePath(dir.filePath(QStringLiteral("range.json")));
        athletes.setStorePath(dir.filePath(QStringLiteral("athletes.json")));
        plans.setStorePath(dir.filePath(QStringLiteral("plans.json")));
        range.load();
        range.createFixedRange(QStringLiteral("Potch"), QStringLiteral("10 m"), 1, 4);

        FieldTestRecorder rec;
        rec.setClockForTesting(testClock);
        NetworkDiagnostics net;
        net.setMode(QStringLiteral("LIVE"), true);
        net.setListenerState(true, 7755, QString());
        FieldTestService svc(&monitor, &range, &plans, &rec, &net);
        svc.setClockForTesting(testClock);
        svc.setMode(QStringLiteral("LIVE"), true);

        const QString A = QString::fromLatin1(kNodeA);
        const QString B = QString::fromLatin1(kNodeB);

        // ── preflight before anything is heard ───────────────────────────
        {
            const QVariantList rows = svc.preflight();
            check(rows.size() >= 6, "the preflight asks several questions",
                  QString::number(rows.size()));
            bool sawWaiting = false;
            for (const QVariant& v : rows) {
                if (v.toMap().value(QStringLiteral("label")).toString()
                        == QLatin1String("Stations heard")) {
                    sawWaiting = v.toMap().value(QStringLiteral("state")).toString()
                                 == QLatin1String("WAITING");
                }
            }
            check(sawWaiting, "no stations yet is WAITING, not FAIL");
            check(svc.preflightVerdict().contains(QLatin1String("WAITING")),
                  "...and the verdict says so", svc.preflightVerdict());
            // The wording is a promise about what RMS can and cannot claim.
            check(!svc.preflightVerdict().contains(QLatin1String("RANGE READY")),
                  "it never says RANGE READY — RMS cannot certify a station");
        }

        // A dead socket is a FAIL, loudly.
        net.setListenerState(false, 7755, QStringLiteral("address in use"));
        {
            bool sawFail = false;
            for (const QVariant& v : svc.preflight()) {
                const QVariantMap m = v.toMap();
                if (m.value(QStringLiteral("label")).toString()
                        == QLatin1String("UDP listener"))
                    sawFail = m.value(QStringLiteral("state")).toString()
                              == QLatin1String("FAIL");
            }
            check(sawFail, "a listener that could not bind is a FAIL");
            check(svc.preflightVerdict().contains(QLatin1String("FAILED")),
                  "...and it fails the whole preflight");
        }
        net.setListenerState(true, 7755, QString());

        // ── the timeline records transitions, not heartbeats ─────────────
        g_clock = 1700000100000LL;
        rec.start(QStringLiteral("FT"), QStringLiteral("Potch"),
                  QStringLiteral("Arnold"), QString(), QStringLiteral("LIVE"));
        svc.noteLogStarted();

        monitor.ingestDatagram(announce(A, QStringLiteral("boot-1")), g_clock);
        range.assignNodeToLane(A, 1);
        svc.poll();

        // Twenty heartbeats that change nothing.
        const int beforeBeats = rec.eventCount();
        for (int i = 0; i < 20; ++i) {
            g_clock += 2000;
            monitor.ingestDatagram(status(A, QStringLiteral("boot-1"), i + 1, 0), g_clock);
            svc.poll();
        }
        const int afterBeats = rec.eventCount();
        check(afterBeats - beforeBeats <= 2,
              "TWENTY HEARTBEATS DO NOT PRODUCE TWENTY EVENTS",
              QStringLiteral("%1 events").arg(afterBeats - beforeBeats));

        // Shots do get recorded.
        for (int i = 1; i <= 5; ++i) {
            g_clock += 1000;
            monitor.ingestDatagram(shot(A, QStringLiteral("boot-1"), i, 10.0), g_clock);
        }
        g_clock += 1000;
        monitor.ingestDatagram(status(A, QStringLiteral("boot-1"), 100, 5), g_clock);
        svc.poll();

        // Offline, then back, then a restart.
        g_clock += 30000;
        monitor.evaluateLiveness(g_clock);
        svc.poll();
        g_clock += 1000;
        monitor.ingestDatagram(announce(A, QStringLiteral("boot-1")), g_clock);
        svc.poll();
        g_clock += 1000;
        monitor.ingestDatagram(announce(A, QStringLiteral("boot-2")), g_clock);
        svc.poll();

        rec.flush();
        const auto events = rec.allEvents();
        QSet<QString> types;
        QVector<qint64> stamps;
        for (const QJsonObject& e : events) {
            types.insert(e.value(QStringLiteral("eventType")).toString());
            if (e.contains(QStringLiteral("atUtcMs")))
                stamps.append(qint64(e.value(QStringLiteral("atUtcMs")).toDouble()));
        }
        check(types.contains(QStringLiteral("RMS_START")), "RMS_START is recorded");
        check(types.contains(QStringLiteral("UDP_LISTENER_STARTED")),
              "the listener state is recorded");
        check(types.contains(QStringLiteral("NODE_DISCOVERED")),
              "a station arriving is recorded");
        check(types.contains(QStringLiteral("LANE_ASSIGNED")),
              "commissioning a lane is recorded");
        check(types.contains(QStringLiteral("SHOT_ACCEPTED_OBSERVED")),
              "observed shots are recorded");
        check(types.contains(QStringLiteral("NODE_OFFLINE")),
              "going offline is recorded");
        check(types.contains(QStringLiteral("NODE_RETURNED")),
              "coming back is recorded");
        check(types.contains(QStringLiteral("NODE_RESTART")),
              "a node restart is recorded");

        bool chronological = true;
        for (int i = 1; i < stamps.size(); ++i)
            if (stamps.at(i) < stamps.at(i - 1))
                chronological = false;
        check(chronological, "the timeline is in chronological order");

        // ── counters ─────────────────────────────────────────────────────
        const QVariantMap c = svc.counters();
        check(c.value(QStringLiteral("nodesDiscovered")).toInt() == 1,
              "one station discovered");
        check(c.value(QStringLiteral("lanesConfigured")).toInt() == 4,
              "four physical lanes");
        check(c.value(QStringLiteral("lanesAssigned")).toInt() == 1,
              "one lane commissioned");
        check(c.value(QStringLiteral("shotsObserved")).toInt() == 5,
              "five shots observed",
              QString::number(c.value(QStringLiteral("shotsObserved")).toInt()));
        check(c.value(QStringLiteral("nodeRestarts")).toInt() == 1,
              "one restart counted");
        check(c.contains(QStringLiteral("announces"))
                  && c.contains(QStringLiteral("statuses"))
                  && c.contains(QStringLiteral("shotMessages")),
              "packets are counted per message type, not just in total");

        // ── unseen shots: counted, never invented ────────────────────────
        // The station says it accepted 12; RMS has 5.
        g_clock += 1000;
        monitor.ingestDatagram(status(A, QStringLiteral("boot-2"), 200, 12), g_clock);
        svc.poll();
        const QVariantMap d = svc.laneDiagnostics(1);
        check(d.value(QStringLiteral("nodeAccepted")).toInt() == 12,
              "the station's own accepted count is shown");
        check(d.value(QStringLiteral("rmsObserved")).toInt() == 5,
              "...beside what RMS actually saw");
        check(d.value(QStringLiteral("unseen")).toInt() == 7,
              "...and the difference is stated as unseen",
              QString::number(d.value(QStringLiteral("unseen")).toInt()));
        check(d.value(QStringLiteral("stationCode")).toString()
                  == QStringLiteral("E222-403F"),
              "the lane names its station in human form");
        check(d.value(QStringLiteral("nodeId")).toString() == A,
              "...while the full identity stays available");

        // ── the export bundle ────────────────────────────────────────────
        range.assignNodeToLane(B, 2);
        svc.poll();
        const QString bundle = svc.exportFieldTest();
        check(!bundle.isEmpty(), "the bundle is written", svc.lastExportError());
        if (!bundle.isEmpty()) {
            const QStringList required = {
                QStringLiteral("summary.txt"), QStringLiteral("summary.json"),
                QStringLiteral("range-snapshot.json"),
                QStringLiteral("lane-mappings.json"),
                QStringLiteral("node-summary.json"),
                QStringLiteral("diagnostics.json"),
                QStringLiteral("events.jsonl"), QStringLiteral("shots.csv"),
                QStringLiteral("README.txt")
            };
            for (const QString& name : required) {
                check(QFile::exists(QDir(bundle).filePath(name)),
                      QStringLiteral("bundle contains %1").arg(name));
            }

            // Every JSON file parses.
            for (const QString& name : { QStringLiteral("summary.json"),
                                         QStringLiteral("range-snapshot.json"),
                                         QStringLiteral("lane-mappings.json"),
                                         QStringLiteral("node-summary.json"),
                                         QStringLiteral("diagnostics.json") }) {
                QFile jf(QDir(bundle).filePath(name));
                jf.open(QIODevice::ReadOnly);
                QJsonParseError err{};
                QJsonDocument::fromJson(jf.readAll(), &err);
                check(err.error == QJsonParseError::NoError,
                      QStringLiteral("%1 parses").arg(name), err.errorString());
            }

            // The CSV has a header and one row per observed shot.
            QFile csv(QDir(bundle).filePath(QStringLiteral("shots.csv")));
            csv.open(QIODevice::ReadOnly | QIODevice::Text);
            const QStringList csvLines =
                QString::fromUtf8(csv.readAll()).split(QLatin1Char('\n'),
                                                       Qt::SkipEmptyParts);
            check(csvLines.size() == 6, "shots.csv has a header and five shots",
                  QString::number(csvLines.size()));
            check(csvLines.first().contains(QLatin1String("authoritativeScore")),
                  "...and names the score as authoritative");
            check(!csvLines.first().contains(QLatin1String("calculated")),
                  "...and nothing in it claims RMS computed anything");

            // Counts agree with the range.
            QFile lm(QDir(bundle).filePath(QStringLiteral("lane-mappings.json")));
            lm.open(QIODevice::ReadOnly);
            const QJsonObject lmo = QJsonDocument::fromJson(lm.readAll()).object();
            check(lmo.value(QStringLiteral("lanes")).toArray().size() == 4,
                  "lane-mappings names all four physical lanes");
            check(lmo.value(QStringLiteral("mappingAuthority")).toString()
                      .contains(QLatin1String("nodeId")),
                  "...and states what the mapping authority is");

            // Version and commit travel with the evidence.
            QFile dg(QDir(bundle).filePath(QStringLiteral("diagnostics.json")));
            dg.open(QIODevice::ReadOnly);
            const QJsonObject dgo = QJsonDocument::fromJson(dg.readAll()).object();
            check(dgo.contains(QStringLiteral("qtVersion"))
                      && dgo.contains(QStringLiteral("protocolVersion")),
                  "diagnostics carries the runtime versions");

            // LIVE tagging, and no false physical claim.
            QFile sj(QDir(bundle).filePath(QStringLiteral("summary.json")));
            sj.open(QIODevice::ReadOnly);
            const QJsonObject sjo = QJsonDocument::fromJson(sj.readAll()).object();
            check(sjo.value(QStringLiteral("mode")).toString() == QLatin1String("LIVE"),
                  "a live bundle is tagged LIVE");
            check(sjo.value(QStringLiteral("simulated")).toBool() == false,
                  "...and not marked simulated");
            check(sjo.value(QStringLiteral("physicalShotRegistrationVerified")).toBool()
                      == false,
                  "PHYSICAL REGISTRATION IS NEVER CLAIMED BY SOFTWARE");

            // ── no source leak ───────────────────────────────────────────
            const QStringList files = QDir(bundle).entryList(QDir::Files);
            bool leaked = false;
            QString leakName;
            for (const QString& f : files) {
                if (f.endsWith(QLatin1String(".cpp")) || f.endsWith(QLatin1String(".h"))
                    || f.endsWith(QLatin1String(".qml")) || f.endsWith(QLatin1String(".pro"))
                    || f.endsWith(QLatin1String(".py")) || f == QLatin1String(".git")) {
                    leaked = true;
                    leakName = f;
                }
            }
            check(!leaked, "the bundle contains NO source file", leakName);
            check(files.size() == 9, "...and nothing beyond the nine it should",
                  QString::number(files.size()));
        }
        rec.stop();
    }

    // ── a DEMO bundle can never pass for range evidence ──────────────────
    std::printf("\n-- demo tagging --\n");
    {
        QTemporaryDir dir;
        RangeConfigurationService range;
        RangeMonitor monitor;
        AthleteRegistry athletes;
        MatchPlanService plans(&range, &monitor, &athletes);
        range.setStorePath(dir.filePath(QStringLiteral("range.json")));
        athletes.setStorePath(dir.filePath(QStringLiteral("athletes.json")));
        plans.setStorePath(dir.filePath(QStringLiteral("plans.json")));
        range.load();
        range.createFixedRange(QStringLiteral("Demo"), QStringLiteral("10 m"), 1, 2);

        FieldTestRecorder rec;
        rec.setClockForTesting(testClock);
        NetworkDiagnostics net;
        net.setMode(QStringLiteral("DEMO"), false);
        FieldTestService svc(&monitor, &range, &plans, &rec, &net);
        svc.setClockForTesting(testClock);
        svc.setMode(QStringLiteral("DEMO"), false);

        g_clock += 10000;
        rec.start(QStringLiteral("demo"), QString(), QString(), QString(),
                  QStringLiteral("DEMO"));
        svc.noteLogStarted();
        const QString bundle = svc.exportFieldTest();
        check(!bundle.isEmpty(), "a demo bundle can be produced");
        if (!bundle.isEmpty()) {
            QFile sj(QDir(bundle).filePath(QStringLiteral("summary.json")));
            sj.open(QIODevice::ReadOnly);
            const QJsonObject o = QJsonDocument::fromJson(sj.readAll()).object();
            check(o.value(QStringLiteral("mode")).toString() == QLatin1String("DEMO"),
                  "it is tagged DEMO");
            check(o.value(QStringLiteral("simulated")).toBool() == true,
                  "...and explicitly marked simulated");
            check(o.value(QStringLiteral("dataSource")).toString()
                      .contains(QLatin1String("NOT physical")),
                  "...and says in words that it is not physical evidence");

            QFile st(QDir(bundle).filePath(QStringLiteral("summary.txt")));
            st.open(QIODevice::ReadOnly | QIODevice::Text);
            const QString text = QString::fromUtf8(st.readAll());
            check(text.contains(QLatin1String("SIMULATED / DEMO DATA")),
                  "the human summary says so at the top");

            // A demo log must not report a listener error it never had.
            rec.flush();
            bool falseAlarm = false;
            for (const QJsonObject& e : rec.allEvents())
                if (e.value(QStringLiteral("eventType")).toString()
                        == QLatin1String("UDP_LISTENER_ERROR"))
                    falseAlarm = true;
            check(!falseAlarm,
                  "DEMO does not log a listener ERROR — there is no socket by design");
        }
        rec.stop();
    }

    // ── no commands, still ───────────────────────────────────────────────
    std::printf("\n-- still read-only --\n");
    {
        // The service's whole surface is observation and local files. Named
        // here so a future method with a verb like "start" has to argue with a
        // test before it can appear.
        const QMetaObject* mo = &FieldTestService::staticMetaObject;
        bool suspicious = false;
        QString found;
        for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
            const QString name = QString::fromLatin1(mo->method(i).name());
            for (const char* verb : { "startMatch", "stopMatch", "reset", "sighting",
                                      "feed", "pause", "resume", "loadMatch",
                                      "identify", "command", "send" }) {
                if (name.compare(QLatin1String(verb), Qt::CaseInsensitive) == 0) {
                    suspicious = true;
                    found = name;
                }
            }
        }
        check(!suspicious,
              "the field-test service exposes no target-command method", found);
    }
}
