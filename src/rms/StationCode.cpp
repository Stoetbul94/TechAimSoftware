#include "rms/StationCode.h"

#include <QSet>

namespace ta {
namespace rms {

namespace {

// The tail of the id is where the entropy is: a UUID's first characters vary
// less usefully than its last across stations imaged from the same disk. Taking
// from the END also matches how people read a serial number off a label.
QString tailGroups(const QString& payload, int hexChars)
{
    const int want = qBound(4, hexChars, payload.size());
    QString tail = payload.right(want).toUpper();
    // Group in fours: "E222403F" -> "E222-403F".
    QString out;
    for (int i = 0; i < tail.size(); ++i) {
        if (i > 0 && (tail.size() - i) % 4 == 0)
            out += QLatin1Char('-');
        out += tail.at(i);
    }
    return out;
}

}  // namespace

QString StationCode::payloadOf(const QString& nodeId)
{
    const QString trimmed = nodeId.trimmed();
    const int dash = trimmed.lastIndexOf(QLatin1Char('-'));
    if (trimmed.startsWith(QLatin1String("TA-NODE-")) && dash >= 0
        && dash + 1 < trimmed.size()) {
        return trimmed.mid(dash + 1);
    }
    return trimmed;
}

QString StationCode::codeOfLength(const QString& nodeId, int hexChars)
{
    const QString payload = payloadOf(nodeId);
    if (payload.isEmpty())
        return QString();
    // Round up to a multiple of four so a code never ends in a stub group.
    const int rounded = ((hexChars + 3) / 4) * 4;
    return tailGroups(payload, rounded);
}

QString StationCode::shortCode(const QString& nodeId)
{
    return codeOfLength(nodeId, kDefaultHexChars);
}

QHash<QString, QString> StationCode::codesFor(const QStringList& nodeIds)
{
    QHash<QString, QString> out;
    if (nodeIds.isEmpty())
        return out;

    // De-duplicate first: the same node listed twice is not a collision.
    QStringList unique;
    QSet<QString> seen;
    for (const QString& id : nodeIds) {
        if (id.trimmed().isEmpty() || seen.contains(id))
            continue;
        seen.insert(id);
        unique << id;
    }

    int longest = 0;
    for (const QString& id : unique)
        longest = qMax(longest, payloadOf(id).size());

    // Grow EVERY code together until they are all distinct. Expanding only the
    // colliding pair would leave two lanes labelled in different formats, which
    // is its own kind of confusing.
    for (int len = kDefaultHexChars; len <= qMax(longest, kDefaultHexChars); len += 4) {
        QHash<QString, QString> attempt;
        QSet<QString> used;
        bool clash = false;
        for (const QString& id : unique) {
            const QString code = codeOfLength(id, len);
            if (used.contains(code)) {
                clash = true;
                break;
            }
            used.insert(code);
            attempt.insert(id, code);
        }
        if (!clash)
            return attempt;
    }

    // Everything collided at full payload length, which means the ids are
    // equal — impossible for distinct nodes. Fall back to the full id so the
    // display is at worst ugly, never ambiguous.
    for (const QString& id : unique)
        out.insert(id, id);
    return out;
}

bool StationCode::wouldCollide(const QStringList& nodeIds)
{
    QSet<QString> codes;
    QSet<QString> seenIds;
    for (const QString& id : nodeIds) {
        if (id.trimmed().isEmpty() || seenIds.contains(id))
            continue;
        seenIds.insert(id);
        const QString c = shortCode(id);
        if (codes.contains(c))
            return true;
        codes.insert(c);
    }
    return false;
}

}  // namespace rms
}  // namespace ta
