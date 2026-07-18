#include "SessionReducer.h"

namespace ta {
namespace rel {

namespace {

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

ReduceResult rejected(const SessionState& current, ReliabilityError code,
                      const QString& detail, quint64 seq)
{
    ReduceResult r;
    r.state = current;
    r.ok = false;
    r.error = ReliabilityResult::failure(code,
        QStringLiteral("The event was rejected by the session reducer."),
        detail, QString(), static_cast<qint64>(seq)).error;
    return r;
}

// Recompute EVERY total from the authoritative records (spec §8: totals
// exist only here; no second accumulator anywhere).
void recomputeTotals(SessionState& s)
{
    qint32 total = 0;
    QMap<qint16, qint32> subtotals;
    for (const StateShotRecord& r : s.officials) {
        const qint32 t = r.effectiveTenths();
        total += t;
        subtotals[r.shot.stageId] += t;
    }
    for (const AdjustmentEntry& a : s.adjustments) {
        total += a.deltaTenths;
        if (a.stageId >= 0)
            subtotals[a.stageId] += a.deltaTenths;
    }
    s.totalTenths = total;
    s.stageSubtotalTenths = subtotals;
}

StateShotRecord* findOfficialBySeq(SessionState& s, quint64 targetSeq)
{
    for (StateShotRecord& r : s.officials)
        if (r.seq == targetSeq)
            return &r;
    return nullptr;
}

} // namespace

qint32 SessionReducer::deriveTotalTenths(const SessionState& state)
{
    qint32 total = 0;
    for (const StateShotRecord& r : state.officials)
        total += r.effectiveTenths();
    for (const AdjustmentEntry& a : state.adjustments)
        total += a.deltaTenths;
    return total;
}

ReliabilityResult SessionReducer::checkInvariants(const SessionState& s)
{
    if (s.totalTenths != deriveTotalTenths(s))
        return ReliabilityResult::failure(ReliabilityError::ReducerRejected,
            QStringLiteral("State invariant violated."),
            QStringLiteral("totalTenths %1 != derived %2")
                .arg(s.totalTenths).arg(deriveTotalTenths(s)));
    qint32 stageSum = 0;
    for (auto it = s.stageSubtotalTenths.constBegin();
         it != s.stageSubtotalTenths.constEnd(); ++it)
        stageSum += it.value();
    qint32 globalAdjust = 0;
    for (const AdjustmentEntry& a : s.adjustments)
        if (a.stageId < 0)
            globalAdjust += a.deltaTenths;
    if (stageSum + globalAdjust != s.totalTenths)
        return ReliabilityResult::failure(ReliabilityError::ReducerRejected,
            QStringLiteral("State invariant violated."),
            QStringLiteral("stage subtotals %1 + global %2 != total %3")
                .arg(stageSum).arg(globalAdjust).arg(s.totalTenths));
    for (const StateShotRecord& r : s.officials)
        if (r.shot.shotNumber < 1)
            return ReliabilityResult::failure(ReliabilityError::ReducerRejected,
                QStringLiteral("State invariant violated."),
                QStringLiteral("official record with shotNumber %1")
                    .arg(r.shot.shotNumber));
    for (const StateShotRecord& r : s.sighters)
        if (r.shot.shotNumber != 0)
            return ReliabilityResult::failure(ReliabilityError::ReducerRejected,
                QStringLiteral("State invariant violated."),
                QStringLiteral("sighter record with shotNumber %1")
                    .arg(r.shot.shotNumber));
    return ReliabilityResult::success();
}

ReduceResult SessionReducer::apply(const SessionState& current,
                                   const EventEnvelope& envelope)
{
    const quint64 seq = envelope.seq;

    // (2) payload validation — envelope-level checks are the journal
    // layer's job (spec §8 order of checks), payload sanity is ours.
    {
        const ReliabilityResult vr = validateEvent(envelope.payload);
        if (!vr.ok)
            return rejected(current, ReliabilityError::InvalidEvent,
                            vr.error.technicalDetail, seq);
    }

    // Defence-layer ordering: reducers only ever see the next sequence.
    if (!current.started) {
        if (seq != 0 && !std::holds_alternative<SessionStarted>(envelope.payload))
            return rejected(current, ReliabilityError::InvalidStateTransition,
                            QStringLiteral("event before SessionStarted"), seq);
    } else if (seq != current.lastSeq + 1) {
        return rejected(current, ReliabilityError::InvalidSequence,
                        QStringLiteral("seq %1, state expects %2")
                            .arg(seq).arg(current.lastSeq + 1), seq);
    }

    SessionState next = current;
    const Lifecycle life = current.lifecycle;
    const bool active = life == Lifecycle::Active;

    auto illegal = [&](const char* what) {
        return rejected(current, ReliabilityError::InvalidStateTransition,
                        QStringLiteral("%1 illegal in lifecycle %2 / phase %3")
                            .arg(QLatin1String(what))
                            .arg(static_cast<int>(life))
                            .arg(static_cast<int>(current.phase)),
                        seq);
    };

    // (3) legality + (4) apply, per event type.
    ReduceResult failure;
    failure.ok = true;   // sentinel: set by handlers on rejection

    std::visit(overloaded{
        [&](const SessionStarted& e) {
            if (current.started) {
                failure = illegal("SessionStarted");
                return;
            }
            next = SessionState();
            next.sessionId = e.sessionId;
            next.schemaVersion = e.schemaVersion;
            next.appVersion = e.appVersion;
            next.createdAtIso = e.createdAtIso;
            next.athlete = e.athlete;
            next.lane = e.lane;
            next.targetId = e.targetId;
            next.deviceId = e.deviceId;
            next.discipline = e.discipline;
            next.matchType = e.matchType;
            next.config = e.config;
            next.started = true;
            next.lifecycle = Lifecycle::Active;
            switch (e.discipline) {
            case Discipline::Finals3P:
                next.disc = Finals3PState{};
                break;
            case Discipline::Training:
                next.disc = TrainingState{};
                break;
            case Discipline::None:
                next.disc = std::monostate{};
                break;
            default:
                next.disc = QualificationState{};
                break;
            }
        },
        [&](const SessionConfigured& e) {
            if (!active) {
                failure = illegal("SessionConfigured");
                return;
            }
            next.discipline = e.discipline;
            next.matchType = e.matchType;
            next.config = e.config;
        },
        [&](const AthleteAssigned& e) {
            if (!active) {
                failure = illegal("AthleteAssigned");
                return;
            }
            next.athlete = e.athlete;
            if (!e.lane.isEmpty())
                next.lane = e.lane;
            if (!e.targetId.isEmpty())
                next.targetId = e.targetId;
        },
        [&](const PreparationStarted& e) {
            if (!active) {
                failure = illegal("PreparationStarted");
                return;
            }
            next.phase = MatchPhase::Preparation;
            next.currentStageId = e.stageId;
        },
        [&](const SightingStarted& e) {
            if (!active) {
                failure = illegal("SightingStarted");
                return;
            }
            next.phase = MatchPhase::Sighting;
            next.currentStageId = e.stageId;
        },
        [&](const OfficialMatchStarted& e) {
            if (!active) {
                failure = illegal("OfficialMatchStarted");
                return;
            }
            next.phase = MatchPhase::OfficialMatch;
            next.currentStageId = e.stageId;
        },
        [&](const ShotAccepted& e) {
            if (!active || current.phase != MatchPhase::OfficialMatch) {
                failure = illegal("ShotAccepted");
                return;
            }
            // duplicate shot identity: shot number, and hardware external
            // id (when the producer supplies one)
            for (const StateShotRecord& r : current.officials) {
                if (r.shot.shotNumber == e.shot.shotNumber) {
                    failure = rejected(current, ReliabilityError::ReducerRejected,
                        QStringLiteral("duplicate official shotNumber %1")
                            .arg(e.shot.shotNumber), seq);
                    return;
                }
                if (e.shot.externalId != 0
                    && r.shot.externalId == e.shot.externalId) {
                    failure = rejected(current, ReliabilityError::ReducerRejected,
                        QStringLiteral("duplicate externalId %1")
                            .arg(e.shot.externalId), seq);
                    return;
                }
            }
            StateShotRecord rec;
            rec.shot = e.shot;
            rec.seq = seq;
            next.officials.append(rec);
            if (auto* f = std::get_if<Finals3PState>(&next.disc)) {
                f->stageId = e.shot.stageId;
                f->windowId = e.shot.windowId;
                ++f->shotsInStage;
            }
            recomputeTotals(next);
        },
        [&](const SighterAccepted& e) {
            if (!active || current.phase == MatchPhase::None) {
                failure = illegal("SighterAccepted");
                return;
            }
            StateShotRecord rec;
            rec.shot = e.shot;
            rec.seq = seq;
            next.sighters.append(rec);
        },
        [&](const StageCompleted& e) {
            if (!active) {
                failure = illegal("StageCompleted");
                return;
            }
            next.stageStatuses.insert(e.stageId, e.status);
        },
        [&](const PositionChanged& e) {
            if (!active) {
                failure = illegal("PositionChanged");
                return;
            }
            next.positionIndex = e.positionIndex;
            if (auto* q = std::get_if<QualificationState>(&next.disc))
                q->positionIndex = e.positionIndex;
        },
        [&](const TimerStarted& e) {
            if (!active) {
                failure = illegal("TimerStarted");
                return;
            }
            next.timer = TimerState();
            next.timer.active = true;
            next.timer.timerId = e.timerId;
            next.timer.durationMs = e.durationMs;
            next.timer.startedAtMonoMs = envelope.monotonicMs;
        },
        [&](const TimerPaused& e) {
            if (!active || !current.timer.active || current.timer.paused) {
                failure = illegal("TimerPaused");
                return;
            }
            next.timer.paused = true;
            next.timer.pausedAtMonoMs = e.atMonoMs;
        },
        [&](const TimerResumed& e) {
            if (!active || !current.timer.active || !current.timer.paused) {
                failure = illegal("TimerResumed");
                return;
            }
            next.timer.paused = false;
            next.timer.pausedAccumMs +=
                e.atMonoMs - current.timer.pausedAtMonoMs;
        },
        [&](const TimerExpired&) {
            if (!active || !current.timer.active) {
                failure = illegal("TimerExpired");
                return;
            }
            next.timer.active = false;
            next.timer.paused = false;
        },
        [&](const MatchCompleted& e) {
            if (!active) {
                failure = illegal("MatchCompleted");
                return;
            }
            // The completion totals must agree with the reduced state —
            // a mismatch means a producer computed its own arithmetic.
            if (e.totalTenths != current.totalTenths
                || e.officialCount != current.officials.size()) {
                failure = rejected(current, ReliabilityError::ReducerRejected,
                    QStringLiteral("MatchCompleted totals %1/%2 != state %3/%4")
                        .arg(e.totalTenths).arg(e.officialCount)
                        .arg(current.totalTenths).arg(current.officials.size()),
                    seq);
                return;
            }
            next.lifecycle = Lifecycle::Complete;
        },
        [&](const SessionSuspended&) {
            if (!active) {
                failure = illegal("SessionSuspended");
                return;
            }
            next.lifecycle = Lifecycle::Suspended;
        },
        [&](const SessionResumed&) {
            if (life != Lifecycle::Suspended) {
                failure = illegal("SessionResumed");
                return;
            }
            next.lifecycle = Lifecycle::Active;
        },
        [&](const StateSnapshot& e) {
            if (!current.started) {
                failure = illegal("StateSnapshot");
                return;
            }
            // Integrity metadata must equal the folded state (drift
            // detector, spec §13).
            if (e.lastAppliedSeq != current.lastSeq
                || e.officialCount != current.officials.size()
                || e.sighterCount != current.sighters.size()
                || e.totalTenths != current.totalTenths) {
                failure = rejected(current, ReliabilityError::ReducerRejected,
                    QStringLiteral("snapshot metadata disagrees with folded "
                                   "state (seq %1/%2, officials %3/%4, "
                                   "sighters %5/%6, total %7/%8)")
                        .arg(e.lastAppliedSeq).arg(current.lastSeq)
                        .arg(e.officialCount).arg(current.officials.size())
                        .arg(e.sighterCount).arg(current.sighters.size())
                        .arg(e.totalTenths).arg(current.totalTenths),
                    seq);
                return;
            }
            SessionState captured;
            const ReliabilityResult dr =
                deserializeSessionState(e.stateJson, &captured);
            if (!dr.ok) {
                failure = rejected(current, dr.error.code,
                                   dr.error.technicalDetail, seq);
                return;
            }
            if (!(captured == current)) {
                failure = rejected(current, ReliabilityError::ReducerRejected,
                    QStringLiteral("snapshot state differs from folded state"),
                    seq);
                return;
            }
            // state unchanged; only lastSeq advances below
        },
        [&](const ShotInvalidated& e) {
            if (!active && life != Lifecycle::Complete) {
                failure = illegal("ShotInvalidated");
                return;
            }
            StateShotRecord* target = findOfficialBySeq(next, e.targetSeq);
            if (!target || target->invalidated) {
                failure = rejected(current, ReliabilityError::ReducerRejected,
                    target ? QStringLiteral("shot at seq %1 already invalidated")
                                 .arg(e.targetSeq)
                           : QStringLiteral("no official shot at seq %1")
                                 .arg(e.targetSeq),
                    seq);
                return;
            }
            CorrectionEntry c;
            c.targetSeq = e.targetSeq;
            c.type = QString::fromLatin1(ShotInvalidated::kType);
            c.reason = e.reason;
            c.fromTenths = static_cast<qint16>(target->effectiveTenths());
            c.toTenths = 0;
            target->invalidated = true;
            next.corrections.append(c);
            recomputeTotals(next);
        },
        [&](const ShotRescored& e) {
            if (!active && life != Lifecycle::Complete) {
                failure = illegal("ShotRescored");
                return;
            }
            StateShotRecord* target = findOfficialBySeq(next, e.targetSeq);
            if (!target) {
                failure = rejected(current, ReliabilityError::ReducerRejected,
                    QStringLiteral("no official shot at seq %1").arg(e.targetSeq),
                    seq);
                return;
            }
            CorrectionEntry c;
            c.targetSeq = e.targetSeq;
            c.type = QString::fromLatin1(ShotRescored::kType);
            c.reason = e.reason;
            c.fromTenths = static_cast<qint16>(target->effectiveTenths());
            c.toTenths = e.newScoreTenths;
            target->rescoredTenths = e.newScoreTenths;
            if (e.hasNewCoordinates) {
                target->shot.xHundredthMm = e.newXHundredthMm;
                target->shot.yHundredthMm = e.newYHundredthMm;
            }
            next.corrections.append(c);
            recomputeTotals(next);
        },
        [&](const SeriesAdjusted& e) {
            if (!active && life != Lifecycle::Complete) {
                failure = illegal("SeriesAdjusted");
                return;
            }
            AdjustmentEntry a;
            a.stageId = e.stageId;
            a.deltaTenths = e.deltaTenths;
            a.kind = QString::fromLatin1(SeriesAdjusted::kType);
            a.reason = e.reason;
            next.adjustments.append(a);
            recomputeTotals(next);
        },
        [&](const CrossShotRecorded& e) {
            if (!active) {
                failure = illegal("CrossShotRecorded");
                return;
            }
            CrossShotRec rec;
            rec.shot = e.shot;
            rec.seq = seq;
            rec.sourceLane = e.sourceLane;
            next.crossShots.append(rec);   // non-scoring by definition
        },
        [&](const EquipmentMalfunctionRecorded& e) {
            if (!active) {
                failure = illegal("EquipmentMalfunctionRecorded");
                return;
            }
            IncidentEntry i;
            i.kind = QStringLiteral("EquipmentMalfunction");
            i.note = e.note;
            i.allowedTimeMs = e.allowedTimeMs;
            i.seq = seq;
            next.incidents.append(i);
        },
        [&](const PenaltyIssued& e) {
            if (!active && life != Lifecycle::Complete) {
                failure = illegal("PenaltyIssued");
                return;
            }
            AdjustmentEntry a;
            a.stageId = -1;
            a.deltaTenths = e.deltaTenths;
            a.kind = QString::fromLatin1(PenaltyIssued::kType);
            a.reason = e.rule;
            next.adjustments.append(a);
            recomputeTotals(next);
        },
        [&](const RecoveryStarted&) {
            // Marker: idempotent no-op on the state (spec §15); only
            // lastSeq advances.
            if (!current.started)
                failure = illegal("RecoveryStarted");
        },
        [&](const RecoveryCompleted&) {
            if (!current.started) {
                failure = illegal("RecoveryCompleted");
                return;
            }
            next.lifecycle = Lifecycle::Active;
        },
        [&](const SessionClosed&) {
            if (life != Lifecycle::Active && life != Lifecycle::Complete
                && life != Lifecycle::Suspended) {
                failure = illegal("SessionClosed");
                return;
            }
            next.lifecycle = Lifecycle::Closed;
        },
        // ── M2 finals flow + persistence markers ─────────────────────
        [&](const StageEntered& e) {
            if (!active) {
                failure = illegal("StageEntered");
                return;
            }
            next.currentStageId = e.stageId;
            if (auto* f = std::get_if<Finals3PState>(&next.disc)) {
                f->stageId = e.stageId;
                f->shotsInStage = 0;
            }
        },
        [&](const StageStatusChanged& e) {
            if (!active) {
                failure = illegal("StageStatusChanged");
                return;
            }
            next.stageStatuses.insert(e.stageId, e.status);
        },
        [&](const TargetModeChanged&) {
            if (!active)
                failure = illegal("TargetModeChanged");
            // marker: target mode is carried per shot (ShotCore.targetMode)
        },
        [&](const WindowOpened& e) {
            if (!active) {
                failure = illegal("WindowOpened");
                return;
            }
            if (auto* f = std::get_if<Finals3PState>(&next.disc))
                f->windowId = e.windowId;
        },
        [&](const WindowClosed&) {
            if (!active)
                failure = illegal("WindowClosed");
            // marker: the id of the last window stays in Finals3PState
        },
        [&](const CommandIssued&) {
            if (!active)
                failure = illegal("CommandIssued");
            // marker: command history is journal-level evidence
        },
        [&](const ShotRejected& e) {
            if (!active) {
                failure = illegal("ShotRejected");
                return;
            }
            IncidentEntry i;
            i.kind = QStringLiteral("ShotRejected");
            i.note = e.reason;
            i.allowedTimeMs = -1;
            i.seq = seq;
            next.incidents.append(i);
        },
        [&](const MissingShotRecorded& e) {
            if (!active) {
                failure = illegal("MissingShotRecorded");
                return;
            }
            IncidentEntry i;
            i.kind = QStringLiteral("MissingShot");
            i.note = QStringLiteral("shot %1: %2")
                         .arg(e.expectedNumber).arg(e.reason);
            i.allowedTimeMs = -1;
            i.seq = seq;
            next.incidents.append(i);
        },
        [&](const PersistenceDegraded&) {
            if (!current.started)
                failure = illegal("PersistenceDegraded");
            // marker: health history lives in the journal
        },
        [&](const PersistenceRestored&) {
            if (!current.started)
                failure = illegal("PersistenceRestored");
        },
        [&](const AuxEventsDropped&) {
            if (!current.started)
                failure = illegal("AuxEventsDropped");
            // marker: recorded data loss (spec §9E) — journal evidence
        },
        [&](const CleanShutdown&) {
            if (!current.started)
                failure = illegal("CleanShutdown");
        }
    }, envelope.payload);

    if (!failure.ok)
        return failure;

    next.lastSeq = seq;

    // (5) post-reduction invariants — a violation here is a reducer bug,
    // surfaced as a typed failure so harnesses catch it immediately.
    const ReliabilityResult inv = checkInvariants(next);
    if (!inv.ok)
        return rejected(current, inv.error.code, inv.error.technicalDetail, seq);

    ReduceResult ok;
    ok.state = next;
    return ok;
}

} // namespace rel
} // namespace ta
