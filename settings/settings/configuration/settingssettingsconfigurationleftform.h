#ifndef SETTINGSSETTINGSCONFIGURATIONLEFTFORM_H
#define SETTINGSSETTINGSCONFIGURATIONLEFTFORM_H

#include <QWidget>

#include <Controllers/cancontroller.h>
#include <can/mycan.h>
#include <interface_button/interfacebutton.h>

namespace Ui {
class SettingsSettingsConfigurationLeftForm;
}

class SettingsSettingsConfigurationLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsSettingsConfigurationLeftForm(CanController* can, QWidget *parent = nullptr);
    ~SettingsSettingsConfigurationLeftForm();

    void updateVisual();
    void checkButton(DeviceStates dev, InterfaceButton* btn);


private slots:

private:
    Ui::SettingsSettingsConfigurationLeftForm *ui;
    QWidget* _parent;
    CanController* _can;
};

#endif // SETTINGSSETTINGSCONFIGURATIONLEFTFORM_H
