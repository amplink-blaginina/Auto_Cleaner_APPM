#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLayoutItem>

#include <linux/can/j1939.h>
#define MAXSOCK 16
#include "password_form.h"

#include <QLayoutItem>
#include <QScroller>
#include <QScrollBar>
#include <QPushButton>

// pwm
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include <linux/can/j1939.h>

#define DEVELOPER_MODE 0

QLocale EngLocale (QLocale::Russian);

static int ptsInc = 0;
QString programmVersionString = "AutoCleaner APPM v3.010";

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


MainWindow::MainWindow(int argc, char *argv[], QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QApplication* a = qobject_cast<QApplication*>(QApplication::instance());

    a->setApplicationName("AutoCleaner_APPM");
    a->setApplicationVersion(programmVersionString);
    // Парсер командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("Запуск приложения с ключами");
    parser.addHelpOption();
    parser.addVersionOption();

//    QCommandLineOption verboseOption(
//        QStringList() << "v" << "verbose",
//        "Включить подробный вывод.");
//    parser.addOption(verboseOption);

    // Разбор аргументов
    parser.process(*a);

    //auto *cam = new VlcWidget("rtsp://admin:1234@192.168.0.10:554/stream1");
    //auto *cam = new VlcWidget_EGL("rtsp://192.168.0.12/");
    //cam->setMinimumSize(640, 360);
    //cam->show();

    ui->label_version->setText("Версия: " + programmVersionString);

    workMode.centralBroomLeft = false;
    workMode.centralBroomRight = false;
    workMode.centralBroomFlow = false;
    workMode.centralBroomPress = false;
    workMode.blowLeft = false;
    workMode.blowRight = false;
    workMode.frontDumpLeft = false;
    workMode.frontDumpRight = false;
    workMode.frontDumpFlow = false;
    workMode.backMagnet = false;
    workMode.frmBroom = false;
    workMode.frmMagnet = false;
    workMode.frmKung = false;
    workMode.sweepType = LightSweep;
    qRegisterMetaType<struct can_frame>();

    // загружаем сторонние шрифты
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
    QFont font;
    font.setFamily("Mont");
    font.setPointSize(13);

    font.setFamily("CentSchbkCyrill BT");
    font.setPointSize(31);

    bool need_to_reconf = false;
    // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
    removeBadSettings();

    if (need_to_reconf)
        QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini");
    settings = new QSettings(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini", QSettings::IniFormat);
    //дефолтные настройки
    defaultValues.insert("Global/canDeivce", "can1");
    defaultValues.insert("Global/j1939Deivce", "can0");
    defaultValues.insert("Global/password", "1234");
    defaultValues.insert("Global/secretPassword", "51234");
    defaultValues.insert("Global/brightness.level", 1);

    defaultValues.insert("Hydraulic/temperatures.Warning", 50);
    defaultValues.insert("Hydraulic/temperatures.Critical", 80);

    defaultValues.insert("Dump/timeouts.DumpDownOut", 10);
    defaultValues.insert("Dump/timeouts.DumpDownIn", 10);
    defaultValues.insert("Dump/timeouts.DumpFlowOut", 2);
    defaultValues.insert("Dump/timeouts.DumpSlideOut", 10);
    defaultValues.insert("Dump/timeouts.DumpSlideIn", 10);
    defaultValues.insert("Dump/timeouts.DumpBounceOut", "0.1");

    defaultValues.insert("CentralBroom/timeouts.BroomDownOut", 5);
    defaultValues.insert("CentralBroom/timeouts.BroomDownIn", 10);
    defaultValues.insert("CentralBroom/timeouts.BroomFlowOut", 2);
    defaultValues.insert("CentralBroom/timeouts.BroomSlideOut", 10);
    defaultValues.insert("CentralBroom/timeouts.BroomSlideIn", 10);
    defaultValues.insert("CentralBroom/timeouts.BroomRotateOut", 1);
    defaultValues.insert("CentralBroom/timeouts.BroomRotateIn", 1);
    defaultValues.insert("CentralBroom/timeouts.BroomBounceOut", "0.1");
    defaultValues.insert("CentralBroom/speeds.LeafSweep", 70);
    defaultValues.insert("CentralBroom/speeds.LightSweep", 80);
    defaultValues.insert("CentralBroom/speeds.MediumSweep", 90);
    defaultValues.insert("CentralBroom/speeds.HeavySweep", 100);

    defaultValues.insert("BackMagnet/timeouts.BackMagnetDownOut", 10);
    defaultValues.insert("BackMagnet/timeouts.BackMagnetDownIn", 15);

    defaultValues.insert("Blower/timeouts.BlowerDownIn", 5);
    defaultValues.insert("Blower/timeouts.BlowerDownOut", 5);
    defaultValues.insert("Blower/timeouts.BlowerSlideIn", 5);
    defaultValues.insert("Blower/timeouts.BlowerSlideOut", 5);
    defaultValues.insert("Blower/timeouts.BlowerRotateIn", 1);
    defaultValues.insert("Blower/timeouts.BlowerRotateOut", 1);
    defaultValues.insert("Blower/speeds.LeafSweep", 70);
    defaultValues.insert("Blower/speeds.LightSweep", 80);
    defaultValues.insert("Blower/speeds.MediumSweep", 90);
    defaultValues.insert("Blower/speeds.HeavySweep", 100);
    defaultValues.insert("Blower/fanAccelRate", 10);

    defaultValues.insert("Engine/rpm.None", 800);
    defaultValues.insert("Engine/rpm.LeafSweep", 1100);
    defaultValues.insert("Engine/rpm.LightSweep", 1400);
    defaultValues.insert("Engine/rpm.MediumSweep", 1700);
    defaultValues.insert("Engine/rpm.HeavySweep", 2000);
    defaultValues.insert("Engine/addr", 7);
    defaultValues.insert("Engine/rpm.dieselDefault", 1100); // дизельная скорость для регулирования
    defaultValues.insert("Engine/temperature.electricWork", 20); // рабочая температура эл. двигателя
    defaultValues.insert("Engine/temperature.dieselWork", 20); // рабочая температура диз. двигателя
    defaultValues.insert("Engine/temperature.electricCrit", 100); // крит температура эл. двигателя
    defaultValues.insert("Engine/temperature.dieselCrit", 100); // крит температура диз. двигателя
    defaultValues.insert("Engine/hydroTempK", "1");
    defaultValues.insert("Engine/hydroTempB", "-40");
    defaultValues.insert("Engine/startRollRequiredDays", 5);
    defaultValues.insert("Engine/startLowTemperatureEdge", -10);
    defaultValues.insert("Engine/starterMaxWorkSec", 15);
    defaultValues.insert("Engine/starterPauseSec", 60);
    defaultValues.insert("Engine/starterMaxAttempts", 3);
    defaultValues.insert("Engine/rollMaxWorkSec", 15);
    defaultValues.insert("Engine/rollPauseSec", 60);
    defaultValues.insert("Engine/rollMaxAttempts", 3);
    defaultValues.insert("Engine/waterSensorRedHours", 2);
    defaultValues.insert("Engine/airFilterRedHours", 20);
    defaultValues.insert("Engine/waterSensorEmergencyMode", true);
    defaultValues.insert("Engine/airFilterEmergencyMode", true);
    defaultValues.insert("Engine/disableRollRequirement", false);
    defaultValues.insert("Engine/disableTemperatureBlock", false);
    defaultValues.insert("Engine/ignoreAllEmergency", false);
    defaultValues.insert("Engine/lastStartDate", QDate::currentDate().addDays(-6));
    defaultValues.insert("Global/hydraulicPressure1K", "1");
    defaultValues.insert("Global/hydraulicPressure1B", "0");
    defaultValues.insert("Global/hydraulicPressure2K", "1");
    defaultValues.insert("Global/hydraulicPressure2B", "0");
    defaultValues.insert("Global/hydraulicPressure3K", "1");
    defaultValues.insert("Global/hydraulicPressure3B", "0");
    defaultValues.insert("Global/hydraulicPressure4K", "1");
    defaultValues.insert("Global/hydraulicPressure4B", "0");

    // инит главных счетчиков
    defaultValues.insert("TOCur/Engine", 0);
    defaultValues.insert("TOCur/EngineToday", 0); // срез на начало дня
    defaultValues.insert("TOCur/DateToday", QDateTime::currentDateTime().date().addDays(-1)); // какая сегодня дата
    defaultValues.insert("TOCur/System", 0);

    // ТО
    defaultValues.insert("TO/EngineOil", 24 * 3600);
    defaultValues.insert("TOCur/EngineOil", 0);
    TONameValues.insert("EngineOil", "Масло и охлаждающая жидкость двигателя");
    TOAlarmValues.insert("EngineOil", 0);
    TOSourceValues.insert("EngineOil", 0);

//    defaultValues.insert("TO/PneumaticCheck", 24 * 3600);
//    defaultValues.insert("TOCur/PneumaticCheck", 0);
//    TONameValues.insert("PneumaticCheck", "Работа пневмосистемы");
//    TOAlarmValues.insert("PneumaticCheck", 0);
//    TOSourceValues.insert("PneumaticCheck", 1);

//    defaultValues.insert("TO/Hydraulic", 70 * 3600);
//    defaultValues.insert("TOCur/Hydraulic", 0);
//    TONameValues.insert("Hydraulic", "Гидравлика");
//    TOAlarmValues.insert("Hydraulic", 0);
//    TOSourceValues.insert("Hydraulic", 1);

//    defaultValues.insert("TO/Sharnirs", 24 * 3600);
//    defaultValues.insert("TOCur/Sharnirs", 0);
//    TONameValues.insert("Sharnirs", "Состояние шарниров");
//    TOAlarmValues.insert("Sharnirs", 0);
//    TOSourceValues.insert("Sharnirs", 1);

//    defaultValues.insert("TO/FanGear", 24 * 3600);
//    defaultValues.insert("TOCur/FanGear", 0);
//    TONameValues.insert("FanGear", "Клиноременная передача вентилятора");
//    TOAlarmValues.insert("FanGear", 0);
//    TOSourceValues.insert("FanGear", 1);

    defaultValues.insert("TO/HydraulicOilCheck", 24 * 3600);
    defaultValues.insert("TOCur/HydraulicOilCheck", 0);
    TONameValues.insert("HydraulicOilCheck", "Проверка масла гидросистемы");
    TOAlarmValues.insert("HydraulicOilCheck", 0);
    TOSourceValues.insert("HydraulicOilCheck", 1);

//    defaultValues.insert("TO/WorkCheck", 24 * 3600);
//    defaultValues.insert("TOCur/WorkCheck", 0);
//    TONameValues.insert("WorkCheck", "Работа органов и спецоборудования");
//    TOAlarmValues.insert("WorkCheck", 0);
//    TOSourceValues.insert("WorkCheck", 1);

//    defaultValues.insert("TO/WaterCheck", 24 * 3600);
//    defaultValues.insert("TOCur/WaterCheck", 0);
//    TONameValues.insert("WaterCheck", "Работа систем увлажнения");
//    TOAlarmValues.insert("WaterCheck", 0);
//    TOSourceValues.insert("WaterCheck", 1);

//    defaultValues.insert("TO/PneumaticJointCheck", 24 * 3600);
//    defaultValues.insert("TOCur/PneumaticJointCheck", 0);
//    TONameValues.insert("PneumaticJointCheck", "Герметичность соединений пневмосистемы");
//    TOAlarmValues.insert("PneumaticJointCheck", 0);
//    TOSourceValues.insert("PneumaticJointCheck", 1);

//    defaultValues.insert("TO/FanWashing", 24 * 3600);
//    defaultValues.insert("TOCur/FanWashing", 0);
//    TONameValues.insert("FanWashing", "Промывка вентилятора");
//    TOAlarmValues.insert("FanWashing", 0);
//    TOSourceValues.insert("FanWashing", 1);

    defaultValues.insert("TO/CarLubrication", 100 * 3600);
    defaultValues.insert("TOCur/CarLubrication", 0);
    TONameValues.insert("CarLubrication", "Смазка машины");
    TOAlarmValues.insert("CarLubrication", 0);
    TOSourceValues.insert("CarLubrication", 1);

    defaultValues.insert("TO/CarTightening", 100 * 3600);
    defaultValues.insert("TOCur/CarTightening", 0);
    TONameValues.insert("CarTightening", "Затяжка резьбовых соединений");
    TOAlarmValues.insert("CarTightening", 0);
    TOSourceValues.insert("CarTightening", 1);

//    defaultValues.insert("TO/SomeCheck1", 100 * 3600);
//    defaultValues.insert("TOCur/SomeCheck1", 0);
//    TONameValues.insert("SomeCheck1", "Привод вентилятора, кард. вал, муфта, натяжение ремня");
//    TOAlarmValues.insert("SomeCheck1", 0);
//    TOSourceValues.insert("SomeCheck1", 1);

//    defaultValues.insert("TO/BroomTightening", 100 * 3600);
//    defaultValues.insert("TOCur/BroomTightening", 0);
//    TONameValues.insert("BroomTightening", "Затяжка болтов гидромоторов щеток");
//    TOAlarmValues.insert("BroomTightening", 0);
//    TOSourceValues.insert("BroomTightening", 1);

//    defaultValues.insert("TO/FanLubricant", 100 * 3600);
//    defaultValues.insert("TOCur/FanLubricant", 0);
//    TONameValues.insert("FanLubricant", "Состояние вентилятора, смазка подшипников");
//    TOAlarmValues.insert("FanLubricant", 0);
//    TOSourceValues.insert("FanLubricant", 1);

//    defaultValues.insert("TO/HydraulicJointCheck", 100 * 3600);
//    defaultValues.insert("TOCur/HydraulicJointCheck", 0);
//    TONameValues.insert("HydraulicJointCheck", "Состояние соединений гидравлической системы");
//    TOAlarmValues.insert("HydraulicJointCheck", 0);
//    TOSourceValues.insert("HydraulicJointCheck", 1);

//    defaultValues.insert("TO/BackCoverSeal", 100 * 3600);
//    defaultValues.insert("TOCur/BackCoverSeal", 0);
//    TONameValues.insert("BackCoverSeal", "Уплотнение задней крышки");
//    TOAlarmValues.insert("BackCoverSeal", 0);
//    TOSourceValues.insert("BackCoverSeal", 1);

//    defaultValues.insert("TO/OilFilterInHydroTank", 100 * 3600);
//    defaultValues.insert("TOCur/OilFilterInHydroTank", 0);
//    TONameValues.insert("OilFilterInHydroTank", "Фильтрующие элементы масляных фильтров в гидробаке");
//    TOAlarmValues.insert("OilFilterInHydroTank", 0);
//    TOSourceValues.insert("OilFilterInHydroTank", 1);

//    defaultValues.insert("TO/HydraulicOilChange", 100 * 3600);
//    defaultValues.insert("TOCur/HydraulicOilChange", 0);
//    TONameValues.insert("HydraulicOilChange", "Замена масла гидросистемы");
//    TOAlarmValues.insert("HydraulicOilChange", 0);
//    TOSourceValues.insert("HydraulicOilChange", 1);

//    defaultValues.insert("TO/ElectricCheck", 100 * 3600);
//    defaultValues.insert("TOCur/ElectricCheck", 0);
//    TONameValues.insert("ElectricCheck", "Работа электрооборудования");
//    TOAlarmValues.insert("ElectricCheck", 0);
//    TOSourceValues.insert("ElectricCheck", 1);

//    defaultValues.insert("TO/HydroTankWash", 100 * 3600);
//    defaultValues.insert("TOCur/HydroTankWash", 0);
//    TONameValues.insert("HydroTankWash", "Промыть водяной бак и коммуникацию");
//    TOAlarmValues.insert("HydroTankWash", 0);
//    TOSourceValues.insert("HydroTankWash", 1);

    defaultValues.insert("TO/EngineTO", 100 * 3600);
    defaultValues.insert("TOCur/EngineTO", 0);
    TONameValues.insert("EngineTO", "ТО двигателя");
    TOAlarmValues.insert("EngineTO", 0);
    TOSourceValues.insert("EngineTO", 1);

//    defaultValues.insert("TO/SleavesCheck", 500 * 3600);
//    defaultValues.insert("TOCur/SleavesCheck", 0);
//    TONameValues.insert("SleavesCheck", "Состояние всех рукавов");
//    TOAlarmValues.insert("SleavesCheck", 0);
//    TOSourceValues.insert("SleavesCheck", 1);

    defaultValues.insert("TO/PressureFilterChange", 500 * 3600);
    defaultValues.insert("TOCur/PressureFilterChange", 0);
    TONameValues.insert("PressureFilterChange", "Замена напорного фильтра");
    TOAlarmValues.insert("PressureFilterChange", 0);
    TOSourceValues.insert("PressureFilterChange", 1);

//    defaultValues.insert("TO/WaterCheck2", 500 * 3600);
//    defaultValues.insert("TOCur/WaterCheck2", 0);
//    TONameValues.insert("WaterCheck2", "Ревизия систем увлажнения");
//    TOAlarmValues.insert("WaterCheck2", 0);
//    TOSourceValues.insert("WaterCheck2", 1);

    can0 = NULL;
    readSettings();

    systemConfigure.configurationVersion = 110;
    systemConfigure.boardsType[0] = BOARD_CP;
    //    systemConfigure.channelsType[0][0] = OUT_MODE_NORMAL;
    //    systemConfigure.channelsType[0][1] = OUT_MODE_NORMAL;
    //    systemConfigure.channelsType[0][2] = OUT_MODE_NORMAL;
    //    systemConfigure.channelsType[0][3] = OUT_MODE_NORMAL;
    //    systemConfigure.channelsType[0][4] = IN_MODE_NORMAL;
    //    systemConfigure.channelsType[0][5] = IN_MODE_NORMAL;
    //    systemConfigure.channelsType[0][6] = IN_MODE_NORMAL;
    //    systemConfigure.channelsType[0][7] = IN_MODE_NORMAL;

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

    readSystemConfigure();

    // верхний лог со скролом
    messageList = new MessageList(this);
    messageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    messageList->verticalScrollBar()->setStyleSheet("QScrollBar {width:0px;}");
    messageList->setStyleSheet("QListView {background: transparent;}");
    messageList->setFixedWidth(519);
    messageList->setFixedHeight(115);
    //WARNING
    QScroller::grabGesture(messageList->viewport(), QScroller::LeftMouseButtonGesture);
    connect(messageList, SIGNAL(entered(QModelIndex)), this, SLOT(messageListPressed()));
    connect(messageList, SIGNAL(viewportEntered()), this, SLOT(messageListPressed()));


    ui->logLayout->addWidget(messageList);

    QString can_device = readSettingsValue("Global/canDeivce").toString();
    QString j1939_device = readSettingsValue("Global/j1939Deivce").toString();
    restartIgnitionDelay = readSettingsValue("Global/restartIgnitionDelay").toInt();
    readSettingsValue("Global/password").toInt();
    readSettingsValue("Global/secretPassword").toInt();
    readSettingsValue("Global/passwordDiag").toInt();
    readSettingsValue("Global/secretPasswordDiag").toInt();

    qDebug() << "can " << can_device << " " << j1939_device;

    ui->label_date->setFont(QFont("Mont",15));
    ui->label_time->setFont(QFont("Mont",20));

    // режим опасной диагностики
    superDiagMode = false;

    // инит логгера (черный ящик)
    logger = new Logger(NULL);

    //Инит CAN и GPIO
    can0 = new MyCan(can_device, logger, true, NULL);//can0
    canForEngine = new MyCanEngine(j1939_device, logger, false, NULL);
    canForEngine->setEngineAddr(enigneAddr);
    canj1939 = new MyCanJ1939(j1939_device, logger, true, NULL);
    canj1939Main = new MyCanJ1939(can_device, logger, false, NULL);// камазовкий кан незя рестартить потому как он на таком же интерфейсе как и ПО ГО. А это опасно
    //gp = new gpio_class();
    gpio = new GPIOWorker();
    gpioMatirx = new GPIOMatrix();

    // закидываем настрокий конфигурацции для кана
    can0->fillSystemConfigure(&systemConfigure, &systemElements);
    logger->fillSystemConfigure(&can0->systemConfigure, &can0->systemElements);// тырим ее у кана потому что он расставляет важные параметры

    menuMode = SweepMode;
    startClean = false;
    starterStarted = false;
    starterStartedTime = QDateTime::currentDateTime();
    //starter = false;
    starterStartedAlarmed = false;
    starterBroomAlarmed = false;
    starterBunkerAlarmed = false;
    chooseFrm = false;
//    pultUp = false;
//    pultUpCounter = 0;
    backGearCounter = 0;
    backControl = false;
    backIdleCounter = 0;
    backLight = false;
    backBlockCounter = 0;
    backLightTimeCounter = 0;
    ignitionOffTimer = 0;
    engineStartedOk = false;
    Password_accepted = false;
    engineTempCrit = false;
    engineTempWarn = false;
    hydroTempCrit = false;
    hydroTempWarn = false;
    chooseGabaritCount = 0;
    startCleanTimeCounter = 0;
    centralBroomLeftTimeCounter = 0;
    centralBroomRightTimeCounter = 0;
    frontDumpLeftTimeCounter = 0;
    frontDumpRightTimeCounter = 0;
    blowerTimeCounter = 0;
    frmTimeCounter = 0;
    leftModeTimeCounter = 0;
    rightModeTimeCounter = 0;

    KVControl = false;
    currentKV = 0;

    serviceSetingsName = new QLabel(this);
    serviceSetingsName->setStyleSheet("color: white");
    font.setFamily("Mont");
    font.setPointSize(16);
    serviceSetingsName->setFont(font);
    serviceSetingsName->setGeometry(12 ,4 ,359 ,43);
    serviceSetingsName->hide();

    serviceGPIOServiceIntervalLeftForm = new ServiceGPIOServiceIntervalLeftForm(this);
    serviceGPIOServiceIntervalLeftForm->hide();

    serviceOtherEngineLeftForm = new ServiceOtherEngineLeftForm(this);
    serviceOtherEngineLeftForm->hide();

    serviceOtherLightLeftForm = new ServiceOtherLightLeftForm(this);
    serviceOtherLightLeftForm->hide();

    serviceDevicesHydraulicsLeftForm = new ServiceDevicesHydraulicsLeftForm(this);
    serviceDevicesHydraulicsLeftForm->hide();

    serviceDevicesDKPLeftForm = new ServiceDevicesDKPLeftForm(this);
    serviceDevicesDKPLeftForm->hide();

    serviceGlobalDateTimeLeftForm = new ServiceGlobalDateTimeLeftForm(this);
    serviceGlobalDateTimeLeftForm->hide();

    serviceGeneralPasswordLeftForm = new ServiceGeneralPasswordLeftForm(this);
    serviceGeneralPasswordLeftForm->hide();

    serviceMainRightForm = new ServiceMainRightForm(this);
    serviceMainRightForm->hide();

    settingsSettingsConfigurationLeftForm = new SettingsSettingsConfigurationLeftForm(this);
    settingsSettingsConfigurationLeftForm->hide();

    settingsWifiLeftForm = new SettingsWifiLeftForm(this);
    settingsWifiLeftForm->hide();

    settingsForm = new SettingsForm(settings, this);
    //settingsForm->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
    settingsForm->hide();
    settingsForm->fillElements();
    connect(settingsForm, SIGNAL(closedAndSave()), this, SLOT(settingsClosed()));

    settingsMainRightForm = new SettingsMainRightForm(this);
    settingsMainRightForm->hide();

    // создаем таймер для обслуживания общих узлов
    connect(&mainProgressTimer, SIGNAL(timeout()), this, SLOT(mainProgress()));
    mainProgressTimer.start(100);

    connect(&repaintTimer, SIGNAL(timeout()), this, SLOT(repaintProgress()));
    repaintTimer.start(500);

    connect(&oneSecondTimer, SIGNAL(timeout()), this, SLOT(oneSecond()));
    oneSecondTimer.start(1000);

    connect (&goHomeTimer, SIGNAL(timeout()), this, SLOT(resetDevices()));

    blockScreen = new BlockForm(this);
    blockScreen->hide();

    // создаем виджет двигателя
    engine = new Engine(canj1939, this);
    // создаем виджеты щеток и прочих модулей
    broomCentral = new CentralBroom(can0, NULL, settings, this);
    frontRail = new FrontRail(can0, NULL, settings, this);
    backMagnet = new BackMagnet(can0, NULL, settings, this);
    blower = new Blower(can0, NULL, settings, this);

    resetDevices();
    //can0->setState(StateBoardsPowerOut, true);
    showWorkMode();

    startIgnition();

    connect(can0, SIGNAL(canPOError()), this, SLOT(canPOError()));
    connect(can0, SIGNAL(canDataReady(struct can_frame)), this, SLOT(incomeData(struct can_frame)));
    connect(canj1939, SIGNAL(canError()), this, SLOT(canJ1939Error()));
    connect(canj1939, SIGNAL(canDataReadyJ1939(quint32, quint8, QByteArray)), this, SLOT(incomeDataJ1939(quint32, quint8, QByteArray)));
    connect(canj1939Main, SIGNAL(canError()), this, SLOT(canJ1939MainError()));
    connect(canj1939Main, SIGNAL(canDataReadyJ1939(quint32, quint8, QByteArray)), this, SLOT(incomeDataJ1939Main(quint32, quint8, QByteArray)));

    engineCoolantTemp = -40;
    vehicleSpeed = 0;
    vehicleVoltage = 0;

    stopInProgress = false;
    waitOnStartAlarmed = false;
    cleanWrongSpeedAlarmed = false;
    addLog("ПВИ запущен", WarningStatus);

    buttonsLightLevel = 0;

    hydroTempCounterToShow = 0;
    pauseCleanTimeCounter = 0;
    pauseActive = false;

    starterMaxWorkSec = 15;
    starterPauseSec = 60;
    starterMaxAttempts = 3;
    rollMaxWorkSec = 15;
    rollPauseSec = 60;
    rollMaxAttempts = 3;
    requireRollAfterDays = 5;
    lowTempRequireWarm = -10;
    waterSensorRedHours = 2;
    airFilterRedHours = 20;
    waterSensorEmergencyMode = true;
    airFilterEmergencyMode = true;
    disableRollRequirement = false;
    disableTemperatureBlock = false;
    ignoreAllEmergency = false;

    engineRunStatePrev = false;
    starterLockedByRoll = false;
    starterLockedByTemperature = false;
    starterLockedByEmergency = false;
    rollLockedByTemperature = false;
    rollLockedByEmergency = false;
    needRollProcedure = false;
    rollCompleted = false;

    starterPauseActive = false;
    starterAttemptsUsed = 0;
    starterNeedReboot = false;
    starterButtonPrev = false;
    starterPauseWarned = false;

    prerollButtonPrev = false;
    prerollStarterButtonPrev = false;
    rollInputPrev = false;
    prerollSequenceActive = false;
    prerollSequenceStep = 0;
    prerollStarterUnlocked = false;
    rollRunActive = false;
    rollPauseActive = false;
    rollAttemptsUsed = 0;
    rollNeedReboot = false;
    rollPauseWarned = false;
    serviceIgnitionAutoRestoreBlocked = false;

    logNeedRollShown = false;
    logNeedWarmShown = false;
    waterSensorActivePrev = false;
    airFilterActivePrev = false;
    oilFilterActivePrev = false;
    heatRelayActivePrev = false;
    waterSensorTimeStarted = false;
    airFilterTimeStarted = false;
    waterSensorStartedAt = 0;
    airFilterStartedAt = 0;

    ui->label_engineLowTemperature->hide();
    ui->label_waterInFuel->hide();
    ui->label_airFilter->hide();
    ui->label_oilFilter->hide();

    can0->setState(StateStarterAllow, false);
    can0->setState(StateStarterRoll, false);
}

void MainWindow::buttonsLightCheck()
{
    if (buttonsLightLevel < buttonsLightLevelEdge) {buttonsLightLevel++;}
    if (buttonsLightLevel > buttonsLightLevelEdge && buttonsLightLevel > 0) {buttonsLightLevel--;}
    //PWMSend();
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addElement(int element_, QString name_, int board_, int channel_, quint8 type_, quint8 median_type_, quint8 pwm_type_, quint8 pfm_low_type_, quint8 pfm_high_type_, quint8 value_change_speed_)
{
    systemElements.insert(element_, new SystemElement(name_, board_, channel_));
    systemConfigure.id[board_][channel_] = element_;
    systemConfigure.channelsType[board_][channel_] = type_;
    systemConfigure.channelsMedianSize[board_][channel_] = median_type_;
    systemConfigure.channelsPWMSize[board_][channel_] = pwm_type_;
    systemConfigure.channelsLowPFM[board_][channel_] = pfm_low_type_;
    systemConfigure.channelsHighPFM[board_][channel_] = pfm_high_type_;
    systemConfigure.channelsValueChangeSpeed[board_][channel_] = value_change_speed_;
}

void MainWindow::getSystemConfigure(SystemConfigure* dst)
{
    //QMutexLocker l(&configureMutex);
    systemConfigure.copy(dst);
}

void MainWindow::getElements(QMap<int, SystemElement*>* dst)
{
    //QMutexLocker l(&configureMutex);
    foreach (int key, systemElements.keys())
    {
        dst->insert(key, new SystemElement(systemElements.value(key)));
    }
}

void MainWindow::readSystemConfigure()
{
    // проверяем есть ли настройки в конфиге
    bool need_to_save = false;
    if (settings->contains("Configuration/configurationVersion"))
    {// настройки есть
        qDebug() << "have config";
        if (settings->value("Configuration/configurationVersion").toUInt() >= systemConfigure.configurationVersion)
        {// версия их актуальна - перечитываем
            qDebug() << "version actual";
            systemConfigure.clear();
            while (systemElements.size() > 0)
                delete systemElements.take(systemElements.firstKey());
            systemConfigure.configurationVersion = settings->value("Configuration/configurationVersion").toUInt();
            for (int i = 1; i < 9; i++)
            {
                systemConfigure.boardsType[i] = settings->value("Configuration/Board" + QString::number(i) + "_boardType").toUInt();
                for (int k = 0; k < 12; k++)
                {
                    systemConfigure.channelsType[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "Type").toUInt();
                    systemConfigure.id[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "ElementId").toUInt();
                    systemConfigure.channelsMedianSize[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "MedianSize").toUInt();
                    systemConfigure.channelsLowPFM[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "LowPFM").toUInt();
                    systemConfigure.channelsHighPFM[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "HighPFM").toUInt();
                    systemConfigure.channelsPWMSize[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "PWMSize").toUInt();
                    systemConfigure.channelsValueChangeSpeed[i][k] = settings->value("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "ValueChangeSpeed").toUInt();

                    if (systemConfigure.id[i][k] != 0)
                    {
                        addElement(settings->value("Configuration/Element" + QString::number(systemConfigure.id[i][k]) + "_id").toUInt(),
                                   settings->value("Configuration/Element" + QString::number(systemConfigure.id[i][k]) + "_name").toString(),
                                   settings->value("Configuration/Element" + QString::number(systemConfigure.id[i][k]) + "_board").toUInt(),
                                   settings->value("Configuration/Element" + QString::number(systemConfigure.id[i][k]) + "_channel").toUInt(),
                                   systemConfigure.channelsType[i][k],
                                   systemConfigure.channelsMedianSize[i][k],
                                   systemConfigure.channelsPWMSize[i][k],
                                   systemConfigure.channelsLowPFM[i][k],
                                   systemConfigure.channelsHighPFM[i][k],
                                   systemConfigure.channelsValueChangeSpeed[i][k]);
                    }
                }
            }
        }
        else
        {
            qDebug() << "version old";
            need_to_save = true;
        }
    }
    else
    {
        qDebug() << "have NO config";
        need_to_save = true;
    }
    if (need_to_save)
    {// конфига или нет или он старый - пересохраняем
        saveSystemConfigure();
    }
}

void MainWindow::removeBadSettings()
{
    QDir dir(QCoreApplication::applicationDirPath(), {"settingsAutoCleaner.ini.*"});
    for(const QString & filename: dir.entryList())
        dir.remove(filename);
}

void MainWindow::saveSystemConfigure()
{
    // почтистим все конфиговое
    if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
        QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");

    settings->beginGroup("Configuration");
    settings->remove("");
    settings->endGroup();
    settings->setValue("Configuration/configurationVersion", systemConfigure.configurationVersion);
    for (int i = 1; i < 9; i++)
    {
        settings->setValue("Configuration/Board" + QString::number(i) + "_boardType", systemConfigure.boardsType[i]);
        for (int k = 0; k < 12; k++)
        {
            settings->setValue("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "Type", systemConfigure.channelsType[i][k]);
            settings->setValue("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "ElementId", systemConfigure.id[i][k]);
            settings->setValue("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "MedianSize", systemConfigure.channelsMedianSize[i][k]);
            settings->setValue("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "LowPFM", systemConfigure.channelsLowPFM[i][k]);
            settings->setValue("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "HighPFM", systemConfigure.channelsHighPFM[i][k]);
            settings->setValue("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "PWMSize", systemConfigure.channelsPWMSize[i][k]);
            settings->setValue("Configuration/Board" + QString::number(i) + "_channel" + QString::number(k) + "ValueChangeSpeed", systemConfigure.channelsValueChangeSpeed[i][k]);
        }
    }
    foreach (int key, systemElements.keys())
    {
        settings->setValue("Configuration/Element" + QString::number(key) + "_id", key);
        settings->setValue("Configuration/Element" + QString::number(key) + "_name", systemElements.value(key)->name);
        settings->setValue("Configuration/Element" + QString::number(key) + "_board", systemElements.value(key)->board);
        settings->setValue("Configuration/Element" + QString::number(key) + "_channel", systemElements.value(key)->channel);
    }
    settings->sync();
    system("sync");
    // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
    removeBadSettings();
}

void MainWindow::messageListPressed()
{// если касались списка лога, то не трогаем его еще 5 секунд, после этого он сам скролится вниз
    qDebug() << "pressed";
    messageList->manualControl = 50;
}

QVariant MainWindow::readSettingsValue(QString name)
{// читает значение из настроек (если значения нет, то берет дефолтное)
    if (settings->contains(name))
        return settings->value(name);
    else
        settings->setValue(name, defaultValues.value(name));
    return defaultValues.value(name);
}

void MainWindow::readSettings()
{// у каждого модуля есть своя функиция чтения настроек. Настройки которые неподвластны каким то модулям зачитываются тут
    // холостой ход
    rpmNone = readSettingsValue("Engine/rpm.None").toInt();

    cleanConfiguration.frontDumpUse = readSettingsValue("CleanConfiguration/frontDumpUse").toBool();
    cleanConfiguration.magnetUse = readSettingsValue("CleanConfiguration/magnetUse").toBool();
    cleanConfiguration.centralBroomUse = readSettingsValue("CleanConfiguration/centralBroomUse").toBool();
    cleanConfiguration.blowUse = readSettingsValue("CleanConfiguration/blowUse").toBool();

    // охлаждение двигателя
    ventEdge = readSettingsValue("Engine/rpm.VentEdge").toInt();
    // адрес двигателя
    enigneAddr = readSettingsValue("Engine/addr").toInt();
    // пороги скорости
    enableCleanSpeed = readSettingsValue("Global/enableCleanSpeed").toInt();
    disableCleanSpeed = readSettingsValue("Global/disableCleanSpeed").toInt();
    // сознаваться ли про чек энжын?
    showCheckEngine = readSettingsValue("Global/showCheckEngine").toBool();

    buttonsLightLevelEdge = readSettingsValue("Global/buttonsLightLevelEdge").toInt();
    buttonsLightLevelEdge = 10; // TODO ????

    // значения моточасов которые будут расти (хранятся в секундах и сливаются на флэшку каждые 5 мотоминут)
    // когда работает вспомогательный двигатель
    TOCurValues["Engine"] = readSettingsValue("TOCur/Engine").toUInt();
    TOCurValues["EngineLast"] = readSettingsValue("TOCur/Engine").toUInt();
    // когда работает просто система (нажали старт)
    TOCurValues["System"] = readSettingsValue("TOCur/System").toUInt();
    TOCurValues["SystemLast"] = readSettingsValue("TOCur/System").toUInt();

    engineToday = readSettingsValue("TOCur/EngineToday").toUInt();
    dateToday = readSettingsValue("TOCur/DateToday").toDate();

    // тут лежат предельные параметры по ТО (сколько моточасов может работать орган)
    // так же тут лежат значения последнего ТО по этой части (Cur)
    foreach (QString key, TONameValues.keys())
    {
        TOValues[key] = readSettingsValue("TO/" + key).toUInt();
        TOCurValues[key] = readSettingsValue("TOCur/" + key).toUInt();
    }

    engineTempWarnEdge = readSettingsValue("Global/engineTempWarnTime").toInt();
    engineTempGoodValue = readSettingsValue("Global/engineTempGood").toInt();
    engineTempWarnValue = readSettingsValue("Global/engineTempWarn").toInt();
    engineTempCritValue = readSettingsValue("Global/engineTempCrit").toInt();

    hydroTempGoodValue = readSettingsValue("Global/hydroTempGood").toInt();
    hydroTempWarnValue = readSettingsValue("Global/hydroTempWarn").toInt();
    hydroTempCritValue = readSettingsValue("Global/hydroTempCrit").toInt();

    hydroTempK = readSettingsValue("Global/hydroTempK").toFloat();
    hydroTempB = readSettingsValue("Global/hydroTempB").toFloat();

    fanAccelRate = readSettingsValue("Blower/fanAccelRate").toInt();
    addElement(StateValveD3, "(D3)Вращение вентилятора", 2, 11, OUT_MODE_PFM, 0, 0, 50, 150, fanAccelRate);

    requireRollAfterDays = readSettingsValue("Engine/startRollRequiredDays").toInt();
    lowTempRequireWarm = readSettingsValue("Engine/startLowTemperatureEdge").toInt();
    starterMaxWorkSec = readSettingsValue("Engine/starterMaxWorkSec").toInt();
    starterPauseSec = readSettingsValue("Engine/starterPauseSec").toInt();
    starterMaxAttempts = readSettingsValue("Engine/starterMaxAttempts").toInt();
    rollMaxWorkSec = readSettingsValue("Engine/rollMaxWorkSec").toInt();
    rollPauseSec = readSettingsValue("Engine/rollPauseSec").toInt();
    rollMaxAttempts = readSettingsValue("Engine/rollMaxAttempts").toInt();
    waterSensorRedHours = readSettingsValue("Engine/waterSensorRedHours").toInt();
    airFilterRedHours = readSettingsValue("Engine/airFilterRedHours").toInt();
    waterSensorEmergencyMode = readSettingsValue("Engine/waterSensorEmergencyMode").toBool();
    airFilterEmergencyMode = readSettingsValue("Engine/airFilterEmergencyMode").toBool();
    disableRollRequirement = readSettingsValue("Engine/disableRollRequirement").toBool();
    disableTemperatureBlock = readSettingsValue("Engine/disableTemperatureBlock").toBool();
    ignoreAllEmergency = readSettingsValue("Engine/ignoreAllEmergency").toBool();
    lastEngineStartDate = readSettingsValue("Engine/lastStartDate").toDate();
    if (!lastEngineStartDate.isValid())
        lastEngineStartDate = QDate::currentDate();

    for (int index = 0; index < 4; ++index)
    {
        const QString suffix = QString::number(index + 1);
        hydraulicPressureK[index] = readSettingsValue("Global/hydraulicPressure" + suffix + "K").toFloat();
        hydraulicPressureB[index] = readSettingsValue("Global/hydraulicPressure" + suffix + "B").toFloat();
    }
}

void MainWindow::startIgnition()
{// запуск зажигания
    qDebug() << "start ignition";
    //gp->Set_GPIO_State(OUT_IGNITION, 1);
    can0->setState(StateIgnitionOut, true);
}

void MainWindow::resetDevices()
{
    //test
    if (DEVELOPER_MODE || blockScreen->developerMode)
    {
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
        blockScreen->label_blockScreenText->setText("Нет связи с блоками управления. Пожалуйста подождите");
        blockScreen->show();
        goHomeTimer.setInterval(1000);
        goHomeTimer.start();
        // выключаем режим автоматики
        if (startClean)
        {
            addLog("Связь с блоками потеряна - остановка уборки", WarningStatus);
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
    else
    {// не можем себе позволить кошерно сбросить состояния пока блоки не подключились
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
    for (int i = 0;i < 8; i++)
    {
        canj1939->setTsc1Byte(0, i);
    }
    goHomeTimer.stop();
    goHomeTimer.setInterval(1000);
    blockScreen->hide();
    emit resetComplete();
}

void MainWindow::canPOError()
{
    if (!ui->POStatus->isVisible())
        ui->POStatus->show();

//    qDebug() << "POError";
    resetDevices();//сбросить все команды и состояния
}
void MainWindow::incomeData(struct can_frame frame)
{
    if (frame.can_id == 0x00000AC0)// && (centralBroom->getState() == CentralBroom::BroomRotated || serviceForm->isVisible()))
    {// КВ
        if (frame.data[3] & 0x10 && (frame.data[3] & ~(0x10)) && workMode.centralBroomPress)
        {// назад
            broomCentral->goPressUp();
            KVControl = true;

            currentKV = 0;
        }
        else if ((frame.data[3] & 0x10) == 0 && frame.data[3] && workMode.centralBroomPress)
        {// вперед
            broomCentral->goPressDown();
            KVControl = true;
        }
        else if (KVControl)
        {
            broomCentral->goPressNone();
            KVControl = false;
        }
    }
}
qint8 MainWindow::getFilteredTemp()
{
    hydroTempBuffer.removeFirst();
    QList<qint8> sortBuffer = hydroTempBuffer;
    for (int i = 0; i < hydroTempBuffer.size(); i++)
    {
        for (int j = i; j < hydroTempBuffer.size() - 1; j++)
        {
            if (sortBuffer[j] > sortBuffer[j + 1])
            {
                qint8 tmp_val = sortBuffer[j];
                sortBuffer[j] = sortBuffer[j + 1];
                sortBuffer[j + 1] = tmp_val;
            }
        }
    }
    hydroTempMedianBuffer.append(sortBuffer[5]);
    qint16 ret = 0;
    if (hydroTempMedianBuffer.size() > 10)
    {
        hydroTempMedianBuffer.removeFirst();
        for (int i = 0; i < hydroTempMedianBuffer.size(); i ++)
            ret += hydroTempMedianBuffer[i];
        ret = ret / 10;
    }
    else
        ret = sortBuffer[5];
    return ret;
}
void MainWindow::canJ1939Error()
{
    if (!ui->J1939Status->isVisible())
        ui->J1939Status->show();
    // сбрасываем значения
    engine->rpm = 0;
    engine->engineCoolantTemp = -40;
    engine->coolantTempEverReceived = false;
}

void MainWindow::canJ1939MainError()
{
    if (!ui->J1939MainStatus->isVisible())
        ui->J1939MainStatus->show();

    engineCoolantTemp = -40;
    vehicleSpeed = 0;
    vehicleVoltage = 0;

//    qDebug() << "J1939MainError";
// //    resetDevices();//сбросить все команды и состояния
}
void MainWindow::incomeDataJ1939(quint32 pgn, quint8 sa, QByteArray data)
{
//    qDebug() << pgn;
    if (ui->J1939Status->isVisible())
        ui->J1939Status->hide();
}
void MainWindow::incomeDataJ1939Main(quint32 pgn, quint8 sa, QByteArray data)
{// данные от переднего двигателя
    //    qDebug() << pgn;
    if (pgn == 0xF004) //eec1
    {//обороты переднего двигателя
        quint16 rpm_ = data[3] + (data[4] << 8);
        //        qDebug() << "rpm " << (rpm_ / 8);
        //        ui->label_rpm->setText(QString::number(rpm_ / 8));
    }
    // лучше брать FEF1 (похоже на круиз) или ETC1(F002) (частота вала) или FEBF - скорее всего abs (FE6C это скорость от тахографа а у нас ее нет похоже)

    if (pgn == 0xFEBF)
    {// скорость по ABS
        QString temp = "%1";
        if ((uint8_t)data[1] < 200)
        {
            temp = temp.arg(data[1], 2, 10, QChar('0'));
//            if (ui->label_speed->text() != temp)
//                ui->label_speed->setText(temp);
            vehicleSpeed = data[1];
        }
        speedCounter = 0;
    }

    if (pgn == 0xFEEE) //et1
    {
        engineCoolantTemp = data[0] - 40;
    }

    if (pgn == 0xF002)
    {// скорость выходного вала трансмиссии
        quint16 speed_ = data[1] + (data[2] << 8);
        //        qDebug() << "speed 1 " << ((float)speed_ * 0.126092385);
        //        QString temp = "%1";
        //        temp = temp.arg((int)((float)speed_ * 0.126092385), 2, 10, QChar('0'));
        //        ui->label_speed->setText(temp);
    }
    if (pgn == 0xFEF1)
    {// скорость по круизу
        //        qDebug() << "speed 2 " << (data[2]);
        //        QString temp = "%1";
        //        temp = temp.arg(data[2], 2, 10, QChar('0'));
        //        ui->label_speed->setText(temp);
    }
    if (pgn == 0xFE6C)
    {// скорость по тахографу
        quint16 speed_ = data[6] + (data[7] << 8);
        //        qDebug() << "speed 3 " << (speed_ / 256);
        //        QString temp = "%1";
        //        temp = temp.arg((int)((float)speed_ / 256), 2, 10, QChar('0'));

        //        ui->label_speed->setText(temp);
    }


    if (pgn == 0xFEF7)
    {// напряжение АКБ
        quint16 volt_ = data[4] + (data[5] << 8);
        //        qDebug() << "volt " << (volt_ / 20);
        QString temp = "%1";
        temp = temp.arg((int)((float)volt_ / 20), 2, 10, QChar('0'));
//        if (ui->label_voltage->text() != temp)
//            ui->label_voltage->setText(temp);
        vehicleVoltage = (float)volt_ / 20;
        voltageCounter = 0;
    }

    if (ui->J1939MainStatus->isVisible())
        ui->J1939MainStatus->hide();
}

void MainWindow::oneSecond()
{// универсальный таймер для всяких нужд (раз в сек)
    can0->setState(State24Volt, true);

    QDateTime DateAndTime=QDateTime::currentDateTime().addMonths(0);
   // DateAndTime= DateAndTime.addSecs(3600);
    QString string_current_date = EngLocale.dayName(DateAndTime.date().dayOfWeek(),QLocale::ShortFormat);
    string_current_date += " "+ QString::number(DateAndTime.date().day(),10);
    string_current_date += " "+ EngLocale.monthName(DateAndTime.date().month(),QLocale::ShortFormat);
    string_current_date += " "+ QString::number(DateAndTime.date().year(),10);
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
    if (engineRunningNow && !engineRunStatePrev)
    {
        lastEngineStartDate = QDate::currentDate();
        settings->setValue("Engine/lastStartDate", lastEngineStartDate);
        settings->sync();
        rollCompleted = false;
        starterNeedReboot = false;
        starterAttemptsUsed = 0;
        starterPauseActive = false;
        logNeedRollShown = false;
    }
    engineRunStatePrev = engineRunningNow;

    if (engineTempWarnTimer > 0)
        engineTempWarnTimer--;

    // узнаем моточасы за сегодня
    if (DateAndTime.date() != dateToday)
    {// надо записать сегодняшний срез и сохранить его
        dateToday = DateAndTime.date();
        engineToday = TOCurValues["Engine"];
        settings->beginGroup("TOCur");
        settings->setValue("EngineToday", engineToday);
        settings->setValue("DateToday", dateToday);
        settings->endGroup();
        settings->sync();
        // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
        removeBadSettings();
    }
    // отображаем моточасы
    QString temp = "%1";
    temp = temp.arg(TOCurValues["Engine"] / 3600, 5, 10, QChar('0'));
    if (temp != ui->label_frontEngineTOTotalValue->text())
        ui->label_frontEngineTOTotalValue->setText(temp);
    temp = "%1";
    temp = temp.arg((TOCurValues["Engine"] - engineToday) / 3600, 2, 10, QChar('0'));
    if (temp != ui->label_engineTODailyValue->text())
        ui->label_engineTODailyValue->setText(temp);
    if (TOCurValues["Engine"] - TOCurValues["EngineLast"] > 5 * 60)
    {// пора сохранить кой какие данные каждые 5 минут
        TOCurValues["EngineLast"] = TOCurValues["Engine"];
        // сохраняем настройки
        // перед этим удаляем lock файл - были случаи что lock файл блокировал запись настроек
        if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
            QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");

        settings->beginGroup("TOCur");
        settings->setValue("Engine", TOCurValues["Engine"]);
        settings->endGroup();
        settings->sync();
        // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
        removeBadSettings();
    }
    if (TOCurValues["System"] - TOCurValues["SystemLast"] > 5 * 60)
    {// пора сохранить кой какие данные каждые 5 минут
        TOCurValues["SystemLast"] = TOCurValues["System"];
        // сохраняем настройки
        // перед этим удаляем lock файл - были случаи что lock файл блокировал запись настроек
        if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
            QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");

        settings->beginGroup("TOCur");
        settings->setValue("System", TOCurValues["System"]);
        settings->endGroup();
        settings->sync();
        // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
        removeBadSettings();
    }

    bool to_test = false;
    foreach (QString key, TONameValues.keys())
    {
        quint32 compare = TOCurValues["Engine"];// системный счетчик по двигателю
        if (TOSourceValues[key] == 1)
            compare = TOCurValues["System"];
        if (compare - TOCurValues[key] > TOValues[key])
        {
            to_test = true;
            if (TOAlarmValues[key] != 1)
            {
                TOAlarmValues[key] = 1;
                addLog(TONameValues[key] + " требует ТО", WarningStatus);
            }
        }
        else
            TOAlarmValues[key] = 0;
    }
    // отображаем знак ТО
    if (to_test && !ui->label_TO->isVisible())
    {
        ui->label_TO->show();
    }
    else if (!to_test && ui->label_TO->isVisible())
    {
        ui->label_TO->hide();
    }
    if (can0->isActive())
    {
        if (ui->POStatus->isVisible())
            ui->POStatus->hide();
    }

    if (can0->isActive())
    {
        // получим температуру гидрооборудования
        qint16 hydro_temp = hydroTempK * can0->getState(StateHydraulicOilTemperature).toUInt() + hydroTempB;
        //qDebug() << frame.data[7];
        hydroTempBuffer.append(hydro_temp);
        if (hydroTempBuffer.size() > 10)// && hydroTempCounterToShow == 0)
        {
            //hydroTempCounterToShow = 10;
            hydro_temp = getFilteredTemp();
            ui->label_hydraulicTemperature->setText(QString::number(hydro_temp));
        }
    }
}

void MainWindow::repaintProgress()
{

    QString text = QString::number(hydroTempK * can0->getState(StateHydraulicOilTemperature).toUInt() + hydroTempB, 'f', 1);
    if (ui->label_hydraulicTemperature->text() != text + " C ТЕМП ГО")
        ui->label_hydraulicTemperature->setText(text + " C ТЕМП ГО");
    text = QString::number(hydraulicPressureValue(2), 'f', 1);
    if (ui->label_fan_pressure->text() != text + " P ТИ3")
        ui->label_fan_pressure->setText(text + " P ТИ3");
    text = QString::number(hydraulicPressureValue(3), 'f', 1);
    if (ui->label_roll_pressure->text() != text + " P ТИ4")
        ui->label_roll_pressure->setText(text + " P ТИ4");

    // защита от залипания графики
    ui->label_time->repaint();
}

bool MainWindow::inHomeState()
{
    if ((frontRail->getState() == FrontRail::FrontRailOff || frontRail->railAlarmed)
            && (backMagnet->getState() == BackMagnet::BackMagnetOff || backMagnet->magnetAlarmed)
            && (broomCentral->getState() == CentralBroom::BroomOff || broomCentral->broomAlarmed)
            && (blower->getState() == Blower::BlowerOff || blower->blowerAlarmed))
        return true;
    return false;
}

// тут проверяются узлы которые являются общими для всех (например насос воды используется 8 блоками, поэтому тут проверяем если он долго никому не нужен то выключаем воду)
void MainWindow::mainProgress()
{

    if (ui->pushButton_backMagnet->isDown())
        qDebug() << "down";
    // проверка подсветки
    buttonsLightCheck();

    // test
    ui->label_key->setText("Кнопка: " + QString::number((int)gpioMatirx->keyPressed));

    if (!can0->getState(Board0IN1).toBool() || can0->getState(StatePVIPowerIn).toBool())
    {// вырубили зажигание-надо готовиться к остановке (или нажали кнопку пви)
        // сделаем флуш всего
        if (!stopInProgress)
        {
            if (!can0->getState(Board0IN1).toBool())
                addLog("Ключ зажигания повернут. Начинается выключение ПВИ", WarningStatus);
            else
                addLog("Нажали кнопку выключения ПВИ. Начинается выключение ПВИ ( удерживайте кнопку )", WarningStatus);
            stopInProgress = true;
            logger->setStop(true);
            //сохраним важные параметры
            if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
                QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");

            settings->beginGroup("TOCur");
            settings->setValue("System", TOCurValues["System"]);
            settings->setValue("Engine", TOCurValues["Engine"]);
            settings->setValue("FrontEngine", TOCurValues["FrontEngine"]);
            settings->endGroup();
            settings->sync();
            // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
            removeBadSettings();
        }
    }
    else
    {
        if (stopInProgress)
            addLog("Выключение ПВИ отменено", WarningStatus);
        stopInProgress = false;
    }

    updateEngineAndRollLocks();
    processPrerollInService();

    if (superDiagMode)
    {
        updateSensorAndWarningIndicators();
        serviceDevicesHydraulicsLeftForm->updateVisual();
        serviceDevicesDKPLeftForm->updateVisual();
        serviceOtherEngineLeftForm->updateVisual();
        serviceOtherLightLeftForm->updateVisual();

        return; // ни чем не управляем пока включена диагностика!!!
    }
    if (menuMode == SettingsMode)
    {
        settingsSettingsConfigurationLeftForm->updateVisual();
    }

    // проверка существованиЯ параментров от переднего двигателя
    speedCounter++;
    voltageCounter++;
    frontRPMCounter++;
    if (frontRPMCounter > 50)
    {
        frontRPM = 0;
    }
    if (engine->online > ENGINE_ONLINE_EDGE * 10)
    {
        if (ui->label_engineTemp->text() != "n/a")
            ui->label_engineTemp->setText("n/a");
        if (ui->label_engineRPM->text() != "n/a")
            ui->label_engineRPM->setText("n/a");
    }
    else
    {
        if (ui->label_engineTemp->text() != QString::number(engine->engineCoolantTemp))
            ui->label_engineTemp->setText(QString::number(engine->engineCoolantTemp));
        if (ui->label_engineRPM->text() != QString::number(engine->getRpm()))
            ui->label_engineRPM->setText(QString::number(engine->getRpm()));
    }
//    if (speedCounter > 50)
//    {
//        if (ui->label_speed->text() != "n/a")
//            ui->label_speed->setText("n/a");
//        vehicleSpeed = 199; // ставим большую скорость на всякий случай (вдруг показания пропали, тогда вода перестанет подаваться)
//    }
//    if (voltageCounter > 50)
//    {
//        if (ui->label_voltage->text() != "n/a")
//            ui->label_voltage->setText("n/a");
//    }

    // проверка перегрева двигателя
//    if (engine->engineCoolantTemp > engineTempCritValue)
//    {
//        if (!engineTempCrit)
//            addLog("Двигатель перегрелся!!!", FatalStatus);
//        engineTempCrit = true;
//        //gp->Set_GPIO_State(OUT_IGNITION, 0);
//        can0->setState(StateIgnitionOut, false);
//        //blockEngineScreen->hide();
//    }
//    else if (engine->engineCoolantTemp > engineTempWarnValue)
//    {
//        if (!engineTempWarn)
//        {
//            engineTempCrit = false;
//            addLog("Двигатель перегревается", WarningStatus);
//            engineTempWarnTimer = engineTempWarnEdge * 60; // столько секунд будем ждать прежде чем загасить двигатель
//        }
//        engineTempWarn = true;
//        // покажем блокировочное окно с повышенной температурой
//        if (engine->getRpm() > 700 && engineTempWarnTimer > 0)// && !blockEngineScreen->isVisible())
//        {// если двигло работает то можно показать блокировщик
////            blockEngineScreen->label_blockScreenText->setText("Двигатель перегрелся. Уборка остановлена. Двигатель охлаждается. Пожалуйста подождите");
////            blockEngineScreen->show();
//        }
//        if (engine->getRpm() > 700 && engineTempWarnTimer == 0)
//        {// все еще перегрет
//            if (!engineTempCrit)
//                addLog("Двигатель не смог охладиться!!!", FatalStatus);
//            engineTempCrit = true;
////            blockEngineScreen->hide();
//            can0->setState(StateIgnitionOut, false);
//        }
////        else if (blockEngineScreen->isVisible())
////            blockEngineScreen->hide();
//        if (startClean)
//            on_pushButton_startstop_clicked();
//    }
//    else
//    {
//        engineTempWarnTimer = 0;
////        blockEngineScreen->hide();
//        engineTempCrit = false;
//        engineTempWarn = false;
//    }
    // проверка перегрева гидросистемы
//    if (ui->progressBar_hydro_temp->value() > ui->progressBar_hydro_temp->crit)
//    {
//        if (!hydroTempCrit)
//            addLog("Гидросистема перегрелась!!!", FatalStatus);
//        hydroTempCrit = true;
//    }
//    else if (ui->progressBar_hydro_temp->value() > ui->progressBar_hydro_temp->warn)
//    {
//        hydroTempCrit = false;
//        if (!hydroTempWarn)
//            addLog("Гидросистема перегрелась", WarningStatus);
//        hydroTempWarn = true;
//    }
//    else
//    {
//        hydroTempCrit = false;
//        hydroTempWarn = false;
//    }

    if (menuMode != DiagMode && menuMode != SettingsMode)
    {// в режиме диагностики не умничаем, в остальных случаях пробуем понять что сейчас не используется и выключить это
        if (!startClean && inHomeState() && !serviceMainRightForm->isVisible())
        {
            // вроде как не надо чтобы горели маяки
//            if (gpio->getInput(GPIOInput::IN_AVAR))
//            {
//            }

            // а так же вырубим коробки отбора мощности
            //if (!gp->GPIO[IN_LEFT_DOWN] && !gp->GPIO[IN_LEFT_UP] && !KVControl && !gp->GPIO[IN_RIGHT_DOWN] && !gp->GPIO[IN_RIGHT_UP])
            if (!KVControl)
            {// только если не заняты работой от кнопок с пульта
                can0->setState(StateValveA1, false);
            }
        }
        // доабвил с аэродрома опасно
        if (!canStart() && ui->pushButton_startstop->isEnabled())
        {// серим старт
            //pushbuttonStartStopEffect->setOpacity(0.2);
            ui->pushButton_startstop->setEnabled(false);
        }
        else if (canStart() && !ui->pushButton_startstop->isEnabled())
        {
            //pushbuttonStartStopEffect->setOpacity(1.0);
            if (isIdleMode())
                ui->pushButton_startstop->setEnabled(true);
        }
//        else if (isIdleMode() && !ui->pushButton_startstop->isEnabled())
//        {// восстанавливаем доступность запуска
//            //pushbuttonStartStopEffect->setOpacity(1.0);
//            ui->pushButton_startstop->setEnabled(true);
//        }

        if (blower->getState() <= Blower::BlowerStates::BlowerDowned
                && broomCentral->getState() <= CentralBroom::BroomStates::BroomDowned)
        {// если модули не крутятся - выключаем распределитель и убавляем обороты
            // устанавливыаем обороты чтобы двигло зря не работал
            quint16 rpm = rpmNone * 8;
            canForEngine->setEngineCommand(rpm);
            //  вроде никто не работет и наверное никому не пригодится распределитель бункера
            //can0->setState(StateValveA1, false);
        }
    }

    // отрисуем в статусной строке общие параметры (насосы, распределители и пр.)
    showStatus();

    // цикл обслуживания командных gpio
    // смотрим нажат ли кто и реагируем соответствующе
    // проверка отключения зажигания (еасли нажали кнопку на пульте)
    showPultOffIgnition();
    showStartClean();
    showModeButton();
    showMatrixFRMButton();
    // работа со стартером
    showStarter();
    // свет
    showFRM();

    // отображаем и отрабатываем нажатие кнопок на экране во время работ
    bool transitioning = isOrgansTransitioning();
    if (!transitioning)
    {
        if (organsWereTransitioning && !pauseActive)
        {
            if (startClean)
                addLog("Органы разложены — управление разблокировано", InfoStatus);
            else
                addLog("Органы сложены — можно начинать движение", InfoStatus);
        }
        // алиасы
        // щетки  - при неактивной программе выбирают левую правую щетку. при активной программе меняют обороты щетки
        showCentralBroomLeft();
        showCentralBroomRight();
        // отвал - при неактивной проге выбирают лево право отвал. при активной двигает отвалом (при удержании isDown)
        showDumpLeft();
        showDumpRight();
        // продувка - при неактивной прое выбирает обдув лево право. в активной проге ничего не делает - опасно
        showBlower();
    }
    organsWereTransitioning = transitioning;

    showPauseButton();

    // если нажата аварийка или грибок питания то завершаем все
    if ((can0->getState(StateAlarmIn).toBool()) && startClean)
    {
        if (can0->getState(StateAlarmIn).toBool())
            addLog("Нажата аварийная кнока", WarningStatus);
        if (startClean)
            on_pushButton_startstop_clicked();
        can0->setState(StateIgnitionOut, false);
        ignitionOffTimer = 0;
    }

    //защита по скорости - если едем слишком быстро надо выключать режим работы (скорость 50 условная - обозначает что нет данных от двигателя)
    if (vehicleSpeed > disableCleanSpeed && vehicleSpeed != 199 && vehicleSpeed < 200 && startClean)
    {
        on_pushButton_startstop_clicked();
        addLog("Превышена скорость уборки. Останавливаем уборку", WarningStatus);
    }
}

void MainWindow::addLog(QString text, LogStatus logStatus)
{
    qDebug() << "add Log" << text;
    QString log_text = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
    QColor color;
    if (logStatus == InfoStatus)
    {
        color.setRgb(255, 255, 255);
        log_text += " [ INFO ]";
    }
    if (logStatus == WarningStatus)
    {
        color.setRgb(255, 223, 0);
        log_text += " [ WARN ]";
    }
    if (logStatus == FatalStatus)
    {
        color.setRgb(255, 0, 0);
        log_text += " [ FAIL ]";
    }

    messageList->addMessage(text,
                            color,
                            QPixmap(),
                            QDateTime::currentDateTime());

    logger->addLogText(log_text + " " + text + '\n' + '\r');
}

void MainWindow::stopStarterOutput()
{
    gpio->setOutput(GPIOOutput::OUT_STARTER, false);
    gpio->setOutput(GPIOOutput::OUT_STARTER_LIGHT, false);
}

void MainWindow::stopRollOutput()
{
    can0->setState(StateStarterRoll, false);
}

bool MainWindow::inStarterPause() const
{
    if (!starterPauseActive)
        return false;
    const int passed = qAbs(starterPauseStartedAt.secsTo(QDateTime::currentDateTime()));
    //qDebug()<<"timer: "<<passed<<"   targetTime: "<<starterPauseSec;
    return passed < starterPauseSec;
}

bool MainWindow::inRollPause() const
{
    if (!rollPauseActive)
        return false;

    const int passed = qAbs(rollPauseStartedAt.secsTo(QDateTime::currentDateTime()));
    return passed < rollPauseSec;
}

int MainWindow::starterPauseSecondsLeft() const
{
    if (!starterPauseActive)
        return 0;
    const int passed = qAbs(starterPauseStartedAt.secsTo(QDateTime::currentDateTime()));
    return qMax(0, starterPauseSec - passed);
}

int MainWindow::rollPauseSecondsLeft() const
{
    if (!rollPauseActive)
        return 0;
    const int passed = qAbs(rollPauseStartedAt.secsTo(QDateTime::currentDateTime()));
    return qMax(0, rollPauseSec - passed);
}

bool MainWindow::starterBlocked() const
{
    return starterLockedByRoll
            || starterLockedByTemperature
            || starterLockedByEmergency
            || starterNeedReboot
            || engine->waitOnStart;
}

bool MainWindow::rollBlocked() const
{
    return rollLockedByTemperature
            || rollLockedByEmergency
            || rollNeedReboot;
}

void MainWindow::updateIndicatorPixmap(QLabel* label, const QString& colorName, const QString& baseName)
{
    const QString iconPath = ":/Images/Images/main/signs/sign_" + baseName + "_" + colorName + "_stub.png";
    label->setPixmap(QPixmap(iconPath));
}

void MainWindow::updateEngineAndRollLocks()
{
    // Теплореле: активно (true) = двигатель достаточно прогрет
    const bool heatRelayActive = can0->getState(StateHeatRele).toBool();
    // Температура учитывается только при живом CAN
    const bool engineTempValid = engine->coolantTempEverReceived
                                 && (engine->online <= ENGINE_ONLINE_EDGE * 10);
    // Температура двигателя: учитываем только если движок уже хоть раз прислал данные
    const bool engineCold = engineTempValid && (engine->engineCoolantTemp < lowTempRequireWarm);

    const int daysFromLastStart = lastEngineStartDate.daysTo(QDate::currentDate());
    needRollProcedure = !rollCompleted && !disableRollRequirement && daysFromLastStart > requireRollAfterDays;
    starterLockedByRoll = needRollProcedure;

    // Блокировка по температуре: если двигатель холодный И теплореле ещё не сработало
    // Если при включении температура уже удовлетворительна — блокировки нет
    // === ИСПРАВЛЕННАЯ ЛОГИКА ТЕМПЕРАТУРНОЙ БЛОКИРОВКИ ===
    if (!disableTemperatureBlock)
    {
        // БЛОКИРОВКА: двигатель холоден по CAN и теплореле ещё не замкнуто
        if (engineCold && !heatRelayActive)
        {
            starterLockedByTemperature = true;
            rollLockedByTemperature = true;
        }
        // РАЗБЛОКИРОВКА: ТОЛЬКО когда сработало физическое теплореле
        else if (heatRelayActive)
        {
            starterLockedByTemperature = false;
            rollLockedByTemperature = false;
        }
        // РАЗБЛОКИРОВКА: данные с CAN недостоверны (пропал) или двигатель уже прогрет
        else
        {
            starterLockedByTemperature = false;
            rollLockedByTemperature = false;
        }
        // Если по CAN уже "тепло", но теплореле ещё не замкнулось —
        // блокировка остаётся висеть (не сбрасываем здесь!)
    }
    else
    {
        starterLockedByTemperature = false;
        rollLockedByTemperature = false;
    }
    // === КОНЕЦ ИСПРАВЛЕНИЯ ===

    const bool waterAlarm = can0->getState(StateWaterSensor).toBool() && waterSensorEmergencyMode;
    const bool airAlarm = can0->getState(StateAirFilterBad).toBool() && airFilterEmergencyMode;
    const bool oilAlarm = can0->getState(StateOilFilterBad).toBool();
    starterLockedByEmergency = !ignoreAllEmergency && (waterAlarm || airAlarm || oilAlarm);
    rollLockedByEmergency = !ignoreAllEmergency && (waterAlarm || airAlarm || oilAlarm);

    if (needRollProcedure && !logNeedRollShown)
    {
        addLog("Требуется прокрутка вспомогательного ДВС", WarningStatus);
        logNeedRollShown = true;
    }
    if (!needRollProcedure)
    {
        logNeedRollShown = false;
    }

    if (starterLockedByTemperature && !logNeedWarmShown)
    {
        addLog("Требуется прогрев вспомогательного ДВС", WarningStatus);
        logNeedWarmShown = true;
    }
    if (!starterLockedByTemperature)
    {
        logNeedWarmShown = false;
    }
}

void MainWindow::processPrerollInService()
{
    QPushButton* prerollButton = serviceOtherEngineLeftForm->findChild<QPushButton*>("pushButton_preroll");
    QPushButton* starterPrerollButton = serviceOtherEngineLeftForm->findChild<QPushButton*>("pushButton_starterPreroll");
    QLabel* statusLabel = serviceOtherEngineLeftForm->findChild<QLabel*>("label_prerollStatus");

    const bool serviceEngineVisible = superDiagMode && serviceOtherEngineLeftForm->isVisible();
    serviceIgnitionAutoRestoreBlocked = serviceEngineVisible || prerollSequenceActive || rollRunActive;

    if (starterPrerollButton != NULL)
        starterPrerollButton->setEnabled(prerollStarterUnlocked && !rollNeedReboot && !rollBlocked());

    // Обновляем состояние кнопки ПРОКРУТКА (зафиксирована когда идёт подготовка или активна)
    if (prerollButton != NULL)
        prerollButton->setChecked(prerollSequenceActive || prerollStarterUnlocked);

    // Обновляем статусную строку
    if (statusLabel != NULL)
    {
        QString statusText;
        if (rollNeedReboot)
            statusText = "Лимит попыток исчерпан. Требуется перезагрузка пульта.";
        else if (rollRunActive)
        {
            const int elapsed = qAbs(rollRunStartedAt.secsTo(QDateTime::currentDateTime()));
            statusText = QString("Прокрутка активна... %1 сек. | Попытка %2/%3")
                .arg(elapsed).arg(rollAttemptsUsed).arg(rollMaxAttempts);
        }
        else if (inRollPause())
            statusText = QString("Пауза между попытками: %1 сек.").arg(rollPauseSecondsLeft());
        else if (prerollSequenceActive)
            statusText = "Подготовка прокрутки (выключение зажигания)...";
        else if (prerollStarterUnlocked)
            statusText = "Готово — нажмите СТАРТЕР ПРОКРУТКА для прокрутки";
        else if (rollBlocked())
        {
            QStringList reasons;
            if (rollLockedByTemperature)    reasons << "холодный двигатель (ждите теплореле)";
            if (rollLockedByEmergency)      reasons << "аварийный режим";
            statusText = "Прокрутка заблокирована: " + reasons.join(", ");
            if (statusLabel->styleSheet() != "color: red;")
                statusLabel->setStyleSheet("color: red;");
        }
        else if (needRollProcedure)
            statusText = "Требуется прокрутка. Нажмите ПРОКРУТКА для подготовки.";
        else if (rollCompleted)
            statusText = "Прокрутка успешно завершена";
        else
            statusText = "";

        if (rollBlocked() && !statusText.isEmpty())
        {
            if (statusLabel->styleSheet() != "color: red;")
                statusLabel->setStyleSheet("color: red;");
        }
        else
        {
            if (statusLabel->styleSheet() != "color: yellow;")
                statusLabel->setStyleSheet("color: yellow;");
        }

        if (statusLabel->text() != statusText)
            statusLabel->setText(statusText);
    }

    const bool prerollPressed = serviceEngineVisible && prerollButton != NULL && prerollButton->isDown();
    const bool prerollPressedEdge = prerollPressed && !prerollButtonPrev;

    const bool prerollStarterPressed = serviceEngineVisible && starterPrerollButton != NULL && starterPrerollButton->isDown();
    const bool prerollStarterPressedEdge = prerollStarterPressed && !prerollStarterButtonPrev;

    const bool rollInputPressed = can0->getState(StateRollIn).toBool();
    const bool rollInputPressedEdge = rollInputPressed && !rollInputPrev;

    if (prerollPressedEdge)
    {
        if (prerollSequenceActive || prerollStarterUnlocked)
        {
            // ОТМЕНА: выходим из режима прокрутки, восстанавливаем зажигание
            prerollSequenceActive = false;
            prerollStarterUnlocked = false;
            stopRollOutput();
            can0->setState(StateStarterAllow, false);
            restoreIgnitionAfterRoll();
            addLog("Режим прокрутки отменён", InfoStatus);
        }
        else if (rollBlocked())
        {
            addLog("Прокрутка заблокирована", WarningStatus);
        }
        else
        {
            addLog("Запуск алгоритма прокрутки", WarningStatus);
            prerollSequenceActive = true;
            prerollSequenceStep = 1;
            prerollStepStartedAt = QDateTime::currentDateTime();
            prerollStarterUnlocked = false;
            stopRollOutput();
            can0->setState(StateStarterAllow, false);
            can0->setState(StateIgnitionOut, false);
            ignitionOffTimer = 0;
        }
    }

    if (prerollSequenceActive)
    {
        const int elapsed = qAbs(prerollStepStartedAt.secsTo(QDateTime::currentDateTime()));
        if (prerollSequenceStep == 1 && elapsed >= 2)
        {
            can0->setState(StateStarterAllow, true);
            prerollSequenceStep = 2;
            prerollStepStartedAt = QDateTime::currentDateTime();
        }
        else if (prerollSequenceStep == 2 && elapsed >= 1)
        {
            prerollStarterUnlocked = true;
            prerollSequenceActive = false;
            addLog("Прокрутка подготовлена", InfoStatus);
        }
    }

    if (rollPauseActive && !inRollPause())
    {
        rollPauseActive = false;
        rollPauseWarned = false;
    }

    const bool rollStartRequest = prerollStarterPressedEdge || rollInputPressedEdge;
    if (rollStartRequest && !rollRunActive)
    {
        if (rollBlocked())
        {
            addLog("Прокрутка заблокирована", WarningStatus);
        }
        else if (inRollPause())
        {
            addLog("Пауза между пусками " + QString::number(rollPauseSecondsLeft()) + " секунды осталось", WarningStatus);
            rollPauseWarned = true;
        }
        else if (!prerollStarterUnlocked && !rollInputPressedEdge)
        {
            addLog("Сначала выполните подготовку прокрутки", WarningStatus);
        }
        else
        {
            rollRunActive = true;
            rollRunStartedAt = QDateTime::currentDateTime();
            rollAttemptsUsed++;
            stopStarterOutput();
            can0->setState(StateStarterRoll, true);
            addLog("Стартер прокрутка включен", WarningStatus);
        }
    }

    if (rollRunActive)
    {
        // Прокрутка работает только пока оператор удерживает кнопку/вход
        const bool rollButtonStillPressed = prerollStarterPressed || rollInputPressed;
        if (!rollButtonStillPressed)
        {
            stopRollOutput();
            rollRunActive = false;
            addLog("Прокрутка остановлена оператором", InfoStatus);
            // Добровольная остановка — не считаем за неудачу, паузу не запускаем
        }
        else
        {
            const bool oilRele = !can0->getState(StateOilRele).toBool();
            const int elapsedRoll = qAbs(rollRunStartedAt.secsTo(QDateTime::currentDateTime()));
            if (oilRele)
            {
                stopRollOutput();
                rollRunActive = false;
                rollPauseActive = false;
                rollNeedReboot = false;
                rollAttemptsUsed = 0;
                prerollStarterUnlocked = false;
                can0->setState(StateStarterAllow, false);
                rollCompleted = true;
                needRollProcedure = false;
                starterLockedByRoll = false;
                lastEngineStartDate = QDate::currentDate();
                settings->setValue("Engine/lastStartDate", lastEngineStartDate);
                settings->sync();
                // ВОССТАНАВЛИВАЕМ ЗАЖИГАНИЕ после успешной прокрутки
                restoreIgnitionAfterRoll();


                addLog("Прокрутка завершена по реле масла", InfoStatus);
            }
            else if (elapsedRoll >= rollMaxWorkSec)
            {
                stopRollOutput();
                rollRunActive = false;
                rollPauseActive = true;
                rollPauseStartedAt = QDateTime::currentDateTime();
                restoreIgnitionAfterRoll();
                addLog("Долгая работа стартера", FatalStatus);
                if (rollAttemptsUsed >= rollMaxAttempts)
                {
                    rollNeedReboot = true;
                    addLog("Достигнут лимит попыток прокрутки, требуется перезагрузка пульта", FatalStatus);
                }
            }
        }
    }

    if (rollNeedReboot)
    {
        prerollStarterUnlocked = false;
        can0->setState(StateStarterAllow, false);
    }

    prerollButtonPrev = prerollPressed;
    prerollStarterButtonPrev = prerollStarterPressed;
    rollInputPrev = rollInputPressed;
    prerollButtonPrev = prerollPressed;
    prerollStarterButtonPrev = prerollStarterPressed;
    rollInputPrev = rollInputPressed;
}

void MainWindow::restoreIgnitionAfterRoll()
{
    // Восстанавливаем зажигание немедленно
    can0->setState(StateIgnitionOut, true);
    ignitionOffTimer = 0;

    // Если сервисный экран закрыт — разрешаем авто-восстановление работать штатно
    // Если открыт — оно всё равно заблокировано, но зажигание уже включено
    addLog("Зажигание восстановлено после прокрутки", InfoStatus);
}

void MainWindow::updateSensorAndWarningIndicators()
{
    const bool waterSensor = can0->getState(StateWaterSensor).toBool();
    const bool airFilter = can0->getState(StateAirFilterBad).toBool();
    const bool oilFilter = can0->getState(StateOilFilterBad).toBool();
    const bool heatRelay = !can0->getState(StateHeatRele).toBool();
    const bool lowTemperature = engine->online <= ENGINE_ONLINE_EDGE * 10 && engine->engineCoolantTemp < lowTempRequireWarm;

    if (waterSensor && !waterSensorActivePrev)
    {
        addLog("Вода в топливе текущие", WarningStatus);
        waterSensorStartedAt = TOCurValues["Engine"];
        waterSensorTimeStarted = true;
    }
    else if (!waterSensor)
    {
        waterSensorTimeStarted = false;
    }
    const bool waterRed = waterSensor && waterSensorTimeStarted
            && (TOCurValues["Engine"] - waterSensorStartedAt >= (quint32)(waterSensorRedHours * 3600));
    if (waterSensor)
    {
        updateIndicatorPixmap(ui->label_waterInFuel, waterRed ? "red" : "yellow", "water_in_fuel");
        ui->label_waterInFuel->show();
    }
    else
    {
        ui->label_waterInFuel->hide();
    }

    if (airFilter && !airFilterActivePrev)
    {
        addLog("Засорение воздушного фильтра", WarningStatus);
        airFilterStartedAt = TOCurValues["Engine"];
        airFilterTimeStarted = true;
    }
    else if (!airFilter)
    {
        airFilterTimeStarted = false;
    }
    const bool airRed = airFilter && airFilterTimeStarted
            && (TOCurValues["Engine"] - airFilterStartedAt >= (quint32)(airFilterRedHours * 3600));
    if (airFilter)
    {
        updateIndicatorPixmap(ui->label_airFilter, airRed ? "red" : "yellow", "air_filter");
        ui->label_airFilter->show();
    }
    else
    {
        ui->label_airFilter->hide();
    }

    if (oilFilter && !oilFilterActivePrev)
    {
        addLog("Засорение масляного фильтра", FatalStatus);
    }
    if (oilFilter)
    {
        updateIndicatorPixmap(ui->label_oilFilter, "red", "oil_filter");
        ui->label_oilFilter->show();
        can0->setState(StateIgnitionOut, false);
        ignitionOffTimer = 0;
    }
    else
    {
        ui->label_oilFilter->hide();
    }

    if (heatRelay && !heatRelayActivePrev)
    {
        addLog("Требуется прогрев вспомогательного ДВС", WarningStatus);
    }

    if (heatRelay || lowTemperature)
    {
        updateIndicatorPixmap(ui->label_engineLowTemperature, "yellow", "engine_low_temperature");
        ui->label_engineLowTemperature->show();
    }
    else
    {
        ui->label_engineLowTemperature->hide();
    }

    waterSensorActivePrev = waterSensor;
    airFilterActivePrev = airFilter;
    oilFilterActivePrev = oilFilter;
    heatRelayActivePrev = heatRelay;
}

void MainWindow::showStatus()
{
    updateEngineAndRollLocks();

    // wait_on_start
    if (engine->waitOnStart && !ui->label_wait_on_start->isVisible())
    {
        addLog("Требуется прогрев двигателя", MainWindow::WarningStatus);
        ui->label_wait_on_start->show();
    }
    else if (!engine->waitOnStart && ui->label_wait_on_start->isVisible())
    {
        addLog("Двигатель прогрет", MainWindow::InfoStatus);
        ui->label_wait_on_start->hide();
    }
    // повлеждекние двигателя
    if ((engine->damage == 2 || engine->damage == 1) && !ui->label_engine_damage->isVisible() && showCheckEngine)
    {
        addLog("Двигателю требуется обслуживание", MainWindow::FatalStatus);
        addLog("Код ошибки SPM=" + QString::number(engine->DM01SPNValue) + " FMI=" + QString::number(engine->DM01FMIValue), MainWindow::FatalStatus);
        ui->label_engine_damage->show();
    }
    else if ((engine->damage == 0 || !showCheckEngine) && ui->label_engine_damage->isVisible())
    {
        ui->label_engine_damage->hide();
    }
    // клапан а1
    if (can0->getState(StateValveA1).toBool() && !ui->label_a1->isVisible())
    {
//        addLog("Засорен напорный фильтр", MainWindow::WarningStatus);
        ui->label_a1->show();
    }
    else if (!can0->getState(StateValveA1).toBool() && ui->label_a1->isVisible())
    {
        ui->label_a1->hide();
    }
    // напорный фильтр
    if ((can0->getState(StatePressureFilter1).toBool() || can0->getState(StatePressureFilter1).toBool() || can0->getState(StatePressureFilter3).toBool()) && !ui->label_pressure_filter->isVisible())
    {
        addLog("Засорен напорный фильтр", MainWindow::WarningStatus);
        ui->label_pressure_filter->show();
    }
    else if (!(can0->getState(StatePressureFilter1).toBool() || can0->getState(StatePressureFilter1).toBool() || can0->getState(StatePressureFilter3).toBool()) && ui->label_pressure_filter->isVisible())
    {
        ui->label_pressure_filter->hide();
    }
    // сливной фильтр
    if (can0->getState(StateDrainFilterD28).toBool() && !ui->label_drain_filter->isVisible())
    {
        addLog("Засорен сливной фильтр", MainWindow::WarningStatus);
        ui->label_drain_filter->show();
    }
    else if (!can0->getState(StateDrainFilterD28).toBool() && ui->label_drain_filter->isVisible())
    {
        ui->label_drain_filter->hide();
    }

    updateSensorAndWarningIndicators();
}

void MainWindow::showPultOffIgnition()
{
    if (serviceIgnitionAutoRestoreBlocked)
        return;

    ignitionOffTimer++;
    if (ignitionOffTimer == restartIgnitionDelay * 10)
    {
        startIgnition();
    }
}

void MainWindow::showFRM()
{
    // frm кунг
    if (workMode.frmKung && ui->pushButton_frmKung->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_k_on.png);")
        ui->pushButton_frmKung->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_k_on.png);");
    else if (!workMode.frmKung && ui->pushButton_frmKung->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_k_off.png);")
        ui->pushButton_frmKung->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_k_off.png);");
    if (workMode.frmKung)
        can0->setState(StateKungL5, true);
    else
        can0->setState(StateKungL5, false);

    // frm щетка
    if (workMode.frmBroom && ui->pushButton_frmBroom->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_h_on.png);")
        ui->pushButton_frmBroom->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_h_on.png);");
    else if (!workMode.frmBroom && ui->pushButton_frmBroom->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_h_off.png);")
        ui->pushButton_frmBroom->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_h_off.png);");
    if (workMode.frmBroom)
        can0->setState(StateFRMBroomL1, true);
    else
        can0->setState(StateFRMBroomL1, false);

    // frm магнит
    if (workMode.frmMagnet && ui->pushButton_frmMagnet->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_m_on.png);")
        ui->pushButton_frmMagnet->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_m_on.png);");
    else if (!workMode.frmMagnet && ui->pushButton_frmMagnet->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_m_off.png);")
        ui->pushButton_frmMagnet->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_m_off.png);");
    if (workMode.frmMagnet)
        can0->setState(StateFRMBackL2, true);
    else
        can0->setState(StateFRMBackL2, false);
}

void MainWindow::showStarter()
{
    if (engine->getRpm() < 500 && engineStartedOk)
        addLog("Двигатель заглох!!!", FatalStatus);

    if (inStarterPause() == false)
    {
        starterPauseActive = false;
        starterPauseWarned = false;
    }

    const bool starterPressed = gpioMatirx->keyPressed == GPIOInput::IN_STARTER;
    const bool starterPressedEdge = starterPressed && !starterButtonPrev;

    // БЛОКИРОВКА: пока идёт прокрутка — кнопка стартера не управляет зажиганием
    if (rollRunActive || prerollSequenceActive)
    {
        starterButtonPrev = starterPressed;
        return;
    }

    const bool engineRunning = engine->getRpm() > 700;

    if (engineRunning)
    {
        stopStarterOutput();
        starterStarted = false;
    }

    if (starterPressedEdge && engineRunning)
    {
        // Повторное нажатие при работающем двигателе — глушим ДВС
        can0->setState(StateIgnitionOut, false);
        ignitionOffTimer = 0;
        engineStartedOk = false;
        starterPauseActive = true;
        addLog("Повторное нажатие старт/стоп: выключаем зажигание", WarningStatus);
    }
    else if (starterPressed)
    {
        can0->setState(StateIgnitionOut, true);

        if (!engineRunning)
        {
            if (starterNeedReboot)
            {
                if (starterPressedEdge)
                    addLog("Достигнут лимит попыток запуска, требуется перезагрузка пульта", FatalStatus);
                stopStarterOutput();
                starterStarted = false;
            }
            else if (starterBlocked())
            {
                if (starterPressedEdge)
                {
                    if (starterLockedByRoll)
                        addLog("Требуется прокрутка вспомогательного ДВС", WarningStatus);
                    else if (starterLockedByTemperature || engine->waitOnStart)
                        addLog("Требуется прогрев вспомогательного ДВС", WarningStatus);
                    else
                        addLog("Стартер заблокирован аварийным состоянием", FatalStatus);
                }
                stopStarterOutput();
                starterStarted = false;
            }
            else if (inStarterPause())
            {
                if (starterPressedEdge || !starterPauseWarned)
                {
                    addLog("Пауза между пусками " + QString::number(starterPauseSecondsLeft()) + " секунды осталось", WarningStatus);
                    starterPauseWarned = true;
                }
                stopStarterOutput();
                starterStarted = false;
            }
            else
            {
                if (!starterStarted)
                {
                    starterStarted = true;
                    starterStartedTime = QDateTime::currentDateTime();
                    starterAttemptsUsed++;
                    addLog("Стартер включен", WarningStatus);
                    gpio->setOutput(GPIOOutput::OUT_STARTER, true);
                    gpio->setOutput(GPIOOutput::OUT_STARTER_LIGHT, true);
                }

                const int starterRunTime = qAbs(starterStartedTime.secsTo(QDateTime::currentDateTime()));
                if (engineRunning)
                {
                    stopStarterOutput();
                    starterStarted = false;
                    starterPauseActive = false;
                    starterNeedReboot = false;
                    starterAttemptsUsed = 0;
                    addLog("Двигатель набрал обороты", WarningStatus);
                }
                else if (starterRunTime >= starterMaxWorkSec)
                {
                    stopStarterOutput();
                    starterStarted = false;
                    starterPauseActive = true;
                    starterPauseStartedAt = QDateTime::currentDateTime();
                    addLog("Долгая работа стартера", FatalStatus);
                    if (starterAttemptsUsed >= starterMaxAttempts)
                    {
                        starterNeedReboot = true;
                        addLog("Достигнут лимит попыток запуска, требуется перезагрузка пульта", FatalStatus);
                    }
                }
            }
        }
    }
    else
    {
        if (starterStarted)
        {
            stopStarterOutput();
            starterStarted = false;
            if (!engineRunning)
            {
                starterPauseActive = true;
                starterPauseStartedAt = QDateTime::currentDateTime();
                if (starterAttemptsUsed >= starterMaxAttempts)
                {
                    starterNeedReboot = true;
                    addLog("Достигнут лимит попыток запуска, требуется перезагрузка пульта", FatalStatus);
                }
            }
        }
    }

    if (engineRunning)
    {
        engineStartedOk = true;
        starterNeedReboot = false;
        starterAttemptsUsed = 0;
    }
    else
    {
        engineStartedOk = false;
    }

    starterButtonPrev = starterPressed;
}

void MainWindow::showStartClean()
{
    if (gpioMatirx->keyPressed == GPIOInput::IN_STARTCLEAN)
    {
        startCleanTimeCounter++;
    }
    else
    {
        if (startCleanTimeCounter > 1)
            on_pushButton_startstop_clicked();
        startCleanTimeCounter = 0;
    }
}

void MainWindow::showModeButton()
{
    if (gpioMatirx->keyPressed == GPIOInput::IN_MODE_LEFT)
    {// нажали кнопку
        leftModeTimeCounter++;
    }
    else
    {// отжата кнопка (и ее нажимали до этого) и это не длительное нажатие
        if (leftModeTimeCounter > 1)
        {
            // отработаем нажатие
            if (workMode.sweepType == LightSweep)
                on_pushButton_leafSweep_clicked();
            else if (workMode.sweepType == MediumSweep)
                on_pushButton_lightSweep_clicked();
            else if (workMode.sweepType == HeavySweep)
                on_pushButton_mediumSweep_clicked();
        }
        leftModeTimeCounter = 0;
    }
    if (gpioMatirx->keyPressed == GPIOInput::IN_MODE_RIGHT)
    {// нажали кнопку
        rightModeTimeCounter++;
    }
    else
    {// отжата кнопка (и ее нажимали до этого) и это не длительное нажатие
        if (rightModeTimeCounter > 1)
        {
            // отработаем нажатие
            if (workMode.sweepType == LeafSweep)
                on_pushButton_lightSweep_clicked();
            else if (workMode.sweepType == LightSweep)
                on_pushButton_mediumSweep_clicked();
            else if (workMode.sweepType == MediumSweep)
                on_pushButton_heavySweep_clicked();
        }
        rightModeTimeCounter = 0;
    }
}

void MainWindow::showMatrixFRMButton()
{
    if (gpioMatirx->keyPressed == GPIOInput::IN_FRM)
    {
        frmTimeCounter++;
    }
    else
    {
        if (frmTimeCounter > 1)
            toggleAllFrm();
        frmTimeCounter = 0;
    }
}

// нажали пуск - запускаем все выбранные устройства
void MainWindow::on_pushButton_startstop_clicked()
{
    if (startClean)
    {// или отжали или был выбран режим защиты от неприятностей
        if (pauseActive)
        {// снимаем паузу, уборка продолжается
            pauseActive = false;
            addLog("Пауза снята", InfoStatus);
            showWorkMode();
            return;
        }
        addLog("Уборка окончена", WarningStatus);
        startClean = false;
    }
    else
    {
        addLog("Уборка начата", WarningStatus);
        startClean = true;
    }
    showWorkMode();
}


void MainWindow::settingsAskPassword()
{
    if (DEVELOPER_MODE)
        Password_accepted = true;
    if (!Password_accepted)
    {
        Password_Form *Password_window = new Password_Form (this, true, true);
        Password_window->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
        Password_window->setAttribute(Qt::WA_DeleteOnClose,true);

        connect(this,SIGNAL(Pass_close()),Password_window,SLOT(close()));
        connect(this,SIGNAL(Send_Pass_2_pass_form(int)),Password_window,SLOT(Recieve_pass_name(int)));
        connect(this,SIGNAL(Send_SecretPass_2_pass_form(int)),Password_window,SLOT(Recieve_secret_pass_name(int)));
        connect(Password_window,SIGNAL(Send_correct(int)),this,SLOT(passwordSettingsOk(int)));

        settings->beginGroup("Global");
        emit Send_Pass_2_pass_form(settings->value("password").toInt());
        emit Send_SecretPass_2_pass_form(settings->value("secretPassword").toInt());
        settings->endGroup();
        Password_window->show();
    }
    else
    {
        Password_accepted = false;
        menuMode = SettingsMode;
        //superDiagMode = true;
        //menuMode = DiagMode;
        settingsForm->fillElements();
        serviceSetingsName->show();
        settingsMainRightForm->show();

        return;
    }
}

void MainWindow::diagAskPassword()
{
    if (DEVELOPER_MODE)
        Password_accepted = true;
    if (!Password_accepted)
    {
        Password_Form *Password_window = new Password_Form (this, true, false);
        Password_window->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
        Password_window->setAttribute(Qt::WA_DeleteOnClose,true);

        connect(this,SIGNAL(Pass_close()),Password_window,SLOT(close()));
        connect(this,SIGNAL(Send_Pass_2_pass_form(int)),Password_window,SLOT(Recieve_pass_name(int)));
        connect(this,SIGNAL(Send_SecretPass_2_pass_form(int)),Password_window,SLOT(Recieve_secret_pass_name(int)));
        connect(Password_window,SIGNAL(Send_correct(int)),this,SLOT(passwordDiagOk(int)));

        settings->beginGroup("Global");
        emit Send_Pass_2_pass_form(settings->value("passwordDiag").toInt());
        emit Send_SecretPass_2_pass_form(settings->value("secretPasswordDiag").toInt());
        settings->endGroup();
        Password_window->show();
    }
    else
    {
        Password_accepted = false;
        superDiagMode = true;
        menuMode = DiagMode;
        //serviceSetingsName->show();
        serviceMainRightForm->show();
    }
}

void MainWindow::passwordDiagOk(int pass)
{
    if (pass == readSettingsValue("Global/secretPasswordDiag").toString().toInt())
    {// сбросим одноразовы пароль
        if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
            QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");
        int random = std::rand() % ((9999 + 1) - 1) + 1;
        //qDebug() << random;
        settings->setValue("Global/secretPasswordDiag", random);// рандом от 1 до 9999
        settings->sync();
        system("sync");
        // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
        removeBadSettings();
    }
    Password_accepted = true;
    diagAskPassword();
}

void MainWindow::passwordSettingsOk(int pass)
{
    if (pass == readSettingsValue("Global/secretPassword").toString().toInt())
    {// сбросим одноразовы пароль
        if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
            QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");
        int random = std::rand() % ((9999 + 1) - 1) + 1;
        //qDebug() << random;
        settings->setValue("Global/secretPassword", random);// рандом от 1 до 9999
        settings->sync();
        system("sync");
        // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
        removeBadSettings();
    }
    Password_accepted = true;
    settingsAskPassword();
}

void MainWindow::serviceClosed()
{// не работает
    superDiagMode = false;
    menuMode = SweepMode;
}

void MainWindow::settingsClosed()
{// нужно перезачитьать все сохраненные настройки
    broomCentral->readSettings();
    backMagnet->readSettings();
    frontRail->readSettings();
    blower->readSettings();
    readSettings();
    canForEngine->setEngineAddr(enigneAddr);
}

void MainWindow::showWorkMode()
{
    if (!pauseActive)
    {
        // задаем режимы органам
        // щетка
        if (workMode.centralBroomLeft||workMode.centralBroomRight)
        {
            broomCentral->setNeedState(CentralBroom::BroomFlowed);
            broomCentral->choosed = true;
        }
        else
        {
            broomCentral->setNeedState(CentralBroom::BroomOff);
            broomCentral->choosed = false;
        }
        if (workMode.centralBroomLeft)
        {
            broomCentral->needSlided = true;
        }
        else
        {
            broomCentral->needSlided = false;
        }

        // отвал
        if (workMode.frontDumpLeft||workMode.frontDumpRight)
        {
            frontRail->setNeedState(FrontRail::FrontRailFlowed);
            frontRail->choosed = true;
        }
        else
        {
            frontRail->setNeedState(FrontRail::FrontRailOff);
            frontRail->choosed = false;
        }
        if (workMode.frontDumpLeft)
        {
            frontRail->needSlided = true;
        }
        else
        {
            frontRail->needSlided = false;
        }

        // дулка
        if (workMode.blowLeft||workMode.blowRight)
        {
            blower->setNeedState(Blower::BlowerRotated);
            blower->choosed = true;
        }
        else
        {
            blower->setNeedState(Blower::BlowerOff);
            blower->choosed = false;
        }

        // магнит
        if (workMode.backMagnet)
        {
            backMagnet->setNeedState(BackMagnet::BackMagnetDowned);
            backMagnet->choosed = true;
        }
        else
        {
            backMagnet->setNeedState(BackMagnet::BackMagnetOff);
            backMagnet->choosed = false;
        }

        // Применяем режимы к уже разложенным органам сразу при изменении workMode.
        applyWorkModeToDeployedOrgans();
    }


    // проверим доступность кнопошков
    if (startClean)
    {
        // домашнее сервис и настройки
        if (ui->pushButton_homeState->isEnabled())
        {
            ui->pushButton_homeState->setEnabled(false);
            ui->pushButton_settings->setEnabled(false);
            ui->pushButton_service->setEnabled(false);
        }

        // дулка
        if (!ui->pushButton_blowerDown->isEnabled())
        {
            ui->pushButton_blowerDown->setEnabled(true);
            ui->pushButton_blowerUp->setEnabled(true);
        }

        // щетка
        if (!ui->pushButton_centralBroomDown->isEnabled())
        {
            qDebug()<<"show work mode: true";
            ui->pushButton_centralBroomDown->setEnabled(true);
            ui->pushButton_centralBroomUp->setEnabled(true);
        }

        // отвал
        if (!ui->pushButton_dumpDown->isEnabled())
        {
            ui->pushButton_dumpDown->setEnabled(true);
            ui->pushButton_dumpUp->setEnabled(true);
        }
    }
    else
    {
        // домашнее сервис и настройки
        if (!ui->pushButton_homeState->isEnabled())
        {
            ui->pushButton_homeState->setEnabled(true);
            ui->pushButton_settings->setEnabled(true);
            ui->pushButton_service->setEnabled(true);
        }

        // дулка
        if (ui->pushButton_blowerDown->isEnabled())
        {
            ui->pushButton_blowerDown->setEnabled(false);
            ui->pushButton_blowerUp->setEnabled(false);
        }
        if (!ui->pushButton_blowerLeft->isEnabled())
        {
            ui->pushButton_blowerLeft->setEnabled(true);
            ui->pushButton_blowerRight->setEnabled(true);
        }
        // щетка
        if (ui->pushButton_centralBroomDown->isEnabled())
        {

            qDebug()<<"show work mode: false";
            ui->pushButton_centralBroomDown->setEnabled(false);
            ui->pushButton_centralBroomUp->setEnabled(false);
        }
        if (!ui->pushButton_centralBroomLeft->isEnabled())
        {
            ui->pushButton_centralBroomLeft->setEnabled(true);
            ui->pushButton_centralBroomRight->setEnabled(true);
        }
        // отвал
        if (ui->pushButton_dumpDown->isEnabled())
        {
            ui->pushButton_dumpDown->setEnabled(false);
            ui->pushButton_dumpUp->setEnabled(false);
        }
        if (!ui->pushButton_dumpLeft->isEnabled())
        {
            ui->pushButton_dumpLeft->setEnabled(true);
            ui->pushButton_dumpRight->setEnabled(true);
        }
    }
    // меняем картиночки доступности кнопок после анализа
    // дулка
    if (ui->pushButton_blowerDown->isEnabled())
    {
        if (ui->label_blowerUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_off.png);")
            ui->label_blowerUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_off.png);");
    }
    else
    {
        if (ui->label_blowerUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_blocked.png);")
            ui->label_blowerUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_blocked.png);");
    }
    // щетка
    if (ui->pushButton_centralBroomDown->isEnabled())
    {

        qDebug()<<"show work mode: true";
        if (ui->label_centralBroomUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_off.png);")
            ui->label_centralBroomUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_off.png);");
    }
    else
    {

        qDebug()<<"show work mode: false";
        if (ui->label_centralBroomUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_blocked.png);")
            ui->label_centralBroomUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_blocked.png);");
    }
    // отвал
    if (ui->pushButton_dumpDown->isEnabled())
    {
        if (ui->label_dumpUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_off.png);")
            ui->label_dumpUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_off.png);");
    }
    else
    {
        if (ui->label_dumpUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_blocked.png);")
            ui->label_dumpUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_blocked.png);");
    }

    // смет листья
    if (workMode.sweepType == LeafSweep && ui->pushButton_leafSweep->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_easy_on.png);")
        ui->pushButton_leafSweep->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_easy_on.png);");
    if (workMode.sweepType != LeafSweep && ui->pushButton_leafSweep->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_easy_off.png);")
        ui->pushButton_leafSweep->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_easy_off.png);");
    // смет легкий
    if (workMode.sweepType == LightSweep && ui->pushButton_lightSweep->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_average_on.png);")
        ui->pushButton_lightSweep->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_average_on.png);");
    if (workMode.sweepType != LightSweep && ui->pushButton_lightSweep->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_average_off.png);")
        ui->pushButton_lightSweep->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_average_off.png);");
    // смет средний
    if (workMode.sweepType == MediumSweep && ui->pushButton_mediumSweep->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_hard_on.png);")
        ui->pushButton_mediumSweep->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_hard_on.png);");
    if (workMode.sweepType != MediumSweep && ui->pushButton_mediumSweep->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_hard_off.png);")
        ui->pushButton_mediumSweep->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_hard_off.png);");
    // смет тяжелый
    if (workMode.sweepType == HeavySweep && ui->pushButton_heavySweep->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_leafHarvesting_on.png);")
        ui->pushButton_heavySweep->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_leafHarvesting_on.png);");
    if (workMode.sweepType != HeavySweep && ui->pushButton_heavySweep->styleSheet() != "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_leafHarvesting_off.png);")
        ui->pushButton_heavySweep->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/cleaningMode_button_leafHarvesting_off.png);");

    // задняя щетка
    if (workMode.centralBroomLeft && !workMode.centralBroomRight && ui->label_centralBroom->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsBelow_left_on.png);")
        ui->label_centralBroom->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsBelow_left_on.png);");
    else if (!workMode.centralBroomLeft && workMode.centralBroomRight && ui->label_centralBroom->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsBelow_right_on.png);")
        ui->label_centralBroom->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsBelow_right_on.png);");
    else if (!workMode.centralBroomLeft && !workMode.centralBroomRight && ui->label_centralBroom->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsBelow_off.png);")
        ui->label_centralBroom->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_off.png);");
    if (workMode.centralBroomFlow && workMode.centralBroomPress && ui->label_centralBroomFloatPress->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_on.png);")
        ui->label_centralBroomFloatPress->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_variable_on.png);");
    else if (workMode.centralBroomFlow && !workMode.centralBroomPress && ui->label_centralBroomFloatPress->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_up_on.png);")
        ui->label_centralBroomFloatPress->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_variable_up_on.png);");
    else if (!workMode.centralBroomFlow && workMode.centralBroomPress && ui->label_centralBroomFloatPress->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_down_on.png);")
        ui->label_centralBroomFloatPress->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_variable_down_on.png);");
    else if (!workMode.centralBroomFlow && !workMode.centralBroomPress && ui->label_centralBroomFloatPress->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_off.png);")
        ui->label_centralBroomFloatPress->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_variable_off.png);");


    // передний отвыал
    if (workMode.frontDumpLeft && !workMode.frontDumpRight && ui->label_dump->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_left_on.png);")
        ui->label_dump->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_left_on.png);");
    else if (!workMode.frontDumpLeft && workMode.frontDumpRight && ui->label_dump->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_right_on.png);")
        ui->label_dump->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_right_on.png);");
    else if (!workMode.frontDumpLeft && !workMode.frontDumpRight && ui->label_dump->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_off.png);")
        ui->label_dump->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_off.png);");
    if (workMode.frontDumpFlow && ui->label_dumpFloatPress->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_up_on_down_blocked.png);")
        ui->label_dumpFloatPress->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_variable_up_on_down_blocked.png);");
    else if (!workMode.frontDumpFlow && ui->label_dumpFloatPress->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_variable_up_off_down_blocked.png);")
        ui->label_dumpFloatPress->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_variable_up_off_down_blocked.png);");

    // магнит
    if (workMode.backMagnet && ui->label_backMagnet->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_magnet_on.png);")
        ui->label_backMagnet->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_magnet_on.png);");
    else if (!workMode.backMagnet && ui->label_backMagnet->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_magnet_off.png);")
        ui->label_backMagnet->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_magnet_off.png);");

    // дулка
    if (workMode.blowLeft && !workMode.blowRight && ui->label_blower->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_turn_left_on.png);")
        ui->label_blower->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_turn_left_on.png);");
    else if (!workMode.blowLeft && workMode.blowRight && ui->label_blower->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_turn_right_on.png);")
        ui->label_blower->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_turn_right_on.png);");
    else if (!workMode.blowLeft && !workMode.blowRight && ui->label_blower->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_turn_off.png);")
        ui->label_blower->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_turn_off.png);");

    // старт стоп
    if (startClean && ui->pushButton_startstop->styleSheet() != "outline: none;border-style:none;background-image: url(:/Images/Images/main/buttons/button_start_on.png);")
        ui->pushButton_startstop->setStyleSheet("outline: none;border-style:none;background-image: url(:/Images/Images/main/buttons/button_start_on.png);");
    else if (!startClean && ui->pushButton_startstop->styleSheet() != "outline: none;border-style:none;background-image: url(:/Images/Images/main/buttons/button_start_off.png);")
        ui->pushButton_startstop->setStyleSheet("outline: none;border-style:none;background-image: url(:/Images/Images/main/buttons/button_start_off.png);");
}

void MainWindow::on_pushButton_service_clicked()
{
    logger->addUserLogInfo(Logger::UF_SERVICE_PRESSED, 1);
    diagAskPassword();
}

void MainWindow::on_pushButton_lightSweep_clicked()
{
    if (workMode.sweepType != LightSweep)
    {
        workMode.sweepType = LightSweep;
        showWorkMode();
    }
}


void MainWindow::on_pushButton_mediumSweep_clicked()
{
    if (workMode.sweepType != MediumSweep)
    {
        workMode.sweepType = MediumSweep;
        showWorkMode();
    }
}

void MainWindow::on_pushButton_heavySweep_clicked()
{
    if (workMode.sweepType != HeavySweep)
    {// защита от поднятой щетки
        workMode.sweepType = HeavySweep;
        showWorkMode();
    }
}

void MainWindow::on_pushButton_leafSweep_clicked()
{
    if (workMode.sweepType != LeafSweep)
    {
        workMode.sweepType = LeafSweep;
        showWorkMode();
    }
}

bool MainWindow::canStart()
{
    return true;
}

float MainWindow::hydraulicPressureValue(int index) const
{
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

void MainWindow::toggleAllFrm()
{
    const bool enable = !(workMode.frmKung && workMode.frmBroom && workMode.frmMagnet);
    workMode.frmKung = enable;
    workMode.frmBroom = enable;
    workMode.frmMagnet = enable;
    showWorkMode();
}

bool MainWindow::isIdleMode()
{
    // проверяекм надо ли затенять кнопки переключения режимов
    if (blower->getState() != Blower::BlowerOff
            || broomCentral->getState() != CentralBroom::BroomOff
            || frontRail->getState() != FrontRail::FrontRailOff
            || backMagnet->getState() != BackMagnet::BackMagnetOff
            || startClean)
    {
        return false;
    }
    return true;
}

bool MainWindow::isOrgansTransitioning()
{
    // органы в процессе перехода - ручное управление заблокировано
    // при движении к работе цель ограничивается ableState, при выключении всегда идём к Off

    CentralBroom::BroomStates broomTarget = broomCentral->needState;
    if (broomCentral->needState != CentralBroom::BroomOff && broomCentral->ableState < broomCentral->needState)
        broomTarget = broomCentral->ableState;
    if (broomCentral->state != broomTarget)
        return true;

    FrontRail::FrontRailStates railTarget = frontRail->needState;
    if (frontRail->needState != FrontRail::FrontRailOff && frontRail->ableState < frontRail->needState)
        railTarget = frontRail->ableState;
    if (frontRail->state != railTarget)
        return true;

    BackMagnet::BackMagnetStates magnetTarget = backMagnet->needState;
    if (backMagnet->needState != BackMagnet::BackMagnetOff && backMagnet->ableState < backMagnet->needState)
        magnetTarget = backMagnet->ableState;
    if (backMagnet->state != magnetTarget)
        return true;

    Blower::BlowerStates blowerTarget = blower->needState;
    if (blower->needState != Blower::BlowerOff && blower->ableState < blower->needState)
        blowerTarget = blower->ableState;
    if (blower->state != blowerTarget)
        return true;

    return false;
}

void MainWindow::applyWorkModeToDeployedOrgans()
{
    // При отключенном прижиме сбрасываем поджим сразу, чтобы не оставались активные клапаны.
    if (!workMode.centralBroomPress)
        broomCentral->goPressNone();

    if (startClean && broomCentral->getState() >= CentralBroom::BroomFlowed)
    {
        if (workMode.centralBroomFlow)
            broomCentral->goFlow();
        else
            broomCentral->goNoFlow();
    }

    if (startClean && frontRail->getState() >= FrontRail::FrontRailFlowed)
    {
        if (workMode.frontDumpFlow)
            frontRail->goFlow();
        else
            frontRail->goNoFlow();
    }
}

void MainWindow::on_pushButton_settings_clicked()
{
    settingsAskPassword();
}

void  MainWindow::showCentralBroomLeft()
{
    if (startClean)
    {
        if (ui->pushButton_centralBroomUp->isDown() || gpioMatirx->keyPressed == GPIOInput::IN_BROOM_UP)
        {
            if (ui->label_centralBroomUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_up_on.png);")
                ui->label_centralBroomUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_up_on.png);");

            ui->pushButton_centralBroomUp->setProperty("wasDown", true);
            if (workMode.centralBroomPress)
                broomCentral->goPressUp();
            else
                broomCentral->goUp();
        }
        else if (ui->pushButton_centralBroomUp->property("wasDown").toBool())
        {
            if (ui->label_centralBroomUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_off.png);")
                ui->label_centralBroomUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_off.png);");

            ui->pushButton_centralBroomUp->setProperty("wasDown", false);
            if (workMode.centralBroomPress)
                broomCentral->goPressNone();
            else
                broomCentral->goNone();
        }
        if (ui->pushButton_centralBroomDown->isDown() || gpioMatirx->keyPressed == GPIOInput::IN_BROOM_DOWN)
        {
            if (ui->label_centralBroomUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_down_on.png);")
                ui->label_centralBroomUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_down_on.png);");

            ui->pushButton_centralBroomDown->setProperty("wasDown", true);
            if (workMode.centralBroomPress)
                broomCentral->goPressDown();
            else
                broomCentral->goDown();

        }
        else if (ui->pushButton_centralBroomDown->property("wasDown").toBool())
        {
            if (ui->label_centralBroomUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_off.png);")
                ui->label_centralBroomUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_off.png);");

            ui->pushButton_centralBroomDown->setProperty("wasDown", false);
            if (workMode.centralBroomPress)
                broomCentral->goPressNone();
            else
                broomCentral->goNone();
        }
    }

    if ((startClean && ui->pushButton_centralBroomLeft->isDown()) || gpioMatirx->keyPressed == GPIOInput::IN_BROOM_LEFT)
    {// нажали кнопку
        centralBroomLeftTimeCounter++;
        if (startClean)
            broomCentral->goLeft();
    }
    else
    {// отжата кнопка (и ее нажимали до этого) и это не длительное нажатие
        if (startClean && centralBroomLeftTimeCounter > 0)
            broomCentral->goNone();
        if (centralBroomLeftTimeCounter > 1)
        {
        // отработаем нажатие
            if (!startClean)
                on_pushButton_centralBroomLeft_clicked();
//            else
//                broomCentral->decreaseSpeed();
        }
        centralBroomLeftTimeCounter = 0;
    }
}

void  MainWindow::showCentralBroomRight()
{
    if ((startClean && ui->pushButton_centralBroomRight->isDown()) || gpioMatirx->keyPressed == GPIOInput::IN_BROOM_RIGHT)
    {// нажали кнопку
        centralBroomRightTimeCounter++;
        if (startClean)
            broomCentral->goRight();
    }
    else
    {// отжата кнопка (и ее нажимали до этого) и это не длительное нажатие
        if (startClean && centralBroomRightTimeCounter > 0)
            broomCentral->goNone();
        if (centralBroomRightTimeCounter > 1)
        {
            // отработаем нажатие
            if (!startClean)
                on_pushButton_centralBroomRight_clicked();
//            else
//                broomCentral->increaseSpeed();
        }
        centralBroomRightTimeCounter = 0;
    }
}

void MainWindow::showDumpLeft()
{
    if (startClean)
    {
        if (ui->pushButton_dumpUp->isDown() || gpioMatirx->keyPressed == GPIOInput::IN_DUMP_UP)
        {
            if (ui->label_dumpUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_up_off.png);")
                ui->label_dumpUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_up_off.png);");

            ui->pushButton_dumpUp->setProperty("wasDown", true);
            frontRail->goUp();
        }
        else if (ui->pushButton_dumpUp->property("wasDown").toBool())
        {
            if (ui->label_dumpUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_off.png);")
                ui->label_dumpUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_off.png);");

            ui->pushButton_dumpUp->setProperty("wasDown", false);
            frontRail->goNone();
        }
        if (ui->pushButton_dumpDown->isDown() || gpioMatirx->keyPressed == GPIOInput::IN_DUMP_DOWN)
        {
            if (ui->label_dumpUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_down_off.png);")
                ui->label_dumpUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_down_off.png);");

            ui->pushButton_dumpDown->setProperty("wasDown", true);
            frontRail->goDown();
        }
        else if (ui->pushButton_dumpDown->property("wasDown").toBool())
        {
            if (ui->label_dumpUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_off.png);")
                ui->label_dumpUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_off.png);");

            ui->pushButton_dumpDown->setProperty("wasDown", false);
            frontRail->goNone();
        }
    }

    if ((startClean && ui->pushButton_dumpLeft->isDown()) || gpioMatirx->keyPressed == GPIOInput::IN_DUMP_LEFT)
    {// нажали кнопку
        frontDumpLeftTimeCounter++;
        if (startClean)
            frontRail->goLeft();
    }
    else
    {// отжата кнопка (и ее нажимали до этого) и это не длительное нажатие
        if (startClean && frontDumpLeftTimeCounter > 0)
            frontRail->goNone();
        if (frontDumpLeftTimeCounter > 1)
        {
        // отработаем нажатие
            if (!startClean)
                on_pushButton_dumpLeft_clicked();
        }
        frontDumpLeftTimeCounter = 0;
    }
}

void MainWindow::showDumpRight()
{
    if ((startClean && ui->pushButton_dumpRight->isDown()) || gpioMatirx->keyPressed == GPIOInput::IN_DUMP_RIGHT)
    {// нажали кнопку
        frontDumpRightTimeCounter++;
        if (startClean)
            frontRail->goRight();
    }
    else
    {// отжата кнопка (и ее нажимали до этого) и это не длительное нажатие
        if (startClean && frontDumpRightTimeCounter > 0)
            frontRail->goNone();
        if (frontDumpRightTimeCounter > 1)
        {
        // отработаем нажатие
            if (!startClean)
                on_pushButton_dumpRight_clicked();
        }
        frontDumpRightTimeCounter = 0;
    }
}

void MainWindow::showBlower()
{
    if (startClean)
    {
        if (ui->pushButton_blowerUp->isDown() || gpioMatirx->keyPressed == GPIOInput::IN_BLOW_UP)
        {
            if (ui->label_blowerUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_up_on.png);")
                ui->label_blowerUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_up_on.png);");

            ui->pushButton_blowerUp->setProperty("wasDown", true);
            blower->goUp();
        }
        else if (ui->pushButton_blowerUp->property("wasDown").toBool())
        {
            if (ui->label_blowerUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_off.png);")
                ui->label_blowerUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_off.png);");

            ui->pushButton_blowerUp->setProperty("wasDown", false);
            blower->goOff();
        }
        if (ui->pushButton_blowerDown->isDown() || gpioMatirx->keyPressed == GPIOInput::IN_BLOW_DOWN)
        {
            if (ui->label_blowerUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_down_on.png);")
                ui->label_blowerUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_down_on.png);");

            ui->pushButton_blowerDown->setProperty("wasDown", true);
            blower->goDown();
        }
        else if (ui->pushButton_blowerDown->property("wasDown").toBool())
        {
            if (ui->label_blowerUpDown->styleSheet() != "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_off.png);")
                ui->label_blowerUpDown->setStyleSheet("background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_off.png);");

            ui->pushButton_blowerDown->setProperty("wasDown", false);
            blower->goOff();
        }
    }

    if (1!=1)//(gp->GPIO[IN_CHANGE_DIRECT])
    {// нажали кнопку
        blowerTimeCounter++;
    }
    else
    {// отжата кнопка (и ее нажимали до этого) и это не длительное нажатие
        if (blowerTimeCounter > 1)
        {
        // отработаем нажатие
            if (!startClean)
            {
                if (!workMode.blowLeft && !workMode.blowRight)
                {
                    workMode.backMagnet = true;
                    on_pushButton_blowerLeft_clicked();
                }
                else if (workMode.blowLeft)
                {
                    workMode.backMagnet = true;
                    on_pushButton_blowerRight_clicked();
                }
                else
                {
                    workMode.backMagnet = false;
                    on_pushButton_blowerRight_clicked();
                }
            }
            else
                on_pushButton_startstop_clicked();
        }
        blowerTimeCounter = 0;
    }
}

void MainWindow::showPauseButton()
{
    if (gpioMatirx->keyPressed == GPIOInput::IN_PAUSE_HOME)
    {
        pauseCleanTimeCounter++;
    }
    else
    {// отжата кнопка (и ее нажимали до этого) и это не длительное нажатие
        if (pauseCleanTimeCounter > 1 && startClean)
        {
            pauseActive = !pauseActive;
            if (pauseActive)
            {
                addLog("Пауза включена", WarningStatus);
                // поднимаем органы в промежуточное состояние
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
                addLog("Пауза снята", InfoStatus);
            }
            showWorkMode();
        }
        if (pauseCleanTimeCounter > 1 && !startClean)
            on_pushButton_homeState_clicked();
        pauseCleanTimeCounter = 0;
    }
}

void MainWindow::on_pushButton_dumpUp_clicked()
{

}

void MainWindow::on_pushButton_dumpDown_clicked()
{

}

void MainWindow::on_pushButton_dumpLeft_clicked()
{
    //if (!startClean)
    {
        workMode.frontDumpLeft = !workMode.frontDumpLeft;
        workMode.frontDumpRight = false;
        showWorkMode();
    }
}

void MainWindow::on_pushButton_dumpRight_clicked()
{
    //if (!startClean)
    {
        workMode.frontDumpRight = !workMode.frontDumpRight;
        workMode.frontDumpLeft = false;
        showWorkMode();
    }
}

void MainWindow::on_pushButton_dumpFlow_clicked()
{
    workMode.frontDumpFlow = !workMode.frontDumpFlow;
    showWorkMode();
}

void MainWindow::on_pushButton_centralBroomLeft_clicked()
{
    //if (!startClean)
    {
        workMode.centralBroomLeft = !workMode.centralBroomLeft;
        workMode.centralBroomRight = false;
        showWorkMode();
    }
}

void MainWindow::on_pushButton_centralBroomRight_clicked()
{
    //if (!startClean)
    {
        workMode.centralBroomRight = !workMode.centralBroomRight;
        workMode.centralBroomLeft = false;
        showWorkMode();
    }
}

void MainWindow::on_pushButton_centralBroomUp_clicked()
{

}

void MainWindow::on_pushButton_centralBroomDown_clicked()
{

}

void MainWindow::on_pushButton_centralBroomFlow_clicked()
{
    workMode.centralBroomFlow = !workMode.centralBroomFlow;
    showWorkMode();
}

void MainWindow::on_pushButton_centralBroomPress_clicked()
{
    workMode.centralBroomPress = !workMode.centralBroomPress;
    showWorkMode();
}

void MainWindow::on_pushButton_blowerUp_clicked()
{

}

void MainWindow::on_pushButton_blowerDown_clicked()
{

}

void MainWindow::on_pushButton_blowerLeft_clicked()
{
    //if (!startClean)
    {
        workMode.blowLeft = !workMode.blowLeft;
        workMode.blowRight = false;
        showWorkMode();
    }
}

void MainWindow::on_pushButton_blowerRight_clicked()
{
    //if (!startClean)
    {
        workMode.blowRight = !workMode.blowRight;
        workMode.blowLeft = false;
        showWorkMode();
    }
}

void MainWindow::on_pushButton_backMagnet_clicked()
{
    if (!startClean)
    {
        workMode.backMagnet = !workMode.backMagnet;
        showWorkMode();
    }
}

void MainWindow::on_pushButton_frmKung_clicked()
{
    workMode.frmKung = !workMode.frmKung;
    showWorkMode();
}

void MainWindow::on_pushButton_frmBroom_clicked()
{
    workMode.frmBroom = !workMode.frmBroom;
    showWorkMode();
}

void MainWindow::on_pushButton_frmMagnet_clicked()
{
    workMode.frmMagnet = !workMode.frmMagnet;
    showWorkMode();
}

void MainWindow::on_pushButton_homeState_clicked()
{
    addLog("Переход в домашнее состояние, ожидайте", MainWindow::WarningStatus);
    // вынуждаем все органы убраться поновой. обманка
    backMagnet->state = BackMagnet::BackMagnetDowned;
    blower->state = Blower::BlowerRotated;
    frontRail->state = FrontRail::FrontRailFlowed;
    broomCentral->state = CentralBroom::BroomRotated;
}
