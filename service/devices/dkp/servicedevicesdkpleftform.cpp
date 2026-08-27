#include "servicedevicesdkpleftform.h"
#include "ui_servicedevicesdkpleftform.h"

#include "mainwindow.h"

ServiceDevicesDKPLeftForm::ServiceDevicesDKPLeftForm(CanController* can, QWidget *parent)  :
    QWidget(parent),
    ui(new Ui::ServiceDevicesDKPLeftForm)
{
    ui->setupUi(this);

    _parent = parent;
    _can = can;
}

ServiceDevicesDKPLeftForm::~ServiceDevicesDKPLeftForm(){
    delete ui;
}

void ServiceDevicesDKPLeftForm::checkDKP(DeviceStates key, QLabel* label){
    QString path = "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_";
    path += (_can->getState(key)? "on.png);":"off.png);");
    if (label->styleSheet() != path)
        label->setStyleSheet(path);
}

void ServiceDevicesDKPLeftForm::updateVisual(){
    checkDKP(StateDKPDumpLeft, ui->label_dumpLeftDKP1);
    checkDKP(StateDKPDumpRight, ui->label_dumpRightDKP1);
    checkDKP(StateDKPDumpUp, ui->label_dumpUpDKP1);
    checkDKP(StateDKPBroomLeft, ui->label_broomLeftDKP1);
    checkDKP(StateDKPBroomRight, ui->label_broomRightDKP1);
    checkDKP(StateDKPBroomUp, ui->label_broomUpDKP1);
    checkDKP(StateDKPBlowerUp1, ui->label_blowUp11);
    checkDKP(StateDKPBlowerUp2, ui->label_blowUp21);
    checkDKP(StateDKPBackMagnetUp, ui->label_magnetUp1);

    if (_can->getSensorPower() != ui->pushButton_sensorsPower->isChecked())
        ui->pushButton_sensorsPower->setChecked(_can->getSensorPower());

    if (_can->getHydraulicFan() != ui->pushButton_hydraulicCoolingFan->isChecked())
        ui->pushButton_hydraulicCoolingFan->setChecked(_can->getHydraulicFan());
}

void ServiceDevicesDKPLeftForm::on_pushButton_sensorsPower_clicked(){
    _can->invertState(StateSensorsPower);
}

void ServiceDevicesDKPLeftForm::on_pushButton_hydraulicCoolingFan_clicked(){
    _can->invertState(StateHydraulicFan);
}
