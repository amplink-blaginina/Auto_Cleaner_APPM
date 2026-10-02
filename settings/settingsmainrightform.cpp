#include "settingsmainrightform.h"
#include "ui_settingsmainrightform.h"

#include "settings/gpio/superDiag/serviceBUConfigform.h"
#include "settings/gpio/gpioPu/serviceGPIOPUform.h"
#include "settings/gpio/dvr/serviceDVRform.h"

#include "mainwindow.h"
#include <QTimer>

// «Регистратор» (GPIO, 4-я кнопка): нет ни одной камеры и нет записей - заходить некуда, кнопка заблокирована (RPI-RES_260929_55)
// force - стиль задать всегда (после перерисовки меню), иначе только при смене состояния (таймер)
static void updateDvrMenuButton(QPushButton *btn, const QString &png, bool force)
{
    const ServiceDVRForm::Availability a = ServiceDVRForm::availability();
    const bool on = a.cameras || a.records;
    if (!force && btn->property("dvrOn").isValid() && btn->property("dvrOn").toBool() == on)
        return;
    btn->setProperty("dvrOn", on);
    btn->setEnabled(on);
    btn->setStyleSheet("border-style:none;outline: none;background-image:url(" + png + (on ? "_off.png);" : "_dis.png);"));
}

SettingsMainRightForm::SettingsMainRightForm(MyCan* can, SettingsForm *settingsForm, MainWindow* mainWindow, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsMainRightForm)
{
    ui->setupUi(this);

    _can = can;
    _settingsForm = settingsForm;
    _mainWindow = mainWindow;
    _parent = parent;

    currentLevel = 0;
    currentElement = -1;

    buttons[0] = ui->pushButton_1;
    buttons[1] = ui->pushButton_2;
    buttons[2] = ui->pushButton_3;
    buttons[3] = ui->pushButton_4;

    //_mainWindow->settingsGlobalGlobalLeftForm
    addMenu("Общие настройки", 0, 0, 1, NULL, ":/Images/Images/settings/buttons/settings_button_gpio");
    addMenu("GPIO ПУ", 1, 0, 1, NULL, ":/Images/Images/settings/gpio/buttons/settings_gpio_button_gpioPy");
    addMenu("GPIO БУЦ", 1, 1, 1, NULL, ":/Images/Images/settings/gpio/buttons/settings_gpio_button_gpioBym");
    addMenu("WIFI", 1, 2, 1, _mainWindow->settingsWifiLeftForm, ":/Images/Images/settings/gpio/buttons/settings_gpio_button_update");
    addMenu("Регистратор", 1, 3, 1, NULL, ":/Images/Images/settings/gpio/buttons/settings_gpio_button_camera");

    addMenu("Режимы", 0, 1, 2, NULL, ":/Images/Images/settings/buttons/settings_button_mode");
    addMenu("Легкий + листья", 2, 0, 2, _mainWindow->settingsForm, ":/Images/Images/settings/modes/buttons/settings_mode_button_easyAndLeafHarvesting");
    addMenu("Средний", 2, 1, 2, _settingsForm, ":/Images/Images/settings/modes/buttons/settings_mode_button_average");
    addMenu("Тяжелый", 2, 2, 2, _settingsForm, ":/Images/Images/settings/modes/buttons/settings_mode_button_hard");
    addMenu("", 2, 3, 0, NULL, "");

    addMenu("Тайминги", 0, 2, 3, NULL, ":/Images/Images/settings/buttons/settings_button_timing");
    addMenu("Перед", 3, 0, 3, _settingsForm, ":/Images/Images/settings/timings/buttons/settings_timing_button_front");
    addMenu("Средний", 3, 1, 3, _settingsForm, ":/Images/Images/settings/timings/buttons/settings_timing_button_middle");
    addMenu("Задний", 3, 2, 3, _settingsForm, ":/Images/Images/settings/timings/buttons/settings_timing_button_down");
    addMenu("", 3, 3, 0, NULL, "");

    addMenu("Настройки", 0, 3, 4, NULL, ":/Images/Images/settings/buttons/settings_button_settings");
    addMenu("Пароль", 4, 0, 4, _mainWindow->serviceGeneralPasswordLeftForm, ":/Images/Images/settings/settings/buttons/settings_settings_button_password");
    addMenu("Оборудование", 4, 1, 4, _mainWindow->settingsSettingsConfigurationLeftForm, ":/Images/Images/settings/settings/buttons/settings_settings_button_configuration");
    addMenu("Общие", 4, 2, 4, _settingsForm, ":/Images/Images/settings/settings/buttons/settings_settings_button_different");
    addMenu("", 4, 3, 0, NULL, "");

    // добавим попытку выхода с сохранением и без
    addMenu("Выход", 0, 4, 5, NULL, ":/Images/Images/settings/buttons/settings_button_reserve");
    addMenu("Да", 5, 0, 5, NULL, ":/Images/Images/settings/buttons/settings_button_save");
    addMenu("Нет", 5, 1, 5, NULL, ":/Images/Images/settings/buttons/settings_button_notSave");
    addMenu("", 5, 2, 5, NULL, "");
    addMenu("", 5, 3, 0, NULL, "");

    // камеры и флешка появляются/пропадают, пока меню открыто - перепроверяем раз в 3 с (RPI-RES_260929_55)
    QTimer *dvrTimer = new QTimer(this);
    connect(dvrTimer, &QTimer::timeout, this, [this]() {
        if (isVisible() && currentLevel == 1)
            updateDvrMenuButton(buttons[3], menu[1][3].png, false);
    });
    dvrTimer->start(3000);

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
    if(currentElement <0){}
    for (int i = 0; i < 4; i++)
    {
        buttons[i]->setEnabled(true);// блокировка «Регистратора» - только на уровне GPIO (RPI-RES_260929_55)
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
    if (currentLevel == 1)
        updateDvrMenuButton(buttons[3], menu[1][3].png, true);

    if (currentLevel == 0)
    {// нарисуем выход ниже всех
        ui->pushButton_exit->setStyleSheet("border-style:none;outline: none;background-image:url(:/Images/Images/service/buttons/service_button_exit_off.png);");
    }
    else
    {
        ui->pushButton_exit->setStyleSheet("border-style:none;outline: none;background-image:url(:/Images/Images/settings/buttons/settings_button_cancel_off.png);");
    }

    //ищем специальные пункты которые не так просты как мы думаем
    if (currentElement == 0 && currentLevel == 4){// сброс пароля

        _mainWindow->resetPassword();//serviceGeneralPasswordLeftForm->passwordVariable = "password";
        //_mainWindow->serviceGeneralPasswordLeftForm->goStep(0);
    }
    if (currentElement == 0 && currentLevel == 1)
    {// GPIO ПУ
        currentElement = -1;
        ServiceGPIOPUForm * f = new ServiceGPIOPUForm(_can, _mainWindow, _parent);
        f->show();
        f->raise();
    }
    if (currentElement == 1 && currentLevel == 1)
    {//супердиаг
        currentElement = -1;
        ServiceBUConfigForm * f = new ServiceBUConfigForm(_can, _mainWindow, _parent);
        f->show();
        f->raise();
    }
    if (currentElement == 3 && currentLevel == 1)
    {// регистратор: камеры и записи
        currentElement = -1;
        ServiceDVRForm * f = new ServiceDVRForm(_mainWindow, _parent);
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
        _settingsForm->callGlobal();
    if (currentElement == 0 && currentLevel == 2)
        _settingsForm->callLightSweep();
    if (currentElement == 1 && currentLevel == 2)
        _settingsForm->callMediumSweep();
    if (currentElement == 2 && currentLevel == 2)
        _settingsForm->callHeavySweep();
    if (currentElement == 0 && currentLevel == 3)
        _settingsForm->callFrontTimings();
    if (currentElement == 1 && currentLevel == 3)
        _settingsForm->callMiddleTimings();
    if (currentElement == 2 && currentLevel == 3)
        _settingsForm->callBackTimings();

    if(currentElement>=0){
    // подпись заголовка
    auto form = menu[currentLevel][currentElement].form;
    _mainWindow->setServiceFormName(form, menu[currentLevel][currentElement].name);
    //_mainWindow->serviceSetingsName->raise();
    // if ( == _mainWindow->settingsForm)
    //     _mainWindow->serviceSetingsName->setText();
    // else
    //     _mainWindow->serviceSetingsName->setText("");
    }
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
        _mainWindow->settingsForm->on_pushButton_save_clicked();
        _mainWindow->serviceSetingsName->hide();
        hide();

        _mainWindow->currentState->setSweepMode();//menuMode = MainWindow::SweepMode;
        //_mainWindow->superDiagMode = false;
        //_mainWindow->Password_accepted_settings = false;
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
        _mainWindow->serviceSetingsName->hide();
        hide();
        _mainWindow->currentState->setSweepMode();//menuMode = MainWindow::SweepMode;
        //_mainWindow->superDiagMode = false;
        //_mainWindow->Password_accepted_settings = false;
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
//        _mainWindow->menuMode = MainWindow::SweepMode;
//        _mainWindow->superDiagMode = false;
//        //_mainWindow->Password_accepted_settings = false;
    }
    else
    {// назад в корень меню (раньше moveMenu(3) через скрытый пункт "", в GPIO там теперь «Регистратор»)
        if (currentElement != -1 && menu[currentLevel][currentElement].form)
            menu[currentLevel][currentElement].form->hide();
        currentElement = -1;
        currentLevel = 0;
        showService();
    }
}


