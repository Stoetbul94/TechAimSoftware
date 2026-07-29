#ifndef TA_TRAINING_WINDMAPCONTROLLER_H
#define TA_TRAINING_WINDMAPCONTROLLER_H

// Wind Map — controller (QML context property WINDMAP).
//
// Training Lab Release 2. 50m Rifle Prone and 50m Rifle 3 Positions ONLY.
// Owns ALL state via its OWN reliability SessionStore (sessionKind=Training,
// programId=wind_map), reusing the accepted Training infrastructure exactly as
// Technical Blocks, Call & Diagnose and Position Transition do.
//
// WHAT THIS PROGRAMME IS: the athlete records the wind they observe; each
// accepted shot takes an IMMUTABLE COPY of that condition so the two can be
// reviewed together afterwards. See docs/training-lab-wind-map-spec-review.md.
//
// WHAT IT IS NOT: a live sight-correction assistant, a coaching command
// system, a competition workflow, or an official ISSF mode. Nothing here
// scores, corrects or advises. It never resolves as Qualification, Final,
// Open Practice or an official result.
//
// QML NEVER CONSTRUCTS DOMAIN EVENTS. Every operation goes through the
// Q_INVOKABLE methods below; the controller validates, submits durably, and
// only then updates its projection.
//
// SPEED IS FIXED POINT. QML passes an operator-facing m/s value; the
// conversion to hundredths happens here through the shared domain helper.
// The raw hundredths value is never exposed to QML.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

#include "WindMapTypes.h"
#include "reliability/store/SessionStore.h"
#include "reliability/recovery/RecoveryCoordinator.h"

class WindMapController : public QObject
{
    Q_OBJECT

    // phase — ta::training::WindMapPhase, mirrored durably as
    // SessionState::wmPhase: 0 Idle, 1 Setup, 2 Sighters, 3 CountedShots,
    // 4 PositionReview (3P only), 5 SessionReview, 6 Completed.
    Q_PROPERTY(int phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(QString phaseName READ phaseName NOTIFY phaseChanged)
    Q_PROPERTY(bool active READ active NOTIFY phaseChanged)
    Q_PROPERTY(bool inSetup READ inSetup NOTIFY phaseChanged)
    Q_PROPERTY(bool inSighters READ inSighters NOTIFY phaseChanged)
    Q_PROPERTY(bool counting READ counting NOTIFY phaseChanged)
    Q_PROPERTY(bool positionReviewOpen READ positionReviewOpen NOTIFY phaseChanged)
    Q_PROPERTY(bool reviewOpen READ reviewOpen NOTIFY phaseChanged)
    Q_PROPERTY(bool completed READ isCompleted NOTIFY phaseChanged)

    // configuration
    Q_PROPERTY(QString disciplineId READ disciplineId NOTIFY configChanged)
    Q_PROPERTY(bool threePositions READ threePositions NOTIFY configChanged)
    Q_PROPERTY(int shotPlan READ shotPlan NOTIFY configChanged)
    Q_PROPERTY(bool sightersEnabled READ sightersEnabled NOTIFY configChanged)
    Q_PROPERTY(QString positionSequence READ positionSequence NOTIFY configChanged)

    // live progress
    Q_PROPERTY(int currentPosition READ currentPosition NOTIFY progressChanged)
    Q_PROPERTY(QString positionName READ positionName NOTIFY progressChanged)
    Q_PROPERTY(int countedShots READ countedShots NOTIFY progressChanged)
    Q_PROPERTY(int sighterCount READ sighterCount NOTIFY progressChanged)
    Q_PROPERTY(int totalCountedShots READ totalCountedShots NOTIFY progressChanged)
    Q_PROPERTY(int totalSighterShots READ totalSighterShots NOTIFY progressChanged)
    Q_PROPERTY(int conditionChanges READ conditionChanges NOTIFY conditionChanged)

    // the standing wind condition — operator-facing values only
    Q_PROPERTY(bool hasWindReading READ hasWindReading NOTIFY conditionChanged)
    Q_PROPERTY(bool isCalm READ isCalm NOTIFY conditionChanged)
    Q_PROPERTY(int directionDegrees READ directionDegrees NOTIFY conditionChanged)
    Q_PROPERTY(QString directionLabel READ directionLabel NOTIFY conditionChanged)
    Q_PROPERTY(double speedMetresPerSecond READ speedMetresPerSecond NOTIFY conditionChanged)
    Q_PROPERTY(QString conditionNote READ conditionNote NOTIFY conditionChanged)
    Q_PROPERTY(QString conditionSummary READ conditionSummary NOTIFY conditionChanged)
    Q_PROPERTY(qint64 conditionRecordedMs READ conditionRecordedMs NOTIFY conditionChanged)

    Q_PROPERTY(QString sessionId READ sessionId NOTIFY phaseChanged)
    Q_PROPERTY(QString sessionOperatingMode READ sessionOperatingMode NOTIFY phaseChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY phaseChanged)
    Q_PROPERTY(QString lastStartError READ lastStartError NOTIFY startErrorChanged)

public:
    explicit WindMapController(QObject* parent = nullptr);
    ~WindMapController() override;

    // ── availability ────────────────────────────────────────────────────
    // The single authority on where Wind Map may run. Unsupported disciplines
    // fail CLOSED here and in every path below — never a silent fallback.
    Q_INVOKABLE static bool isDisciplineSupported(const QString& disciplineId);
    Q_INVOKABLE static QString unsupportedDisciplineMessage(const QString& disciplineId);

    // ── setup ───────────────────────────────────────────────────────────
    Q_INVOKABLE bool configureSession(const QString& disciplineId, int shotPlan,
                                      bool enableSighters);
    Q_INVOKABLE QString validateConfig() const;
    Q_INVOKABLE bool startWindMap(const QString& athlete);

    // ── wind condition ──────────────────────────────────────────────────
    // Three DISTINCT states. Calm is a recorded observation; No Reading is
    // absence. Neither is ever represented by a default 0-degree direction.
    Q_INVOKABLE bool setCalmCondition(const QString& note = QString());
    Q_INVOKABLE bool setMeasuredCondition(int directionDegrees,
                                          double speedMetresPerSecond,
                                          const QString& note = QString());
    Q_INVOKABLE bool setNoReadingCondition();

    // ── workflow ────────────────────────────────────────────────────────
    // Every transition goes through the domain rule
    // (ta::training::windMapTransitionAllowed). An illegal one is REFUSED —
    // never clamped to the nearest legal phase, never silently skipped.
    Q_INVOKABLE bool beginSighters();
    Q_INVOKABLE bool finishSighters();        // Sighters → CountedShots
    Q_INVOKABLE bool beginCountedShots();     // Setup/PositionReview → CountedShots
    Q_INVOKABLE bool endPosition();           // CountedShots → PositionReview (3P)
    Q_INVOKABLE bool changePosition(int toPosition);   // 3P only
    Q_INVOKABLE bool registerShot(double xMm, double yMm, double decimalScore,
                                  qint64 externalId = -1, double directionDeg = 0.0,
                                  int shotSource = 0);
    Q_INVOKABLE bool endCapture();            // → SessionReview
    Q_INVOKABLE bool completeSession();       // SessionReview → Completed
    Q_INVOKABLE bool closeSessionCleanly();
    Q_INVOKABLE void discardRecovery(const QString& sessionId);
    Q_INVOKABLE qint64 recoveredMaxExternalId() const;

    // ── recovery ────────────────────────────────────────────────────────
    // Restores from an already-replayed projection. Submits NOTHING: a resume
    // must not re-journal the start, the conditions or the shots.
    Q_INVOKABLE bool resumeFromRecovery(const QString& sessionId);
    // The same restoration, from an already-built recovered state. This is the
    // seam the tests drive (with an injected journal file, touching no disk) —
    // it is the SAME code path the real resume runs, not a parallel one.
    bool resumeFromRecoveredState(const ta::rel::RecoveredMatchState& recovered);
    ta::rel::SessionStore* storeForTesting() { return m_store.get(); }

    // ── basic review projection (Stage 5: factual capture proof only) ────
    Q_INVOKABLE QVariantMap reviewSummary() const;
    Q_INVOKABLE QVariantList reviewShots() const;

    // property readers
    int phase() const { return static_cast<int>(m_phase); }
    QString phaseName() const { return ta::training::windMapPhaseName(m_phase); }
    bool active() const { return m_phase != ta::training::WindMapPhase::Idle; }
    bool inSetup() const { return m_phase == ta::training::WindMapPhase::Setup; }
    bool inSighters() const { return m_phase == ta::training::WindMapPhase::Sighters; }
    bool counting() const { return m_phase == ta::training::WindMapPhase::CountedShots; }
    bool positionReviewOpen() const { return m_phase == ta::training::WindMapPhase::PositionReview; }
    bool reviewOpen() const { return m_phase == ta::training::WindMapPhase::SessionReview; }
    bool isCompleted() const { return m_phase == ta::training::WindMapPhase::Completed; }
    QString disciplineId() const { return m_disciplineId; }
    bool threePositions() const { return m_threePositions; }
    int shotPlan() const { return m_shotPlan; }
    bool sightersEnabled() const { return m_sightersEnabled; }
    QString positionSequence() const;
    int currentPosition() const;
    QString positionName() const;
    int countedShots() const;              // current position only
    int sighterCount() const;              // current position only
    int totalCountedShots() const;
    int totalSighterShots() const;
    int conditionChanges() const;
    bool hasWindReading() const { return m_wind.hasReading(); }
    bool isCalm() const { return m_wind.isCalm(); }
    int directionDegrees() const { return m_wind.directionDegrees; }
    QString directionLabel() const;
    double speedMetresPerSecond() const { return m_wind.speedMetresPerSecond(); }
    QString conditionNote() const { return m_wind.note; }
    QString conditionSummary() const;
    qint64 conditionRecordedMs() const { return m_wind.recordedMsSinceEpoch; }
    QString sessionId() const;
    QString sessionOperatingMode() const { return m_store ? st().operatingMode : QString(); }
    QString lastError() const { return m_lastError; }
    QString lastStartError() const { return m_lastStartError; }

    void setOperatingMode(int mode) { m_operatingMode = mode; }

signals:
    void phaseChanged();
    void configChanged();
    void progressChanged();
    void conditionChanged();
    void startErrorChanged();
    void shotRejected(const QString& reason);
    void shotAccepted(bool sighter, int shotId, int position);

signals:
    void journalWriteFailed(QString path, QString detail);

private:
    const ta::rel::SessionState& st() const { return m_store->state(); }
    bool submit(const ta::rel::DomainEvent& ev);
    bool applyCondition(const ta::training::WindConditionSnapshot& snap);
    bool goToPhase(ta::training::WindMapPhase to);
    bool acceptShotInternal(bool sighter, double xMm, double yMm,
                            double decimalScore, qint64 externalId,
                            double directionDeg, int shotSource);
    bool fail(const QString& message, const char* code);
    void rebuildFromState();

    std::unique_ptr<ta::rel::SessionStore> m_store;
    std::unique_ptr<ta::rel::RecoveryCoordinator> m_recovery;

    ta::training::WindMapPhase m_phase = ta::training::WindMapPhase::Idle;
    QString m_disciplineId;
    bool    m_threePositions = false;
    int     m_shotPlan = 40;
    bool    m_sightersEnabled = true;
    qint64  m_lastExternalId = -1;
    int     m_operatingMode = -1;
    QString m_lastError;
    QString m_lastStartError;

    // Cached copy of the STANDING condition. The reducer projection
    // (SessionState::wmWind*) is authoritative and is what this is rebuilt
    // from on resume; the copy exists so the readers stay valid after the
    // session is closed and so it clears deliberately rather than by accident.
    ta::training::WindConditionSnapshot m_wind;
};

#endif // TA_TRAINING_WINDMAPCONTROLLER_H
