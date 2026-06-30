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
    Q_INVOKABLE void setServerPath(QString path);
    Q_INVOKABLE void createPdf(QString filePath);

signals:
    void saveComplete();

public slots:
private:
    QList <QImage> m_images;
    QString m_serverPath;
    TachusWidget* m_tachus;
};

#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

class TimedMessageBox : public QMessageBox
{
Q_OBJECT

public:
   TimedMessageBox(int timeoutSeconds, const QString & title, const QString & text, Icon icon, int button0, int button1, int button2,
                   QWidget * parent, Qt::WindowFlags flags = (Qt::WindowFlags) Qt::Dialog|Qt::MSWindowsFixedSizeDialogHint)
      : QMessageBox(title, text, icon, button0, button1, button2, parent, flags)
      , _timeoutSeconds(timeoutSeconds+1)
      , _text(text)
   {
      connect(&_timer, SIGNAL(timeout()), this, SLOT(Tick()));
      _timer.setInterval(1000);
      setText(_text);
   }

   virtual void showEvent(QShowEvent * e)
   {
      QMessageBox::showEvent(e);
      Tick();
      _timer.start();
   }

   void setOkayButtonText(QString okayText) {
       setButtonText(QMessageBox::Ok, okayText);
   }

private slots:
   void Tick()
   {
      if (--_timeoutSeconds >= 0) setOkayButtonText(QString("Okay (%1)").arg(_timeoutSeconds));
      else
      {
         _timer.stop();
         defaultButton()->animateClick();
      }
   }

private:
   QString _text;
   int _timeoutSeconds;
   QTimer _timer;
};
#endif // CUSTOMPRINT_H
