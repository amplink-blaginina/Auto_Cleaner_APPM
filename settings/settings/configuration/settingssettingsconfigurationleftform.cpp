#include "settingssettingsconfigurationleftform.h"
#include "ui_settingssettingsconfigurationleftform.h"

#include "mainwindow.h"

SettingsSettingsConfigurationLeftForm::SettingsSettingsConfigurationLeftForm(CanController* can, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsSettingsConfigurationLeftForm)
{
    ui->setupUi(this);
    _can = can;
    _parent = parent;

}

SettingsSettingsConfigurationLeftForm::~SettingsSettingsConfigurationLeftForm(){
    delete ui;
}

void SettingsSettingsConfigurationLeftForm::updateVisual(){
}

void SettingsSettingsConfigurationLeftForm::checkButton(DeviceStates dev, InterfaceButton* btn){
    bool btnState = _can->getState(dev);
    if ( btnState && btn->state != InterfaceButton::On)
    {
        btn->state = InterfaceButton::On;
        btn->updateVisual();
        return;
    }

    if (!btnState && btn->state != InterfaceButton::Off)
    {
        btn->state = InterfaceButton::Off;
        btn->updateVisual();
    }
}
