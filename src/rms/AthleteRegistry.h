#ifndef TA_RMS_ATHLETEREGISTRY_H
#define TA_RMS_ATHLETEREGISTRY_H

// RMS's own start list. Create, edit, remove, persist. Nothing here reaches a
// target node, and nothing here touches the target application's athlete data.

#include "Athlete.h"
#include "RmsJsonStore.h"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

namespace ta {
namespace rms {

class AthleteRegistry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY athletesChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit AthleteRegistry(QObject* parent = nullptr);

    void setStorePath(const QString& path);
    void load();

    int count() const { return int(m_athletes.size()); }
    QString lastError() const { return m_lastError; }
    const QVector<Athlete>& athletes() const { return m_athletes; }
    const Athlete* byId(const QString& athleteId) const;

    // Quick field-test entry: a name is enough, and the athlete is usable at
    // once. Returns the new athleteId, or empty when refused.
    Q_INVOKABLE QString addAthlete(const QString& displayName,
                                   const QString& club = QString(),
                                   const QString& country = QString(),
                                   bool temporary = false);
    Q_INVOKABLE bool updateAthlete(const QString& athleteId, const QString& displayName,
                                   const QString& club, const QString& country,
                                   const QString& notes);
    // Refused while any plan still refers to the athlete — the caller must
    // resolve that first. Removing them silently would leave a lane pointing
    // at somebody who no longer exists.
    Q_INVOKABLE bool removeAthlete(const QString& athleteId);

    Q_INVOKABLE QString displayNameFor(const QString& athleteId) const;
    Q_INVOKABLE QVariantMap athleteAt(int index) const;
    Q_INVOKABLE QVariantList allAthletes() const;

    // Set by MatchPlanService so the registry can refuse a removal that would
    // orphan a plan lane. Returns the plan name using the athlete, or empty.
    using InUseCheck = std::function<QString(const QString& athleteId)>;
    void setInUseCheck(InUseCheck check) { m_inUse = std::move(check); }

signals:
    void athletesChanged();
    void lastErrorChanged();
    void rejected(const QString& reason);

private:
    bool commit();
    void setError(const QString& reason);
    int indexOf(const QString& athleteId) const;

    RmsJsonStore m_store;
    QVector<Athlete> m_athletes;
    QString m_lastError;
    InUseCheck m_inUse;
};

} // namespace rms
} // namespace ta

#endif // TA_RMS_ATHLETEREGISTRY_H
