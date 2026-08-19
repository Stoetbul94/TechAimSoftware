#include "rms/FieldTestService.h"

#include "rms/FieldTestRecorder.h"
#include "rms/MatchPlanService.h"
#include "rms/NetworkDiagnostics.h"
#include "rms/RangeConfigurationService.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "rms/StationCode.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QOperatingSystemVersion>
#include <QSaveFile>
#include <QSysInfo>

namespace ta {
namespace rms {

namespace {

QString ageLabel(qint64 nowMs, qint64 thenMs)
{
    if (thenMs <= 0)
        return QStringLiteral("never");
    const qint64 d = qMax<qint64>(0, nowMs - thenMs);
    if (d < 1000)
        return QStringLiteral("%1 ms ago").arg(d);
    if (d < 90000)
        return QStringLiteral("%1 s ago").arg(d / 1000.0, 0, 'f', 1);
    return QStringLiteral("%1 min ago").arg(d / 60000);
}

QString csvField(const QString& v)
{
    QString s = v;
    s.replace(QLatin1Char('"'), QLatin1String("\"\""));
    if (s.contains(QLatin1Char(',')) || s.contains(QLatin1Char('"'))
        || s.contains(QLatin1Char('\n')))
        return QLatin1Char('"') + s + QLatin1Char('"');
    return s;
}

}  // namespace

FieldTestService::FieldTestService(RangeMonitor* monitor,
                                   RangeConfigurationService* range,
                                   MatchPlanService* plans,
                                   FieldTestRecorder* recorder,
                                   NetworkDiagnostics* network,
                                   QObject* parent)
    : QObject(parent)
    , m_monitor(monitor)
    , m_range(range)
    , m_plans(plans)
    , m_recorder(recorder)
    , m_network(network)
{
    if (m_monitor) {
        connect(m_monitor, &RangeMonitor::nodeAdded,
                this, &FieldTestService::onNodeAdded);
        connect(m_monitor, &RangeMonitor::shotObserved,
                this, &FieldTestService::onShot);
    }
    if (m_range) {
        connect(m_range, &RangeConfigurationService::rangeChanged,
                this, [this]() { poll(); });
    }
}

qint64 FieldTestService::now() const
{
    return m_clock ? m_clock() : QDateTime::currentMSecsSinceEpoch();
}

void FieldTestService::setMode(const QString& mode, bool live)
{
    m_mode = mode;
    m_live = live;
    emit changed();
}

// ── station identity ────────────────────────────────────────────────────

void FieldTestService::refreshCodes()
{
    if (!m_monitor)
        return;
    QStringList ids;
    for (const QString& id : m_monitor->nodeIds())
        ids << id;
    // Lanes that are configured but whose station has not been heard from yet
    // still need a code, or a commissioned range would show blanks until the
    // tablet came up.
    if (m_range) {
        for (const LaneDefinition& lane : m_range->range().lanes) {
            if (lane.isAssigned() && !ids.contains(lane.assignedNodeId))
                ids << lane.assignedNodeId;
        }
    }
    m_codes = StationCode::codesFor(ids);
}

QString FieldTestService::stationCode(const QString& nodeId) const
{
    if (nodeId.isEmpty())
        return QString();
    const auto it = m_codes.constFind(nodeId);
    if (it != m_codes.constEnd())
        return it.value();
    // Not in the current set: still deterministic, just not collision-checked
    // against anything.
    return StationCode::shortCode(nodeId);
}

bool FieldTestService::stationCodesCollide() const
{
    if (!m_monitor)
        return false;
    QStringList ids;
    for (const QString& id : m_monitor->nodeIds())
        ids << id;
    return StationCode::wouldCollide(ids);
}

int FieldTestService::laneNumberFor(const QString& nodeId) const
{
    return m_range ? m_range->laneNumberForNode(nodeId) : 0;
}

// ── the timeline ────────────────────────────────────────────────────────

void FieldTestService::onNodeAdded(const QString& nodeId)
{
    refreshCodes();
    if (!m_recorder)
        return;
    const int lane = laneNumberFor(nodeId);
    QJsonObject d;
    d[QStringLiteral("assignedToLane")] = lane > 0;
    m_recorder->recordLane(QStringLiteral("NODE_DISCOVERED"),
                           lane > 0
                               ? QStringLiteral("Station %1 discovered (Lane %2)")
                                     .arg(stationCode(nodeId)).arg(lane)
                               : QStringLiteral("Station %1 discovered — UNASSIGNED")
                                     .arg(stationCode(nodeId)),
                           lane, nodeId, stationCode(nodeId), d);
}

void FieldTestService::onShot(const QString& nodeId, int sequence)
{
    if (!m_recorder || !m_recorder->isActive() || !m_monitor)
        return;
    const TargetNodeRecord* r = m_monitor->nodeById(nodeId);
    if (!r)
        return;
    const int lane = laneNumberFor(nodeId);
    QJsonObject d;
    d[QStringLiteral("shotSequence")] = sequence;
    if (r->ledger.hasShots()) {
        const AcceptedShot s = r->ledger.latestReceived();
        d[QStringLiteral("authoritativeScore")] = s.authoritativeScore;
        d[QStringLiteral("xMm")] = s.rawXMm;
        d[QStringLiteral("yMm")] = s.rawYMm;
        d[QStringLiteral("sessionId")] = s.sessionId;
    }
    m_recorder->recordLane(QStringLiteral("SHOT_ACCEPTED_OBSERVED"),
                           QStringLiteral("Lane %1 shot #%2").arg(lane).arg(sequence),
                           lane, nodeId, stationCode(nodeId), d);
}

void FieldTestService::noteLogStarted()
{
    if (!m_recorder || !m_recorder->isActive())
        return;

    QJsonObject d;
    d[QStringLiteral("mode")] = m_mode;
#ifdef RMS_VERSION_STR
    d[QStringLiteral("rmsVersion")] = QStringLiteral(RMS_VERSION_STR);
#endif
#ifdef RMS_GIT_SHA
    d[QStringLiteral("gitCommit")] = QStringLiteral(RMS_GIT_SHA);
#endif
    d[QStringLiteral("protocolVersion")] = kProtocolVersion;
    m_recorder->record(QStringLiteral("RMS_START"),
                       QStringLiteral("RMS observing in %1 mode").arg(m_mode), d);

    if (m_network) {
        QJsonObject n;
        n[QStringLiteral("port")] = m_network->port();
        n[QStringLiteral("error")] = m_network->listenerError();
        // In DEMO there is no socket BY DESIGN, and calling that an error
        // would put a false alarm at the top of every demo bundle.
        if (!m_live) {
            m_recorder->record(QStringLiteral("UDP_LISTENER_NOT_OBSERVING"),
                               QStringLiteral("DEMO mode — no socket is opened"), n);
        } else {
            m_recorder->record(m_network->listening()
                                   ? QStringLiteral("UDP_LISTENER_STARTED")
                                   : QStringLiteral("UDP_LISTENER_ERROR"),
                               m_network->listening()
                                   ? QStringLiteral("Listening on UDP %1")
                                         .arg(m_network->port())
                                   : QStringLiteral("UDP %1 could not be opened")
                                         .arg(m_network->port()),
                               n);
        }
    }

    // The stations already known when the log opened. A timeline that started
    // mid-range would otherwise show a lane going offline that it never showed
    // arriving.
    if (m_monitor) {
        for (const QString& id : m_monitor->nodeIds()) {
            const TargetNodeRecord* r = m_monitor->nodeById(id);
            if (!r)
                continue;
            const int lane = laneNumberFor(id);
            QJsonObject s;
            s[QStringLiteral("connection")] = toString(r->connection);
            s[QStringLiteral("bootId")] = r->bootId;
            s[QStringLiteral("nodeAccepted")] = r->shotsAcceptedByNode;
            s[QStringLiteral("rmsObserved")] = r->ledger.observedCount();
            m_recorder->recordLane(QStringLiteral("STATION_ALREADY_KNOWN"),
                                   QStringLiteral("Station %1 already known (Lane %2, %3)")
                                       .arg(stationCode(id)).arg(lane)
                                       .arg(toString(r->connection)),
                                   lane, id, stationCode(id), s);
        }
    }
    // Snapshot now, so the first poll does not report every existing state as
    // a transition.
    poll();
}

void FieldTestService::poll()
{
    if (!m_monitor)
        return;
    refreshCodes();

    const bool logging = m_recorder && m_recorder->isActive();

    // ── lane assignment changes ──────────────────────────────────────────
    if (m_range) {
        QHash<int, QString> current;
        for (const LaneDefinition& lane : m_range->range().lanes) {
            if (lane.isAssigned())
                current.insert(lane.laneNumber, lane.assignedNodeId);
        }
        if (logging) {
            for (auto it = current.constBegin(); it != current.constEnd(); ++it) {
                const QString was = m_assigned.value(it.key());
                if (was == it.value())
                    continue;
                QJsonObject d;
                d[QStringLiteral("nodeId")] = it.value();
                if (!was.isEmpty())
                    d[QStringLiteral("previousNodeId")] = was;
                m_recorder->recordLane(
                    was.isEmpty() ? QStringLiteral("LANE_ASSIGNED")
                                  : QStringLiteral("LANE_REASSIGNED"),
                    was.isEmpty()
                        ? QStringLiteral("Lane %1 assigned to station %2")
                              .arg(it.key()).arg(stationCode(it.value()))
                        : QStringLiteral("Lane %1 reassigned from %2 to %3")
                              .arg(it.key()).arg(stationCode(was))
                              .arg(stationCode(it.value())),
                    it.key(), it.value(), stationCode(it.value()), d);
            }
            for (auto it = m_assigned.constBegin(); it != m_assigned.constEnd(); ++it) {
                if (current.contains(it.key()))
                    continue;
                m_recorder->recordLane(QStringLiteral("LANE_UNASSIGNED"),
                                       QStringLiteral("Lane %1 unassigned").arg(it.key()),
                                       it.key(), it.value(), stationCode(it.value()));
            }
        }
        m_assigned = current;
    }

    // ── per-node transitions ─────────────────────────────────────────────
    for (const QString& nodeId : m_monitor->nodeIds()) {
        const TargetNodeRecord* r = m_monitor->nodeById(nodeId);
        if (!r)
            continue;
        Snapshot& prev = m_snapshots[nodeId];
        const int lane = laneNumberFor(nodeId);
        const QString code = stationCode(nodeId);
        const int observed = r->ledger.observedCount();
        const int unseen = qMax(0, r->shotsAcceptedByNode - observed);

        if (logging && prev.known) {
            if (prev.connection != r->connection) {
                // ONLY the transition, never the heartbeat that confirmed it.
                const QString from = toString(prev.connection);
                const QString to = toString(r->connection);
                QJsonObject d;
                d[QStringLiteral("from")] = from;
                d[QStringLiteral("to")] = to;
                QString type = QStringLiteral("CONNECTION_CHANGED");
                QString text = QStringLiteral("Lane %1 %2 → %3").arg(lane).arg(from, to);
                if (r->connection == ConnectionState::Offline) {
                    type = QStringLiteral("NODE_OFFLINE");
                    text = QStringLiteral("Lane %1 (%2) went OFFLINE").arg(lane).arg(code);
                } else if (prev.connection == ConnectionState::Offline) {
                    type = QStringLiteral("NODE_RETURNED");
                    text = QStringLiteral("Lane %1 (%2) RETURNED").arg(lane).arg(code);
                } else if (r->connection == ConnectionState::TargetConnected) {
                    type = QStringLiteral("TARGET_CONNECTED");
                    text = QStringLiteral("Lane %1 target CONNECTED").arg(lane);
                } else if (prev.connection == ConnectionState::TargetConnected) {
                    type = QStringLiteral("TARGET_DISCONNECTED");
                    text = QStringLiteral("Lane %1 target DISCONNECTED").arg(lane);
                }
                m_recorder->recordLane(type, text, lane, nodeId, code, d);
            }
            if (!prev.bootId.isEmpty() && prev.bootId != r->bootId) {
                // A restart is a NODE event, never a lane event: the station
                // is the same station and stays on the same lane.
                QJsonObject d;
                d[QStringLiteral("previousBootId")] = prev.bootId;
                d[QStringLiteral("bootId")] = r->bootId;
                d[QStringLiteral("nodeRestarts")] = r->nodeRestarts;
                m_recorder->recordLane(
                    QStringLiteral("NODE_RESTART"),
                    QStringLiteral("Lane %1 (%2) node RESTARTED — same station, new boot")
                        .arg(lane).arg(code),
                    lane, nodeId, code, d);
            }
            // ONLY A NEW WORST GAP, or a gap fully closing.
            //
            // The two numbers come from different messages at different rates:
            // `shotsAcceptedByNode` rides the 2 s status heartbeat while shot
            // events arrive as they happen, so the difference flaps by one
            // continuously during normal shooting. Logging every change buried
            // the real outages under hundreds of lines that meant nothing.
            if (unseen > prev.worstUnseen) {
                QJsonObject d;
                d[QStringLiteral("nodeAccepted")] = r->shotsAcceptedByNode;
                d[QStringLiteral("rmsObserved")] = observed;
                d[QStringLiteral("unseen")] = unseen;
                d[QStringLiteral("previousWorst")] = prev.worstUnseen;
                m_recorder->recordLane(
                    QStringLiteral("UNSEEN_COUNT_CHANGED"),
                    QStringLiteral("Lane %1: node accepted %2, RMS observed %3, "
                                   "%4 unseen").arg(lane)
                        .arg(r->shotsAcceptedByNode).arg(observed).arg(unseen),
                    lane, nodeId, code, d);
            } else if (unseen == 0 && prev.worstUnseen > 0) {
                QJsonObject d;
                d[QStringLiteral("previousWorst")] = prev.worstUnseen;
                m_recorder->recordLane(
                    QStringLiteral("UNSEEN_RECONCILED"),
                    QStringLiteral("Lane %1: all %2 previously unseen shots "
                                   "accounted for").arg(lane).arg(prev.worstUnseen),
                    lane, nodeId, code, d);
            }
        }

        prev.known = true;
        prev.connection = r->connection;
        prev.bootId = r->bootId;
        prev.shotsAcceptedByNode = r->shotsAcceptedByNode;
        prev.observed = observed;
        prev.unseen = unseen;
        prev.worstUnseen = (unseen == 0) ? 0 : qMax(prev.worstUnseen, unseen);
        prev.laneNumber = lane;
    }

    emit changed();
}

// ── counters ────────────────────────────────────────────────────────────

QVariantMap FieldTestService::counters() const
{
    QVariantMap m;
    if (!m_monitor)
        return m;
    const qint64 t = now();

    int online = 0, offline = 0, unassigned = 0, targetConnected = 0;
    int restarts = 0, offlineEpisodes = 0, duplicates = 0, outOfOrder = 0;
    int conflicts = 0, observedShots = 0, acceptedByNodes = 0;
    for (const QString& id : m_monitor->nodeIds()) {
        const TargetNodeRecord* r = m_monitor->nodeById(id);
        if (!r)
            continue;
        if (r->isOffline()) ++offline; else ++online;
        if (r->connection == ConnectionState::TargetConnected) ++targetConnected;
        if (laneNumberFor(id) <= 0) ++unassigned;
        restarts += r->nodeRestarts;
        offlineEpisodes += r->offlineEpisodes;
        duplicates += r->ledger.duplicatesSuppressed();
        outOfOrder += r->ledger.outOfOrderAccepted();
        conflicts += r->ledger.sequenceConflicts();
        observedShots += r->ledger.observedCount();
        acceptedByNodes += r->shotsAcceptedByNode;
    }

    m[QStringLiteral("lanesConfigured")] = m_range ? m_range->laneCount() : 0;
    m[QStringLiteral("lanesAssigned")] = m_range ? m_range->assignedLaneCount() : 0;
    m[QStringLiteral("nodesDiscovered")] = m_monitor->nodeCount();
    m[QStringLiteral("nodesOnline")] = online;
    m[QStringLiteral("nodesOffline")] = offline;
    m[QStringLiteral("targetsConnected")] = targetConnected;
    m[QStringLiteral("unassignedDevices")] = unassigned;

    m[QStringLiteral("packetsAccepted")] = m_monitor->acceptedDatagrams();
    m[QStringLiteral("packetsRejected")] = m_monitor->rejectedDatagrams();
    m[QStringLiteral("malformed")] = m_monitor->malformedDatagrams();
    m[QStringLiteral("unknownVersion")] = m_monitor->unknownVersionDatagrams();
    m[QStringLiteral("unknownType")] = m_monitor->unknownTypeDatagrams();
    m[QStringLiteral("announces")] = m_monitor->announcesReceived();
    m[QStringLiteral("statuses")] = m_monitor->statusesReceived();
    m[QStringLiteral("shotMessages")] = m_monitor->shotMessagesReceived();

    m[QStringLiteral("duplicatesSuppressed")] = duplicates;
    m[QStringLiteral("outOfOrder")] = outOfOrder;
    m[QStringLiteral("sequenceConflicts")] = conflicts;
    m[QStringLiteral("nodeRestarts")] = restarts;
    m[QStringLiteral("offlineEpisodes")] = offlineEpisodes;

    // THE NUMBER THAT MATTERS ON A RANGE DAY. Not a percentage, not a health
    // score: how many shots the stations accepted that RMS never saw.
    m[QStringLiteral("shotsObserved")] = observedShots;
    m[QStringLiteral("shotsAcceptedByNodes")] = acceptedByNodes;
    m[QStringLiteral("shotsUnseen")] = qMax(0, acceptedByNodes - observedShots);

    m[QStringLiteral("lastPacketAge")] =
        ageLabel(t, m_monitor->lastDatagramUtcMs());
    return m;
}

// ── preflight ───────────────────────────────────────────────────────────

QVariantList FieldTestService::preflight() const
{
    QVariantList rows;
    auto row = [&rows](const QString& label, const QString& state,
                       const QString& detail) {
        QVariantMap m;
        m[QStringLiteral("label")] = label;
        m[QStringLiteral("state")] = state;      // PASS | WARNING | FAIL | WAITING
        m[QStringLiteral("detail")] = detail;
        rows.append(m);
    };

    // ── the machine ──────────────────────────────────────────────────────
    row(QStringLiteral("RMS mode"),
        m_live ? QStringLiteral("PASS") : QStringLiteral("WARNING"),
        m_live ? QStringLiteral("LIVE — observing real stations")
               : QStringLiteral("DEMO — nothing on screen is a real target"));

    if (m_network) {
        if (!m_live) {
            row(QStringLiteral("UDP listener"), QStringLiteral("WARNING"),
                QStringLiteral("not observing: RMS is in DEMO mode"));
        } else if (m_network->listening()) {
            row(QStringLiteral("UDP listener"), QStringLiteral("PASS"),
                QStringLiteral("listening on UDP %1").arg(m_network->port()));
        } else {
            // The loudest thing on the page. An empty range with a dead socket
            // must never look like an empty range with quiet tablets.
            row(QStringLiteral("UDP listener"), QStringLiteral("FAIL"),
                m_network->listenerError().isEmpty()
                    ? QStringLiteral("UDP %1 could not be opened")
                          .arg(m_network->port())
                    : m_network->listenerError());
        }
        row(QStringLiteral("Network interface"),
            m_network->hasUsableInterface() ? QStringLiteral("PASS")
                                            : QStringLiteral("FAIL"),
            m_network->hasUsableInterface()
                ? QStringLiteral("%1 usable IPv4 interface(s)")
                      .arg(m_network->interfaces().size())
                : QStringLiteral("no usable IPv4 interface — check the adapter"));
    }

    // ── the range ────────────────────────────────────────────────────────
    const bool configured = m_range && m_range->isConfigured();
    row(QStringLiteral("Range configured"),
        configured ? QStringLiteral("PASS") : QStringLiteral("FAIL"),
        configured ? QStringLiteral("%1 — %2 physical lanes")
                         .arg(m_range->rangeName()).arg(m_range->laneCount())
                   : QStringLiteral("no range defined — create one in Range Setup"));

    if (configured) {
        const int assigned = m_range->assignedLaneCount();
        // Nothing commissioned yet is WAITING, not FAIL: it is the normal
        // state of a range five minutes before the test starts, and a red
        // line there teaches an operator to ignore red lines.
        row(QStringLiteral("Lanes commissioned"),
            assigned == 0 ? QStringLiteral("WAITING")
                          : (assigned < m_range->laneCount()
                                 ? QStringLiteral("WARNING") : QStringLiteral("PASS")),
            assigned == 0
                ? QStringLiteral("none yet — assign stations in Range Setup")
                : QStringLiteral("%1 of %2 lanes have a station assigned")
                      .arg(assigned).arg(m_range->laneCount()));

        // A node on two lanes would make every later observation ambiguous.
        QHash<QString, int> seen;
        QStringList dupes;
        for (const LaneDefinition& lane : m_range->range().lanes) {
            if (!lane.isAssigned())
                continue;
            if (seen.contains(lane.assignedNodeId)) {
                dupes << QStringLiteral("%1 on lanes %2 and %3")
                             .arg(stationCode(lane.assignedNodeId))
                             .arg(seen.value(lane.assignedNodeId))
                             .arg(lane.laneNumber);
            }
            seen.insert(lane.assignedNodeId, lane.laneNumber);
        }
        row(QStringLiteral("Duplicate assignments"),
            dupes.isEmpty() ? QStringLiteral("PASS") : QStringLiteral("FAIL"),
            dupes.isEmpty() ? QStringLiteral("none")
                            : dupes.join(QStringLiteral("; ")));
    }

    row(QStringLiteral("Station codes"),
        stationCodesCollide() ? QStringLiteral("WARNING") : QStringLiteral("PASS"),
        stationCodesCollide()
            ? QStringLiteral("two stations share a short code — codes are shown longer")
            : QStringLiteral("all distinct"));

    // ── the stations ─────────────────────────────────────────────────────
    if (m_monitor) {
        const int discovered = m_monitor->nodeCount();
        if (discovered == 0) {
            row(QStringLiteral("Stations heard"), QStringLiteral("WAITING"),
                QStringLiteral("no station has broadcast yet"));
        } else {
            int off = 0;
            for (const QString& id : m_monitor->nodeIds()) {
                const TargetNodeRecord* r = m_monitor->nodeById(id);
                if (r && r->isOffline()) ++off;
            }
            row(QStringLiteral("Stations heard"),
                off == 0 ? QStringLiteral("PASS") : QStringLiteral("WARNING"),
                off == 0 ? QStringLiteral("%1 station(s), all answering").arg(discovered)
                         : QStringLiteral("%1 station(s), %2 offline")
                               .arg(discovered).arg(off));
        }
    }

    // ── the instrument ───────────────────────────────────────────────────
    const bool logging = m_recorder && m_recorder->isActive();
    row(QStringLiteral("Field-test log"),
        logging ? QStringLiteral("PASS") : QStringLiteral("WAITING"),
        logging ? QStringLiteral("recording — %1").arg(m_recorder->sessionId())
                : QStringLiteral("ready — press START FIELD TEST LOG"));

    return rows;
}

QString FieldTestService::preflightVerdict() const
{
    const QVariantList rows = preflight();
    bool fail = false, warn = false, waiting = false;
    for (const QVariant& v : rows) {
        const QString s = v.toMap().value(QStringLiteral("state")).toString();
        if (s == QLatin1String("FAIL")) fail = true;
        else if (s == QLatin1String("WARNING")) warn = true;
        else if (s == QLatin1String("WAITING")) waiting = true;
    }
    // DELIBERATELY NOT "RANGE READY". RMS cannot command or certify a station,
    // so it has no business declaring a range fit for competition. It can only
    // say that its own observation path checks out.
    if (fail)
        return QStringLiteral("OBSERVATION PREFLIGHT FAILED");
    if (waiting)
        return QStringLiteral("OBSERVATION PREFLIGHT WAITING");
    if (warn)
        return QStringLiteral("OBSERVATION PREFLIGHT COMPLETE — WITH WARNINGS");
    return QStringLiteral("OBSERVATION PREFLIGHT COMPLETE");
}

// ── per-lane diagnostics ────────────────────────────────────────────────

QVariantMap FieldTestService::laneDiagnostics(int laneNumber) const
{
    QVariantMap m;
    m[QStringLiteral("laneNumber")] = laneNumber;
    if (!m_range || !m_monitor)
        return m;

    const QString nodeId = m_range->nodeForLaneNumber(laneNumber);
    m[QStringLiteral("assigned")] = !nodeId.isEmpty();
    if (nodeId.isEmpty())
        return m;

    m[QStringLiteral("nodeId")] = nodeId;
    m[QStringLiteral("stationCode")] = stationCode(nodeId);

    const TargetNodeRecord* r = m_monitor->nodeById(nodeId);
    m[QStringLiteral("heard")] = (r != nullptr);
    if (!r)
        return m;

    const qint64 t = now();
    m[QStringLiteral("connection")] = toString(r->connection);
    m[QStringLiteral("online")] = !r->isOffline();
    m[QStringLiteral("bootId")] = r->bootId;
    m[QStringLiteral("appVersion")] = r->appVersion;
    m[QStringLiteral("deviceIdentity")] = r->deviceIdentity;
    m[QStringLiteral("observedProgramme")] = r->programmeId;
    m[QStringLiteral("observedSession")] = r->sessionId;
    m[QStringLiteral("observedPhase")] = toString(r->phase);
    m[QStringLiteral("observedAthlete")] = r->athleteName;
    m[QStringLiteral("lastSeen")] = ageLabel(t, r->lastSeenUtcMs);
    m[QStringLiteral("lastSeenUtcMs")] = r->lastSeenUtcMs;
    m[QStringLiteral("firstSeenUtcMs")] = r->firstSeenUtcMs;

    // Reconciliation — the heart of the field test.
    const int observed = r->ledger.observedCount();
    m[QStringLiteral("nodeAccepted")] = r->shotsAcceptedByNode;
    m[QStringLiteral("rmsObserved")] = observed;
    m[QStringLiteral("unseen")] = qMax(0, r->shotsAcceptedByNode - observed);
    m[QStringLiteral("duplicatesSuppressed")] = r->ledger.duplicatesSuppressed();
    m[QStringLiteral("outOfOrder")] = r->ledger.outOfOrderAccepted();
    m[QStringLiteral("sequenceConflicts")] = r->ledger.sequenceConflicts();
    m[QStringLiteral("nodeRestarts")] = r->nodeRestarts;
    m[QStringLiteral("offlineEpisodes")] = r->offlineEpisodes;
    m[QStringLiteral("staleStatusDropped")] = r->staleStatusDropped;
    m[QStringLiteral("staleBootDropped")] = r->staleBootDropped;
    m[QStringLiteral("nodeTotal")] = r->totalScoreByNode;
    m[QStringLiteral("observedSum")] = r->ledger.observedScoreSum();

    // Which sequence numbers are missing, named rather than counted: "gap at
    // 14-17" is actionable, "3 missing" is not. The ledger already knows -
    // asking it beats a second implementation that could disagree.
    const QList<int> missing = r->ledger.missingSequences();
    QStringList gaps;
    for (int i = 0; i < missing.size();) {
        int j = i;
        while (j + 1 < missing.size() && missing.at(j + 1) == missing.at(j) + 1)
            ++j;
        gaps << (j > i ? QStringLiteral("%1-%2").arg(missing.at(i)).arg(missing.at(j))
                       : QString::number(missing.at(i)));
        i = j + 1;
    }
    m[QStringLiteral("sequenceGaps")] =
        gaps.isEmpty() ? QStringLiteral("none") : gaps.join(QStringLiteral(", "));
    m[QStringLiteral("sequenceGapCount")] = int(missing.size());
    return m;
}

// ── export ──────────────────────────────────────────────────────────────

QJsonObject FieldTestService::rangeSnapshotJson() const
{
    QJsonObject o;
    if (!m_range || !m_range->isConfigured())
        return o;
    o[QStringLiteral("rangeName")] = m_range->rangeName();
    o[QStringLiteral("rangeType")] = m_range->rangeType();
    o[QStringLiteral("rangeMode")] = m_range->rangeModeLabel();
    o[QStringLiteral("laneCount")] = m_range->laneCount();
    o[QStringLiteral("firstLaneNumber")] = m_range->firstLaneNumber();
    o[QStringLiteral("lastLaneNumber")] = m_range->lastLaneNumber();
    o[QStringLiteral("assignedLaneCount")] = m_range->assignedLaneCount();
    return o;
}

QJsonObject FieldTestService::laneMappingsJson() const
{
    QJsonObject o;
    QJsonArray lanes;
    if (m_range) {
        for (const LaneDefinition& lane : m_range->range().lanes) {
            QJsonObject l;
            l[QStringLiteral("laneNumber")] = lane.laneNumber;
            l[QStringLiteral("laneId")] = lane.laneId;
            l[QStringLiteral("enabled")] = lane.enabled;
            l[QStringLiteral("assignedNodeId")] = lane.assignedNodeId;
            if (lane.isAssigned())
                l[QStringLiteral("stationCode")] = stationCode(lane.assignedNodeId);
            lanes.append(l);
        }
    }
    o[QStringLiteral("lanes")] = lanes;
    // Said explicitly in the evidence, because it is the rule the whole
    // commissioning model rests on.
    o[QStringLiteral("mappingAuthority")] =
        QStringLiteral("laneId <-> nodeId. IP, bootId, COM port and discovery "
                       "order are diagnostics and are never identity.");
    return o;
}

QJsonObject FieldTestService::nodeSummaryJson() const
{
    QJsonObject o;
    QJsonArray arr;
    if (m_monitor) {
        const qint64 t = now();
        for (const QString& id : m_monitor->nodeIds()) {
            const TargetNodeRecord* r = m_monitor->nodeById(id);
            if (!r)
                continue;
            QJsonObject n;
            n[QStringLiteral("nodeId")] = id;
            n[QStringLiteral("stationCode")] = stationCode(id);
            n[QStringLiteral("laneNumber")] = laneNumberFor(id);
            n[QStringLiteral("bootId")] = r->bootId;
            n[QStringLiteral("connection")] = toString(r->connection);
            n[QStringLiteral("appVersion")] = r->appVersion;
            n[QStringLiteral("deviceIdentity")] = r->deviceIdentity;
            n[QStringLiteral("programmeId")] = r->programmeId;
            n[QStringLiteral("sessionId")] = r->sessionId;
            n[QStringLiteral("nodeAccepted")] = r->shotsAcceptedByNode;
            n[QStringLiteral("rmsObserved")] = r->ledger.observedCount();
            n[QStringLiteral("unseen")] =
                qMax(0, r->shotsAcceptedByNode - r->ledger.observedCount());
            n[QStringLiteral("nodeRestarts")] = r->nodeRestarts;
            n[QStringLiteral("offlineEpisodes")] = r->offlineEpisodes;
            n[QStringLiteral("duplicatesSuppressed")] = r->ledger.duplicatesSuppressed();
            n[QStringLiteral("outOfOrder")] = r->ledger.outOfOrderAccepted();
            n[QStringLiteral("sequenceConflicts")] = r->ledger.sequenceConflicts();
            n[QStringLiteral("totalScoreByNode")] = r->totalScoreByNode;
            n[QStringLiteral("observedScoreSum")] = r->ledger.observedScoreSum();
            n[QStringLiteral("lastSeenUtcMs")] = r->lastSeenUtcMs;
            n[QStringLiteral("lastSeen")] = ageLabel(t, r->lastSeenUtcMs);
            QJsonArray boots;
            for (const QString& b : r->priorBootIds)
                boots.append(b);
            n[QStringLiteral("priorBootIds")] = boots;
            arr.append(n);
        }
    }
    o[QStringLiteral("nodes")] = arr;
    return o;
}

QJsonObject FieldTestService::diagnosticsJson() const
{
    QJsonObject o;
    o[QStringLiteral("mode")] = m_mode;
    o[QStringLiteral("live")] = m_live;
    o[QStringLiteral("protocolVersion")] = kProtocolVersion;
    o[QStringLiteral("observationPort")] = int(kObservationPort);
#ifdef RMS_VERSION_STR
    o[QStringLiteral("rmsVersion")] = QStringLiteral(RMS_VERSION_STR);
#endif
#ifdef RMS_GIT_SHA
    o[QStringLiteral("gitCommit")] = QStringLiteral(RMS_GIT_SHA);
#endif
    o[QStringLiteral("qtVersion")] = QStringLiteral(QT_VERSION_STR);
    o[QStringLiteral("os")] = QSysInfo::prettyProductName();
    o[QStringLiteral("kernel")] = QSysInfo::kernelVersion();
    if (m_network) {
        o[QStringLiteral("listening")] = m_network->listening();
        o[QStringLiteral("listenerError")] = m_network->listenerError();
        o[QStringLiteral("hostName")] = m_network->hostName();
        QJsonArray ifs;
        for (const QVariant& v : m_network->interfaces()) {
            const QVariantMap m = v.toMap();
            QJsonObject j;
            j[QStringLiteral("name")] = m.value(QStringLiteral("name")).toString();
            j[QStringLiteral("address")] = m.value(QStringLiteral("address")).toString();
            j[QStringLiteral("netmask")] = m.value(QStringLiteral("netmask")).toString();
            ifs.append(j);
        }
        o[QStringLiteral("interfaces")] = ifs;
    }
    const QVariantMap c = counters();
    QJsonObject cj;
    for (auto it = c.constBegin(); it != c.constEnd(); ++it)
        cj[it.key()] = QJsonValue::fromVariant(it.value());
    o[QStringLiteral("counters")] = cj;
    return o;
}

QJsonObject FieldTestService::summaryJson() const
{
    QJsonObject o;
    if (m_recorder) {
        o[QStringLiteral("testSessionId")] = m_recorder->sessionId();
        o[QStringLiteral("testName")] = m_recorder->testName();
        o[QStringLiteral("rangeName")] = m_recorder->rangeName();
        o[QStringLiteral("operator")] = m_recorder->operatorName();
        o[QStringLiteral("notes")] = m_recorder->notes();
        o[QStringLiteral("durationMs")] = m_recorder->elapsedMs();
        o[QStringLiteral("duration")] = m_recorder->elapsedLabel();
        o[QStringLiteral("eventCount")] = m_recorder->eventCount();
        if (!m_recorder->previousSessionId().isEmpty())
            o[QStringLiteral("previousTestSessionId")] = m_recorder->previousSessionId();
    }
    o[QStringLiteral("mode")] = m_mode;
    // THE LINE THAT STOPS A DEMO BUNDLE BEING READ AS RANGE EVIDENCE.
    o[QStringLiteral("simulated")] = !m_live;
    o[QStringLiteral("dataSource")] = m_live
        ? QStringLiteral("LIVE — real target stations observed on UDP 7755")
        : QStringLiteral("DEMO — every station, shot and score was generated "
                         "locally. NOT physical evidence.");
    // Never claimed by software. Only the physical checklist can set this.
    o[QStringLiteral("physicalShotRegistrationVerified")] = false;
    o[QStringLiteral("physicalQualification")] =
        QStringLiteral("NOT PHYSICAL — see docs/test/"
                       "rms-physical-shot-registration-checklist.md");
    o[QStringLiteral("range")] = rangeSnapshotJson();
    o[QStringLiteral("counters")] = diagnosticsJson().value(QStringLiteral("counters"));
    return o;
}

QString FieldTestService::summaryText() const
{
    QStringList L;
    L << QStringLiteral("TECH AIM RMS — FIELD TEST SUMMARY");
    L << QStringLiteral("=================================");
    L << QString();
    if (!m_live) {
        L << QStringLiteral("*** SIMULATED / DEMO DATA — NOT PHYSICAL EVIDENCE ***");
        L << QStringLiteral("Every station, shot and score below was generated "
                            "inside RMS.");
        L << QString();
    }
    if (m_recorder) {
        L << QStringLiteral("Test session:  %1").arg(m_recorder->sessionId());
        if (!m_recorder->testName().isEmpty())
            L << QStringLiteral("Test name:     %1").arg(m_recorder->testName());
        if (!m_recorder->rangeName().isEmpty())
            L << QStringLiteral("Range:         %1").arg(m_recorder->rangeName());
        if (!m_recorder->operatorName().isEmpty())
            L << QStringLiteral("Operator:      %1").arg(m_recorder->operatorName());
        L << QStringLiteral("Duration:      %1").arg(m_recorder->elapsedLabel());
        L << QStringLiteral("Events:        %1").arg(m_recorder->eventCount());
        if (!m_recorder->previousSessionId().isEmpty()) {
            L << QStringLiteral("Continues:     %1 (RMS restarted; the gap is "
                                "not reconstructable)")
                     .arg(m_recorder->previousSessionId());
        }
    }
    L << QStringLiteral("Mode:          %1").arg(m_mode);
    L << QString();

    const QVariantMap c = counters();
    L << QStringLiteral("RANGE");
    L << QStringLiteral("  Physical lanes:        %1")
             .arg(c.value(QStringLiteral("lanesConfigured")).toInt());
    L << QStringLiteral("  Lanes commissioned:    %1")
             .arg(c.value(QStringLiteral("lanesAssigned")).toInt());
    L << QStringLiteral("  Stations observed:     %1")
             .arg(c.value(QStringLiteral("nodesDiscovered")).toInt());
    L << QStringLiteral("  Unassigned devices:    %1")
             .arg(c.value(QStringLiteral("unassignedDevices")).toInt());
    L << QString();
    L << QStringLiteral("OBSERVATION QUALITY");
    L << QStringLiteral("  Shots accepted by stations: %1")
             .arg(c.value(QStringLiteral("shotsAcceptedByNodes")).toInt());
    L << QStringLiteral("  Shots observed by RMS:      %1")
             .arg(c.value(QStringLiteral("shotsObserved")).toInt());
    L << QStringLiteral("  UNSEEN by RMS:              %1")
             .arg(c.value(QStringLiteral("shotsUnseen")).toInt());
    L << QStringLiteral("  Duplicates suppressed:      %1")
             .arg(c.value(QStringLiteral("duplicatesSuppressed")).toInt());
    L << QStringLiteral("  Out of order accepted:      %1")
             .arg(c.value(QStringLiteral("outOfOrder")).toInt());
    L << QStringLiteral("  Sequence conflicts:         %1")
             .arg(c.value(QStringLiteral("sequenceConflicts")).toInt());
    L << QStringLiteral("  Node restarts:              %1")
             .arg(c.value(QStringLiteral("nodeRestarts")).toInt());
    L << QStringLiteral("  Offline episodes:           %1")
             .arg(c.value(QStringLiteral("offlineEpisodes")).toInt());
    L << QStringLiteral("  Protocol rejects:           %1 (malformed %2, "
                        "unknown version %3, unknown type %4)")
             .arg(c.value(QStringLiteral("packetsRejected")).toInt())
             .arg(c.value(QStringLiteral("malformed")).toInt())
             .arg(c.value(QStringLiteral("unknownVersion")).toInt())
             .arg(c.value(QStringLiteral("unknownType")).toInt());
    L << QString();

    L << QStringLiteral("PER LANE");
    if (m_range) {
        for (const LaneDefinition& lane : m_range->range().lanes) {
            if (!lane.isAssigned())
                continue;
            const QVariantMap d = laneDiagnostics(lane.laneNumber);
            L << QStringLiteral("  Lane %1  station %2")
                     .arg(lane.laneNumber)
                     .arg(d.value(QStringLiteral("stationCode")).toString());
            if (!d.value(QStringLiteral("heard")).toBool()) {
                L << QStringLiteral("      never heard from");
                continue;
            }
            const int accepted = d.value(QStringLiteral("nodeAccepted")).toInt();
            const int observed = d.value(QStringLiteral("rmsObserved")).toInt();
            const int unseen = d.value(QStringLiteral("unseen")).toInt();
            L << QStringLiteral("      node %1   boot %2")
                     .arg(d.value(QStringLiteral("connection")).toString(),
                          d.value(QStringLiteral("bootId")).toString());
            L << QStringLiteral("      node accepted %1, RMS observed %2, %3 unseen")
                     .arg(accepted).arg(observed).arg(unseen);
            if (observed > accepted) {
                // Not an error, and worth saying so before somebody files one.
                L << QStringLiteral("      (RMS has seen shots newer than the "
                                    "station's last status heartbeat — the two "
                                    "counts are sampled at different rates)");
            }
            if (unseen > 0) {
                // Said in words as well as numbers, because this is exactly the
                // sentence somebody will quote out of the bundle.
                L << QStringLiteral("      %1 individual impacts unavailable to "
                                    "RMS — not fabricated, not estimated")
                         .arg(unseen);
            }
            L << QStringLiteral("      restarts %1, offline episodes %2, "
                                "duplicates %3, out-of-order %4, conflicts %5")
                     .arg(d.value(QStringLiteral("nodeRestarts")).toInt())
                     .arg(d.value(QStringLiteral("offlineEpisodes")).toInt())
                     .arg(d.value(QStringLiteral("duplicatesSuppressed")).toInt())
                     .arg(d.value(QStringLiteral("outOfOrder")).toInt())
                     .arg(d.value(QStringLiteral("sequenceConflicts")).toInt());
            L << QStringLiteral("      sequence gaps: %1")
                     .arg(d.value(QStringLiteral("sequenceGaps")).toString());
        }
    }
    L << QString();
    L << QStringLiteral("PHYSICAL SHOT REGISTRATION: NOT VERIFIED BY THIS BUNDLE.");
    L << QStringLiteral("RMS observes and configures. It cannot command a target,");
    L << QStringLiteral("and it does not calculate a score.");
    return L.join(QLatin1Char('\n'));
}

QString FieldTestService::shotsCsv() const
{
    QStringList lines;
    lines << QStringLiteral("timestampUtcMs,laneNumber,stationCode,nodeId,bootId,"
                            "sessionId,programmeId,shotSequence,xMm,yMm,"
                            "authoritativeScore,observedIndex,mode,simulated");
    if (!m_monitor)
        return lines.join(QLatin1Char('\n'));

    for (const QString& id : m_monitor->nodeIds()) {
        const TargetNodeRecord* r = m_monitor->nodeById(id);
        if (!r)
            continue;
        const int lane = laneNumberFor(id);
        const QString code = stationCode(id);
        int index = 0;
        for (const AcceptedShot& s : r->ledger.shotsInOrder()) {
            ++index;
            QStringList f;
            f << QString::number(s.timestampUtcMs)
              << QString::number(lane)
              << csvField(code)
              << csvField(id)
              << csvField(s.bootId)
              << csvField(s.sessionId)
              << csvField(s.programmeId)
              << QString::number(s.shotSequence)
              << QString::number(s.rawXMm, 'f', 3)
              << QString::number(s.rawYMm, 'f', 3)
              // TRANSPORTED, never recalculated.
              << QString::number(s.authoritativeScore, 'f', 1)
              << QString::number(index)
              << csvField(m_mode)
              << (m_live ? QStringLiteral("false") : QStringLiteral("true"));
            lines << f.join(QLatin1Char(','));
        }
    }
    return lines.join(QLatin1Char('\n'));
}

QString FieldTestService::exportFieldTest()
{
    m_lastExportError.clear();

    const QDateTime dt = QDateTime::fromMSecsSinceEpoch(now(), Qt::UTC);
    const QString stamp = dt.toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString dirName = QStringLiteral("TechAim-RMS-FieldTest-%1").arg(stamp);
    const QString dir = QDir(FieldTestRecorder::fieldTestRoot()).filePath(dirName);
    if (!QDir().mkpath(dir)) {
        m_lastExportError = QStringLiteral("cannot create %1").arg(dir);
        emit changed();
        return QString();
    }

    auto write = [&](const QString& name, const QByteArray& data) -> bool {
        QSaveFile f(QDir(dir).filePath(name));
        if (!f.open(QIODevice::WriteOnly)) {
            m_lastExportError = QStringLiteral("cannot write %1").arg(name);
            return false;
        }
        f.write(data);
        if (!f.commit()) {
            m_lastExportError = QStringLiteral("cannot commit %1").arg(name);
            return false;
        }
        return true;
    };
    auto writeJson = [&](const QString& name, const QJsonObject& o) {
        return write(name, QJsonDocument(o).toJson(QJsonDocument::Indented));
    };

    if (m_recorder)
        m_recorder->flush();

    bool ok = true;
    ok = ok && write(QStringLiteral("summary.txt"), summaryText().toUtf8());
    ok = ok && writeJson(QStringLiteral("summary.json"), summaryJson());
    ok = ok && writeJson(QStringLiteral("range-snapshot.json"), rangeSnapshotJson());
    ok = ok && writeJson(QStringLiteral("lane-mappings.json"), laneMappingsJson());
    ok = ok && writeJson(QStringLiteral("node-summary.json"), nodeSummaryJson());
    ok = ok && writeJson(QStringLiteral("diagnostics.json"), diagnosticsJson());
    ok = ok && write(QStringLiteral("shots.csv"), shotsCsv().toUtf8());

    // The events, copied out of the live log so the bundle stands alone.
    if (ok && m_recorder) {
        QByteArray jsonl;
        for (const QJsonObject& e : m_recorder->allEvents()) {
            jsonl += QJsonDocument(e).toJson(QJsonDocument::Compact);
            jsonl += '\n';
        }
        ok = write(QStringLiteral("events.jsonl"), jsonl);
    }

    if (ok) {
        QStringList readme;
        readme << QStringLiteral("TECH AIM RMS — FIELD TEST EVIDENCE BUNDLE");
        readme << QStringLiteral("=========================================");
        readme << QString();
        if (!m_live) {
            readme << QStringLiteral("*** SIMULATED / DEMO DATA ***");
            readme << QStringLiteral("This bundle was produced in DEMO mode. No real");
            readme << QStringLiteral("target station was involved. It is NOT physical");
            readme << QStringLiteral("qualification evidence.");
            readme << QString();
        }
        readme << QStringLiteral("Contents");
        readme << QStringLiteral("  summary.txt          human-readable result");
        readme << QStringLiteral("  summary.json         the same, machine-readable");
        readme << QStringLiteral("  range-snapshot.json  the range as configured");
        readme << QStringLiteral("  lane-mappings.json   laneId <-> nodeId, the only mapping");
        readme << QStringLiteral("  node-summary.json    per-station observation totals");
        readme << QStringLiteral("  diagnostics.json     versions, network, counters");
        readme << QStringLiteral("  events.jsonl         the timeline, one JSON object per line");
        readme << QStringLiteral("  shots.csv            every impact RMS observed");
        readme << QString();
        readme << QStringLiteral("What this bundle does NOT contain");
        readme << QStringLiteral("  Source code, repository paths or test material.");
        readme << QStringLiteral("  Shots the stations accepted but RMS never received —");
        readme << QStringLiteral("  those are COUNTED as unseen and never invented.");
        readme << QString();
        readme << QStringLiteral("Scores are the stations' own. RMS does not calculate them.");
        readme << QStringLiteral("Physical shot registration is NOT verified by this bundle.");
        ok = write(QStringLiteral("README.txt"), readme.join(QLatin1Char('\n')).toUtf8());
    }

    if (!ok) {
        emit changed();
        return QString();
    }

    m_lastExportPath = dir;
    if (m_recorder && m_recorder->isActive()) {
        QJsonObject d;
        d[QStringLiteral("path")] = dir;
        m_recorder->record(QStringLiteral("FIELD_TEST_EXPORTED"),
                           QStringLiteral("Evidence bundle written"), d);
    }
    emit changed();
    return dir;
}

}  // namespace rms
}  // namespace ta
