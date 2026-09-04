#ifndef TA_TELEMETRY_SHOTTELEMETRY_H
#define TA_TELEMETRY_SHOTTELEMETRY_H

// ONE definition of "an accepted shot, as the range sees it".
//
// WHY THIS FILE EXISTS. Live telemetry publishes a shot the moment the node
// accepts it; replay republishes the SAME shot later out of the journal. RMS
// deduplicates on eventId, so those two paths must produce byte-identical
// events - and the only way to guarantee that is for both to call one
// function. Two copies of the eventId formula would drift, and the day they
// drifted every catch-up would insert duplicates instead of suppressing them.
//
// NO SCORING HAPPENS HERE. scoreTenths and the coordinates are values the node
// has ALREADY accepted; this converts units and nothing else.

#include "reliability/events/EventTypes.h"
#include "rms/RmsProtocol.h"

#include <QString>

namespace ta {
namespace telemetry {

// The stable, DERIVED, reproducible identity of one accepted shot. A
// retransmission regenerates the same id, which is what lets RMS suppress the
// copy instead of showing shot 17 twice.
inline QString shotEventId(const QString& sessionId, int shotNumber)
{
    return QStringLiteral("%1:official:%2").arg(sessionId).arg(shotNumber);
}

// Everything the wire carries about one accepted shot. `nodeId`, `bootId`,
// `laneId` and `programmeId` are the publisher's, not the shot's, so they are
// passed in rather than guessed at here.
inline ta::rms::AcceptedShot toAcceptedShot(const ta::rel::ShotCore& core,
                                            const QString& sessionId,
                                            const QString& nodeId,
                                            const QString& bootId,
                                            const QString& laneId,
                                            const QString& programmeId,
                                            qint64 timestampUtcMs)
{
    ta::rms::AcceptedShot shot;
    shot.protocolVersion = ta::rms::kProtocolVersion;
    shot.eventId      = shotEventId(sessionId, core.shotNumber);
    shot.nodeId       = nodeId;
    shot.bootId       = bootId;
    shot.laneId       = laneId;
    shot.sessionId    = sessionId;
    shot.programmeId  = programmeId;
    // POSITION IS DELIBERATELY EMPTY. The only per-position identity the
    // reducer holds is an integer index, and its meaning differs between rule
    // authorities - the ISSF and DSB three-position orders are not the same.
    // Labelling a lane with the wrong position is worse than labelling it with
    // none; carrying the index needs a v2 field agreed against a rule source.
    shot.position     = QString();
    // THE NODE'S OWN ACCEPTED SEQUENCE - not a packet count, not a model row.
    shot.shotSequence = core.shotNumber;
    shot.rawXMm       = double(core.xHundredthMm) / 100.0;
    shot.rawYMm       = double(core.yHundredthMm) / 100.0;
    // THE NODE'S SCORE, TRANSPORTED. RMS never recalculates it.
    shot.authoritativeScore = double(core.scoreTenths) / 10.0;
    shot.integerScore = core.scoreTenths / 10;
    // NOT AVAILABLE. Inner-ten is a ring-geometry fact and the accepted shot
    // record does not carry one; deriving it from a score threshold here would
    // be RMS-visible scoring invented by the transport. False means "not
    // reported", and a v2 field is the honest fix.
    shot.innerTen     = false;
    shot.timestampUtcMs = timestampUtcMs;
    shot.acquisitionStatus = core.simulated ? QStringLiteral("SIMULATED")
                                            : QStringLiteral("ACCEPTED");
    return shot;
}

} // namespace telemetry
} // namespace ta

#endif // TA_TELEMETRY_SHOTTELEMETRY_H
