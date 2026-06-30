#include <QDebug>

#include "servicetoelement.h"
#include "ui_servicetoelement.h"

ServiceTOElement::ServiceTOElement(QString humanName_, QString settingsName_, int value_, int valueTO_, int minVal_, int maxVal_, float step_, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServiceTOElement)
{
    ui->setupUi(this);

    humanName = humanName_;
    settingsName = settingsName_;
    value = value_;
    valueTO = valueTO_;
    minVal = minVal_;
    maxVal = maxVal_;
    step = step_;
    elementButton = ui->pushButton_element;
    elementChoosedLabel = ui->label_elementChoosed;
    elementChoosedLabel->hide();
    elementTOValueLabel = ui->label_valueTO;
    elementValueLabel = ui->label_valueCurrent;

    ui->label_name->setFont(QFont("Rotonda Bold", 11));
    ui->label_valueTO_up->setFont(QFont("Rotonda Bold", 11));
    ui->label_valueCurrent_up->setFont(QFont("Rotonda Bold", 11));
    ui->label_valueCurrent->setFont(QFont("Mont", 16));
    ui->label_valueCurrent->setText( QStringLiteral("%1").arg(value / 3600, 4, 10, QLatin1Char('0')));
    ui->label_valueTO->setFont(QFont("Mont", 16));
    ui->label_valueTO->setText( QStringLiteral("%1").arg(valueTO / 3600, 4, 10, QLatin1Char('0')));
    if (humanName == "")
        ui->label_name->setText(settingsName);
    else
        ui->label_name->setText(humanName);
}

ServiceTOElement::~ServiceTOElement()
{
    delete ui;
}
