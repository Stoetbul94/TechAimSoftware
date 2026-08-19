#include "SimulatedRange.h"

#include "../RmsProtocol.h"

namespace ta {
namespace rms {
namespace dev {

namespace {

constexpr qint64 kStepMs        = 250;
constexpr qint64 kStatusEveryMs = 2000;
constexpr qint64 kShotEveryMs   = 1500;

struct LanePlan {
    const char* lane;
    const char* athlete;
    const char* programmeId;
    const char* rulesetId;
    const char* targetStandardId;
    int         shotsExpected;
};

// Programme identities taken verbatim from CompetitionCatalogue.qml. They are
// stable ids, not labels, which is exactly why they can be hardcoded here
// without repeating the QML-LANG-001 mistake.
const LanePlan kPlan[] = {
    { "Lane 1", "A. Bailie",   "issf.10m.air-rifle.qualification60",
      "issf",    "issf.10m.air-rifle",  60 },
    { "Lane 2", "M. Keller",   "issf.10m.air-pistol.qualification60",
      "issf",    "issf.10m.air-pistol", 60 },
    { "Lane 3", "S. Nkosi",    "techaim.10m.air-rifle.match40",
      "techaim", "issf.10m.air-rifle",  40 },
    { "Lane 4", "J. Bergmann", "issf.50m.rifle.qualification60",
      "issf",    "issf.50m.rifle",      60 },
    { "Lane 5", "P. Rossouw",  "techaim.10m.air-pistol.match20",
      "techaim", "issf.10m.air-pistol", 20 },
    { "Lane 6", "T. Adeyemi",  "techaim.50m.pistol.free",
      "techaim", "issf.50m.pistol",     -1 }
};

} // namespace

SimulatedRange::SimulatedRange(QObject* parent)
    : QObject(parent)
{
}

void SimulatedRange::configure(int laneCount, Scenario scenario)
{
    const int n = qBound(3, laneCount, 6);
    m_nodes.clear();
    m_nowMs = 0;
    m_emitted = 0;
    m_rand = 20260819u;

    for (int i = 0; i < n; ++i) {
        SimNode s;
        // A stable node identity that is NOT the COM port, NOT the IP and NOT
        // the lane — the whole point of §3 of the milestone brief.
        s.nodeId    = QStringLiteral("TA-NODE-%1").arg(i + 1, 3, 10, QLatin1Char('0'));
        s.bootId    = QStringLiteral("boot-%1-a").arg(i + 1);
        s.laneId    = QString::fromLatin1(kPlan[i].lane);
        s.sessionId = QStringLiteral("sess-%1-2026-08-19").arg(i + 1);
        s.programmeId      = QString::fromLatin1(kPlan[i].programmeId);
        s.rulesetId        = QString::fromLatin1(kPlan[i].rulesetId);
        s.targetStandardId = QString::fromLatin1(kPlan[i].targetStandardId);
        s.athlete   = QString::fromLatin1(kPlan[i].athlete);
        s.device    = QStringLiteral("TechAim-EST/%1").arg(4100 + i);
        s.shotsExpected = kPlan[i].shotsExpected;
        // Stagger the first shot so the dashboard does not pulse in lockstep.
        s.nextShotMs = 3000 + (i * 400);
        if (scenario == Scenario::FieldTestDemo) {
            // Lane 3 goes off and stays off: an operator who opens the demo at
            // any moment must find an offline lane waiting, not one that has
            // already healed. Lane 4 goes off and comes back, which is what
            // produces the unseen-shot warning - and it is given an explicit
            // window rather than being left to a restart's two silent seconds,
            // which may or may not swallow a shot.
            s.dropsOut = false;
            s.restarts = false;
            if (i == 2) { s.silentFromMs = 14000; s.silentToMs = -1; }
            if (i == 3) { s.silentFromMs = 14000; s.silentToMs = 34000; }
        } else {
            s.dropsOut = (i == 2);   // Lane 3 loses the network and comes back
            s.restarts = (i == 4);   // Lane 5's application restarts
        }
        s.swapPending = true;    // every lane delivers one pair out of order
        m_nodes.append(s);
    }
}

double SimulatedRange::nextScore()
{
    // Deterministic LCG (Numerical Recipes constants). Range 8.0 .. 10.9,
    // which is a realistic decimal spread and keeps the totals readable.
    m_rand = 1664525u * m_rand + 1013904223u;
    const int tenths = int((m_rand >> 16) % 30u);
    return 8.0 + (tenths / 10.0);
}

bool SimulatedRange::isSilent(const SimNode& n, qint64 tMs) const
{
    if (n.silentFromMs >= 0 && tMs >= n.silentFromMs
        && (n.silentToMs < 0 || tMs < n.silentToMs))
        return true;
    if (n.dropsOut && tMs >= laneDropoutStartMs() && tMs < laneDropoutEndMs())
        return true;
    // A restarting node is silent for one heartbeat around the restart.
    if (n.restarts && tMs >= nodeRestartAtMs() && tMs < nodeRestartAtMs() + 2000)
        return true;
    return false;
}

void SimulatedRange::concludeLane(int laneNumber)
{
    const QString lane = QStringLiteral("Lane %1").arg(laneNumber);
    for (SimNode& n : m_nodes) {
        if (n.laneId == lane)
            n.concluded = true;
    }
}

void SimulatedRange::advanceTo(qint64 nowMs)
{
    while (m_nowMs < nowMs) {
        m_nowMs += kStepMs;
        stepOnce(m_nowMs);
    }
}

void SimulatedRange::stepOnce(qint64 tMs)
{
    for (SimNode& n : m_nodes) {

        // THE MATCH CONTINUES WHEN RMS CANNOT SEE IT. `silent` suppresses
        // TRANSMISSION only — the simulated node goes on accepting shots and
        // advancing its own state exactly as a real node does when the
        // network drops. Skipping the whole node here would model a target
        // that politely stops shooting while the range office is offline,
        // which is the opposite of the invariant being demonstrated.
        const bool silent = isSilent(n, tMs);

        // A node that restarted comes back with a NEW bootId. Its nodeId is
        // unchanged — that is how RMS tells "restarted" from "new node".
        if (n.restarts && tMs >= nodeRestartAtMs() + 2000
            && n.bootId.endsWith(QLatin1Char('a'))) {
            n.bootId = n.bootId.left(n.bootId.size() - 1) + QLatin1Char('b');
            n.announced = false;
            n.statusSeq = 0;
        }

        if (!silent && !n.announced) {
            emitAnnounce(n, tMs, silent);
            n.announced = true;
            n.nextStatusMs = tMs;   // status immediately after announcing
        }

        if (tMs >= n.nextStatusMs) {
            emitStatus(n, tMs, silent);
            n.nextStatusMs = tMs + kStatusEveryMs;
        }

        const bool matchRunning = tMs >= 3000 && !n.concluded
                                  && (n.shotsExpected < 0 || n.shotsAccepted < n.shotsExpected);
        if (matchRunning && tMs >= n.nextShotMs) {
            emitShot(n, tMs, silent);
            n.nextShotMs = tMs + kShotEveryMs;
        }
    }
}

void SimulatedRange::emitAnnounce(SimNode& n, qint64 tMs, bool silent)
{
    if (silent)
        return;
    NodeAnnounce a;
    a.nodeId          = n.nodeId;
    a.bootId          = n.bootId;
    a.laneId          = n.laneId;
    a.deviceIdentity  = n.device;
    a.appVersion      = QStringLiteral("0.9.0-rms-sim");
    a.productIdentity = QStringLiteral("Tech Aim");
    a.timestampUtcMs  = tMs;
    ++m_emitted;
    emit datagramProduced(encode(a));
}

void SimulatedRange::emitStatus(SimNode& n, qint64 tMs, bool silent)
{
    NodeStatus s;
    s.nodeId           = n.nodeId;
    s.bootId           = n.bootId;
    s.laneId           = n.laneId;
    s.sessionId        = n.sessionId;
    s.programmeId      = n.programmeId;
    s.rulesetId        = n.rulesetId;
    s.targetStandardId = n.targetStandardId;
    s.athleteName      = n.athlete;
    s.position         = QString();
    s.connection       = ConnectionState::TargetConnected;
    s.shotsAccepted    = n.shotsAccepted;
    s.shotsExpected    = n.shotsExpected;
    s.totalScore       = n.totalScore;
    s.health           = QStringLiteral("OK");
    s.statusSeq        = ++n.statusSeq;
    s.timestampUtcMs   = tMs;

    if (tMs < 2000)
        s.phase = MatchPhase::Preparation;
    else if (tMs < 3000)
        s.phase = MatchPhase::Sighting;
    else if (n.shotsExpected > 0 && n.shotsAccepted >= n.shotsExpected)
        s.phase = MatchPhase::Complete;
    else
        s.phase = MatchPhase::Match;

    // The node's own statusSeq advanced whether or not anyone was listening.
    if (silent)
        return;
    ++m_emitted;
    emit datagramProduced(encode(s));
}

void SimulatedRange::emitShot(SimNode& n, qint64 tMs, bool silent)
{
    ++n.shotsAccepted;
    AcceptedShot sh;
    sh.eventId      = QStringLiteral("%1-%2-%3")
                          .arg(n.nodeId, n.sessionId)
                          .arg(++n.eventCounter);
    sh.nodeId       = n.nodeId;
    sh.bootId       = n.bootId;
    sh.laneId       = n.laneId;
    sh.sessionId    = n.sessionId;
    sh.programmeId  = n.programmeId;
    sh.shotSequence = n.shotsAccepted;
    sh.rawXMm       = ((int(nextScore() * 10) % 21) - 10) / 2.0;
    sh.rawYMm       = ((int(nextScore() * 10) % 19) - 9)  / 2.0;
    // THE SIMULATED NODE SCORES THE SHOT. RMS transports this value.
    sh.authoritativeScore = nextScore();
    sh.integerScore = int(sh.authoritativeScore);
    sh.innerTen     = sh.authoritativeScore >= 10.7;
    sh.timestampUtcMs = tMs;
    sh.acquisitionStatus = QStringLiteral("ACCEPTED");
    n.totalScore += sh.authoritativeScore;

    // The shot WAS accepted by the node — n.shotsAccepted and n.totalScore
    // above are already updated. Only its broadcast is lost.
    if (silent)
        return;

    const QByteArray payload = encode(sh);

    // Out-of-order delivery, once per node: hold shot #5 back and release it
    // after #6 so the dashboard is proven not to reorder or lose it.
    if (n.swapPending && sh.shotSequence == 5) {
        n.heldShot = payload;
        return;                       // nothing emitted yet — #6 goes first
    }
    ++m_emitted;
    emit datagramProduced(payload);

    if (n.swapPending && sh.shotSequence == 6 && !n.heldShot.isEmpty()) {
        ++m_emitted;
        emit datagramProduced(n.heldShot);   // late #5
        n.heldShot.clear();
        n.swapPending = false;
    }

    // Duplicate delivery, once per node: UDP genuinely does this. Shot #3 is
    // re-sent byte-identically and must appear exactly once.
    if (!n.emittedDuplicate && sh.shotSequence == 3) {
        n.emittedDuplicate = true;
        ++m_emitted;
        emit datagramProduced(payload);
    }
}

} // namespace dev
} // namespace rms
} // namespace ta
