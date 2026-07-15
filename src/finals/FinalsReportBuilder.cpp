#include "FinalsReportBuilder.h"
#include "Finals3PTypes.h"

#include <algorithm>

namespace techaim {
namespace finals {

namespace {

struct StageDef { Stage stage; const char* label; int expected; };

// The nine official finals stages, in program order (approved schema).
const StageDef kStages[] = {
    { Stage::KneelingMatch,   "KNEELING",  10 },
    { Stage::ProneMatch,      "PRONE",     10 },
    { Stage::StandingSeries1, "SERIES 1",   5 },
    { Stage::StandingSeries2, "SERIES 2",   5 },
    { Stage::StandingSingle1, "SHOT 31",    1 },
    { Stage::StandingSingle2, "SHOT 32",    1 },
    { Stage::StandingSingle3, "SHOT 33",    1 },
    { Stage::StandingSingle4, "SHOT 34",    1 },
    { Stage::StandingSingle5, "SHOT 35",    1 },
};

} // namespace

FinalsReportData FinalsReportBuilder::build(const QVariantList& officialShots,
                                            const QVariantList& missingShots,
                                            const QVariantList& incidents,
                                            const QVariantMap& stageStatuses,
                                            const QVariantMap& stageSubtotals,
                                            double cumulativeTotal,
                                            const QVariantList& commandEvents,
                                            const QVariantMap& meta)
{
    FinalsReportData d;

    // ── official shots (real detections only; ordered by official number) ──
    for (const QVariant& v : officialShots) {
        const QVariantMap m = v.toMap();
        FinalsShotReport s;
        s.number      = m.value(QStringLiteral("finalsShotNumber")).toInt();
        s.stageId     = m.value(QStringLiteral("finalsStageId")).toInt();
        s.stageLabel  = m.value(QStringLiteral("finalsStageLabel")).toString();
        s.score       = m.value(QStringLiteral("calculatedscore")).toDouble();
        s.direction   = m.value(QStringLiteral("direction")).toString();
        s.timeUsedSec = m.value(QStringLiteral("timeComsumed")).toInt();
        s.xMm         = m.value(QStringLiteral("xmm")).toDouble();
        s.yMm         = m.value(QStringLiteral("ymm")).toDouble();
        s.seriesIndex = m.value(QStringLiteral("finalsSeriesIndex")).toInt();
        s.provisional = false;
        d.officialShots.append(s);
    }

    // ── [P1 = Option B] DNS rows from MissingShot records (report layer
    //    only — never synthetic model rows) ──
    for (const QVariant& v : missingShots) {
        const QVariantMap m = v.toMap();
        FinalsMissingShotReport ms;
        ms.expectedNumber = m.value(QStringLiteral("expectedShotNumber")).toInt();
        ms.stageId        = m.value(QStringLiteral("stageId")).toInt();
        ms.stageLabel     = m.value(QStringLiteral("stageLabel")).toString();
        ms.reason         = m.value(QStringLiteral("reason")).toString();
        d.missingShots.append(ms);

        FinalsShotReport s;
        s.number      = ms.expectedNumber;
        s.stageId     = ms.stageId;
        s.stageLabel  = ms.stageLabel;
        s.score       = 0.0;
        s.seriesIndex = seriesIndexFor(static_cast<Stage>(ms.stageId));
        s.provisional = true;
        d.officialShots.append(s);
    }
    std::sort(d.officialShots.begin(), d.officialShots.end(),
              [](const FinalsShotReport& a, const FinalsShotReport& b) {
                  return a.number < b.number;
              });

    // ── per-stage sections ──
    for (const StageDef& def : kStages) {
        const QString key = stageName(def.stage);
        FinalsStageReport st;
        st.stageId    = static_cast<int>(def.stage);
        st.label      = QLatin1String(def.label);
        st.expected   = def.expected;
        st.subtotal   = stageSubtotals.value(key, 0.0).toDouble();
        st.status     = stageStatuses.value(key, 0).toInt();
        st.statusName = stageStatusName(static_cast<StageStatus>(st.status));
        for (const FinalsShotReport& s : d.officialShots)
            if (s.stageId == st.stageId && !s.provisional)
                ++st.fired;
        d.stages.append(st);
    }

    // ── incidents ──
    for (const QVariant& v : incidents) {
        const QVariantMap m = v.toMap();
        FinalsIncidentReport in;
        in.reason      = m.value(QStringLiteral("reason")).toString();
        in.displayText = m.value(QStringLiteral("displayText")).toString();
        in.stageLabel  = m.value(QStringLiteral("stageLabel")).toString();
        in.timestamp   = m.value(QStringLiteral("timestamp")).toString();
        in.windowId    = m.value(QStringLiteral("finalsWindowId")).toInt();
        in.externalId  = m.value(QStringLiteral("externalShotId")).toLongLong();
        d.incidents.append(in);
    }

    // ── command timeline (controller order) ──
    for (const QVariant& v : commandEvents) {
        const QVariantMap m = v.toMap();
        FinalsTimelineEvent t;
        t.sequence   = m.value(QStringLiteral("sequenceNumber")).toInt();
        t.type       = m.value(QStringLiteral("typeName")).toString();
        t.text       = m.value(QStringLiteral("text")).toString();
        t.stage      = m.value(QStringLiteral("stage")).toString();
        t.issuedAtMs = m.value(QStringLiteral("issuedAt")).toLongLong();
        d.timeline.append(t);
    }

    // ── summary + completion verdict ──
    d.summary.athlete = meta.value(QStringLiteral("athlete")).toString();
    d.summary.eventName = meta.value(QStringLiteral("eventName"),
                                     QStringLiteral("50m Rifle 3P FINAL (35)")).toString();
    d.summary.dateTime = meta.value(QStringLiteral("dateTime")).toString();
    d.summary.cumulativeTotal = cumulativeTotal;
    d.summary.sighterCount = meta.value(QStringLiteral("sighterCount")).toInt();
    d.summary.missingCount = d.missingShots.size();
    d.summary.incidentCount = d.incidents.size();
    int officials = 0;
    for (const FinalsShotReport& s : d.officialShots)
        if (!s.provisional) ++officials;
    d.summary.officialShotCount = officials;

    bool anyAborted = false, anyIncomplete = false, allComplete = true;
    for (const FinalsStageReport& st : d.stages) {
        if (st.status == static_cast<int>(StageStatus::Aborted)) anyAborted = true;
        if (st.status == static_cast<int>(StageStatus::Incomplete)) anyIncomplete = true;
        if (st.status != static_cast<int>(StageStatus::Complete)) allComplete = false;
    }
    d.completionStatus = anyAborted ? QStringLiteral("ABORTED")
                       : anyIncomplete ? QStringLiteral("INCOMPLETE")
                       : allComplete ? QStringLiteral("COMPLETE")
                                     : QStringLiteral("IN PROGRESS");
    return d;
}

QVariantMap FinalsReportBuilder::toVariant(const FinalsReportData& d)
{
    QVariantMap out;
    QVariantMap sum;
    sum[QStringLiteral("athlete")] = d.summary.athlete;
    sum[QStringLiteral("eventName")] = d.summary.eventName;
    sum[QStringLiteral("dateTime")] = d.summary.dateTime;
    sum[QStringLiteral("cumulativeTotal")] = d.summary.cumulativeTotal;
    sum[QStringLiteral("officialShotCount")] = d.summary.officialShotCount;
    sum[QStringLiteral("sighterCount")] = d.summary.sighterCount;
    sum[QStringLiteral("missingCount")] = d.summary.missingCount;
    sum[QStringLiteral("incidentCount")] = d.summary.incidentCount;
    out[QStringLiteral("summary")] = sum;

    QVariantList stages;
    for (const FinalsStageReport& st : d.stages) {
        QVariantMap m;
        m[QStringLiteral("stageId")] = st.stageId;
        m[QStringLiteral("label")] = st.label;
        m[QStringLiteral("expected")] = st.expected;
        m[QStringLiteral("fired")] = st.fired;
        m[QStringLiteral("subtotal")] = st.subtotal;
        m[QStringLiteral("status")] = st.status;
        m[QStringLiteral("statusName")] = st.statusName;
        stages << m;
    }
    out[QStringLiteral("stages")] = stages;

    QVariantList shots;
    for (const FinalsShotReport& s : d.officialShots) {
        QVariantMap m;
        m[QStringLiteral("number")] = s.number;
        m[QStringLiteral("stageLabel")] = s.stageLabel;
        m[QStringLiteral("score")] = s.score;
        m[QStringLiteral("direction")] = s.direction;
        m[QStringLiteral("timeUsedSec")] = s.timeUsedSec;
        m[QStringLiteral("xmm")] = s.xMm;
        m[QStringLiteral("ymm")] = s.yMm;
        m[QStringLiteral("seriesIndex")] = s.seriesIndex;
        m[QStringLiteral("provisional")] = s.provisional;
        shots << m;
    }
    out[QStringLiteral("shots")] = shots;

    QVariantList incidents;
    for (const FinalsIncidentReport& in : d.incidents) {
        QVariantMap m;
        m[QStringLiteral("reason")] = in.reason;
        m[QStringLiteral("displayText")] = in.displayText;
        m[QStringLiteral("stageLabel")] = in.stageLabel;
        m[QStringLiteral("timestamp")] = in.timestamp;
        incidents << m;
    }
    out[QStringLiteral("incidents")] = incidents;

    QVariantList timeline;
    for (const FinalsTimelineEvent& t : d.timeline) {
        QVariantMap m;
        m[QStringLiteral("sequence")] = t.sequence;
        m[QStringLiteral("type")] = t.type;
        m[QStringLiteral("text")] = t.text;
        m[QStringLiteral("stage")] = t.stage;
        m[QStringLiteral("issuedAtMs")] = t.issuedAtMs;
        timeline << m;
    }
    out[QStringLiteral("timeline")] = timeline;
    out[QStringLiteral("completionStatus")] = d.completionStatus;
    return out;
}

} // namespace finals
} // namespace techaim
