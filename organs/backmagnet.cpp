#include "backmagnet.h"

#include "mainwindow.h"

#include <QDebug>
#include <QTimer>
#include <QThread>

BackMagnet::BackMagnet(const MachineIo &machine, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ViewController *logger_, MainWindow* mainWindow, QObject *parent_) : QObject(parent_)
{
    io = machine.io;
    hydraulics = machine.hydraulics;
    engineRpm = machine.engineRpm;
    myCanJ1939 = myCanJ1939_;
    parent = parent_;
    logger = logger_;
    _mainWindow = mainWindow;
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

void BackMagnet::readSettings(){
    timeouts.clear();
    SettingsReader* reader = _mainWindow->getReader();
//    // назначаем таймауты на длительные операции
    timeouts.insert(BackMagnetDownOut, reader->readSettingsValue("BackMagnet/timeouts.BackMagnetDownOut").toInt());
    timeouts.insert(BackMagnetDownIn, reader->readSettingsValue("BackMagnet/timeouts.BackMagnetDownIn").toInt());
}

QString BackMagnet::toString(BackMagnetStates s){
    const char *key = QMetaEnum::fromType<BackMagnetStates>().valueToKey(s);
    return key ? QString::fromLatin1(key) : QStringLiteral("UnknownState");}

void BackMagnet::setState(BackMagnetStates state_)
{
    state = state_;

    if (state == BackMagnet::BackMagnetOff)
    {// выключили
        goOff();
        logger->addLog("Магнит поднят");
       //_mainWindow->addLog("Магнит поднят", MainWindow::InfoStatus);
        //io->set(StateValveC5, false);
        //io->set(StateFRMBackL2, false);
    }
    if (state == BackMagnet::BackMagnetDownIn)
    {// поднимаем
        startActionTime = QDateTime::currentDateTime();
        logger->addLog("Поднимаем магнит");
        //_mainWindow->addLog("Поднимаем магнит", MainWindow::InfoStatus);
        goUp();
        //io->set(StateValveC5, true);
    }
    if (state == BackMagnet::BackMagnetDownOut)
    {// опускаем
        startActionTime = QDateTime::currentDateTime();
        logger->addLog("Опускаем магнит");
        //_mainWindow->addLog("Опускаем магнит", MainWindow::InfoStatus);
        goDown();
        //io->set(StateFRMBackL2, true);
        //io->set(StateValveC5, true);
    }
    if (state == BackMagnet::BackMagnetDowned)
    {// опустили
        startActionTime = QDateTime::currentDateTime();
        logger->addLog("Магнит опущен");
        //_mainWindow->addLog("Магнит опущен", MainWindow::InfoStatus);
        goOff();
        //io->set(StateValveC5, true);
    }
}

void BackMagnet::goOff()
{
    io->set(StateValveE2, false);
    io->set(StateValveE6, false);
    hydraulics->request(this, false);// магнит стоит - гидравлика ему не нужна
}

void BackMagnet::goUp()
{
    //io->set(StateValveK1, true);
    hydraulics->request(this, true);
    io->set(StateValveE2, true);
    io->set(StateValveE6, false);
}

void BackMagnet::goDown()
{
    //io->set(StateValveK1, true);
    hydraulics->request(this, true);
    io->set(StateValveE2, false);
    io->set(StateValveE6, true);
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

void BackMagnet::checkNeedState(){// утанавливает максимальную границу до которой может дойти щетка (при текущих параметрах)
    if (needState != BackMagnetOff){
        if (!startClean)// пуск отжат или никакой режим смета не выбран или если щетки не выдвинуты
            ableState = BackMagnetOff;// можно только продолжать пытаться включиться (используется такой странный статус потому что надо показать постоянно желание включиться даже если не нажали пуск например)
        else
            ableState = BackMagnetDowned;
    }
    else
        ableState = BackMagnetOff;
}

int BackMagnet::getTimeout(){//получает таймаут в секундах (сколько надо простаивать в той или иной операции)
    return timeouts.value(state, 0);
}

bool BackMagnet::testStateTimer(){// мощная функция проверки таймаута одновременно с концевиками и прочими условиями (для каждого состояния)
    qint64 msecs_to = startActionTime.msecsTo(QDateTime::currentDateTime());
    qint64 tmp_msecs = msecs_to;
    if (msecs_to > getTimeout() * 1000)
        tmp_msecs = getTimeout() * 1000;
    bool timeTest = false;
    if (msecs_to > getTimeout() * 1000){// тест по времени прошел а мы ничего не достигли. Нужны тревоги
        timeTest = true;
        //return true;
    }

    // проверяем концевики
    bool dkpAndPositionTest = false;
    // магнимт идет вверх, ждем концевик
    if (state == BackMagnet::BackMagnetDownIn){
        const bool sensorReached = io->get(StateDKPBackMagnetUp).toBool();
        if (timeTest && !sensorReached){
            if (!magnetAlarmed){
                logger->addLog("Магнит: достигнут тайм-аут");
                goOff();
            }
            magnetAlarmed = true;
        }
        else if (sensorReached){
            logger->addLog("Магнит: достигнут датчик");
        }
        if (timeTest || sensorReached)
            dkpAndPositionTest = true;// не ждем таймера и разрешаем завершить процесс

    }
    // вниз концевика нет. если таймер прошел то считаем что все ок
    if (state == BackMagnet::BackMagnetDownOut && timeTest)
        dkpAndPositionTest = true;

    if (dkpAndPositionTest){
        magnetAlarmed = false;
        return true;// достигнут концевик или нужное положение (мы молодцы)
    }
    return false;
}

void BackMagnet::checkFriendVars(){
    startClean = _mainWindow->startClean;
}

void BackMagnet::progressLoop(){
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
