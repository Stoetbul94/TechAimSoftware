#include "EventRegistry.h"

#include <QHash>

#include <type_traits>

namespace ta {
namespace rel {

namespace {

// Compile-time alternative index of T inside DomainEvent.
template <typename T, std::size_t I = 0>
constexpr int variantIndexOf()
{
    static_assert(I < std::variant_size_v<DomainEvent>,
                  "type is not a DomainEvent alternative");
    if constexpr (std::is_same_v<std::variant_alternative_t<I, DomainEvent>, T>)
        return static_cast<int>(I);
    else
        return variantIndexOf<T, I + 1>();
}

template <typename T>
constexpr EventMeta row(DurabilityClass d, BroadcastClass b, ReducerClass rc)
{
    return EventMeta{T::kType, T::kVersion, variantIndexOf<T>(), d, b, rc};
}

// Durability per spec §12; broadcast per spec §4 "Bus→" column.
const EventMeta kRows[] = {
    // M1 active
    row<SessionStarted>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                        ReducerClass::Mutating),
    row<SessionConfigured>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                           ReducerClass::Mutating),
    row<AthleteAssigned>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                         ReducerClass::Mutating),
    row<PreparationStarted>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                            ReducerClass::Mutating),
    row<SightingStarted>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                         ReducerClass::Mutating),
    row<OfficialMatchStarted>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                              ReducerClass::Mutating),
    row<ShotAccepted>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                      ReducerClass::Mutating),
    row<SighterAccepted>(DurabilityClass::Flush, BroadcastClass::Broadcast,
                         ReducerClass::Mutating),          // spec D2
    row<StageCompleted>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                        ReducerClass::Mutating),
    row<PositionChanged>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                         ReducerClass::Mutating),
    row<TimerStarted>(DurabilityClass::Flush, BroadcastClass::Broadcast,
                      ReducerClass::Mutating),
    row<TimerPaused>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                     ReducerClass::Mutating),
    row<TimerResumed>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                      ReducerClass::Mutating),
    row<TimerExpired>(DurabilityClass::Flush, BroadcastClass::Broadcast,
                      ReducerClass::Mutating),
    row<MatchCompleted>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                        ReducerClass::Mutating),
    row<SessionSuspended>(DurabilityClass::Sync, BroadcastClass::Internal,
                          ReducerClass::Mutating),
    row<SessionResumed>(DurabilityClass::Flush, BroadcastClass::Internal,
                        ReducerClass::Mutating),
    row<StateSnapshot>(DurabilityClass::Sync, BroadcastClass::Internal,
                       ReducerClass::Mutating),
    // reserved (typed now; producers arrive M2+)
    row<ShotInvalidated>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                         ReducerClass::Mutating),
    row<ShotRescored>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                      ReducerClass::Mutating),
    row<SeriesAdjusted>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                        ReducerClass::Mutating),
    row<CrossShotRecorded>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                           ReducerClass::Mutating),
    row<EquipmentMalfunctionRecorded>(DurabilityClass::Sync,
                                      BroadcastClass::Broadcast,
                                      ReducerClass::Mutating),
    row<PenaltyIssued>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                       ReducerClass::Mutating),
    row<RecoveryStarted>(DurabilityClass::Sync, BroadcastClass::Internal,
                         ReducerClass::Marker),
    row<RecoveryCompleted>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                           ReducerClass::Mutating),
    row<SessionClosed>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                       ReducerClass::Mutating),
    // M2 — finals flow + persistence markers (durability per spec §12)
    row<StageEntered>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                      ReducerClass::Mutating),
    row<StageStatusChanged>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                            ReducerClass::Mutating),
    row<TargetModeChanged>(DurabilityClass::Flush, BroadcastClass::Broadcast,
                           ReducerClass::Marker),
    row<WindowOpened>(DurabilityClass::Flush, BroadcastClass::Internal,
                      ReducerClass::Mutating),
    row<WindowClosed>(DurabilityClass::Flush, BroadcastClass::Internal,
                      ReducerClass::Marker),
    row<CommandIssued>(DurabilityClass::Flush, BroadcastClass::Broadcast,
                       ReducerClass::Marker),
    row<ShotRejected>(DurabilityClass::Append, BroadcastClass::Internal,
                      ReducerClass::Mutating),
    row<MissingShotRecorded>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                             ReducerClass::Mutating),
    row<PersistenceDegraded>(DurabilityClass::Sync, BroadcastClass::Internal,
                             ReducerClass::Marker),
    row<PersistenceRestored>(DurabilityClass::Flush, BroadcastClass::Internal,
                             ReducerClass::Marker),
    row<AuxEventsDropped>(DurabilityClass::Sync, BroadcastClass::Internal,
                          ReducerClass::Marker),
    row<CleanShutdown>(DurabilityClass::Sync, BroadcastClass::Internal,
                       ReducerClass::Marker),
    // M3 Phase A — generic EST incident + Jury-decision events. Durability
    // Sync: an incident/adjustment must be on disk before acting on it.
    row<EstIncidentRaised>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                           ReducerClass::Mutating),
    row<TimeCreditGranted>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                           ReducerClass::Mutating),
    row<RecoveryPhaseEntered>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                              ReducerClass::Mutating),
    row<EstIncidentResolved>(DurabilityClass::Sync, BroadcastClass::Broadcast,
                             ReducerClass::Mutating),
};

struct RegistryIndex {
    QHash<QString, const EventMeta*> byType;
    QList<const EventMeta*> all;
    RegistryIndex()
    {
        for (const EventMeta& m : kRows) {
            byType.insert(QString::fromLatin1(m.typeId), &m);
            all.append(&m);
        }
    }
};

const RegistryIndex& index()
{
    static const RegistryIndex idx;
    return idx;
}

} // namespace

const EventMeta* EventRegistry::metaForType(const QString& typeId)
{
    return index().byType.value(typeId, nullptr);
}

const EventMeta* EventRegistry::metaForEvent(const DomainEvent& event)
{
    const int wanted = static_cast<int>(event.index());
    for (const EventMeta* m : index().all)
        if (m->variantIndex == wanted)
            return m;
    return nullptr;   // unreachable if the table is complete (tested)
}

const QList<const EventMeta*>& EventRegistry::allTypes()
{
    return index().all;
}

} // namespace rel
} // namespace ta
