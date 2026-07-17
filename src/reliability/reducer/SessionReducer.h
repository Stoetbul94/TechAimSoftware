#ifndef TA_REL_SESSIONREDUCER_H
#define TA_REL_SESSIONREDUCER_H

// Session Reliability Layer — pure deterministic reducer (M1, spec §8).
//
// apply(state, envelope) -> new state or typed error. The reducer:
//   - performs NO I/O, reads NO clock, NO settings, NO globals
//   - emits NO signals, touches NO controllers or QML
//   - assumes the JOURNAL layer already verified seq/sid/chain; it still
//     enforces its own ordering (seq == lastSeq + 1) as a defence layer
//   - validates transition LEGALITY against lifecycle/phase, then applies,
//     then re-derives totals FROM THE RECORDS (never incremented in
//     parallel anywhere else — spec §8) and checks post-invariants.
//
// Correction events (ShotInvalidated / ShotRescored / SeriesAdjusted /
// PenaltyIssued) are fully reduced in M1 per spec §28. Live producers for
// them arrive with the M2+ jury UI.

#include "SessionState.h"
#include "reliability/events/EventEnvelope.h"

namespace ta {
namespace rel {

struct ReduceResult {
    SessionState state;      // new state on ok, the UNCHANGED input otherwise
    bool ok = true;
    ErrorInfo error;
};

namespace SessionReducer {

ReduceResult apply(const SessionState& current, const EventEnvelope& envelope);

// Post-reduction invariants (also callable from tests/harnesses):
// totals equal the derivation from records; counts consistent.
ReliabilityResult checkInvariants(const SessionState& state);

// The single place official totals are derived (exposed for tests).
qint32 deriveTotalTenths(const SessionState& state);

} // namespace SessionReducer

} // namespace rel
} // namespace ta

#endif // TA_REL_SESSIONREDUCER_H
