# RMS node telemetry — the target station's publisher.
#
# Two groups, deliberately separated:
#
#   TELEMETRY_CORE_*   QtCore only. The shared protocol contract, the node
#                      identity and the publisher. Compiles in the GUI-free,
#                      network-free reliability harness, which is what proves
#                      it carries no UI and no socket dependency.
#
#   TELEMETRY_NET_*    the UDP sink. The ONLY piece that needs QtNetwork, and
#                      the only piece that touches a socket.
#
# Included by Seta.pro (both groups) and by test .pro files, which pick the
# groups they need. Same sharing pattern as Reliability.pri.

INCLUDEPATH += $$PWD/src

TELEMETRY_CORE_HEADERS = \
    $$PWD/src/rms/RmsProtocol.h \
    $$PWD/src/telemetry/ITelemetrySink.h \
    $$PWD/src/telemetry/NodeIdentity.h \
    $$PWD/src/telemetry/NodeTelemetryService.h

TELEMETRY_CORE_SOURCES = \
    $$PWD/src/rms/RmsProtocol.cpp \
    $$PWD/src/telemetry/NodeIdentity.cpp \
    $$PWD/src/telemetry/NodeTelemetryService.cpp

TELEMETRY_NET_HEADERS = $$PWD/src/telemetry/UdpTelemetrySink.h
TELEMETRY_NET_SOURCES = $$PWD/src/telemetry/UdpTelemetrySink.cpp

# Set TELEMETRY_NO_NET before including to take the core group only. The
# QtCore-only reliability harness does exactly that: compiling the publisher
# there is what PROVES it has no socket and no GUI dependency.
isEmpty(TELEMETRY_NO_NET) {
    HEADERS += $$TELEMETRY_CORE_HEADERS $$TELEMETRY_NET_HEADERS
    SOURCES += $$TELEMETRY_CORE_SOURCES $$TELEMETRY_NET_SOURCES
} else {
    HEADERS += $$TELEMETRY_CORE_HEADERS
    SOURCES += $$TELEMETRY_CORE_SOURCES
}
