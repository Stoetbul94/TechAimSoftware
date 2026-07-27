#ifndef PRODUCTIDENTITYBRIDGE_H
#define PRODUCTIDENTITYBRIDGE_H

#include <QObject>
#include <QString>

#include "ProductIdentity.h"

// QML-facing view of ta::app::identity(), exposed as the PRODUCT context
// property. Read-only by construction: identity is a build-time fact, so
// there are no setters and no NOTIFY signals — the values cannot change
// while the application is running.
//
// QML must read product strings from here rather than hardcoding them, so
// that the future OEM flavour needs no QML edits.
class ProductIdentityBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString fullProductName READ fullProductName CONSTANT)
    Q_PROPERTY(QString releaseDescription READ releaseDescription CONSTANT)
    Q_PROPERTY(QString executableBaseName READ executableBaseName CONSTANT)
    Q_PROPERTY(QString legalPublisher READ legalPublisher CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString releaseChannel READ releaseChannel CONSTANT)
    Q_PROPERTY(QString softwareVersionLabel READ softwareVersionLabel CONSTANT)
    Q_PROPERTY(QString supportContact READ supportContact CONSTANT)
    Q_PROPERTY(QString defaultTheme READ defaultTheme CONSTANT)
    Q_PROPERTY(QString flavour READ flavour CONSTANT)

public:
    explicit ProductIdentityBridge(QObject* parent = nullptr) : QObject(parent) {}

    QString displayName() const        { return ta::app::identity().displayName; }
    QString fullProductName() const    { return ta::app::identity().fullProductName; }
    QString releaseDescription() const { return ta::app::identity().releaseDescription; }
    QString executableBaseName() const { return ta::app::identity().executableBaseName; }
    QString legalPublisher() const     { return ta::app::identity().legalPublisher; }
    QString version() const            { return ta::app::identity().version; }
    QString releaseChannel() const     { return ta::app::identity().releaseChannel; }
    QString softwareVersionLabel() const { return ta::app::identity().softwareVersionLabel(); }
    QString supportContact() const     { return ta::app::identity().supportContact; }
    QString defaultTheme() const       { return ta::app::identity().defaultTheme; }
    QString flavour() const            { return ta::app::flavourName(ta::app::currentFlavour()); }
};

#endif // PRODUCTIDENTITYBRIDGE_H
