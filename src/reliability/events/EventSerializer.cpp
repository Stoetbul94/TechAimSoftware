#include "EventSerializer.h"
#include "EventRegistry.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <cstdint>
#include <limits>

namespace ta {
namespace rel {

// ── OrderedJsonWriter ─────────────────────────────────────────────────

void OrderedJsonWriter::beginObject()
{
    prepareArrayElement();
    m_buf += '{';
    m_scopes.append(Scope{false, false});
}

void OrderedJsonWriter::endObject()
{
    m_buf += '}';
    if (!m_scopes.isEmpty())
        m_scopes.removeLast();
}

void OrderedJsonWriter::prepareArrayElement()
{
    if (!m_scopes.isEmpty() && m_scopes.last().isArray) {
        if (m_scopes.last().needComma)
            m_buf += ',';
        m_scopes.last().needComma = true;
    }
}

void OrderedJsonWriter::prepareField(const char* key)
{
    if (!m_scopes.isEmpty()) {
        if (m_scopes.last().needComma)
            m_buf += ',';
        m_scopes.last().needComma = true;
    }
    m_buf += '"';
    m_buf += key;          // keys are internal ASCII identifiers
    m_buf += "\":";
}

void OrderedJsonWriter::beginObjectField(const char* key)
{
    prepareField(key);
    m_buf += '{';
    m_scopes.append(Scope{false, false});
}

void OrderedJsonWriter::beginArrayField(const char* key)
{
    prepareField(key);
    m_buf += '[';
    m_scopes.append(Scope{false, true});
}

void OrderedJsonWriter::endArray()
{
    m_buf += ']';
    if (!m_scopes.isEmpty())
        m_scopes.removeLast();
}

void OrderedJsonWriter::field(const char* key, qint64 value)
{
    prepareField(key);
    m_buf += QByteArray::number(value);   // locale-free by contract
}

void OrderedJsonWriter::fieldU(const char* key, quint64 value)
{
    prepareField(key);
    m_buf += QByteArray::number(value);
}

void OrderedJsonWriter::field(const char* key, bool value)
{
    prepareField(key);
    m_buf += value ? "true" : "false";
}

void OrderedJsonWriter::field(const char* key, const QString& value)
{
    prepareField(key);
    m_buf += '"';
    appendEscaped(m_buf, value);
    m_buf += '"';
}

void OrderedJsonWriter::fieldHash(const char* key, const QByteArray& hashHex)
{
    prepareField(key);
    m_buf += '"';
    m_buf += hashHex;      // validated 32-hex ASCII by the caller
    m_buf += '"';
}

QByteArray OrderedJsonWriter::take()
{
    return std::move(m_buf);
}

void OrderedJsonWriter::appendEscaped(QByteArray& out, const QString& value)
{
    const QByteArray utf8 = value.toUtf8();
    for (char c : utf8) {
        const uchar u = static_cast<uchar>(c);
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b";  break;
        case '\f': out += "\\f";  break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (u < 0x20) {
                out += "\\u00";
                const char* hex = "0123456789abcdef";
                out += hex[(u >> 4) & 0xF];
                out += hex[u & 0xF];
            } else {
                out += c;   // ASCII and multi-byte UTF-8 pass through raw
            }
        }
    }
}

// ── payload serialization (frozen field orders) ───────────────────────

namespace {

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void writeConfig(OrderedJsonWriter& w, const DisciplineConfig& c)
{
    w.beginObjectField("config");
    EventSerializer::serializeDisciplineConfigFields(c, w);
    w.endObject();
}

void writeShotCore(OrderedJsonWriter& w, const ShotCore& s)
{
    EventSerializer::serializeShotCoreFields(s, w);
}

} // namespace

// ── shared sub-structure layouts (single source of truth) ─────────────

void EventSerializer::serializeDisciplineConfigFields(const DisciplineConfig& c,
                                                      OrderedJsonWriter& w)
{
    w.field("officialShots", static_cast<qint64>(c.officialShots));
    w.field("seriesSize", static_cast<qint64>(c.seriesSize));
    w.field("matchMs", c.matchMs);
    w.field("perPositionShots", static_cast<qint64>(c.perPositionShots));
    w.field("sighterLimit", static_cast<qint64>(c.sighterLimit));
    w.field("configVersion", static_cast<qint64>(c.configVersion));
}

void EventSerializer::serializeShotCoreFields(const ShotCore& s,
                                              OrderedJsonWriter& w)
{
    w.field("shotNumber", static_cast<qint64>(s.shotNumber));
    w.field("withinStage", static_cast<qint64>(s.withinStage));
    w.field("stageId", static_cast<qint64>(s.stageId));
    w.field("seriesIndex", static_cast<qint64>(s.seriesIndex));
    w.field("xHundredthMm", static_cast<qint64>(s.xHundredthMm));
    w.field("yHundredthMm", static_cast<qint64>(s.yHundredthMm));
    w.field("scoreTenths", static_cast<qint64>(s.scoreTenths));
    w.field("directionCentiDeg", static_cast<qint64>(s.directionCentiDeg));
    w.field("splitMs", static_cast<qint64>(s.splitMs));
    w.field("windowId", static_cast<qint64>(s.windowId));
    w.field("targetMode", static_cast<qint64>(s.targetMode));
    w.field("externalId", s.externalId);
    w.field("simulated", s.simulated);
}

void EventSerializer::serializePayloadInto(const DomainEvent& event,
                                           OrderedJsonWriter& w)
{
    std::visit(overloaded{
        [&](const SessionStarted& e) {
            w.field("sessionId", e.sessionId);
            w.field("schemaVersion", static_cast<qint64>(e.schemaVersion));
            w.field("appVersion", e.appVersion);
            w.field("createdAtIso", e.createdAtIso);
            w.field("athlete", e.athlete);
            w.field("lane", e.lane);
            w.field("targetId", e.targetId);
            w.field("discipline", QString::fromLatin1(disciplineId(e.discipline)));
            w.field("matchType", e.matchType);
            writeConfig(w, e.config);
            w.field("deviceId", e.deviceId);
        },
        [&](const SessionConfigured& e) {
            w.field("discipline", QString::fromLatin1(disciplineId(e.discipline)));
            w.field("matchType", e.matchType);
            writeConfig(w, e.config);
        },
        [&](const AthleteAssigned& e) {
            w.field("athlete", e.athlete);
            w.field("lane", e.lane);
            w.field("targetId", e.targetId);
        },
        [&](const PreparationStarted& e) {
            w.field("stageId", static_cast<qint64>(e.stageId));
        },
        [&](const SightingStarted& e) {
            w.field("stageId", static_cast<qint64>(e.stageId));
        },
        [&](const OfficialMatchStarted& e) {
            w.field("stageId", static_cast<qint64>(e.stageId));
        },
        [&](const ShotAccepted& e) { writeShotCore(w, e.shot); },
        [&](const SighterAccepted& e) { writeShotCore(w, e.shot); },
        [&](const StageCompleted& e) {
            w.field("stageId", static_cast<qint64>(e.stageId));
            w.field("status", static_cast<qint64>(e.status));
        },
        [&](const PositionChanged& e) {
            w.field("positionIndex", static_cast<qint64>(e.positionIndex));
        },
        [&](const TimerStarted& e) {
            w.field("timerId", static_cast<qint64>(e.timerId));
            w.field("durationMs", e.durationMs);
        },
        [&](const TimerPaused& e) {
            w.field("timerId", static_cast<qint64>(e.timerId));
            w.field("atMonoMs", e.atMonoMs);
        },
        [&](const TimerResumed& e) {
            w.field("timerId", static_cast<qint64>(e.timerId));
            w.field("atMonoMs", e.atMonoMs);
        },
        [&](const TimerExpired& e) {
            w.field("timerId", static_cast<qint64>(e.timerId));
        },
        [&](const MatchCompleted& e) {
            w.field("totalTenths", static_cast<qint64>(e.totalTenths));
            w.field("officialCount", static_cast<qint64>(e.officialCount));
        },
        [&](const SessionSuspended& e) {
            w.field("reason", static_cast<qint64>(e.reason));
        },
        [&](const SessionResumed&) {
            // no fields
        },
        [&](const StateSnapshot& e) {
            w.field("stateVersion", static_cast<qint64>(e.stateVersion));
            w.fieldU("lastAppliedSeq", e.lastAppliedSeq);
            w.field("officialCount", static_cast<qint64>(e.officialCount));
            w.field("sighterCount", static_cast<qint64>(e.sighterCount));
            w.field("totalTenths", static_cast<qint64>(e.totalTenths));
            w.field("state", QString::fromUtf8(e.stateJson));
        },
        [&](const ShotInvalidated& e) {
            w.fieldU("targetSeq", e.targetSeq);
            w.field("reason", e.reason);
            w.field("authority", static_cast<qint64>(e.authority));
        },
        [&](const ShotRescored& e) {
            w.fieldU("targetSeq", e.targetSeq);
            w.field("newScoreTenths", static_cast<qint64>(e.newScoreTenths));
            w.field("hasNewCoordinates", e.hasNewCoordinates);
            w.field("newXHundredthMm", static_cast<qint64>(e.newXHundredthMm));
            w.field("newYHundredthMm", static_cast<qint64>(e.newYHundredthMm));
            w.field("reason", e.reason);
            w.field("authority", static_cast<qint64>(e.authority));
        },
        [&](const SeriesAdjusted& e) {
            w.field("stageId", static_cast<qint64>(e.stageId));
            w.field("deltaTenths", static_cast<qint64>(e.deltaTenths));
            w.field("reason", e.reason);
            w.field("authority", static_cast<qint64>(e.authority));
        },
        [&](const CrossShotRecorded& e) {
            writeShotCore(w, e.shot);
            w.field("sourceLane", e.sourceLane);
        },
        [&](const EquipmentMalfunctionRecorded& e) {
            w.field("note", e.note);
            w.field("allowedTimeMs", e.allowedTimeMs);
        },
        [&](const PenaltyIssued& e) {
            w.field("deltaTenths", static_cast<qint64>(e.deltaTenths));
            w.field("rule", e.rule);
            w.field("authority", static_cast<qint64>(e.authority));
        },
        [&](const RecoveryStarted& e) {
            w.fieldU("fromSeq", e.fromSeq);
        },
        [&](const RecoveryCompleted& e) {
            w.fieldU("resumedAtSeq", e.resumedAtSeq);
            w.field("truncatedTail", e.truncatedTail);
        },
        [&](const SessionClosed& e) {
            w.field("reason", static_cast<qint64>(e.reason));
        },
        [&](const StageEntered& e) {
            w.field("stageId", static_cast<qint64>(e.stageId));
        },
        [&](const StageStatusChanged& e) {
            w.field("stageId", static_cast<qint64>(e.stageId));
            w.field("status", static_cast<qint64>(e.status));
        },
        [&](const TargetModeChanged& e) {
            w.field("mode", static_cast<qint64>(e.mode));
        },
        [&](const WindowOpened& e) {
            w.field("windowId", static_cast<qint64>(e.windowId));
        },
        [&](const WindowClosed& e) {
            w.field("windowId", static_cast<qint64>(e.windowId));
        },
        [&](const CommandIssued& e) {
            w.field("commandId", static_cast<qint64>(e.commandId));
            w.field("commandType", static_cast<qint64>(e.commandType));
            w.field("text", e.text);
            w.field("sequenceNumber", static_cast<qint64>(e.sequenceNumber));
            w.field("audioCueId", e.audioCueId);
            w.field("issuedAtMs", e.issuedAtMs);
            w.field("effectiveAtMs", e.effectiveAtMs);
        },
        [&](const ShotRejected& e) {
            w.field("reason", e.reason);
            w.field("externalId", e.externalId);
            w.field("xHundredthMm", static_cast<qint64>(e.xHundredthMm));
            w.field("yHundredthMm", static_cast<qint64>(e.yHundredthMm));
            w.field("simulated", e.simulated);
        },
        [&](const MissingShotRecorded& e) {
            w.field("expectedNumber", static_cast<qint64>(e.expectedNumber));
            w.field("stageId", static_cast<qint64>(e.stageId));
            w.field("reason", e.reason);
        },
        [&](const PersistenceDegraded& e) {
            w.field("queuedCount", static_cast<qint64>(e.queuedCount));
        },
        [&](const PersistenceRestored& e) {
            w.field("queuedCount", static_cast<qint64>(e.queuedCount));
        },
        [&](const AuxEventsDropped& e) {
            w.fieldU("firstSeq", e.firstSeq);
            w.fieldU("lastSeq", e.lastSeq);
            w.field("count", static_cast<qint64>(e.count));
        },
        [&](const CleanShutdown&) {
            // no fields
        },
        [&](const EstIncidentRaised& e) {
            w.field("incidentId", e.incidentId);
            w.field("incidentType", static_cast<qint64>(e.incidentType));
            w.field("scope", static_cast<qint64>(e.scope));
            w.field("firingPoint", e.firingPoint);
            w.field("relayId", e.relayId);
            w.field("interruptionStartUtc", e.interruptionStartUtc);
            w.field("reason", e.reason);
        },
        [&](const TimeCreditGranted& e) {
            w.field("incidentId", e.incidentId);
            w.field("durationMs", e.durationMs);
            w.field("authority", static_cast<qint64>(e.authority));
            w.field("authorisedBy", e.authorisedBy);
            w.field("reason", e.reason);
        },
        [&](const RecoveryPhaseEntered& e) {
            w.field("incidentId", e.incidentId);
            w.field("phase", static_cast<qint64>(e.phase));
            w.field("durationMs", e.durationMs);
            w.field("authority", static_cast<qint64>(e.authority));
            w.field("authorisedBy", e.authorisedBy);
        },
        [&](const EstIncidentResolved& e) {
            w.field("incidentId", e.incidentId);
            w.field("status", static_cast<qint64>(e.status));
            w.field("calculatedDurationMs", e.calculatedDurationMs);
            w.field("officiallyAcceptedDurationMs", e.officiallyAcceptedDurationMs);
            w.field("systemRestoredUtc", e.systemRestoredUtc);
            w.field("targetMoved", e.targetMoved);
            w.field("originalTarget", e.originalTarget);
            w.field("reserveTarget", e.reserveTarget);
            w.field("backupScoreReviewed", e.backupScoreReviewed);
            w.field("juryNote", e.juryNote);
            w.field("rangeOfficerNote", e.rangeOfficerNote);
            w.field("incidentReportRef", e.incidentReportRef);
        }
    }, event);
}

// ── payload deserialization ───────────────────────────────────────────

namespace {

// Accumulating field reader: first failure wins, typed.
struct FieldReader {
    const QJsonObject& o;
    ErrorInfo err{};
    bool failed = false;

    void fail(ReliabilityError code, const QString& detail)
    {
        if (failed)
            return;
        failed = true;
        err = ReliabilityResult::failure(code,
            QStringLiteral("Malformed journal line."), detail).error;
    }

    qint64 reqInt(const char* key, qint64 min, qint64 max)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined()) {
            fail(ReliabilityError::MissingField,
                 QStringLiteral("missing field '%1'").arg(QLatin1String(key)));
            return 0;
        }
        if (!v.isDouble()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("field '%1' is not a number").arg(QLatin1String(key)));
            return 0;
        }
        const qint64 i = v.toInteger();
        if (static_cast<double>(i) != v.toDouble()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("field '%1' is not an integer").arg(QLatin1String(key)));
            return 0;
        }
        if (i < min || i > max) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("field '%1' value %2 outside %3..%4")
                     .arg(QLatin1String(key)).arg(i).arg(min).arg(max));
            return 0;
        }
        return i;
    }

    quint64 reqUInt(const char* key)
    {
        // JSON integers arrive as qint64; sequences never exceed that.
        return static_cast<quint64>(
            reqInt(key, 0, std::numeric_limits<qint64>::max()));
    }

    QString reqString(const char* key)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined()) {
            fail(ReliabilityError::MissingField,
                 QStringLiteral("missing field '%1'").arg(QLatin1String(key)));
            return QString();
        }
        if (!v.isString()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("field '%1' is not a string").arg(QLatin1String(key)));
            return QString();
        }
        return v.toString();
    }

    QString optString(const char* key)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined())
            return QString();
        if (!v.isString()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("field '%1' is not a string").arg(QLatin1String(key)));
            return QString();
        }
        return v.toString();
    }

    bool reqBool(const char* key)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined()) {
            fail(ReliabilityError::MissingField,
                 QStringLiteral("missing field '%1'").arg(QLatin1String(key)));
            return false;
        }
        if (!v.isBool()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("field '%1' is not a boolean").arg(QLatin1String(key)));
            return false;
        }
        return v.toBool();
    }

    QJsonObject reqObject(const char* key)
    {
        const QJsonValue v = o.value(QLatin1String(key));
        if (v.isUndefined()) {
            fail(ReliabilityError::MissingField,
                 QStringLiteral("missing field '%1'").arg(QLatin1String(key)));
            return QJsonObject();
        }
        if (!v.isObject()) {
            fail(ReliabilityError::InvalidFieldType,
                 QStringLiteral("field '%1' is not an object").arg(QLatin1String(key)));
            return QJsonObject();
        }
        return v.toObject();
    }
};

DisciplineConfig readConfig(FieldReader& top)
{
    DisciplineConfig c;
    const QJsonObject obj = top.reqObject("config");
    if (top.failed)
        return c;
    const ReliabilityResult r = EventSerializer::deserializeDisciplineConfig(obj, &c);
    if (!r.ok) {
        top.failed = true;
        top.err = r.error;
    }
    return c;
}

Discipline readDiscipline(FieldReader& r, const char* key)
{
    const QString id = r.reqString(key);
    if (r.failed)
        return Discipline::None;
    Discipline d = Discipline::None;
    if (!disciplineFromId(id, &d)) {
        r.fail(ReliabilityError::InvalidFieldType,
               QStringLiteral("unknown discipline id '%1'").arg(id));
        return Discipline::None;
    }
    return d;
}

ShotCore readShotCore(FieldReader& r)
{
    ShotCore s;
    const ReliabilityResult res = EventSerializer::deserializeShotCore(r.o, &s);
    if (!res.ok && !r.failed) {
        r.failed = true;
        r.err = res.error;
    }
    return s;
}

TimerId readTimerId(FieldReader& r)
{
    return static_cast<TimerId>(r.reqInt("timerId", 0, 3));
}

Authority readAuthority(FieldReader& r)
{
    return static_cast<Authority>(r.reqInt("authority", 0, 1));
}

} // namespace

ReliabilityResult EventSerializer::deserializeDisciplineConfig(
    const QJsonObject& obj, DisciplineConfig* out)
{
    FieldReader r{obj};
    out->officialShots = static_cast<qint32>(r.reqInt("officialShots", 0, INT32_MAX));
    out->seriesSize = static_cast<qint32>(r.reqInt("seriesSize", 0, INT32_MAX));
    out->matchMs = r.reqInt("matchMs", 0, std::numeric_limits<qint64>::max());
    out->perPositionShots =
        static_cast<qint32>(r.reqInt("perPositionShots", 0, INT32_MAX));
    out->sighterLimit = static_cast<qint32>(r.reqInt("sighterLimit", -1, INT32_MAX));
    out->configVersion = static_cast<qint32>(r.reqInt("configVersion", 1, INT32_MAX));
    if (r.failed)
        return ReliabilityResult::failureFrom(r.err);
    return ReliabilityResult::success();
}

ReliabilityResult EventSerializer::deserializeShotCore(const QJsonObject& obj,
                                                       ShotCore* out)
{
    FieldReader r{obj};
    out->shotNumber = static_cast<qint16>(r.reqInt("shotNumber", INT16_MIN, INT16_MAX));
    out->withinStage = static_cast<qint16>(r.reqInt("withinStage", INT16_MIN, INT16_MAX));
    out->stageId = static_cast<qint16>(r.reqInt("stageId", INT16_MIN, INT16_MAX));
    out->seriesIndex = static_cast<qint8>(r.reqInt("seriesIndex", INT8_MIN, INT8_MAX));
    out->xHundredthMm =
        static_cast<qint32>(r.reqInt("xHundredthMm", INT32_MIN, INT32_MAX));
    out->yHundredthMm =
        static_cast<qint32>(r.reqInt("yHundredthMm", INT32_MIN, INT32_MAX));
    out->scoreTenths = static_cast<qint16>(r.reqInt("scoreTenths", INT16_MIN, INT16_MAX));
    out->directionCentiDeg =
        static_cast<qint32>(r.reqInt("directionCentiDeg", INT32_MIN, INT32_MAX));
    out->splitMs = static_cast<qint32>(r.reqInt("splitMs", INT32_MIN, INT32_MAX));
    out->windowId = static_cast<qint16>(r.reqInt("windowId", INT16_MIN, INT16_MAX));
    out->targetMode = static_cast<qint8>(r.reqInt("targetMode", INT8_MIN, INT8_MAX));
    out->externalId = r.reqInt("externalId", std::numeric_limits<qint64>::min(),
                               std::numeric_limits<qint64>::max());
    out->simulated = r.reqBool("simulated");
    if (r.failed)
        return ReliabilityResult::failureFrom(r.err);
    return ReliabilityResult::success();
}

ReliabilityResult EventSerializer::deserializePayload(const QString& typeId,
                                                      const QJsonObject& p,
                                                      DomainEvent* out)
{
    FieldReader r{p};

    if (typeId == QLatin1String(SessionStarted::kType)) {
        SessionStarted e;
        e.sessionId = r.reqString("sessionId");
        e.schemaVersion = static_cast<qint32>(r.reqInt("schemaVersion", 1, INT32_MAX));
        e.appVersion = r.reqString("appVersion");
        e.createdAtIso = r.reqString("createdAtIso");
        e.athlete = r.reqString("athlete");
        e.lane = r.optString("lane");
        e.targetId = r.optString("targetId");
        e.discipline = readDiscipline(r, "discipline");
        e.matchType = r.reqString("matchType");
        e.config = readConfig(r);
        e.deviceId = r.optString("deviceId");
        *out = e;
    } else if (typeId == QLatin1String(SessionConfigured::kType)) {
        SessionConfigured e;
        e.discipline = readDiscipline(r, "discipline");
        e.matchType = r.reqString("matchType");
        e.config = readConfig(r);
        *out = e;
    } else if (typeId == QLatin1String(AthleteAssigned::kType)) {
        AthleteAssigned e;
        e.athlete = r.reqString("athlete");
        e.lane = r.optString("lane");
        e.targetId = r.optString("targetId");
        *out = e;
    } else if (typeId == QLatin1String(PreparationStarted::kType)) {
        PreparationStarted e;
        e.stageId = static_cast<qint16>(r.reqInt("stageId", INT16_MIN, INT16_MAX));
        *out = e;
    } else if (typeId == QLatin1String(SightingStarted::kType)) {
        SightingStarted e;
        e.stageId = static_cast<qint16>(r.reqInt("stageId", INT16_MIN, INT16_MAX));
        *out = e;
    } else if (typeId == QLatin1String(OfficialMatchStarted::kType)) {
        OfficialMatchStarted e;
        e.stageId = static_cast<qint16>(r.reqInt("stageId", INT16_MIN, INT16_MAX));
        *out = e;
    } else if (typeId == QLatin1String(ShotAccepted::kType)) {
        ShotAccepted e;
        e.shot = readShotCore(r);
        *out = e;
    } else if (typeId == QLatin1String(SighterAccepted::kType)) {
        SighterAccepted e;
        e.shot = readShotCore(r);
        *out = e;
    } else if (typeId == QLatin1String(StageCompleted::kType)) {
        StageCompleted e;
        e.stageId = static_cast<qint16>(r.reqInt("stageId", INT16_MIN, INT16_MAX));
        e.status = static_cast<qint8>(r.reqInt("status", INT8_MIN, INT8_MAX));
        *out = e;
    } else if (typeId == QLatin1String(PositionChanged::kType)) {
        PositionChanged e;
        e.positionIndex = static_cast<qint8>(r.reqInt("positionIndex", INT8_MIN, INT8_MAX));
        *out = e;
    } else if (typeId == QLatin1String(TimerStarted::kType)) {
        TimerStarted e;
        e.timerId = readTimerId(r);
        e.durationMs = r.reqInt("durationMs", std::numeric_limits<qint64>::min(),
                                std::numeric_limits<qint64>::max());
        *out = e;
    } else if (typeId == QLatin1String(TimerPaused::kType)) {
        TimerPaused e;
        e.timerId = readTimerId(r);
        e.atMonoMs = r.reqInt("atMonoMs", std::numeric_limits<qint64>::min(),
                              std::numeric_limits<qint64>::max());
        *out = e;
    } else if (typeId == QLatin1String(TimerResumed::kType)) {
        TimerResumed e;
        e.timerId = readTimerId(r);
        e.atMonoMs = r.reqInt("atMonoMs", std::numeric_limits<qint64>::min(),
                              std::numeric_limits<qint64>::max());
        *out = e;
    } else if (typeId == QLatin1String(TimerExpired::kType)) {
        TimerExpired e;
        e.timerId = readTimerId(r);
        *out = e;
    } else if (typeId == QLatin1String(MatchCompleted::kType)) {
        MatchCompleted e;
        e.totalTenths = static_cast<qint32>(r.reqInt("totalTenths", INT32_MIN, INT32_MAX));
        e.officialCount = static_cast<qint16>(
            r.reqInt("officialCount", INT16_MIN, INT16_MAX));
        *out = e;
    } else if (typeId == QLatin1String(SessionSuspended::kType)) {
        SessionSuspended e;
        e.reason = static_cast<SuspendReason>(r.reqInt("reason", 0, 1));
        *out = e;
    } else if (typeId == QLatin1String(SessionResumed::kType)) {
        *out = SessionResumed{};
    } else if (typeId == QLatin1String(StateSnapshot::kType)) {
        StateSnapshot e;
        e.stateVersion = static_cast<qint32>(r.reqInt("stateVersion", 1, INT32_MAX));
        e.lastAppliedSeq = r.reqUInt("lastAppliedSeq");
        e.officialCount = static_cast<qint32>(
            r.reqInt("officialCount", INT32_MIN, INT32_MAX));
        e.sighterCount = static_cast<qint32>(
            r.reqInt("sighterCount", INT32_MIN, INT32_MAX));
        e.totalTenths = static_cast<qint32>(r.reqInt("totalTenths", INT32_MIN, INT32_MAX));
        e.stateJson = r.reqString("state").toUtf8();
        *out = e;
    } else if (typeId == QLatin1String(ShotInvalidated::kType)) {
        ShotInvalidated e;
        e.targetSeq = r.reqUInt("targetSeq");
        e.reason = r.reqString("reason");
        e.authority = readAuthority(r);
        *out = e;
    } else if (typeId == QLatin1String(ShotRescored::kType)) {
        ShotRescored e;
        e.targetSeq = r.reqUInt("targetSeq");
        e.newScoreTenths = static_cast<qint16>(
            r.reqInt("newScoreTenths", INT16_MIN, INT16_MAX));
        e.hasNewCoordinates = r.reqBool("hasNewCoordinates");
        e.newXHundredthMm = static_cast<qint32>(
            r.reqInt("newXHundredthMm", INT32_MIN, INT32_MAX));
        e.newYHundredthMm = static_cast<qint32>(
            r.reqInt("newYHundredthMm", INT32_MIN, INT32_MAX));
        e.reason = r.reqString("reason");
        e.authority = readAuthority(r);
        *out = e;
    } else if (typeId == QLatin1String(SeriesAdjusted::kType)) {
        SeriesAdjusted e;
        e.stageId = static_cast<qint16>(r.reqInt("stageId", INT16_MIN, INT16_MAX));
        e.deltaTenths = static_cast<qint32>(r.reqInt("deltaTenths", INT32_MIN, INT32_MAX));
        e.reason = r.reqString("reason");
        e.authority = readAuthority(r);
        *out = e;
    } else if (typeId == QLatin1String(CrossShotRecorded::kType)) {
        CrossShotRecorded e;
        e.shot = readShotCore(r);
        e.sourceLane = r.optString("sourceLane");
        *out = e;
    } else if (typeId == QLatin1String(EquipmentMalfunctionRecorded::kType)) {
        EquipmentMalfunctionRecorded e;
        e.note = r.reqString("note");
        e.allowedTimeMs = r.reqInt("allowedTimeMs",
                                   std::numeric_limits<qint64>::min(),
                                   std::numeric_limits<qint64>::max());
        *out = e;
    } else if (typeId == QLatin1String(PenaltyIssued::kType)) {
        PenaltyIssued e;
        e.deltaTenths = static_cast<qint32>(r.reqInt("deltaTenths", INT32_MIN, INT32_MAX));
        e.rule = r.reqString("rule");
        e.authority = readAuthority(r);
        *out = e;
    } else if (typeId == QLatin1String(RecoveryStarted::kType)) {
        RecoveryStarted e;
        e.fromSeq = r.reqUInt("fromSeq");
        *out = e;
    } else if (typeId == QLatin1String(RecoveryCompleted::kType)) {
        RecoveryCompleted e;
        e.resumedAtSeq = r.reqUInt("resumedAtSeq");
        e.truncatedTail = r.reqBool("truncatedTail");
        *out = e;
    } else if (typeId == QLatin1String(SessionClosed::kType)) {
        SessionClosed e;
        e.reason = static_cast<CloseReason>(r.reqInt("reason", 0, 2));
        *out = e;
    } else if (typeId == QLatin1String(StageEntered::kType)) {
        StageEntered e;
        e.stageId = static_cast<qint16>(r.reqInt("stageId", INT16_MIN, INT16_MAX));
        *out = e;
    } else if (typeId == QLatin1String(StageStatusChanged::kType)) {
        StageStatusChanged e;
        e.stageId = static_cast<qint16>(r.reqInt("stageId", INT16_MIN, INT16_MAX));
        e.status = static_cast<qint8>(r.reqInt("status", INT8_MIN, INT8_MAX));
        *out = e;
    } else if (typeId == QLatin1String(TargetModeChanged::kType)) {
        TargetModeChanged e;
        e.mode = static_cast<qint8>(r.reqInt("mode", INT8_MIN, INT8_MAX));
        *out = e;
    } else if (typeId == QLatin1String(WindowOpened::kType)) {
        WindowOpened e;
        e.windowId = static_cast<qint16>(r.reqInt("windowId", INT16_MIN, INT16_MAX));
        *out = e;
    } else if (typeId == QLatin1String(WindowClosed::kType)) {
        WindowClosed e;
        e.windowId = static_cast<qint16>(r.reqInt("windowId", INT16_MIN, INT16_MAX));
        *out = e;
    } else if (typeId == QLatin1String(CommandIssued::kType)) {
        CommandIssued e;
        e.commandId = static_cast<qint32>(r.reqInt("commandId", INT32_MIN, INT32_MAX));
        e.commandType = static_cast<qint16>(
            r.reqInt("commandType", INT16_MIN, INT16_MAX));
        e.text = r.reqString("text");
        e.sequenceNumber = static_cast<qint16>(
            r.reqInt("sequenceNumber", INT16_MIN, INT16_MAX));
        e.audioCueId = r.optString("audioCueId");
        e.issuedAtMs = r.reqInt("issuedAtMs", std::numeric_limits<qint64>::min(),
                                std::numeric_limits<qint64>::max());
        e.effectiveAtMs = r.reqInt("effectiveAtMs",
                                   std::numeric_limits<qint64>::min(),
                                   std::numeric_limits<qint64>::max());
        *out = e;
    } else if (typeId == QLatin1String(ShotRejected::kType)) {
        ShotRejected e;
        e.reason = r.reqString("reason");
        e.externalId = r.reqInt("externalId", std::numeric_limits<qint64>::min(),
                                std::numeric_limits<qint64>::max());
        e.xHundredthMm = static_cast<qint32>(
            r.reqInt("xHundredthMm", INT32_MIN, INT32_MAX));
        e.yHundredthMm = static_cast<qint32>(
            r.reqInt("yHundredthMm", INT32_MIN, INT32_MAX));
        e.simulated = r.reqBool("simulated");
        *out = e;
    } else if (typeId == QLatin1String(MissingShotRecorded::kType)) {
        MissingShotRecorded e;
        e.expectedNumber = static_cast<qint16>(
            r.reqInt("expectedNumber", INT16_MIN, INT16_MAX));
        e.stageId = static_cast<qint16>(r.reqInt("stageId", INT16_MIN, INT16_MAX));
        e.reason = r.reqString("reason");
        *out = e;
    } else if (typeId == QLatin1String(PersistenceDegraded::kType)) {
        PersistenceDegraded e;
        e.queuedCount = static_cast<qint32>(
            r.reqInt("queuedCount", INT32_MIN, INT32_MAX));
        *out = e;
    } else if (typeId == QLatin1String(PersistenceRestored::kType)) {
        PersistenceRestored e;
        e.queuedCount = static_cast<qint32>(
            r.reqInt("queuedCount", INT32_MIN, INT32_MAX));
        *out = e;
    } else if (typeId == QLatin1String(AuxEventsDropped::kType)) {
        AuxEventsDropped e;
        e.firstSeq = r.reqUInt("firstSeq");
        e.lastSeq = r.reqUInt("lastSeq");
        e.count = static_cast<qint32>(r.reqInt("count", INT32_MIN, INT32_MAX));
        *out = e;
    } else if (typeId == QLatin1String(CleanShutdown::kType)) {
        *out = CleanShutdown{};
    } else if (typeId == QLatin1String(EstIncidentRaised::kType)) {
        EstIncidentRaised e;
        e.incidentId = r.reqString("incidentId");
        e.incidentType = static_cast<IncidentType>(r.reqInt("incidentType", 0, 9));
        e.scope = static_cast<IncidentScope>(r.reqInt("scope", 0, 2));
        e.firingPoint = r.optString("firingPoint");
        e.relayId = r.optString("relayId");
        e.interruptionStartUtc = r.reqString("interruptionStartUtc");
        e.reason = r.optString("reason");
        *out = e;
    } else if (typeId == QLatin1String(TimeCreditGranted::kType)) {
        TimeCreditGranted e;
        e.incidentId = r.reqString("incidentId");
        e.durationMs = r.reqInt("durationMs", std::numeric_limits<qint64>::min(),
                                std::numeric_limits<qint64>::max());
        e.authority = readAuthority(r);
        e.authorisedBy = r.reqString("authorisedBy");
        e.reason = r.optString("reason");
        *out = e;
    } else if (typeId == QLatin1String(RecoveryPhaseEntered::kType)) {
        RecoveryPhaseEntered e;
        e.incidentId = r.reqString("incidentId");
        e.phase = static_cast<RecoveryPhaseKind>(r.reqInt("phase", 0, 2));
        e.durationMs = r.reqInt("durationMs", std::numeric_limits<qint64>::min(),
                                std::numeric_limits<qint64>::max());
        e.authority = readAuthority(r);
        e.authorisedBy = r.reqString("authorisedBy");
        *out = e;
    } else if (typeId == QLatin1String(EstIncidentResolved::kType)) {
        EstIncidentResolved e;
        e.incidentId = r.reqString("incidentId");
        e.status = static_cast<IncidentStatus>(r.reqInt("status", 0, 3));
        e.calculatedDurationMs = r.reqInt("calculatedDurationMs",
                                          std::numeric_limits<qint64>::min(),
                                          std::numeric_limits<qint64>::max());
        e.officiallyAcceptedDurationMs =
            r.reqInt("officiallyAcceptedDurationMs",
                     std::numeric_limits<qint64>::min(),
                     std::numeric_limits<qint64>::max());
        e.systemRestoredUtc = r.optString("systemRestoredUtc");
        e.targetMoved = r.reqBool("targetMoved");
        e.originalTarget = r.optString("originalTarget");
        e.reserveTarget = r.optString("reserveTarget");
        e.backupScoreReviewed = r.reqBool("backupScoreReviewed");
        e.juryNote = r.optString("juryNote");
        e.rangeOfficerNote = r.optString("rangeOfficerNote");
        e.incidentReportRef = r.optString("incidentReportRef");
        *out = e;
    } else {
        return ReliabilityResult::failure(ReliabilityError::UnsupportedEventType,
            QStringLiteral("Unknown event type in journal."),
            QStringLiteral("type '%1' is not in the event registry").arg(typeId));
    }

    if (r.failed)
        return ReliabilityResult::failureFrom(r.err);
    return ReliabilityResult::success();
}

// ── envelope serialization ────────────────────────────────────────────

ReliabilityResult EventSerializer::serializeCoreWithoutCurrentHash(
    const EventEnvelope& envelope, QByteArray* out)
{
    ReliabilityResult v = envelope.validate();
    if (!v.ok)
        return v;
    v = validateEvent(envelope.payload);
    if (!v.ok)
        return v;
    if (!isWellFormedHashHex(envelope.previousHash))
        return ReliabilityResult::failure(ReliabilityError::SerializationFailed,
            QStringLiteral("Internal error while writing the journal."),
            QStringLiteral("previousHash is not 32 lowercase hex chars"),
            QString(), static_cast<qint64>(envelope.seq));
    const EventMeta* meta = EventRegistry::metaForType(envelope.eventType);
    if (!meta)
        return ReliabilityResult::failure(ReliabilityError::UnsupportedEventType,
            QStringLiteral("Internal error while writing the journal."),
            QStringLiteral("no registry entry for '%1'").arg(envelope.eventType),
            QString(), static_cast<qint64>(envelope.seq));
    if (envelope.payloadVersion != meta->payloadVersion)
        return ReliabilityResult::failure(ReliabilityError::UnsupportedEventVersion,
            QStringLiteral("Internal error while writing the journal."),
            QStringLiteral("writer pv %1 != registry pv %2 for '%3'")
                .arg(envelope.payloadVersion).arg(meta->payloadVersion)
                .arg(envelope.eventType),
            QString(), static_cast<qint64>(envelope.seq));

    OrderedJsonWriter w;
    w.beginObject();
    w.field("sv", static_cast<qint64>(envelope.schemaVersion));
    w.field("pv", static_cast<qint64>(envelope.payloadVersion));
    w.field("sid", envelope.sessionId);
    w.field("lane", envelope.lane);
    w.fieldU("seq", envelope.seq);
    w.field("tw", envelope.wallTimestampIso);
    w.field("tm", envelope.monotonicMs);
    w.field("t", envelope.eventType);
    w.beginObjectField("p");
    serializePayloadInto(envelope.payload, w);
    w.endObject();
    if (envelope.seq == 0) {
        w.field("av", envelope.appVersion);
        if (!envelope.deviceId.isEmpty())
            w.field("dev", envelope.deviceId);
    }
    w.fieldHash("ph", envelope.previousHash);
    w.endObject();
    *out = w.take();
    return ReliabilityResult::success();
}

ReliabilityResult EventSerializer::serializeCompleteEnvelope(
    const EventEnvelope& envelope, QByteArray* out)
{
    if (!isWellFormedHashHex(envelope.currentHash))
        return ReliabilityResult::failure(ReliabilityError::SerializationFailed,
            QStringLiteral("Internal error while writing the journal."),
            QStringLiteral("currentHash is not 32 lowercase hex chars"),
            QString(), static_cast<qint64>(envelope.seq));
    QByteArray core;
    const ReliabilityResult r = serializeCoreWithoutCurrentHash(envelope, &core);
    if (!r.ok)
        return r;
    // Deterministic completion: the core always ends in '}' — replace it
    // with the h field. Both serializations share every preceding byte.
    core.chop(1);
    core += ",\"h\":\"";
    core += envelope.currentHash;
    core += "\"}";
    *out = core;
    return ReliabilityResult::success();
}

// ── envelope deserialization ──────────────────────────────────────────

ReliabilityResult EventSerializer::deserializeEnvelope(const QByteArray& lineBytes,
                                                       EventEnvelope* out)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(lineBytes, &parseError);
    if (doc.isNull())
        return ReliabilityResult::failure(ReliabilityError::InvalidJson,
            QStringLiteral("Journal line is not valid JSON."),
            QStringLiteral("%1 at offset %2")
                .arg(parseError.errorString()).arg(parseError.offset));
    if (!doc.isObject())
        return ReliabilityResult::failure(ReliabilityError::InvalidJson,
            QStringLiteral("Journal line is not a JSON object."),
            QStringLiteral("top-level JSON value is not an object"));

    const QJsonObject obj = doc.object();
    FieldReader r{obj};

    // Schema version first: a newer schema may not even have these fields.
    const qint32 sv = static_cast<qint32>(r.reqInt("sv", INT32_MIN, INT32_MAX));
    if (r.failed)
        return ReliabilityResult::failureFrom(r.err);
    if (sv > kJournalSchemaVersion)
        return ReliabilityResult::failure(ReliabilityError::SchemaTooNew,
            QStringLiteral("This session was written by a newer TechAim version."),
            QStringLiteral("sv=%1, supported=%2").arg(sv).arg(kJournalSchemaVersion));
    if (sv < 1)
        return ReliabilityResult::failure(ReliabilityError::InvalidFieldType,
            QStringLiteral("Malformed journal line."),
            QStringLiteral("sv=%1 < 1").arg(sv));

    EventEnvelope env;
    env.schemaVersion = sv;
    env.payloadVersion = static_cast<qint32>(r.reqInt("pv", 1, INT32_MAX));
    env.sessionId = r.reqString("sid");
    env.lane = r.optString("lane");
    env.seq = r.reqUInt("seq");
    env.wallTimestampIso = r.reqString("tw");
    env.monotonicMs = r.reqInt("tm", std::numeric_limits<qint64>::min(),
                               std::numeric_limits<qint64>::max());
    env.eventType = r.reqString("t");
    const QJsonObject payloadObj = r.reqObject("p");
    env.appVersion = r.optString("av");
    env.deviceId = r.optString("dev");
    const QString ph = r.reqString("ph");
    const QString h = r.reqString("h");
    if (r.failed) {
        r.err.sequence = static_cast<qint64>(env.seq);
        return ReliabilityResult::failureFrom(r.err);
    }

    env.previousHash = ph.toLatin1();
    env.currentHash = h.toLatin1();
    if (!isWellFormedHashHex(env.previousHash))
        return ReliabilityResult::failure(ReliabilityError::InvalidFieldType,
            QStringLiteral("Malformed journal line."),
            QStringLiteral("ph is not 32 lowercase hex chars"),
            QString(), static_cast<qint64>(env.seq));
    if (!isWellFormedHashHex(env.currentHash))
        return ReliabilityResult::failure(ReliabilityError::InvalidFieldType,
            QStringLiteral("Malformed journal line."),
            QStringLiteral("h is not 32 lowercase hex chars"),
            QString(), static_cast<qint64>(env.seq));

    const EventMeta* meta = EventRegistry::metaForType(env.eventType);
    if (!meta)
        return ReliabilityResult::failure(ReliabilityError::UnsupportedEventType,
            QStringLiteral("This session contains an event this TechAim version "
                           "does not understand."),
            QStringLiteral("type '%1' not in registry").arg(env.eventType),
            QString(), static_cast<qint64>(env.seq));
    if (env.payloadVersion > meta->payloadVersion)
        return ReliabilityResult::failure(ReliabilityError::UnsupportedEventVersion,
            QStringLiteral("This session was written by a newer TechAim version."),
            QStringLiteral("pv %1 > supported %2 for '%3'")
                .arg(env.payloadVersion).arg(meta->payloadVersion).arg(env.eventType),
            QString(), static_cast<qint64>(env.seq));

    ReliabilityResult pr = deserializePayload(env.eventType, payloadObj, &env.payload);
    if (!pr.ok) {
        pr.error.sequence = static_cast<qint64>(env.seq);
        return pr;
    }

    ReliabilityResult vr = env.validate();
    if (!vr.ok)
        return vr;
    vr = validateEvent(env.payload);
    if (!vr.ok) {
        vr.error.sequence = static_cast<qint64>(env.seq);
        return vr;
    }

    *out = env;
    return ReliabilityResult::success();
}

} // namespace rel
} // namespace ta
