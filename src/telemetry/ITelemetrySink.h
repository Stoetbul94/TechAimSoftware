#ifndef TA_TELEMETRY_ITELEMETRYSINK_H
#define TA_TELEMETRY_ITELEMETRYSINK_H

// Where a telemetry datagram goes. The interface exists so that
// NodeTelemetryService stays QtCore-only and can be exercised in the GUI-free,
// network-free reliability harness with a recording double — the production
// UDP socket is the only piece that needs QtNetwork.
//
// `send` returns false on failure and MUST NOT block, retry or throw. A
// telemetry failure is never allowed to reach the shot that triggered it.

#include <QByteArray>
#include <QString>

namespace ta {
namespace telemetry {

class ITelemetrySink
{
public:
    virtual ~ITelemetrySink() = default;
    virtual bool send(const QByteArray& datagram) = 0;

    // Why the last send failed, when the sink knows. Defaulted rather than pure
    // so a test double need not implement it, but present on the INTERFACE so a
    // failure can be reported with its cause instead of just its existence -
    // "send failed" without an errno is what makes a field failure unactionable.
    virtual QString lastError() const { return QString(); }
};

} // namespace telemetry
} // namespace ta

#endif // TA_TELEMETRY_ITELEMETRYSINK_H
