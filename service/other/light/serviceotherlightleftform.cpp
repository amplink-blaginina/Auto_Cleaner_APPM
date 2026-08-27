#include "serviceotherlightleftform.h"
#include "ui_serviceotherlightleftform.h"

#include "mainwindow.h"

ServiceOtherLightLeftForm::ServiceOtherLightLeftForm(CanController* can, QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceOtherLightLeftForm)
{
    ui->setupUi(this);
    _can = can;
    parent = parent_;
}

ServiceOtherLightLeftForm::~ServiceOtherLightLeftForm(){
    delete ui;
}

void ServiceOtherLightLeftForm::updateVisual(){
    if (_can->getState(StateFRMBroomL1) != ui->pushButton_lightFRM1->isChecked())
        ui->pushButton_lightFRM1->setChecked(_can->getState(StateFRMBroomL1));
    if (_can->getState(StateFRMBackL2) != ui->pushButton_lightFRM2->isChecked())
        ui->pushButton_lightFRM2->setChecked(_can->getState(StateFRMBackL2));
    if (_can->getState(StateKungL5) != ui->pushButton_lightFRM3->isChecked())
        ui->pushButton_lightFRM3->setChecked(_can->getState(StateKungL5));
}

void ServiceOtherLightLeftForm::on_pushButton_lightFRM1_clicked(){
    _can->invertState(StateFRMBroomL1);
}

void ServiceOtherLightLeftForm::on_pushButton_lightFRM2_clicked(){
    _can->invertState(StateFRMBackL2);
}

void ServiceOtherLightLeftForm::on_pushButton_lightFRM3_clicked(){
    _can->invertState(StateKungL5);
}

//void ServiceOtherLightLeftForm::on_pushButton_lightBeam1_clicked(){
//    _can->invertState(StateOUTBeamFrontOut);
//}

//void ServiceOtherLightLeftForm::on_pushButton_lightBeam2_clicked(){
//    _can->invertState(StateOUTBeamBackOut);
//}
