#ifndef SERVICEBUCONFIGELEMENTFORM_H
#define SERVICEBUCONFIGELEMENTFORM_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>

#include <can/mycan.h>

namespace Ui {
class ServiceBUConfigElementForm;
}

class ServiceBUConfigElementForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceBUConfigElementForm(int boardNum_, SystemConfigure* systemConfigure_, QMap<int, SystemElement*>* systemElements_,  bool* BUCPConfigured_, QWidget *parentMain, QWidget *parent = nullptr);
    ~ServiceBUConfigElementForm();

    QMap<int, QString> boardsString;
    QList<QString> channelsString;
    QList<QPushButton*> buttonsLess;
    QList<QPushButton*> buttonsMore;
    QList<QLabel*> values;
    QList<QLabel*> names;
    QList<QProgressBar*> progressBars;

    int getBoardType();
    bool isConfigured();
    void fillBoard();

    QWidget* parent;
    QWidget* parentMain;

    QTimer mainProgressTimer;

    int boardNum;
    SystemConfigure* systemConfigure;
    QMap<int, SystemElement*>* systemElements;
    bool* BUCPConfigured;

private slots:
    void mainProgress();
    void valuesClicked();

private:
    Ui::ServiceBUConfigElementForm *ui;
};

#endif // SERVICEBUCONFIGELEMENTFORM_H
