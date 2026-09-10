#include "servicegeneralpasswordleftform.h"
#include "ui_servicegeneralpasswordleftform.h"

#include "mainwindow.h"

ServiceGeneralPasswordLeftForm::ServiceGeneralPasswordLeftForm(MainWindow* mainWindow, QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceGeneralPasswordLeftForm)
{
    ui->setupUi(this);
    _mainWindow = mainWindow;
    parent = parent_;

    wrongPassword = false;
    stringpasswrd = "";

    goStep(0);
    ui->label_text_result->hide();
    connect(&wrongPasswordTimer, SIGNAL(timeout()), this, SLOT(wrongPasswordFlash()));
}

ServiceGeneralPasswordLeftForm::~ServiceGeneralPasswordLeftForm()
{
    delete ui;
}

void ServiceGeneralPasswordLeftForm::goStep(quint8 step)
{
    if (step == 0)
    {
        passwrd = _mainWindow->settings->value("Global/" + passwordVariable).toInt();
        labels.clear();
        labels.append(ui->label_old_1);
        labels.append(ui->label_old_2);
        labels.append(ui->label_old_3);
        labels.append(ui->label_old_4);
        labels.append(ui->label_old_5);
        ui->label_text_new->hide();
        ui->label_new_1->hide();
        ui->label_new_2->hide();
        ui->label_new_3->hide();
        ui->label_new_4->hide();
        ui->label_new_5->hide();
        ui->label_text_repeat->hide();
        ui->label_repeat_1->hide();
        ui->label_repeat_2->hide();
        ui->label_repeat_3->hide();
        ui->label_repeat_4->hide();
        ui->label_repeat_5->hide();
    }
    if (step == 1)
    {
        labels.clear();
        labels.append(ui->label_new_1);
        labels.append(ui->label_new_2);
        labels.append(ui->label_new_3);
        labels.append(ui->label_new_4);
        labels.append(ui->label_new_5);
        ui->label_text_new->show();
        ui->label_new_1->show();
        ui->label_new_2->show();
        ui->label_new_3->show();
        ui->label_new_4->show();
        ui->label_new_5->show();
        ui->label_text_result->hide();
    }
    if (step == 2)
    {
        labels.clear();
        labels.append(ui->label_repeat_1);
        labels.append(ui->label_repeat_2);
        labels.append(ui->label_repeat_3);
        labels.append(ui->label_repeat_4);
        labels.append(ui->label_repeat_5);
        ui->label_text_repeat->show();
        ui->label_repeat_1->show();
        ui->label_repeat_2->show();
        ui->label_repeat_3->show();
        ui->label_repeat_4->show();
        ui->label_repeat_5->show();
    }
    if (step == 3)
    {
        ui->label_text_result->show();
    }
    currentStep = step;
}

void ServiceGeneralPasswordLeftForm::on_pushButton_1_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() < 5)
    {
        labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        stringpasswrd += "1";
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_2_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() < 5)
    {
        labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        stringpasswrd += "2";
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_3_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() < 5)
    {
        labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        stringpasswrd += "3";
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_4_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() < 5)
    {
        labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        stringpasswrd += "4";
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_5_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() < 5)
    {
        labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        stringpasswrd += "5";
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_6_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() < 5)
    {
        labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        stringpasswrd += "6";
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_7_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() < 5)
    {
        labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        stringpasswrd += "7";
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_8_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() < 5)
    {
        labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        stringpasswrd += "8";
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_9_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() < 5)
    {
        labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        stringpasswrd += "9";
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_0_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() < 5)
    {
        labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        stringpasswrd += "0";
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_backspace_clicked()
{
    stopWrongPassword();
    if (stringpasswrd.length() > 0)
    {
        labels.at(stringpasswrd.length() - 1)->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_off.png"));
        stringpasswrd = stringpasswrd.left(stringpasswrd.length() - 1);
    }
}

void ServiceGeneralPasswordLeftForm::on_pushButton_save_clicked()
{
    if (stringpasswrd.toInt() == passwrd || passwrd == 0)
    {
        if (currentStep == 0)
        {
            goStep(1);
            passwrd = 0;
            stopWrongPassword();
        }
        else if (currentStep == 1)
        {
            passwrd = stringpasswrd.toInt();
            goStep(2);
            stopWrongPassword();
        }
        else
        {
            goStep(3);
            goStep(0);
            if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
                QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");

            _mainWindow->settings->beginGroup("Global");
            _mainWindow->settings->setValue(passwordVariable, passwrd);
            _mainWindow->settings->endGroup();
            _mainWindow->settings->sync();
        }
        for (int i = 0; i < 5; i++)
        {
            labels.at(i)->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_off.png"));
        }
        stringpasswrd = "";
    }
    else
    {
        wrongPassword = true;
        for (int i = 0; i < 5; i++)
        {
            labels.at(i)->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
        }
        wrongPasswordTime = QDateTime::currentDateTime();
        wrongPasswordTimer.start(300);
    }
}

void ServiceGeneralPasswordLeftForm::wrongPasswordFlash()
{
    for (int i = 0; i < 5; i++)
    {
        if ((wrongPasswordTime.msecsTo(QDateTime::currentDateTime()) / 300) % 2)
            labels.at(i)->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_off.png"));
        else
            labels.at(i)->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_on.png"));
    }
    if (wrongPasswordTime.secsTo(QDateTime::currentDateTime()) > 3)
        stopWrongPassword();
}

void ServiceGeneralPasswordLeftForm::stopWrongPassword()
{
    if (wrongPasswordTimer.isActive())
    {
        wrongPassword = false;
        wrongPasswordTimer.stop();
        for (int i = 0; i < 5; i++)
        {
            labels.at(i)->setPixmap(QPixmap(":/Images/Images/service/global/password/buttons/service_generalSettings_password_point_off.png"));
        }
        stringpasswrd.clear();
    }
}
void ServiceGeneralPasswordLeftForm::resetPassword(){
    passwordVariable = "password";
    goStep(0);
}
