#include <QDebug>

#include "settingselement.h"
#include "ui_settingselement.h"

SettingsElement::SettingsElement(QString humanName_, QString settingsGroup_, QString settingsName_, float value_, float minVal_, float maxVal_, float step_, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsElement)
{
    ui->setupUi(this);

    humanName = humanName_;
    settingsGroup = settingsGroup_;
    settingsName = settingsName_;
    value = value_;
    minVal = minVal_;
    maxVal = maxVal_;
    step = step_;

    ui->label_name->setFont(QFont("Rotonda Bold", 11));
    ui->label_value->setFont(QFont("Mont", 16));

    if (step != (int)step)
        ui->label_value->setText( QStringLiteral("%1").arg(value, 4, 'f', 2, QLatin1Char('0')));
    else
        ui->label_value->setText( QStringLiteral("%1").arg((int)value, 4, 10, QLatin1Char('0')));

    if (humanName == "")
        ui->label_name->setText(settingsName);
    else
        ui->label_name->setText(humanName);
}

SettingsElement::~SettingsElement()
{
    delete ui;
}

void SettingsElement::on_pushButton_minus_clicked()
{
    value -= step;
    if (value < minVal)
        value = minVal;
    if (step != (int)step)
        ui->label_value->setText( QStringLiteral("%1").arg(value, 4, 'f', 2, QLatin1Char('0')));
    else
        ui->label_value->setText( QStringLiteral("%1").arg((int)value, 4, 10, QLatin1Char('0')));
    //ui->label_value->setText(QString::number(value));
}

void SettingsElement::on_pushButton_plus_clicked()
{
    value += step;
    if (value > maxVal)
        value = maxVal;
    if (step != (int)step)
        ui->label_value->setText( QStringLiteral("%1").arg(value, 4, 'f', 2, QLatin1Char('0')));
    else
        ui->label_value->setText( QStringLiteral("%1").arg((int)value, 4, 10, QLatin1Char('0')));
    //ui->label_value->setText(QString::number(value));
}
