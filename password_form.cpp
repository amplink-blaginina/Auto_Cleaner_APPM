#include "password_form.h"
#include "ui_password_form.h"
#include <QMessageBox>
#include <QDebug>

#include "mainwindow.h"
#include "fogotform.h"

int passwrd;
int secret_passwrd = -1;
int form_number;

Password_Form::Password_Form(QWidget *parent_, bool showFogotPassword_, bool backKind_) :
    QWidget(NULL),
    ui(new Ui::Password_Form)
{
    parent = parent_;
    showFogotPassword = showFogotPassword_;

    ui->setupUi(this);
    stringpasswrd = "";

    labels.append(ui->label_1);
    labels.append(ui->label_2);
    labels.append(ui->label_3);
    labels.append(ui->label_4);
    labels.append(ui->label_5);

    QFont font;
    font.setFamily("Futura Md BT [Rus by me]");
    font.setPointSize(25);
    ui->label_text->setFont(font);
    ui->label_text->setStyleSheet("color: white");
    title = "Введите пароль";
    ui->label_text->setText(title);
    labelText = ui->label_text;
    connect(&wrongPasswordTimer, SIGNAL(timeout()), this, SLOT(wrongPasswordFlash()));

    if (backKind_)
        ui->frame->setStyleSheet("QFrame#frame {background-image: url(:/Images/Images/password/password_background_settings.png);}");
    else
        ui->frame->setStyleSheet("QFrame#frame {background-image: url(:/Images/Images/password/password_background_service.png);}");


    ui->pushButton_0->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_1->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_2->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_3->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_4->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_5->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_6->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_7->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_8->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_9->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_clean->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_ok->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_back->setStyleSheet("border-style:none;outline: none;");
    ui->pushButton_forgot->setStyleSheet("border-style:none;outline: none;");
}

Password_Form::~Password_Form()
{
    disconnect(&wrongPasswordTimer, SIGNAL(timeout()), this, SLOT(wrongPasswordFlash()));
    delete ui;
}

void Password_Form::on_pushButton_0_clicked()
{
    if (ui->pushButton_0->styleSheet() != "border-style:none;outline: none;")
    {
        stopWrongPassword();

        if (stringpasswrd.length() < 5)
        {
            labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
            stringpasswrd+="0";
        }
        ui->pushButton_0->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_1_clicked()
{
    if (ui->pushButton_1->styleSheet() != "border-style:none;outline: none;")
    {
        stopWrongPassword();

        if (stringpasswrd.length() < 5)
        {
            labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
            stringpasswrd+="1";
        }
        ui->pushButton_1->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_2_clicked()
{
    if (ui->pushButton_2->styleSheet() != "border-style:none;outline: none;")
    {
        stopWrongPassword();

        if (stringpasswrd.length() < 5)
        {
            labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
            stringpasswrd+="2";
        }
        ui->pushButton_2->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_3_clicked()
{
    if (ui->pushButton_3->styleSheet() != "border-style:none;outline: none;")
    {
        stopWrongPassword();

        if (stringpasswrd.length() < 5)
        {
            labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
            stringpasswrd+="3";
        }
        ui->pushButton_3->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_4_clicked()
{
    if (ui->pushButton_4->styleSheet() != "border-style:none;outline: none;")
    {
        stopWrongPassword();

        if (stringpasswrd.length() < 5)
        {
            labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
            stringpasswrd+="4";
        }
        ui->pushButton_4->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_5_clicked()
{
    if (ui->pushButton_5->styleSheet() != "border-style:none;outline: none;")
    {
        stopWrongPassword();

        if (stringpasswrd.length() < 5)
        {
            labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
            stringpasswrd+="5";
        }
        ui->pushButton_5->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_6_clicked()
{
    if (ui->pushButton_6->styleSheet() != "border-style:none;outline: none;")
    {
        stopWrongPassword();

        if (stringpasswrd.length() < 5)
        {
            labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
            stringpasswrd+="6";
        }
        ui->pushButton_6->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_7_clicked()
{
    if (ui->pushButton_7->styleSheet() != "border-style:none;outline: none;")
    {
        stopWrongPassword();

        if (stringpasswrd.length() < 5)
        {
            labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
            stringpasswrd+="7";
        }
        ui->pushButton_7->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_8_clicked()
{
    if (ui->pushButton_8->styleSheet() != "border-style:none;outline: none;")
    {
        stopWrongPassword();

        if (stringpasswrd.length() < 5)
        {
            labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
            stringpasswrd+="8";
        }
        ui->pushButton_8->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_9_clicked()
{
    if (ui->pushButton_9->styleSheet() != "border-style:none;outline: none;")
    {
        qDebug() << "clicked";
        stopWrongPassword();

        if (stringpasswrd.length() < 5)
        {
            labels.at(stringpasswrd.length())->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
            stringpasswrd+="9";
        }
        ui->pushButton_9->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_clean_clicked()
{
    if (ui->pushButton_clean->styleSheet() != "border-style:none;outline: none;")
    {
        stopWrongPassword();

        ui->label_1->setPixmap(QPixmap(":/Images/Images/password/password_point_off.png"));
        ui->label_2->setPixmap(QPixmap(":/Images/Images/password/password_point_off.png"));
        ui->label_3->setPixmap(QPixmap(":/Images/Images/password/password_point_off.png"));
        ui->label_4->setPixmap(QPixmap(":/Images/Images/password/password_point_off.png"));
        ui->label_5->setPixmap(QPixmap(":/Images/Images/password/password_point_off.png"));

        stringpasswrd.clear();

        ui->pushButton_clean->setStyleSheet("border-style:none;outline: none;");
    }
}

void Password_Form::on_pushButton_back_clicked()
{
    this->close();
}

void Password_Form::on_pushButton_ok_clicked()
{
    if (stringpasswrd.toInt()==passwrd || stringpasswrd.toInt() == secret_passwrd || passwrd == 0)
    {
        emit Result_pass(true);
        emit Send_correct(stringpasswrd.toInt());
        this->close();
    }
    else
    {
        //ui->label_text->setStyleSheet("color: red;");
        ui->label_text->setText("Неправильный пароль");
        for (int i = 0; i < 5; i++)
        {
            //labels.at(i)->setPixmap(QPixmap(":/Images/Images/password_input_on.png"));
            labels.at(i)->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
        }
        wrongPasswordTime = QDateTime::currentDateTime();
        wrongPasswordTimer.start(300);
//        QMessageBox msgBox;
//        msgBox.setText("Wrong Password.");
//        msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
//        msgBox.exec();
    }
}

void Password_Form::wrongPasswordFlash()
{
    for (int i = 0; i < 5; i++)
    {
        if ((wrongPasswordTime.msecsTo(QDateTime::currentDateTime()) / 300) % 2)
            labels.at(i)->setPixmap(QPixmap(":/Images/Images/password/password_point_off.png"));
        else
            labels.at(i)->setPixmap(QPixmap(":/Images/Images/password/password_point_on.png"));
    }
    if (wrongPasswordTime.secsTo(QDateTime::currentDateTime()) > 3)
        stopWrongPassword();
}

void Password_Form::stopWrongPassword()
{
    if (wrongPasswordTimer.isActive())
    {
        wrongPasswordTimer.stop();
        //ui->label_text->setStyleSheet("color: white;");
        ui->label_text->setText(title);
        for (int i = 0; i < 5; i++)
        {
            //labels.at(i)->setPixmap(QPixmap(":/Images/Images/password_input_on.png"));
            labels.at(i)->setPixmap(QPixmap(":/Images/Images/password/password_point_off.png"));
        }
        stringpasswrd.clear();
    }
}

void Password_Form::setTitle(QString title_)
{
    title = title_;
    ui->label_text->setText(title);
}

void Password_Form::Recieve_pass_name(int value)
{
    passwrd = value;
}

void Password_Form::Recieve_secret_pass_name(int value)
{
    secret_passwrd = value;
}

void Password_Form::on_pushButton_0_pressed()
{
    ui->pushButton_0->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_0_on.png);");
}

void Password_Form::on_pushButton_1_pressed()
{
    ui->pushButton_1->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_1_on.png);");
}

void Password_Form::on_pushButton_2_pressed()
{
    ui->pushButton_2->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_2_on.png);");
}

void Password_Form::on_pushButton_3_pressed()
{
    ui->pushButton_3->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_3_on.png);");
}

void Password_Form::on_pushButton_4_pressed()
{
    ui->pushButton_4->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_4_on.png);");
}

void Password_Form::on_pushButton_5_pressed()
{
    ui->pushButton_5->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_5_on.png);");
}

void Password_Form::on_pushButton_6_pressed()
{
    ui->pushButton_6->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_6_on.png);");
}

void Password_Form::on_pushButton_7_pressed()
{
    ui->pushButton_7->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_7_on.png);");
}

void Password_Form::on_pushButton_8_pressed()
{
    ui->pushButton_8->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_8_on.png);");
}

void Password_Form::on_pushButton_9_pressed()
{
    qDebug() << "pressed";
    ui->pushButton_9->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_9_on.png);");
}

void Password_Form::on_pushButton_clean_pressed()
{
    ui->pushButton_clean->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/password/password_button_cancel_on.png);");
}

void Password_Form::on_pushButton_forgot_clicked()
{
    ui->label_text->setText("Сообщите код службе поддержки " + QString::number(secret_passwrd ^ 0x000FFFFF));
//    FogotForm *fogotForm = new FogotForm(secret_passwrd ^ 0x000FFFFF, this);
//    fogotForm->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
//    fogotForm->setAttribute(Qt::WA_DeleteOnClose,true);
//    fogotForm->setGeometry(50,50,904,656);
//    fogotForm->show();
}

