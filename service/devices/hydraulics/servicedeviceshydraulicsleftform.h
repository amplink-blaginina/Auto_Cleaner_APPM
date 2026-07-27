#ifndef SERVICEDEVICESHYDRAULICSLEFTFORM_H
#define SERVICEDEVICESHYDRAULICSLEFTFORM_H

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <screenlog.h>

#include <can/mycan.h>
#include <interface_button/interfacebutton.h>

namespace Ui {
class ServiceDevicesHydraulicsLeftForm;
}

class ServiceDevicesHydraulicsLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceDevicesHydraulicsLeftForm(ScreenLog *logger_,QWidget *parent = nullptr);
    ~ServiceDevicesHydraulicsLeftForm();

    void refreshSliders();
    void updateVisual();

    QWidget* parent;

private slots:
    void on_horizontalSlider_broomRotateLeft_valueChanged(int value);

    void on_horizontalSlider_broomRotateRight_valueChanged(int value);

    void on_horizontalSlider_fanRotateRight_valueChanged(int value);

    void on_pushButton_dumpFlow_clicked();

    void on_pushButton_broomFlow_clicked();

    void on_pushButton_broomPressUp_clicked();

    void on_pushButton_broomPressDown_clicked();

private:
    Ui::ServiceDevicesHydraulicsLeftForm *ui;
    ScreenLog* logger;
};

#endif // SERVICEDEVICESHYDRAULICSLEFTFORM_H
