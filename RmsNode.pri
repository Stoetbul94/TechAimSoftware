# RMS NODE CONTROL PLANE — the node half of the R2 control channel, bound to
# the Tech Aim application.
#
# Two groups, separated for the same reason Telemetry.pri separates its own:
#
#   RMSNODE_CORE_*  QtCore only. The wire contract, the HMAC handshake, the
#                   endpoint, the durable command journal and the versioned
#                   store. Every refusal in the protocol is decided here, and
#                   compiling without QtNetwork is what proves none of it
#                   depends on a socket.
#
#   RMSNODE_NET_*   the TCP listener and the Tech Aim command binding. The only
#                   pieces that touch a socket or the application.
#
# The CORE group is byte-identical to the qualified RMS sources; it is shared,
# not forked. A second hand-maintained copy would drift, and a control protocol
# that drifts silently sends the wrong command to the wrong lane.

INCLUDEPATH += $$PWD/src

RMSNODE_CORE_HEADERS = \
    $$PWD/src/rms/RmsJsonStore.h \
    $$PWD/src/rms/control/ControlProtocol.h \
    $$PWD/src/rms/control/ControlAuth.h \
    $$PWD/src/rms/control/CommandJournal.h \
    $$PWD/src/rms/control/NodeControlEndpoint.h

RMSNODE_CORE_SOURCES = \
    $$PWD/src/rms/RmsJsonStore.cpp \
    $$PWD/src/rms/control/ControlProtocol.cpp \
    $$PWD/src/rms/control/ControlAuth.cpp \
    $$PWD/src/rms/control/CommandJournal.cpp \
    $$PWD/src/rms/control/NodeControlEndpoint.cpp

RMSNODE_NET_HEADERS = \
    $$PWD/src/rms/node/NodeControlServer.h \
    $$PWD/src/rms/node/TechAimNodeCommands.h

RMSNODE_NET_SOURCES = \
    $$PWD/src/rms/node/NodeControlServer.cpp \
    $$PWD/src/rms/node/TechAimNodeCommands.cpp

HEADERS += $$RMSNODE_CORE_HEADERS $$RMSNODE_NET_HEADERS
SOURCES += $$RMSNODE_CORE_SOURCES $$RMSNODE_NET_SOURCES
