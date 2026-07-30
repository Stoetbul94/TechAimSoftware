#ifndef PRODUCTIDENTITYBRIDGE_H
#define PRODUCTIDENTITYBRIDGE_H

#include <QObject>
#include <QString>

#include "ProductIdentity.h"
#include "training/WindMapAnalytics.h"   // kWindMapAnalyticsVersion

#include <QSysInfo>

// The build injects these (Seta.pro). A build that lost them must say so
// rather than silently claim an identity it does not have.
#ifndef APP_GIT_SHA
#define APP_GIT_SHA "unknown"
#endif
#ifndef APP_BUILD_CONFIG
#define APP_BUILD_CONFIG "Unknown"
#endif

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
    // 0.9.0-RC1 field-test identity, all compile-time.
    Q_PROPERTY(QString fieldTestNotice READ fieldTestNotice CONSTANT)
    Q_PROPERTY(bool isFieldTest READ isFieldTest CONSTANT)
    Q_PROPERTY(QString gitCommit READ gitCommit CONSTANT)
    Q_PROPERTY(QString buildDate READ buildDate CONSTANT)
    Q_PROPERTY(QString buildConfig READ buildConfig CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString analyticsVersion READ analyticsVersion CONSTANT)
    Q_PROPERTY(QString architecture READ architecture CONSTANT)
    Q_PROPERTY(QString windowsVersion READ windowsVersion CONSTANT)

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

    // ── 0.9.0-RC1 field-test build identity ───────────────────────────────
    // Everything an operator needs to identify EXACTLY which binary produced a
    // result, without a repository, Git or Qt on the machine. All of it is
    // baked in at compile time.
    QString fieldTestNotice() const    { return ta::app::identity().fieldTestNotice; }
    bool    isFieldTest() const        { return !ta::app::identity().fieldTestNotice.isEmpty(); }
    QString gitCommit() const          { return QStringLiteral(APP_GIT_SHA); }
    QString buildDate() const          { return QStringLiteral(__DATE__ " " __TIME__); }
    QString buildConfig() const        { return QStringLiteral(APP_BUILD_CONFIG); }
    QString qtVersion() const          { return QStringLiteral(QT_VERSION_STR); }
    QString analyticsVersion() const
    {
        // Not QStringLiteral: the constant is a const char*, not a literal.
        return QString::fromLatin1(ta::training::kWindMapAnalyticsVersion);
    }
    QString architecture() const       { return QSysInfo::buildCpuArchitecture(); }
    QString windowsVersion() const     { return QSysInfo::prettyProductName(); }
};

#endif // PRODUCTIDENTITYBRIDGE_H
