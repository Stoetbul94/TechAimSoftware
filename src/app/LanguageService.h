#ifndef LANGUAGESERVICE_H
#define LANGUAGESERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QTranslator>

class QQmlEngine;

// ─────────────────────────────────────────────────────────────────────────
// P0 Phase F — UI language selection (English / German).
//
// English is the SOURCE language: every qsTr()/tr() literal in the codebase
// is English, so English needs no catalogue and can never be "missing". Any
// string absent from a translation catalogue falls back to that English
// source automatically (Qt's own behaviour) — a missing translation can
// therefore never render as an empty label or as a raw key.
//
// LANGUAGE IS NOT BRAND. Selecting Deutsch must not change the logo, the
// theme, the executable name, the publisher or the AppData identity. This
// service touches translations only; it deliberately has no access to
// ProductIdentity, and no scoring, session or journal state.
//
// Persistence lives in config.ini alongside app_mode, so the choice survives
// a restart without introducing a second settings store.
// ─────────────────────────────────────────────────────────────────────────

namespace ta { namespace app {

struct LanguageOption {
    QString code;        // "en", "de-DE"
    QString endonym;     // language's own name, as shown in the selector
    bool    beta = false;
};

} } // namespace ta::app

class LanguageService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString languageCode READ languageCode NOTIFY languageChanged)
    Q_PROPERTY(QString languageName READ languageName NOTIFY languageChanged)
    Q_PROPERTY(bool isBetaTranslation READ isBetaTranslation NOTIFY languageChanged)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY restartRequiredChanged)
    Q_PROPERTY(QVariantList availableLanguages READ availableLanguages CONSTANT)

public:
    explicit LanguageService(const QString& configFilePath, QObject* parent = nullptr);

    QString languageCode() const { return m_code; }
    QString languageName() const;
    bool isBetaTranslation() const;
    bool restartRequired() const { return m_restartRequired; }
    QVariantList availableLanguages() const;

    // Applies the persisted language at startup, before the QML engine loads.
    void applyPersistedLanguage();

    // The engine is needed to re-evaluate qsTr() bindings on a live switch.
    void setQmlEngine(QQmlEngine* engine) { m_engine = engine; }

    // Selects a language: persists it, installs the catalogue and retranslates
    // the live UI. Returns false only if the code is not offered.
    Q_INVOKABLE bool selectLanguage(const QString& code);

    // Diagnostics: catalogue that failed to load, if any. Empty when fine.
    Q_INVOKABLE QString lastLoadDiagnostic() const { return m_diagnostic; }

    static QList<ta::app::LanguageOption> supportedLanguages();

signals:
    void languageChanged();
    void restartRequiredChanged();

private:
    bool installCatalogue(const QString& code);
    void persist(const QString& code);
    QString readPersisted() const;

    QString      m_configPath;
    QString      m_code;
    QString      m_diagnostic;
    bool         m_restartRequired = false;
    QTranslator  m_translator;
    bool         m_installed = false;
    QQmlEngine*  m_engine = nullptr;
};

#endif // LANGUAGESERVICE_H
