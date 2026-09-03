// TIME SYNC AND SCHEDULED STARTS (R2B §16-§20).
//
// THE PROBLEM. "Start all lanes" cannot mean "send START and hope". Fifty
// commands leave RMS at fifty slightly different moments and arrive after
// fifty different delays; a node that starts on ARRIVAL starts its
// competition clock at its own delivery jitter. On a 60-shot match that is a
// per-lane head start nobody chose.
//
// THE ANSWER UNDER TEST. RMS measures each node's clock offset, then names an
// INSTANT rather than an action: "start at T". Each node converts T into its
// own clock using its own measured offset and schedules. Delivery jitter then
// moves only how much warning a lane gets, not when it starts.
//
// WHAT IS CLAIMED, AND WHAT IS NOT. The offset estimate is only as good as the
// path is symmetric, and this exchange can bound it by no better than half the
// round trip. That bound is REPORTED - GOOD / DEGRADED / UNUSABLE - and an
// UNUSABLE lane is refused rather than started on a guess. No competition-grade
// synchronisation claim is made here, and none may be made from these tests:
// they measure software against a simulator, on one machine, with no network.

#include "control_harness.h"
#include "test_support.h"

#include <cstdio>

using namespace ta::rms;
using namespace ta::rms::control;
using ta::rms::test::ControlHarness;

namespace {

QByteArray key32() { return QByteArray(32, '\x2c'); }

// One four-stamp exchange, with the transit delays chosen by the caller so a
// symmetric path and a badly asymmetric one can both be exercised.
//
//   t0            RMS sends
//   t0+d1         it arrives; the node stamps t1 on ITS clock
//   +processing   the node stamps t2 and replies
//   t3            RMS receives
TimeSync exchange(RangeControlCoordinator& c, const QString& nodeId,
                  qint64 trueOffsetMs, qint64 t0, qint64 d1, qint64 d2,
                  qint64 processingMs = 0)
{
    const qint64 nodeT1 = t0 + d1 + trueOffsetMs;
    const qint64 nodeT2 = nodeT1 + processingMs;
    const qint64 t3     = t0 + d1 + processingMs + d2;
    return c.measureTimeSync(nodeId, t0, t3, nodeT1, nodeT2);
}

void testSymmetricPathIsExact()
{
    ControlHarness h(key32());
    h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"), 4000);
    h.connectAll();

    // A node whose clock is four seconds ahead, on a 6 ms symmetric path.
    const TimeSync s = exchange(h.coordinator(), QStringLiteral("TA-NODE-001"),
                                4000, h.now(), 3, 3, 2);
    check(s.offsetMs == 4000, "timesync: a symmetric path recovers the offset exactly");
    check(s.rttMs == 6, "timesync: the round trip excludes the node's processing time");
    check(s.uncertaintyMs == 3, "timesync: uncertainty is half the round trip");
    check(s.quality == SyncQuality::Good, "timesync: a 3 ms bound is GOOD");
    check(syncQualityName(s.quality) == QLatin1String("GOOD"),
          "timesync: the quality is reported by name, not as a bare number");
}

void testAsymmetryShowsUpAsError()
{
    ControlHarness h(key32());
    h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.connectAll();

    // 40 ms out, 4 ms back. The estimate is wrong by (d1-d2)/2 = 18 ms - and
    // the reported uncertainty of 22 ms COVERS that error, which is the whole
    // reason the bound is published alongside the number.
    const TimeSync s = exchange(h.coordinator(), QStringLiteral("TA-NODE-001"),
                                0, h.now(), 40, 4);
    check(s.offsetMs == 18, "timesync: an asymmetric path biases the estimate");
    check(s.uncertaintyMs == 22, "timesync: and the reported bound covers the error");
    check(qAbs(s.offsetMs - 0) <= s.uncertaintyMs,
          "timesync: the true offset lies inside the reported bound");
    check(s.quality == SyncQuality::Good, "timesync: 22 ms is still GOOD");
}

void testDegradedAndUnusable()
{
    ControlHarness h(key32());
    h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.addNode(QStringLiteral("TA-NODE-002"), QStringLiteral("Lane 2"));
    h.connectAll();

    const TimeSync d = exchange(h.coordinator(), QStringLiteral("TA-NODE-001"),
                                0, h.now(), 60, 60);
    check(d.uncertaintyMs == 60, "timesync: a 120 ms round trip bounds at 60 ms");
    check(d.quality == SyncQuality::Degraded, "timesync: that is DEGRADED, not GOOD");

    const TimeSync u = exchange(h.coordinator(), QStringLiteral("TA-NODE-002"),
                                0, h.now(), 900, 900);
    check(u.quality == SyncQuality::Unusable,
          "timesync: a 1.8 s round trip is UNUSABLE");
    check(syncQualityName(u.quality) == QLatin1String("UNUSABLE"),
          "timesync: reported as UNUSABLE by name");
}

void testImpossibleMeasurementIsRejected()
{
    ControlHarness h(key32());
    h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"));
    h.connectAll();

    // A node that claims to have spent longer processing than the whole
    // exchange took. The round trip comes out negative, which cannot happen -
    // so the measurement is discarded rather than believed.
    const TimeSync s = h.coordinator().measureTimeSync(
        QStringLiteral("TA-NODE-001"), 1000, 1010, 5000, 5100);
    check(s.rttMs < 0, "timesync: the arithmetic detects the impossibility");
    check(s.quality == SyncQuality::Unusable,
          "timesync: an impossible measurement is UNUSABLE, never trusted");
}

// ── the point of all of it ──────────────────────────────────────────────────
void testScheduledStartLandsTogether()
{
    ControlHarness h(key32());
    // Three lanes whose clocks disagree by seconds - which is what a range of
    // independently-booted tablets actually looks like.
    struct Lane { const char* id; qint64 offset; };
    const Lane lanes[] = {{"TA-NODE-001", -2500}, {"TA-NODE-002", 0}, {"TA-NODE-003", 7100}};
    for (const Lane& l : lanes)
        h.addNode(QLatin1String(l.id), QStringLiteral("Lane"), l.offset);
    check(h.connectAll() == 3, "startat: three lanes authenticate");

    for (const Lane& l : lanes)
        exchange(h.coordinator(), QLatin1String(l.id), l.offset, h.now(), 5, 5, 1);

    const qint64 startAt = h.now() + 30000;      // half a minute from now
    const FanOutResult r = h.coordinator().startAt(
        QStringList{"TA-NODE-001", "TA-NODE-002", "TA-NODE-003"}, startAt, h.now());
    check(r.allAccepted(), "startat: all three lanes accept the scheduled start");
    check(r.accepted() == 3, "startat: three acceptances");

    // THE ASSERTION THAT MATTERS. Each node scheduled on its OWN clock; convert
    // each back through its true offset and they must name the SAME instant.
    bool together = true;
    for (const Lane& l : lanes) {
        const auto* n = h.node(QLatin1String(l.id));
        if (n->scheduledStartNodeMs() - l.offset != startAt) together = false;
    }
    check(together, "startat: every lane starts at the same real instant");

    // And they did NOT merely schedule the raw number: a lane 7.1 s ahead
    // scheduled 7.1 s later on its own clock.
    check(h.node(QStringLiteral("TA-NODE-003"))->scheduledStartNodeMs() == startAt + 7100,
          "startat: the offset was applied, not ignored");
    check(h.node(QStringLiteral("TA-NODE-001"))->scheduledStartNodeMs() == startAt - 2500,
          "startat: including a lane whose clock is behind");
}

void testUnusableSyncRefusesTheStart()
{
    ControlHarness h(key32());
    h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"), 0);
    h.addNode(QStringLiteral("TA-NODE-002"), QStringLiteral("Lane 2"), 0);
    h.connectAll();

    exchange(h.coordinator(), QStringLiteral("TA-NODE-001"), 0, h.now(), 4, 4);
    // Lane 2 is never measured at all, so its quality stays UNUSABLE.

    const FanOutResult r = h.coordinator().startAt(
        QStringList{"TA-NODE-001", "TA-NODE-002"}, h.now() + 10000, h.now());
    check(!r.allAccepted(), "startat: the fan-out does not claim success");
    check(r.accepted() == 1 && r.failed() == 1,
          "startat: one lane started, one refused - reported separately");
    check(r.failures().first().nodeId == QLatin1String("TA-NODE-002"),
          "startat: the failure names WHICH lane");
    check(r.failures().first().reasonCode == QLatin1String("SYNC_UNUSABLE"),
          "startat: and why");

    // The refused lane was not started on a guess.
    check(h.node(QStringLiteral("TA-NODE-002"))->scheduledStartNodeMs() < 0,
          "startat: an unsynchronised lane is left unscheduled, not started blind");
    check(h.node(QStringLiteral("TA-NODE-001"))->scheduledStartNodeMs() > 0,
          "startat: the synchronised lane is unaffected by its neighbour");
}

void testSecondStartIsRefused()
{
    ControlHarness h(key32());
    auto* n = h.addNode(QStringLiteral("TA-NODE-001"), QStringLiteral("Lane 1"), 0);
    h.connectAll();
    exchange(h.coordinator(), QStringLiteral("TA-NODE-001"), 0, h.now(), 4, 4);

    const qint64 first = h.now() + 10000;
    check(h.coordinator().startAt(QStringList{"TA-NODE-001"}, first, h.now()).allAccepted(),
          "startat: the first scheduled start is accepted");
    const qint64 scheduled = n->scheduledStartNodeMs();

    // A second START_AT with a DIFFERENT command id and a different instant.
    // It must be refused: silently re-basing a competition clock that is
    // already running would rewrite the elapsed time of a live match.
    const FanOutResult again =
        h.coordinator().startAt(QStringList{"TA-NODE-001"}, first + 5000, h.now() + 1);
    check(!again.allAccepted(), "startat: a second start is refused");
    check(again.failures().first().reasonCode == QLatin1String(reason::kPreconditionFailed),
          "startat: refused as a precondition failure");
    check(n->scheduledStartNodeMs() == scheduled,
          "startat: and the already-scheduled instant is UNCHANGED");
}

} // namespace

void run_timesync_tests()
{
    std::printf("\n-- time sync and scheduled starts --\n");
    testSymmetricPathIsExact();
    testAsymmetryShowsUpAsError();
    testDegradedAndUnusable();
    testImpossibleMeasurementIsRejected();
    testScheduledStartLandsTogether();
    testUnusableSyncRefusesTheStart();
    testSecondStartIsRefused();
    std::fflush(stdout);
}
