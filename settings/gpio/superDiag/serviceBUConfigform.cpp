#include "serviceBUConfigform.h"
#include "ui_serviceBUConfigform.h"

#include "mainwindow.h"

ServiceBUConfigForm::ServiceBUConfigForm(MyCan* can, MainWindow* mainWindow, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServiceBUConfigForm)
{
    ui->setupUi(this);
    _can = can;
    _mainWindow = mainWindow;
    _parent = parent;

    _mainWindow->getSystemConfigure(&systemConfigure);
    _mainWindow->getElements(&systemElements);
    BUCPConfigured = _can->isConfigured();

    for (int i = 0; i <= 8; i++)
    {// 0 плата это БУ ЦП (создаем ее просто как заглушку)
        if (i == 0)
            boards.append(NULL);
        else
        {
            boards.append(new ServiceBUConfigElementForm(i, &systemConfigure, &systemElements, &BUCPConfigured, _can, _parent, this));

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

    if (_can->isConfigured())
    {
        if (!ui->frame_buCP->styleSheet().contains("70,248,114"))
        {

        }
    }
    else
    {

    }
    checkElement(ui->frame_buCP, NULL, "frame_buCP", _can->isConfigured());

    checkElement(ui->frame_in1, ui->label_in1_value, "frame_in1", _can->getState(DeviceStates::Board0IN1).toBool());
    checkElement(ui->frame_in2, ui->label_in2_value, "frame_in2", _can->getState(DeviceStates::Board0IN2).toBool());
    checkElement(ui->frame_in3, ui->label_in3_value, "frame_in3", _can->getState(DeviceStates::Board0IN3).toBool());
    checkElement(ui->frame_in4, ui->label_in4_value, "frame_in4", _can->getState(DeviceStates::Board0IN4).toBool());

    checkElement(ui->frame_out1, ui->label_out1_value, "frame_out1", _can->getState(DeviceStates::Board0OUT1).toBool());
    checkElement(ui->frame_out2, ui->label_out2_value, "frame_out2", _can->getState(DeviceStates::Board0OUT2).toBool());
    checkElement(ui->frame_out3, ui->label_out3_value, "frame_out3", _can->getState(DeviceStates::Board0OUT3).toBool());
    checkElement(ui->frame_out4, ui->label_out4_value, "frame_out4", _can->getState(DeviceStates::Board0OUT4).toBool());
}

void ServiceBUConfigForm::checkElement(QFrame* frame, QLabel* label, QString frame_name, bool state)
{
    auto style = "QFrame#" + frame_name + (state? "{background-color: rgb(70,248,114);}":"{background-color: rgb(248,70,73);}");
    _mainWindow->getView()->setStyle(frame, style);
    if (label)
        label->setText(state?"ВКЛ":"ВЫКЛ");


    // if (state)
    // {
    //     if (!frame->styleSheet().contains("70,248,114"))
    //     {
    //         frame->setStyleSheet("QFrame#" + frame_name + "{background-color: rgb(70,248,114);}");
    //         if (label)
    //             label->setText("ВКЛ");
    //     }
    // }
    // else
    // {
    //     if (!frame->styleSheet().contains("248,70,73"))
    //     {
    //         frame->setStyleSheet("QFrame#" + frame_name + "{background-color: rgb(248,70,73);}");
    //         if (label)
    //             label->setText("ВЫКЛ");
    //     }
    // }
}

void ServiceBUConfigForm::on_pushButton_exit_clicked()
{
    close();
}
