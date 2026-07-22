#include "settingssettingsconfigurationleftform.h"
#include "ui_settingssettingsconfigurationleftform.h"

#include "mainwindow.h"

SettingsSettingsConfigurationLeftForm::SettingsSettingsConfigurationLeftForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::SettingsSettingsConfigurationLeftForm)
{
    ui->setupUi(this);

    parent = parent_;

}

SettingsSettingsConfigurationLeftForm::~SettingsSettingsConfigurationLeftForm()
{
    delete ui;
}

void SettingsSettingsConfigurationLeftForm::updateVisual()
{
}

void SettingsSettingsConfigurationLeftForm::checkButton(DeviceStates dev, InterfaceButton* btn)
{
    bool btnState = ((MainWindow*)parent)->can0->getState(dev).toBool();
    if ( btnState && btn->state != InterfaceButton::On)
    {
        btn->state = InterfaceButton::On;
        btn->updateVisual();
    }
    if (!btnState && btn->state != InterfaceButton::Off)
    {
        btn->state = InterfaceButton::Off;
        btn->updateVisual();
    }
}
