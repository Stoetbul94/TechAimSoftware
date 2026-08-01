#ifndef TA_TARGET_PAPERFEEDCOORDINATOR_H
#define TA_TARGET_PAPERFEEDCOORDINATOR_H

// Tech Aim 0.9.0-RC2 — the one authority that decides when paper feeds.
//
// WHY THIS EXISTS. In RC1 the automatic feed at the accepted-shot path was
// commented out (CenterPane.qml:346, above a note reading "Auto feed not
// working #76"), and the only live automatic call sat inside a ListView
// DELEGATE (CenterPane.qml:917). A delegate is created and destroyed by the
// view as it scrolls and as the model resets, so a motor command attached to
// one can fire late, fire twice, or never fire. Manual feed worked because it
// is a direct button press. That is exactly what the field test showed.
//
// So the decision moves out of QML entirely. QML may DISPLAY feed status; it
// may not cause a motor command.
//
// Pure QtCore. The actual register write is injected, so the whole policy is
// testable with no hardware and no Modbus.

#include <QQueue>
#include <QSet>
#include <QString>
#include <functional>

namespace ta {
namespace target {

// Why a shot did not feed. Every skip is logged with one of these — silence
// would be indistinguishable from a motor failure.
enum class FeedSkipReason : int {
    NotSkipped = 0,
    NotLiveMode,        // Demo/simulation: never drive physical hardware
    DurationZero,       // operator disabled feeding for this shot type
    DuplicateShot,      // same shot identity already fed
    RecoveryReplay,     // journal replay after restart
    HistoricalLoad,     // opening an old session, report or analysis
    NotAccepted,        // rejected or malformed shot
    TargetDisconnected,
};

QString feedSkipReasonName(FeedSkipReason r);

enum class ShotKind : int { Sighter = 0, Counted = 1 };

// One accepted physical shot asking for one feed.
struct FeedRequest {
    QString sessionId;      // scopes the duplicate set
    qint64  shotIdentity = 0;   // external/sequence id, unique within the session
    ShotKind kind = ShotKind::Counted;
    double  durationSeconds = 0.0;
    qint64  monotonicMs = 0;
};

struct FeedDecision {
    bool accepted = false;
    FeedSkipReason skip = FeedSkipReason::NotSkipped;
    double durationSeconds = 0.0;
    QString detail;
};

// Everything the coordinator needs to know about the world, injected so the
// policy can be tested exhaustively without hardware.
struct FeedContext {
    bool liveMode = false;          // app_mode=Live; Demo never drives the motor
    bool targetConnected = false;
    bool replaying = false;         // recovery replay or historical load in progress
    double matchDurationSeconds = 0.0;
    double sighterDurationSeconds = 0.0;
};

// A practical ceiling. A stuck value of, say, 600 would hold the motor on for
// ten minutes; refusing is safer than obeying.
inline constexpr double kMaxFeedSeconds = 30.0;

// Bounded so a long session cannot grow the duplicate set without limit.
inline constexpr int kRecentIdentityLimit = 512;

class PaperFeedCoordinator
{
public:
    // The injected motor command: (durationSeconds) -> success. In production
    // this is TachusWidget's Modbus register write; in tests it records calls.
    using MotorCommand = std::function<bool(double)>;
    // Optional operational log sink.
    using LogSink = std::function<void(const QString&)>;

    explicit PaperFeedCoordinator(MotorCommand motor = {}, LogSink log = {});

    void setMotorCommand(MotorCommand motor) { m_motor = std::move(motor); }
    void setLogSink(LogSink log) { m_log = std::move(log); }
    void setContext(const FeedContext& ctx) { m_ctx = ctx; }
    FeedContext context() const { return m_ctx; }

    // Validate an operator-entered duration. Returns the value to store.
    // Negative is refused (returns 0 and reports why); zero is legal and means
    // "no automatic feed for this shot type"; above the ceiling is clamped.
    static double sanitiseDuration(double seconds, QString* why = nullptr);

    // THE ENTRY POINT. Call once per accepted, durably recorded physical shot.
    // Returns what was decided so the caller can log or display it.
    FeedDecision onShotAccepted(const FeedRequest& req);

    // Manual "Feed paper". Deliberately separate: it bypasses the duplicate
    // set and the shot-type rules, and is logged distinctly so a manual feed
    // can never be mistaken for an automatic one.
    bool requestManualFeed(double durationSeconds);

    // A new session scopes the duplicate set: shot 1 of session B must not be
    // suppressed because session A also had a shot 1.
    void beginSession(const QString& sessionId);
    void endSession();

    // Test/diagnostic accessors.
    int  queuedCount() const { return m_queue.size(); }
    bool motorBusy() const { return m_motorBusy; }
    int  feedsIssued() const { return m_feedsIssued; }
    int  duplicatesPrevented() const { return m_duplicatesPrevented; }

    // Drives the queue. In production the motor command blocks for the
    // duration, so this returns once the queue is drained; the point is that
    // commands are SERIAL, never overlapping.
    void pumpQueue();

private:
    void log(const QString& line) const;
    bool rememberIdentity(const QString& sessionId, qint64 identity);

    MotorCommand m_motor;
    LogSink m_log;
    FeedContext m_ctx;

    QString m_sessionId;
    QQueue<FeedRequest> m_queue;
    QSet<QString> m_recentIdentities;      // "sessionId#identity"
    QQueue<QString> m_recentOrder;         // bounds m_recentIdentities
    bool m_motorBusy = false;
    int  m_feedsIssued = 0;
    int  m_duplicatesPrevented = 0;
};

}} // namespace ta::target

#endif // TA_TARGET_PAPERFEEDCOORDINATOR_H
