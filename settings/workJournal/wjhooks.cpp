/*
 * Журнал работы — точки связи с программой Auto_Cleaner (реализация для пульта)
 * Версия: 03, 2026-09-30
 * Изменения от 02: ini WorkJournal/speedBus; скорость программы больше не передаётся
 * Изменения от 01: положение оборудования по состояниям модулей программы; ini fuelLevelBus / fuelRateMask / odoBus
 * Зависимости: исходник Auto_Cleaner_318D4 (MainWindow, CanController, CurrentState, MyCanJ1939),
 *   модуль Журнала ТО (tojournalhooks, tojournalkeyboard)
 */
#include "wjhooks.h"
#include "wjcollector.h"
#include "wjdefaults.h"

#include "mainwindow.h"
#include "settings/toJournal/tojournalhooks.h"
#include "settings/toJournal/tojournalkeyboard.h"

#include <QCoreApplication>
#include <QSettings>
#include <QDateTime>
#include <QPointer>
#include <QDebug>

namespace WJ
{
namespace Hooks
{

static QSettings& ini()
{
    static QSettings s(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini", QSettings::IniFormat);
    return s;
}

static Collector* gCollector = nullptr;

Collector* collector()
{
    return gCollector;
}

QString databasePath()
{
    return ini().value("WorkJournal/dbPath", QString::fromLatin1(kDefaultDbPath)).toString();
}

QString exportDir()
{
    return ini().value("WorkJournal/exportDir", QString::fromLatin1(kDefaultExportDir)).toString();
}

QString machineTitle(MainWindow* mw)
{
    return ToJ::Hooks::machineTitle(mw);
}

double engineHours(MainWindow* mw)
{
    return ToJ::Hooks::engineHours(mw);
}

bool nextTo(MainWindow* mw, int* milestone)
{
    int m = 0;
    ToJ::Hooks::tick(mw, &m);            // берёт кэш Журнала ТО (пересчёт — не чаще раза в минуту)
    if (m <= 0 || m >= 1000000000)
        return false;
    if (milestone)
        *milestone = m;
    return true;
}

void askPassword(MainWindow* mw, QObject* ctx, std::function<void()> onOk)
{
    ToJ::Hooks::askPassword(mw, ctx, onOk);
}

void editText(QLineEdit* edit)
{
    ToJ::Keyboard::open(edit);
}

void settingsChanged()
{
    if (gCollector)
        gCollector->reloadSettings();
}

void tick(MainWindow* mw)
{
    if (!mw || !mw->can)
        return;
    if (!gCollector)
    {
        static bool failed = false;
        if (failed)
            return;
        gCollector = new Collector(databasePath());
        if (!gCollector->ok())
        {
            qDebug() << "WORK JOURNAL: сборщик не запущен (БД)";
            delete gCollector;
            gCollector = nullptr;
            failed = true;           // повтор — после перезапуска программы
            return;
        }
        gCollector->setBuses(ini().value("WorkJournal/fuelLevelBus", 1).toInt(),
                             ini().value("WorkJournal/fuelRateMask", 3).toInt(),
                             ini().value("WorkJournal/odoBus", 1).toInt(),
                             ini().value("WorkJournal/speedBus", 1).toInt());
        if (mw->canj1939)
            QObject::connect(mw->canj1939, &MyCanJ1939::canDataReadyJ1939, gCollector, &Collector::onEngineFrame, Qt::QueuedConnection);
        if (mw->canj1939Main)
            QObject::connect(mw->canj1939Main, &MyCanJ1939::canDataReadyJ1939, gCollector, &Collector::onChassisFrame, Qt::QueuedConnection);
        QObject::connect(qApp, &QCoreApplication::aboutToQuit, []()
        {
            delete gCollector;       // запись последней минуты
            gCollector = nullptr;
        });
        qDebug() << "WORK JOURNAL: сборщик запущен; уровень топлива — шина" << ini().value("WorkJournal/fuelLevelBus", 1).toInt()
                 << "расход — маска" << ini().value("WorkJournal/fuelRateMask", 3).toInt() << "одометр — шина" << ini().value("WorkJournal/odoBus", 1).toInt()
                 << "скорость — шина" << ini().value("WorkJournal/speedBus", 1).toInt();
    }

    // сигналы БУЦ — через IoBus (в режиме --sim их даёт симуляция)
    IoBus* io = mw->ioBus();
    auto on = [io](DeviceStates s) { return io->get(s).toBool(); };
    Inputs in;
    in.now = QDateTime::currentSecsSinceEpoch();
    in.bucKnown = mw->isSimulation() || mw->can->isConfigured();// платы БУЦ сконфигурированы — данные входов верные
    in.keyOn = in.bucKnown ? on(Board0IN1) : true;
    in.pvi = in.bucKnown && on(StatePVIPowerIn);
    in.cleanRunning = mw->startClean;
    in.paused = mw->startClean && mw->pauseActive;
    if (in.bucKnown)
    {
        const uint raw = io->get(StateHydraulicOilTemperature).toUInt();
        if (raw < 0xFB)
            in.hydro = mw->hydroTempK * raw + mw->hydroTempB;
        in.eq[EqRotD1]     = on(StateValveD1);
        in.eq[EqRotD2]     = on(StateValveD2);
        in.eq[EqRotD3]     = on(StateValveD3);
        in.filters[FltDrain]    = on(StateDrainFilterD28);
        in.filters[FltPress1]   = on(StatePressureFilter1);
        in.filters[FltPress2]   = on(StatePressureFilter2);
        in.filters[FltPress3]   = on(StatePressureFilter3);
        in.filters[FltAir]      = on(StateAirFilterBad);
        in.filters[FltOil]      = on(StateOilFilterBad);
        in.filters[FltWater]    = on(StateWaterSensor);
        in.filters[FltAlarmBtn] = on(StateAlarmIn);
    }
    // положение оборудования — по логике программы (датчики ДКП на машине ненадёжны): «опускается» и ниже = опущено
    in.eq[EqBroomDown]  = mw->broomCentral && mw->broomCentral->getState() >= CentralBroom::BroomDownOut;
    in.eq[EqDumpDown]   = mw->frontRail && mw->frontRail->getState() >= FrontRail::FrontRailDownOut;
    in.eq[EqMagnetDown] = mw->backMagnet && mw->backMagnet->getState() >= BackMagnet::BackMagnetDownOut;
    in.eq[EqBlowerDown] = mw->blower && mw->blower->getState() >= Blower::BlowerDownOut;
    const double h = engineHours(mw);
    in.engineH = h >= 0 ? h : noValue();
    gCollector->tick(in);
}

}
}
