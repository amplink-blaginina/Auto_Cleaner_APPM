#include "serviceotherengineleftform.h"
#include "qdebug.h"
#include "ui_serviceotherengineleftform.h"

#include "mainwindow.h"

ServiceOtherEngineLeftForm::ServiceOtherEngineLeftForm(MainWindow* mainWindow, QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceOtherEngineLeftForm){

    ui->setupUi(this);
    _mainWindow = mainWindow;
    parent = parent_;
    rpm_need = 6400;//800
}

ServiceOtherEngineLeftForm::~ServiceOtherEngineLeftForm(){
    delete ui;
}
bool ServiceOtherEngineLeftForm::starterBtnStatus(){
    return ui->pushButton_starter->isDown();
}
void ServiceOtherEngineLeftForm::updateVisual(){
    auto view = _mainWindow->getView();
    QString path = "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_";
    view->setText(ui->label_needRPM, QString::number(rpm_need/8));
    _mainWindow->canForEngine->setEngineCommand(rpm_need);

    view ->setText(ui->label_realRPM, QString::number(_mainWindow->engine->rpm));// обороты

    //_mainWindow->gpioMatirx->keyPressed == GPIOInput::IN_BLOW_LEFT
    //_mainWindow->starter->setStarterPressed(ui->pushButton_starter->isDown() || _mainWindow->gpioMatirx->keyPressed == GPIOInput::IN_BLOW_LEFT);// выходы

    auto getIgnition = _mainWindow->can->getIgnition();
    if (getIgnition != ui->pushButton_ignition->isChecked()){
        ui->pushButton_ignition->setChecked(getIgnition);
        //qDebug()<<"Нажали стартер";
    }

    // входы
    view->setStyle(ui->label_canExternal1, path + (_mainWindow->canj1939->canFailStatus? "off.png);":"on.png);"));
    view->setStyle(ui->label_canInternal1, path + (_mainWindow->canj1939Main->canFailStatus? "off.png);":"on.png);"));

    auto curState = _mainWindow->currentState;
    view->setText(ui->label_temperatureExternal, QString::number(_mainWindow->engine->engineCoolantTemp) + "C t ДВС");
    view->setText(ui->label_temperatureInternal, QString::number(curState->engineCoolantTemp) + "C t ДВС");
    view->setText(ui->label_voltageInternal, QString::number(curState->vehicleVoltage, 'f', 1) + "V U БОРТ");
}

void ServiceOtherEngineLeftForm::on_pushButton_ignition_clicked(){
    qDebug()<<"* ignition";
    _mainWindow->can->invertIgnition();
}

void ServiceOtherEngineLeftForm::on_pushButton_preroll_clicked(){
    _mainWindow->preroll->setPrerollPressed(true);
    qDebug()<<"* preroll";
}

void ServiceOtherEngineLeftForm::on_pushButton_starter_pressed(){
    _mainWindow->starter->setStarterPressed(true);
    qDebug()<<"* starter pressed";
}

void ServiceOtherEngineLeftForm::on_pushButton_starter_released(){
    _mainWindow->starter->setStarterPressed(false);
    qDebug()<<"* starter released";
}


void ServiceOtherEngineLeftForm::on_pushButton_starterPreroll_clicked(){
    qDebug()<<"* starterPreroll";
}


void ServiceOtherEngineLeftForm::on_pushButton_lessRPM_clicked()
{
    if (rpm_need > 6400){
        rpm_need -= 800;
//        ui->label_needRPM->setText(QString::number(rpm_need/8));
//        _mainWindow->canForEngine->setEngineCommand(rpm_need);
    }
}

void ServiceOtherEngineLeftForm::on_pushButton_moreRPM_clicked()
{
    if (rpm_need < 17600){
        rpm_need += 800;
//        ui->label_needRPM->setText(QString::number(rpm_need/8));
//        _mainWindow->canForEngine->setEngineCommand(rpm_need);
    }
}
