#ifndef CUSTOMPRINT_H
#define CUSTOMPRINT_H

#include <QObject>
#include <QVariant>
#include "ModReader/forms/tachuswidget.h"


class CustomPrint : public QObject
{
    Q_OBJECT
public:
    explicit CustomPrint(TachusWidget* tachus, QObject *parent = nullptr);

    Q_INVOKABLE void print(QVariant data);
    Q_INVOKABLE void clearImagesList();
    Q_INVOKABLE void addImage(QVariant data);
    Q_INVOKABLE void createPdf();
    Q_INVOKABLE void createSummryPdf();
    Q_INVOKABLE void createFinalsPdf();
    // T1.4: Training-specific PDF. Writes the accumulated A4 page images to
    // `filePath` directly (no blocking file dialog), one image per page fitted
    // KeepAspectRatio. Emits printingNotice with the saved path on success, or
    // an error notice if the file could not be written. Returns true on success.
    Q_INVOKABLE bool createTrainingPdf(QString filePath);
    Q_INVOKABLE void setServerPath(QString path);
    Q_INVOKABLE void createPdf(QString filePath);

signals:
    void saveComplete();
    // TechAim dialog framework (C5): auto-export progress notice, rendered by
    // the QML dialogManager (auto-dismisses after timeoutMs).
    void printingNotice(QString message, int timeoutMs);

public slots:
private:
    QList <QImage> m_images;
    QString m_serverPath;
    TachusWidget* m_tachus;
};

#endif // CUSTOMPRINT_H
