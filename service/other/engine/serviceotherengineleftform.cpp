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

void ServiceOtherEngineLeftForm::updateVisual(){
    auto mainWindow = ((MainWindow*)parent);
    auto view = mainWindow->getView();
    QString path = "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_";
    view->setText(ui->label_needRPM, QString::number(rpm_need/8));
    mainWindow->canForEngine->setEngineCommand(rpm_need);

    // обороты
    view ->setText(ui->label_realRPM, QString::number(mainWindow->engine->rpm));
    // if (ui->label_realRPM->text() != QString::number(mainWindow->engine->rpm))
    //     ui->label_realRPM->setText(QString::number(mainWindow->engine->rpm));

    // выходы
    mainWindow->starter->setStarterPressed(ui->pushButton_starter->isDown());

    if (mainWindow->can0->getState(StateIgnitionOut).toBool() != ui->pushButton_ignition->isChecked()){
        ui->pushButton_ignition->setChecked(mainWindow->can0->getState(StateIgnitionOut).toBool());
        //qDebug()<<"Нажали стартер";
    }

    // входы
    view->setStyle(ui->label_canExternal1, path + (mainWindow->canj1939->canFailStatus? "off.png);":"on.png);"));
    view->setStyle(ui->label_canInternal1, path + (mainWindow->canj1939Main->canFailStatus? "off.png);":"on.png);"));

    QString text = QString::number(mainWindow->engine->engineCoolantTemp);
    view->setText(ui->label_temperatureExternal, text + "C t ДВС");
    text = QString::number(mainWindow->engineCoolantTemp);
    view->setText(ui->label_temperatureInternal, text + "C t ДВС");
    text = QString::number(mainWindow->vehicleVoltage, 'f', 1);
    view->setText(ui->label_voltageInternal, text + text + "V U БОРТ");
}

void ServiceOtherEngineLeftForm::on_pushButton_ignition_clicked(){
    ((MainWindow*)parent)->can0->setState(StateIgnitionOut, !((MainWindow*)parent)->can0->getState(StateIgnitionOut).toBool());
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
