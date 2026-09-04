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

namespace ta {
namespace telemetry {

class ITelemetrySink
{
public:
    virtual ~ITelemetrySink() = default;
    virtual bool send(const QByteArray& datagram) = 0;
};

} // namespace telemetry
} // namespace ta

#endif // TA_TELEMETRY_ITELEMETRYSINK_H
