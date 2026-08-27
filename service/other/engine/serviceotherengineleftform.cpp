#include "serviceotherengineleftform.h"
#include "ui_serviceotherengineleftform.h"

#include "mainwindow.h"

ServiceOtherEngineLeftForm::ServiceOtherEngineLeftForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceOtherEngineLeftForm){

    ui->setupUi(this);
    parent = parent_;
    rpm_need = 6400;//800
}

ServiceOtherEngineLeftForm::~ServiceOtherEngineLeftForm(){
    delete ui;
}

void ServiceOtherEngineLeftForm::updateVisual(){
    auto mainWindow = ((MainWindow*)parent);
    auto view = mainWindow->getView();
    QString path = "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_";
    view->setText(ui->label_needRPM, QString::number(rpm_need/8));
    mainWindow->canForEngine->setEngineCommand(rpm_need);

    view ->setText(ui->label_realRPM, QString::number(mainWindow->engine->rpm));// обороты
    mainWindow->starter->setStarterPressed(ui->pushButton_starter->isDown());// выходы
//
    auto getIgnition = mainWindow->can->getIgnition();
    qDebug()<<"#Ignition: "<< getIgnition;
    if (getIgnition != ui->pushButton_ignition->isChecked()){
        ui->pushButton_ignition->setChecked(getIgnition);
        //qDebug()<<"Нажали стартер";
    }

    // входы
    view->setStyle(ui->label_canExternal1, path + (mainWindow->canj1939->canFailStatus? "off.png);":"on.png);"));
    view->setStyle(ui->label_canInternal1, path + (mainWindow->canj1939Main->canFailStatus? "off.png);":"on.png);"));

    auto curState = mainWindow->currentState;
    view->setText(ui->label_temperatureExternal, QString::number(mainWindow->engine->engineCoolantTemp) + "C t ДВС");
    view->setText(ui->label_temperatureInternal, QString::number(curState->engineCoolantTemp) + "C t ДВС");
    view->setText(ui->label_voltageInternal, QString::number(curState->vehicleVoltage, 'f', 1) + "V U БОРТ");
}

void ServiceOtherEngineLeftForm::on_pushButton_ignition_clicked(){
    ((MainWindow*)parent)->can->invertIgnition();
}

void ServiceOtherEngineLeftForm::on_pushButton_lessRPM_clicked()
{
    if (rpm_need > 6400){
        rpm_need -= 800;
//        ui->label_needRPM->setText(QString::number(rpm_need/8));
//        ((MainWindow*)parent)->canForEngine->setEngineCommand(rpm_need);
    }
}

void ServiceOtherEngineLeftForm::on_pushButton_moreRPM_clicked()
{
    if (rpm_need < 17600){
        rpm_need += 800;
//        ui->label_needRPM->setText(QString::number(rpm_need/8));
//        ((MainWindow*)parent)->canForEngine->setEngineCommand(rpm_need);
    }
}
