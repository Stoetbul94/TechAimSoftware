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
        && lastSeq == o.lastSeq && disc == o.disc;
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
            QStringLiteral("Snapshot written by a newer TechAim version."),
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
        if (er.failed) {
            r.failed = true;
            r.err = er.err;
            break;
        }
        s.estIncidents.append(i);
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
