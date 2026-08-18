#pragma once
// DSB 1.20 — Luftgewehr 3-Stellung, the gated independent-position-clock
// sequencer (Sportordnung 1.20, Stand 01.01.2026).
//
// This is NOT the ISSF three-position course with different numbers. The rule
// is structurally different in three ways, and each one is why this controller
// exists rather than a shot-count parameter on the existing 3P path:
//
//   1. THREE INDEPENDENT CLOCKS. 3x10 runs 25 / 20 / 30 minutes and 3x20 runs
//      35 / 30 / 40. Those are three separate competitions of time; the fact
//      that they sum to 75 and 105 is arithmetic, not a master clock.
//   2. GATED TRANSITIONS. A finished position never starts the next one. Match
//      control does, through an authorised action. Between the two the
//      competition sits in a gate with NO clock running.
//   3. SIGHTING INSIDE THE POSITION CLOCK. Prone and standing open with their
//      clock already running: sighters there cost the athlete competition time.
//      Kneeling does not, because the shared 15-minute preparation before it
//      already provided its sighting period.
//
// The engine decides whether an action is legal — never the UI. QML asks; this
// answers. That is also what makes a future central range controller a matter
// of calling the same actions from somewhere else.
//
// QtCore-only (QObject for signals/invokables), so the console harness
// exercises it directly.

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

#include "reliability/store/SessionStore.h"
#include "reliability/recovery/RecoveryCoordinator.h"

namespace ta {
namespace dsb {

// The durable phase. Position + phase + the armed gate describe the whole
// competition; there is no separate per-position enum explosion.
enum class Phase : quint8 {
    Idle = 0,
    PreparationSighting = 1,   // the single 15 min, before kneeling
    WaitingStart = 2,          // GATE: nothing is running, next position armed
    PositionSighting = 3,      // clock RUNNING, shots are sighters
    PositionMatch = 4,         // clock running, shots count
    PositionChange = 5,        // position closed, gate not yet armed
    Finished = 6
};

enum class Position : qint8 { None = -1, Kneeling = 0, Prone = 1, Standing = 2 };

} // namespace dsb
} // namespace ta

class Dsb120Controller : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool active READ active NOTIFY sessionChanged)
    Q_PROPERTY(int phaseId READ phaseId NOTIFY stateChanged)
    Q_PROPERTY(int positionIndex READ positionIndex NOTIFY stateChanged)
    Q_PROPERTY(int nextPositionIndex READ nextPositionIndex NOTIFY stateChanged)
    Q_PROPERTY(bool sightingLocked READ sightingLocked NOTIFY stateChanged)
    Q_PROPERTY(int matchShotsInPosition READ matchShotsInPosition NOTIFY shotCountsChanged)
    Q_PROPERTY(int shotsPerPosition READ shotsPerPosition NOTIFY sessionChanged)
    Q_PROPERTY(int totalMatchShots READ totalMatchShots NOTIFY shotCountsChanged)
    Q_PROPERTY(int totalShotsRequired READ totalShotsRequired NOTIFY sessionChanged)
    Q_PROPERTY(qint64 positionDurationMs READ positionDurationMs NOTIFY stateChanged)
    // remainingMs is deliberately NOT a property: it changes every second
    // without an event, so a property would advertise a value that updates
    // only when the state does - and it would shadow the invokable the HUD
    // polls. The clock is read as a function call.
    Q_PROPERTY(QString journalPath READ journalPath NOTIFY sessionChanged)
    Q_PROPERTY(bool recovered READ recovered NOTIFY sessionChanged)

public:
    explicit Dsb120Controller(QObject* parent = nullptr);
    ~Dsb120Controller() override;

    // ── lifecycle ────────────────────────────────────────────────────────
    // `authority` is the ADOPTED competition definition (the same map the rule
    // authority is persisted from). Every duration this controller uses comes
    // from there — never from the catalogue, and never from a literal in this
    // file, so a recovered session runs on what it adopted.
    Q_INVOKABLE bool startSession(const QVariantMap& authority,
                                  const QString& athlete,
                                  const QString& lane = QString(),
                                  const QString& targetId = QString());
    Q_INVOKABLE void closeSession();
    Q_INVOKABLE void setOperatingMode(int mode) { m_operatingMode = mode; }

    // ── authorised competition-control actions ───────────────────────────
    // Each returns false when the action is not legal in the current state.
    // Refusal is the engine's answer, not the button's.
    Q_INVOKABLE bool startPreparation();
    Q_INVOKABLE bool startPosition(int positionIndex);   // START_KNEELING/PRONE/STANDING
    Q_INVOKABLE bool enterMatchPhase();                  // sighting -> match
    // The RETURN to sighting, which the engine refuses once the position has an
    // accepted match shot. It is an action rather than an absent button because
    // "the operator cannot click it" is not the same statement as "the
    // competition does not allow it", and only the second one is a rule.
    Q_INVOKABLE bool requestSighting();
    Q_INVOKABLE bool endPosition();                      // END_POSITION
    Q_INVOKABLE bool finishMatch();                      // FINISH

    // ── shots ────────────────────────────────────────────────────────────
    // Classification comes from the competition state, never from a caller's
    // opinion about what kind of shot this is.
    Q_INVOKABLE bool submitShot(double xMm, double yMm, double score,
                                qint64 externalId, double directionDeg,
                                bool simulated);

    // ── recovery ─────────────────────────────────────────────────────────
    Q_INVOKABLE QVariantList scanForRecovery();
    Q_INVOKABLE bool resumeFromRecovery(const QString& sessionId);
    Q_INVOKABLE void discardRecovery(const QString& sessionId);
    Q_INVOKABLE QVariantList recoveredShots() const;

    // The adopted authority of the LIVE session, for the report/UI layer.
    // Read from the session, never from the catalogue.
    Q_INVOKABLE QVariantMap sessionRuleAuthority() const;

    // ── reads (all derived from the reducer's authoritative state) ───────
    bool active() const;
    int phaseId() const;
    int positionIndex() const;
    int nextPositionIndex() const;
    bool sightingLocked() const;
    int matchShotsInPosition() const;
    int shotsPerPosition() const { return m_shotsPerPosition; }
    int totalMatchShots() const;
    int totalShotsRequired() const { return m_shotsPerPosition * 3; }
    qint64 positionDurationMs() const;
    // Remaining on the CURRENT clock. During a gate there is no clock, and this
    // returns -1 rather than the next position's duration — a waiting position
    // must never look like a running one.
    Q_INVOKABLE qint64 remainingMs() const;
    Q_INVOKABLE int positionSubtotalTenths(int positionIndex) const;
    Q_INVOKABLE int matchShotsIn(int positionIndex) const;
    Q_INVOKABLE QString scoringMode() const { return m_scoringMode; }
    QString journalPath() const;
    bool recovered() const { return m_recovered; }

    ta::rel::SessionStore* storeForTesting() { return m_store.get(); }

signals:
    void sessionChanged();
    void stateChanged();
    void shotCountsChanged();
    void shotAccepted(QVariantMap record);
    void journalWriteFailed(QString path, QString detail);

private:
    ta::dsb::Phase phase() const;
    bool step(ta::rel::Dsb120Step s, int positionIndex, qint64 durationMs);
    void submitEvent(const ta::rel::DomainEvent& event);
    qint64 durationForPosition(int index) const;

    std::unique_ptr<ta::rel::SessionStore> m_store;
    std::unique_ptr<ta::rel::RecoveryCoordinator> m_recovery;
    bool m_recovered = false;
    int m_operatingMode = -1;
    // Adopted definition, in ms. Populated at startSession from the authority
    // map and at resume from the session's persisted authority.
    qint64 m_preparationMs = 0;
    QVector<qint64> m_positionMs;
    int m_shotsPerPosition = 0;
    QString m_scoringMode;
    // A resumed session continues from the FROZEN remaining time, then runs
    // again from the moment of resume. Both halves are needed: the frozen
    // value is the competition fact, and the new anchor is what makes the
    // clock live again instead of standing still.
    qint64 m_resumeRemainingMs = -1;
    qint64 m_resumeAtMonoMs = 0;
};
