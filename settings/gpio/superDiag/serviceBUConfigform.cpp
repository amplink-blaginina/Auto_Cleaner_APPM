#include "serviceBUConfigform.h"
#include "ui_serviceBUConfigform.h"

#include "mainwindow.h"

ServiceBUConfigForm::ServiceBUConfigForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceBUConfigForm)
{
    ui->setupUi(this);

    parent = parent_;

    ((MainWindow*)parent)->getSystemConfigure(&systemConfigure);
    ((MainWindow*)parent)->getElements(&systemElements);
    BUCPConfigured = ((MainWindow*)parent)->can0->isConfigured();

    for (int i = 0; i <= 8; i++)
    {// 0 плата это БУ ЦП (создаем ее просто как заглушку)
        if (i == 0)
            boards.append(NULL);
        else
        {
            boards.append(new ServiceBUConfigElementForm(i, &systemConfigure, &systemElements, &BUCPConfigured, parent, this));

            // разместим на форме нашу плату (236x384)
            boards.last()->setGeometry((((i - 1) + 4) % 4) * 236, (i / 5) * 300, 236, 300);
        }
    }

    connect(&mainProgressTimer, SIGNAL(timeout()), this, SLOT(mainProgress()));

    mainProgressTimer.start(1000);
}

ServiceBUConfigForm::~ServiceBUConfigForm()
{
    delete ui;
}

void ServiceBUConfigForm::mainProgress()
{
    if (!isVisible())
        return;

    if (((MainWindow*)parent)->can0->isConfigured())
    {
        if (!ui->frame_buCP->styleSheet().contains("70,248,114"))
        {

        }
    }
    else
    {

    }
    checkElement(ui->frame_buCP, NULL, "frame_buCP", ((MainWindow*)parent)->can0->isConfigured());

    checkElement(ui->frame_in1, ui->label_in1_value, "frame_in1", ((MainWindow*)parent)->can0->getState(DeviceStates::Board0IN1).toBool());
    checkElement(ui->frame_in2, ui->label_in2_value, "frame_in2", ((MainWindow*)parent)->can0->getState(DeviceStates::Board0IN2).toBool());
    checkElement(ui->frame_in3, ui->label_in3_value, "frame_in3", ((MainWindow*)parent)->can0->getState(DeviceStates::Board0IN3).toBool());
    checkElement(ui->frame_in4, ui->label_in4_value, "frame_in4", ((MainWindow*)parent)->can0->getState(DeviceStates::Board0IN4).toBool());

    checkElement(ui->frame_out1, ui->label_out1_value, "frame_out1", ((MainWindow*)parent)->can0->getState(DeviceStates::Board0OUT1).toBool());
    checkElement(ui->frame_out2, ui->label_out2_value, "frame_out2", ((MainWindow*)parent)->can0->getState(DeviceStates::Board0OUT2).toBool());
    checkElement(ui->frame_out3, ui->label_out3_value, "frame_out3", ((MainWindow*)parent)->can0->getState(DeviceStates::Board0OUT3).toBool());
    checkElement(ui->frame_out4, ui->label_out4_value, "frame_out4", ((MainWindow*)parent)->can0->getState(DeviceStates::Board0OUT4).toBool());
}

void ServiceBUConfigForm::checkElement(QFrame* frame, QLabel* label, QString frame_name, bool state)
{
    if (state)
    {
        if (!frame->styleSheet().contains("70,248,114"))
        {
            frame->setStyleSheet("QFrame#" + frame_name + "{background-color: rgb(70,248,114);}");
            if (label)
                label->setText("ВКЛ");
        }
    }
    else
    {
        if (!frame->styleSheet().contains("248,70,73"))
        {
            frame->setStyleSheet("QFrame#" + frame_name + "{background-color: rgb(248,70,73);}");
            if (label)
                label->setText("ВЫКЛ");
        }
    }
}

void ServiceBUConfigForm::on_pushButton_exit_clicked()
{
    close();
}
