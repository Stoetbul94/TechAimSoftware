#include "SessionState.h"

#include "reliability/events/EventSerializer.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <cstdint>
#include <limits>

namespace ta {
namespace rel {

// ── equality ──────────────────────────────────────────────────────────

bool SessionState::operator==(const SessionState& o) const
{
    return sessionId == o.sessionId && schemaVersion == o.schemaVersion
        && appVersion == o.appVersion && createdAtIso == o.createdAtIso
        && athlete == o.athlete && lane == o.lane && targetId == o.targetId
        && deviceId == o.deviceId && discipline == o.discipline
        && matchType == o.matchType && config == o.config
        && started == o.started && lifecycle == o.lifecycle && phase == o.phase
        && currentStageId == o.currentStageId && positionIndex == o.positionIndex
        && officials == o.officials && sighters == o.sighters
        && crossShots == o.crossShots && corrections == o.corrections
        && adjustments == o.adjustments && incidents == o.incidents
        && estIncidents == o.estIncidents
        && totalTenths == o.totalTenths
        && stageSubtotalTenths == o.stageSubtotalTenths
        && stageStatuses == o.stageStatuses && timer == o.timer
        && lastSeq == o.lastSeq && disc == o.disc
        // Wind Map (state v4). Included precisely BECAUSE it is
        // snapshot-serialised: this is what makes snapshotsAgreeWithFold a
        // real check on the Wind Map projection rather than a no-op.
        && wmActive == o.wmActive && wmCompleted == o.wmCompleted
        && wmProgramId == o.wmProgramId && wmDisciplineId == o.wmDisciplineId
        && wmThreePositions == o.wmThreePositions
        && wmCurrentPosition == o.wmCurrentPosition
        && wmPositionSequence == o.wmPositionSequence
        && wmPhase == o.wmPhase                    // state v6
        && wmConditionChanges == o.wmConditionChanges
        && wmNextShotId == o.wmNextShotId
        && wmWindValid == o.wmWindValid && wmWindCalm == o.wmWindCalm
        && wmWindDirectionDegrees == o.wmWindDirectionDegrees
        && wmWindSpeedHundredthMs == o.wmWindSpeedHundredthMs
        && wmWindSource == o.wmWindSource
        && wmWindRecordedMs == o.wmWindRecordedMs
        && wmWindNote == o.wmWindNote
        && wmShots == o.wmShots
        // Training Lab programmes (state v5). Same reasoning as Wind Map:
        // these are snapshot-serialised, so comparing them here is what makes
        // ReplayEngine::snapshotsAgreeWithFold a real check on them.
        && sessionKind == o.sessionKind
        // Rule authority is snapshot-serialised, so comparing it here is what
        // makes snapshotsAgreeWithFold a real check on it too.
        && ruleAuthority == o.ruleAuthority
        && trainingActive == o.trainingActive
        && trainingCompleted == o.trainingCompleted
        && trainingProgramId == o.trainingProgramId
        && trainingBlockCount == o.trainingBlockCount
        && trainingShotsPerBlock == o.trainingShotsPerBlock
        && trainingVisibility == o.trainingVisibility
        && trainingFocus == o.trainingFocus
        && trainingCurrentBlock == o.trainingCurrentBlock
        && trainingCurrentPosition == o.trainingCurrentPosition
        && trainingBlocks == o.trainingBlocks
        && trainingInSighterPhase == o.trainingInSighterPhase
        && trainingSighterPosition == o.trainingSighterPosition
        && trainingSighterBeforeBlock == o.trainingSighterBeforeBlock
        && trainingSighters == o.trainingSighters
        && trainingSighterPos == o.trainingSighterPos
        && cdActive == o.cdActive && cdCompleted == o.cdCompleted
        && cdCallingActive == o.cdCallingActive
        && cdProgramId == o.cdProgramId && cdFocus == o.cdFocus
        && cdShotCount == o.cdShotCount
        && cdCurrentPosition == o.cdCurrentPosition
        && cdThreePositions == o.cdThreePositions
        && cdSessionNote == o.cdSessionNote
        && cdShots == o.cdShots
        && ptActive == o.ptActive && ptCompleted == o.ptCompleted
        && ptProgramId == o.ptProgramId && ptSequence == o.ptSequence
        && ptFocus == o.ptFocus
        && ptVerificationShots == o.ptVerificationShots
        && ptRepeats == o.ptRepeats && ptChecklistMode == o.ptChecklistMode
        && ptCurrentPosition == o.ptCurrentPosition
        && ptCurrentRepeat == o.ptCurrentRepeat
        && ptInSetup == o.ptInSetup && ptVerifying == o.ptVerifying
        && ptSessionNote == o.ptSessionNote
        && ptRecords == o.ptRecords;
}

// ── serialization ─────────────────────────────────────────────────────

namespace {

void writeShotRecord(OrderedJsonWriter& w, const StateShotRecord& r)
{
    w.beginObject();
    EventSerializer::serializeShotCoreFields(r.shot, w);
    w.fieldU("seq", r.seq);
    w.field("invalidated", r.invalidated);
    w.field("rescoredTenths", static_cast<qint64>(r.rescoredTenths));
    w.endObject();
}

const char* discKindName(const DisciplineState& d)
{
    if (std::holds_alternative<QualificationState>(d)) return "qualification";
    if (std::holds_alternative<Finals3PState>(d))      return "finals3p";
    if (std::holds_alternative<TrainingState>(d))      return "training";
    if (std::holds_alternative<Finals10mState>(d))     return "finals10m";
    return "none";
}

} // namespace

QByteArray serializeSessionState(const SessionState& s)
{
    OrderedJsonWriter w;
    w.beginObject();
    w.field("stateVersion", static_cast<qint64>(kSessionStateVersion));
    w.field("sessionId", s.sessionId);
    w.field("schemaVersion", static_cast<qint64>(s.schemaVersion));
    w.field("appVersion", s.appVersion);
    w.field("createdAtIso", s.createdAtIso);
    w.field("athlete", s.athlete);
    w.field("lane", s.lane);
    w.field("targetId", s.targetId);
    w.field("deviceId", s.deviceId);
    w.field("discipline", QString::fromLatin1(disciplineId(s.discipline)));
    w.field("matchType", s.matchType);
    w.beginObjectField("config");
    EventSerializer::serializeDisciplineConfigFields(s.config, w);
    w.endObject();
    w.field("started", s.started);
    w.field("lifecycle", static_cast<qint64>(s.lifecycle));
    w.field("phase", static_cast<qint64>(s.phase));
    w.field("currentStageId", static_cast<qint64>(s.currentStageId));
    w.field("positionIndex", static_cast<qint64>(s.positionIndex));

    w.beginArrayField("officials");
    for (const StateShotRecord& r : s.officials)
        writeShotRecord(w, r);
    w.endArray();
    w.beginArrayField("sighters");
    for (const StateShotRecord& r : s.sighters)
        writeShotRecord(w, r);
    w.endArray();
    w.beginArrayField("crossShots");
    for (const CrossShotRec& r : s.crossShots) {
        w.beginObject();
        EventSerializer::serializeShotCoreFields(r.shot, w);
        w.fieldU("seq", r.seq);
        w.field("sourceLane", r.sourceLane);
        w.endObject();
    }
    w.endArray();
    w.beginArrayField("corrections");
    for (const CorrectionEntry& c : s.corrections) {
        w.beginObject();
        w.fieldU("targetSeq", c.targetSeq);
        w.field("type", c.type);
        w.field("reason", c.reason);
        w.field("fromTenths", static_cast<qint64>(c.fromTenths));
        w.field("toTenths", static_cast<qint64>(c.toTenths));
        w.endObject();
    }
    w.endArray();
    w.beginArrayField("adjustments");
    for (const AdjustmentEntry& a : s.adjustments) {
        w.beginObject();
        w.field("stageId", static_cast<qint64>(a.stageId));
        w.field("deltaTenths", static_cast<qint64>(a.deltaTenths));
        w.field("kind", a.kind);
        w.field("reason", a.reason);
        w.endObject();
    }
    w.endArray();
    w.beginArrayField("incidents");
    for (const IncidentEntry& i : s.incidents) {
        w.beginObject();
        w.field("kind", i.kind);
        w.field("note", i.note);
        w.field("allowedTimeMs", i.allowedTimeMs);
        w.fieldU("seq", i.seq);
        w.endObject();
    }
    w.endArray();
    // estIncidents (state v2). Field order frozen; all fields always written.
    w.beginArrayField("estIncidents");
    for (const EstIncidentRecord& i : s.estIncidents) {
        w.beginObject();
        w.field("incidentId", i.incidentId);
        w.field("incidentType", static_cast<qint64>(i.incidentType));
        w.field("scope", static_cast<qint64>(i.scope));
        w.field("firingPoint", i.firingPoint);
        w.field("relayId", i.relayId);
        w.field("interruptionStartUtc", i.interruptionStartUtc);
        w.field("systemRestoredUtc", i.systemRestoredUtc);
        w.field("calculatedDurationMs", i.calculatedDurationMs);
        w.field("officiallyAcceptedDurationMs", i.officiallyAcceptedDurationMs);
        w.field("targetMoved", i.targetMoved);
        w.field("originalTarget", i.originalTarget);
        w.field("reserveTarget", i.reserveTarget);
        w.field("backupScoreReviewed", i.backupScoreReviewed);
        w.field("timeCreditMs", i.timeCreditMs);
        w.field("preparationGranted", i.preparationGranted);
        w.field("sightingGranted", i.sightingGranted);
        w.field("officialResumeAuthorised", i.officialResumeAuthorised);
        w.field("authorisedBy", i.authorisedBy);
        w.field("juryNote", i.juryNote);
        w.field("rangeOfficerNote", i.rangeOfficerNote);
        w.field("incidentReportRef", i.incidentReportRef);
        w.field("status", static_cast<qint64>(i.status));
        w.field("reason", i.reason);
        w.fieldU("raisedSeq", i.raisedSeq);
        // state v3 (Phase E)
        w.field("creditDecision", static_cast<qint64>(i.creditDecision));
        w.field("backupReview", static_cast<qint64>(i.backupReview));
        w.field("raisedAtMonoMs", i.raisedAtMonoMs);
        w.fieldU("sightingGrantedSeq", i.sightingGrantedSeq);
        w.endObject();
    }
    w.endArray();

    w.field("totalTenths", static_cast<qint64>(s.totalTenths));
    // QMap iterates in key order — deterministic by construction.
    w.beginArrayField("stageSubtotals");
    for (auto it = s.stageSubtotalTenths.constBegin();
         it != s.stageSubtotalTenths.constEnd(); ++it) {
        w.beginObject();
        w.field("stageId", static_cast<qint64>(it.key()));
        w.field("tenths", static_cast<qint64>(it.value()));
        w.endObject();
    }
    w.endArray();
    w.beginArrayField("stageStatuses");
    for (auto it = s.stageStatuses.constBegin();
         it != s.stageStatuses.constEnd(); ++it) {
        w.beginObject();
        w.field("stageId", static_cast<qint64>(it.key()));
        w.field("status", static_cast<qint64>(it.value()));
        w.endObject();
    }
    w.endArray();

    // ── Training Lab programmes (state v5) ───────────────────────────────
    // Technical Blocks, Call & Diagnose and Position Transition were NOT
    // snapshot-serialised before this. They were safe only because nothing in
    // production emits a StateSnapshot, and ReplayEngine folds just the tail
    // after one — so enabling periodic snapshots would have silently truncated
    // all three at the boundary. Same fix as Wind Map: persist the projection,
    // and compare it in operator== so snapshotsAgreeWithFold is a real check.
    //
    // sessionKind is serialised here too. Without it a recovered Training
    // session reads as a competition session, which is worse than losing the
    // projection: RecoveryCoordinator classifies on this field.
    w.field("sessionKind", s.sessionKind);
    // Adopted rule authority (state v7). Written only when present, so a
    // legacy session snapshot keeps exactly the bytes it always had.
    if (s.ruleAuthority.isPresent()) {
        w.beginObjectField("ruleAuthority");
        w.field("authorityVersion", static_cast<qint64>(s.ruleAuthority.authorityVersion));
        w.field("programmeId", s.ruleAuthority.programmeId);
        w.field("rulesetId", s.ruleAuthority.rulesetId);
        w.field("rulesetVersion", s.ruleAuthority.rulesetVersion);
        w.field("ruleNumber", s.ruleAuthority.ruleNumber);
        w.field("programmeVariant", s.ruleAuthority.programmeVariant);
        w.field("competitionContext", s.ruleAuthority.competitionContext);
        w.field("scoringMode", s.ruleAuthority.scoringMode);
        w.field("timingModel", s.ruleAuthority.timingModel);
        w.field("targetStandardId", s.ruleAuthority.targetStandardId);
        w.field("disciplineId", s.ruleAuthority.disciplineId);
        w.field("distanceM", static_cast<qint64>(s.ruleAuthority.distanceM));
        w.field("preparationMs", s.ruleAuthority.preparationMs);
        w.field("matchMs", s.ruleAuthority.matchMs);
        w.field("positionSequence", s.ruleAuthority.positionSequence);
        w.field("positionDurationsMs", s.ruleAuthority.positionDurationsMs);
        w.endObject();
    }

    w.beginObjectField("training");
    w.field("active", s.trainingActive);
    w.field("completed", s.trainingCompleted);
    w.field("programId", s.trainingProgramId);
    w.field("blockCount", static_cast<qint64>(s.trainingBlockCount));
    w.field("shotsPerBlock", static_cast<qint64>(s.trainingShotsPerBlock));
    w.field("visibility", static_cast<qint64>(s.trainingVisibility));
    w.field("focus", s.trainingFocus);
    w.field("currentBlock", static_cast<qint64>(s.trainingCurrentBlock));
    w.field("currentPosition", static_cast<qint64>(s.trainingCurrentPosition));
    // Sighter phase state. Sighters are kept completely separate from counted
    // blocks, and that separation has to survive recovery intact.
    w.field("inSighterPhase", s.trainingInSighterPhase);
    w.field("sighterPosition", static_cast<qint64>(s.trainingSighterPosition));
    w.field("sighterBeforeBlock", static_cast<qint64>(s.trainingSighterBeforeBlock));
    w.endObject();

    w.beginArrayField("trainingBlocks");
    for (const TrainingBlockData& b : s.trainingBlocks) {
        w.beginObject();
        w.field("blockIndex", static_cast<qint64>(b.blockIndex));
        w.field("position", static_cast<qint64>(b.position));
        w.field("completed", b.completed);
        w.field("note", b.note);
        w.beginArrayField("shots");
        for (const ShotCore& sh : b.shots) {
            w.beginObject();
            EventSerializer::serializeShotCoreFields(sh, w);
            w.endObject();
        }
        w.endArray();
        w.endObject();
    }
    w.endArray();

    // Sighters are written as ONE array of {shot, position} objects rather
    // than the in-memory parallel vectors, so the two can never desync across
    // a snapshot boundary.
    w.beginArrayField("trainingSighters");
    for (int i = 0; i < s.trainingSighters.size(); ++i) {
        w.beginObject();
        EventSerializer::serializeShotCoreFields(s.trainingSighters[i], w);
        w.field("position", static_cast<qint64>(
            i < s.trainingSighterPos.size() ? s.trainingSighterPos[i] : qint8(0)));
        w.endObject();
    }
    w.endArray();

    w.beginObjectField("callDiagnose");
    w.field("active", s.cdActive);
    w.field("completed", s.cdCompleted);
    // The phase flag. A session interrupted in the sighter phase must not
    // resume in the calling phase, or vice versa.
    w.field("callingActive", s.cdCallingActive);
    w.field("programId", s.cdProgramId);
    w.field("focus", s.cdFocus);
    w.field("shotCount", static_cast<qint64>(s.cdShotCount));
    w.field("currentPosition", static_cast<qint64>(s.cdCurrentPosition));
    w.field("threePositions", s.cdThreePositions);
    w.field("sessionNote", s.cdSessionNote);
    w.endObject();

    // Each record carries the ACTUAL shot and, separately, whether a call has
    // been made for it. hasCall=false is the "awaiting the athlete's call"
    // state: the actual shot is already recorded but must not be revealed or
    // discarded on recovery.
    w.beginArrayField("cdShots");
    for (const CallDiagnoseShotRecord& c : s.cdShots) {
        w.beginObject();
        EventSerializer::serializeShotCoreFields(c.actual, w);
        w.field("shotNumber", static_cast<qint64>(c.shotNumber));
        w.field("position", static_cast<qint64>(c.position));
        w.field("hasCall", c.hasCall);
        w.field("calledXHundredthMm", static_cast<qint64>(c.calledXHundredthMm));
        w.field("calledYHundredthMm", static_cast<qint64>(c.calledYHundredthMm));
        w.field("callSplitMs", static_cast<qint64>(c.callSplitMs));
        w.field("note", c.note);
        w.endObject();
    }
    w.endArray();

    w.beginObjectField("positionTransition");
    w.field("active", s.ptActive);
    w.field("completed", s.ptCompleted);
    w.field("programId", s.ptProgramId);
    w.field("sequence", s.ptSequence);
    w.field("focus", s.ptFocus);
    w.field("verificationShots", static_cast<qint64>(s.ptVerificationShots));
    w.field("repeats", static_cast<qint64>(s.ptRepeats));
    w.field("checklistMode", static_cast<qint64>(s.ptChecklistMode));
    w.field("currentPosition", static_cast<qint64>(s.ptCurrentPosition));
    w.field("currentRepeat", static_cast<qint64>(s.ptCurrentRepeat));
    // inSetup / verifying are what keep setup, sighters and counted
    // verification shots distinguishable after a crash.
    w.field("inSetup", s.ptInSetup);
    w.field("verifying", s.ptVerifying);
    w.field("sessionNote", s.ptSessionNote);
    w.endObject();

    w.beginArrayField("ptRecords");
    for (const PtPositionRecord& p : s.ptRecords) {
        w.beginObject();
        w.field("position", static_cast<qint64>(p.position));
        w.field("repeat", static_cast<qint64>(p.repeat));
        // Setup timing and the ready stamp are the rhythm/cadence inputs; they
        // are recorded values, never re-derived, so they must persist.
        w.field("setupDurationMs", static_cast<qint64>(p.setupDurationMs));
        w.field("readyMonoMs", static_cast<qint64>(p.readyMonoMs));
        w.field("note", p.note);
        w.field("completed", p.completed);
        w.beginArrayField("sighters");
        for (const ShotCore& sh : p.sighters) {
            w.beginObject();
            EventSerializer::serializeShotCoreFields(sh, w);
            w.endObject();
        }
        w.endArray();
        w.beginArrayField("verifShots");
        for (const ShotCore& sh : p.verifShots) {
            w.beginObject();
            EventSerializer::serializeShotCoreFields(sh, w);
            w.endObject();
        }
        w.endArray();
        w.beginArrayField("checklist");
        for (qint8 c : p.checklist) {
            w.beginObject();
            w.field("v", static_cast<qint64>(c));
            w.endObject();
        }
        w.endArray();
        w.endObject();
    }
    w.endArray();

    // ── Wind Map (state v4) ──────────────────────────────────────────────
    // Wind Map projections ARE snapshot-serialised, unlike the other Training
    // programmes. The reason is concrete: ReplayEngine::replay defaults to the
    // snapshot fast path and folds only the tail after the last StateSnapshot,
    // so anything absent from the snapshot is silently lost at that boundary.
    // Relying on "no production code emits snapshots today" would be relying
    // on an undocumented accident. Field order frozen; all fields always
    // written so the bytes are deterministic.
    w.beginObjectField("windMap");
    w.field("active", s.wmActive);
    w.field("completed", s.wmCompleted);
    w.field("programId", s.wmProgramId);
    w.field("disciplineId", s.wmDisciplineId);
    w.field("threePositions", s.wmThreePositions);
    w.field("currentPosition", static_cast<qint64>(s.wmCurrentPosition));
    w.field("positionSequence", s.wmPositionSequence);
    // v6: the durable workflow phase. Appended after positionSequence, so the
    // preceding field order (and therefore every earlier key's bytes) is
    // unchanged; only the new key is added.
    w.field("phase", static_cast<qint64>(s.wmPhase));
    w.field("conditionChanges", static_cast<qint64>(s.wmConditionChanges));
    w.field("nextShotId", static_cast<qint64>(s.wmNextShotId));
    // The STANDING condition. windValid=false is a real recorded state
    // ("No wind reading recorded") and must survive as itself.
    w.field("windValid", s.wmWindValid);
    w.field("windCalm", s.wmWindCalm);
    w.field("windDirDeg", static_cast<qint64>(s.wmWindDirectionDegrees));
    w.field("windSpeedHundredthMs", static_cast<qint64>(s.wmWindSpeedHundredthMs));
    w.field("windSource", static_cast<qint64>(s.wmWindSource));
    w.field("windRecordedMs", s.wmWindRecordedMs);
    w.field("windNote", s.wmWindNote);
    w.endObject();

    // Every recorded shot with ITS OWN immutable snapshot. Sighter/counted
    // classification travels with the record so the two never merge.
    w.beginArrayField("windMapShots");
    for (const WindMapShotRecord& r : s.wmShots) {
        w.beginObject();
        EventSerializer::serializeShotCoreFields(r.shot, w);
        w.field("shotId", static_cast<qint64>(r.shotId));
        w.field("position", static_cast<qint64>(r.position));
        w.field("sighter", r.sighter);
        w.field("windValid", r.windValid);
        w.field("windCalm", r.windCalm);
        w.field("windDirDeg", static_cast<qint64>(r.windDirectionDegrees));
        w.field("windSpeedHundredthMs", static_cast<qint64>(r.windSpeedHundredthMs));
        w.field("windSource", static_cast<qint64>(r.windSource));
        w.field("windRecordedMs", r.windRecordedMs);
        w.field("windNote", r.windNote);
        w.endObject();
    }
    w.endArray();

    w.beginObjectField("timer");
    w.field("active", s.timer.active);
    w.field("timerId", static_cast<qint64>(s.timer.timerId));
    w.field("durationMs", s.timer.durationMs);
    w.field("startedAtMonoMs", s.timer.startedAtMonoMs);
    w.field("paused", s.timer.paused);
    w.field("pausedAtMonoMs", s.timer.pausedAtMonoMs);
    w.field("pausedAccumMs", s.timer.pausedAccumMs);
    w.endObject();

    w.fieldU("lastSeq", s.lastSeq);

    w.beginObjectField("disc");
    w.field("kind", QString::fromLatin1(discKindName(s.disc)));
    if (const auto* q = std::get_if<QualificationState>(&s.disc)) {
        w.field("positionIndex", static_cast<qint64>(q->positionIndex));
        w.field("sighterMode", q->sighterMode);
        w.field("version", static_cast<qint64>(q->version));
    } else if (const auto* f = std::get_if<Finals3PState>(&s.disc)) {
        w.field("stageId", static_cast<qint64>(f->stageId));
        w.field("windowId", static_cast<qint64>(f->windowId));
        w.field("shotsInStage", static_cast<qint64>(f->shotsInStage));
        w.field("version", static_cast<qint64>(f->version));
    } else if (const auto* t = std::get_if<TrainingState>(&s.disc)) {
        w.field("version", static_cast<qint64>(t->version));
    } else if (const auto* f10 = std::get_if<Finals10mState>(&s.disc)) {
        w.field("stageId", static_cast<qint64>(f10->stageId));
        w.field("windowId", static_cast<qint64>(f10->windowId));
        w.field("shotsInStage", static_cast<qint64>(f10->shotsInStage));
        w.field("version", static_cast<qint64>(f10->version));
    }
    w.endObject();

    w.endObject();
    return w.take();
}

// ── deserialization ───────────────────────────────────────────────────

namespace {

// Local typed field reader (mirrors the serializer's; state parsing only).
struct StateReader {
    const QJsonObject& o;
    ErrorInfo err{};
    bool failed = false;

    void fail(ReliabilityError code, const QString& detail)
    {
        if (failed)
            return;
        failed = true;
        err = ReliabilityResult::failure(code,
            QStringLiteral("Malformed state snapshot."), detail).error;
    }
    qint64 reqInt(const char* key, qint64 min, qint64 max)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined()) {
            fail(ReliabilityError::MissingField,
                 QStringLiteral("missing '%1'").arg(QLatin1String(key)));
            return 0;
        }
        if (!v.isDouble()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("'%1' not a number").arg(QLatin1String(key)));
            return 0;
        }
        const qint64 i = v.toInteger();
        if (i < min || i > max) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("'%1' out of range").arg(QLatin1String(key)));
            return 0;
        }
        return i;
    }
    QString reqString(const char* key)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined()) {
            fail(ReliabilityError::MissingField,
                 QStringLiteral("missing '%1'").arg(QLatin1String(key)));
            return QString();
        }
        if (!v.isString()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("'%1' not a string").arg(QLatin1String(key)));
            return QString();
        }
        return v.toString();
    }
    bool reqBool(const char* key)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (!v.isBool()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("'%1' not a bool").arg(QLatin1String(key)));
            return false;
        }
        return v.toBool();
    }
    QJsonObject reqObject(const char* key)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (!v.isObject()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("'%1' not an object").arg(QLatin1String(key)));
            return QJsonObject();
        }
        return v.toObject();
    }
    QJsonArray reqArray(const char* key)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (!v.isArray()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("'%1' not an array").arg(QLatin1String(key)));
            return QJsonArray();
        }
        return v.toArray();
    }
    // Optional array: absent key -> empty (backward compat for state v1 that
    // predates the field); present-but-not-array -> typed failure.
    QJsonArray optArray(const char* key)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined())
            return QJsonArray();
        if (!v.isArray()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("'%1' not an array").arg(QLatin1String(key)));
            return QJsonArray();
        }
        return v.toArray();
    }
    // Optional integer: absent key -> `def` (backward compat for fields added
    // in later state versions); present-but-wrong -> typed failure.
    qint64 optIntDef(const char* key, qint64 def, qint64 min, qint64 max)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined())
            return def;
        return reqInt(key, min, max);
    }
    // Same contract as optIntDef for the other two scalar kinds — needed by
    // state v4 (Wind Map), where an older snapshot simply has no such key.
    bool optBoolDef(const char* key, bool def)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined())
            return def;
        if (!v.isBool()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("'%1' not a bool").arg(QLatin1String(key)));
            return def;
        }
        return v.toBool();
    }
    QString optStringDef(const char* key, const QString& def = QString())
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined())
            return def;
        if (!v.isString()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("'%1' not a string").arg(QLatin1String(key)));
            return def;
        }
        return v.toString();
    }
};

bool readShotRecord(const QJsonValue& v, StateShotRecord* out, StateReader& top)
{
    if (!v.isObject()) {
        top.fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("shot record is not an object"));
        return false;
    }
    const QJsonObject obj = v.toObject();
    const ReliabilityResult sr = EventSerializer::deserializeShotCore(obj, &out->shot);
    if (!sr.ok) {
        top.failed = true;
        top.err = sr.error;
        return false;
    }
    StateReader r{obj};
    out->seq = static_cast<quint64>(
        r.reqInt("seq", 0, std::numeric_limits<qint64>::max()));
    out->invalidated = r.reqBool("invalidated");
    out->rescoredTenths = static_cast<qint16>(
        r.reqInt("rescoredTenths", INT16_MIN, INT16_MAX));
    if (r.failed) {
        top.failed = true;
        top.err = r.err;
        return false;
    }
    return true;
}

} // namespace

ReliabilityResult deserializeSessionState(const QByteArray& json, SessionState* out)
{
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &pe);
    if (doc.isNull() || !doc.isObject())
        return ReliabilityResult::failure(ReliabilityError::InvalidJson,
            QStringLiteral("Malformed state snapshot."),
            doc.isNull() ? pe.errorString()
                         : QStringLiteral("state is not a JSON object"));
    const QJsonObject obj = doc.object();
    StateReader r{obj};

    SessionState s;
    const qint32 stateVersion =
        static_cast<qint32>(r.reqInt("stateVersion", 1, INT32_MAX));
    if (!r.failed && stateVersion > kSessionStateVersion)
        return ReliabilityResult::failure(ReliabilityError::SchemaTooNew,
            QStringLiteral("Snapshot written by a newer version of this application."),
            QStringLiteral("stateVersion %1 > supported %2")
                .arg(stateVersion).arg(kSessionStateVersion));
    s.sessionId = r.reqString("sessionId");
    s.schemaVersion = static_cast<qint32>(r.reqInt("schemaVersion", 1, INT32_MAX));
    s.appVersion = r.reqString("appVersion");
    s.createdAtIso = r.reqString("createdAtIso");
    s.athlete = r.reqString("athlete");
    s.lane = r.reqString("lane");
    s.targetId = r.reqString("targetId");
    s.deviceId = r.reqString("deviceId");
    {
        Discipline d = Discipline::None;
        const QString id = r.reqString("discipline");
        if (!r.failed && !disciplineFromId(id, &d))
            r.fail(ReliabilityError::InvalidFieldType,
                   QStringLiteral("unknown discipline '%1'").arg(id));
        s.discipline = d;
    }
    s.matchType = r.reqString("matchType");
    {
        const QJsonObject cfg = r.reqObject("config");
        if (!r.failed) {
            const ReliabilityResult cr =
                EventSerializer::deserializeDisciplineConfig(cfg, &s.config);
            if (!cr.ok) {
                r.failed = true;
                r.err = cr.error;
            }
        }
    }
    s.started = r.reqBool("started");
    s.lifecycle = static_cast<Lifecycle>(r.reqInt("lifecycle", 0, 4));
    s.phase = static_cast<MatchPhase>(r.reqInt("phase", 0, 3));
    s.currentStageId = static_cast<qint16>(
        r.reqInt("currentStageId", INT16_MIN, INT16_MAX));
    s.positionIndex = static_cast<qint8>(r.reqInt("positionIndex", INT8_MIN, INT8_MAX));

    for (const QJsonValue& v : r.reqArray("officials")) {
        StateShotRecord rec;
        if (!readShotRecord(v, &rec, r))
            break;
        s.officials.append(rec);
    }
    for (const QJsonValue& v : r.reqArray("sighters")) {
        StateShotRecord rec;
        if (!readShotRecord(v, &rec, r))
            break;
        s.sighters.append(rec);
    }
    for (const QJsonValue& v : r.reqArray("crossShots")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType,
                   QStringLiteral("cross shot is not an object"));
            break;
        }
        const QJsonObject co = v.toObject();
        CrossShotRec rec;
        const ReliabilityResult sr =
            EventSerializer::deserializeShotCore(co, &rec.shot);
        if (!sr.ok) {
            r.failed = true;
            r.err = sr.error;
            break;
        }
        StateReader cr{co};
        rec.seq = static_cast<quint64>(
            cr.reqInt("seq", 0, std::numeric_limits<qint64>::max()));
        rec.sourceLane = cr.reqString("sourceLane");
        if (cr.failed) {
            r.failed = true;
            r.err = cr.err;
            break;
        }
        s.crossShots.append(rec);
    }
    for (const QJsonValue& v : r.reqArray("corrections")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType,
                   QStringLiteral("correction is not an object"));
            break;
        }
        StateReader cr{v.toObject()};
        CorrectionEntry c;
        c.targetSeq = static_cast<quint64>(
            cr.reqInt("targetSeq", 0, std::numeric_limits<qint64>::max()));
        c.type = cr.reqString("type");
        c.reason = cr.reqString("reason");
        c.fromTenths = static_cast<qint16>(cr.reqInt("fromTenths", INT16_MIN, INT16_MAX));
        c.toTenths = static_cast<qint16>(cr.reqInt("toTenths", INT16_MIN, INT16_MAX));
        if (cr.failed) {
            r.failed = true;
            r.err = cr.err;
            break;
        }
        s.corrections.append(c);
    }
    for (const QJsonValue& v : r.reqArray("adjustments")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType,
                   QStringLiteral("adjustment is not an object"));
            break;
        }
        StateReader ar{v.toObject()};
        AdjustmentEntry a;
        a.stageId = static_cast<qint16>(ar.reqInt("stageId", INT16_MIN, INT16_MAX));
        a.deltaTenths = static_cast<qint32>(ar.reqInt("deltaTenths", INT32_MIN, INT32_MAX));
        a.kind = ar.reqString("kind");
        a.reason = ar.reqString("reason");
        if (ar.failed) {
            r.failed = true;
            r.err = ar.err;
            break;
        }
        s.adjustments.append(a);
    }
    for (const QJsonValue& v : r.reqArray("incidents")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType,
                   QStringLiteral("incident is not an object"));
            break;
        }
        StateReader ir{v.toObject()};
        IncidentEntry i;
        i.kind = ir.reqString("kind");
        i.note = ir.reqString("note");
        i.allowedTimeMs = ir.reqInt("allowedTimeMs",
                                    std::numeric_limits<qint64>::min(),
                                    std::numeric_limits<qint64>::max());
        i.seq = static_cast<quint64>(
            ir.reqInt("seq", 0, std::numeric_limits<qint64>::max()));
        if (ir.failed) {
            r.failed = true;
            r.err = ir.err;
            break;
        }
        s.incidents.append(i);
    }
    // estIncidents (state v2). optArray → a v1 snapshot with no such key
    // yields an empty vector (backward compatible).
    for (const QJsonValue& v : r.optArray("estIncidents")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType,
                   QStringLiteral("est incident is not an object"));
            break;
        }
        StateReader er{v.toObject()};
        EstIncidentRecord i;
        i.incidentId = er.reqString("incidentId");
        i.incidentType = static_cast<quint8>(er.reqInt("incidentType", 0, 255));
        i.scope = static_cast<quint8>(er.reqInt("scope", 0, 255));
        i.firingPoint = er.reqString("firingPoint");
        i.relayId = er.reqString("relayId");
        i.interruptionStartUtc = er.reqString("interruptionStartUtc");
        i.systemRestoredUtc = er.reqString("systemRestoredUtc");
        i.calculatedDurationMs = er.reqInt("calculatedDurationMs",
                                           std::numeric_limits<qint64>::min(),
                                           std::numeric_limits<qint64>::max());
        i.officiallyAcceptedDurationMs =
            er.reqInt("officiallyAcceptedDurationMs",
                      std::numeric_limits<qint64>::min(),
                      std::numeric_limits<qint64>::max());
        i.targetMoved = er.reqBool("targetMoved");
        i.originalTarget = er.reqString("originalTarget");
        i.reserveTarget = er.reqString("reserveTarget");
        i.backupScoreReviewed = er.reqBool("backupScoreReviewed");
        i.timeCreditMs = er.reqInt("timeCreditMs",
                                   std::numeric_limits<qint64>::min(),
                                   std::numeric_limits<qint64>::max());
        i.preparationGranted = er.reqBool("preparationGranted");
        i.sightingGranted = er.reqBool("sightingGranted");
        i.officialResumeAuthorised = er.reqBool("officialResumeAuthorised");
        i.authorisedBy = er.reqString("authorisedBy");
        i.juryNote = er.reqString("juryNote");
        i.rangeOfficerNote = er.reqString("rangeOfficerNote");
        i.incidentReportRef = er.reqString("incidentReportRef");
        i.status = static_cast<quint8>(er.reqInt("status", 0, 255));
        i.reason = er.reqString("reason");
        i.raisedSeq = static_cast<quint64>(
            er.reqInt("raisedSeq", 0, std::numeric_limits<qint64>::max()));
        // state v3 fields — absent in v2 snapshots, default there.
        i.creditDecision = static_cast<quint8>(
            er.optIntDef("creditDecision", 0, 0, 255));
        i.backupReview = static_cast<quint8>(
            er.optIntDef("backupReview", 0, 0, 255));
        i.raisedAtMonoMs = er.optIntDef("raisedAtMonoMs", 0,
                                        std::numeric_limits<qint64>::min(),
                                        std::numeric_limits<qint64>::max());
        i.sightingGrantedSeq = static_cast<quint64>(
            er.optIntDef("sightingGrantedSeq", 0, 0,
                         std::numeric_limits<qint64>::max()));
        if (er.failed) {
            r.failed = true;
            r.err = er.err;
            break;
        }
        s.estIncidents.append(i);
    }

    // ── Training Lab programmes (state v5) ───────────────────────────────
    // Every key optional: a v1-v4 snapshot simply has none of them and
    // restores to "no programme", which is what those snapshots meant.
    s.sessionKind = r.optStringDef("sessionKind");
    {
        // Absent in every state v1-v6 snapshot, and in any session that never
        // had a competition profile. Absent restores as LEGACY - explicitly,
        // not as a guess.
        const QJsonValue av = r.o.value(QLatin1String("ruleAuthority"));
        if (av.isObject()) {
            StateReader ar{av.toObject()};
            RuleAuthority a;
            a.authorityVersion = static_cast<qint32>(ar.optIntDef("authorityVersion", 1, 1, INT32_MAX));
            a.programmeId = ar.optStringDef("programmeId");
            a.rulesetId = ar.optStringDef("rulesetId");
            a.rulesetVersion = ar.optStringDef("rulesetVersion");
            a.ruleNumber = ar.optStringDef("ruleNumber");
            a.programmeVariant = ar.optStringDef("programmeVariant");
            a.competitionContext = ar.optStringDef("competitionContext");
            a.scoringMode = ar.optStringDef("scoringMode");
            a.timingModel = ar.optStringDef("timingModel");
            a.targetStandardId = ar.optStringDef("targetStandardId");
            a.disciplineId = ar.optStringDef("disciplineId");
            a.distanceM = static_cast<qint32>(ar.optIntDef("distanceM", 0, 0, INT32_MAX));
            a.preparationMs = ar.optIntDef("preparationMs", 0, 0, INT64_MAX);
            a.matchMs = ar.optIntDef("matchMs", 0, 0, INT64_MAX);
            a.positionSequence = ar.optStringDef("positionSequence");
            a.positionDurationsMs = ar.optStringDef("positionDurationsMs");
            s.ruleAuthority = a;
        }
    }
    {
        const QJsonValue tv = r.o.value(QLatin1String("training"));
        if (tv.isObject()) {
            StateReader tr{tv.toObject()};
            s.trainingActive = tr.optBoolDef("active", false);
            s.trainingCompleted = tr.optBoolDef("completed", false);
            s.trainingProgramId = tr.optStringDef("programId");
            s.trainingBlockCount = static_cast<qint16>(tr.optIntDef("blockCount", 0, 0, INT16_MAX));
            s.trainingShotsPerBlock = static_cast<qint16>(tr.optIntDef("shotsPerBlock", 0, 0, INT16_MAX));
            s.trainingVisibility = static_cast<qint8>(tr.optIntDef("visibility", 0, 0, 127));
            s.trainingFocus = tr.optStringDef("focus");
            s.trainingCurrentBlock = static_cast<qint16>(tr.optIntDef("currentBlock", 0, 0, INT16_MAX));
            s.trainingCurrentPosition = static_cast<qint8>(tr.optIntDef("currentPosition", 0, 0, 127));
            s.trainingInSighterPhase = tr.optBoolDef("inSighterPhase", false);
            s.trainingSighterPosition = static_cast<qint8>(tr.optIntDef("sighterPosition", 0, 0, 127));
            s.trainingSighterBeforeBlock = static_cast<qint16>(tr.optIntDef("sighterBeforeBlock", 1, 0, INT16_MAX));
            if (tr.failed) { r.failed = true; r.err = tr.err; }
        } else if (!tv.isUndefined()) {
            r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("'training' not an object"));
        }
    }
    for (const QJsonValue& v : r.optArray("trainingBlocks")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("training block is not an object"));
            break;
        }
        StateReader br{v.toObject()};
        TrainingBlockData b;
        b.blockIndex = static_cast<qint16>(br.reqInt("blockIndex", 0, INT16_MAX));
        b.position = static_cast<qint8>(br.reqInt("position", 0, 127));
        b.completed = br.optBoolDef("completed", false);
        b.note = br.optStringDef("note");
        for (const QJsonValue& sv : br.optArray("shots")) {
            if (!sv.isObject()) {
                r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("training shot is not an object"));
                break;
            }
            ShotCore sh;
            const ReliabilityResult sr = EventSerializer::deserializeShotCore(sv.toObject(), &sh);
            if (!sr.ok) { r.failed = true; r.err = sr.error; break; }
            b.shots.append(sh);
        }
        if (br.failed) { r.failed = true; r.err = br.err; break; }
        if (r.failed) break;
        s.trainingBlocks.append(b);
    }
    for (const QJsonValue& v : r.optArray("trainingSighters")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("training sighter is not an object"));
            break;
        }
        const QJsonObject obj = v.toObject();
        ShotCore sh;
        const ReliabilityResult sr = EventSerializer::deserializeShotCore(obj, &sh);
        if (!sr.ok) { r.failed = true; r.err = sr.error; break; }
        StateReader sr2{obj};
        const qint8 pos = static_cast<qint8>(sr2.optIntDef("position", 0, 0, 127));
        if (sr2.failed) { r.failed = true; r.err = sr2.err; break; }
        s.trainingSighters.append(sh);
        s.trainingSighterPos.append(pos);
    }
    {
        const QJsonValue cv = r.o.value(QLatin1String("callDiagnose"));
        if (cv.isObject()) {
            StateReader cr{cv.toObject()};
            s.cdActive = cr.optBoolDef("active", false);
            s.cdCompleted = cr.optBoolDef("completed", false);
            s.cdCallingActive = cr.optBoolDef("callingActive", false);
            s.cdProgramId = cr.optStringDef("programId");
            s.cdFocus = cr.optStringDef("focus");
            s.cdShotCount = static_cast<qint16>(cr.optIntDef("shotCount", 0, 0, INT16_MAX));
            s.cdCurrentPosition = static_cast<qint8>(cr.optIntDef("currentPosition", 0, 0, 127));
            s.cdThreePositions = cr.optBoolDef("threePositions", false);
            s.cdSessionNote = cr.optStringDef("sessionNote");
            if (cr.failed) { r.failed = true; r.err = cr.err; }
        } else if (!cv.isUndefined()) {
            r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("'callDiagnose' not an object"));
        }
    }
    for (const QJsonValue& v : r.optArray("cdShots")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("call/diagnose shot is not an object"));
            break;
        }
        const QJsonObject obj = v.toObject();
        CallDiagnoseShotRecord c;
        const ReliabilityResult sr = EventSerializer::deserializeShotCore(obj, &c.actual);
        if (!sr.ok) { r.failed = true; r.err = sr.error; break; }
        StateReader cr{obj};
        c.shotNumber = static_cast<qint16>(cr.reqInt("shotNumber", 0, INT16_MAX));
        c.position = static_cast<qint8>(cr.reqInt("position", 0, 127));
        c.hasCall = cr.optBoolDef("hasCall", false);
        c.calledXHundredthMm = static_cast<qint32>(cr.optIntDef("calledXHundredthMm", 0, INT32_MIN, INT32_MAX));
        c.calledYHundredthMm = static_cast<qint32>(cr.optIntDef("calledYHundredthMm", 0, INT32_MIN, INT32_MAX));
        c.callSplitMs = static_cast<qint32>(cr.optIntDef("callSplitMs", 0, INT32_MIN, INT32_MAX));
        c.note = cr.optStringDef("note");
        if (cr.failed) { r.failed = true; r.err = cr.err; break; }
        s.cdShots.append(c);
    }
    {
        const QJsonValue pv = r.o.value(QLatin1String("positionTransition"));
        if (pv.isObject()) {
            StateReader pr{pv.toObject()};
            s.ptActive = pr.optBoolDef("active", false);
            s.ptCompleted = pr.optBoolDef("completed", false);
            s.ptProgramId = pr.optStringDef("programId");
            s.ptSequence = pr.optStringDef("sequence");
            s.ptFocus = pr.optStringDef("focus");
            s.ptVerificationShots = static_cast<qint16>(pr.optIntDef("verificationShots", 0, 0, INT16_MAX));
            s.ptRepeats = static_cast<qint16>(pr.optIntDef("repeats", 1, 0, INT16_MAX));
            s.ptChecklistMode = static_cast<qint8>(pr.optIntDef("checklistMode", 0, 0, 127));
            s.ptCurrentPosition = static_cast<qint8>(pr.optIntDef("currentPosition", 0, 0, 127));
            s.ptCurrentRepeat = static_cast<qint16>(pr.optIntDef("currentRepeat", 1, 0, INT16_MAX));
            s.ptInSetup = pr.optBoolDef("inSetup", false);
            s.ptVerifying = pr.optBoolDef("verifying", false);
            s.ptSessionNote = pr.optStringDef("sessionNote");
            if (pr.failed) { r.failed = true; r.err = pr.err; }
        } else if (!pv.isUndefined()) {
            r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("'positionTransition' not an object"));
        }
    }
    for (const QJsonValue& v : r.optArray("ptRecords")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("pt record is not an object"));
            break;
        }
        StateReader pr{v.toObject()};
        PtPositionRecord p;
        p.position = static_cast<qint8>(pr.reqInt("position", 0, 127));
        p.repeat = static_cast<qint16>(pr.reqInt("repeat", 0, INT16_MAX));
        p.setupDurationMs = static_cast<qint32>(pr.optIntDef("setupDurationMs", 0, INT32_MIN, INT32_MAX));
        p.readyMonoMs = static_cast<qint32>(pr.optIntDef("readyMonoMs", 0, INT32_MIN, INT32_MAX));
        p.note = pr.optStringDef("note");
        p.completed = pr.optBoolDef("completed", false);
        for (const QJsonValue& sv : pr.optArray("sighters")) {
            if (!sv.isObject()) { r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("pt sighter is not an object")); break; }
            ShotCore sh;
            const ReliabilityResult sr = EventSerializer::deserializeShotCore(sv.toObject(), &sh);
            if (!sr.ok) { r.failed = true; r.err = sr.error; break; }
            p.sighters.append(sh);
        }
        for (const QJsonValue& sv : pr.optArray("verifShots")) {
            if (!sv.isObject()) { r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("pt verification shot is not an object")); break; }
            ShotCore sh;
            const ReliabilityResult sr = EventSerializer::deserializeShotCore(sv.toObject(), &sh);
            if (!sr.ok) { r.failed = true; r.err = sr.error; break; }
            p.verifShots.append(sh);
        }
        for (const QJsonValue& cv2 : pr.optArray("checklist")) {
            if (!cv2.isObject()) { r.fail(ReliabilityError::InvalidFieldType, QStringLiteral("pt checklist entry is not an object")); break; }
            StateReader kr{cv2.toObject()};
            p.checklist.append(static_cast<qint8>(kr.reqInt("v", -128, 127)));
            if (kr.failed) { r.failed = true; r.err = kr.err; break; }
        }
        if (pr.failed) { r.failed = true; r.err = pr.err; break; }
        if (r.failed) break;
        s.ptRecords.append(p);
    }

    // ── Wind Map (state v4) ──────────────────────────────────────────────
    // Optional throughout: a v1-v3 snapshot has no windMap key and restores to
    // the defaults, which is exactly "no Wind Map session". Nothing is
    // inferred — an absent wind reading stays absent.
    {
        const QJsonValue wmv = r.o.value(QLatin1String("windMap"));
        if (wmv.isObject()) {
            StateReader wr{wmv.toObject()};
            s.wmActive = wr.optBoolDef("active", false);
            s.wmCompleted = wr.optBoolDef("completed", false);
            s.wmProgramId = wr.optStringDef("programId");
            s.wmDisciplineId = wr.optStringDef("disciplineId");
            s.wmThreePositions = wr.optBoolDef("threePositions", false);
            s.wmCurrentPosition = static_cast<qint8>(wr.optIntDef("currentPosition", 0, 0, 3));
            s.wmPositionSequence = wr.optStringDef("positionSequence");
            // v6. Absent in a v4/v5 snapshot -> 0 (Idle), which is right: those
            // snapshots predate the capture workflow entirely.
            s.wmPhase = static_cast<qint8>(wr.optIntDef("phase", 0, 0, 6));
            s.wmConditionChanges =static_cast<qint32>(wr.optIntDef("conditionChanges", 0, 0, INT32_MAX));
            s.wmNextShotId = static_cast<qint32>(wr.optIntDef("nextShotId", 1, 0, INT32_MAX));
            s.wmWindValid = wr.optBoolDef("windValid", false);
            s.wmWindCalm = wr.optBoolDef("windCalm", false);
            s.wmWindDirectionDegrees = static_cast<qint16>(wr.optIntDef("windDirDeg", 0, 0, 359));
            s.wmWindSpeedHundredthMs = static_cast<qint32>(wr.optIntDef("windSpeedHundredthMs", 0, 0, 100000));
            s.wmWindSource = static_cast<qint8>(wr.optIntDef("windSource", 0, 0, 1));
            s.wmWindRecordedMs = wr.optIntDef("windRecordedMs", 0, 0, std::numeric_limits<qint64>::max());
            s.wmWindNote = wr.optStringDef("windNote");
            if (wr.failed) { r.failed = true; r.err = wr.err; }
        } else if (!wmv.isUndefined()) {
            r.fail(ReliabilityError::InvalidFieldType,
                   QStringLiteral("'windMap' not an object"));
        }
    }
    for (const QJsonValue& v : r.optArray("windMapShots")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType,
                   QStringLiteral("wind map shot is not an object"));
            break;
        }
        const QJsonObject obj = v.toObject();
        WindMapShotRecord rec;
        const ReliabilityResult sr = EventSerializer::deserializeShotCore(obj, &rec.shot);
        if (!sr.ok) { r.failed = true; r.err = sr.error; break; }
        StateReader wr{obj};
        rec.shotId = static_cast<qint32>(wr.reqInt("shotId", 0, INT32_MAX));
        rec.position = static_cast<qint8>(wr.reqInt("position", 0, 3));
        rec.sighter = wr.optBoolDef("sighter", false);
        rec.windValid = wr.optBoolDef("windValid", false);
        rec.windCalm = wr.optBoolDef("windCalm", false);
        rec.windDirectionDegrees = static_cast<qint16>(wr.optIntDef("windDirDeg", 0, 0, 359));
        rec.windSpeedHundredthMs = static_cast<qint32>(wr.optIntDef("windSpeedHundredthMs", 0, 0, 100000));
        rec.windSource = static_cast<qint8>(wr.optIntDef("windSource", 0, 0, 1));
        rec.windRecordedMs = wr.optIntDef("windRecordedMs", 0, 0, std::numeric_limits<qint64>::max());
        rec.windNote = wr.optStringDef("windNote");
        if (wr.failed) { r.failed = true; r.err = wr.err; break; }
        s.wmShots.append(rec);
    }

    s.totalTenths = static_cast<qint32>(r.reqInt("totalTenths", INT32_MIN, INT32_MAX));
    for (const QJsonValue& v : r.reqArray("stageSubtotals")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType,
                   QStringLiteral("stage subtotal is not an object"));
            break;
        }
        StateReader sr{v.toObject()};
        const qint16 stageId =
            static_cast<qint16>(sr.reqInt("stageId", INT16_MIN, INT16_MAX));
        const qint32 tenths =
            static_cast<qint32>(sr.reqInt("tenths", INT32_MIN, INT32_MAX));
        if (sr.failed) {
            r.failed = true;
            r.err = sr.err;
            break;
        }
        s.stageSubtotalTenths.insert(stageId, tenths);
    }
    for (const QJsonValue& v : r.reqArray("stageStatuses")) {
        if (!v.isObject()) {
            r.fail(ReliabilityError::InvalidFieldType,
                   QStringLiteral("stage status is not an object"));
            break;
        }
        StateReader sr{v.toObject()};
        const qint16 stageId =
            static_cast<qint16>(sr.reqInt("stageId", INT16_MIN, INT16_MAX));
        const qint8 status = static_cast<qint8>(sr.reqInt("status", INT8_MIN, INT8_MAX));
        if (sr.failed) {
            r.failed = true;
            r.err = sr.err;
            break;
        }
        s.stageStatuses.insert(stageId, status);
    }

    {
        const QJsonObject to = r.reqObject("timer");
        if (!r.failed) {
            StateReader tr{to};
            s.timer.active = tr.reqBool("active");
            s.timer.timerId = static_cast<TimerId>(tr.reqInt("timerId", 0, 3));
            s.timer.durationMs = tr.reqInt("durationMs",
                                           std::numeric_limits<qint64>::min(),
                                           std::numeric_limits<qint64>::max());
            s.timer.startedAtMonoMs = tr.reqInt("startedAtMonoMs",
                                                std::numeric_limits<qint64>::min(),
                                                std::numeric_limits<qint64>::max());
            s.timer.paused = tr.reqBool("paused");
            s.timer.pausedAtMonoMs = tr.reqInt("pausedAtMonoMs",
                                               std::numeric_limits<qint64>::min(),
                                               std::numeric_limits<qint64>::max());
            s.timer.pausedAccumMs = tr.reqInt("pausedAccumMs",
                                              std::numeric_limits<qint64>::min(),
                                              std::numeric_limits<qint64>::max());
            if (tr.failed) {
                r.failed = true;
                r.err = tr.err;
            }
        }
    }

    s.lastSeq = static_cast<quint64>(
        r.reqInt("lastSeq", 0, std::numeric_limits<qint64>::max()));

    {
        const QJsonObject dobj = r.reqObject("disc");
        if (!r.failed) {
            StateReader dr{dobj};
            const QString kind = dr.reqString("kind");
            if (kind == QLatin1String("qualification")) {
                QualificationState q;
                q.positionIndex = static_cast<qint8>(
                    dr.reqInt("positionIndex", INT8_MIN, INT8_MAX));
                q.sighterMode = dr.reqBool("sighterMode");
                q.version = static_cast<qint32>(dr.reqInt("version", 1, INT32_MAX));
                s.disc = q;
            } else if (kind == QLatin1String("finals3p")) {
                Finals3PState f;
                f.stageId = static_cast<qint16>(dr.reqInt("stageId", INT16_MIN, INT16_MAX));
                f.windowId = static_cast<qint16>(dr.reqInt("windowId", INT16_MIN, INT16_MAX));
                f.shotsInStage =
                    static_cast<qint32>(dr.reqInt("shotsInStage", 0, INT32_MAX));
                f.version = static_cast<qint32>(dr.reqInt("version", 1, INT32_MAX));
                s.disc = f;
            } else if (kind == QLatin1String("training")) {
                TrainingState t;
                t.version = static_cast<qint32>(dr.reqInt("version", 1, INT32_MAX));
                s.disc = t;
            } else if (kind == QLatin1String("finals10m")) {
                Finals10mState f;
                f.stageId = static_cast<qint16>(dr.reqInt("stageId", INT16_MIN, INT16_MAX));
                f.windowId = static_cast<qint16>(dr.reqInt("windowId", INT16_MIN, INT16_MAX));
                f.shotsInStage =
                    static_cast<qint32>(dr.reqInt("shotsInStage", 0, INT32_MAX));
                f.version = static_cast<qint32>(dr.reqInt("version", 1, INT32_MAX));
                s.disc = f;
            } else if (kind == QLatin1String("none")) {
                s.disc = std::monostate{};
            } else if (!dr.failed) {
                dr.fail(ReliabilityError::InvalidFieldType,
                        QStringLiteral("unknown disc kind '%1'").arg(kind));
            }
            if (dr.failed) {
                r.failed = true;
                r.err = dr.err;
            }
        }
    }

    if (r.failed)
        return ReliabilityResult::failureFrom(r.err);
    *out = s;
    return ReliabilityResult::success();
}

StateSnapshot buildStateSnapshot(const SessionState& state)
{
    StateSnapshot snap;
    snap.stateVersion = kSessionStateVersion;
    snap.lastAppliedSeq = state.lastSeq;
    snap.officialCount = state.officials.size();
    snap.sighterCount = state.sighters.size();
    snap.totalTenths = state.totalTenths;
    snap.stateJson = serializeSessionState(state);
    return snap;
}

} // namespace rel
} // namespace ta
