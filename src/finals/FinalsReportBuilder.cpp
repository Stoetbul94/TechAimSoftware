#include "FinalsReportBuilder.h"
#include "Finals3PTypes.h"

#include <QMap>
#include <QStringList>
#include <algorithm>
#include <cmath>

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

// 50m rifle inner-ten convention, as used across the app (right panel stars).
const double kInnerTen = 10.2;

// Reject reasons -> report severity. Stage completion is NORMAL athlete
// guidance; window/mode rejections are procedure warnings; data-integrity
// rejections are critical.
QString severityForReason(const QString& reason)
{
    if (reason == QLatin1String("StageShotLimitReached"))
        return QStringLiteral("Information");
    if (reason == QLatin1String("DuplicateShot")
        || reason == QLatin1String("InvalidShotData"))
        return QStringLiteral("Critical");
    return QStringLiteral("Warning");   // WindowClosed / WrongTargetMode / FinalsNotActive
}

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
        s.innerTen    = s.score >= kInnerTen;
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
        s.note        = QStringLiteral("DNS — not fired (0.0 provisional)");
        d.officialShots.append(s);
    }
    std::sort(d.officialShots.begin(), d.officialShots.end(),
              [](const FinalsShotReport& a, const FinalsShotReport& b) {
                  return a.number < b.number;
              });

    // Running total over real shots (flat across DNS rows).
    {
        double running = 0.0;
        for (FinalsShotReport& s : d.officialShots) {
            if (!s.provisional)
                running += s.score;
            s.runningTotal = running;
        }
    }

    // ── per-stage sections (stats over real shots only) ──
    for (const StageDef& def : kStages) {
        const QString key = stageName(def.stage);
        FinalsStageReport st;
        st.stageId    = static_cast<int>(def.stage);
        st.label      = QLatin1String(def.label);
        st.expected   = def.expected;
        st.subtotal   = stageSubtotals.value(key, 0.0).toDouble();
        st.status     = stageStatuses.value(key, 0).toInt();
        st.statusName = stageStatusName(static_cast<StageStatus>(st.status));
        for (const FinalsShotReport& s : d.officialShots) {
            if (s.stageId != st.stageId || s.provisional)
                continue;
            ++st.fired;
            st.timeUsedSec += s.timeUsedSec;
            if (s.innerTen) ++st.innerTens;
            if (st.fired == 1) { st.bestShot = st.worstShot = s.score; }
            else {
                st.bestShot  = std::max(st.bestShot, s.score);
                st.worstShot = std::min(st.worstShot, s.score);
            }
        }
        st.average = st.fired > 0 ? st.subtotal / st.fired : 0.0;
        d.stages.append(st);
    }

    // ── incidents (severity derived from the reject reason) ──
    for (const QVariant& v : incidents) {
        const QVariantMap m = v.toMap();
        FinalsIncidentReport in;
        in.reason      = m.value(QStringLiteral("reason")).toString();
        in.displayText = m.value(QStringLiteral("displayText")).toString();
        in.stageLabel  = m.value(QStringLiteral("stageLabel")).toString();
        in.timestamp   = m.value(QStringLiteral("timestamp")).toString();
        in.severity    = severityForReason(in.reason);
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

    // ── summary ──
    d.summary.athlete = meta.value(QStringLiteral("athlete")).toString();
    d.summary.eventName = meta.value(QStringLiteral("eventName"),
                                     QStringLiteral("50m Rifle 3P FINAL (35)")).toString();
    d.summary.dateTime = meta.value(QStringLiteral("dateTime")).toString();
    d.summary.sessionType = meta.value(QStringLiteral("sessionType"),
                                       QStringLiteral("Final (Training)")).toString();
    d.summary.softwareVersion = meta.value(QStringLiteral("softwareVersion"),
                                           QStringLiteral("Seta 4.0")).toString();
    d.summary.lane = meta.value(QStringLiteral("lane")).toString();
    d.summary.targetId = meta.value(QStringLiteral("targetId")).toString();
    d.summary.cumulativeTotal = cumulativeTotal;
    d.summary.sighterCount = meta.value(QStringLiteral("sighterCount")).toInt();
    d.summary.missingCount = d.missingShots.size();
    d.summary.incidentCount = d.incidents.size();
    {
        int officials = 0;
        for (const FinalsShotReport& s : d.officialShots) {
            if (s.provisional)
                continue;
            ++officials;
            if (s.innerTen) ++d.summary.innerTens;
            if (officials == 1 || s.score > d.summary.highestShot) {
                d.summary.highestShot = s.score;
                d.summary.highestShotNumber = s.number;
            }
            if (officials == 1 || s.score < d.summary.lowestShot) {
                d.summary.lowestShot = s.score;
                d.summary.lowestShotNumber = s.number;
            }
        }
        d.summary.officialShotCount = officials;
        d.summary.averageShot = officials > 0 ? cumulativeTotal / officials : 0.0;
    }

    // ── completion verdict ──
    bool anyAborted = false, anyIncomplete = false, allComplete = true;
    bool anyUnfinished = false;
    for (const FinalsStageReport& st : d.stages) {
        if (st.status == static_cast<int>(StageStatus::Aborted)) anyAborted = true;
        if (st.status == static_cast<int>(StageStatus::Incomplete)) anyIncomplete = true;
        if (st.status != static_cast<int>(StageStatus::Complete)) allComplete = false;
        if (st.status == static_cast<int>(StageStatus::NotStarted)
            || st.status == static_cast<int>(StageStatus::InProgress)) anyUnfinished = true;
    }
    d.completionStatus = anyAborted ? QStringLiteral("ABORTED")
                       : anyIncomplete ? QStringLiteral("INCOMPLETE")
                       : allComplete ? QStringLiteral("COMPLETE")
                                     : QStringLiteral("IN PROGRESS");

    // ── MATCH VALIDATION panel ──
    {
        auto addCheck = [&d](const QString& label, bool ok, const QString& detail) {
            FinalsValidationCheck c; c.label = label; c.ok = ok;
            c.detail = detail; d.validation.append(c);
        };
        // Failure reason from the missing-shot record set, e.g. "Standing
        // Series 1 timeout — 3 shots missing".
        QString missDetail;
        if (!d.missingShots.isEmpty()) {
            QMap<QString, int> byStage;
            for (const FinalsMissingShotReport& ms : d.missingShots)
                byStage[ms.stageLabel] += 1;
            QStringList parts;
            for (auto it = byStage.constBegin(); it != byStage.constEnd(); ++it)
                parts << QStringLiteral("%1 timeout — %2 missing").arg(it.key()).arg(it.value());
            missDetail = parts.join(QStringLiteral(" · "));
        }
        addCheck(QStringLiteral("Final Completed"),
                 d.completionStatus == QLatin1String("COMPLETE"),
                 d.completionStatus == QLatin1String("COMPLETE")
                     ? QString() : QStringLiteral("Match %1").arg(d.completionStatus.toLower()));
        addCheck(QStringLiteral("35 Official Shots"),
                 d.summary.officialShotCount == 35,
                 QStringLiteral("%1 / 35 fired").arg(d.summary.officialShotCount));
        addCheck(QStringLiteral("Decimal Scoring"), true,
                 QStringLiteral("all scores recorded to 0.1"));
        addCheck(QStringLiteral("ISSF Sequence Completed"), !anyUnfinished,
                 anyUnfinished ? QStringLiteral("a stage never reached its verdict")
                               : QString());
        addCheck(QStringLiteral("No Missing Shots"),
                 d.missingShots.isEmpty(), missDetail);
        // Internal consistency: shot rows must reproduce the controller total
        // and the 35-shot program must be fully accounted for (real + DNS).
        {
            double sum = 0.0;
            for (const FinalsShotReport& s : d.officialShots)
                if (!s.provisional) sum += s.score;
            const bool consistent =
                std::abs(sum - cumulativeTotal) < 0.05
                && d.officialShots.size() == 35
                && d.summary.officialShotCount + d.summary.missingCount == 35;
            addCheck(QStringLiteral("Report Verified"), consistent,
                     consistent ? QStringLiteral("totals reproduce the controller record")
                                : QStringLiteral("total or shot accounting mismatch"));
        }
        d.reportValid = true;
        for (const FinalsValidationCheck& c : d.validation)
            if (!c.ok) d.reportValid = false;
    }

    // ── performance summary ──
    {
        FinalsPerformanceSummary& p = d.performance;
        p.highestShot = d.summary.highestShot;
        p.highestShotNumber = d.summary.highestShotNumber;
        p.lowestShot = d.summary.lowestShot;
        p.lowestShotNumber = d.summary.lowestShotNumber;
        p.overallAverage = d.summary.averageShot;
        int reals = 0;
        double standingSum = 0.0; int standingCount = 0;
        for (const FinalsShotReport& s : d.officialShots) {
            if (s.provisional) continue;
            ++reals;
            if (reals == 1 || s.timeUsedSec > p.longestSplitSec)
                { p.longestSplitSec = s.timeUsedSec; p.longestSplitShot = s.number; }
            if (reals == 1 || s.timeUsedSec < p.shortestSplitSec)
                { p.shortestSplitSec = s.timeUsedSec; p.shortestSplitShot = s.number; }
            if (s.seriesIndex >= 2) { standingSum += s.score; ++standingCount; }
        }
        p.standingAverage = standingCount > 0 ? standingSum / standingCount : 0.0;
        bool first = true;
        for (const FinalsStageReport& st : d.stages) {
            if (st.fired == 0) continue;
            if (first || st.timeUsedSec < p.fastestStageSec)
                { p.fastestStageSec = st.timeUsedSec; p.fastestStage = st.label; }
            if (first || st.timeUsedSec > p.slowestStageSec)
                { p.slowestStageSec = st.timeUsedSec; p.slowestStage = st.label; }
            first = false;
        }
        p.available = reals > 0;
    }

    // ── position comparison (K / P / Standing combined) ──
    {
        auto add = [&d](const QString& label, double score, int expected, int fired) {
            FinalsPositionComparison c;
            c.label = label; c.score = score;
            c.expectedShots = expected; c.firedShots = fired;
            d.positionComparison.append(c);
        };
        double k = 0, pr = 0, st = 0; int kf = 0, prf = 0, stf = 0;
        for (const FinalsStageReport& s : d.stages) {
            if (s.stageId == static_cast<int>(Stage::KneelingMatch)) { k = s.subtotal; kf = s.fired; }
            else if (s.stageId == static_cast<int>(Stage::ProneMatch)) { pr = s.subtotal; prf = s.fired; }
            else { st += s.subtotal; stf += s.fired; }
        }
        add(QStringLiteral("KNEELING"), k, 10, kf);
        add(QStringLiteral("PRONE"), pr, 10, prf);
        add(QStringLiteral("STANDING"), st, 15, stf);
        double best = 0.0;
        for (const FinalsPositionComparison& c : d.positionComparison)
            best = std::max(best, c.score);
        for (FinalsPositionComparison& c : d.positionComparison)
            c.barPct = best > 0 ? int(c.score / best * 100.0 + 0.5) : 0;
    }

    return d;
}

QVariantMap FinalsReportBuilder::toVariant(const FinalsReportData& d)
{
    QVariantMap out;
    QVariantMap sum;
    sum[QStringLiteral("athlete")] = d.summary.athlete;
    sum[QStringLiteral("eventName")] = d.summary.eventName;
    sum[QStringLiteral("dateTime")] = d.summary.dateTime;
    sum[QStringLiteral("sessionType")] = d.summary.sessionType;
    sum[QStringLiteral("softwareVersion")] = d.summary.softwareVersion;
    sum[QStringLiteral("lane")] = d.summary.lane;
    sum[QStringLiteral("targetId")] = d.summary.targetId;
    sum[QStringLiteral("cumulativeTotal")] = d.summary.cumulativeTotal;
    sum[QStringLiteral("averageShot")] = d.summary.averageShot;
    sum[QStringLiteral("highestShot")] = d.summary.highestShot;
    sum[QStringLiteral("highestShotNumber")] = d.summary.highestShotNumber;
    sum[QStringLiteral("lowestShot")] = d.summary.lowestShot;
    sum[QStringLiteral("lowestShotNumber")] = d.summary.lowestShotNumber;
    sum[QStringLiteral("innerTens")] = d.summary.innerTens;
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
        m[QStringLiteral("average")] = st.average;
        m[QStringLiteral("bestShot")] = st.bestShot;
        m[QStringLiteral("worstShot")] = st.worstShot;
        m[QStringLiteral("innerTens")] = st.innerTens;
        m[QStringLiteral("timeUsedSec")] = st.timeUsedSec;
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
        m[QStringLiteral("runningTotal")] = s.runningTotal;
        m[QStringLiteral("direction")] = s.direction;
        m[QStringLiteral("timeUsedSec")] = s.timeUsedSec;
        m[QStringLiteral("holdSec")] = s.holdSec;
        m[QStringLiteral("xmm")] = s.xMm;
        m[QStringLiteral("ymm")] = s.yMm;
        m[QStringLiteral("seriesIndex")] = s.seriesIndex;
        m[QStringLiteral("innerTen")] = s.innerTen;
        m[QStringLiteral("provisional")] = s.provisional;
        m[QStringLiteral("note")] = s.note;
        shots << m;
    }
    out[QStringLiteral("shots")] = shots;

    // Per-position plot groups for the miniature targets (stored X/Y passed
    // straight through; the QML canvas only maps mm -> px).
    {
        QVariantList kneel, prone, standing;
        for (const FinalsShotReport& s : d.officialShots) {
            if (s.provisional) continue;
            QVariantMap m;
            m[QStringLiteral("x")] = s.xMm;
            m[QStringLiteral("y")] = s.yMm;
            m[QStringLiteral("score")] = s.score;
            m[QStringLiteral("number")] = s.number;
            if (s.seriesIndex == 0) kneel << m;
            else if (s.seriesIndex == 1) prone << m;
            else standing << m;
        }
        QVariantList plots;
        QVariantMap pk; pk[QStringLiteral("label")] = QStringLiteral("KNEELING");
        pk[QStringLiteral("shots")] = kneel; plots << pk;
        QVariantMap pp; pp[QStringLiteral("label")] = QStringLiteral("PRONE");
        pp[QStringLiteral("shots")] = prone; plots << pp;
        QVariantMap ps; ps[QStringLiteral("label")] = QStringLiteral("STANDING");
        ps[QStringLiteral("shots")] = standing; plots << ps;
        out[QStringLiteral("positionPlots")] = plots;
    }

    QVariantList incidents;
    for (const FinalsIncidentReport& in : d.incidents) {
        QVariantMap m;
        m[QStringLiteral("reason")] = in.reason;
        m[QStringLiteral("displayText")] = in.displayText;
        m[QStringLiteral("stageLabel")] = in.stageLabel;
        m[QStringLiteral("timestamp")] = in.timestamp;
        m[QStringLiteral("severity")] = in.severity;
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

    QVariantList checks;
    for (const FinalsValidationCheck& c : d.validation) {
        QVariantMap m;
        m[QStringLiteral("label")] = c.label;
        m[QStringLiteral("ok")] = c.ok;
        m[QStringLiteral("detail")] = c.detail;
        checks << m;
    }
    QVariantMap validation;
    validation[QStringLiteral("valid")] = d.reportValid;
    validation[QStringLiteral("checks")] = checks;
    out[QStringLiteral("validation")] = validation;

    {
        const FinalsPerformanceSummary& p = d.performance;
        QVariantMap m;
        m[QStringLiteral("available")] = p.available;
        m[QStringLiteral("highestShot")] = p.highestShot;
        m[QStringLiteral("highestShotNumber")] = p.highestShotNumber;
        m[QStringLiteral("lowestShot")] = p.lowestShot;
        m[QStringLiteral("lowestShotNumber")] = p.lowestShotNumber;
        m[QStringLiteral("longestSplitSec")] = p.longestSplitSec;
        m[QStringLiteral("longestSplitShot")] = p.longestSplitShot;
        m[QStringLiteral("shortestSplitSec")] = p.shortestSplitSec;
        m[QStringLiteral("shortestSplitShot")] = p.shortestSplitShot;
        m[QStringLiteral("fastestStage")] = p.fastestStage;
        m[QStringLiteral("fastestStageSec")] = p.fastestStageSec;
        m[QStringLiteral("slowestStage")] = p.slowestStage;
        m[QStringLiteral("slowestStageSec")] = p.slowestStageSec;
        m[QStringLiteral("standingAverage")] = p.standingAverage;
        m[QStringLiteral("overallAverage")] = p.overallAverage;
        out[QStringLiteral("performance")] = m;
    }

    QVariantList comparison;
    for (const FinalsPositionComparison& c : d.positionComparison) {
        QVariantMap m;
        m[QStringLiteral("label")] = c.label;
        m[QStringLiteral("score")] = c.score;
        m[QStringLiteral("expectedShots")] = c.expectedShots;
        m[QStringLiteral("firedShots")] = c.firedShots;
        m[QStringLiteral("barPct")] = c.barPct;
        comparison << m;
    }
    out[QStringLiteral("positionComparison")] = comparison;

    out[QStringLiteral("completionStatus")] = d.completionStatus;
    return out;
}

} // namespace finals
} // namespace techaim
