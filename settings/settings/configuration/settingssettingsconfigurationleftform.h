#ifndef SETTINGSSETTINGSCONFIGURATIONLEFTFORM_H
#define SETTINGSSETTINGSCONFIGURATIONLEFTFORM_H

#include <QWidget>

#include <can/mycan.h>
#include <interface_button/interfacebutton.h>

namespace Ui {
class SettingsSettingsConfigurationLeftForm;
}

class SettingsSettingsConfigurationLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsSettingsConfigurationLeftForm(QWidget *parent = nullptr);
    ~SettingsSettingsConfigurationLeftForm();

    void updateVisual();
    void checkButton(DeviceStates dev, InterfaceButton* btn);

    QWidget* parent;

private slots:

private:
    Ui::SettingsSettingsConfigurationLeftForm *ui;
};

#endif // SETTINGSSETTINGSCONFIGURATIONLEFTFORM_H
