#include "HashChain.h"

#include "reliability/events/EventEnvelope.h"

#include <QCryptographicHash>

namespace ta {
namespace rel {

QByteArray HashChain::genesisPreviousHash()
{
    return QByteArray(kHashHexLength, '0');
}

QByteArray HashChain::computeLineHash(const QByteArray& previousHashHex,
                                      const QByteArray& coreBytes)
{
    QCryptographicHash sha(QCryptographicHash::Sha256);
    sha.addData(previousHashHex);
    sha.addData(coreBytes);
    // toHex() is lowercase; truncate the 64-char digest to 32 (spec D8).
    return sha.result().toHex().left(kHashHexLength);
}

} // namespace rel
} // namespace ta
