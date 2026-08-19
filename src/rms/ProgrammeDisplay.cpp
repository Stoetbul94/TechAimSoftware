#include "ProgrammeDisplay.h"

#include <QRegularExpression>
#include <QStringList>

namespace ta {
namespace rms {

namespace {

// "air-rifle" -> "Air Rifle"
QString titleiseHyphenated(const QString& token)
{
    QStringList parts = token.split(QLatin1Char('-'), Qt::SkipEmptyParts);
    for (QString& p : parts)
        if (!p.isEmpty())
            p[0] = p.at(0).toUpper();
    return parts.join(QLatin1Char(' '));
}

// "10m" -> "10 m",  "50m" -> "50 m"
QString formatDistance(const QString& token)
{
    static const QRegularExpression re(QStringLiteral("^(\\d+)m$"));
    const auto m = re.match(token);
    return m.hasMatch() ? (m.captured(1) + QStringLiteral(" m")) : token;
}

// "qualification60" -> "Qualification 60";  "match20" -> "Match 20";
// "free" -> "Free"
QString formatProgramme(const QString& token)
{
    static const QRegularExpression re(QStringLiteral("^([a-z]+)(\\d+)$"));
    const auto m = re.match(token);
    if (m.hasMatch()) {
        QString word = m.captured(1);
        word[0] = word.at(0).toUpper();
        return word + QLatin1Char(' ') + m.captured(2);
    }
    return titleiseHyphenated(token);
}

} // namespace

QString ProgrammeDisplay::describe(const QString& programmeId)
{
    if (programmeId.isEmpty())
        return QString();

    QStringList seg = programmeId.split(QLatin1Char('.'), Qt::SkipEmptyParts);

    // Optional trailing ".p15" variant marker.
    QString variant;
    if (seg.size() > 1 && seg.last() == QLatin1String("p15")) {
        seg.removeLast();
        variant = QStringLiteral(" (15-shot)");
    }

    // Expected shape: <ruleset>.<distance>.<weapon>.<programme>
    if (seg.size() != 4)
        return programmeId;

    const QString distance  = formatDistance(seg.at(1));
    const QString weapon    = titleiseHyphenated(seg.at(2));
    const QString programme = formatProgramme(seg.at(3));

    // A distance segment that did not parse means the id is not the shape
    // this function understands — return it verbatim rather than half-format.
    if (distance == seg.at(1))
        return programmeId;

    return distance + QLatin1Char(' ') + weapon
           + QStringLiteral(" · ") + programme + variant;
}

QString ProgrammeDisplay::shortDescribe(const QString& programmeId)
{
    const QString full = describe(programmeId);
    const int dot = full.indexOf(QStringLiteral(" · "));
    return dot > 0 ? full.left(dot) : full;
}

bool ProgrammeDisplay::isOfficialProgramme(const QString& rulesetId)
{
    return rulesetId == QLatin1String("issf");
}

} // namespace rms
} // namespace ta
