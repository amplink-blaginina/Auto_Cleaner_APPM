#ifndef SERVICEDEVICESDKPLEFTFORM_H
#define SERVICEDEVICESDKPLEFTFORM_H

#include <QWidget>
#include <QLabel>
#include <QSlider>

#include <can/mycan.h>
#include <interface_button/interfacebutton.h>

namespace Ui {
class ServiceDevicesDKPLeftForm;
}

class ServiceDevicesDKPLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceDevicesDKPLeftForm(QWidget *parent = nullptr);
    ~ServiceDevicesDKPLeftForm();

    void checkDKP(DeviceStates state, QLabel* label);
    void updateVisual();

    QWidget* parent;



private slots:

    void on_pushButton_sensorsPower_clicked();

    void on_pushButton_hydraulicCoolingFan_clicked();

private:
    Ui::ServiceDevicesDKPLeftForm *ui;
};

#endif // SERVICEDEVICESDKPLEFTFORM_H
