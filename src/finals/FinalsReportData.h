#pragma once
// 3P FINAL — immutable report data (Phase D1). Assembled from stored session
// state by FinalsReportBuilder; NO qualification-report assumptions (no six
// 10-shot series, no integer-primary totals, no 60-shot completion rules).

#include <QString>
#include <QVector>

namespace techaim {
namespace finals {

struct FinalsSessionSummary {
    QString athlete;
    QString eventName;       // "50m Rifle 3P FINAL (35)"
    QString dateTime;
    double cumulativeTotal = 0.0;
    int officialShotCount = 0;
    int sighterCount = 0;
    int missingCount = 0;
    int incidentCount = 0;
};

struct FinalsStageReport {
    int stageId = 0;
    QString label;            // KNEELING / PRONE / SERIES 1 / ... / SHOT 35
    int expected = 0;
    int fired = 0;
    double subtotal = 0.0;
    int status = 0;           // StageStatus
    QString statusName;
};

struct FinalsShotReport {
    int number = 0;           // official 1-35; provisional rows use expected number
    int stageId = 0;
    QString stageLabel;
    double score = 0.0;
    QString direction;
    int timeUsedSec = 0;
    double xMm = 0.0;
    double yMm = 0.0;
    int seriesIndex = -1;     // approved schema: 0 K, 1 P, 2 S1, 3 S2, 4-8 singles
    bool provisional = false; // [P1=B] DNS / not fired — 0.0 provisional
};

struct FinalsMissingShotReport {
    int expectedNumber = 0;
    int stageId = 0;
    QString stageLabel;
    QString reason;           // TimeExpired
};

struct FinalsIncidentReport {
    QString reason;
    QString displayText;
    QString stageLabel;
    QString timestamp;
    int windowId = 0;
    qint64 externalId = -1;
};

struct FinalsTimelineEvent {
    int sequence = 0;
    QString type;             // command typeName
    QString text;
    QString stage;
    qint64 issuedAtMs = 0;    // scaled ms since final start
};

struct FinalsReportData {
    FinalsSessionSummary summary;
    QVector<FinalsStageReport> stages;
    QVector<FinalsShotReport> officialShots;   // incl. provisional DNS rows, ordered 1-35
    QVector<FinalsMissingShotReport> missingShots;
    QVector<FinalsIncidentReport> incidents;
    QVector<FinalsTimelineEvent> timeline;
    QString completionStatus;                  // COMPLETE / INCOMPLETE / ABORTED
};

} // namespace finals
} // namespace techaim
