#include "serviceglobaldatetimeleftform.h"
#include "ui_serviceglobaldatetimeleftform.h"

#include "mainwindow.h"

ServiceGlobalDateTimeLeftForm::ServiceGlobalDateTimeLeftForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceGlobalDateTimeLeftForm)
{
    ui->setupUi(this);

    parent = parent_;

    actualTime();
}

ServiceGlobalDateTimeLeftForm::~ServiceGlobalDateTimeLeftForm()
{
    delete ui;
}

void ServiceGlobalDateTimeLeftForm::actualTime()
{
    curDT = QDateTime::currentDateTime();
    changed = false;

    showTime();
}

void ServiceGlobalDateTimeLeftForm::showTime()
{
    ui->label_hour->setText(QString("%1").arg(curDT.time().hour(), 2, 10, QChar('0')));
    ui->label_minute->setText(QString("%1").arg(curDT.time().minute(), 2, 10, QChar('0')));
    ui->label_second->setText(QString("%1").arg(curDT.time().second(), 2, 10, QChar('0')));
    ui->label_day->setText(QString("%1").arg(curDT.date().day(), 2, 10, QChar('0')));
    ui->label_month->setText(QString("%1").arg(curDT.date().month(), 2, 10, QChar('0')));
    ui->label_year->setText(QString("%1").arg(curDT.date().year(), 4, 10, QChar('0')));
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_hour_more_clicked()
{
    curDT = curDT.addSecs(3600);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_hour_less_clicked()
{
    curDT = curDT.addSecs(-3600);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_minute_more_clicked()
{
    curDT = curDT.addSecs(60);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_minute_less_clicked()
{
    curDT = curDT.addSecs(-60);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_second_more_clicked()
{
    curDT = curDT.addSecs(1);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_second_less_clicked()
{
    curDT = curDT.addSecs(-1);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_year_more_clicked()
{
    curDT = curDT.addYears(1);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_year_less_clicked()
{
    curDT = curDT.addYears(-1);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_month_more_clicked()
{
    curDT = curDT.addMonths(1);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_month_less_clicked()
{
    curDT = curDT.addMonths(-1);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_day_more_clicked()
{
    curDT = curDT.addDays(1);
    showTime();
    changed = true;
}

void ServiceGlobalDateTimeLeftForm::on_pushButton_day_less_clicked()
{
    curDT = curDT.addDays(-1);
    showTime();
    changed = true;
}

