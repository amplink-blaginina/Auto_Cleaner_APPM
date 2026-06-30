#ifndef SERVICEOTHERENGINELEFTFORM_H
#define SERVICEOTHERENGINELEFTFORM_H

#include <QWidget>
#include <QLabel>
#include <QSlider>

#include <can/mycan.h>
#include <interface_button/interfacebutton.h>

namespace Ui {
class ServiceOtherEngineLeftForm;
}

class ServiceOtherEngineLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceOtherEngineLeftForm(QWidget *parent = nullptr);
    ~ServiceOtherEngineLeftForm();

    void updateVisual();

    quint16 rpm_need;

    QWidget* parent;



private slots:

    void on_pushButton_ignition_clicked();

    void on_pushButton_lessRPM_clicked();

    void on_pushButton_moreRPM_clicked();

private:
    Ui::ServiceOtherEngineLeftForm *ui;
};

#endif // SERVICEOTHERENGINELEFTFORM_H
