#ifndef TA_RMS_COMPETITIONSTATE_H
#define TA_RMS_COMPETITIONSTATE_H

// ─────────────────────────────────────────────────────────────────────────────
// COMPETITION STATUS — a third, independent axis.
//
// A lane has three separate states that are routinely confused and must not be:
//
//   NODE STATUS         is the station reachable?        ONLINE / OFFLINE
//   TARGET STATUS       is its target answering?         CONNECTED / …
//   COMPETITION STATUS  where is the athlete in the      ACTIVE / WAITING /
//                       competition?                     FINISHED / ELIMINATED
//
// An eliminated athlete's station is usually perfectly healthy. Collapsing
// these into one "status" is how an eliminated finalist ends up looking like a
// network fault, or a dead tablet ends up looking like an elimination.
//
// ═══ RMS NEVER DECIDES THIS. ═══════════════════════════════════════════════
//
// Elimination in a 3P final is determined by Finals3PController on the target
// node, against the ISSF rules the node implements. RMS is a display. It must
// NEVER infer elimination from:
//
//     rank alone · score alone · shot count · how many athletes are still
//     shooting · translated text · another athlete disappearing from the range
//
// Every one of those is a plausible-looking heuristic that would eventually
// tell an athlete they were out when they were not. The authority chain is:
//
//     Finals3PController  →  authoritative elimination state
//                         →  telemetry
//                         →  RMS and every display
//
// ═══ IT IS NOT REPORTED YET. ═══════════════════════════════════════════════
//
// Protocol v1 carries no competition status, so for every real station this is
// `Unknown` with source `NotReported`, and RMS says exactly that rather than
// guessing. The fields below exist so the display layer is built around the
// right shape now; they are populated when the protocol gains them
// DELIBERATELY, in a version bump — never by widening v1.
//
// See docs/architecture/rms-finals-elimination-display.md.
// ─────────────────────────────────────────────────────────────────────────────

#include <QString>

namespace ta {
namespace rms {

enum class CompetitionStatus {
    Unknown,      // not reported — the only value a v1 station can produce
    Active,       // shooting now
    Waiting,      // in the final, not currently firing (between stages, etc.)
    Finished,     // completed the course
    Eliminated    // counted out; the node decided this, RMS did not
};

QString toString(CompetitionStatus s);
// For the FUTURE v2 decoder. Defined here so the wire tokens have one spelling
// when that revision arrives, and so nothing has to be invented then.
CompetitionStatus competitionStatusFromString(const QString& s, bool* ok = nullptr);

struct CompetitionState {
    // Where the value came from. A display may legitimately treat
    // DevelopmentInjection differently, and an audit must be able to tell a
    // real elimination from a demonstration one.
    enum class Source {
        NotReported,          // protocol v1: nothing to report
        Telemetry,            // the node said so — the only production source
        DevelopmentInjection  // a development tool said so, for visual evidence
    };

    CompetitionStatus status = CompetitionStatus::Unknown;
    Source  source = Source::NotReported;

    // Finals metadata. Reported alongside the status when the protocol carries
    // it; never derived here.
    int     rank = 0;                    // 0 = not reported
    double  finalScore = 0.0;
    bool    finalScoreReported = false;
    QString finalsStage;                 // where the final was when this applied
    QString eliminatedAtStage;

    // A state the athlete does not shoot on from here. A display must be able
    // to present these; it must not assume every lane is mid-course.
    bool isTerminal() const
    {
        return status == CompetitionStatus::Finished
            || status == CompetitionStatus::Eliminated;
    }
    bool isEliminated() const { return status == CompetitionStatus::Eliminated; }
    bool isReported() const { return source != Source::NotReported; }
    bool isSimulated() const { return source == Source::DevelopmentInjection; }

    QString rankLabel() const;   // "8th", or empty when not reported
    QString scoreLabel() const;  // "402.7", or "—" when not reported
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_COMPETITIONSTATE_H
