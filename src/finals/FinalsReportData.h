#pragma once
// 3P FINAL — immutable report data (Phase D1; expanded for the professional
// report redesign). Assembled from stored session state by FinalsReportBuilder;
// NO qualification-report assumptions (no six 10-shot series, no
// integer-primary totals, no 60-shot completion rules).
//
// DATA-SOURCE RULE: every displayed value originates HERE. QML never derives —
// it only formats. If a value is missing, extend this model and the builder.

#include <QString>
#include <QVector>

namespace techaim {
namespace finals {

struct FinalsSessionSummary {
    QString athlete;
    QString eventName;       // "50m Rifle 3P FINAL (35)"
    QString dateTime;
    QString sessionType;     // "Final (Training)" — competitions later (RMS)
    QString softwareVersion;
    QString lane;            // future (RMS) — empty until wired
    QString targetId;        // future (RMS) — empty until wired
    double cumulativeTotal = 0.0;
    double averageShot = 0.0;
    double highestShot = 0.0;
    int highestShotNumber = 0;
    double lowestShot = 0.0;
    int lowestShotNumber = 0;
    int innerTens = 0;       // 50m rifle inner-ten convention (>= 10.2)
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
    double average = 0.0;     // over real (non-provisional) shots; 0 when none
    double bestShot = 0.0;
    double worstShot = 0.0;
    int innerTens = 0;
    int timeUsedSec = 0;      // sum of per-shot splits in this stage
    int status = 0;           // StageStatus
    QString statusName;
};

struct FinalsShotReport {
    int number = 0;           // official 1-35; provisional rows use expected number
    int stageId = 0;
    QString stageLabel;
    double score = 0.0;
    double runningTotal = 0.0; // cumulative over real shots (flat across DNS rows)
    QString direction;
    int timeUsedSec = 0;      // per-shot split (FIX-R3 semantics)
    int holdSec = -1;         // hold time — future hardware feature; -1 = n/a
    double xMm = 0.0;
    double yMm = 0.0;
    int seriesIndex = -1;     // approved schema: 0 K, 1 P, 2 S1, 3 S2, 4-8 singles
    bool innerTen = false;
    bool provisional = false; // [P1=B] DNS / not fired — 0.0 provisional
    QString note;             // "DNS — not fired" etc.
};

// Match-validation panel: each check is a verdict the builder derived from
// stored session data (never recomputed in QML).
struct FinalsValidationCheck {
    QString label;
    bool ok = false;
    QString detail;           // shown when a check fails (e.g. timeout reason)
};

struct FinalsPerformanceSummary {
    double highestShot = 0.0;  int highestShotNumber = 0;
    double lowestShot = 0.0;   int lowestShotNumber = 0;
    int longestSplitSec = 0;   int longestSplitShot = 0;
    int shortestSplitSec = 0;  int shortestSplitShot = 0;
    QString fastestStage;      int fastestStageSec = 0;
    QString slowestStage;      int slowestStageSec = 0;
    double standingAverage = 0.0;
    double overallAverage = 0.0;
    bool available = false;    // false until at least one real shot exists
};

// Horizontal-bar position comparison (K / P / Standing combined).
struct FinalsPositionComparison {
    QString label;
    double score = 0.0;
    int expectedShots = 0;
    int firedShots = 0;
    int barPct = 0;            // bar width relative to the best position (100)
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
    QString severity;         // Information / Warning / Critical (builder-derived)
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
    QVector<FinalsValidationCheck> validation; // MATCH VALIDATION panel
    bool reportValid = false;                  // all validation checks ok
    FinalsPerformanceSummary performance;
    QVector<FinalsPositionComparison> positionComparison; // K / P / Standing
    QString completionStatus;                  // COMPLETE / INCOMPLETE / ABORTED
};

} // namespace finals
} // namespace techaim
