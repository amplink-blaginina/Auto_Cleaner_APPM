#include "servicedeviceshydraulicsleftform.h"
#include "ui_servicedeviceshydraulicsleftform.h"

#include "mainwindow.h"

ServiceDevicesHydraulicsLeftForm::ServiceDevicesHydraulicsLeftForm(ScreenLog *logger_, QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceDevicesHydraulicsLeftForm)
{
    ui->setupUi(this);
    logger = logger_;
    parent = parent_;
}

ServiceDevicesHydraulicsLeftForm::~ServiceDevicesHydraulicsLeftForm()
{
    delete ui;
}

void ServiceDevicesHydraulicsLeftForm::refreshSliders()
{
    ui->horizontalSlider_broomRotateLeft->setValue(((MainWindow*)parent)->can0->getState(StateValveD1).toInt());
    ui->horizontalSlider_broomRotateRight->setValue(((MainWindow*)parent)->can0->getState(StateValveD2).toInt());
    ui->horizontalSlider_fanRotateRight->setValue(((MainWindow*)parent)->can0->getState(StateValveD3).toInt());
}

void ServiceDevicesHydraulicsLeftForm::updateVisual()
{
    auto mainWindow = (MainWindow*)parent;
    if (mainWindow->can0->getState(StateValveC1).toBool() != ui->pushButton_broomFlow->isChecked())
        ui->pushButton_broomFlow->setChecked(mainWindow->can0->getState(StateValveC1).toBool());
    if (mainWindow->can0->getState(StateValveC3).toBool() != ui->pushButton_dumpFlow->isChecked())
        ui->pushButton_dumpFlow->setChecked(mainWindow->can0->getState(StateValveC3).toBool());

    if (mainWindow->can0->getState(StateValveF2).toBool() != ui->pushButton_broomPressUp->isChecked())
        ui->pushButton_broomPressUp->setChecked(mainWindow->can0->getState(StateValveF2).toBool());
    if (mainWindow->can0->getState(StateValveF8).toBool() != ui->pushButton_broomPressDown->isChecked())
        ui->pushButton_broomPressDown->setChecked(mainWindow->can0->getState(StateValveF8).toBool());

    if (ui->pushButton_dumpLeft->isDown())
        mainWindow->frontRail->setDirection(organsEnums::Left);
    else if (ui->pushButton_dumpRight->isDown())
        mainWindow->frontRail->setDirection(organsEnums::Right);
    else if (ui->pushButton_dumpUp->isDown())
        mainWindow->frontRail->setDirection(organsEnums::Up);
    else if (ui->pushButton_dumpDown->isDown())
        mainWindow->frontRail->setDirection(organsEnums::Down);
    else
        mainWindow->frontRail->setDirection(organsEnums::None);

    if (ui->pushButton_broomLeft->isDown())
        mainWindow->broomCentral->setDirection(organsEnums::Left);
    else if (ui->pushButton_broomRight->isDown())
        mainWindow->broomCentral->setDirection(organsEnums::Right);
    else if (ui->pushButton_broomUp->isDown())
        mainWindow->broomCentral->setDirection(organsEnums::Up, false);
    else if (ui->pushButton_broomDown->isDown())
        mainWindow->broomCentral->setDirection(organsEnums::Down, false);
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

    QString text = QString::number(mainWindow->hydroTempK * mainWindow->can0->getState(StateHydraulicOilTemperature).toUInt() + mainWindow->hydroTempB, 'f', 1);
    if (ui->label_hydraulicTemperature->text() != text + " C ТЕМП ГО")
        ui->label_hydraulicTemperature->setText(text + " C ТЕМП ГО");
    text = QString::number(mainWindow->hydraulicPressureValue(0), 'f', 1);
    if (ui->label_hydraulicPressure->text() != text + " P ТИ1")
        ui->label_hydraulicPressure->setText(text + " P ТИ1");
    text = QString::number(mainWindow->hydraulicPressureValue(1), 'f', 1);
    if (ui->label_hydraulicPressure2->text() != text + " P ТИ2")
        ui->label_hydraulicPressure2->setText(text + " P ТИ2");

    if (mainWindow->can0->getState(StateHydroTankLevelD27).toBool())
    {
        if (ui->label_hydraulicLevel1->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);")
            ui->label_hydraulicLevel1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);");
    }
    else
    {
        if (ui->label_hydraulicLevel1->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);")
            ui->label_hydraulicLevel1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);");
    }
    if (mainWindow->can0->getState(StateDrainFilterD28).toBool())
    {
        if (ui->label_drainFilter1->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);")
            ui->label_drainFilter1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);");
    }
    else
    {
        if (ui->label_drainFilter1->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);")
            ui->label_drainFilter1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);");
    }
    if (mainWindow->can0->getState(StatePressureFilter1).toBool())
    {
        if (ui->label_powerFilter1->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);")
            ui->label_powerFilter1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);");
    }
    else
    {
        if (ui->label_powerFilter1->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);")
            ui->label_powerFilter1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);");
    }
    if (mainWindow->can0->getState(StatePressureFilter2).toBool())
    {
        if (ui->label_powerFilter2->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);")
            ui->label_powerFilter2->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_on.png);");
    }
    else
    {
        if (ui->label_powerFilter2->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);")
            ui->label_powerFilter2->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/service/buttons/service_indication_off.png);");
    }
}

void ServiceDevicesHydraulicsLeftForm::on_horizontalSlider_broomRotateLeft_valueChanged(int value)
{
    ((MainWindow*)parent)->can0->setState(StateValveD1, value);
}

void ServiceDevicesHydraulicsLeftForm::on_horizontalSlider_broomRotateRight_valueChanged(int value)
{
    ((MainWindow*)parent)->can0->setState(StateValveD2, value);
}

void ServiceDevicesHydraulicsLeftForm::on_horizontalSlider_fanRotateRight_valueChanged(int value)
{
    ((MainWindow*)parent)->can0->setState(StateValveD3, value);
}

void ServiceDevicesHydraulicsLeftForm::on_pushButton_dumpFlow_clicked()
{
    ((MainWindow*)parent)->can0->setState(StateValveC3, !((MainWindow*)parent)->can0->getState(StateValveC3).toBool());
    ((MainWindow*)parent)->can0->setState(StateValveC4, !((MainWindow*)parent)->can0->getState(StateValveC4).toBool());
}

void ServiceDevicesHydraulicsLeftForm::on_pushButton_broomFlow_clicked()
{
    ((MainWindow*)parent)->can0->setState(StateValveC1, !((MainWindow*)parent)->can0->getState(StateValveC1).toBool());
    ((MainWindow*)parent)->can0->setState(StateValveC2, !((MainWindow*)parent)->can0->getState(StateValveC2).toBool());
}

void ServiceDevicesHydraulicsLeftForm::on_pushButton_broomPressUp_clicked()
{
    ((MainWindow*)parent)->can0->setState(StateValveF2, !((MainWindow*)parent)->can0->getState(StateValveF2).toBool());
}

void ServiceDevicesHydraulicsLeftForm::on_pushButton_broomPressDown_clicked()
{
    ((MainWindow*)parent)->can0->setState(StateValveF8, !((MainWindow*)parent)->can0->getState(StateValveF8).toBool());
}
