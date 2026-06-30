#include "servicedevicesdkpleftform.h"
#include "ui_servicedevicesdkpleftform.h"

#include "mainwindow.h"

ServiceDevicesDKPLeftForm::ServiceDevicesDKPLeftForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceDevicesDKPLeftForm)
{
    ui->setupUi(this);

    parent = parent_;
}

ServiceDevicesDKPLeftForm::~ServiceDevicesDKPLeftForm()
{
    delete ui;
}

void ServiceDevicesDKPLeftForm::checkDKP(DeviceStates state, QLabel* label)
{
    if (((MainWindow*)parent)->can0->getState(state).toBool())
    {
        if (label->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);")
            label->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);");
    }
    else
    {
        if (label->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);")
            label->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);");
    }
}

void ServiceDevicesDKPLeftForm::updateVisual()
{
    checkDKP(StateDKPDumpLeft, ui->label_dumpLeftDKP1);
    checkDKP(StateDKPDumpRight, ui->label_dumpRightDKP1);
    checkDKP(StateDKPDumpUp, ui->label_dumpUpDKP1);
    checkDKP(StateDKPBroomLeft, ui->label_broomLeftDKP1);
    checkDKP(StateDKPBroomRight, ui->label_broomRightDKP1);
    checkDKP(StateDKPBroomUp, ui->label_broomUpDKP1);
    checkDKP(StateDKPBlowerUp1, ui->label_blowUp11);
    checkDKP(StateDKPBlowerUp2, ui->label_blowUp21);
    checkDKP(StateDKPBackMagnetUp, ui->label_magnetUp1);

    if (((MainWindow*)parent)->can0->getState(StateSensorsPower).toBool() != ui->pushButton_sensorsPower->isChecked())
        ui->pushButton_sensorsPower->setChecked(((MainWindow*)parent)->can0->getState(StateSensorsPower).toBool());
    if (((MainWindow*)parent)->can0->getState(StateHydraulicFan).toBool() != ui->pushButton_hydraulicCoolingFan->isChecked())
        ui->pushButton_hydraulicCoolingFan->setChecked(((MainWindow*)parent)->can0->getState(StateHydraulicFan).toBool());
}

void ServiceDevicesDKPLeftForm::on_pushButton_sensorsPower_clicked()
{
    ((MainWindow*)parent)->can0->setState(StateSensorsPower, !((MainWindow*)parent)->can0->getState(StateSensorsPower).toBool());
}

void ServiceDevicesDKPLeftForm::on_pushButton_hydraulicCoolingFan_clicked()
{
    ((MainWindow*)parent)->can0->setState(StateHydraulicFan, !((MainWindow*)parent)->can0->getState(StateHydraulicFan).toBool());
}
