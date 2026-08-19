#include "AthleteRegistry.h"
#include "MatchPlan.h"

#include <QJsonArray>
#include <QJsonObject>

namespace ta {
namespace rms {

AthleteRegistry::AthleteRegistry(QObject* parent)
    : QObject(parent)
{
    m_store.setPath(rmsDataFile(QStringLiteral("athletes.json")));
}

void AthleteRegistry::setStorePath(const QString& path)
{
    m_store.setPath(path);
}

void AthleteRegistry::load()
{
    QJsonObject doc;
    const StoreResult r = m_store.load(kAthleteSchemaVersion, &doc);
    m_athletes.clear();
    if (r.ok) {
        const QJsonArray arr = doc.value(QStringLiteral("athletes")).toArray();
        for (const QJsonValue& v : arr) {
            const Athlete a = Athlete::fromJson(v.toObject());
            if (a.isValid())
                m_athletes.append(a);
        }
        setError(QString());
    } else if (r.error == StoreError::NotFound) {
        // No start list yet. Not a fault.
        setError(QString());
    } else {
        setError(r.detail);
    }
    emit athletesChanged();
}

void AthleteRegistry::setError(const QString& reason)
{
    if (m_lastError == reason)
        return;
    m_lastError = reason;
    emit lastErrorChanged();
}

int AthleteRegistry::indexOf(const QString& athleteId) const
{
    for (int i = 0; i < m_athletes.size(); ++i)
        if (m_athletes.at(i).athleteId == athleteId)
            return i;
    return -1;
}

const Athlete* AthleteRegistry::byId(const QString& athleteId) const
{
    const int i = indexOf(athleteId);
    return i < 0 ? nullptr : &m_athletes.at(i);
}

bool AthleteRegistry::commit()
{
    QJsonArray arr;
    for (const Athlete& a : m_athletes)
        arr.append(a.toJson());
    QJsonObject doc;
    doc[QStringLiteral("athletes")] = arr;

    const StoreResult r = m_store.save(kAthleteSchemaVersion, doc);
    if (!r.ok) {
        setError(r.detail);
        emit athletesChanged();
        return false;
    }
    setError(QString());
    emit athletesChanged();
    return true;
}

QString AthleteRegistry::addAthlete(const QString& displayName, const QString& club,
                                    const QString& country, bool temporary)
{
    if (displayName.trimmed().isEmpty()) {
        setError(QStringLiteral("An athlete needs a name."));
        emit rejected(m_lastError);
        return QString();
    }
    Athlete a = Athlete::create(displayName, temporary);
    a.club = club.trimmed();
    a.country = country.trimmed();
    // TWO PEOPLE MAY SHARE A NAME. The id is what an assignment refers to, so
    // a duplicate display name is allowed and is not even unusual on a club
    // start list.
    m_athletes.append(a);
    if (!commit())
        return QString();
    return a.athleteId;
}

bool AthleteRegistry::updateAthlete(const QString& athleteId, const QString& displayName,
                                    const QString& club, const QString& country,
                                    const QString& notes)
{
    const int i = indexOf(athleteId);
    if (i < 0)
        return false;
    if (displayName.trimmed().isEmpty()) {
        setError(QStringLiteral("An athlete needs a name."));
        emit rejected(m_lastError);
        return false;
    }
    m_athletes[i].displayName = displayName.trimmed();
    m_athletes[i].club        = club.trimmed();
    m_athletes[i].country     = country.trimmed();
    m_athletes[i].notes       = notes;
    return commit();
}

bool AthleteRegistry::removeAthlete(const QString& athleteId)
{
    const int i = indexOf(athleteId);
    if (i < 0)
        return false;

    // A lane pointing at somebody who no longer exists is worse than refusing
    // the deletion, so the operator is told which plan to clear first.
    if (m_inUse) {
        const QString planName = m_inUse(athleteId);
        if (!planName.isEmpty()) {
            setError(QStringLiteral("%1 is on a lane in \"%2\". Clear that lane first.")
                         .arg(m_athletes.at(i).displayName, planName));
            emit rejected(m_lastError);
            return false;
        }
    }
    m_athletes.remove(i);
    return commit();
}

QString AthleteRegistry::displayNameFor(const QString& athleteId) const
{
    const Athlete* a = byId(athleteId);
    return a ? a->displayName : QString();
}

QVariantMap AthleteRegistry::athleteAt(int index) const
{
    QVariantMap m;
    if (index < 0 || index >= m_athletes.size())
        return m;
    const Athlete& a = m_athletes.at(index);
    m[QStringLiteral("athleteId")]   = a.athleteId;
    m[QStringLiteral("displayName")] = a.displayName;
    m[QStringLiteral("club")]        = a.club;
    m[QStringLiteral("country")]     = a.country;
    m[QStringLiteral("notes")]       = a.notes;
    m[QStringLiteral("temporary")]   = a.temporary;
    return m;
}

QVariantList AthleteRegistry::allAthletes() const
{
    QVariantList out;
    for (int i = 0; i < m_athletes.size(); ++i)
        out.append(athleteAt(i));
    return out;
}

} // namespace rms
} // namespace ta
