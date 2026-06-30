#include "settingsmainrightform.h"
#include "ui_settingsmainrightform.h"

#include "settings/gpio/superDiag/serviceBUConfigform.h"

#include "mainwindow.h"

SettingsMainRightForm::SettingsMainRightForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::SettingsMainRightForm)
{
    ui->setupUi(this);

    parent = parent_;

    currentLevel = 0;
    currentElement = -1;

    buttons[0] = ui->pushButton_1;
    buttons[1] = ui->pushButton_2;
    buttons[2] = ui->pushButton_3;
    buttons[3] = ui->pushButton_4;

    //((MainWindow*)parent)->settingsGlobalGlobalLeftForm
    addMenu("Общие настройки", 0, 0, 1, NULL, ":/Images/Images/settings/buttons/settings_button_gpio");
    addMenu("GPIO ПУ", 1, 0, 1, NULL, ":/Images/Images/settings/gpio/buttons/settings_gpio_button_gpioPy");
    addMenu("GPIO БУЦ", 1, 1, 1, NULL, ":/Images/Images/settings/gpio/buttons/settings_gpio_button_gpioBym");
    addMenu("WIFI", 1, 2, 1, ((MainWindow*)parent)->settingsWifiLeftForm, ":/Images/Images/settings/gpio/buttons/settings_gpio_button_update");
    addMenu("", 1, 3, 0, NULL, "");

    addMenu("Режимы", 0, 1, 2, NULL, ":/Images/Images/settings/buttons/settings_button_mode");
    addMenu("Легкий + листья", 2, 0, 2, ((MainWindow*)parent)->settingsForm, ":/Images/Images/settings/modes/buttons/settings_mode_button_easyAndLeafHarvesting");
    addMenu("Средний", 2, 1, 2, ((MainWindow*)parent)->settingsForm, ":/Images/Images/settings/modes/buttons/settings_mode_button_average");
    addMenu("Тяжелый", 2, 2, 2, ((MainWindow*)parent)->settingsForm, ":/Images/Images/settings/modes/buttons/settings_mode_button_hard");
    addMenu("", 2, 3, 0, NULL, "");

    addMenu("Тайминги", 0, 2, 3, NULL, ":/Images/Images/settings/buttons/settings_button_timing");
    addMenu("Перед", 3, 0, 3, ((MainWindow*)parent)->settingsForm, ":/Images/Images/settings/timings/buttons/settings_timing_button_front");
    addMenu("Средний", 3, 1, 3, ((MainWindow*)parent)->settingsForm, ":/Images/Images/settings/timings/buttons/settings_timing_button_middle");
    addMenu("Задний", 3, 2, 3, ((MainWindow*)parent)->settingsForm, ":/Images/Images/settings/timings/buttons/settings_timing_button_down");
    addMenu("", 3, 3, 0, NULL, "");

    addMenu("Настройки", 0, 3, 4, NULL, ":/Images/Images/settings/buttons/settings_button_settings");
    addMenu("Пароль", 4, 0, 4, ((MainWindow*)parent)->serviceGeneralPasswordLeftForm, ":/Images/Images/settings/settings/buttons/settings_settings_button_password");
    addMenu("Оборудование", 4, 1, 4, ((MainWindow*)parent)->settingsSettingsConfigurationLeftForm, ":/Images/Images/settings/settings/buttons/settings_settings_button_configuration");
    addMenu("Общие", 4, 2, 4, ((MainWindow*)parent)->settingsForm, ":/Images/Images/settings/settings/buttons/settings_settings_button_different");
    addMenu("", 4, 3, 0, NULL, "");

    // добавим попытку выхода с сохранением и без
    addMenu("Выход", 0, 4, 5, NULL, ":/Images/Images/settings/buttons/settings_button_reserve");
    addMenu("Да", 5, 0, 5, NULL, ":/Images/Images/settings/buttons/settings_button_save");
    addMenu("Нет", 5, 1, 5, NULL, ":/Images/Images/settings/buttons/settings_button_notSave");
    addMenu("", 5, 2, 5, NULL, "");
    addMenu("", 5, 3, 0, NULL, "");

    showService();
}

SettingsMainRightForm::~SettingsMainRightForm()
{
    delete ui;
}

void SettingsMainRightForm::addMenu(QString name_, quint8 id_, quint8 id1_, quint8 goLevel_, QWidget* form_, QString png_)
{
    menu[id_][id1_].goLevel = goLevel_;
    menu[id_][id1_].form = form_;
    menu[id_][id1_].png = png_;
    menu[id_][id1_].name = name_;
}

void SettingsMainRightForm::showService()
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
//                if (menu[currentLevel][i].form)
//                    menu[currentLevel][i].form->hide();
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

    //ищем специальные пункты которые не так просты как мы думаем
    if (currentElement == 0 && currentLevel == 4)
    {// сброс пароля
        ((MainWindow*)parent)->serviceGeneralPasswordLeftForm->passwordVariable = "password";
        ((MainWindow*)parent)->serviceGeneralPasswordLeftForm->goStep(0);
    }
    if (currentElement == 1 && currentLevel == 1)
    {//супердиаг
        currentElement = -1;
        ServiceBUConfigForm * f = new ServiceBUConfigForm(parent);
        f->show();
        f->raise();
    }
    // душим езернет чтобы не мешал
    if (currentElement == 2 && currentLevel == 1)
    {//сеть
        QProcess p;
        p.start("ifconfig eth0 down");
        p.waitForFinished(1000 * 120);
    }
//    if (currentElement == 2 && currentLevel == 1)
//    {//супердиаг Тимурки
//        QProcess *proc = new QProcess;
//        QDir::setCurrent("/etc/");
//        proc->setWorkingDirectory("/etc/");
//        proc->startDetached("./GPIO_TESTING_NEW -platform eglfs --touch 0");
//        qDebug() << proc->errorString();
//        QApplication::quit();
//    }
    // наполним настройки
    if (currentElement == 2 && currentLevel == 4)
        ((MainWindow*)parent)->settingsForm->callGlobal();
    if (currentElement == 0 && currentLevel == 2)
        ((MainWindow*)parent)->settingsForm->callLightSweep();
    if (currentElement == 1 && currentLevel == 2)
        ((MainWindow*)parent)->settingsForm->callMediumSweep();
    if (currentElement == 2 && currentLevel == 2)
        ((MainWindow*)parent)->settingsForm->callHeavySweep();
    if (currentElement == 0 && currentLevel == 3)
        ((MainWindow*)parent)->settingsForm->callFrontTimings();
    if (currentElement == 1 && currentLevel == 3)
        ((MainWindow*)parent)->settingsForm->callMiddleTimings();
    if (currentElement == 2 && currentLevel == 3)
        ((MainWindow*)parent)->settingsForm->callBackTimings();

    // подпись заголовка
    ((MainWindow*)parent)->serviceSetingsName->raise();
    if (menu[currentLevel][currentElement].form == ((MainWindow*)parent)->settingsForm)
        ((MainWindow*)parent)->serviceSetingsName->setText(menu[currentLevel][currentElement].name);
    else
        ((MainWindow*)parent)->serviceSetingsName->setText("");
}

void SettingsMainRightForm::moveMenu(qint8 level)
{// организуем переход по меню
    if (currentElement != -1 && menu[currentLevel][currentElement].form)
        menu[currentLevel][currentElement].form->hide();
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

void SettingsMainRightForm::on_pushButton_1_clicked()
{
    if (currentLevel == 5)
    {// нажали в диалоге выхода ДА
        ((MainWindow*)parent)->settingsForm->on_pushButton_save_clicked();
        ((MainWindow*)parent)->serviceSetingsName->hide();
        hide();
        ((MainWindow*)parent)->menuMode = MainWindow::SweepMode;
        ((MainWindow*)parent)->superDiagMode = false;
        //((MainWindow*)parent)->Password_accepted_settings = false;
        // нажимаем выход чтобы нарисовать главный экран
        moveMenu(3);
        showService();
    }
    else
    {
        moveMenu(0);
        showService();
    }
}

void SettingsMainRightForm::on_pushButton_2_clicked()
{
    if (currentLevel == 5)
    {// Нажали в диалоге выхода НЕТ
        ((MainWindow*)parent)->serviceSetingsName->hide();
        hide();
        ((MainWindow*)parent)->menuMode = MainWindow::SweepMode;
        ((MainWindow*)parent)->superDiagMode = false;
        //((MainWindow*)parent)->Password_accepted_settings = false;
        // нажимаем выход чтобы нарисовать главный экран
        moveMenu(3);
        showService();
    }
    else
    {
        moveMenu(1);
        showService();
    }
}

void SettingsMainRightForm::on_pushButton_3_clicked()
{
    moveMenu(2);
    showService();
}

void SettingsMainRightForm::on_pushButton_4_clicked()
{
    moveMenu(3);
    showService();
}

void SettingsMainRightForm::on_pushButton_exit_clicked()
{
    // врубаем езернет при выходе с обновления
    if (currentElement == 2 && currentLevel == 1)
    {//сеть
        QProcess p;
        p.start("ifconfig eth0 up");
        p.waitForFinished(1000 * 120);
    }

    if (currentLevel == 0)
    {
        moveMenu(4);
        showService();
//        hide();
//        ((MainWindow*)parent)->menuMode = MainWindow::SweepMode;
//        ((MainWindow*)parent)->superDiagMode = false;
//        //((MainWindow*)parent)->Password_accepted_settings = false;
    }
    else
    {
        moveMenu(3);
        showService();
    }
}


