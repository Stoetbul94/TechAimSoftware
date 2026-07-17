#ifndef TA_REL_EVENTSERIALIZER_H
#define TA_REL_EVENTSERIALIZER_H

// Session Reliability Layer — deterministic envelope serialization (M1,
// spec §5 + M1 step 7).
//
// The serialized representation is BYTE-STABLE and platform-identical:
//   - compact UTF-8 JSON, one envelope per line, LF line endings
//   - top-level fields and every payload's fields are written in a FROZEN
//     order by OrderedJsonWriter — QJsonObject ordering is never relied on
//   - authoritative numeric values are integers (fixed-point raw values)
//   - no locale involvement anywhere (integers formatted by hand)
//   - non-ASCII text passes through as raw UTF-8 (no \u escaping except
//     the mandatory control/quote/backslash escapes)
//
// Verification is STRUCTURED (M1 step 8): deserialize the line, then
// deterministically re-serialize the core and recompute the hash — no
// string-splitting around ',"h":"'. This is sound precisely because the
// serializer is deterministic.

#include "EventEnvelope.h"

#include <QByteArray>
#include <QVarLengthArray>

QT_BEGIN_NAMESPACE
class QJsonObject;
QT_END_NAMESPACE

namespace ta {
namespace rel {

// Minimal ordered JSON emitter. Emits fields exactly in call order —
// the caller IS the schema. Supports nested objects (comma state stack).
class OrderedJsonWriter {
public:
    void beginObject();                       // also: object element in an array
    void endObject();
    void beginObjectField(const char* key);
    void beginArrayField(const char* key);
    void endArray();
    void field(const char* key, qint64 value);
    void fieldU(const char* key, quint64 value);
    void field(const char* key, bool value);
    void field(const char* key, const QString& value);
    void fieldHash(const char* key, const QByteArray& hashHex); // ASCII hex
    QByteArray take();
    static void appendEscaped(QByteArray& out, const QString& value);

private:
    struct Scope { bool needComma; bool isArray; };
    void prepareField(const char* key);
    void prepareArrayElement();
    QByteArray m_buf;
    QVarLengthArray<Scope, 8> m_scopes;
};

namespace EventSerializer {

// Serialize every field EXCEPT `h` (includes `ph`) — the exact bytes the
// hash chain covers. Fails typed on invalid envelope/payload.
ReliabilityResult serializeCoreWithoutCurrentHash(const EventEnvelope& envelope,
                                                  QByteArray* out);

// Full line bytes (core + `h`), no trailing LF. envelope.currentHash must
// be well-formed.
ReliabilityResult serializeCompleteEnvelope(const EventEnvelope& envelope,
                                            QByteArray* out);

// Parse one line (without LF) into a typed envelope. Typed failures:
// InvalidJson, MissingField, InvalidFieldType, UnsupportedEventType,
// UnsupportedEventVersion, SchemaTooNew, InvalidEvent.
ReliabilityResult deserializeEnvelope(const QByteArray& lineBytes,
                                      EventEnvelope* out);

// Payload-level entry points (used by the registry mapping and tests).
void serializePayloadInto(const DomainEvent& event, OrderedJsonWriter& writer);
ReliabilityResult deserializePayload(const QString& typeId,
                                     const QJsonObject& payloadObject,
                                     DomainEvent* out);

// Shared sub-structures (reused by the SessionState serializer so the
// byte layout of ShotCore/DisciplineConfig exists in exactly one place).
// The *Fields variants write into the writer's currently open object.
void serializeShotCoreFields(const ShotCore& shot, OrderedJsonWriter& writer);
ReliabilityResult deserializeShotCore(const QJsonObject& object, ShotCore* out);
void serializeDisciplineConfigFields(const DisciplineConfig& config,
                                     OrderedJsonWriter& writer);
ReliabilityResult deserializeDisciplineConfig(const QJsonObject& object,
                                              DisciplineConfig* out);

} // namespace EventSerializer

} // namespace rel
} // namespace ta

#endif // TA_REL_EVENTSERIALIZER_H
