#include "serviceotherengineleftform.h"
#include "ui_serviceotherengineleftform.h"

#include "mainwindow.h"

ServiceOtherEngineLeftForm::ServiceOtherEngineLeftForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceOtherEngineLeftForm)
{
    ui->setupUi(this);

    parent = parent_;

    rpm_need=6400;//800
}

ServiceOtherEngineLeftForm::~ServiceOtherEngineLeftForm()
{
    delete ui;
}

void ServiceOtherEngineLeftForm::updateVisual()
{
    if (ui->label_needRPM->text() != QString::number(rpm_need/8))
        ui->label_needRPM->setText(QString::number(rpm_need/8));
    ((MainWindow*)parent)->canForEngine->setEngineCommand(rpm_need);

    // обороты
    if (ui->label_realRPM->text() != QString::number(((MainWindow*)parent)->engine->rpm))
        ui->label_realRPM->setText(QString::number(((MainWindow*)parent)->engine->rpm));
    // выходы
    if (ui->pushButton_starter->isDown())
        ((MainWindow*)parent)->gpio->setOutput(GPIOOutput::OUT_STARTER, true);
    else
        ((MainWindow*)parent)->gpio->setOutput(GPIOOutput::OUT_STARTER, false);
    if (((MainWindow*)parent)->can0->getState(StateIgnitionOut).toBool() != ui->pushButton_ignition->isChecked())
        ui->pushButton_ignition->setChecked(((MainWindow*)parent)->can0->getState(StateIgnitionOut).toBool());
    // входы
    if (!((MainWindow*)parent)->canj1939->canFailStatus)
    {
        if (ui->label_canExternal1->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);")
            ui->label_canExternal1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);");
    }
    else
    {
        if (ui->label_canExternal1->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);")
            ui->label_canExternal1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);");
    }
    if (!((MainWindow*)parent)->canj1939Main->canFailStatus)
    {
        if (ui->label_canInternal1->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);")
            ui->label_canInternal1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);");
    }
    else
    {
        if (ui->label_canInternal1->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);")
            ui->label_canInternal1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);");
    }
    QString text = QString::number(((MainWindow*)parent)->engine->engineCoolantTemp);
    if (ui->label_temperatureExternal->text() != text + "C t ДВС")
        ui->label_temperatureExternal->setText(text + "C t ДВС");
    text = QString::number(((MainWindow*)parent)->engineCoolantTemp);
    if (ui->label_temperatureInternal->text() != text + "C t ДВС")
        ui->label_temperatureInternal->setText(text + "C t ДВС");
    text = QString::number(((MainWindow*)parent)->vehicleVoltage, 'f', 1);
    if (ui->label_voltageInternal->text() != text + "V U БОРТ")
        ui->label_voltageInternal->setText(text + "V U БОРТ");
}

void ServiceOtherEngineLeftForm::on_pushButton_ignition_clicked()
{
    ((MainWindow*)parent)->can0->setState(StateIgnitionOut, !((MainWindow*)parent)->can0->getState(StateIgnitionOut).toBool());
}

void ServiceOtherEngineLeftForm::on_pushButton_lessRPM_clicked()
{
    if (rpm_need > 6400)
    {
        rpm_need -= 800;
//        ui->label_needRPM->setText(QString::number(rpm_need/8));
//        ((MainWindow*)parent)->canForEngine->setEngineCommand(rpm_need);
    }
}

void ServiceOtherEngineLeftForm::on_pushButton_moreRPM_clicked()
{
    if (rpm_need < 17600)
    {
        rpm_need += 800;
//        ui->label_needRPM->setText(QString::number(rpm_need/8));
//        ((MainWindow*)parent)->canForEngine->setEngineCommand(rpm_need);
    }
}
