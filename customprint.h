#ifndef CUSTOMPRINT_H
#define CUSTOMPRINT_H

#include <QObject>
#include <QVariant>
#include "ModReader/forms/tachuswidget.h"

class TestModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TestModel(QObject *parent = 0);

    void populateData(const QList<QString> &contactName,const QList<QString> &contactPhone);

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

private:
    QList<QString> tm_contact_name;
    QList<QString> tm_contact_phone;

};

class CustomPrint : public QObject
{
    Q_OBJECT
public:
    explicit CustomPrint(TachusWidget* tachus, QObject *parent = nullptr);

    Q_INVOKABLE void print(QVariant data);
    Q_INVOKABLE void printPNG(QVariant data);
    void printTest();
    Q_INVOKABLE void clearImagesList();
    Q_INVOKABLE void addImage(QVariant data);
    Q_INVOKABLE void createPdf();
    Q_INVOKABLE void createTablePdf();
    Q_INVOKABLE void createSummryPdf();
    Q_INVOKABLE void createFinalsPdf();
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
