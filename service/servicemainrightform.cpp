#include "servicemainrightform.h"
#include "ui_servicemainrightform.h"

//#include "service/gpio/superDiag/serviceBUConfigform.h"
#include "password_form.h"

#include "mainwindow.h"

ServiceMainRightForm::ServiceMainRightForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceMainRightForm)
{
    ui->setupUi(this);

    parent = parent_;

    currentLevel = 0;
    currentElement = -1;
    oldLevel = 0;
    oldCurrentElement = -1;
    oldCurrentLevel = 0;

    buttons[0] = ui->pushButton_1;
    buttons[1] = ui->pushButton_2;
    buttons[2] = ui->pushButton_3;
    buttons[3] = ui->pushButton_4;

    addMenu("Глобальные настройки", 0, 0, 1, NULL, ":/Images/Images/service/buttons/service_button_generalSettings");
    addMenu("Дата, время", 1, 0, 1, ((MainWindow*)parent)->serviceGlobalDateTimeLeftForm, ":/Images/Images/service/global/buttons/service_generalSettings_button_time");
    addMenu("Пароль", 1, 1, 1, ((MainWindow*)parent)->serviceGeneralPasswordLeftForm, ":/Images/Images/service/global/buttons/service_generalSettings_button_password");
    addMenu("PDF", 1, 2, 1, NULL, ":/Images/Images/service/global/buttons/service_generalSettings_button_PDF");
    addMenu("", 1, 3, 0, NULL, "");

    addMenu("Устройства", 0, 1, 2, NULL, ":/Images/Images/service/buttons/service_button_engine");
    addMenu("Гидравлика", 2, 0, 2, ((MainWindow*)parent)->serviceDevicesHydraulicsLeftForm, ":/Images/Images/service/devices/buttons/service_valve_button_hydraulics");
    addMenu("ДКП", 2, 1, 2, ((MainWindow*)parent)->serviceDevicesDKPLeftForm, ":/Images/Images/service/devices/buttons/service_valve_button_DKP");
    addMenu("", 2, 2, 2, NULL, "");
    addMenu("", 2, 3, 0, NULL, "");

    addMenu("Прочее", 0, 2, 3, NULL, ":/Images/Images/service/buttons/service_button_valve");
    addMenu("ДВС", 3, 0, 3, ((MainWindow*)parent)->serviceOtherEngineLeftForm, ":/Images/Images/service/other/buttons/service_engine_button_engine");
    addMenu("ТО", 3, 1, 3, ((MainWindow*)parent)->serviceGPIOServiceIntervalLeftForm, ":/Images/Images/service/other/buttons/service_engine_button_TO");
    addMenu("Освещение", 3, 2, 3, ((MainWindow*)parent)->serviceOtherLightLeftForm, ":/Images/Images/service/other/buttons/service_engine_button_light");
    addMenu("", 3, 3, 0, NULL, "");

    addMenu("", 0, 3, 0, NULL, ":/Images/Images/settings/buttons/settings_reserve");
    addMenu("", 4, 0, 4, NULL, ":/Images/Images/settings/buttons/settings_reserve");
    addMenu("", 4, 1, 4, NULL, ":/Images/Images/settings/buttons/settings_reserve");
    addMenu("", 4, 2, 4, NULL, ":/Images/Images/settings/buttons/settings_reserve");
    addMenu("", 4, 3, 0, NULL, "");

    // добавим попытку выхода с сохранением и без
    addMenu("Выход", 0, 4, 5, NULL, ":/Images/Images/settings/buttons/settings_reserve");
    addMenu("Да", 5, 0, 5, NULL, ":/Images/Images/settings/buttons/settings_button_save");
    addMenu("Нет", 5, 1, 5, NULL, ":/Images/Images/settings/buttons/settings_button_notSave");
    addMenu("", 5, 2, 5, NULL, "");
    addMenu("", 5, 3, 0, NULL, "");

    showService();
}

ServiceMainRightForm::~ServiceMainRightForm()
{
    delete ui;
}

void ServiceMainRightForm::addMenu(QString name_, quint8 id_, quint8 id1_, quint8 goLevel_, QWidget* form_, QString png_)
{
    menu[id_][id1_].goLevel = goLevel_;
    menu[id_][id1_].form = form_;
    menu[id_][id1_].png = png_;
    menu[id_][id1_].name = name_;
}

void ServiceMainRightForm::showService()
{// отображаем правое меню и левую форму в соответствии с данными меню
    for (int i = 0; i < 4; i++)
    {
        if (menu[currentLevel][i].png != "")
        {
            buttons[i]->show();

            if (i == currentElement && menu[currentLevel][i].form)
            {
                buttons[i]->setStyleSheet("border-style:none;outline: none;background-image:url(" + menu[currentLevel][i].png + "_on.png);");
                menu[currentLevel][i].form->show();
                menu[currentLevel][i].form->raise();
            }
            else
            {
                if (menu[currentLevel][i].form)
                    menu[currentLevel][i].form->hide();
                buttons[i]->setStyleSheet("border-style:none;outline: none;background-image:url(" + menu[currentLevel][i].png + "_off.png);");
            }
        }
        else
            buttons[i]->hide();

    }
    if (currentLevel == 0)
    {// нарисуем выход ниже всех
        ui->pushButton_exit->setStyleSheet("border-style:none;outline: none;background-image:url(:/Images/Images/service/buttons/service_button_exit_off.png);");
    }
    else
    {
        ui->pushButton_exit->setStyleSheet("border-style:none;outline: none;background-image:url(:/Images/Images/settings/buttons/settings_button_cancel_off.png);");
    }

    // подпись заголовка
//    ui->label_name->raise();
//    if (currentElement == 1 && currentLevel == 3)
//        ui->label_name->setText(menu[currentLevel][currentElement].name);
//    else
//        ui->label_name->setText("");

    if (currentElement == 1 && currentLevel == 3 && oldCurrentElement != currentElement && oldCurrentLevel != currentLevel)
    {// техосмотр
        Password_Form *Password_window = new Password_Form (parent, true);
        Password_window->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
        Password_window->setAttribute(Qt::WA_DeleteOnClose,true);

        connect(this,SIGNAL(Pass_close()),Password_window,SLOT(close()));
        connect(this,SIGNAL(Send_Pass_2_pass_form(int)),Password_window,SLOT(Recieve_pass_name(int)));
        connect(this,SIGNAL(Send_SecretPass_2_pass_form(int)),Password_window,SLOT(Recieve_secret_pass_name(int)));
        connect(Password_window,SIGNAL(Send_correct(int)),this,SLOT(passwordDiagOk(int)));

        ((MainWindow*)parent)->settings->beginGroup("Global");
        emit Send_Pass_2_pass_form(((MainWindow*)parent)->settings->value("passwordDiag").toInt());
        emit Send_SecretPass_2_pass_form(((MainWindow*)parent)->settings->value("secretPasswordDiag").toInt());
        ((MainWindow*)parent)->settings->endGroup();
        Password_window->show();        
        menu[currentLevel][currentElement].form->hide();
        ((MainWindow*)parent)->serviceGPIOServiceIntervalLeftForm->fillElements();
    }
    if (currentElement == 0 && currentLevel == 2)
    {// гидравлика инициализация
        ((MainWindow*)parent)->serviceDevicesHydraulicsLeftForm->refreshSliders();
    }
    if (currentElement == 1 && currentLevel == 1)
    {// сброс пароля
        ((MainWindow*)parent)->serviceGeneralPasswordLeftForm->passwordVariable = "passwordDiag";
        ((MainWindow*)parent)->serviceGeneralPasswordLeftForm->goStep(0);
    }
    if (currentElement == 0 && currentLevel == 1 && oldCurrentElement != currentElement && oldCurrentLevel != currentLevel)
    {// выставить текущую дату
        ((MainWindow*)parent)->serviceGlobalDateTimeLeftForm->actualTime();
    }
    //ищем специальные пункты которые не так просты как мы думаем
    if (currentElement == 2 && currentLevel == 1)
    {//pdf
        currentElement = -1;
        pdfWidget = new QWidget();
        pdfWidget->setGeometry(0,0,1024,600);
        pdfWidget->show();
        pdfScroller = new PdfScroller(pdfWidget);
        pdfScroller->chooseFile();
    }
}

void ServiceMainRightForm::passwordDiagOk(int pass)
{
    if (pass == ((MainWindow*)parent)->readSettingsValue("Global/secretPasswordDiag").toString().toInt())
    {// сбросим одноразовы пароль
        if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
            QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");
        int random = std::rand() % ((9999 + 1) - 1) + 1;
        //qDebug() << random;
        ((MainWindow*)parent)->settings->setValue("Global/secretPasswordDiag", random);// рандом от 1 до 9999
        ((MainWindow*)parent)->settings->sync();
        system("sync");
        // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
        ((MainWindow*)parent)->removeBadSettings();
    }
    menu[currentLevel][currentElement].form->show();
}

void ServiceMainRightForm::moveMenu(qint8 level)
{// организуем переход по меню
    if (level == 0 && currentLevel == 5)
    {// ДА
        currentLevel = oldCurrentLevel;
        currentElement = oldCurrentElement;
        menu[currentLevel][currentElement].form->hide();
        level = oldLevel;
        if (currentElement == 1 && currentLevel == 3)
        {// ТО
            ((MainWindow*)parent)->serviceGPIOServiceIntervalLeftForm->saveIntervals();
            ((MainWindow*)parent)->readSettings();
            menu[currentLevel][currentElement].form->hide();
        }
        if (currentElement == 0 && currentLevel == 1)
        {// дату редактировали
            ((MainWindow*)parent)->serviceGlobalDateTimeLeftForm->changed = false;
            QString dataa = "date -s @\"" + QString::number(((MainWindow*)parent)->serviceGlobalDateTimeLeftForm->curDT.toTime_t()) + "\"";
//            QString dataa = "date -s "
//                    +QString::number(((MainWindow*)parent)->serviceGlobalDateTimeLeftForm->curDT.date().year(),10)
//                    +"."
//                    +QString::number(((MainWindow*)parent)->serviceGlobalDateTimeLeftForm->curDT.date().month(),10)
//                    +"."
//                    +QString::number(((MainWindow*)parent)->serviceGlobalDateTimeLeftForm->curDT.date().day(),10)
//                    +"-"
//                    +QString::number(((MainWindow*)parent)->serviceGlobalDateTimeLeftForm->curDT.time().hour(),10)
//                    +":"
//                    +QString::number(((MainWindow*)parent)->serviceGlobalDateTimeLeftForm->curDT.time().minute(),10)
//                    +":"
//                    +QString::number(0,10);
            system(dataa.toUtf8().constData());
            system("hwclock -w");
            system("sync");
        }
        oldCurrentElement = -1;
        oldCurrentLevel = -1;
    }
    if (level == 1 && currentLevel == 5)
    {// НЕТ
        currentLevel = oldCurrentLevel;
        currentElement = oldCurrentElement;
        if (currentElement == 1 && currentLevel == 3)
            menu[currentLevel][currentElement].form->hide();
        if (currentElement == 0 && currentLevel == 1)
            ((MainWindow*)parent)->serviceGlobalDateTimeLeftForm->changed = false;// дату редактировали
        level = oldLevel;
        oldCurrentElement = -1;
        oldCurrentLevel = -1;
    }
    if (level == 3 && currentLevel == 5)
    {// ОТМЕНА сохранения
        currentLevel = oldCurrentLevel;
        currentElement = oldCurrentElement;
        showService();
        return;
    }

    // в некоторых случаях при переходе из меню в меню надо попасть на вопрос о сохранении или не сохранении. попробуем сделать это перенаправляясь на скрытое меню
    if (currentElement == 0 && currentLevel == 1 && ((MainWindow*)parent)->serviceGlobalDateTimeLeftForm->changed)
    {// дата время сменилась
        oldCurrentLevel = currentLevel;
        oldCurrentElement = currentElement;
        oldLevel = level;
        currentLevel = 5;
        currentElement = -1;
        return;
    }
    if (currentElement == 1 && currentLevel == 3 && menu[currentLevel][currentElement].form->isVisible())
    {// вышли из ТО
        oldCurrentLevel = currentLevel;
        oldCurrentElement = currentElement;
        oldLevel = level;
        currentLevel = 5;
        currentElement = -1;
        return;
    }

    if (menu[currentLevel][level].goLevel == currentLevel)
        currentElement = level;
    else
    {
        if (currentElement != -1 && menu[currentLevel][currentElement].form)
            menu[currentLevel][currentElement].form->hide();
        currentElement = -1;
    }
    currentLevel = menu[currentLevel][level].goLevel;
}

void ServiceMainRightForm::on_pushButton_1_clicked()
{
    moveMenu(0);
    showService();
}

void ServiceMainRightForm::on_pushButton_2_clicked()
{
    moveMenu(1);
    showService();
}

void ServiceMainRightForm::on_pushButton_3_clicked()
{
    moveMenu(2);
    showService();
}

void ServiceMainRightForm::on_pushButton_4_clicked()
{
    moveMenu(3);
    showService();
}

void ServiceMainRightForm::on_pushButton_exit_clicked()
{
    if (currentLevel == 0)
    {
        ((MainWindow*)parent)->serviceSetingsName->hide();
        hide();
        ((MainWindow*)parent)->menuMode = MainWindow::SweepMode;
        ((MainWindow*)parent)->superDiagMode = false;
        ((MainWindow*)parent)->ignitionOffTimer = 0;
        //((MainWindow*)parent)->Password_accepted_settings = false;
    }
    else
    {
        // выход невидимый
        moveMenu(3);
        showService();
    }
}


