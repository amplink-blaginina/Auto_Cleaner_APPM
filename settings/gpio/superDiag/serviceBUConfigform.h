#ifndef SERVICEBUCONFIGFORM_H
#define SERVICEBUCONFIGFORM_H

#include <QWidget>

#include "serviceBUConfigElementform.h"
#include <can/mycan.h>

namespace Ui {
class ServiceBUConfigForm;
}

class ServiceBUConfigForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceBUConfigForm(CanController* can, QWidget *parent = nullptr);
    ~ServiceBUConfigForm();

    void checkElement(QFrame* frame, QLabel* label, QString frame_name, bool state);

    QTimer mainProgressTimer;

    SystemConfigure systemConfigure;
    QMap<int, SystemElement*> systemElements;
    bool BUCPConfigured;

    QList<ServiceBUConfigElementForm*> boards;

private slots:
    void mainProgress();
    void on_pushButton_exit_clicked();

private:
    Ui::ServiceBUConfigForm *ui;
    CanController* _can;
    QWidget* _parent;
};

#endif // SERVICEBUCONFIGFORM_H
