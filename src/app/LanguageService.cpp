#include "LanguageService.h"
#include "ProductIdentity.h"

#include <QCoreApplication>
#include <QQmlEngine>
#include <QSettings>
#include <QVariantMap>
#include <QDebug>

namespace {
// config.ini is sectioned; the language preference lives in the same
// [App_Settings] group as app_mode, so operator-facing settings stay together.
const char* kLanguageKey = "App_Settings/ui_language";
}

QList<ta::app::LanguageOption> LanguageService::supportedLanguages()
{
    // English first: it is the source language and the fallback.
    return {
        { QStringLiteral("en"),    QStringLiteral("English"), false },
        { QStringLiteral("de-DE"), QStringLiteral("Deutsch"), true  },
    };
}

LanguageService::LanguageService(const QString& configFilePath, QObject* parent)
    : QObject(parent)
    , m_configPath(configFilePath)
    , m_code(ta::app::identity().defaultLanguage)
{
}

QString LanguageService::languageName() const
{
    for (const ta::app::LanguageOption& o : supportedLanguages())
        if (o.code == m_code) return o.endonym;
    return QStringLiteral("English");
}

bool LanguageService::isBetaTranslation() const
{
    for (const ta::app::LanguageOption& o : supportedLanguages())
        if (o.code == m_code) return o.beta;
    return false;
}

QVariantList LanguageService::availableLanguages() const
{
    QVariantList out;
    for (const ta::app::LanguageOption& o : supportedLanguages()) {
        QVariantMap m;
        m[QStringLiteral("code")]    = o.code;
        m[QStringLiteral("name")]    = o.endonym;
        m[QStringLiteral("beta")]    = o.beta;
        // The label carries the Beta marker: the German catalogue is an
        // untested-by-native-speaker beta and must be presented as such.
        m[QStringLiteral("label")]   = o.beta ? o.endonym + QStringLiteral(" (Beta)")
                                              : o.endonym;
        out.append(m);
    }
    return out;
}

QString LanguageService::readPersisted() const
{
    if (m_configPath.isEmpty()) return QString();
    QSettings s(m_configPath, QSettings::IniFormat);
    return s.value(QLatin1String(kLanguageKey)).toString().trimmed();
}

void LanguageService::persist(const QString& code)
{
    if (m_configPath.isEmpty()) return;
    QSettings s(m_configPath, QSettings::IniFormat);
    s.setValue(QLatin1String(kLanguageKey), code);
    s.sync();
}

bool LanguageService::installCatalogue(const QString& code)
{
    m_diagnostic.clear();

    if (m_installed) {
        QCoreApplication::removeTranslator(&m_translator);
        m_installed = false;
    }

    // English is the source language: there is no catalogue to load, and
    // removing the translator is exactly what returns the UI to English.
    if (code == QLatin1String("en"))
        return true;

    // Catalogues ship inside the binary (translations.qrc), so a missing
    // file on disk cannot break a deployed install.
    const QString base = QStringLiteral("techaim_%1").arg(QString(code).replace('-', '_'));
    if (!m_translator.load(QStringLiteral(":/translations/") + base)) {
        m_diagnostic = QStringLiteral("Translation catalogue '%1' could not be loaded; "
                                      "the interface stays in English.").arg(base);
        qWarning().noquote() << "i18n:" << m_diagnostic;
        return false;
    }
    QCoreApplication::installTranslator(&m_translator);
    m_installed = true;
    return true;
}

void LanguageService::applyPersistedLanguage()
{
    const QString stored = readPersisted();
    QString code = ta::app::identity().defaultLanguage;
    if (!stored.isEmpty()) {
        bool known = false;
        for (const ta::app::LanguageOption& o : supportedLanguages())
            if (o.code.compare(stored, Qt::CaseInsensitive) == 0) { code = o.code; known = true; break; }
        if (!known) {
            qWarning().noquote() << "i18n: unknown ui_language" << stored
                                 << "- falling back to" << code;
        }
    }
    m_code = code;
    installCatalogue(m_code);
    qInfo().noquote() << "UI language:" << m_code
                      << (isBetaTranslation() ? "(beta translation)" : "(source language)");
    emit languageChanged();
}

bool LanguageService::selectLanguage(const QString& code)
{
    QString resolved;
    for (const ta::app::LanguageOption& o : supportedLanguages())
        if (o.code.compare(code, Qt::CaseInsensitive) == 0) { resolved = o.code; break; }
    if (resolved.isEmpty()) return false;
    if (resolved == m_code) return true;

    m_code = resolved;
    persist(m_code);
    installCatalogue(m_code);

    // Re-evaluate every qsTr() binding in the loaded QML. Where a string was
    // NOT written as a binding it keeps its old value until the next load —
    // that is what restartRequired reports, so the UI can offer Restart Now
    // rather than showing a half-translated screen.
    if (m_engine) {
        m_engine->retranslate();
        m_restartRequired = false;
    } else {
        m_restartRequired = true;
    }
    emit languageChanged();
    emit restartRequiredChanged();
    return true;
}
