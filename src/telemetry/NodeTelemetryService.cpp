#include "NodeTelemetryService.h"

#include "reliability/reducer/SessionState.h"

#include <QDateTime>
#include <QTimer>

#include <variant>

namespace ta {
namespace telemetry {

namespace {

constexpr int kFlushIntervalMs    = 25;      // off the acquisition stack
constexpr int kStatusIntervalMs   = 2000;    // RMS times out after 3 of these
constexpr int kAnnounceIntervalMs = 30000;   // slow rediscovery

// The node's fixed-point record -> the wire's decimals. This is FORMATTING,
// not scoring: scoreTenths is the value the node already accepted.
double tenthsToScore(qint16 tenths) { return double(tenths) / 10.0; }
double hundredthMmToMm(qint32 v)    { return double(v) / 100.0; }

ta::rms::MatchPhase phaseFor(const ta::rel::SessionState& s)
{
    if (s.lifecycle == ta::rel::Lifecycle::Complete
        || s.lifecycle == ta::rel::Lifecycle::Closed)
        return ta::rms::MatchPhase::Complete;
    switch (s.phase) {
    case ta::rel::MatchPhase::Preparation:   return ta::rms::MatchPhase::Preparation;
    case ta::rel::MatchPhase::Sighting:      return ta::rms::MatchPhase::Sighting;
    case ta::rel::MatchPhase::OfficialMatch: return ta::rms::MatchPhase::Match;
    case ta::rel::MatchPhase::None:          break;
    }
    return ta::rms::MatchPhase::Idle;
}

} // namespace

NodeTelemetryService::NodeTelemetryService(NodeIdentity identity,
                                           ITelemetrySink* sink, QObject* parent)
    : QObject(parent)
    , m_identity(std::move(identity))
    , m_sink(sink)
    , m_clock([] { return QDateTime::currentMSecsSinceEpoch(); })
{
}

NodeTelemetryService::~NodeTelemetryService() = default;

qint64 NodeTelemetryService::nowMs() const
{
    return m_clock ? m_clock() : 0;
}

void NodeTelemetryService::setTargetConnected(bool connected)
{
    if (m_targetConnected == connected)
        return;
    m_targetConnected = connected;
    // A target coming or going is exactly the sort of thing a range officer
    // needs immediately, not on the next heartbeat.
    if (m_running)
        publishStatus();
}

void NodeTelemetryService::setProgramme(const QString& programmeId,
                                        const QString& rulesetId,
                                        const QString& targetStandardId)
{
    if (m_programmeId == programmeId && m_rulesetId == rulesetId
        && m_targetStandardId == targetStandardId)
        return;
    m_programmeId = programmeId;
    m_rulesetId = rulesetId;
    m_targetStandardId = targetStandardId;
    if (m_running)
        publishStatus();
}

void NodeTelemetryService::attachStore(ta::rel::SessionStore* store)
{
    if (!store || m_stores.contains(store))
        return;
    m_stores.append(store);
    connect(store, &ta::rel::SessionStore::eventApplied, this,
            [this, store](const ta::rel::DomainEvent& e, bool replayed) {
                onEventApplied(store, e, replayed);
            });
    // A store destroyed under us must not leave a dangling active pointer.
    connect(store, &QObject::destroyed, this, [this, store]() {
        m_stores.removeAll(store);
        if (m_activeStore == store)
            m_activeStore = nullptr;
    });
}

ta::rel::SessionStore* NodeTelemetryService::activeStore() const
{
    if (m_activeStore && m_activeStore->active())
        return m_activeStore;
    // Fall back to any store that is currently live — the active pointer is a
    // hint, the store's own `active()` is the fact.
    for (ta::rel::SessionStore* s : m_stores)
        if (s && s->active())
            return s;
    return nullptr;
}

// ── the accepted-shot seam ───────────────────────────────────────────────

void NodeTelemetryService::onEventApplied(ta::rel::SessionStore* store,
                                          const ta::rel::DomainEvent& event,
                                          bool replayed)
{
    // Recovery replay rebuilds history. It is not a shot being fired, and v1
    // has no message that means "this already happened".
    if (replayed || !m_running)
        return;

    const ta::rel::SessionState& state = store->state();

    // A new session invalidates the per-session publish guard.
    if (state.sessionId != m_publishedSession) {
        m_publishedSession = state.sessionId;
        m_publishedShots.clear();
    }

    if (std::holds_alternative<ta::rel::ShotAccepted>(event)) {
        const ta::rel::ShotCore& core = std::get<ta::rel::ShotAccepted>(event).shot;

        // One accepted shot, one logical event. A duplicated internal
        // delivery of the same accepted shot must not become a second
        // observation on the range display.
        if (m_publishedShots.contains(core.shotNumber))
            return;
        m_publishedShots.insert(core.shotNumber);

        ta::rms::AcceptedShot shot;
        // STABLE, DERIVED, REPRODUCIBLE. A retransmission of this same shot
        // regenerates this same id, which is what lets RMS suppress the copy
        // instead of showing shot 17 twice.
        shot.eventId      = QStringLiteral("%1:official:%2")
                                .arg(state.sessionId).arg(core.shotNumber);
        shot.nodeId       = m_identity.nodeId();
        shot.bootId       = m_identity.bootId();
        shot.laneId       = m_laneHint;
        shot.sessionId    = state.sessionId;
        shot.programmeId  = m_programmeId;
        // POSITION IS DELIBERATELY EMPTY. The only per-position identity the
        // reducer holds is an integer index, and its meaning differs between
        // rule authorities (the ISSF and DSB three-position orders are not the
        // same). Labelling a lane with the wrong position is worse than
        // labelling it with none; carrying the index needs a v2 field agreed
        // against a rule source. See the milestone document.
        shot.position     = QString();
        // THE NODE'S OWN ACCEPTED SEQUENCE — not a packet count, not a model
        // row, not a visual tally.
        shot.shotSequence = core.shotNumber;
        shot.rawXMm       = hundredthMmToMm(core.xHundredthMm);
        shot.rawYMm       = hundredthMmToMm(core.yHundredthMm);
        // THE NODE'S SCORE, TRANSPORTED. RMS never recalculates it.
        shot.authoritativeScore = tenthsToScore(core.scoreTenths);
        shot.integerScore = core.scoreTenths / 10;
        // NOT AVAILABLE. Inner-ten is a ring-geometry fact and the accepted
        // shot record does not carry one; deriving it from a score threshold
        // here would be RMS-visible scoring invented by the transport. False
        // means "not reported", and a v2 field is the honest fix.
        shot.innerTen     = false;
        shot.timestampUtcMs   = nowMs();
        shot.acquisitionStatus = core.simulated ? QStringLiteral("SIMULATED")
                                                : QStringLiteral("ACCEPTED");
        enqueue(ta::rms::encode(shot));
        ++m_shotsPublished;
        // The shot moved the authoritative count and total, so the range view
        // should follow it immediately rather than at the next heartbeat.
        publishStatus();
        return;
    }

    // SIGHTERS ARE NOT PUBLISHED AS ACCEPTED SHOTS. A SighterAccepted carries
    // shotNumber 0 by construction, protocol v1 requires a positive
    // competition sequence, and there is no field that classifies a shot as a
    // sighter. Synthesising a sequence for it would put sighters into RMS's
    // match ledger — the one thing the classification exists to prevent. The
    // node's own record keeps them, exactly as before.
    if (std::holds_alternative<ta::rel::SighterAccepted>(event))
        return;

    // Phase and lifecycle transitions: publish promptly so a range display
    // does not lag a start or a finish by a heartbeat.
    const bool phaseEvent =
           std::holds_alternative<ta::rel::SessionStarted>(event)
        || std::holds_alternative<ta::rel::PreparationStarted>(event)
        || std::holds_alternative<ta::rel::SightingStarted>(event)
        || std::holds_alternative<ta::rel::OfficialMatchStarted>(event)
        || std::holds_alternative<ta::rel::PositionChanged>(event)
        || std::holds_alternative<ta::rel::MatchCompleted>(event)
        || std::holds_alternative<ta::rel::SessionClosed>(event);

    if (phaseEvent) {
        if (std::holds_alternative<ta::rel::SessionStarted>(event))
            m_activeStore = store;
        publishStatus();
    }
}

// ── message construction ─────────────────────────────────────────────────

ta::rms::NodeStatus NodeTelemetryService::buildStatus() const
{
    ta::rms::NodeStatus s;
    s.nodeId = m_identity.nodeId();
    s.bootId = m_identity.bootId();
    s.laneId = m_laneHint;
    // OFFLINE IS NEVER SENT. It is RMS's conclusion from silence; a node
    // asserting it would be a contradiction. TargetConnected vs
    // TargetDisconnected is the real distinction this node can make.
    s.connection = m_targetConnected ? ta::rms::ConnectionState::TargetConnected
                                     : ta::rms::ConnectionState::TargetDisconnected;
    s.health = m_targetConnected ? QStringLiteral("OK")
                                 : QStringLiteral("target link down");
    s.timestampUtcMs = nowMs();

    const ta::rel::SessionStore* store = activeStore();
    if (!store) {
        // No competition session open. Everything below stays at its default,
        // which is the truthful description of an idle lane.
        s.phase = ta::rms::MatchPhase::Idle;
        s.shotsExpected = -1;
        return s;
    }

    const ta::rel::SessionState& st = store->state();
    s.sessionId        = st.sessionId;
    s.programmeId      = m_programmeId;
    s.rulesetId        = m_rulesetId;
    s.targetStandardId = m_targetStandardId;
    s.athleteName      = st.athlete;
    s.position         = QString();          // see the note in onEventApplied
    s.phase            = phaseFor(st);
    // THE AUTHORITATIVE COUNT AND TOTAL, read from the reducer-owned record.
    s.shotsAccepted    = int(st.officials.size());
    s.shotsExpected    = st.config.officialShots > 0 ? st.config.officialShots : -1;
    s.totalScore       = double(st.totalTenths) / 10.0;
    if (!st.lane.isEmpty())
        s.laneId = st.lane;                  // provisional display only
    return s;
}

void NodeTelemetryService::publishAnnounce()
{
    ta::rms::NodeAnnounce a;
    a.nodeId          = m_identity.nodeId();
    a.bootId          = m_identity.bootId();
    a.laneId          = m_laneHint;
    a.deviceIdentity  = m_deviceIdentity;
    a.appVersion      = m_appVersion;
    a.productIdentity = m_productIdentity;
    a.timestampUtcMs  = nowMs();
    enqueue(ta::rms::encode(a));
    ++m_announces;
}

void NodeTelemetryService::publishStatus()
{
    ta::rms::NodeStatus s = buildStatus();
    // Monotonic within this boot. RMS drops anything not strictly newer, which
    // is what stops a reordered datagram from dragging a live lane backwards.
    s.statusSeq = ++m_statusSeq;
    enqueue(ta::rms::encode(s));
    ++m_statuses;
}

// ── outbox ───────────────────────────────────────────────────────────────

void NodeTelemetryService::enqueue(const QByteArray& datagram)
{
    if (m_outbox.size() >= kOutboxCapacity) {
        // Drop the OLDEST. Telemetry is a live view: the newest observation is
        // the one worth keeping, and RMS reconciles any gap from the
        // authoritative counts in the next status.
        m_outbox.dequeue();
        ++m_dropped;
        emit telemetryDropped(m_dropped);
    }
    m_outbox.enqueue(datagram);
    // Nothing is sent here. This function is reached from inside the shot's
    // own call stack, and a socket call there is exactly what §12 forbids.
}

int NodeTelemetryService::flushOutbox()
{
    if (!m_sink)
        return 0;
    int sent = 0;
    while (!m_outbox.isEmpty()) {
        const QByteArray datagram = m_outbox.dequeue();
        if (m_sink->send(datagram)) {
            ++sent;
            continue;
        }
        // One attempt, then dropped and counted. No synchronous retry, no
        // growing backlog: RMS already reports the difference between what the
        // node accepted and what it observed, which is the correct place for a
        // lost datagram to become visible.
        ++m_sendFailures;
        emit telemetrySendFailed(QStringLiteral("telemetry datagram dropped (%1 total)")
                                     .arg(m_sendFailures));
    }
    return sent;
}

// ── lifecycle ────────────────────────────────────────────────────────────

void NodeTelemetryService::start()
{
    if (m_running)
        return;
    m_running = true;

    if (!m_flushTimer) {
        m_flushTimer = new QTimer(this);
        m_flushTimer->setInterval(kFlushIntervalMs);
        connect(m_flushTimer, &QTimer::timeout, this, [this] { flushOutbox(); });

        m_statusTimer = new QTimer(this);
        m_statusTimer->setInterval(kStatusIntervalMs);
        connect(m_statusTimer, &QTimer::timeout, this, [this] { publishStatus(); });

        m_announceTimer = new QTimer(this);
        m_announceTimer->setInterval(kAnnounceIntervalMs);
        connect(m_announceTimer, &QTimer::timeout, this, [this] { publishAnnounce(); });
    }
    m_flushTimer->start();
    m_statusTimer->start();
    m_announceTimer->start();

    publishAnnounce();
    publishStatus();
}

void NodeTelemetryService::stop()
{
    m_running = false;
    if (m_flushTimer) {
        m_flushTimer->stop();
        m_statusTimer->stop();
        m_announceTimer->stop();
    }
    m_outbox.clear();
}

} // namespace telemetry
} // namespace ta
