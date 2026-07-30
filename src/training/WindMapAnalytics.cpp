#include "WindMapAnalytics.h"

#include <QtGlobal>
#include <algorithm>
#include <cmath>

namespace ta {
namespace training {

using ta::rel::SessionState;
using ta::rel::WindMapShotRecord;

// ── labels ─────────────────────────────────────────────────────────────────

QString evidenceLabel(Evidence e)
{
    switch (e) {
    case Evidence::Insufficient: return QStringLiteral("Insufficient sample");
    case Evidence::Indicative:   return QStringLiteral("Indicative");
    case Evidence::Sufficient:   return QStringLiteral("Sufficient");
    case Evidence::Strong:       return QStringLiteral("Strong");
    }
    return QString();
}

QString referenceKindLabel(ReferenceKind k)
{
    switch (k) {
    case ReferenceKind::None:          return QStringLiteral("None");
    case ReferenceKind::CalmGroup:     return QStringLiteral("Calm group");
    case ReferenceKind::MostPopulated: return QStringLiteral("Most populated condition");
    case ReferenceKind::SessionCentre: return QStringLiteral("Session centre");
    case ReferenceKind::CoachSelected: return QStringLiteral("Coach-selected condition");
    }
    return QString();
}

QString patternCategoryLabel(PatternCategory c)
{
    switch (c) {
    case PatternCategory::InsufficientSample:         return QStringLiteral("Insufficient sample");
    case PatternCategory::TightButOffset:             return QStringLiteral("Tight but offset");
    case PatternCategory::WiderUnderCondition:        return QStringLiteral("Wider under a condition");
    case PatternCategory::SimilarAcrossConditions:    return QStringLiteral("Similar across conditions");
    case PatternCategory::WideAcrossAllConditions:    return QStringLiteral("Wide across all conditions");
    case PatternCategory::PositionSpecificDifference: return QStringLiteral("Position-specific difference");
    }
    return QString();
}

QString findingScopeLabel(FindingScope s)
{
    switch (s) {
    case FindingScope::Session:   return QStringLiteral("Session");
    case FindingScope::Position:  return QStringLiteral("Position");
    case FindingScope::Condition: return QStringLiteral("Condition");
    }
    return QString();
}

// ── ConditionKey ───────────────────────────────────────────────────────────

bool ConditionKey::operator==(const ConditionKey& o) const
{
    if (hasReading != o.hasReading) return false;
    if (!hasReading) return true;              // every "no reading" is one group
    if (calm != o.calm) return false;
    if (calm) return true;                     // every recorded calm is one group
    return directionDegrees == o.directionDegrees
        && speedHundredthMs == o.speedHundredthMs
        && sector == o.sector && band == o.band;
}

QString ConditionKey::label(GroupingMode mode) const
{
    // Calm and No reading are ALWAYS distinct groups, in every mode.
    if (!hasReading) return QStringLiteral("No reading");
    if (calm)        return QStringLiteral("Calm");
    switch (mode) {
    case GroupingMode::Direction:
        return windSectorLabel(static_cast<WindSector>(sector));
    case GroupingMode::SpeedBand:
        switch (static_cast<WindSpeedBand>(band)) {
        case WindSpeedBand::Light:      return QStringLiteral("0-2.0 m/s");
        case WindSpeedBand::Moderate:   return QStringLiteral("2.0-4.0 m/s");
        case WindSpeedBand::Strong:     return QStringLiteral("4.0-7.0 m/s");
        case WindSpeedBand::VeryStrong: return QStringLiteral("over 7.0 m/s");
        default:                        return QStringLiteral("Calm");
        }
    case GroupingMode::ExactCondition:
        return QStringLiteral("%1 · %2 m/s")
            .arg(windSectorLabel(static_cast<WindSector>(sector)),
                 QString::number(hundredthsToMetresPerSecond(speedHundredthMs), 'f', 1));
    }
    return QString();
}

ConditionKey WindMapAnalyticsEngine::keyFor(const WindConditionSnapshot& w, GroupingMode mode)
{
    ConditionKey k;
    k.hasReading = w.valid;
    k.calm = w.calm;
    if (!w.valid || w.calm)
        return k;                              // no direction/speed is meaningful
    k.sector = static_cast<qint8>(w.sector());
    k.band = static_cast<qint8>(w.band());
    if (mode == GroupingMode::ExactCondition) {
        k.directionDegrees = w.directionDegrees;
        k.speedHundredthMs = w.speedHundredthMs;
    } else if (mode == GroupingMode::Direction) {
        k.band = -1;                           // speed is not part of this identity
    } else {
        k.sector = -1;                         // direction is not part of this identity
    }
    return k;
}

// ── GroupStats ─────────────────────────────────────────────────────────────

int GroupStats::shotsNeededForMpi() const
{
    return n >= kMinSamplesMpi ? 0 : kMinSamplesMpi - n;
}

int GroupStats::shotsNeededForDispersion() const
{
    return n >= kMinSamplesDispersion ? 0 : kMinSamplesDispersion - n;
}

GroupStats WindMapAnalyticsEngine::statsFor(const QVector<const TimelineEntry*>& shots,
                                            const ConditionKey& key, GroupingMode mode)
{
    GroupStats g;
    g.key = key;
    g.label = key.label(mode);
    g.n = shots.size();
    if (g.n == 0)
        return g;

    // Mean score is meaningful from the first shot; it is an average, not a
    // claim about a group.
    double sumScore = 0.0;
    for (const TimelineEntry* s : shots) sumScore += s->score;
    g.hasMeanScore = true;
    g.meanScore = sumScore / g.n;

    // MPI — the mean point of impact. Withheld below the approved threshold.
    if (g.n < kMinSamplesMpi) {
        g.evidence = Evidence::Insufficient;
        return g;
    }
    double sx = 0.0, sy = 0.0;
    for (const TimelineEntry* s : shots) { sx += s->xMm; sy += s->yMm; }
    g.hasMpi = true;
    g.mpiXMm = sx / g.n;
    g.mpiYMm = sy / g.n;

    if (g.n < kMinSamplesDispersion) {
        g.evidence = Evidence::Indicative;     // a centre, but no spread claim
        return g;
    }

    // Dispersion. Mean radius is measured from the group's OWN centre.
    double sumR = 0.0, minX = 0.0, maxX = 0.0, minY = 0.0, maxY = 0.0;
    bool first = true;
    QVector<double> radii;
    radii.reserve(g.n);
    for (const TimelineEntry* s : shots) {
        const double dx = s->xMm - g.mpiXMm;
        const double dy = s->yMm - g.mpiYMm;
        const double r = std::sqrt(dx * dx + dy * dy);
        radii.append(r);
        sumR += r;
        if (first) { minX = maxX = s->xMm; minY = maxY = s->yMm; first = false; }
        else {
            minX = qMin(minX, s->xMm); maxX = qMax(maxX, s->xMm);
            minY = qMin(minY, s->yMm); maxY = qMax(maxY, s->yMm);
        }
    }
    g.hasDispersion = true;
    g.meanRadiusMm = sumR / g.n;
    g.horizontalSpreadMm = maxX - minX;
    g.verticalSpreadMm = maxY - minY;

    // Group diameter = MAXIMUM SPREAD: the largest centre-to-centre distance
    // between any two shots. Not the bounding box, and not 2x the mean radius.
    double maxD = 0.0;
    for (int i = 0; i < shots.size(); ++i)
        for (int j = i + 1; j < shots.size(); ++j) {
            const double dx = shots[i]->xMm - shots[j]->xMm;
            const double dy = shots[i]->yMm - shots[j]->yMm;
            maxD = qMax(maxD, std::sqrt(dx * dx + dy * dy));
        }
    g.groupDiameterMm = maxD;

    // Sample standard deviations (n-1): these describe THIS sample, and n >= 5
    // here so the denominator is always >= 4.
    double vs = 0.0, vr = 0.0;
    for (const TimelineEntry* s : shots) {
        const double d = s->score - g.meanScore;
        vs += d * d;
    }
    for (double r : radii) {
        const double d = r - g.meanRadiusMm;
        vr += d * d;
    }
    g.scoreStdDev = std::sqrt(vs / (g.n - 1));
    g.radiusStdDev = std::sqrt(vr / (g.n - 1));
    g.evidence = (g.n >= 10) ? Evidence::Strong : Evidence::Sufficient;
    return g;
}

QVector<GroupStats> WindMapAnalyticsEngine::group(const QVector<TimelineEntry>& shots,
                                                   GroupingMode mode)
{
    QVector<ConditionKey> keys;
    QVector<QVector<const TimelineEntry*>> buckets;
    for (const TimelineEntry& s : shots) {
        if (s.sighter) continue;               // sighters are NEVER counted
        const ConditionKey k = keyFor(s.wind, mode);
        int idx = -1;
        for (int i = 0; i < keys.size(); ++i)
            if (keys[i] == k) { idx = i; break; }
        if (idx < 0) {
            keys.append(k);
            buckets.append(QVector<const TimelineEntry*>());
            idx = keys.size() - 1;
        }
        buckets[idx].append(&s);
    }
    QVector<GroupStats> out;
    out.reserve(keys.size());
    for (int i = 0; i < keys.size(); ++i)
        out.append(statsFor(buckets[i], keys[i], mode));
    // Largest first, so the report leads with the best-evidenced group.
    std::sort(out.begin(), out.end(), [](const GroupStats& a, const GroupStats& b) {
        return a.n > b.n;
    });
    return out;
}

// ── reference centre ───────────────────────────────────────────────────────

ReferenceCentre WindMapAnalyticsEngine::chooseReference(const QVector<GroupStats>& exact,
                                                        const QVector<TimelineEntry>& shots,
                                                        const Options& opts)
{
    ReferenceCentre ref;

    auto fromGroup = [&](const GroupStats& g, ReferenceKind kind) {
        ref.kind = kind; ref.key = g.key; ref.label = g.label;
        ref.valid = g.hasMpi; ref.n = g.n; ref.xMm = g.mpiXMm; ref.yMm = g.mpiYMm;
    };

    // 1. An explicitly chosen condition always wins.
    if (opts.preferredReference == ReferenceKind::CoachSelected && opts.hasCoachSelected) {
        for (const GroupStats& g : exact)
            if (g.key == opts.coachSelected && g.hasMpi) {
                fromGroup(g, ReferenceKind::CoachSelected);
                return ref;
            }
    }
    // 2. The recorded-calm group, when it is large enough to compare against.
    if (opts.preferredReference == ReferenceKind::CalmGroup) {
        for (const GroupStats& g : exact)
            if (g.key.hasReading && g.key.calm && g.n >= kMinSamplesComparison && g.hasMpi) {
                fromGroup(g, ReferenceKind::CalmGroup);
                return ref;
            }
    }
    // 3. The most populated group that carries a real reading. A "no reading"
    //    group can never be a reference — there is nothing to reference TO.
    for (const GroupStats& g : exact)          // already sorted largest first
        if (g.key.hasReading && g.n >= kMinSamplesComparison && g.hasMpi) {
            fromGroup(g, ReferenceKind::MostPopulated);
            return ref;
        }
    // 4. Fall back to the centre of every counted shot in this position.
    int n = 0; double sx = 0.0, sy = 0.0;
    for (const TimelineEntry& s : shots) {
        if (s.sighter) continue;
        sx += s.xMm; sy += s.yMm; ++n;
    }
    if (n >= kMinSamplesMpi) {
        ref.kind = ReferenceKind::SessionCentre;
        ref.label = QStringLiteral("Session centre");
        ref.valid = true; ref.n = n;
        ref.xMm = sx / n; ref.yMm = sy / n;
    }
    return ref;
}

// ── shift vectors ──────────────────────────────────────────────────────────

ShiftVector WindMapAnalyticsEngine::shiftOf(const GroupStats& g, const ReferenceCentre& ref)
{
    ShiftVector v;
    v.key = g.key;
    v.label = g.label;
    v.n = g.n;
    v.referenceN = ref.n;
    v.evidence = g.evidence;

    // A comparison needs the approved minimum on BOTH sides. Below that the
    // conclusion is withheld and the shortfall reported.
    if (!ref.valid || !g.hasMpi
        || g.n < kMinSamplesComparison || ref.n < kMinSamplesComparison) {
        v.valid = false;
        v.shotsNeeded = qMax(0, kMinSamplesComparison - g.n);
        return v;
    }
    v.valid = true;
    v.dxMm = g.mpiXMm - ref.xMm;
    v.dyMm = g.mpiYMm - ref.yMm;
    v.magnitudeMm = std::sqrt(v.dxMm * v.dxMm + v.dyMm * v.dyMm);
    // Bearing measured on the target face: 0 = straight up, clockwise.
    v.bearingDegrees = std::fmod(std::atan2(v.dxMm, v.dyMm) * 180.0 / M_PI + 360.0, 360.0);

    // Plain words for the two axes. DESCRIPTIVE — where the group centre sat,
    // not where to move anything.
    const QString h = v.dxMm >= 0 ? QStringLiteral("right") : QStringLiteral("left");
    const QString ve = v.dyMm >= 0 ? QStringLiteral("high") : QStringLiteral("low");
    v.directionWords = QStringLiteral("%1 mm %2, %3 mm %4")
        .arg(QString::number(std::fabs(v.dxMm), 'f', 1), h,
             QString::number(std::fabs(v.dyMm), 'f', 1), ve);
    return v;
}

// ── findings ───────────────────────────────────────────────────────────────

QVector<Finding> WindMapAnalyticsEngine::deriveFindings(const SessionAnalysis& a)
{
    QVector<Finding> out;

    for (const PositionAnalysis& p : a.positions) {
        const QString where = a.threePositions
            ? QStringLiteral("%1: ").arg(p.positionName) : QString();
        // UI-WIND-006: every finding raised inside this loop is about THIS
        // position. Tagging at the source is what lets the view scope them.
        auto tagPosition = [&](Finding& f) {
            f.scope = FindingScope::Position;
            f.position = p.position;
            f.positionName = p.positionName;
        };
        auto tagCondition = [&](Finding& f, const QString& label) {
            f.scope = FindingScope::Condition;
            f.position = p.position;
            f.positionName = p.positionName;
            f.conditionLabel = label;
        };

        // Nothing to say at all.
        if (p.countedShots < kMinSamplesMpi) {
            Finding f;
            f.category = PatternCategory::InsufficientSample;
            f.n = p.countedShots;
            f.shotsNeeded = kMinSamplesMpi - p.countedShots;
            f.text = QStringLiteral("%1Only %2 counted shots were recorded. "
                                    "No reliable comparison can be made.")
                         .arg(where).arg(p.countedShots);
            f.suggestion = QStringLiteral("Record at least %1 more counted shots.")
                               .arg(f.shotsNeeded);
            tagPosition(f);
            out.append(f);
            continue;
        }

        // Groups that carry enough evidence to be described at all.
        QVector<const GroupStats*> solid;
        for (const GroupStats& g : p.byExactCondition)
            if (g.hasDispersion) solid.append(&g);

        // Every group wide -> this is not a wind story.
        if (solid.size() >= 2) {
            double widest = 0.0, tightest = 1e18;
            for (const GroupStats* g : solid) {
                widest = qMax(widest, g->groupDiameterMm);
                tightest = qMin(tightest, g->groupDiameterMm);
            }
            if (tightest > 0.0 && widest / tightest < 1.3) {
                // Similar dispersion everywhere: no condition stands out.
                bool anyShift = false;
                for (const ShiftVector& s : p.shifts)
                    if (s.valid && s.magnitudeMm > s.n * 0.0 && s.magnitudeMm >= 3.0) anyShift = true;
                if (!anyShift) {
                    Finding f;
                    f.category = PatternCategory::SimilarAcrossConditions;
                    f.n = p.countedShots;
                    f.text = QStringLiteral("%1Group size and centre were similar across the "
                                            "recorded conditions. This session does not "
                                            "distinguish a condition effect.").arg(where);
                    f.suggestion = QStringLiteral("Record more shots in each condition, or wait "
                                                  "for a more repeatable flag state.");
                    tagPosition(f);
                    out.append(f);
                }
            }
        }

        // Tight but offset — the classic repeatable shift.
        for (const ShiftVector& s : p.shifts) {
            if (!s.valid) {
                if (s.shotsNeeded > 0) {
                    Finding f;
                    f.category = PatternCategory::InsufficientSample;
                    f.n = s.n; f.shotsNeeded = s.shotsNeeded;
                    f.text = QStringLiteral("%1Only %2 shots were recorded under %3. "
                                            "No reliable comparison can be made.")
                                 .arg(where).arg(s.n).arg(s.label);
                    f.suggestion = QStringLiteral("Repeat this condition and record at least "
                                                  "%1 more shots.").arg(s.shotsNeeded);
                    tagCondition(f, s.label);
                    out.append(f);
                }
                continue;
            }
            const GroupStats* g = nullptr;
            for (const GroupStats& c : p.byExactCondition)
                if (c.key == s.key) { g = &c; break; }
            if (!g || !g->hasDispersion) continue;

            if (s.magnitudeMm >= 3.0 && g->groupDiameterMm <= 25.0) {
                Finding f;
                f.category = PatternCategory::TightButOffset;
                f.n = s.n;
                f.text = QStringLiteral("%1%2 shots recorded under %3 formed a group %4 mm from "
                                        "the %5 (%6).")
                             .arg(where).arg(s.n).arg(s.label,
                                  QString::number(s.magnitudeMm, 'f', 1),
                                  p.reference.label.toLower(), s.directionWords);
                f.suggestion = QStringLiteral("Repeat this condition in a later session to "
                                              "confirm whether the shift is repeatable.");
                tagCondition(f, s.label);
                out.append(f);
            }
        }

        // Wider under one condition than the reference.
        const GroupStats* refGroup = nullptr;
        for (const GroupStats& c : p.byExactCondition)
            if (c.key == p.reference.key) { refGroup = &c; break; }
        if (refGroup && refGroup->hasDispersion) {
            for (const GroupStats& c : p.byExactCondition) {
                if (!c.hasDispersion || c.key == refGroup->key) continue;
                if (c.horizontalSpreadMm > refGroup->horizontalSpreadMm * 1.5) {
                    Finding f;
                    f.category = PatternCategory::WiderUnderCondition;
                    f.n = c.n;
                    f.text = QStringLiteral("%1Horizontal spread was greater under %2 "
                                            "(%3 mm, n=%4) than under %5 (%6 mm, n=%7).")
                                 .arg(where, c.label,
                                      QString::number(c.horizontalSpreadMm, 'f', 1))
                                 .arg(c.n).arg(refGroup->label,
                                      QString::number(refGroup->horizontalSpreadMm, 'f', 1))
                                 .arg(refGroup->n);
                    f.suggestion = QStringLiteral("Practise condition selection and shot timing "
                                                  "in this condition.");
                    tagCondition(f, c.label);
                    out.append(f);
                }
            }
        }

        // Wide everywhere -> refer to technique, not to the wind.
        if (!solid.isEmpty()) {
            bool allWide = true;
            for (const GroupStats* g : solid)
                if (g->groupDiameterMm <= 40.0) allWide = false;
            if (allWide) {
                Finding f;
                f.category = PatternCategory::WideAcrossAllConditions;
                f.n = p.countedShots;
                f.text = QStringLiteral("%1Groups were wide under every recorded condition.")
                             .arg(where);
                f.suggestion = QStringLiteral("Review position stability and use Group Pattern "
                                              "Coach; the recorded conditions do not separate "
                                              "these groups.");
                tagPosition(f);
                out.append(f);
            }
        }
    }

    // 3P: a position-to-position difference is reported as a position finding,
    // never attributed to the wind.
    if (a.threePositions && a.positions.size() >= 2) {
        const PositionAnalysis* widest = nullptr;
        double widestVal = -1.0;
        for (const PositionAnalysis& p : a.positions) {
            for (const GroupStats& g : p.byExactCondition) {
                if (!g.hasDispersion) continue;
                if (g.groupDiameterMm > widestVal) { widestVal = g.groupDiameterMm; widest = &p; }
            }
        }
        if (widest && widestVal > 0.0) {
            Finding f;
            f.category = PatternCategory::PositionSpecificDifference;
            // Deliberately SESSION scope: this compares positions AGAINST EACH
            // OTHER, so it must never be shown as a single position's result.
            f.scope = FindingScope::Session;
            f.n = widest->countedShots;
            f.text = QStringLiteral("The widest recorded group was in %1 (%2 mm). "
                                    "Positions have different stability demands and are "
                                    "analysed separately.")
                         .arg(widest->positionName, QString::number(widestVal, 'f', 1));
            f.suggestion = QStringLiteral("Compare each position against itself across "
                                          "sessions rather than against another position.");
            out.append(f);
        }
    }

    if (out.isEmpty()) {
        Finding f;
        f.category = PatternCategory::SimilarAcrossConditions;
        f.scope = FindingScope::Session;
        f.n = a.countedShots;
        f.text = QStringLiteral("No condition-specific shift was established in this session.");
        f.suggestion = QStringLiteral("Record every condition you observe, and repeat a "
                                      "condition often enough to compare it with itself.");
        out.append(f);
    }
    return out;
}

// ── the entry point ────────────────────────────────────────────────────────

SessionAnalysis WindMapAnalyticsEngine::analyse(const SessionState& state)
{
    return analyse(state, Options());
}

SessionAnalysis WindMapAnalyticsEngine::analyse(const SessionState& state, const Options& opts)
{
    SessionAnalysis a;
    if (state.wmProgramId != QLatin1String("wind_map"))
        return a;                              // fails closed: not a Wind Map session
    a.valid = true;
    a.disciplineId = state.wmDisciplineId;
    a.threePositions = state.wmThreePositions;
    a.conditionEntries = state.wmConditionChanges;

    // ── flatten to the timeline, in journal order ───────────────────────
    QVector<WindConditionSnapshot> distinct;
    bool sawCounted = false;
    WindConditionSnapshot prev;
    bool havePrev = false;
    for (const WindMapShotRecord& r : state.wmShots) {
        TimelineEntry e;
        e.shotId = r.shotId;
        e.sighter = r.sighter;
        e.position = r.position;
        e.positionName = windMapPositionName(static_cast<WindMapPosition>(r.position));
        e.xMm = r.shot.xHundredthMm / 100.0;
        e.yMm = r.shot.yHundredthMm / 100.0;
        e.score = r.shot.scoreTenths / 10.0;
        e.splitMs = r.shot.splitMs;
        e.wind.valid = r.windValid;
        e.wind.calm = r.windCalm;
        e.wind.directionDegrees = r.windDirectionDegrees;
        e.wind.speedHundredthMs = r.windSpeedHundredthMs;
        e.wind.source = (r.windSource == 1) ? WindSource::WeatherStation : WindSource::Manual;
        e.wind.recordedMsSinceEpoch = r.windRecordedMs;
        e.wind.note = r.windNote;
        e.conditionLabel = keyFor(e.wind, GroupingMode::ExactCondition)
                               .label(GroupingMode::ExactCondition);
        e.conditionChangedBefore = havePrev && !prev.sameConditionAs(e.wind);
        e.phaseChangedBefore = !e.sighter && !sawCounted;
        if (!e.sighter) sawCounted = true;
        prev = e.wind; havePrev = true;

        if (!e.sighter) {
            ++a.countedShots;
            if (!e.wind.valid) ++a.countedNoReading;
            else if (e.wind.calm) ++a.countedCalm;
            else ++a.countedWithReading;
            bool seen = false;
            for (const WindConditionSnapshot& d : distinct)
                if (d.sameConditionAs(e.wind)) { seen = true; break; }
            if (!seen) distinct.append(e.wind);
        } else {
            ++a.sighterShots;
        }
        a.timeline.append(e);
    }
    a.uniqueConditions = distinct.size();

    // ── per position, INDEPENDENTLY ─────────────────────────────────────
    // Kneeling, Prone and Standing are never pooled. Prone50 has one bucket.
    QVector<int> positions;
    if (a.threePositions) { positions << 1 << 2 << 3; }
    else                  { positions << 0; }

    for (int pos : positions) {
        QVector<TimelineEntry> mine;
        for (const TimelineEntry& e : a.timeline)
            if (e.position == pos) mine.append(e);
        if (mine.isEmpty()) continue;

        PositionAnalysis p;
        p.position = pos;
        p.positionName = a.threePositions
            ? windMapPositionName(static_cast<WindMapPosition>(pos))
            : QStringLiteral("Prone");
        for (const TimelineEntry& e : mine)
            if (e.sighter) ++p.sighterShots; else ++p.countedShots;

        p.byDirection      = group(mine, GroupingMode::Direction);
        p.bySpeedBand      = group(mine, GroupingMode::SpeedBand);
        p.byExactCondition = group(mine, GroupingMode::ExactCondition);

        if (p.countedShots >= kMinSamplesMpi) {
            double sx = 0.0, sy = 0.0;
            for (const TimelineEntry& e : mine) {
                if (e.sighter) continue;
                sx += e.xMm; sy += e.yMm;
            }
            p.hasOverallMpi = true;
            p.overallMpiXMm = sx / p.countedShots;
            p.overallMpiYMm = sy / p.countedShots;
        }

        // The reference is POSITION-SPECIFIC — Standing is never measured
        // against a Prone centre.
        p.reference = chooseReference(p.byExactCondition, mine, opts);
        for (const GroupStats& g : p.byExactCondition) {
            if (p.reference.valid && g.key == p.reference.key) continue;  // vs itself
            p.shifts.append(shiftOf(g, p.reference));
        }
        a.positions.append(p);
    }

    a.findings = deriveFindings(a);

    a.limitations
        << QStringLiteral("The recorded condition is an athlete observation and may not "
                          "represent wind across the complete bullet path.")
        << QStringLiteral("Sighters are recorded with their conditions but are excluded from "
                          "every counted statistic.")
        << QStringLiteral("Observed differences between groups are descriptions of where "
                          "shots landed. They do not establish a cause.")
        << QStringLiteral("Training material only — never an official competition result.");
    if (a.countedNoReading > 0)
        a.limitations << QStringLiteral("%1 counted shots were recorded with no wind reading "
                                        "and are excluded from condition comparisons.")
                             .arg(a.countedNoReading);
    return a;
}

} // namespace training
} // namespace ta
