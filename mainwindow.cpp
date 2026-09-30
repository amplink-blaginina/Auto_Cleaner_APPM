#include "mainwindow.h"

#include "BoolStateWatcher.h"
#include "password_form.h"
#include "ui_mainwindow.h"

#include <Controllers/viewcontroller.h>
#include <currentstate.h>
#include <screenlog.h>
#include <settingsreader.h>

#include <QLayoutItem>
#include <QPushButton>
#include <QScroller>
#include <QScrollBar>

#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/j1939.h>

#define MAXSOCK 16
#define DEVELOPER_MODE 0

QLocale EngLocale (QLocale::Russian);

static int ptsInc = 0;
const QString programmVersionString = QStringLiteral("AutoCleaner APPM v3.021");

//Changes
// 3.001 - форкнулся от APPM2 imx6, удалил лишнее и накатил на нее все от разбери с 200 и 318D4
// 3.002 - доработка интерфейса, изменение метода сохранения времени(метод imx6 не работал для разбери), добавление новых сигналов и передвижение старых
// 3.003 - переназначение матричной клавиатуры, фиксы интерфейсные, добавлены отскоки, добавлена пауза, защиты процессов с 200 машины, вывод нажатой кнопки
// 3.004 - очень много изменений в логике и алгоритмах(скорее всего все сломается),фикс включения стартера, сдвиг времени на экране,логика для физических кнопок(много), логика для экранных кнопок(много), сообщения о концевиках и таймаутах, коэфф для всех давлений, снятие запретов для ручного режима
// 3.005 - если выбран прижим щетки то нажатие на кнопки щетки во время программы вызывает поджим
// 3.006 - работа с прижимом и плавающим режимом во время уборки. новая работа с двигателем. кучи блокировок и проверок, так же прокрутка (изменнеия около 650 строк по логике, надеюсь что то заработает), новые параметры в настройках(по двигателю и стартеру)
// 3.007 - режим прокрутки датчик теплореле инвертирован, темп разгона вентиялтора
// 3.008 - включение питания датчиков, фикс онлайна вспомогательного двигателя(спамм сообщений+возможно дергание логики прокрутки), wifi обновление до каталога APPM
// 3.009 - новые параметры настройки (отключение требования прокрутки, отключение блокировки по температуре, игнорирование аварий), фикс логики блокировки по температуре (теплореле снимает блок а не устанавливает; блокируем только если движок когда-либо прислал данные и температура ниже порога), статус-лейбл и фикс кнопок прокрутки в сервисе, инверсия реле масла
// 3.010 - заблокированная кнопка прокрутки показывала шлак от скопированной кнопки, проверил настройки(был добавлены)-должно свайпаться вниз, гшлушит двигло по нажатию кнопки, уведомление о завершении прокрутки, учет последней даты прокрутки

// на ПВИ должен быть настроен j1939 (ядро с поддержкой) . так же должен быть сконфигурен can1 can2 как j1939 с адресом 0x08 для камаза и 0x07 для движка сзади
// на последних машинах перепутали can0 и can1(((


void MainWindow::createTimers()
{
    connect(&mainProgressTimer, &QTimer::timeout, this, &MainWindow::mainProgress);
    mainProgressTimer.start(100);

    connect(&repaintTimer, &QTimer::timeout, this, &MainWindow::repaintProgress);
    repaintTimer.start(500);

    connect(&oneSecondTimer, &QTimer::timeout, this, &MainWindow::oneSecond);
    oneSecondTimer.start(1000);
}

void MainWindow::registerPhysButtons(){

    m_buttonManager.registerButton(GPIOInput::IN_BROOM_UP,{
                   .onPressed = [this](){},//emit broomCentral->setDirection(organsEnums::Up);
                   .onReleased = [this](){}});//emit broomCentral->setDirection(organsEnums::None);

    m_buttonManager.registerButton(GPIOInput::IN_BROOM_DOWN,{
                   .onPressed = [this](){},//broomCentral->setDirection(organsEnums::Down);
                   .onReleased = [this](){}});//broomCentral->setDirection(organsEnums::None);

    m_buttonManager.registerButton(GPIOInput::IN_BROOM_LEFT,{
                   .onPressed = [this](){},//broomCentral->setDirection(organsEnums::Left);
                   .onReleased = [this](){}});//broomCentral->setDirection(organsEnums::None);

    m_buttonManager.registerButton( GPIOInput::IN_BROOM_RIGHT,{
                   .onPressed = [this](){ },//broomCentral->setDirection(organsEnums::Right);
                   .onReleased = [this](){}});//broomCentral->setDirection(organsEnums::None);

    m_buttonManager.registerButton(GPIOInput::IN_DUMP_UP,{
                                                           .onPressed = [this](){ qDebug()<<"!!!DumpUp";
                                                               //frontRail->setDirection(organsEnums::Up);
                                                           },
                   .onReleased = [this](){qDebug()<<"!!!DumpUp stop";
                                                               //frontRail->setDirection(organsEnums::None);
                                                           }});

    m_buttonManager.registerButton( GPIOInput::IN_DUMP_DOWN,{
                   .onPressed = [this](){qDebug()<<"!!!DumpDown";//frontRail->setDirection(organsEnums::Down);
                                                             },
                   .onReleased = [this](){qDebug()<<"!!!DumpDown stop";//frontRail->setDirection(organsEnums::None);
                                                             }});

    m_buttonManager.registerButton(GPIOInput::IN_DUMP_LEFT,{
                   .onPressed = [this](){},//frontRail->setDirection(organsEnums::Left);
                   .onReleased = [this](){}});//frontRail->setDirection(organsEnums::None);

    m_buttonManager.registerButton(GPIOInput::IN_DUMP_RIGHT,{
                   .onPressed = [this](){},//frontRail->setDirection(organsEnums::Right);
                   .onReleased = [this](){}});//frontRail->setDirection(organsEnums::None);

    m_buttonManager.registerButton(GPIOInput::IN_BLOW_UP,{
                   .onPressed = [this](){},
                       // blower->goUp();
                       // printOrganStatus(organsEnums::Blower, organsEnums::Up, true);
                   .onReleased = [this](){}});
                       // blower->goOff();
                       // printOrganStatus(organsEnums::Blower, organsEnums::Up, false);

    m_buttonManager.registerButton(GPIOInput::IN_BLOW_DOWN,{
                   .onPressed = [this](){},
                       // blower->goDown();
                       // printOrganStatus(organsEnums::Blower, organsEnums::Down, true);
                   .onReleased = [this](){}});
                       // blower->goOff();
                       // printOrganStatus(organsEnums::Blower, organsEnums::Down, false);


    m_buttonManager.registerButton(GPIOInput::IN_BLOW_LEFT,{
                   .onPressed = [this](){},//blower->goSlide(false);printOrganStatus(organsEnums::Blower, organsEnums::Left, true);
                   .onReleased = [this](){}});//blower->goNone();printOrganStatus(organsEnums::Blower, organsEnums::Left, false);

    m_buttonManager.registerButton(GPIOInput::IN_BLOW_RIGHT,{
                   .onPressed = [this](){},//blower->goSlide(true); printOrganStatus(organsEnums::Blower, organsEnums::Right, true);
                   .onReleased = [this](){}});//blower->goNone(); printOrganStatus(organsEnums::Blower, organsEnums::Right, false);
    m_buttonManager.registerButton(GPIOInput::IN_STARTER,{
                  .onPressed = [this](){},//blower->goSlide(true); printOrganStatus(organsEnums::Blower, organsEnums::Right, true);
                  .onReleased = [this](){}});//blower->goNone(); printOrganStatus(organsEnums::Blower, organsEnums::Right, false);

}



void MainWindow::configureFilters(){
    m_waterSensorWatcher = BoolStateWatcher{
        {
           // .onUpdate = []{},
            .onActivated = [this] {
                  view->addLogWarning("Вода в топливе текущие");
                  waterSensorStartedAt = TOCurValues["Engine"];},
            .onDeactivated = [] {},
            .whileActive = [this] {
                  const bool waterRed = (TOCurValues["Engine"] - waterSensorStartedAt >= (quint32)(globals->waterSensorRedHours * 3600));
                  updateIndicatorPixmap(ui->label_waterInFuel, waterRed ? "red" : "yellow", "water_in_fuel");
                  ui->label_waterInFuel->show();},
            .whileInactive = [this] {
                ui->label_waterInFuel->hide();}
        }
    };


    m_oilFilterWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {view->addLogError("Засорение масляного фильтра");},
            .onDeactivated = [this] {view->addLog("Сигнал засорения масляного фильтра снят");},
            .whileActive = [this] {
                updateIndicatorPixmap( ui->label_oilFilter, "red", "oil_filter" );
                ui->label_oilFilter->show();
                can0->setState(StateIgnitionOut, false);
                starter->resetIgnitionTimer();},
            .whileInactive = [this] { ui->label_oilFilter->hide();}
        }
    };

    m_airFilterWatcher = BoolStateWatcher{
        {
            .onActivated = [this]{
                view->addLogWarning("Засорение воздушного фильтра");
                airFilterStartedAt = TOCurValues["Engine"];},
            .onDeactivated = []{},
            .whileActive = [this]{
                const bool airRed = (TOCurValues["Engine"] - airFilterStartedAt >= (quint32)(globals->airFilterRedHours * 3600));
                updateIndicatorPixmap(ui->label_airFilter, airRed ? "red" : "yellow", "air_filter");
                ui->label_airFilter->show();},
            .whileInactive = [this]{ ui->label_airFilter->hide();}
        }
    };

    m_heatRelayWatcher = BoolStateWatcher{
        {
           .onActivated = [this]{view->addLogWarning("Требуется прогрев вспомогательного ДВС");},
           .onDeactivated = []{},
           .whileActive = [this]{
               updateIndicatorPixmap(ui->label_engineLowTemperature, "yellow", "engine_low_temperature");
               ui->label_engineLowTemperature->show();
           },
           .whileInactive = [this]{ui->label_engineLowTemperature->hide();}
        }
    };
}
void MainWindow::setBroomFlow(bool state)
{
    if (state == workMode.centralBroomFlow) {
        return;
    }

    workMode.centralBroomFlow = state;

    if (startClean) {
        broomCentral->setFlowActive(state);
    }

    showWorkMode();
    updateBroomFlowPressIcon();
}

void MainWindow::setDumpFlow(bool state){
    if(state == workMode.frontDumpFlow){
        return;
    }
    workMode.frontDumpFlow = state;
    if (startClean){
        view->addLog(state? "Отвал плавание": "Отвал плавание завершено");
        frontRail->setFlowActive(state);}
    else{
        if(state)
            view->addLog("Отвал выбрано плавание ");
    }
    QString path = //"background-image: url(:/Images/main/buttons/configuration_button_variable_up_";
        "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_up_";////dozerBlade_lift_";
    view->setStyle(ui->label_dumpFloatPress, path + (state?"on_down_blocked);":"off_down_blocked);" ));
    showWorkMode();
}

void MainWindow::updateBroomFlowPressIcon()
{
    QString path =
        "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_";

    path += workMode.centralBroomFlow
                ? (workMode.centralBroomPress ? "on.png);" : "up_on.png);")
                : (workMode.centralBroomPress ? "down_on.png);" : "off.png);");

    view->setStyle(ui->label_centralBroomFloatPress, path);
}

void MainWindow::setButtonVisualState(QPushButton* button, QLabel* iconLabel, const QString& style, bool wasDown)
{
    button->setProperty("wasDown", wasDown);

    if (iconLabel != nullptr) {
        view->setStyle(iconLabel, style);
    }
}

void MainWindow::configureButtons(){

    m_dumpUpWatcher = BoolStateWatcher{
   {
       .onActivated = [this] {

        setButtonVisualState(
            ui->pushButton_dumpUp,
            ui->label_dumpUpDown,
            dumpVertPath + "up_off.png);",
            true
            );

           if(isDumpTransitioning()){
               view->addLog("Отвал в движении, ожидайте");
               return;
           }
           if (startClean){
               frontRail->setDirection(organsEnums::Up);
           }
       },
       .onDeactivated = [this] {
             setButtonVisualState(
                 ui->pushButton_dumpUp,
                 ui->label_dumpUpDown,
                 dumpVertPath + "off.png);",
                 false
                 );

           if(isDumpTransitioning()){
               view->addLog("Отвал в движении, ожидайте");
               return;
           }

           if (startClean){
               frontRail->setDirection(organsEnums::None);}
       },
       .whileActive = [this] {},
       .whileInactive = [this] {}
   }};

    m_dumpDownWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_dumpDown,
                    ui->label_dumpUpDown,
                    dumpVertPath + "down_off.png);",
                    true
                    );

                if (isDumpTransitioning()) {
                    view->addLog("Отвал в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    frontRail->setDirection(organsEnums::Down);
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_dumpDown,
                    ui->label_dumpUpDown,
                    dumpVertPath + "off.png);",
                    false
                    );

                if (isDumpTransitioning()) {
                    view->addLog("Отвал в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    frontRail->setDirection(organsEnums::None);
                }
            },

            .whileActive = [this] {},
            .whileInactive = [this] {}
        }
    };

    m_dumpLeftWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_dumpLeft,
                    ui->label_dump,
                    dumpHorPath + "left_on.png);",
                    true
                    );

                if (isDumpTransitioning()) {
                    view->addLog("Отвал в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    frontRail->setDirection(organsEnums::Left);
                } else {
                    workMode.frontDumpLeft = !workMode.frontDumpLeft;

                    if (workMode.frontDumpLeft) {
                        view->addLog("Отвал: выбрана левая сторона");
                    }

                    workMode.frontDumpRight = false;
                    showWorkMode();
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_dumpLeft,
                    ui->label_dump,
                    getDumpDefaultIcon(),
                    false
                    );

                if (isDumpTransitioning()) {
                    view->addLog("Отвал в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    frontRail->setDirection(organsEnums::None);
                }
            },

            .whileActive = [this] {},
            .whileInactive = [this] {}
        }
    };

    m_dumpRightWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_dumpRight,
                    ui->label_dump,
                    dumpHorPath + "right_on.png);",
                    true
                    );

                if (isDumpTransitioning()) {
                    view->addLog("Отвал в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    frontRail->setDirection(organsEnums::Right);
                } else {
                    workMode.frontDumpRight = !workMode.frontDumpRight;

                    if (workMode.frontDumpRight) {
                        view->addLog("Отвал: выбрана правая сторона");
                    }

                    workMode.frontDumpLeft = false;
                    showWorkMode();
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_dumpRight,
                    ui->label_dump,
                    getDumpDefaultIcon(),
                    false
                    );

                if (isDumpTransitioning()) {
                    return;
                }

                if (startClean) {
                    frontRail->setDirection(organsEnums::None);
                }
            },

            .whileActive = [this] {},
            .whileInactive = [this] {}
        }
    };

    m_dumpFlowWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_dumpFlow,
                    nullptr,
                    QString(),
                    true
                    );

                setDumpFlow(!workMode.frontDumpFlow);
            },

            .onDeactivated = [this] {},
            .whileActive = [this] {},
            .whileInactive = [this] {}
        }
    };


    //-------------------------------------------------------------------------
    m_broomUpWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_centralBroomUp,
                    ui->label_centralBroomUpDown,
                    broomVertPath + "up_on.png);",
                    true
                    );

                if (isBroomTransitioning()) {
                    view->addLog("Щетка в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    //view->addLog("Щётка: движение вверх");
                    broomCentral->setDirection(organsEnums::Up);
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_centralBroomUp,
                    ui->label_centralBroomUpDown,
                    broomVertPath + "off.png);",
                    false
                    );

                if (isBroomTransitioning()) {
                    return;
                }

                if (startClean) {
                    //view->addLog("Щётка движение вверх завершено");
                    broomCentral->setDirection(organsEnums::None);
                }
            },

            .whileActive = [this] {},
            .whileInactive = [this] {}
        }
    };

    m_broomDownWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_centralBroomDown,
                    ui->label_centralBroomUpDown,
                    broomVertPath + "down_on.png);",
                    true
                    );

                if (isBroomTransitioning()) {
                    view->addLog("Щетка в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    //view->addLog("Щётка: движение вниз");
                    broomCentral->setDirection(organsEnums::Down);
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_centralBroomDown,
                    ui->label_centralBroomUpDown,
                    broomVertPath + "off.png);",
                    false
                    );

                if (startClean) {
                    //view->addLog("Щётка движение вниз завершено");
                    broomCentral->setDirection(organsEnums::None);
                }
            },

            .whileActive = [this] {},
            .whileInactive = [this] {}
        }
    };

    m_broomLeftWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_centralBroomLeft,
                    ui->label_centralBroom,
                    broomHorPath + "left_on.png);",
                    true
                    );

                if (isBroomTransitioning()) {
                    view->addLog("Щетка в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    broomCentral->setDirection(organsEnums::Left);
                } else {
                    workMode.centralBroomLeft = !workMode.centralBroomLeft;

                    if (workMode.centralBroomLeft) {
                        view->addLog("Щетка: выбрана левая сторона");
                    }

                    workMode.centralBroomRight = false;
                    showWorkMode();
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_centralBroomLeft,
                    ui->label_centralBroom,
                    getBroomDefaultIcon(),
                    false
                    );

                if (startClean) {
                    //view->addLog("Щетка движение влево завершено");
                    broomCentral->setDirection(organsEnums::None);
                }
            },

            .whileActive = [this] {},
            .whileInactive = [this] {}
        }
    };

    m_broomRightWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_centralBroomRight,
                    ui->label_centralBroom,
                    broomHorPath + "right_on.png);",
                    true
                    );

                if (isBroomTransitioning()) {
                    view->addLog("Щетка в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    broomCentral->setDirection(organsEnums::Right);
                } else {
                    workMode.centralBroomRight = !workMode.centralBroomRight;

                    if (workMode.centralBroomRight) {
                        view->addLog("Щетка: выбрана правая сторона");
                    }

                    workMode.centralBroomLeft = false;
                    showWorkMode();
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_centralBroomRight,
                    ui->label_centralBroom,
                    getBroomDefaultIcon(),
                    false
                    );

                if (startClean) {
                    broomCentral->setDirection(organsEnums::None);
                }
            },

            .whileActive = [this] {
            },

            .whileInactive = [this] {}
        }
    };

    m_broomFlowWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_centralBroomFlow,
                    nullptr,
                    QString(),
                    true
                    );

                setBroomFlow(!workMode.centralBroomFlow);
            },

            .onDeactivated = [this] {},
            .whileActive = [this] {},
            .whileInactive = [this] {}
        }
    };

    m_broomPressWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_centralBroomPress,
                    nullptr,
                    QString(),
                    true
                    );

                workMode.centralBroomPress = !workMode.centralBroomPress;
                broomCentral->setPressActive(workMode.centralBroomPress);
                showWorkMode();

                QString path =
                    "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_";

                path += workMode.centralBroomFlow
                            ? (workMode.centralBroomPress ? "on.png);" : "up_on.png);")
                            : (workMode.centralBroomPress ? "down_on.png);" : "off.png);");

                view->setStyle(ui->label_centralBroomFloatPress, path);
            },

            .onDeactivated = [this] {},
            .whileActive = [this] {},
            .whileInactive = [this] {}
        }
    };


    //-------------------------------------------------------------------------
    m_blowUpWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_blowerUp,
                    ui->label_blowerUpDown,
                    blowerVertPath + "up_on.png);",
                    true
                    );

                if (isBlowTransitioning()) {
                    view->addLog("Обдув в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    if (blower->isRotating()) {
                        blower->setStartMomentForStopping();
                        view->addLog(
                            "Удерживайте кнопку вверх для остановки обдува и подъёма"
                            );
                    } else {
                        blower->goUp();
                    }
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_blowerUp,
                    ui->label_blowerUpDown,
                    getBlowerVertIcon(),
                    false
                    );

                if (isBlowTransitioning()) {
                    return;
                }

                if (startClean) {
                    blower->goNone();
                }
            },

            .whileActive = [this] {
                blower->updateWhenUpPressed();
            },

            .whileInactive = [this] {}
        }
    };

    m_blowDownWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_blowerDown,
                    ui->label_blowerUpDown,
                    blowerVertPath + "down_on.png);",
                    true
                    );

                if (isBlowTransitioning()) {
                    view->addLog("Обдув в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    if (!blower->isRotating()) {
                        blower->setStartMomentForStarting();
                    }

                    // blower->goDown();
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_blowerDown,
                    ui->label_blowerUpDown,
                    getBlowerVertIcon(),
                    false
                    );

                if (isBlowTransitioning()) {
                    return;
                }

                if (startClean) {
                    blower->goNone();
                }
            },

            .whileActive = [this] {
                blower->updateWhenDownPressed();
            },

            .whileInactive = [this] {}
        }
    };

    m_blowLeftWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_blowerLeft,
                    ui->label_blower,
                    blowerHorPath + "left_on.png);",
                    true
                    );

                if (isBlowTransitioning()) {
                    view->addLog("Обдув в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    blower->setStartMomentForRotation();// отсчёт удержания нужен и когда обдув не крутится
                    if (blower->isRotating()) {
                        view->addLog(
                            "Удерживайте кнопку влево для смены направления обдува"
                            );
                    } else {
                        blower->goSlide(false);
                    }
                } else {
                    view->addLog("Обдув: выбрана левая сторона");
                    workMode.blowLeft = !workMode.blowLeft;
                    workMode.blowRight = false;
                    showWorkMode();
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_blowerLeft,
                    ui->label_blower,
                    getBlowerDefaultIcon(),
                    false
                    );

                if (isBlowTransitioning()) {
                    return;
                }

                if (startClean) {
                    blower->goNone();
                }
            },

            .whileActive = [this] {
                blower->updateWhenRotationPressed(false);
            },

            .whileInactive = [this] {}
        }
    };

    m_blowRightWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(
                    ui->pushButton_blowerRight,
                    ui->label_blower,
                    blowerHorPath + "right_on.png);",
                    true
                    );

                if (isBlowTransitioning()) {
                    view->addLog("Обдув в движении, ожидайте");
                    return;
                }

                if (startClean) {
                    blower->setStartMomentForRotation();// отсчёт удержания нужен и когда обдув не крутится
                    if (blower->isRotating()) {
                        view->addLog(
                            "Удерживайте кнопку вправо для смены направления обдува"
                            );
                    } else {
                        blower->goSlide(true);
                    }
                } else {
                    view->addLog("Обдув: выбрана правая сторона");
                    workMode.blowRight = !workMode.blowRight;
                    workMode.blowLeft = false;
                    showWorkMode();
                }
            },

            .onDeactivated = [this] {
                setButtonVisualState(
                    ui->pushButton_blowerRight,
                    ui->label_blower,
                    getBlowerDefaultIcon(),
                    false
                    );

                if (isBlowTransitioning()) {
                    return;
                }

                if (startClean) {
                    blower->goNone();
                }
            },

            .whileActive = [this] {
                blower->updateWhenRotationPressed(true);
            },

            .whileInactive = [this] {}
        }
    };

    m_broomPressWatcher = BoolStateWatcher{
        {
            .onActivated = [this] {
                setButtonVisualState(ui->pushButton_centralBroomPress, nullptr, QString(), true);
                workMode.centralBroomPress = !workMode.centralBroomPress;
                broomCentral->setPressActive(workMode.centralBroomPress);

                showWorkMode();
                updateBroomFlowPressIcon();
            },

            .onDeactivated = [this] {},
            .whileActive = [this] {},
            .whileInactive = [this] {}
        }
    };
    // m_broomPressWatcher = BoolStateWatcher{
    //   {
    //       .onActivated = [this] {
    //           ui->pushButton_centralBroomFlow->setProperty("wasDown", true);
    //           workMode.centralBroomPress = !workMode.centralBroomPress;
    //           broomCentral->setPressActive(workMode.centralBroomPress);
    //           showWorkMode();


    //           QString path = "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_";
    //           path += workMode.centralBroomFlow?(workMode.centralBroomPress? "on.png);": "up_on.png);"):(workMode.centralBroomPress? "down_on.png);": "off.png);");
    //           view->setStyle(ui->label_centralBroomFloatPress, path);
    //       },
    //       .onDeactivated = [this] {},
    //       .whileActive = [this] {},
    //       .whileInactive = [this] {}
    //   }};

}


MainWindow::MainWindow(int argc, char *argv[], QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    can0 = NULL;
    settings = new QSettings(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini", QSettings::IniFormat);
    _settingsReader = new SettingsReader(settings);
    _settingsReader->setDefaults();//дефолтные настройки
    settingsStore = new SettingsStore(settings);

    maintenanceTracker = new MaintenanceTracker(
        _settingsReader,
        settingsStore
        );

    maintenanceTracker->createRules();
    maintenanceTracker->load();

    QString can_device = _settingsReader->readSettingsValue("Global/canDeivce").toString();
    QString j1939_device = _settingsReader->readSettingsValue("Global/j1939Deivce").toString();

    _settingsReader->readSettingsValue("Global/password").toInt();
    _settingsReader->readSettingsValue("Global/secretPassword").toInt();
    _settingsReader->readSettingsValue("Global/passwordDiag").toInt();
    _settingsReader->readSettingsValue("Global/secretPasswordDiag").toInt();

    globals = new GlobalSettings(_settingsReader);
    logger = new Logger(nullptr);// инит логгера (черный ящик)

    //Инит CAN и GPIO
    can0 = new MyCan(can_device, logger, true, nullptr);//can0
    canForEngine = new MyCanEngine(j1939_device, logger, false, nullptr);
    canForEngine->setEngineAddr(globals->enigneAddr);
    canj1939 = new MyCanJ1939(j1939_device, logger, true, nullptr);
    engine = new Engine(canj1939, this);// создаем виджет двигателя
    canj1939Main = new MyCanJ1939(can_device, logger, false, nullptr);// камазовкий кан незя рестартить потому как он на таком же интерфейсе как и ПО ГО. А это опасно
    //gp = new gpio_class();
    gpio = new GPIOWorker();
    gpioMatirx = new GPIOMatrix();
    can = new CanController(can0);
    _gpio = new GPIOController(gpio, gpioMatirx);
    currentState = new CurrentState(_settingsReader, _gpio, can);
    view = new ViewController(this, logger);
    ui->logLayout->addWidget(view->getMessageList());
    starter = new StarterController(globals, currentState, engine, view->screenLog, can, _gpio, this);
    preroll = new PrerollController(globals, engine, view->screenLog, can, starter, currentState, view, this);

    registerPhysButtons();

    QApplication* a = qobject_cast<QApplication*>(QApplication::instance());

    a->setApplicationName("AutoCleaner_APPM");
    a->setApplicationVersion(programmVersionString);

    QCommandLineParser parser;// Парсер командной строки
    parser.setApplicationDescription("Запуск приложения с ключами");
    parser.addHelpOption();
    parser.addVersionOption();

    // Разбор аргументов
    parser.process(*a);

    //auto *cam = new VlcWidget("rtsp://admin:1234@192.168.0.10:554/stream1");
    //auto *cam = new VlcWidget_EGL("rtsp://192.168.0.12/");
    //cam->setMinimumSize(640, 360);
    //cam->show();

    ui->label_version->setText("Версия: " + programmVersionString);

    setDefaultWorkMode();
    qRegisterMetaType<struct can_frame>();

    loadAndSetFonts();// загружаем сторонние шрифты
    removeBadSettings();// надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231

    bool need_to_reconf = false;
    if (need_to_reconf)
        QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini");

//    maintenanceTracker = new MaintenanceTracker(_settingsReader, settingsStore);
//    maintenanceTracker->createRules();
//    maintenanceTracker->load();
    //insertValues();
    readValues();

    configureChannelTypes();
    configureFilters();
    configureButtons();

    addElement(StateValveA1, "Силовой клапан A1", 1, 0, OUT_MODE_NORMAL);
    addElement(StateValveF1, "(F1)Подъем отвала", 1, 1, OUT_MODE_NORMAL);
    addElement(StateValveF2, "(F2)Отжим щетки", 1, 2, OUT_MODE_NORMAL);
    addElement(StateValveF3, "(F3)Поворот щетки вправо", 1, 3, OUT_MODE_NORMAL);
    addElement(StateValveF4, "(F4)Опускание щетки", 1, 4, OUT_MODE_NORMAL);
    addElement(StateValveF6, "(F6)Поворот отвала вправо", 1, 5, OUT_MODE_NORMAL);
    addElement(StateValveF7, "(F7)Опускание отвала", 1, 6, OUT_MODE_NORMAL);
    addElement(StateValveF8, "(F8)Прижим щетки", 1, 7, OUT_MODE_NORMAL);
    addElement(StateValveF9, "(F9)Поворот щетки влево", 1, 8, OUT_MODE_NORMAL);
    addElement(StateValveF10, "(F10)Подъем щетки", 1, 9, OUT_MODE_NORMAL);
    addElement(StateValveF12, "(F12)Поворот отвала влево", 1, 10, OUT_MODE_NORMAL);
    addElement(StateValveE1, "(E1)Подъем воздуходувки", 1, 11, OUT_MODE_NORMAL);

    addElement(StateValveE2, "(E2)Подъем магнитной плиты", 2, 0, OUT_MODE_NORMAL);
    addElement(StateValveE3, "(E3)Направление воздуха влево", 2, 1, OUT_MODE_NORMAL);
    addElement(StateValveE5, "(E5)Опускание воздуходувки", 2, 2, OUT_MODE_NORMAL);
    addElement(StateValveE6, "(E6)Опускание магнитной плиты", 2, 3, OUT_MODE_NORMAL);
    addElement(StateValveE7, "(E7)Направление воздуха вправо", 2, 4, OUT_MODE_NORMAL);
    addElement(StateValveC1, "(C1)Плавающий режим щетка", 2, 5, OUT_MODE_NORMAL);
    addElement(StateValveC2, "(C2)Плавающий режим щетка", 2, 6, OUT_MODE_NORMAL);
    addElement(StateValveC3, "(C3)Плавающий режим отвал", 2, 7, OUT_MODE_NORMAL);
    addElement(StateValveC4, "(C4)Плавающий режим отвал", 2, 8, OUT_MODE_NORMAL);
    addElement(StateValveD1, "(D1)Вращение щетки", 2, 9, OUT_MODE_PFM, 0, 0, 50, 150, 10);
    addElement(StateValveD2, "(D2)Вращение щетки в обратную сторону", 2, 10, OUT_MODE_PFM, 0, 0, 50, 150, 10);
    addElement(StateValveD3, "(D3)Вращение вентилятора", 2, 11, OUT_MODE_PFM, 0, 0, 50, 150, fanAccelRate);

    addElement(StateDrainFilterD28, "Датчик сливного фильтра", 3, 0, IN_MODE_NORMAL);
    addElement(StatePressureFilter1, "Датчик напорного фильтра1", 3, 1, IN_MODE_NORMAL);
    addElement(StatePressureFilter2, "Датчик напорного фильтра2", 3, 2, IN_MODE_NORMAL);
    addElement(StateHydroTankLevelD27, "Датчик уровня гидробака", 3, 3, IN_MODE_NORMAL);
    addElement(StateHydraulicDistibutorPressure, "Давление гидросистемы распределителя 1", 3, 4, IN_MODE_ANALOG_8);
    addElement(StateHydraulicBroomPressure, "Давление гидросистемы контур вращения щетки", 3, 5, IN_MODE_ANALOG_8);
    addElement(StateHydraulicFanPressure, "Давление гидросистемы контур вращения вентилятора", 3, 6, IN_MODE_ANALOG_8);
    addElement(StateHydraulicBroomPressPressure, "Давление гидросистемы контура поджатия щетки", 3, 7, IN_MODE_ANALOG_8);
    addElement(StateHydraulicOilTemperature, "Датчик температуры гидросистемы", 3, 8, IN_MODE_ANALOG_8);
    addElement(StateDKPBackMagnetUp, "Датчик ДКП магнит верх", 3, 9, IN_MODE_NORMAL);
//    addElement(StateEngineReady, "Датчик готовности ДВС", 3, 10, IN_MODE_NORMAL);
    addElement(StateDKPBlowerUp1, "Датчик ДКП продувка верх 1", 3, 10, IN_MODE_NORMAL);
    addElement(StateDKPBlowerUp2, "Датчик ДКП продувка верх 2", 3, 11, IN_MODE_NORMAL);

    addElement(StateStarterAllow, "Разрешение стартер", 5, 0, OUT_MODE_NORMAL);
    addElement(StateStarterRoll, "Стартер прокрутка", 5, 1, OUT_MODE_NORMAL);
    addElement(State24Volt, "+24", 5, 2, OUT_MODE_NORMAL);

    addElement(StateWaterSensor, "Датчик воды", 6, 0, IN_MODE_NORMAL);
    addElement(StateRollIn, "Прокрутка вх.", 6, 1, IN_MODE_NORMAL);
    addElement(StateAirFilterBad, "Засор ВФ", 6, 2, IN_MODE_NORMAL);
    addElement(StateOilFilterBad, "Засор МФ", 6, 3, IN_MODE_NORMAL);
    addElement(StateOilRele, "Реле масла", 6, 4, IN_MODE_NORMAL);
    addElement(StateHeatRele, "Теплореле", 6, 5, IN_MODE_NORMAL);

    addElement(StateFRMBroomL1, "Свет щетка", 7, 0, OUT_MODE_NORMAL);
    addElement(StateFRMBackL2, "Свет зад", 7, 1, OUT_MODE_NORMAL);
    addElement(StateKungL5, "Свет кунг", 7, 2, OUT_MODE_NORMAL);
    addElement(StateSensorsPower, "Питание датчиков", 7, 3, OUT_MODE_NORMAL);
    addElement(StateHydraulicFan, "Вентилятор охлаждения гидросистемы ", 7, 4, OUT_MODE_NORMAL);

    addElement(StatePressureFilter3, "Датчик напорного фильтра3", 8, 0, IN_MODE_NORMAL);
    addElement(StateDKPDumpLeft, "Датчик ДКП отвал лево", 8, 4, IN_MODE_NORMAL);
    addElement(StateDKPDumpRight, "Датчик ДКП отвал право", 8, 5, IN_MODE_NORMAL);
    addElement(StateDKPDumpUp, "Датчик ДКП отвал верх", 8, 6, IN_MODE_NORMAL);
    addElement(StateDKPBroomLeft, "Датчик ДКП щетка лево", 8, 7, IN_MODE_NORMAL);
    addElement(StateDKPBroomRight, "Датчик ДКП щетка право", 8, 8, IN_MODE_NORMAL);
    addElement(StateDKPBroomUp, "Датчик ДКП щетка верх", 8, 9, IN_MODE_NORMAL);

    saveSystemConfigure();
    qDebug() << "can " << can_device << " " << j1939_device;

    ui->label_date->setFont(QFont("Mont",15));
    ui->label_time->setFont(QFont("Mont",20));

    currentState->setDiagMode(false);// режим опасной диагностики


    // закидываем настрокий конфигурацции для кана
    can0->fillSystemConfigure(&systemConfigure, &systemElements);
    logger->fillSystemConfigure(&can0->systemConfigure, &can0->systemElements);// тырим ее у кана потому что он расставляет важные параметры

    setDefaultValues();

    font.setFamily("Mont");
    font.setPointSize(16);
    serviceSetingsName->setFont(font);
    serviceSetingsName->setGeometry(12, 4, 359, 43);
    serviceSetingsName->hide();

    createFormsAndHide();

    createTimers();// создаем таймер для обслуживания общих узлов
    connect (&goHomeTimer, &QTimer::timeout, this, &MainWindow::resetDevices);

    // создаем виджеты щеток и прочих модулей
    broomCentral = new CentralBroom(can0, NULL, settings, view, this, this);
    frontRail = new FrontRail(can0, NULL, settings, view, this, this);
    backMagnet = new BackMagnet(can0, NULL, settings, view, this, this);
    blower = new Blower(can0, NULL, settings, view, this, this);
    resetDevices();
    //can0->setState(StateBoardsPowerOut, true);
    showWorkMode();

    starter->setIgnition(true);

    connect(can0, &MyCan::canPOError, this, &MainWindow::canPOError);
    connect(can0, &MyCan::canDataReady, this, &MainWindow::incomeData);
    connect(canj1939, &MyCanJ1939::canError, this, &MainWindow::canJ1939Error);
    connect(canj1939, &MyCanJ1939::canDataReadyJ1939, this,&MainWindow::incomeDataJ1939);
    connect(canj1939Main, &MyCanJ1939::canError, this, &MainWindow::canJ1939MainError);
    connect(canj1939Main, &MyCanJ1939::canDataReadyJ1939, this, &MainWindow::incomeDataJ1939Main);

//    connect(canj1939, &MyCanJ1939::canDataReadyJ1939(quint32, quint8, QByteArray)), this, SLOT(incomeDataJ1939(quint32, quint8, QByteArray)));
//    connect(canj1939Main, SIGNAL(canError()), this, SLOT(canJ1939MainError()));
//    connect(canj1939Main, SIGNAL(canDataReadyJ1939(quint32, quint8, QByteArray)), this, SLOT(incomeDataJ1939Main(quint32, quint8, QByteArray)));

    view->addLogWarning("ПВИ запущен");
    ui->label_engineLowTemperature->hide();
    ui->label_waterInFuel->hide();
    ui->label_airFilter->hide();
    ui->label_oilFilter->hide();

    can->setStarterAvailable(false);
    can->setRollStarter(false);
    //can0->setState(StateStarterAllow, false);
    //can0->setState(StateStarterRoll, false);
    createButtons();
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::createFormsAndHide(){
    serviceGPIOServiceIntervalLeftForm = new ServiceGPIOServiceIntervalLeftForm(this, this);
    serviceGPIOServiceIntervalLeftForm->hide();

    serviceOtherEngineLeftForm = new ServiceOtherEngineLeftForm(this);
    preroll->setEngineForm(serviceOtherEngineLeftForm);
    starter->setEngineForm(serviceOtherEngineLeftForm);
    serviceOtherEngineLeftForm->hide();

    serviceOtherLightLeftForm = new ServiceOtherLightLeftForm(can, this);
    serviceOtherLightLeftForm->hide();

    serviceDevicesHydraulicsLeftForm = new ServiceDevicesHydraulicsLeftForm(can, view, this);
    serviceDevicesHydraulicsLeftForm->hide();

    serviceDevicesDKPLeftForm = new ServiceDevicesDKPLeftForm(can, this);
    serviceDevicesDKPLeftForm->hide();

    serviceGlobalDateTimeLeftForm = new ServiceGlobalDateTimeLeftForm(this);
    serviceGlobalDateTimeLeftForm->hide();

    serviceGeneralPasswordLeftForm = new ServiceGeneralPasswordLeftForm(this, this);
    serviceGeneralPasswordLeftForm->hide();

    serviceMainRightForm = new ServiceMainRightForm(this, this);
    serviceMainRightForm->hide();

    settingsSettingsConfigurationLeftForm = new SettingsSettingsConfigurationLeftForm(can, this);
    settingsSettingsConfigurationLeftForm->hide();

    settingsWifiLeftForm = new SettingsWifiLeftForm(this);
    settingsWifiLeftForm->hide();

    settingsForm = new SettingsForm(settings, this);
    //settingsForm->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
    settingsForm->hide();
    settingsForm->fillElements();
    connect(settingsForm, &SettingsForm::closedAndSave, this, &MainWindow::settingsClosed);

    settingsMainRightForm = new SettingsMainRightForm(can0, settingsForm, this, this);
    settingsMainRightForm->hide();

    blockScreen = new BlockForm(this);
    blockScreen->hide();
}

void MainWindow::setDefaultValues(){
    starter->setDefaults();
    currentState->setDefaults();
    currentState->setMode(CurrentState::SweepMode);

    startClean = false;
    chooseFrm = false;
    backGearCounter = 0;
    backControl = false;
    backIdleCounter = 0;
    backLight = false;
    backBlockCounter = 0;
    backLightTimeCounter = 0;
    Password_accepted = false;
    engineTempCrit = false;
    engineTempWarn = false;
    engineTempWarnTimer = 0;
    speedCounter = 51;// до первых данных показываем n/a
    hydroTempCrit = false;
    hydroTempWarn = false;
    chooseGabaritCount = 0;
    centralBroomLeftTimeCounter = 0;
    centralBroomRightTimeCounter = 0;
    frontDumpLeftTimeCounter = 0;
    frontDumpRightTimeCounter = 0;
    blowerTimeCounter = 0;
    KVControl = false;
    currentKV = 0;

    serviceSetingsName = new QLabel(this);
    serviceSetingsName->setStyleSheet("color: white");

    stopInProgress = false;
    waitOnStartAlarmed = false;
    cleanWrongSpeedAlarmed = false;
    buttonsLightLevel = 0;
    pauseActive = false;

    globals->setDefaults();
}

void MainWindow::loadAndSetFonts(){
    QFontDatabase fontDB;
    fontDB.addApplicationFont(":/Images/Fonts/Montserrat.ttf");
    fontDB.addApplicationFont(":/Images/Fonts/ArialBlack.ttf");
    fontDB.addApplicationFont(":/Images/Fonts/calibri.ttf");
    fontDB.addApplicationFont(":/Images/Fonts/vemana2000.ttf");
    fontDB.addApplicationFont(":/Images/Fonts/RotondaBold.ttf");
    fontDB.addApplicationFont(":/Images/Fonts/Mont_Heavy.ttf");
    fontDB.addApplicationFont(":/Images/Fonts/CentSchbkCerill_BT.ttf");
    for(int i=0; i<fontDB.families().size(); i++)
    {
        qDebug() << fontDB.families().at(i);
    }
    // универсальный шрифт Liberation Sans
    QFont regular;
    //regular.setFamily("Liberation Sans");
    regular.setFamily("Montserrat");
    regular.setPointSize(10);
    QApplication::setFont(regular);

    // назначаем шрифты на экзотические места
    font.setFamily("Mont");
    font.setPointSize(13);

    font.setFamily("CentSchbkCyrill BT");
    font.setPointSize(31);
}

void MainWindow::setDefaultWorkMode(){
    workMode.centralBroomLeft = false;
    workMode.centralBroomRight = false;
    workMode.centralBroomFlow = false;
    workMode.centralBroomPress = false;
    workMode.blowLeft = false;
    workMode.blowRight = false;
    workMode.blowLifted = false;
    workMode.frontDumpLeft = false;
    workMode.frontDumpRight = false;
    workMode.frontDumpFlow = false;
    workMode.backMagnet = false;
    workMode.frmBroom = false;
    workMode.frmMagnet = false;
    workMode.frmKung = false;
    workMode.sweepType = LightSweep;
}

void MainWindow::setDefaultSettings(){}

void MainWindow::createButtons(){}

void MainWindow::configureChannelTypes(){
    systemConfigure.configurationVersion = 110;

    systemConfigure.boardsType[0] = BOARD_CP;
    systemConfigure.boardsType[1] = BOARD_OUT;

    systemConfigure.channelsType[1][0] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][1] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][2] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][3] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][4] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][5] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][6] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][7] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][8] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][9] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][10] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[1][11] = OUT_MODE_NORMAL;

    systemConfigure.boardsType[2] = BOARD_OUT;
    systemConfigure.channelsType[2][0] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[2][1] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[2][2] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[2][3] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[2][4] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[2][5] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[2][6] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[2][7] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[2][8] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[2][9] = OUT_MODE_PFM;
    systemConfigure.channelsType[2][10] = OUT_MODE_PFM;
    systemConfigure.channelsType[2][11] = OUT_MODE_PFM;

    systemConfigure.boardsType[3] = BOARD_IN_AN;
    systemConfigure.channelsType[3][0] = IN_MODE_NORMAL;
    systemConfigure.channelsType[3][1] = IN_MODE_NORMAL;
    systemConfigure.channelsType[3][2] = IN_MODE_NORMAL;
    systemConfigure.channelsType[3][3] = IN_MODE_NORMAL;
    systemConfigure.channelsType[3][4] = IN_MODE_ANALOG_8;
    systemConfigure.channelsType[3][5] = IN_MODE_ANALOG_8;
    systemConfigure.channelsType[3][6] = IN_MODE_ANALOG_8;
    systemConfigure.channelsType[3][7] = IN_MODE_ANALOG_8;
    systemConfigure.channelsType[3][8] = IN_MODE_ANALOG_8;
    systemConfigure.channelsType[3][9] = IN_MODE_NORMAL;
    systemConfigure.channelsType[3][10] = IN_MODE_NORMAL;
    systemConfigure.channelsType[3][11] = IN_MODE_NORMAL;

    systemConfigure.boardsType[4] = BOARD_UNKNOWN;
    systemConfigure.channelsType[4][0] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][1] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][2] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][3] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][4] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][5] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][6] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][7] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][8] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][9] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][10] = IN_MODE_NORMAL;
    systemConfigure.channelsType[4][11] = IN_MODE_NORMAL;

    systemConfigure.boardsType[5] = BOARD_OUT;
    systemConfigure.channelsType[5][0] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][1] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][2] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][3] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][4] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][5] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][6] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][7] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][8] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][9] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][10] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[5][11] = OUT_MODE_NORMAL;

    systemConfigure.boardsType[6] = BOARD_IN;
    systemConfigure.channelsType[6][0] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][1] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][2] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][3] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][4] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][5] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][6] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][7] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][8] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][9] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][10] = IN_MODE_NORMAL;
    systemConfigure.channelsType[6][11] = IN_MODE_NORMAL;


    systemConfigure.boardsType[7] = BOARD_OUT;
    systemConfigure.channelsType[7][0] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][1] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][2] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][3] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][4] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][5] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][6] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][7] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][8] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][9] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][10] = OUT_MODE_NORMAL;
    systemConfigure.channelsType[7][11] = OUT_MODE_NORMAL;

    systemConfigure.boardsType[8] = BOARD_IN;
    systemConfigure.channelsType[8][0] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][1] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][2] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][3] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][4] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][5] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][6] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][7] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][8] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][9] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][10] = IN_MODE_NORMAL;
    systemConfigure.channelsType[8][11] = IN_MODE_NORMAL;
}

void MainWindow::buttonsLightCheck(){
    if (buttonsLightLevel < buttonsLightLevelEdge) {buttonsLightLevel++;}
    if (buttonsLightLevel > buttonsLightLevelEdge && buttonsLightLevel > 0) {buttonsLightLevel--;}
    //PWMSend();
}

void MainWindow::addElement(int element_, QString name_, int board_, int channel_, quint8 type_, quint8 median_type_, quint8 pwm_type_, quint8 pfm_low_type_, quint8 pfm_high_type_, quint8 value_change_speed_){
    systemElements.insert(element_, new SystemElement(name_, board_, channel_));
    systemConfigure.id[board_][channel_] = element_;
    systemConfigure.channelsType[board_][channel_] = type_;
    systemConfigure.channelsMedianSize[board_][channel_] = median_type_;
    systemConfigure.channelsPWMSize[board_][channel_] = pwm_type_;
    systemConfigure.channelsLowPFM[board_][channel_] = pfm_low_type_;
    systemConfigure.channelsHighPFM[board_][channel_] = pfm_high_type_;
    systemConfigure.channelsValueChangeSpeed[board_][channel_] = value_change_speed_;
}

void MainWindow::getSystemConfigure(SystemConfigure* dst){
    //QMutexLocker l(&configureMutex);
    systemConfigure.copy(dst);
}

void MainWindow::getElements(QMap<int, SystemElement*>* dst){
    //QMutexLocker l(&configureMutex);
    foreach (int key, systemElements.keys()){
        dst->insert(key, new SystemElement(systemElements.value(key)));
    }
}

void MainWindow::readSystemConfigure(){
    saveSystemConfigure();

    // проверяем есть ли настройки в конфиге
    // bool need_to_save = false;
    // if (settings->contains("Configuration/configurationVersion"))
    // {// настройки есть
    //     qDebug() << "have config";
    //     if (settings->value("Configuration/configurationVersion").toUInt() >= systemConfigure.configurationVersion)
    //     {// версия их актуальна - перечитываем
    //         qDebug() << "version actual";
    //         systemConfigure.clear();
    //         while (systemElements.size() > 0)
    //             delete systemElements.take(systemElements.firstKey());
    //         systemConfigure.configurationVersion = settings->value("Configuration/configurationVersion").toUInt();
    //         for (int i = 1; i < 9; i++)
    //         {
    //             systemConfigure.boardsType[i] = settings->value("Configuration/Board" + QString::number(i) + "_boardType").toUInt();
    //             for (int k = 0; k < 12; k++)
    //             {
    //                 systemConfigure.channelsType[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "Type").toUInt();
    //                 systemConfigure.id[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "ElementId").toUInt();
    //                 systemConfigure.channelsMedianSize[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "MedianSize").toUInt();
    //                 systemConfigure.channelsLowPFM[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "LowPFM").toUInt();
    //                 systemConfigure.channelsHighPFM[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "HighPFM").toUInt();
    //                 systemConfigure.channelsPWMSize[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "PWMSize").toUInt();
    //                 systemConfigure.channelsValueChangeSpeed[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "ValueChangeSpeed").toUInt();

    //                 if (systemConfigure.id[i][k] != 0)
    //                 {
    //                     addElement(settings->value("Configuration/Element" + QString::number(systemConfigure.id[i][k]) + "_id").toUInt(),
    //                                settings->value("Configuration/Element" + QString::number(systemConfigure.id[i][k]) + "_name").toString(),
    //                                settings->value("Configuration/Element" + QString::number(systemConfigure.id[i][k]) + "_board").toUInt(),
    //                                settings->value("Configuration/Element" + QString::number(systemConfigure.id[i][k]) + "_channel").toUInt(),
    //                                systemConfigure.channelsType[i][k],
    //                                systemConfigure.channelsMedianSize[i][k],
    //                                systemConfigure.channelsPWMSize[i][k],
    //                                systemConfigure.channelsLowPFM[i][k],
    //                                systemConfigure.channelsHighPFM[i][k],
    //                                systemConfigure.channelsValueChangeSpeed[i][k]);
    //                 }
    //             }
    //         }
    //     }
    //     else
    //     {
    //         qDebug() << "version old";
    //         need_to_save = true;
    //     }
    // }
    // else
    // {
    //     qDebug() << "have NO config";
    //     need_to_save = true;
    // }
    // if (need_to_save)
    // {// конфига или нет или он старый - пересохраняем
    //     saveSystemConfigure();
    // }
}

void MainWindow::removeBadSettings(){
    QDir dir(QCoreApplication::applicationDirPath(), {"settingsAutoCleaner.ini.*"});
    for(const QString & filename: dir.entryList())
        dir.remove(filename);
}

void MainWindow::saveSystemConfigure(){
    settingsStore->commitGroup("Configuration", [this](QSettings *s){
        s->remove(""); // почистим всё конфиговое
        s->setValue("configurationVersion", systemConfigure.configurationVersion);

        for (int i = 1; i < 9; i++)
        {
            s->setValue("Board" + QString::number(i) + "_boardType", systemConfigure.boardsType[i]);
            for (int k = 0; k < 12; k++)
            {
                const QString basePath = "Board" + QString::number(i) + "_channel" + QString::number(k);
                s->setValue(basePath + "Type", systemConfigure.channelsType[i][k]);
                s->setValue(basePath + "ElementId", systemConfigure.id[i][k]);
                s->setValue(basePath + "MedianSize", systemConfigure.channelsMedianSize[i][k]);
                s->setValue(basePath + "LowPFM", systemConfigure.channelsLowPFM[i][k]);
                s->setValue(basePath + "HighPFM", systemConfigure.channelsHighPFM[i][k]);
                s->setValue(basePath + "PWMSize", systemConfigure.channelsPWMSize[i][k]);
                s->setValue(basePath + "ValueChangeSpeed", systemConfigure.channelsValueChangeSpeed[i][k]);
            }
        }

        foreach (int key, systemElements.keys())
        {
            const QString basePath = "Element" + QString::number(key);
            s->setValue(basePath + "_id", key);
            s->setValue(basePath + "_name", systemElements.value(key)->name);
            s->setValue(basePath + "_board", systemElements.value(key)->board);
            s->setValue(basePath + "_channel", systemElements.value(key)->channel);
        }
    });
}

void MainWindow::readValues(){// у каждого модуля есть своя функиция чтения настроек. Настройки которые неподвластны каким то модулям зачитываются тут
    globals->readValues();
    currentState ->readValues();

    cleanConfiguration.frontDumpUse = _settingsReader->readSettingsValue("CleanConfiguration/frontDumpUse").toBool();
    cleanConfiguration.magnetUse = _settingsReader->readSettingsValue("CleanConfiguration/magnetUse").toBool();
    cleanConfiguration.centralBroomUse = _settingsReader->readSettingsValue("CleanConfiguration/centralBroomUse").toBool();
    cleanConfiguration.blowUse = _settingsReader->readSettingsValue("CleanConfiguration/blowUse").toBool();

    showCheckEngine = _settingsReader->readSettingsValue("Global/showCheckEngine").toBool();// сознаваться ли про чек энжын?
    buttonsLightLevelEdge = _settingsReader->readSettingsValue("Global/buttonsLightLevelEdge").toInt();
   // buttonsLightLevelEdge = 10; // TODO ????
    maintenanceTracker->load();

    engineTempWarnEdge = _settingsReader->readSettingsValue("Global/engineTempWarnTime").toInt();
    engineTempGoodValue = _settingsReader->readSettingsValue("Global/engineTempGood").toInt();
    engineTempWarnValue = _settingsReader->readSettingsValue("Global/engineTempWarn").toInt();
    engineTempCritValue = _settingsReader->readSettingsValue("Global/engineTempCrit").toInt();

    hydroTempGoodValue = _settingsReader->readSettingsValue("Global/hydroTempGood").toInt();
    hydroTempWarnValue = _settingsReader->readSettingsValue("Global/hydroTempWarn").toInt();
    hydroTempCritValue = _settingsReader->readSettingsValue("Global/hydroTempCrit").toInt();

    hydroTempK = _settingsReader->readSettingsValue("Global/hydroTempK").toFloat();
    hydroTempB = _settingsReader->readSettingsValue("Global/hydroTempB").toFloat();

    fanAccelRate = _settingsReader->readSettingsValue("Blower/fanAccelRate").toInt();
    addElement(StateValveD3, "(D3)Вращение вентилятора", 2, 11, OUT_MODE_PFM, 0, 0, 50, 150, fanAccelRate);

    waterSensorEmergencyMode = _settingsReader->readSettingsValue("Engine/waterSensorEmergencyMode").toBool();
    airFilterEmergencyMode = _settingsReader->readSettingsValue("Engine/airFilterEmergencyMode").toBool();
    //disableRollRequirement = _settingsReader->readSettingsValue("Engine/disableRollRequirement").toBool();
    disableTemperatureBlock = _settingsReader->readSettingsValue("Engine/disableTemperatureBlock").toBool();
    ignoreAllEmergency = _settingsReader->readSettingsValue("Engine/ignoreAllEmergency").toBool();

    for (int index = 0; index < 4; ++index){
        const QString suffix = QString::number(index + 1);
        hydraulicPressureK[index] = _settingsReader->readSettingsValue("Global/hydraulicPressure" + suffix + "K").toFloat();
        hydraulicPressureB[index] = _settingsReader->readSettingsValue("Global/hydraulicPressure" + suffix + "B").toFloat();
    }
}

void MainWindow::resetDevices(){
    //test
    if (DEVELOPER_MODE || blockScreen->developerMode){
        if (blockScreen->isVisible())
            blockScreen->hide();
        emit resetComplete();
        return;
    }

    // тут надо перейти в домашнее состояние из текущего
    // текущее состояние определяется последними данными полученными по CAN
    // применим стандартную последовательность с установленными таймаутами для перехода из состояния
    // на время перехода появляется стандартное блокирующее окно с текстом
    blockScreen->raise();
    if (ui->POStatus->isVisible())
    {// связи с коробками нет - надо показать страшные тексты и заблокировать все до появления связи а потом безопасно вернуть органы в домашнее состояние
        //        last0CA0A100.can_id = 0;
        //        last0CC0A100.can_id = 0;
        //        last0CC0A200.can_id = 0;
        qDebug() << "resetDevices";
        blockScreen->label_blockScreenText->setText("Нет связи с блоками управления. Пожалуйста подождите");
        blockScreen->show();
        goHomeTimer.setInterval(1000);
        goHomeTimer.start();
        // выключаем режим автоматики
        if (startClean){
            view->addLogWarning("Связь с блоками потеряна - остановка уборки");
            on_pushButton_startstop_clicked();
        }
        return;
    }
    // Если хоть каких то данных нет, то считаем что переход в домашнее состояние перешел прекрасно
    if (can0->isActive())//can0->last0CA0A100.can_id == 0x0CA0A100 && can0->last0CC0A100.can_id == 0x0CC0A100 && can0->last0CC0A200.can_id == 0x0CC0A200)
    {
        // последовательно переводим модули в нужное состояние. выполнениек блокирующее - считаем что эта процедура самая важная и обратная связь не важна
        // по сути конфликтным состоянием является только центральная щетка - при повороте и опускании она может что то задеть
        // поэтому проверяем только опускание и попворот центральной (после поворота и поднятия просто все выставляем в 0 без таймаута)
        // при повороте и поднятии лотковые щетки должны быть открыты (распределители не важны)
    }
    else{// не можем себе позволить кошерно сбросить состояния пока блоки не подключились
        blockScreen->label_blockScreenText->setText("Переход в домашнее состояние, ожидание данных от блоков управления. Пожалуйста подождите");
        blockScreen->show();
        goHomeTimer.setInterval(1000);
        goHomeTimer.start();
        return;
    }
    // Втягиваем все органы - конфиликтов нет (reset devices)
    //выставляем статусы устройств
    // скомандуем свернуть все

    broomCentral->setState(CentralBroom::BroomOff);
    backMagnet->setState(BackMagnet::BackMagnetOff);
    frontRail->setState(FrontRail::FrontRailOff);
    blower->setState(Blower::BlowerOff);

    // выставляем в 0 команды
    for (int i = 0;i < 8; i++){
        canj1939->setTsc1Byte(0, i);
    }
    goHomeTimer.stop();
    goHomeTimer.setInterval(1000);
    blockScreen->hide();
    emit resetComplete();
}

void MainWindow::canPOError(){
    if (!ui->POStatus->isVisible()){
        qDebug() << "POError";
        ui->POStatus->show();}

    resetDevices();//сбросить все команды и состояния
}

void MainWindow::incomeData(struct can_frame frame){
    if (frame.can_id == 0x00000AC0)// && (centralBroom->getState() == CentralBroom::BroomRotated || serviceForm->isVisible()))
    {// КВ
        if (frame.data[3] & 0x10 && (frame.data[3] & ~(0x10)) && workMode.centralBroomPress){// назад
            broomCentral->goPressUp(true);
            KVControl = true;

            currentKV = 0;
        }
        else if ((frame.data[3] & 0x10) == 0 && frame.data[3] && workMode.centralBroomPress){// вперед
            broomCentral->goPressDown(true);
            KVControl = true;
        }
        else if (KVControl){
            broomCentral->stopPress();
            KVControl = false;
        }
    }
}

void MainWindow::canJ1939Error(){
    qDebug()<<"canStatusError";
    if (!ui->J1939Status->isVisible())
        ui->J1939Status->show();
    // сбрасываем значения
    engine->resetValues();
}

void MainWindow::canJ1939MainError(){
    qDebug()<<"canMainError";
    if (!ui->J1939MainStatus->isVisible())
        ui->J1939MainStatus->show();
    currentState->resetVehicleValues();
}

void MainWindow::incomeDataJ1939(quint32 pgn, quint8 sa, QByteArray data){
    if (ui->J1939Status->isVisible())
        ui->J1939Status->hide();
}

void MainWindow::incomeDataJ1939Main(quint32 pgn, quint8 sa, QByteArray data){// данные от переднего двигателя
    if (pgn == 0xF004){//обороты переднего двигателя //eec1
        quint16 rpm_ = data[3] + (data[4] << 8);
    }
    // лучше брать FEF1 (похоже на круиз) или ETC1(F002) (частота вала) или FEBF - скорее всего abs (FE6C это скорость от тахографа а у нас ее нет похоже)

    if (pgn == 0xFEBF){// скорость по ABS
        QString temp = "%1";
        if ((uint8_t)data[1] < 200)
        {
            temp = temp.arg(data[1], 2, 10, QChar('0'));
            currentState->setVehicleSpeed(data[1]);
            //vehicleSpeed = data[1];
        }
        speedCounter = 0;
    }

    if (pgn == 0xFEEE){ //et1
        currentState->setCoolantTmp(data[0] - 40);//engineCoolantTemp = data[0] - 40;
    }

    if (pgn == 0xF002){// скорость выходного вала трансмиссии
        quint16 speed_ = data[1] + (data[2] << 8);
        //        qDebug() << "speed 1 " << ((float)speed_ * 0.126092385);
        //        QString temp = "%1";
        //        temp = temp.arg((int)((float)speed_ * 0.126092385), 2, 10, QChar('0'));
        //        ui->label_speed->setText(temp);
    }
    if (pgn == 0xFEF1){// скорость по круизу
        //        qDebug() << "speed 2 " << (data[2]);
        //        QString temp = "%1";
        //        temp = temp.arg(data[2], 2, 10, QChar('0'));
        //        ui->label_speed->setText(temp);
    }
    if (pgn == 0xFE6C){// скорость по тахографу
        quint16 speed_ = data[6] + (data[7] << 8);
        //        qDebug() << "speed 3 " << (speed_ / 256);
        //        QString temp = "%1";
        //        temp = temp.arg((int)((float)speed_ / 256), 2, 10, QChar('0'));

        //        ui->label_speed->setText(temp);
    }

    if (pgn == 0xFEF7){// напряжение АКБ
        quint16 volt_ = data[4] + (data[5] << 8);
        //        qDebug() << "volt " << (volt_ / 20);
        QString temp = "%1";
        temp = temp.arg((int)((float)volt_ / 20), 2, 10, QChar('0'));
        currentState->setVehicleVoltage ((float)volt_ / 20);
        voltageCounter = 0;
    }

    if (ui->J1939MainStatus->isVisible())
        ui->J1939MainStatus->hide();
}

void MainWindow::oneSecond(){// универсальный таймер для всяких нужд (раз в сек)
    can0->setState(State24Volt, true);

    QDateTime DateAndTime = QDateTime::currentDateTime().addMonths(0);
    auto date = DateAndTime.date();
    QString string_current_date = EngLocale.dayName(date.dayOfWeek(),QLocale::ShortFormat);
    string_current_date += " "+ QString::number(date.day(), 10);
    string_current_date += " "+ EngLocale.monthName(date.month(),QLocale::ShortFormat);
    string_current_date += " "+ QString::number(date.year(), 10);
    ui->label_date->setText(string_current_date);
    ui->label_time->setText(DateAndTime.time().toString());

    // проверка температуры ПВИ
//    QFile f("/sys/devices/virtual/thermal/thermal_zone0/temp");
//    if (f.open(QIODevice::ReadOnly))
//    {
//        QByteArray tt = f.readAll();
//        qint8 tempLevel = QString::fromLocal8Bit(tt).toInt() / 1000;
//        f.close();
//        if (tempLevel <= PVI_TEMP_EDGE_OFF)
//            gp->Set_GPIO_State(OUT_PVI_TEMP, 0);
//        if (tempLevel >= PVI_TEMP_EDGE_ON)
//            gp->Set_GPIO_State(OUT_PVI_TEMP, 1);
//        //qDebug() << "current temp level:" << tempLevel;
//    }

    if (engine->getRpm() > 700)
        TOCurValues["Engine"]++;
    if (startClean)
        TOCurValues["System"]++;

    const bool engineRunningNow = engine->getRpm() > 700;
    if (engineRunningNow && !engineRunStatePrev){
        qDebug()<<"!!! setCurrentData";
        currentState->updateStartDate();
        starter->resetValues();
        preroll->resetValues();
    }

    engineRunStatePrev = engineRunningNow;

    if (engineTempWarnTimer > 0)
        engineTempWarnTimer--;
    checkEngineOverheat();

    // узнаем моточасы за сегодня
    if (DateAndTime.date() != dateToday){// надо записать сегодняшний срез и сохранить его
        dateToday = date;
        engineToday = TOCurValues["Engine"];
        settingsStore->commitGroup("TOCur", [this](QSettings *s){
            s->setValue("EngineToday", engineToday);
            s->setValue("DateToday", dateToday);
        });
    }
    // отображаем моточасы
    QString temp = "%1";
    temp = temp.arg(TOCurValues["Engine"] / 3600, 5, 10, QChar('0'));
    view->setText(ui->label_frontEngineTOTotalValue, temp);
    temp = "%1";
    temp = temp.arg((TOCurValues["Engine"] - engineToday) / 3600, 2, 10, QChar('0'));
    view->setText(ui->label_engineTODailyValue, temp);
    if (TOCurValues["Engine"] - TOCurValues["EngineLast"] > 5 * 60) {// пора сохранить кой какие данные каждые 5 минут
        TOCurValues["EngineLast"] = TOCurValues["Engine"];

        settingsStore->commitGroup("TOCur", [this](QSettings *s){
            s->setValue("Engine", TOCurValues["Engine"]);
        });
    }
    if (TOCurValues["System"] - TOCurValues["SystemLast"] > 5 * 60){// пора сохранить кой какие данные каждые 5 минут
        TOCurValues["SystemLast"] = TOCurValues["System"];
        // сохраняем настройки
        // перед этим удаляем lock файл - были случаи что lock файл блокировал запись настроек

        settingsStore->commitGroup("TOCur", [this](QSettings *s){
            s->setValue("System", TOCurValues["System"]);
        });
    }

    bool to_test = false;
    foreach (QString key, TONameValues.keys()){
        quint32 compare = TOCurValues["Engine"];// системный счетчик по двигателю
        if (TOSourceValues[key] == 1)
            compare = TOCurValues["System"];
        if (compare - TOCurValues[key] > TOValues[key]){
            to_test = true;
            if (TOAlarmValues[key] != 1){
                TOAlarmValues[key] = 1;
                view->addLogWarning(TONameValues[key] + " требует ТО");
            }
        }
        else
            TOAlarmValues[key] = 0;
    }
    // отображаем знак ТО
    if (to_test && !ui->label_TO->isVisible()){
        ui->label_TO->show();
    }
    else if (!to_test && ui->label_TO->isVisible()){
        ui->label_TO->hide();
    }
    //qDebug() << "!!! can0->isActive: "<<can0->isActive();
    if (can0->isActive()){
        if (ui->POStatus->isVisible())
            ui->POStatus->hide();
    }

    if (can0->isActive()){
        // получим температуру гидрооборудования
        const qint16 hydro_temp = hydroTempK * can->getOilTmp() + hydroTempB;
        qint16 filtered;
        if (hydroTempFilter.process(hydro_temp, filtered)){
            ui->label_hydraulicTemperature->setText(QString::number(filtered));
            checkHydraulicOverheat(filtered);
        }
    }
}

void MainWindow::checkHydraulicOverheat(qint16 temp){// проверка перегрева гидросистемы
    if (hydroTempK == 0 || hydroTempWarnValue <= 0 || hydroTempCritValue <= 0)
        return;// датчик не откалиброван или пороги не заданы

    if (temp > hydroTempCritValue){
        if (!hydroTempCrit)
            view->addLog("Гидросистема перегрелась!!!", ViewController::FatalStatus);
        hydroTempCrit = true;
        hydroTempWarn = true;
    }
    else if (temp > hydroTempWarnValue){
        hydroTempCrit = false;
        if (!hydroTempWarn)
            view->addLogWarning("Гидросистема перегревается");
        hydroTempWarn = true;
    }
    else{
        hydroTempCrit = false;
        hydroTempWarn = false;
    }
}

void MainWindow::repaintProgress(){
    // положение обдува меняется само по себе - обновляем иконку, если кнопки вверх/вниз не нажаты
    if (!ui->pushButton_blowerUp->property("wasDown").toBool() && !ui->pushButton_blowerDown->property("wasDown").toBool())
        view->setStyle(ui->label_blowerUpDown, getBlowerVertIcon());

    QString text = QString::number(hydroTempK * can->getOilTmp() + hydroTempB, 'f', 1);
    text = QString::number(hydraulicPressureValue(2), 'f', 1);
    if (ui->label_fan_pressure->text() != text + " P ТИ3")
        ui->label_fan_pressure->setText(text + " P ТИ3");
    text = QString::number(hydraulicPressureValue(3), 'f', 1);
    if (ui->label_roll_pressure->text() != text + " P ТИ4")
        ui->label_roll_pressure->setText(text + " P ТИ4");
    ui->label_time->repaint();// защита от залипания графики
}

bool MainWindow::inHomeState(){
    if ((frontRail->getState() == FrontRail::FrontRailOff || frontRail->railAlarmed)
            && (backMagnet->getState() == BackMagnet::BackMagnetOff || backMagnet->magnetAlarmed)
            && (broomCentral->getState() == CentralBroom::BroomOff || broomCentral->broomAlarmed)
            && (blower->getState() == Blower::BlowerOff || blower->blowerAlarmed))
        return true;
    return false;
}

// тут проверяются узлы которые являются общими для всех (например насос воды используется 8 блоками, поэтому тут проверяем если он долго никому не нужен то выключаем воду)
void MainWindow::checkIgnition(){
    if (!currentState->isIgnitionEnabled()){//can->isDisabled()){// вырубили зажигание-надо готовиться к остановке (или нажали кнопку пви)
        // сделаем флуш всего
        if (!stopInProgress){
            stopInProgress = true;
            if (!can->isBoard0IN())
                view->addLogWarning("Ключ зажигания повернут. Начинается выключение ПВИ");
            else
                view->addLogWarning("Нажали кнопку выключения ПВИ. Начинается выключение ПВИ ( удерживайте кнопку )");
            logger->setStop(true);
            //сохраним важные параметры

            settingsStore->commitGroup("TOCur", [this](QSettings *s){
                s->setValue("System", TOCurValues["System"]);
                s->setValue("Engine", TOCurValues["Engine"]);
                s->setValue("FrontEngine", TOCurValues["FrontEngine"]);
            });
        }
    }
    else{
        if (stopInProgress)
            view->addLogWarning("Выключение ПВИ отменено");
        stopInProgress = false;
    }
}

void MainWindow::mainProgress(){
    updatePhysButtons();
    updateButtonsUniversal();

    buttonsLightCheck();// проверка подсветки

    // test
    ui->label_key->setText("Кнопка: " + QString::number((int)gpioMatirx->keyPressed));

    checkIgnition();
    checkEngineAndRollLocks();

    preroll->update();

    if (currentState->isDiagMode()){
        updateSensorAndWarningIndicators();
        serviceDevicesHydraulicsLeftForm->updateVisual();
        serviceDevicesDKPLeftForm->updateVisual();
        serviceOtherEngineLeftForm->updateVisual();
        serviceOtherLightLeftForm->updateVisual();

        return; // ни чем не управляем пока включена диагностика!!!
    }
    if (currentState->isSettingsMode()){//menuMode == SettingsMode){
        settingsSettingsConfigurationLeftForm->updateVisual();
    }

    // проверка существованиЯ параментров от переднего двигателя
    speedCounter++;
    voltageCounter++;
    frontRPMCounter++;
    if (frontRPMCounter > 50){
        frontRPM = 0;
    }
    if (speedCounter > 50)// нет данных о скорости дольше 5 с
        view->setText(ui->label_speed, "n/a");
    else
        view->setText(ui->label_speed, QString::number(currentState->vehicleSpeed));
    if (engine->online > ENGINE_ONLINE_EDGE * 10){
        view->setText(ui->label_engineTemp, "n/a");
        view->setText(ui->label_engineRPM, "n/a");
    }
    else{
        view->setText(ui->label_engineTemp, QString::number(engine->engineCoolantTemp));
        view->setText(ui->label_engineRPM, QString::number(engine->getRpm()));
    }

    if (!currentState->isDiagOrSettingsMode()){// в режиме диагностики не умничаем, в остальных случаях пробуем понять что сейчас не используется и выключить это
        if (!startClean && inHomeState() && !serviceMainRightForm->isVisible()){
            // а так же вырубим коробки отбора мощности
            //if (!gp->GPIO[IN_LEFT_DOWN] && !gp->GPIO[IN_LEFT_UP] && !KVControl && !gp->GPIO[IN_RIGHT_DOWN] && !gp->GPIO[IN_RIGHT_UP])
            if (!KVControl){// только если не заняты работой от кнопок с пульта
                can0->setState(StateValveA1, false);
            }
        }
        // доабвил с аэродрома опасно

        if (!canStart() && ui->pushButton_startstop->isEnabled()){// серим старт
            //pushbuttonStartStopEffect->setOpacity(0.2);
            ui->pushButton_startstop->setEnabled(false);
        }
        else if (canStart() && !ui->pushButton_startstop->isEnabled()){
            //pushbuttonStartStopEffect->setOpacity(1.0);
            if (isIdleMode())//
                ui->pushButton_startstop->setEnabled(true);
        }

        if (blower->getState() <= Blower::BlowerStates::BlowerDowned
                && broomCentral->getState() <= CentralBroom::BroomStates::BroomDowned){// если модули не крутятся - выключаем распределитель и убавляем обороты
            // устанавливыаем обороты чтобы двигло зря не работал
            //quint16 rpm = rpmNone * 8;
            canForEngine->setEngineCommand(globals->getRpm());
            //  вроде никто не работет и наверное никому не пригодится распределитель бункера
            //can0->setState(StateValveA1, false);
        }
    }


    checkAndShowStatus();// отрисуем в статусной строке общие параметры (насосы, распределители и пр.)


    // цикл обслуживания командных gpio
    // смотрим нажат ли кто и реагируем соответствующе
    // проверка отключения зажигания (еасли нажали кнопку на пульте)
    showPultOffIgnition();
    showStartClean();
    showModeButton();
    showMatrixFRMButton();

    starter->showStarter();// работа со стартером
    updateFRM();// свет

    // отображаем и отрабатываем нажатие кнопок на экране во время работ
    bool transitioning = isOrgansTransitioning();
    if (!transitioning){
        if (organsWereTransitioning && !pauseActive){
            if (startClean)
                view->addLogWarning("Органы разложены — управление разблокировано");
            else
                view->addLogWarning("Органы сложены — можно начинать движение");
        }
        // алиасы
        // щетки  - при неактивной программе выбирают левую правую щетку. при активной программе меняют обороты щетки
        // if (startClean){
        //     setBroomState();
        // }

        //updateCentralBroomLeft();
        //updateCentralBroomRight();
        // отвал - при неактивной проге выбирают лево право отвал. при активной двигает отвалом (при удержании isDown)
        //showDumpLeft();
        //showDumpRight();
        // продувка - при неактивной прое выбирает обдув лево право. в активной проге ничего не делает - опасно
        //showBlower();
    }
    organsWereTransitioning = transitioning;
    showPauseButton();

    // если нажата аварийка или грибок питания то завершаем все
    if (can->getAlarm() && startClean){
        if (can->getAlarm())
            view->addLogWarning("Нажата аварийная кнопка");
        if (startClean)
            on_pushButton_startstop_clicked();
        starter->setIgnition(false);
        starter->resetIgnitionTimer();
    }

    //защита по скорости - если едем слишком быстро надо выключать режим работы (скорость 50 условная - обозначает что нет данных от двигателя)
    if (isSpeedTooHigh()){
        stopCleaningForSafety("Превышена скорость уборки. Уборка остановлена");
    }
}

void MainWindow::stopCleaningForSafety(const QString &reason){// аварийная остановка уборки, в том числе из паузы
    if (!startClean)
        return;
    pauseActive = false;
    view->addLogError(reason);
    startCleaning(false);
    showWorkMode();
}

void MainWindow::checkEngineOverheat(){// проверка перегрева двигателя (раз в секунду)
    const bool engineTempValid = engine->coolantTempEverReceived && (engine->online <= ENGINE_ONLINE_EDGE * 10);
    if (!engineTempValid || engineTempWarnValue <= 0 || engineTempCritValue <= 0)
        return;// нет свежих данных (например, зажигание выключено) или пороги не заданы - состояние не меняем

    const int temp = engine->engineCoolantTemp;

    if (temp > engineTempCritValue){
        if (!engineTempCrit){
            view->addLog("Двигатель перегрелся!!! Зажигание выключено", ViewController::FatalStatus);
            starter->forceStopIgnition();
        }
        engineTempCrit = true;
        engineTempWarn = true;
        stopCleaningForSafety("Перегрев двигателя. Уборка остановлена");
        return;
    }

    if (temp > engineTempWarnValue){
        if (!engineTempWarn){
            view->addLogError("Двигатель перегревается. Ожидайте охлаждения");
            engineTempWarnTimer = engineTempWarnEdge * 60;// столько секунд ждём, прежде чем заглушить двигатель
        }
        engineTempWarn = true;
        stopCleaningForSafety("Перегрев двигателя. Уборка остановлена");

        if (engine->getRpm() > 700 && engineTempWarnTimer == 0 && !engineTempCrit){// всё ещё перегрет
            view->addLog("Двигатель не смог охладиться!!! Зажигание выключено", ViewController::FatalStatus);
            engineTempCrit = true;
            starter->forceStopIgnition();
        }
        return;
    }

    if (engineTempWarn || engineTempCrit)
        view->addLog("Температура двигателя в норме");
    engineTempWarnTimer = 0;
    engineTempWarn = false;
    engineTempCrit = false;
}


void MainWindow::updateIndicatorPixmap(QLabel* label, const QString& colorName, const QString& baseName){
    const QString iconPath = ":/Images/Images/main/signs/sign_" + baseName + "_" + colorName + "_stub.png";
    label->setPixmap(QPixmap(iconPath));
}

void MainWindow::checkEngineAndRollLocks(){

    const bool isEngineWarmEnough = can->getHeatState();// Теплореле: активно (true) = двигатель достаточно прогрет
    const bool engineTempValid = engine->coolantTempEverReceived && (engine->online <= ENGINE_ONLINE_EDGE * 10);// Температура учитывается только при живом CAN
    const bool engineCold = engineTempValid && (globals->isEngineCold(engine->engineCoolantTemp));// Температура двигателя: учитываем только если движок уже хоть раз прислал данные
    const int daysFromLastStart = currentState->getDaysFromStart();

    preroll->checkIfAwaitForRoll(daysFromLastStart);

    // Блокировка по температуре: если двигатель холодный И теплореле ещё не сработало
    // Если при включении температура уже удовлетворительна — блокировки нет
    // === ИСПРАВЛЕННАЯ ЛОГИКА ТЕМПЕРАТУРНОЙ БЛОКИРОВКИ ===
    if (!disableTemperatureBlock){
        if (engineCold && !isEngineWarmEnough){// БЛОКИРОВКА: двигатель холоден по CAN и теплореле ещё не замкнуто
            preroll->lockByTemperature(true);}
        else if (isEngineWarmEnough){// РАЗБЛОКИРОВКА: ТОЛЬКО когда сработало физическое теплореле
            preroll->lockByTemperature(false);}
        else{// РАЗБЛОКИРОВКА: данные с CAN недостоверны (пропал) или двигатель уже прогрет
            preroll->lockByTemperature(false);}
        // Если по CAN уже "тепло", но теплореле ещё не замкнулось —
        // блокировка остаётся висеть (не сбрасываем здесь!)
    }
    else{
        preroll->lockByTemperature(false);}
    // === КОНЕЦ ИСПРАВЛЕНИЯ ===

    preroll->checkEmergencies();
}

void MainWindow::updateSensorAndWarningIndicators(){
    const bool waterSensor = can0->getState(StateWaterSensor).toBool();
    const bool airFilter = can0->getState(StateAirFilterBad).toBool();
    const bool oilFilter = can0->getState(StateOilFilterBad).toBool();
    const bool heatRelay = !can0->getState(StateHeatRele).toBool();
    const bool lowTemperature = engine->online <= ENGINE_ONLINE_EDGE * 10 && engine->engineCoolantTemp < globals->lowTempRequireWarm;

    m_waterSensorWatcher.update(waterSensor);
    m_airFilterWatcher.update(airFilter);
    m_oilFilterWatcher.update(oilFilter);
    m_heatRelayWatcher.update(heatRelay || lowTemperature);
}

void MainWindow::updateButtonsUniversal(){
    //обновляем состояние всех кнопок: получаем правильное с антидребезгом состояние физ. кнопок и состояние кнопок на экране
    //-----------------------------------щётка------------------------------------------
    m_broomUpWatcher.update(ui->pushButton_centralBroomUp->isEnabled() && (ui->pushButton_centralBroomUp->isDown() || m_buttonManager.isPressed(GPIOInput::IN_BROOM_UP)));
    m_broomDownWatcher.update(ui->pushButton_centralBroomDown->isEnabled() && (ui->pushButton_centralBroomDown->isDown() || m_buttonManager.isPressed(GPIOInput::IN_BROOM_DOWN)));
    m_broomLeftWatcher.update(ui->pushButton_centralBroomLeft->isDown() || m_buttonManager.isPressed(GPIOInput::IN_BROOM_LEFT));
    m_broomRightWatcher.update(ui->pushButton_centralBroomRight->isDown() || m_buttonManager.isPressed(GPIOInput::IN_BROOM_RIGHT));

    m_broomFlowWatcher.update(ui->pushButton_centralBroomFlow->isDown());
    m_broomPressWatcher.update(ui->pushButton_centralBroomPress->isDown());
    //-----------------------------------отвал------------------------------------------
    m_dumpUpWatcher.update(ui->pushButton_dumpUp->isEnabled() && (ui->pushButton_dumpUp->isDown() || m_buttonManager.isPressed(GPIOInput::IN_DUMP_UP)));
    m_dumpDownWatcher.update(ui->pushButton_dumpDown->isEnabled() && (ui->pushButton_dumpDown->isDown() || m_buttonManager.isPressed(GPIOInput::IN_DUMP_DOWN)));
    m_dumpLeftWatcher.update(ui->pushButton_dumpLeft->isDown() || m_buttonManager.isPressed(GPIOInput::IN_DUMP_LEFT));
    m_dumpRightWatcher.update(ui->pushButton_dumpRight->isDown() || m_buttonManager.isPressed(GPIOInput::IN_DUMP_RIGHT));

    m_dumpFlowWatcher.update(ui->pushButton_dumpFlow->isDown());//|| m_buttonManager.isPressed(GPIOInput::)
    //-----------------------------------обдув------------------------------------------
    m_blowUpWatcher.update(ui->pushButton_blowerUp->isEnabled() && (ui->pushButton_blowerUp->isDown() || m_buttonManager.isPressed(GPIOInput::IN_BLOW_UP)));
    m_blowDownWatcher.update(ui->pushButton_blowerDown->isEnabled() &&(ui->pushButton_blowerDown->isDown()|| m_buttonManager.isPressed(GPIOInput::IN_BLOW_DOWN)));
    m_blowLeftWatcher.update(ui->pushButton_blowerLeft->isDown() || m_buttonManager.isPressed(GPIOInput::IN_BLOW_LEFT));
    m_blowRightWatcher.update(ui->pushButton_blowerRight->isDown() || m_buttonManager.isPressed(GPIOInput::IN_BLOW_RIGHT));

    starter->updateButtons(serviceOtherEngineLeftForm->starterBtnStatus()||gpioMatirx->keyPressed == GPIOInput::IN_STARTER);
    //qDebug()<<"* starter"<<(gpioMatirx->keyPressed == GPIOInput::IN_STARTER);
    //starter->updateButtons(m_buttonManager.isPressed(GPIOInput::IN_STARTER));
}

bool MainWindow::getGPIOInput(GPIOInput id){
    return m_buttonManager.isPressed(id);
}

void MainWindow::checkAndShowStatus(){
    //checkEngineAndRollLocks();

    // wait_on_start
    showStatus(ui->label_wait_on_start, engine->waitOnStart, "Требуется прогрев двигателя", "Двигатель прогрет");

    // повреждение двигателя
    if ((engine->damage == 2 || engine->damage == 1) && !ui->label_engine_damage->isVisible() && showCheckEngine){
        view->addLogError("Двигателю требуется обслуживание");
        view->addLogError("Код ошибки SPM=" + QString::number(engine->DM01SPNValue) + " FMI=" + QString::number(engine->DM01FMIValue));
        ui->label_engine_damage->show();
    }
    else if ((engine->damage == 0 || !showCheckEngine) && ui->label_engine_damage->isVisible()){
        ui->label_engine_damage->hide();
    }

    // клапан а1
    auto valveA1State = can0->getState(StateValveA1).toBool();
    showStatus(ui->label_a1, valveA1State);//"Засорен напорный фильтр"
    // напорный фильтр
    auto pressureFiltersState = can0->getState(StatePressureFilter1).toBool() || can0->getState(StatePressureFilter2).toBool() || can0->getState(StatePressureFilter3).toBool();
    showStatus(ui->label_pressure_filter, pressureFiltersState, "Засорен напорный фильтр");
    // сливной фильтр
    auto drainFilterState = can0->getState(StateDrainFilterD28).toBool();
    showStatus(ui->label_drain_filter, drainFilterState, "Засорен сливной фильтр");

    updateSensorAndWarningIndicators();
}


void MainWindow::showPultOffIgnition(){
    if (serviceIgnitionAutoRestoreBlocked || engineTempCrit)// после перегрева зажигание само не восстанавливаем
        return;
    starter->increaseIgnitionTimer();
}

void MainWindow::updateFRM(){
    QString path = "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_";
    // frm кунг
    view->updateFRM(ui->pushButton_frmKung, workMode.frmKung, "k");
    can0->setState(StateKungL5, workMode.frmKung);
    // frm щетка
    view->updateFRM(ui->pushButton_frmBroom, workMode.frmBroom, "h");
    can0->setState(StateFRMBroomL1, workMode.frmBroom);
    // frm магнит
    view->updateFRM(ui->pushButton_frmMagnet, workMode.frmMagnet, "m");
    can0->setState(StateFRMBackL2, workMode.frmMagnet);
}

void MainWindow::showStartClean(){
    if (startCleanKey.update(gpioMatirx->keyPressed == GPIOInput::IN_STARTCLEAN))
        on_pushButton_startstop_clicked();
}

void MainWindow::showModeButton(){
    const auto pressedKey = gpioMatirx->keyPressed;

    if (modeLeftKey.update(pressedKey == GPIOInput::IN_MODE_LEFT)){
        if (workMode.sweepType == LightSweep)
            on_pushButton_leafSweep_clicked();
        else if (workMode.sweepType == MediumSweep)
            on_pushButton_lightSweep_clicked();
        else if (workMode.sweepType == HeavySweep)
            on_pushButton_mediumSweep_clicked();
    }

    if (modeRightKey.update(pressedKey == GPIOInput::IN_MODE_RIGHT)){
        if (workMode.sweepType == LeafSweep)
            on_pushButton_lightSweep_clicked();
        else if (workMode.sweepType == LightSweep)
            on_pushButton_mediumSweep_clicked();
        else if (workMode.sweepType == MediumSweep)
            on_pushButton_heavySweep_clicked();
    }
}

void MainWindow::showMatrixFRMButton(){
    const bool frmPressed =
        gpioMatirx->keyPressed == GPIOInput::IN_FRM;

    if (frmKey.update(frmPressed))
        toggleAllFrm();
}



void MainWindow::settingsAskPassword(){
    if (DEVELOPER_MODE)
        Password_accepted = true;
    if (!Password_accepted){
        Password_Form *Password_window = new Password_Form (this, true, true);
        Password_window->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
        Password_window->setAttribute(Qt::WA_DeleteOnClose,true);

        connect(this,&MainWindow::Pass_close, Password_window, &Password_Form::close);
        connect(this,&MainWindow::Send_Pass_2_pass_form,Password_window, &Password_Form::Recieve_pass_name);
        connect(this,&MainWindow::Send_SecretPass_2_pass_form,Password_window, &Password_Form::Recieve_secret_pass_name);
        connect(Password_window,&Password_Form::Send_correct,this,&MainWindow::passwordSettingsOk);

        emit Send_Pass_2_pass_form(_settingsReader->readSettingsValue("Global/password").toInt());
        emit Send_SecretPass_2_pass_form(_settingsReader->readSettingsValue("Global/secretPassword").toInt());
        Password_window->show();
    }
    else{
        Password_accepted = false;
        currentState->setSettingsMode();
        settingsForm->fillElements();
        serviceSetingsName->show();
        settingsMainRightForm->show();
        return;
    }
}

void MainWindow::diagAskPassword(){
    if (DEVELOPER_MODE)
        Password_accepted = true;
    if (!Password_accepted){
        Password_Form *Password_window = new Password_Form (this, true, false);
        Password_window->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
        Password_window->setAttribute(Qt::WA_DeleteOnClose, true);

        connect(this,&MainWindow::Pass_close, Password_window,&Password_Form::close);
        connect(this,&MainWindow::Send_Pass_2_pass_form,Password_window,&Password_Form::Recieve_pass_name);
        connect(this,&MainWindow::Send_SecretPass_2_pass_form,Password_window,&Password_Form::Recieve_secret_pass_name);
        connect(Password_window,&Password_Form::Send_correct,this,&MainWindow::passwordDiagOk);

        emit Send_Pass_2_pass_form(_settingsReader->readSettingsValue("Global/passwordDiag").toInt());
        emit Send_SecretPass_2_pass_form(_settingsReader->readSettingsValue("Global/secretPasswordDiag").toInt());
        Password_window->show();
    }
    else{
        Password_accepted = false;
        currentState->setDiagMode();
        serviceMainRightForm->show();
    }
}

void MainWindow::passwordDiagOk(int pass){
    setRandomPassword(pass,"secretPasswordDiag");
    diagAskPassword();
}

void MainWindow::passwordSettingsOk(int pass){
    setRandomPassword(pass, "secretPassword");
    settingsAskPassword();
}

void MainWindow::setRandomPassword(int pass, QString passwordName){
    if (pass == _settingsReader->readSettingsValue("Global/"+passwordName).toString().toInt()){// сбросим одноразовы пароль
        if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
            QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");
        int random = std::rand() % ((9999 + 1) - 1) + 1;
        settings->setValue("Global/"+passwordName, random);// рандом от 1 до 9999
        settings->sync();
        system("sync");

        removeBadSettings();// надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
    }
    Password_accepted = true;
}

void MainWindow::serviceClosed(){// не работает
    currentState->setSweepMode();
    // superDiagMode = false;
    // menuMode = SweepMode;
}

void MainWindow::settingsClosed(){// нужно перезачитьать все сохраненные настройки
    broomCentral->readSettings();
    backMagnet->readSettings();
    frontRail->readSettings();
    blower->readSettings();
    readValues();
    canForEngine->setEngineAddr(globals->enigneAddr);
}

void MainWindow::showWorkMode(){
    if (!pauseActive)
        updateOrgansStates();
    updateButtonsActiveState();// проверим доступность кнопошков
    updateButtonsIcons();// меняем картиночки доступности кнопок после анализа
}

bool MainWindow::canStart(){
    return !engineTempWarn;// при перегреве двигателя уборку не начинаем
}

float MainWindow::hydraulicPressureValue(int index) const{
    static const DeviceStates pressureStates[4] = {
        StateHydraulicDistibutorPressure,
        StateHydraulicBroomPressure,
        StateHydraulicFanPressure,
        StateHydraulicBroomPressPressure
    };

    if (index < 0 || index >= 4)
        return 0.0f;

    return hydraulicPressureK[index] * can0->getState(pressureStates[index]).toFloat() + hydraulicPressureB[index];
}

void MainWindow::toggleAllFrm(){
    const bool enable = !(workMode.frmKung && workMode.frmBroom && workMode.frmMagnet);
    workMode.frmKung = enable;
    workMode.frmBroom = enable;
    workMode.frmMagnet = enable;
    showWorkMode();
}

bool MainWindow::isIdleMode(){
    // проверяекм надо ли затенять кнопки переключения режимов
    if (blower->getState() != Blower::BlowerOff
        || broomCentral->getState() != CentralBroom::BroomOff
        || frontRail->getState() != FrontRail::FrontRailOff
        || backMagnet->getState() != BackMagnet::BackMagnetOff
        || startClean){
        return false;
    }
    return true;
}

bool MainWindow::isBroomTransitioning(){
    CentralBroom::BroomStates broomTarget = broomCentral->needState;
    if (broomCentral->needState != CentralBroom::BroomOff && broomCentral->ableState < broomCentral->needState)
        broomTarget = broomCentral->ableState;
    return broomCentral->state != broomTarget;
}

bool MainWindow::isDumpTransitioning(){
    FrontRail::FrontRailStates railTarget = frontRail->needState;
    if (frontRail->needState != FrontRail::FrontRailOff && frontRail->ableState < frontRail->needState)
        railTarget = frontRail->ableState;
    return frontRail->state != railTarget;
}

bool MainWindow:: isMagnetTransitioning(){
    BackMagnet::BackMagnetStates magnetTarget = backMagnet->needState;
    if (backMagnet->needState != BackMagnet::BackMagnetOff && backMagnet->ableState < backMagnet->needState)
        magnetTarget = backMagnet->ableState;
    return backMagnet->state != magnetTarget;
}

Blower::BlowerStates MainWindow::blowerTargetState(){
    // при движении к работе цель ограничивается ableState, при выключении всегда идём к Off
    Blower::BlowerStates blowerTarget = blower->needState;
    if (blower->needState != Blower::BlowerOff && blower->ableState < blower->needState)
        blowerTarget = blower->ableState;
    return blowerTarget;
}

bool MainWindow::isBlowTransitioning(){
    return blower->state != blowerTargetState();
}

bool MainWindow::isOrgansTransitioning(){
    // органы в процессе перехода - ручное управление заблокировано
    // при движении к работе цель ограничивается ableState, при выключении всегда идём к Off
    if(isBroomTransitioning())
        return true;
    if(isDumpTransitioning())
        return true;
    if(isMagnetTransitioning())
        return true;
    if(isBlowTransitioning())
        return true;
    return false;
}


void MainWindow::setVertButtonsView(bool state){
    if(state){
        view->setStyle(ui->label_dumpUpDown, dumpVertPath + "off.png);");
        view->setStyle(ui->label_centralBroomUpDown, broomVertPath +  "off.png);");
        view->setStyle(ui->label_blowerUpDown, getBlowerVertIcon());
    }
    else{
        view->setStyle(ui->label_dumpUpDown, dumpVertPath + "blocked.png);");
        view->setStyle(ui->label_centralBroomUpDown, broomVertPath + "blocked.png);");
        view->setStyle(ui->label_blowerUpDown, blowerVertPath +"blocked.png);");
    }
}

void MainWindow::updatePhysButtons(){
    m_buttonManager.update(GPIOInput::IN_DUMP_UP, gpioMatirx->keyPressed == GPIOInput::IN_DUMP_UP);
    m_buttonManager.update(GPIOInput::IN_DUMP_DOWN, gpioMatirx->keyPressed == GPIOInput::IN_DUMP_DOWN);
    m_buttonManager.update(GPIOInput::IN_DUMP_RIGHT, gpioMatirx->keyPressed == GPIOInput::IN_DUMP_RIGHT);
    m_buttonManager.update(GPIOInput::IN_DUMP_LEFT, gpioMatirx->keyPressed == GPIOInput::IN_DUMP_LEFT);

    m_buttonManager.update(GPIOInput::IN_BROOM_UP, gpioMatirx->keyPressed == GPIOInput::IN_BROOM_UP);
    m_buttonManager.update(GPIOInput::IN_BROOM_DOWN, gpioMatirx->keyPressed == GPIOInput::IN_BROOM_DOWN);
    m_buttonManager.update(GPIOInput::IN_BROOM_RIGHT, gpioMatirx->keyPressed == GPIOInput::IN_BROOM_RIGHT);
    m_buttonManager.update(GPIOInput::IN_BROOM_LEFT, gpioMatirx->keyPressed == GPIOInput::IN_BROOM_LEFT);

    m_buttonManager.update(GPIOInput::IN_BLOW_UP, gpioMatirx->keyPressed == GPIOInput::IN_BLOW_UP);
    m_buttonManager.update(GPIOInput::IN_BLOW_DOWN, gpioMatirx->keyPressed == GPIOInput::IN_BLOW_DOWN);
    m_buttonManager.update(GPIOInput::IN_BLOW_RIGHT, gpioMatirx->keyPressed == GPIOInput::IN_BLOW_RIGHT);
    m_buttonManager.update(GPIOInput::IN_BLOW_LEFT, gpioMatirx->keyPressed == GPIOInput::IN_BLOW_LEFT);

    m_buttonManager.update(GPIOInput::IN_STARTER, gpioMatirx->keyPressed == GPIOInput::IN_STARTER);
}

void MainWindow::showPauseButton(){
    const bool pauseOrHomePressed =
        gpioMatirx->keyPressed == GPIOInput::IN_PAUSE_HOME;

    if (!pauseKey.update(pauseOrHomePressed))
        return;

    // Кнопка отпущена после корректного нажатия.
    if (startClean)
    {
        pauseActive = !pauseActive;

        if (pauseActive)
        {
            view->addLogWarning("Пауза включена");

            // Переводим выбранные органы
            // в промежуточное безопасное положение.
            if (frontRail->choosed)
                frontRail->setNeedState(FrontRail::FrontRailBounced);

            if (broomCentral->choosed)
                broomCentral->setNeedState(CentralBroom::BroomRotateIn);

            if (backMagnet->choosed)
                backMagnet->setNeedState(BackMagnet::BackMagnetDownIn);

            if (blower->choosed)
                blower->setNeedState(Blower::BlowerDownIn);
        }
        else
        {
            view->addLog("Пауза снята");
        }

        showWorkMode();
    }
    else{
        // В режиме простоя эта же физическая кнопка
        // отправляет органы в домашнее положение.
        on_pushButton_homeState_clicked();
    }
}

void MainWindow::tryToDisableDumpFlow(){
    if(!isDumpTransitioning()){
        setDumpFlow(false);
    }
}

void MainWindow::tryToDisableBroomFlow(){
    if(!isBroomTransitioning()){
        setBroomFlow(false);
    }
}

void MainWindow::changeBlowDirection(bool isRight){
    workMode.blowLeft = !isRight;
    workMode.blowRight = isRight;
    showWorkMode();
}

void MainWindow::setBlowerLifted(bool lifted){
    workMode.blowLifted = lifted;
    showWorkMode();
}
//=============================================================
//====================Buttons click handlers===================
//=============================================================

// нажали пуск - запускаем все выбранные устройства
void MainWindow::on_pushButton_startstop_clicked(){
    if (startClean){// или отжали или был выбран режим защиты от неприятностей
        if (pauseActive){// снимаем паузу, уборка продолжается
            pauseActive = false;
            view->addLog("Пауза снята");
            showWorkMode();
            return;
        }
        view->addLogWarning("Уборка окончена");
        startCleaning(false);
    }
    else{
        view->addLogWarning("Уборка начата");
        startCleaning(true);
    }
    showWorkMode();
}
void MainWindow:: startCleaning(bool state){
    blower->setDirection(workMode.blowRight);
    workMode.blowLifted = false;// при старте уборки выбранная сторона снова разворачивает обдув
    startClean = state;
}
void MainWindow::on_pushButton_service_clicked(){
    logger->addUserLogInfo(Logger::UF_SERVICE_PRESSED, 1);
    diagAskPassword();
}

void MainWindow::on_pushButton_lightSweep_clicked(){changeSweepMode(LightSweep);}

void MainWindow::on_pushButton_mediumSweep_clicked(){changeSweepMode(MediumSweep);}

void MainWindow::on_pushButton_heavySweep_clicked(){ changeSweepMode(HeavySweep);}// защита от поднятой щетки

void MainWindow::on_pushButton_leafSweep_clicked(){ changeSweepMode(LeafSweep);}

void MainWindow::on_pushButton_settings_clicked(){ settingsAskPassword();}

//------------------------------------------------------------------------

void MainWindow::on_pushButton_centralBroomUp_clicked(){m_broomUpWatcher.update(true);}
void MainWindow::on_pushButton_centralBroomDown_clicked(){m_broomDownWatcher.update(true);}
void MainWindow::on_pushButton_centralBroomLeft_clicked(){m_broomLeftWatcher.update(true);}
void MainWindow::on_pushButton_centralBroomRight_clicked(){m_broomRightWatcher.update(true);}

void MainWindow::on_pushButton_dumpUp_clicked(){ m_dumpUpWatcher.update(true);}
void MainWindow::on_pushButton_dumpDown_clicked(){ m_dumpDownWatcher.update(true);}
void MainWindow::on_pushButton_dumpLeft_clicked(){ m_dumpLeftWatcher.update(true);}
void MainWindow::on_pushButton_dumpRight_clicked(){ m_dumpRightWatcher.update(true);}

void MainWindow::on_pushButton_blowerUp_clicked(){m_blowUpWatcher.update(true);}
void MainWindow::on_pushButton_blowerDown_clicked(){m_blowDownWatcher.update(true);}
void MainWindow::on_pushButton_blowerLeft_clicked(){m_blowLeftWatcher.update(true);}
void MainWindow::on_pushButton_blowerRight_clicked(){m_blowRightWatcher.update(true);}

//------------------------------------------------------------------------


void MainWindow::on_pushButton_dumpFlow_clicked(){
    m_dumpFlowWatcher.update(true);
    // workMode.frontDumpFlow = !workMode.frontDumpFlow;
    // if(startClean){
    //     frontRail->setFlowActive(workMode.frontDumpFlow);}
    // showWorkMode();
}

//------------------------------------------------------------------------
void MainWindow::on_pushButton_centralBroomFlow_clicked(){
    qDebug()<<"!!! click broom Flow";
    m_broomFlowWatcher.update(true);
    // workMode.centralBroomFlow = !workMode.centralBroomFlow;
    // bool newState = workMode.centralBroomFlow;
    // setBroomFlowView(newState);
    // if(startClean){
    //     broomCentral->setFlowActive(newState);}
    // showWorkMode();
}


// void MainWindow::setBroomPressed(bool state){
//     workMode.centralBroomPress = state;
//     broomCentral->setPressActive(state);
//     showWorkMode();
// }

void MainWindow::on_pushButton_centralBroomPress_clicked(){
    qDebug()<<"!!! click broom Press";
    m_broomPressWatcher.update(true);
    //setBroomPressed(!workMode.centralBroomPress);
    // workMode.centralBroomPress = !workMode.centralBroomPress;
    // broomCentral->setPressActive(workMode.centralBroomPress);
    //showWorkMode();
}

void MainWindow::on_pushButton_backMagnet_clicked(){
    workMode.backMagnet = !workMode.backMagnet;
    showWorkMode();
}

void MainWindow::on_pushButton_frmKung_clicked(){
    workMode.frmKung = !workMode.frmKung;
    showWorkMode();
}

void MainWindow::on_pushButton_frmBroom_clicked(){
    workMode.frmBroom = !workMode.frmBroom;
    showWorkMode();
}

void MainWindow::on_pushButton_frmMagnet_clicked(){
    workMode.frmMagnet = !workMode.frmMagnet;
    showWorkMode();
}

void MainWindow::on_pushButton_homeState_clicked(){
    view->addLogWarning("Переход в домашнее состояние, ожидайте");
    // вынуждаем все органы убраться поновой. обманка
    backMagnet->state = BackMagnet::BackMagnetDowned;
    blower->state = Blower::BlowerRotated;
    frontRail->state = FrontRail::FrontRailFlowed;
    broomCentral->state = CentralBroom::BroomRotated;
}

//==============================CommonLogic==========================================
void MainWindow::updateOrgansStates(){// задаем режимы органам
    // щетка
    bool isBroomActive = workMode.centralBroomLeft||workMode.centralBroomRight;
    broomCentral->setNeedState(isBroomActive? CentralBroom::BroomFlowed: CentralBroom::BroomOff);
    broomCentral->choosed = isBroomActive;
    broomCentral->needGoLeft = workMode.centralBroomLeft;
    // отвал
    bool isDumpActive = workMode.frontDumpLeft||workMode.frontDumpRight;
    frontRail->setNeedState(isDumpActive? FrontRail::FrontRailFlowed: FrontRail::FrontRailOff);
    frontRail->choosed = isDumpActive;
    frontRail->needGoLeft = workMode.frontDumpLeft;
    // дулка
    bool isBlowerActive = (workMode.blowLeft||workMode.blowRight) && !workMode.blowLifted;
    blower->setNeedState(isBlowerActive? Blower::BlowerRotated: Blower::BlowerOff);
    blower->choosed = isBlowerActive;
    // магнит
    bool isMagnetActive = workMode.backMagnet;
    backMagnet->setNeedState(isMagnetActive? BackMagnet::BackMagnetDowned: BackMagnet::BackMagnetOff);
    backMagnet->choosed = isMagnetActive;

    // Применяем режимы к уже разложенным органам сразу при изменении workMode.

    if (!workMode.centralBroomPress){// При отключенном прижиме сбрасываем поджим сразу, чтобы не оставались активные клапаны.
        broomCentral->stopPress();
    }

    if(!startClean){
        return;
    }

    // if (broomCentral->getState() >= CentralBroom::BroomFlowed){
    //     if (workMode.centralBroomFlow){
    //         broomCentral->setFlowActive(true);
    //     }
    //     else{
    //         //broomCentral->setState(CentralBroom::BroomFlowIn);
    //         broomCentral->setFlowActive(false);
    //         //broomCentral->goNoFlow();
    //     }
    // }

    // if (frontRail->getState() >= FrontRail::FrontRailFlowed){
    //     if (workMode.frontDumpFlow){
    //         //view->addLog("!!!Отвал плавающий");
    //         frontRail->setState(FrontRail::FrontRailFlowIn);
    //         //frontRail->goFlow();
    //     }

    //     else{
    //         //view->addLog("!!!Отвал не плавающий");
    //         frontRail->setState(FrontRail::FrontRailFlowOut);
    //         //frontRail->goNoFlow();
    //     }
    // }
}

//==============================Sweep================================================

void MainWindow:: changeSweepMode(quint8 mode){
    if (workMode.sweepType != mode){
        workMode.sweepType = mode;
        showWorkMode();
    }
}

//==============================Broom================================================
QString MainWindow::getBroomDefaultIcon(){
    return workMode.centralBroomLeft? broomHorPath + "left_on.png);":
        workMode.centralBroomRight?broomHorPath+ "right_on.png);":
        "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_off.png);";
}


//==============================Dump=================================================
QString MainWindow::getDumpDefaultIcon(){
    return workMode.frontDumpLeft? "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_left_on.png);":
        workMode.frontDumpRight?"background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_right_on.png);":
        "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_off.png);";
}

//==============================Blower===============================================
QString MainWindow::getBlowerVertIcon(){// положение обдува, к которому идёт автомат: иконка меняется сразу, как принята команда
    if (!ui->pushButton_blowerDown->isEnabled())
        return blowerVertPath + "blocked.png);";
    const Blower::BlowerStates target = blowerTargetState();
    const bool raised = target == Blower::BlowerOff || target == Blower::BlowerDownIn;
    return blowerVertPath + (raised ? "up_on.png);" : "down_on.png);");
}

QString MainWindow::getBlowerDefaultIcon(){
    return blowerHorPath + getBlowerSideIconName();
}

QString MainWindow::getBlowerSideIconName(){
    if (!workMode.blowLeft && !workMode.blowRight)
        return "off.png);";
    // во время уборки показываем сторону, на которую обдув переходит, не дожидаясь реальной смены
    // (workMode хранит текущую сторону - по ней автомат определяет, что смену надо выполнить)
    const bool right = startClean ? blower->targetRight() : workMode.blowRight;
    return right ? "right_on.png);" : "left_on.png);";
}


//==============================Messages=============================================
// void MainWindow::printOrganStatus(organsEnums::Organ organ, organsEnums::Direction direction, bool state){
//     view->screenLog->printMovementLog(organ, direction, (state? "":" завершено"));
// }

//==============================UI===================================================


void MainWindow::updateButtonsActiveState(){
    if (startClean){
        setVertButtonsView(true);
        // домашнее сервис и настройки
        if (ui->pushButton_homeState->isEnabled()){
            ui->pushButton_homeState->setEnabled(false);
            ui->pushButton_settings->setEnabled(false);
            ui->pushButton_service->setEnabled(false);
        }

        // дулка
        if (!ui->pushButton_blowerDown->isEnabled()){
            ui->pushButton_blowerDown->setEnabled(true);
            ui->pushButton_blowerUp->setEnabled(true);
        }

        // щетка
        if (!ui->pushButton_centralBroomDown->isEnabled()){
            qDebug()<<"show work mode: true";
            ui->pushButton_centralBroomDown->setEnabled(true);
            ui->pushButton_centralBroomUp->setEnabled(true);
        }

        // отвал
        if (!ui->pushButton_dumpDown->isEnabled()){
            ui->pushButton_dumpDown->setEnabled(true);
            ui->pushButton_dumpUp->setEnabled(true);
        }
    }
    else
    {
        setVertButtonsView(false);
        // домашнее сервис и настройки
        if (!ui->pushButton_homeState->isEnabled()){
            ui->pushButton_homeState->setEnabled(true);
            ui->pushButton_settings->setEnabled(true);
            ui->pushButton_service->setEnabled(true);
        }

        // дулка
        if (ui->pushButton_blowerDown->isEnabled()){
            ui->pushButton_blowerDown->setEnabled(false);
            ui->pushButton_blowerUp->setEnabled(false);
        }
        if (!ui->pushButton_blowerLeft->isEnabled()){
            ui->pushButton_blowerLeft->setEnabled(true);
            ui->pushButton_blowerRight->setEnabled(true);
        }

        // щетка
        if (ui->pushButton_centralBroomDown->isEnabled()){

            qDebug()<<"show work mode: false";
            ui->pushButton_centralBroomDown->setEnabled(false);
            ui->pushButton_centralBroomUp->setEnabled(false);
        }
        if (!ui->pushButton_centralBroomLeft->isEnabled()){
            ui->pushButton_centralBroomLeft->setEnabled(true);
            ui->pushButton_centralBroomRight->setEnabled(true);
        }
        // отвал
        if (ui->pushButton_dumpDown->isEnabled()){
            ui->pushButton_dumpDown->setEnabled(false);
            ui->pushButton_dumpUp->setEnabled(false);
        }
        if (!ui->pushButton_dumpLeft->isEnabled()){
            ui->pushButton_dumpLeft->setEnabled(true);
            ui->pushButton_dumpRight->setEnabled(true);
        }
    }
}

// void MainWindow::setBroomFlowView(bool state){
//     //workMode.centralBroomFlow = state;
//     updateBroomBtnsView();
//     //m_broomFlowWatcher(state);
// }

void MainWindow::setDumpFlowView(bool state){
    workMode.frontDumpFlow = state;
    //updateDumpBtnsView();
}

// void MainWindow::setBroomPressView(bool state){
//     //setBroomPressed(state);
//     // workMode.centralBroomPress = state;
//     // broomCentral->setPressActive(state);
//     updateBroomBtnsView();
// }

// void MainWindow::updateBroomBtnsView(){
//     // QString path = "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBrooms";
//     // path += workMode.centralBroomLeft?"Below_left_on.png);":(workMode.centralBroomRight?"Below_right_on.png);":"Front_off.png);");
//     // view->setStyle(ui->label_centralBroom, path);

//     // path = "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_";
//     // path += workMode.centralBroomFlow?(workMode.centralBroomPress? "on.png);": "up_on.png);"):(workMode.centralBroomPress? "down_on.png);": "off.png);");
//     // view->setStyle(ui->label_centralBroomFloatPress, path);
// }


void MainWindow::updateButtonsIcons(){

    QString path ="border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_";
    view->setStyle(ui->pushButton_leafSweep, path + (workMode.sweepType == LeafSweep?"easy_on.png);":"easy_off.png);"));// смет листья
    view->setStyle(ui->pushButton_lightSweep, path + (workMode.sweepType == LightSweep? "average_on.png);":"average_off.png);" ));// смет легкий
    view->setStyle(ui->pushButton_mediumSweep, path + (workMode.sweepType == MediumSweep ? "hard_on.png);" : "hard_off.png);"));// смет средний
    view->setStyle(ui->pushButton_heavySweep, path + (workMode.sweepType == HeavySweep ? "leafHarvesting_on.png);" : "leafHarvesting_off.png);"));// смет тяжелый

    // дулка
    view->setStyle(ui->label_blowerUpDown, getBlowerVertIcon());

    // щетка
    path = "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_";
    view->setStyle(ui->label_centralBroomUpDown, path + (ui->pushButton_centralBroomDown->isEnabled()? "off.png);":"blocked.png);"));

    //updateBroomBtnsView(); // задняя щетка
    //updateDumpBtnsView();// отвал

    // магнит
    path = "background-image: url(:/Images/Images/main/buttons/configuration_button_magnet_";
    view->setStyle(ui->label_backMagnet, path + (workMode.backMagnet? "on.png);": "off.png);"));

    // дулка
    view->setStyle(ui->label_blower, getBlowerDefaultIcon());

    // старт стоп
    path = "outline: none;border-style:none;background-image: url(:/Images/Images/main/buttons/button_start_";
    view->setStyle(ui->pushButton_startstop, path + (startClean? "on.png);": "off.png);"));
}

void MainWindow::setBtnState(QWidget *widget, QString path, std::function<void()> handler){
    view->setStyle(widget, path);
    handler();
}

// void MainWindow::setBtnView(bool isPressed, QLabel *lbl, QPushButton *btn, QString path){
//     btn->setProperty("wasDown", isPressed);
//     view->setStyle(lbl, path);
// }

void MainWindow::setBtnView(bool isPressed, QLabel *lbl, QPushButton *btn, QString onPath, QString offPath){
    btn->setProperty("wasDown", isPressed);
    if(isPressed)
        view->setStyle(lbl, onPath);
    else
        view->setStyle(lbl, offPath);

}

void MainWindow::selectBtnState(bool gpioPressed, QLabel *lbl, QPushButton *btn, QString onPath, QString offPath,
                                std::function<void()> onPressHandler,
                                std::function<void()> onReleaseHandler){

    bool wasPressed = btn->property("wasDown").toBool();
    bool isPressed = btn->isDown() || gpioPressed;
    if(wasPressed == isPressed)
        return;

    btn->setProperty("wasDown", isPressed);
    if(isPressed)
        setBtnState(lbl, onPath, onPressHandler);
    else
        setBtnState(lbl, offPath, onReleaseHandler);
}

void MainWindow::selectBtnState(bool gpioPressed, QPushButton *btn, QString onPath, QString offPath,
                                std::function<void()> onPressHandler,
                                std::function<void()> onReleaseHandler){

    bool wasPressed = btn->property("wasDown").toBool();
    bool isPressed = btn->isDown() || gpioPressed;

    if(wasPressed == isPressed)
        return;
    btn->setProperty("wasDown", isPressed);

    if(isPressed)
        setBtnState(btn, onPath, onPressHandler);
    else
        setBtnState(btn, offPath, onReleaseHandler);
}

void MainWindow::showStatus(QLabel *label, bool check, QString messageOn, QString messageOff) {
    if(check == label->isVisible()){
        return;
    }
    if(check){
        if(messageOn!=NULL){
            view->addLogWarning(messageOn);
        }
        label->show();
    }
    else{
        label->hide();
        if(messageOff!=NULL){
            view->addLog(messageOff);
        }
    }
}

void MainWindow::resetPassword(){
    serviceGeneralPasswordLeftForm->resetPassword();
}

ViewController* MainWindow::getView(){return view;}

SettingsReader* MainWindow::getReader(){return _settingsReader;}

bool MainWindow::isSpeedTooHigh(){
    auto speed = currentState->vehicleSpeed;
    if (globals->disableCleanSpeed <= 0)// порог не задан - проверку не выполняем
        return false;
    return (speed > globals->disableCleanSpeed && speed != 199 && speed < 200);
}

void MainWindow::setServiceFormName(QWidget* form, QString name){
    serviceSetingsName->raise();
    serviceSetingsName->setText((form == settingsForm?name:""));
}
