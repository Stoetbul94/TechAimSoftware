#ifndef TA_REL_EVENTREGISTRY_H
#define TA_REL_EVENTREGISTRY_H

// Session Reliability Layer — event registry (M1, step 9).
//
// One row per event type: stable type string, supported payload version,
// payload variant index, and the durability / broadcast / reducer
// classifications (spec §4/§12/§20). The registry is the single authority
// consulted before any payload is accepted:
//   - unknown type string        -> UnsupportedEventType (typed)
//   - pv newer than supported    -> UnsupportedEventVersion (typed)
// Serialization, deserialization and validation dispatch are centralized
// in EventSerializer / validateEvent (deliberately not per-row function
// pointers — one debuggable dispatcher instead of 27 indirections); the
// registry row tells callers WHICH alternative and versions apply.

#include "DomainEvent.h"

#include <QList>
#include <QString>

namespace ta {
namespace rel {

struct EventMeta {
    const char* typeId;              // stable string identifier
    qint32 payloadVersion;           // highest pv this build reads/writes
    int variantIndex;                // alternative index inside DomainEvent
    DurabilityClass durability;      // spec §12
    BroadcastClass broadcast;        // spec §4 "Bus→"
    ReducerClass reducerClass;       // Mutating / Marker / Reserved
};

namespace EventRegistry {

// nullptr when the type string is unknown.
const EventMeta* metaForType(const QString& typeId);

// Never nullptr — every DomainEvent alternative has a registry row
// (enforced by a test iterating the variant).
const EventMeta* metaForEvent(const DomainEvent& event);

// All rows, for tests and diagnostics.
const QList<const EventMeta*>& allTypes();

} // namespace EventRegistry

} // namespace rel
} // namespace ta

#endif // TA_REL_EVENTREGISTRY_H
