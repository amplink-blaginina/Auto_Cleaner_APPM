#include "servicedeviceshydraulicsleftform.h"
#include "ui_servicedeviceshydraulicsleftform.h"

#include "mainwindow.h"

ServiceDevicesHydraulicsLeftForm::ServiceDevicesHydraulicsLeftForm(CanController* can, ViewController* view, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServiceDevicesHydraulicsLeftForm)
{
    ui->setupUi(this);
    _logger = view->screenLog;
    _parent = parent;
    _can = can;
    _view = view;
}

ServiceDevicesHydraulicsLeftForm::~ServiceDevicesHydraulicsLeftForm(){
    delete ui;
}

void ServiceDevicesHydraulicsLeftForm::refreshSliders(){
    ui->horizontalSlider_broomRotateLeft->setValue(_can->getInt(StateValveD2));
    ui->horizontalSlider_broomRotateRight->setValue(_can->getInt(StateValveD1));
    ui->horizontalSlider_fanRotateRight->setValue(_can->getInt(StateValveD3));
}

void ServiceDevicesHydraulicsLeftForm::updateVisual(){
    auto mainWindow = (MainWindow*)_parent;
    if (_can->getState(StateValveC1) != ui->pushButton_broomFlow->isChecked())
        ui->pushButton_broomFlow->setChecked(_can->getState(StateValveC1));
    if (_can->getState(StateValveC3) != ui->pushButton_dumpFlow->isChecked())
        ui->pushButton_dumpFlow->setChecked(_can->getState(StateValveC3));

    updateBroomPress();

    if (ui->pushButton_dumpLeft->isDown())
        mainWindow->frontRail->setDirection(organsEnums::Left);
    else if (ui->pushButton_dumpRight->isDown())
        mainWindow->frontRail->setDirection(organsEnums::Right);
    else if (ui->pushButton_dumpUp->isDown())
        mainWindow->frontRail->goUp(true);//setDirection(organsEnums::Up)
    else if (ui->pushButton_dumpDown->isDown())
        mainWindow->frontRail->goDown(true);//setDirection(organsEnums::Down)
    else
        mainWindow->frontRail->setDirection(organsEnums::None);

    if (ui->pushButton_broomLeft->isDown())
        mainWindow->broomCentral->setDirection(organsEnums::Left);
    else if (ui->pushButton_broomRight->isDown())
        mainWindow->broomCentral->setDirection(organsEnums::Right);
    else if (ui->pushButton_broomUp->isDown())
        mainWindow->broomCentral->goUpImmediate(true);//setDirection(organsEnums::Up, false);
    else if (ui->pushButton_broomDown->isDown())
        mainWindow->broomCentral->goDownImmediate(true);//setDirection(organsEnums::Down, false);
    else
        mainWindow->broomCentral->setDirection(organsEnums::None);


    if (ui->pushButton_blowLeft->isDown())
        mainWindow->blower->goSlide(false);
    else if (ui->pushButton_blowRight->isDown())
        mainWindow->blower->goSlide(true);
    else if (ui->pushButton_blowUp->isDown())
        mainWindow->blower->goUp();
    else if (ui->pushButton_blowDown->isDown())
        mainWindow->blower->goDown();
    else
        mainWindow->blower->goOff();

    if (ui->pushButton_magnetUp->isDown())
        mainWindow->backMagnet->goUp();
    else if (ui->pushButton_magnetDown->isDown())
        mainWindow->backMagnet->goDown();
    else
        mainWindow->backMagnet->goOff();

    QString text = QString::number(mainWindow->hydroTempK * _can->getOilTmp() + mainWindow->hydroTempB, 'f', 1);
    _view->setText(ui->label_hydraulicTemperature, text + " C ТЕМП ГО");

    text = QString::number(mainWindow->hydraulicPressureValue(0), 'f', 1);
    _view->setText(ui->label_hydraulicPressure, text + " P ТИ1");

    text = QString::number(mainWindow->hydraulicPressureValue(1), 'f', 1);
    _view->setText(ui->label_hydraulicPressure2, text + " P ТИ2");


    QString path = "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_";
    _view->setStyle(ui->label_hydraulicLevel1,
                    path + (_can->getState(StateHydroTankLevelD27)? "on.png);": "off.png);"));
    _view->setStyle(ui->label_drainFilter1,
                    path + (_can->getState(StateDrainFilterD28)? "on.png);": "off.png);"));
    _view->setStyle(ui->label_powerFilter1,
                    path + (_can->getState(StatePressureFilter1)? "on.png);": "off.png);"));
    _view->setStyle(ui->label_powerFilter2,
                    path + (_can->getState(StatePressureFilter2)? "on.png);": "off.png);"));
}

void ServiceDevicesHydraulicsLeftForm::on_horizontalSlider_broomRotateLeft_valueChanged(int value){
    _can->setState(StateValveD2, value);
}

void ServiceDevicesHydraulicsLeftForm::on_horizontalSlider_broomRotateRight_valueChanged(int value){
    _can->setState(StateValveD1, value);
}

void ServiceDevicesHydraulicsLeftForm::on_horizontalSlider_fanRotateRight_valueChanged(int value){
    _can->setState(StateValveD3, value);
}

void ServiceDevicesHydraulicsLeftForm::on_pushButton_dumpFlow_clicked(){
    _can->invertState(StateValveC3);
    _can->invertState(StateValveC4);
}

void ServiceDevicesHydraulicsLeftForm::on_pushButton_broomFlow_clicked(){
    _can->invertState(StateValveC1);
    _can->invertState(StateValveC2);
}

void ServiceDevicesHydraulicsLeftForm::updateBroomPress(){
    // отжим (F2) и прижим (F8) щётки внутри портала - пока держат кнопку, через орган: он открывает A1.
    // Обе сразу не включаем: сначала выключаем другую (её выключение освобождает A1), потом включаем нужную
    auto broom = ((MainWindow*)_parent)->broomCentral;
    if (ui->pushButton_broomPressUp->isDown()){
        broom->goPressDown(false);
        broom->goPressUp(true);
        _broomPressHeld = true;
    }
    else if (ui->pushButton_broomPressDown->isDown()){
        broom->goPressUp(false);
        broom->goPressDown(true);
        _broomPressHeld = true;
    }
    else if (_broomPressHeld){// отпустили - выключаем один раз, чтобы не мешать рукоятке КВ
        broom->stopPress();
        _broomPressHeld = false;
    }
}

void ServiceDevicesHydraulicsLeftForm::hideEvent(QHideEvent *event){
    QWidget::hideEvent(event);
    if (_broomPressHeld){
        ((MainWindow*)_parent)->broomCentral->stopPress();
        _broomPressHeld = false;
    }
}
