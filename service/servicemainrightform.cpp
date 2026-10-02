#include "servicemainrightform.h"
#include "ui_servicemainrightform.h"

//#include "service/gpio/superDiag/serviceBUConfigform.h"
#include "password_form.h"

#include "mainwindow.h"
#include "settings/toJournal/serviceTOJournalform.h"   // Журнал ТО (RPI-RES_260929_01)

ServiceMainRightForm::ServiceMainRightForm(MainWindow* mainWindow, QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceMainRightForm)
{
    ui->setupUi(this);
    _mainWindow = mainWindow;
    parent = parent_;
    _settings = _mainWindow->settings;

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
    addMenu("Дата, время", 1, 0, 1, _mainWindow->serviceGlobalDateTimeLeftForm, ":/Images/Images/service/global/buttons/service_generalSettings_button_time");
    addMenu("Пароль", 1, 1, 1, _mainWindow->serviceGeneralPasswordLeftForm, ":/Images/Images/service/global/buttons/service_generalSettings_button_password");
    addMenu("PDF", 1, 2, 1, NULL, ":/Images/Images/service/global/buttons/service_generalSettings_button_PDF");
    addMenu("", 1, 3, 0, NULL, "");

    addMenu("Устройства", 0, 1, 2, NULL, ":/Images/Images/service/buttons/service_button_engine");
    addMenu("Гидравлика", 2, 0, 2, _mainWindow->serviceDevicesHydraulicsLeftForm, ":/Images/Images/service/devices/buttons/service_valve_button_hydraulics");
    addMenu("ДКП", 2, 1, 2, _mainWindow->serviceDevicesDKPLeftForm, ":/Images/Images/service/devices/buttons/service_valve_button_DKP");
    addMenu("", 2, 2, 2, NULL, "");
    addMenu("", 2, 3, 0, NULL, "");

    addMenu("Прочее", 0, 2, 3, NULL, ":/Images/Images/service/buttons/service_button_valve");
    addMenu("ДВС", 3, 0, 3, _mainWindow->serviceOtherEngineLeftForm, ":/Images/Images/service/other/buttons/service_engine_button_engine");
    addMenu("ТО", 3, 1, 3, NULL,   /* Журнал ТО (RPI-RES_260929_01): экран открывается после пароля в passwordDiagOk() */ ":/Images/Images/service/other/buttons/service_engine_button_TO");
    addMenu("Освещение", 3, 2, 3, _mainWindow->serviceOtherLightLeftForm, ":/Images/Images/service/other/buttons/service_engine_button_light");
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

ServiceMainRightForm::~ServiceMainRightForm(){
    delete ui;
}

void ServiceMainRightForm::addMenu(QString name_, quint8 id_, quint8 id1_, quint8 goLevel_, QWidget* form_, QString png_){
    menu[id_][id1_].goLevel = goLevel_;
    menu[id_][id1_].form = form_;
    menu[id_][id1_].png = png_;
    menu[id_][id1_].name = name_;
}

void ServiceMainRightForm::showService()
{// отображаем правое меню и левую форму в соответствии с данными меню
    QString path = "border-style:none;outline: none;background-image:url(";
    for (int i = 0; i < 4; i++){
        if (menu[currentLevel][i].png != ""){
            buttons[i]->show();

            if (i == currentElement && menu[currentLevel][i].form){
                buttons[i]->setStyleSheet(path + menu[currentLevel][i].png + "_on.png);");
                menu[currentLevel][i].form->show();
                menu[currentLevel][i].form->raise();
            }
            else{
                if (menu[currentLevel][i].form)
                    menu[currentLevel][i].form->hide();
                buttons[i]->setStyleSheet(path + menu[currentLevel][i].png + "_off.png);");
            }
        }
        else
            buttons[i]->hide();
    }

    if (currentLevel == 0){// нарисуем выход ниже всех
        ui->pushButton_exit->setStyleSheet(path + ":/Images/Images/service/buttons/service_button_exit_off.png);");
    }
    else{
        ui->pushButton_exit->setStyleSheet(path + ":/Images/Images/settings/buttons/settings_button_cancel_off.png);");
    }

    // подпись заголовка
//    ui->label_name->raise();
//    if (currentElement == 1 && currentLevel == 3)
//        ui->label_name->setText(menu[currentLevel][currentElement].name);
//    else
//        ui->label_name->setText("");

    if (currentElement == 1 && currentLevel == 3 && oldCurrentElement != currentElement && oldCurrentLevel != currentLevel){// техосмотр
        // Журнал ТО без пароля (RPI-RES_260929_09): пароль спрашивает журнал на страницах «Процедуры ТО» и «Проведение ТО»
        ServiceTOJournalForm* journal = new ServiceTOJournalForm(_mainWindow, parent);
        journal->setGeometry(0, 0, 1024, 600);
        journal->show();
        journal->raise();
        currentElement = -1;
    }
    if (currentElement == 0 && currentLevel == 2){// гидравлика инициализация
        _mainWindow->serviceDevicesHydraulicsLeftForm->refreshSliders();
    }
    if (currentElement == 1 && currentLevel == 1){// сброс пароля
        _mainWindow->serviceGeneralPasswordLeftForm->passwordVariable = "passwordDiag";
        _mainWindow->serviceGeneralPasswordLeftForm->goStep(0);
    }
    if (currentElement == 0 && currentLevel == 1 && oldCurrentElement != currentElement && oldCurrentLevel != currentLevel){// выставить текущую дату
        _mainWindow->serviceGlobalDateTimeLeftForm->actualTime();
    }
    //ищем специальные пункты которые не так просты как мы думаем
    if (currentElement == 2 && currentLevel == 1){//pdf
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
    if (pass == _mainWindow->getReader()->readSettingsValue("Global/secretPasswordDiag").toString().toInt()){// сбросим одноразовы пароль
        if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
            QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");
        int random = std::rand() % ((9999 + 1) - 1) + 1;
        //qDebug() << random;
        _settings->setValue("Global/secretPasswordDiag", random);// рандом от 1 до 9999
        _settings->sync();
        system("sync");
        // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
        _mainWindow->removeBadSettings();
    }
    // Журнал ТО без пароля (RPI-RES_260929_09): журнал открывается без пароля, этот слот больше не вызывается
}

void ServiceMainRightForm::moveMenu(qint8 level){// организуем переход по меню
    if (level == 0 && currentLevel == 5){// ДА
        currentLevel = oldCurrentLevel;
        currentElement = oldCurrentElement;
        menu[currentLevel][currentElement].form->hide();
        level = oldLevel;
        // Журнал ТО (RPI-RES_260929_01): сохранение старого экрана ТО больше не нужно
        if (currentElement == 0 && currentLevel == 1){// дату редактировали
            _mainWindow->serviceGlobalDateTimeLeftForm->changed = false;
            QString dataa = "date -s @\"" + QString::number(_mainWindow->serviceGlobalDateTimeLeftForm->curDT.toTime_t()) + "\"";
//            QString dataa = "date -s "
//                    +QString::number(_mainWindow->serviceGlobalDateTimeLeftForm->curDT.date().year(),10)
//                    +"."
//                    +QString::number(_mainWindow->serviceGlobalDateTimeLeftForm->curDT.date().month(),10)
//                    +"."
//                    +QString::number(_mainWindow->serviceGlobalDateTimeLeftForm->curDT.date().day(),10)
//                    +"-"
//                    +QString::number(_mainWindow->serviceGlobalDateTimeLeftForm->curDT.time().hour(),10)
//                    +":"
//                    +QString::number(_mainWindow->serviceGlobalDateTimeLeftForm->curDT.time().minute(),10)
//                    +":"
//                    +QString::number(0,10);
            system(dataa.toUtf8().constData());
            system("hwclock -w");
            system("sync");
        }
        oldCurrentElement = -1;
        oldCurrentLevel = -1;
    }
    if (level == 1 && currentLevel == 5){// НЕТ
        currentLevel = oldCurrentLevel;
        currentElement = oldCurrentElement;
        if (currentElement == 1 && currentLevel == 3 && menu[currentLevel][currentElement].form)
            menu[currentLevel][currentElement].form->hide();
        if (currentElement == 0 && currentLevel == 1)
            _mainWindow->serviceGlobalDateTimeLeftForm->changed = false;// дату редактировали
        level = oldLevel;
        oldCurrentElement = -1;
        oldCurrentLevel = -1;
    }
    if (level == 3 && currentLevel == 5){// ОТМЕНА сохранения
        currentLevel = oldCurrentLevel;
        currentElement = oldCurrentElement;
        showService();
        return;
    }

    // в некоторых случаях при переходе из меню в меню надо попасть на вопрос о сохранении или не сохранении. попробуем сделать это перенаправляясь на скрытое меню
    if (currentElement == 0 && currentLevel == 1 && _mainWindow->serviceGlobalDateTimeLeftForm->changed){// дата время сменилась
        oldCurrentLevel = currentLevel;
        oldCurrentElement = currentElement;
        oldLevel = level;
        currentLevel = 5;
        currentElement = -1;
        return;
    }
    if (currentElement == 1 && currentLevel == 3 && menu[currentLevel][currentElement].form && menu[currentLevel][currentElement].form->isVisible()){// вышли из ТО
        oldCurrentLevel = currentLevel;
        oldCurrentElement = currentElement;
        oldLevel = level;
        currentLevel = 5;
        currentElement = -1;
        return;
    }

    if (menu[currentLevel][level].goLevel == currentLevel)
        currentElement = level;
    else{
        if (currentElement != -1 && menu[currentLevel][currentElement].form)
            menu[currentLevel][currentElement].form->hide();
        currentElement = -1;
    }
    currentLevel = menu[currentLevel][level].goLevel;
}

void ServiceMainRightForm::on_pushButton_1_clicked(){
    moveMenu(0);
    showService();
}

void ServiceMainRightForm::on_pushButton_2_clicked(){
    moveMenu(1);
    showService();
}

void ServiceMainRightForm::on_pushButton_3_clicked(){
    moveMenu(2);
    showService();
}

void ServiceMainRightForm::on_pushButton_4_clicked(){
    moveMenu(3);
    showService();
}

void ServiceMainRightForm::on_pushButton_exit_clicked(){
    if (currentLevel == 0)    {
        _mainWindow->serviceSetingsName->hide();
        hide();
        _mainWindow->currentState->setSweepMode();
        _mainWindow->starter->resetIgnitionTimer();//ignitionOffTimer = 0;
        //_mainWindow->Password_accepted_settings = false;
    }
    else{
        // выход невидимый
        moveMenu(3);
        showService();
    }
}


