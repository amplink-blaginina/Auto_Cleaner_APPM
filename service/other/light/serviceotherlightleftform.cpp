#include "serviceotherlightleftform.h"
#include "ui_serviceotherlightleftform.h"

#include "mainwindow.h"

ServiceOtherLightLeftForm::ServiceOtherLightLeftForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceOtherLightLeftForm)
{
    ui->setupUi(this);

    parent = parent_;
}

ServiceOtherLightLeftForm::~ServiceOtherLightLeftForm()
{
    delete ui;
}

void ServiceOtherLightLeftForm::updateVisual()
{
    if (((MainWindow*)parent)->can0->getState(StateFRMBroomL1).toBool() != ui->pushButton_lightFRM1->isChecked())
        ui->pushButton_lightFRM1->setChecked(((MainWindow*)parent)->can0->getState(StateFRMBroomL1).toBool());
    if (((MainWindow*)parent)->can0->getState(StateFRMBackL2).toBool() != ui->pushButton_lightFRM2->isChecked())
        ui->pushButton_lightFRM2->setChecked(((MainWindow*)parent)->can0->getState(StateFRMBackL2).toBool());
    if (((MainWindow*)parent)->can0->getState(StateKungL5).toBool() != ui->pushButton_lightFRM3->isChecked())
        ui->pushButton_lightFRM3->setChecked(((MainWindow*)parent)->can0->getState(StateKungL5).toBool());
}

void ServiceOtherLightLeftForm::on_pushButton_lightFRM1_clicked()
{
    ((MainWindow*)parent)->can0->setState(StateFRMBroomL1, !((MainWindow*)parent)->can0->getState(StateFRMBroomL1).toBool());
}

void ServiceOtherLightLeftForm::on_pushButton_lightFRM2_clicked()
{
    ((MainWindow*)parent)->can0->setState(StateFRMBackL2, !((MainWindow*)parent)->can0->getState(StateFRMBackL2).toBool());
}

void ServiceOtherLightLeftForm::on_pushButton_lightFRM3_clicked()
{
    ((MainWindow*)parent)->can0->setState(StateKungL5, !((MainWindow*)parent)->can0->getState(StateKungL5).toBool());
}

//void ServiceOtherLightLeftForm::on_pushButton_lightBeam1_clicked()
//{
//    ((MainWindow*)parent)->can0->setState(StateOUTBeamFrontOut, !((MainWindow*)parent)->can0->getState(StateOUTBeamFrontOut).toBool());
//}

//void ServiceOtherLightLeftForm::on_pushButton_lightBeam2_clicked()
//{
//    ((MainWindow*)parent)->can0->setState(StateOUTBeamBackOut, !((MainWindow*)parent)->can0->getState(StateOUTBeamBackOut).toBool());
//}
