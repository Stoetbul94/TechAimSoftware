#include "rms/node/TechAimNodeCommands.h"

#include "qualification/QualificationController.h"
#include "reliability/journal/JournalReader.h"
#include "reliability/reducer/SessionState.h"
#include "telemetry/ShotTelemetry.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <variant>

namespace ta {
namespace rms {
namespace node {

using namespace ta::rms::control;

TechAimNodeCommands::TechAimNodeCommands(QObject* parent)
    : QObject(parent)
{
}

void TechAimNodeCommands::setSessionControlArmed(bool armed)
{
    if (m_armed == armed)
        return;
    m_armed = armed;
    // A disarm drops any scheduled start with it. Leaving one armed to fire
    // after the operator has withdrawn permission is exactly the surprise this
    // gate exists to prevent.
    if (!m_armed)
        m_scheduledStartUtcMs = -1;
    emit armedChanged();
    emit intentChanged();
}

QStringList TechAimNodeCommands::capabilities() const
{
    // Always safe: both read and change nothing.
    QStringList caps{QLatin1String(cap::kStatus), QLatin1String(cap::kEventReplay)};
    if (m_armed) {
        caps << QLatin1String(cap::kAthleteAssignment)
             << QLatin1String(cap::kSessionPrepare)
             << QLatin1String(cap::kStartAt)
             << QLatin1String(cap::kStop);
    }
    // kPaperFeed is NEVER added. It moves physical hardware and does not become
    // reachable until a node adapter has been physically validated.
    return caps;
}

IControlCommandHandler::Result TechAimNodeCommands::statusResult() const
{
    Result r;
    r.accepted = true;
    r.reasonCode = QLatin1String(reason::kOk);
    QJsonObject s{{"nodeId", m_nodeId}, {"bootId", m_bootId},
                  {"laneId", m_laneHint},
                  {"athlete", m_athlete},
                  {"programmeId", m_programmeId},
                  {"sessionControlArmed", m_armed}};
    if (ta::rel::SessionStore* store = m_store ? m_store() : nullptr) {
        const ta::rel::SessionState& st = store->state();
        s.insert(QStringLiteral("sessionId"), st.sessionId);
        s.insert(QStringLiteral("sessionActive"), store->active());
        s.insert(QStringLiteral("officialShots"), int(st.officials.size()));
    } else {
        s.insert(QStringLiteral("sessionActive"), false);
    }
    if (m_scheduledStartUtcMs >= 0)
        s.insert(QStringLiteral("scheduledStartUtcMs"), double(m_scheduledStartUtcMs));
    r.resultingState = s;
    return r;
}

IControlCommandHandler::Result
TechAimNodeCommands::applyAssignAthlete(const Command& c)
{
    Result r;
    // An assignment is preparation, not a change to a match in progress.
    // Renaming the athlete under a session that is already collecting shots
    // would misattribute a competition record, so it is refused.
    if (ta::rel::SessionStore* store = m_store ? m_store() : nullptr) {
        if (store->active()) {
            r.accepted = false;
            r.reasonCode = QLatin1String(reason::kPreconditionFailed);
            r.message = QStringLiteral("a session is already running on this lane");
            return r;
        }
    }
    m_athlete = c.payload.value(QStringLiteral("athlete")).toString();
    if (!c.laneId.isEmpty())
        m_laneHint = c.laneId;
    emit intentChanged();
    r.accepted = true;
    r.reasonCode = QLatin1String(reason::kOk);
    r.resultingState = QJsonObject{{"athlete", m_athlete}, {"laneId", m_laneHint}};
    return r;
}

IControlCommandHandler::Result
TechAimNodeCommands::applyPrepareSession(const Command& c)
{
    Result r;
    if (!m_qual) {
        r.accepted = false;
        r.reasonCode = QLatin1String(reason::kPreconditionFailed);
        r.message = QStringLiteral("no qualification controller is attached");
        return r;
    }
    if (ta::rel::SessionStore* store = m_store ? m_store() : nullptr) {
        if (store->active()) {
            r.accepted = false;
            r.reasonCode = QLatin1String(reason::kPreconditionFailed);
            r.message = QStringLiteral("a session is already open on this lane");
            return r;
        }
    }
    // The EXISTING public transition. The reducer still validates it; nothing
    // here bypasses the rules that govern a session opening.
    m_qual->beginPreparation();
    r.accepted = true;
    r.reasonCode = QLatin1String(reason::kOk);
    r.resultingState = QJsonObject{{"phase", "PREPARATION"}, {"athlete", m_athlete}};
    return r;
}

IControlCommandHandler::Result
TechAimNodeCommands::applyStartAt(const Command& c)
{
    Result r;
    if (m_scheduledStartUtcMs >= 0) {
        // ALREADY SCHEDULED is refused, not silently re-based. A second
        // START_AT must never move a start an operator is already counting on.
        r.accepted = false;
        r.reasonCode = QLatin1String(reason::kPreconditionFailed);
        r.message = QStringLiteral("a start is already scheduled on this lane");
        return r;
    }
    const qint64 startAtRms =
        qint64(c.payload.value(QStringLiteral("startAtUtcMs")).toDouble());
    if (startAtRms <= 0) {
        r.accepted = false;
        r.reasonCode = QLatin1String(reason::kMalformed);
        r.message = QStringLiteral("START_AT carried no instant");
        return r;
    }
    m_rmsToNodeOffsetMs =
        qint64(c.payload.value(QStringLiteral("rmsToNodeOffsetMs")).toDouble());
    // The RMS instant converted onto THIS node's clock using the measured
    // offset. It does not start on arrival - starting on arrival would give
    // every lane its own delivery jitter as a head start.
    m_scheduledStartUtcMs = startAtRms + m_rmsToNodeOffsetMs;
    emit intentChanged();

    r.accepted = true;
    r.reasonCode = QLatin1String(reason::kOk);
    r.resultingState = QJsonObject{
        {"scheduledStartUtcMs", double(m_scheduledStartUtcMs)},
        {"rmsToNodeOffsetMs", double(m_rmsToNodeOffsetMs)}};
    return r;
}

IControlCommandHandler::Result TechAimNodeCommands::applyStop(const Command& c)
{
    Q_UNUSED(c);
    Result r;
    m_scheduledStartUtcMs = -1;
    emit intentChanged();
    if (!m_qual) {
        r.accepted = false;
        r.reasonCode = QLatin1String(reason::kPreconditionFailed);
        r.message = QStringLiteral("no qualification controller is attached");
        return r;
    }
    ta::rel::SessionStore* store = m_store ? m_store() : nullptr;
    if (!store || !store->active()) {
        // Nothing to stop. Reported as a refusal with a reason rather than a
        // success, so an operator is not told a lane was stopped that was
        // never running.
        r.accepted = false;
        r.reasonCode = QLatin1String(reason::kPreconditionFailed);
        r.message = QStringLiteral("no session is running on this lane");
        return r;
    }
    m_qual->completeMatch();
    r.accepted = true;
    r.reasonCode = QLatin1String(reason::kOk);
    r.resultingState = QJsonObject{{"phase", "COMPLETE"}};
    return r;
}

IControlCommandHandler::Result TechAimNodeCommands::apply(const Command& c)
{
    const QString t = c.commandType;
    if (t == QLatin1String(cmd::kRequestStatus)) return statusResult();
    if (t == QLatin1String(cmd::kAssignAthlete)) return applyAssignAthlete(c);
    if (t == QLatin1String(cmd::kPrepareSession)) return applyPrepareSession(c);
    if (t == QLatin1String(cmd::kStartAt))       return applyStartAt(c);
    if (t == QLatin1String(cmd::kStop))          return applyStop(c);

    Result r;
    r.accepted = false;
    r.reasonCode = QLatin1String(reason::kUnknownCommand);
    return r;
}

QList<QJsonObject> TechAimNodeCommands::replayEvents(const QString& sessionId,
                                                     int afterSequence,
                                                     int maxEvents,
                                                     bool* hasMoreOut)
{
    QList<QJsonObject> out;
    if (hasMoreOut) *hasMoreOut = false;
    if (m_journalDir.isEmpty())
        return out;

    // THE NODE'S OWN JOURNAL is the source - the same authoritative record the
    // live publisher reads from, never a UI model and never a re-read of the
    // target. Nothing is rescored on the way out.
    QDir dir(m_journalDir);
    const QStringList files =
        dir.entryList(QStringList{QStringLiteral("*.jsonl")}, QDir::Files, QDir::Name);

    for (const QString& name : files) {
        const ta::rel::JournalReadResult res =
            ta::rel::JournalReader::readFile(dir.filePath(name));
        if (!res.fileOk)
            continue;                 // an unreadable journal is skipped, not faked

        for (const ta::rel::JournalLine& line : res.lines) {
            if (!line.parsed)
                continue;
            const ta::rel::EventEnvelope& env = line.envelope;
            // A request for a different session returns nothing from this file:
            // answering the wrong session would put one lane's shots onto
            // another lane's ledger.
            if (!sessionId.isEmpty() && env.sessionId != sessionId)
                continue;
            if (!std::holds_alternative<ta::rel::ShotAccepted>(env.payload))
                continue;
            const ta::rel::ShotCore& core =
                std::get<ta::rel::ShotAccepted>(env.payload).shot;
            // Sighters carry shotNumber 0 and are not competition shots; they
            // are excluded here for the same reason the live path excludes them.
            if (core.shotNumber <= 0)
                continue;
            if (core.shotNumber <= afterSequence)
                continue;
            if (out.size() >= maxEvents) {
                if (hasMoreOut) *hasMoreOut = true;
                return out;
            }
            // THE SAME CONVERSION the live path uses, so a replayed shot is
            // byte-identical to the one broadcast when it was fired - which is
            // what lets RMS recognise it as a duplicate rather than insert it
            // twice. The timestamp is the journal's, not now: replay rebuilds
            // history and must not restate when the shot happened.
            const ta::rms::AcceptedShot shot =
                ta::telemetry::toAcceptedShot(core, env.sessionId, m_nodeId,
                                              m_bootId, m_laneHint, m_programmeId,
                                              env.monotonicMs);
            out.append(QJsonDocument::fromJson(ta::rms::encode(shot)).object());
        }
    }
    return out;
}

} // namespace node
} // namespace rms
} // namespace ta
