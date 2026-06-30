#include "backmagnet.h"

#include "mainwindow.h"

#include <QDebug>
#include <QTimer>
#include <QThread>

BackMagnet::BackMagnet(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, QObject *parent_) : QObject(parent_)
{
    myCan = myCan_;
    myCanJ1939 = myCanJ1939_;
    parent = parent_;
    setState(BackMagnetOff);
    setNeedState(BackMagnetOff);
    settings = settings_;
    startClean = false;
    choosed = false;
    magnetAlarmed = false;

    readSettings();

    connect(&progressTimer, SIGNAL(timeout()), this, SLOT(progressLoop()));
    progressTimer.start(100);
}

void BackMagnet::readSettings()
{
    timeouts.clear();

//    // назначаем таймауты на длительные операции
    timeouts.insert(BackMagnetDownOut, ((MainWindow*)parent)->readSettingsValue("BackMagnet/timeouts.BackMagnetDownOut").toInt());
    timeouts.insert(BackMagnetDownIn, ((MainWindow*)parent)->readSettingsValue("BackMagnet/timeouts.BackMagnetDownIn").toInt());
}

QString BackMagnet::toString(BackMagnetStates s)
{
    switch (s) {
    case BackMagnetOff:
        return "BackMagnetOff";
        break;
    case BackMagnetDownIn:
        return "BackMagnetDownIn";
        break;
    case BackMagnetDownOut:
        return "BackMagnetDownOut";
        break;
    case BackMagnetDowned:
        return "BackMagnetDowned";
        break;
    default:
        return "UnknownState";
    }
}

void BackMagnet::setState(BackMagnetStates state_)
{
    state = state_;

    if (state == BackMagnet::BackMagnetOff)
    {// выключили
        goOff();
        ((MainWindow*)parent)->addLog("Магнит поднят", MainWindow::InfoStatus);
        //myCan->setState(StateValveC5, false);
        //myCan->setState(StateFRMBackL2, false);
    }
    if (state == BackMagnet::BackMagnetDownIn)
    {// поднимаем
        startActionTime = QDateTime::currentDateTime();

        ((MainWindow*)parent)->addLog("Поднимаем магнит", MainWindow::InfoStatus);
        goUp();
        //myCan->setState(StateValveC5, true);
    }
    if (state == BackMagnet::BackMagnetDownOut)
    {// опускаем
        startActionTime = QDateTime::currentDateTime();

        ((MainWindow*)parent)->addLog("Опускаем магнит", MainWindow::InfoStatus);
        goDown();
        //myCan->setState(StateFRMBackL2, true);
        //myCan->setState(StateValveC5, true);
    }
    if (state == BackMagnet::BackMagnetDowned)
    {// опустили
        startActionTime = QDateTime::currentDateTime();

        ((MainWindow*)parent)->addLog("Магнит опущен", MainWindow::InfoStatus);
        goOff();
        //myCan->setState(StateValveC5, true);
    }
}

void BackMagnet::goOff()
{
    myCan->setState(StateValveE2, false);
    myCan->setState(StateValveE6, false);
}

void BackMagnet::goUp()
{
    //myCan->setState(StateValveK1, true);
    myCan->setState(StateValveA1, true);
    myCan->setState(StateValveE2, true);
    myCan->setState(StateValveE6, false);
}

void BackMagnet::goDown()
{
    //myCan->setState(StateValveK1, true);
    myCan->setState(StateValveA1, true);
    myCan->setState(StateValveE2, false);
    myCan->setState(StateValveE6, true);
}

BackMagnet::BackMagnetStates BackMagnet::getState()
{
    return state;
}

void BackMagnet::setNeedState(BackMagnetStates state_)
{
    needState = state_;
//    qDebug() << "Central broom needState " << toString(needState);
}

BackMagnet::BackMagnetStates BackMagnet::getAbleState()
{
    return ableState;
}

BackMagnet::BackMagnetStates BackMagnet::getNeedState()
{
    return needState;
}

void BackMagnet::checkNeedState()
{// утанавливает максимальную границу до которой может дойти щетка (при текущих параметрах)
    if (needState != BackMagnetOff)
    {
        if (!startClean)
        {// пуск отжат или никакой режим смета не выбран или если щетки не выдвинуты
            ableState = BackMagnetOff;// можно только продолжать пытаться включиться (используется такой странный статус потому что надо показать постоянно желание включиться даже если не нажали пуск например)
        }
        else
        {
            ableState = BackMagnetDowned;
        }
    }
    else
        ableState = BackMagnetOff;
}

int BackMagnet::getTimeout()
{//получает таймаут в секундах (сколько надо простаивать в той или иной операции)
    return timeouts.value(state, 0);
}

bool BackMagnet::testStateTimer()
{// мощная функция проверки таймаута одновременно с концевиками и прочими условиями (для каждого состояния)
    qint64 msecs_to = startActionTime.msecsTo(QDateTime::currentDateTime());
    qint64 tmp_msecs = msecs_to;
    if (msecs_to > getTimeout() * 1000)
        tmp_msecs = getTimeout() * 1000;
    bool timeTest = false;
    if (msecs_to > getTimeout() * 1000)
    {// тест по времени прошел а мы ничего не достигли. Нужны тревоги
        timeTest = true;
        //return true;
    }

    // проверяем концевики
    bool dkpAndPositionTest = false;
    // магнимт идет вверх, ждем концевик
    if (state == BackMagnet::BackMagnetDownIn)
    {
        const bool sensorReached = myCan->getState(StateDKPBackMagnetUp).toBool();
        if (timeTest && !sensorReached)
        {
            if (!magnetAlarmed)
            {
                ((MainWindow*)parent)->addLog("Магнит: достигнут тайм-аут", MainWindow::InfoStatus);
                goOff();
            }
            magnetAlarmed = true;
        }
        else if (sensorReached)
        {
            ((MainWindow*)parent)->addLog("Магнит: достигнут датчик", MainWindow::InfoStatus);
        }
        if (timeTest || sensorReached)
            dkpAndPositionTest = true;// не ждем таймера и разрешаем завершить процесс

    }
    // вниз концевика нет. если таймер прошел то считаем что все ок
    if (state == BackMagnet::BackMagnetDownOut && timeTest)
        dkpAndPositionTest = true;

    if (dkpAndPositionTest)
    {
        magnetAlarmed = false;
        return true;// достигнут концевик или нужное положение (мы молодцы)
    }
    return false;
}

void BackMagnet::checkFriendVars()
{
    startClean = ((MainWindow*)parent)->startClean;
}

void BackMagnet::progressLoop()
{
    // проверяет соседние модули и собирает информацию о их состояниях (нажатые кнопки, обороты, статусы и пр.)
    checkFriendVars();
    // проверяет до какого состояния может добираться щетка
    checkNeedState();
    if (state < needState && state < ableState)
    {// нужно прогрессировать вверх (выдвигать, мыть и гусей не забыть)
        BackMagnetStates s = state;
        stateUp();
        if (s != state)// && (state == needState || state == ableState))
        {
            qDebug() << "Back magnet state " << toString(state);
        }
    }
    else if (state > needState || state > ableState)
    {// прогрессируем вниз
        BackMagnetStates s = state;
        stateDown();
        if (s != state)// && (state == needState || state == ableState))
        {
            qDebug() << "Back magnet state " << toString(state);
        }
    }
}

BackMagnet::BackMagnetStates BackMagnet::stateUp()
{// пытаемся прогрессировать статусом вверх (если что меняем направление статуса, если вдруг был понижающий прогресс)
    switch (state) {
    case BackMagnetOff:
        // начинаем опускание
        setState(BackMagnetDownOut);
        break;
    case BackMagnetDownOut:
        // заканчиваем опускание по таймеру
        if (testStateTimer())
            setState(BackMagnetDowned);
        break;
    case BackMagnetDownIn:
        // меняем направление на опускание (до этого поднимались)
        setState(BackMagnetDownOut);
        break;
    default:
        break;
    }
    return state;
}

BackMagnet::BackMagnetStates BackMagnet::stateDown()
{// пытаемся прогрессировать статусом вниз (если что меняем направление статуса, если вдруг был повышающий прогресс)
    switch (state) {
    case BackMagnetDownOut:
        // меняем направление на поднимание (до этого опускались)
        setState(BackMagnetDownIn);
        break;
    case BackMagnetDownIn:
        // заканчиваем подъем по таймеру и переходим в стостояние готовности к включению
        if (testStateTimer())
            setState(BackMagnetOff);
        break;
    case BackMagnetDowned:
        // начинаем поднимаение по таймеру
        setState(BackMagnetDownIn);
        break;
    default:
        break;
    }
    return state;
}
