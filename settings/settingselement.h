#ifndef SETTINGSELEMENT_H
#define SETTINGSELEMENT_H

#include <QWidget>

namespace Ui {
class SettingsElement;
}

class SettingsElement : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsElement(QString humanName_, QString settingsGroup_, QString settingsName_, float value_, float minVal_, float maxVal_, float step_ = 1, QWidget *parent = nullptr);
    ~SettingsElement();

    QString humanName;
    QString settingsGroup;
    QString settingsName;
    float value;
    float minVal;
    float maxVal;
    float step;

private slots:
    void on_pushButton_minus_clicked();

    void on_pushButton_plus_clicked();

private:
    Ui::SettingsElement *ui;
};

#endif // SETTINGSELEMENT_H
