// THE READ-ONLY INVARIANT.
//
// Milestone 1 promises that RMS cannot start, stop, reset, sight, match,
// change position, end, feed or recover a target node. A promise in a
// document decays; these checks are the enforcement.
//
// Three independent guards, because each catches what the others cannot:
//   1. a source scan — no RMS file may contain a transmit call or address
//      the node's inbound control port;
//   2. a meta-object scan — nothing reachable from QML is named or shaped
//      like a command;
//   3. a model scan — the dashboard model refuses edits.

#include "test_support.h"

#include "rms/RangeListModel.h"
#include "rms/RangeMonitor.h"
#include "rms/RmsProtocol.h"
#include "rms/RmsUdpObserver.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QMetaMethod>
#include <QMetaObject>
#include <QStringList>

#include <cstdio>

using namespace ta::rms;

namespace {

#ifndef RMS_SOURCE_ROOT
#define RMS_SOURCE_ROOT "."
#endif

// Everything that could put a byte on the wire, or address the node's
// inbound control port. `sendCommand` and friends are listed by name so a
// future author gets a failing test, not a silent capability.
const char* kForbiddenTokens[] = {
    "writeDatagram",
    "connectToHost",
    "QTcpSocket",
    "sendCommand",
    "kReservedCommandPort",
    "startMatchFromServer"
};

QStringList collectSources(const QString& root, const QStringList& filters,
                           const QStringList& excludeDirs)
{
    QStringList files;
    QDirIterator it(root, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = QDir::fromNativeSeparators(it.next());
        bool excluded = false;
        for (const QString& ex : excludeDirs)
            if (path.contains(ex))
                excluded = true;
        if (!excluded)
            files << path;
    }
    files.sort();
    return files;
}

QString readAll(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(f.readAll());
}

bool methodNameSuggestsControl(const QByteArray& name)
{
    const QByteArray n = name.toLower();
    return n.contains("send") || n.contains("transmit") || n.contains("command")
           || n.contains("broadcast") || n.contains("write")
           || n.contains("startmatch") || n.contains("stopmatch");
}

} // namespace

void run_readonly_tests()
{
    std::printf("\n-- read-only invariant --\n");

    const QString root = QString::fromLatin1(RMS_SOURCE_ROOT);

    // ── 1. source scan ─────────────────────────────────────────────────
    //
    // src/rms/dev/ is EXCLUDED: the simulator plays the NODE's role and is
    // allowed to transmit. It is compiled only under TECHAIM_RMS_DEV_SIMULATOR
    // and is not part of the observer.
    const QStringList observerSources =
        collectSources(root + QStringLiteral("/src/rms"),
                       { QStringLiteral("*.cpp"), QStringLiteral("*.h") },
                       { QStringLiteral("/src/rms/dev/") });

    check(observerSources.size() >= 12,
          QStringLiteral("the observer source set was found (%1 files)")
              .arg(observerSources.size()),
          QStringLiteral("root=%1").arg(root));

    for (const QString& path : observerSources) {
        const QString text = readAll(path);
        const QString name = path.section(QLatin1Char('/'), -1);
        for (const char* token : kForbiddenTokens) {
            const QString t = QString::fromLatin1(token);
            // RmsProtocol.h DECLARES the reserved command port so that it can
            // be named and asserted unused; that declaration is the one
            // legitimate mention.
            const bool declarationSite =
                (t == QLatin1String("kReservedCommandPort")
                 && name == QLatin1String("RmsProtocol.h"));
            const bool present = text.contains(t) && !declarationSite;
            check(!present,
                  QStringLiteral("%1 contains no '%2'").arg(name, t),
                  QStringLiteral("read-only invariant"));
        }
    }

    // The application layer must be just as clean.
    // Build output is excluded: generated moc/qrc files are not authored
    // source, and scanning them would make the guard depend on whether a
    // build happened to be in the tree.
    const QStringList buildArtefacts = {
        QStringLiteral("/release/"), QStringLiteral("/debug/"),
        QStringLiteral("/moc_"),     QStringLiteral("/qrc_")
    };
    const QStringList appSources =
        collectSources(root + QStringLiteral("/rms"),
                       { QStringLiteral("*.cpp"), QStringLiteral("*.qml") },
                       buildArtefacts);
    check(!appSources.isEmpty(), "the RMS application sources were found");
    for (const QString& path : appSources) {
        const QString text = readAll(path);
        const QString name = path.section(QLatin1Char('/'), -1);
        check(!text.contains(QLatin1String("writeDatagram")),
              QStringLiteral("%1 sends nothing").arg(name));
        check(!text.contains(QLatin1String("QTcpSocket")),
              QStringLiteral("%1 opens no control connection").arg(name));
    }

    // The observer must never be able to name the node's inbound control
    // port as a destination.
    check(kReservedCommandPort == 7756,
          "the node's inbound control port is documented as reserved");
    check(kObservationPort != kReservedCommandPort,
          "observation and control are different ports");

    // ── 2. meta-object scan ────────────────────────────────────────────
    {
        const QMetaObject* metas[] = {
            &RangeMonitor::staticMetaObject,
            &RangeListModel::staticMetaObject,
            &RmsUdpObserver::staticMetaObject
        };
        for (const QMetaObject* mo : metas) {
            bool clean = true;
            QString offender;
            for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
                const QMetaMethod m = mo->method(i);
                if (methodNameSuggestsControl(m.name())) {
                    clean = false;
                    offender = QString::fromLatin1(m.name());
                }
            }
            check(clean,
                  QStringLiteral("%1 exposes no command-shaped method")
                      .arg(QLatin1String(mo->className())),
                  offender);
        }

        // Everything QML can call is a query. A Q_INVOKABLE that returns void
        // would be doing something rather than reporting something.
        const QMetaObject* mo = &RangeListModel::staticMetaObject;
        bool allQueries = true;
        QString offender;
        for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
            const QMetaMethod m = mo->method(i);
            if (m.methodType() != QMetaMethod::Method)
                continue;
            if (m.returnType() == QMetaType::Void) {
                allQueries = false;
                offender = QString::fromLatin1(m.name());
            }
        }
        check(allQueries,
              "every Q_INVOKABLE the dashboard can call RETURNS data rather than acting",
              offender);
    }

    // ── 3. model scan ──────────────────────────────────────────────────
    {
        RangeMonitor monitor;
        RangeListModel model(&monitor);

        NodeAnnounce a;
        a.nodeId = QStringLiteral("TA-NODE-RO");
        a.bootId = QStringLiteral("boot-ro");
        a.laneId = QStringLiteral("Lane 1");
        monitor.ingestDatagram(encode(a), 0);

        const QModelIndex idx = model.index(0, 0);
        check(idx.isValid(), "the model has a row to attempt to edit");
        check(!(model.flags(idx) & Qt::ItemIsEditable),
              "no dashboard row is editable");
        check(!model.setData(idx, QVariant(QStringLiteral("Lane 99")),
                             RangeListModel::LaneLabelRole),
              "setData is refused - the UI cannot write back into the range");
        check(model.data(idx, RangeListModel::LaneLabelRole).toString()
                  == QLatin1String("Lane 1"),
              "...and the observed value is unchanged");
        check(model.readOnly(), "the model advertises its read-only nature to QML");
    }

    // ── 4. the protocol itself has no command ──────────────────────────
    {
        // If a command type existed, this would decode into something.
        const DecodedMessage d = decode(QByteArray(
            "{\"protocolVersion\":1,\"type\":\"command.startMatch\","
            "\"nodeId\":\"TA-NODE-001\",\"commandId\":\"c1\"}"));
        check(d.type == MessageType::Unknown,
              "a start-match command is not a decodable message in protocol v1");
        check(d.rejectReason.contains(QLatin1String("unknown type")),
              "...and is rejected as an unknown type", d.rejectReason);
    }
}
