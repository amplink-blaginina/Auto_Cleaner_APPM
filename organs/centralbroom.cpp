#include "centralbroom.h"

#include "mainwindow.h"

#include <QDebug>
#include <QTimer>
#include <QThread>

CentralBroom::CentralBroom(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, QObject *parent_) : QObject(parent_)
{
    myCan = myCan_;
    myCanJ1939 = myCanJ1939_;
    parent = parent_;
    setState(BroomOff);
    setNeedState(BroomOff);
    needSlided = false;
    settings = settings_;
    startClean = false;
    choosed = false;
    broomAlarmed = false;

    readSettings();

    connect(&progressTimer, SIGNAL(timeout()), this, SLOT(progressLoop()));
    progressTimer.start(100);
}

void CentralBroom::readSettings()
{
    timeouts.clear();
    rpmForSweepType.clear();
    speedForSweepType.clear();

    auto mainWin = (MainWindow*)parent;

    rpmForSweepType.insert(MainWindow::LeafSweep, mainWin->readSettingsValue("Engine/rpm.LeafSweep").toInt());
    rpmForSweepType.insert(MainWindow::LightSweep, mainWin->readSettingsValue("Engine/rpm.LightSweep").toInt());
    rpmForSweepType.insert(MainWindow::MediumSweep, mainWin->readSettingsValue("Engine/rpm.MediumSweep").toInt());
    rpmForSweepType.insert(MainWindow::HeavySweep, mainWin->readSettingsValue("Engine/rpm.HeavySweep").toInt());

    timeouts.insert(BroomSlideOut, mainWin->readSettingsValue("CentralBroom/timeouts.BroomSlideOut").toInt());
    timeouts.insert(BroomSlideIn, mainWin->readSettingsValue("CentralBroom/timeouts.BroomSlideIn").toInt());
    timeouts.insert(BroomBounceOut, mainWin->readSettingsValue("CentralBroom/timeouts.BroomBounceOut").toFloat());
    timeouts.insert(BroomDownOut, mainWin->readSettingsValue("CentralBroom/timeouts.BroomDownOut").toInt());
    timeouts.insert(BroomDownIn, mainWin->readSettingsValue("CentralBroom/timeouts.BroomDownIn").toInt());
    timeouts.insert(BroomFlowOut, mainWin->readSettingsValue("CentralBroom/timeouts.BroomFlowOut").toInt());
    timeouts.insert(BroomRotateOut, mainWin->readSettingsValue("CentralBroom/timeouts.BroomRotateOut").toInt());
    timeouts.insert(BroomRotateIn, mainWin->readSettingsValue("CentralBroom/timeouts.BroomRotateIn").toInt());

    speedForSweepType.insert(MainWindow::LeafSweep, mainWin->readSettingsValue("CentralBroom/speeds.LeafSweep").toInt());
    speedForSweepType.insert(MainWindow::LightSweep, mainWin->readSettingsValue("CentralBroom/speeds.LightSweep").toInt());
    speedForSweepType.insert(MainWindow::MediumSweep, mainWin->readSettingsValue("CentralBroom/speeds.MediumSweep").toInt());
    speedForSweepType.insert(MainWindow::HeavySweep, mainWin->readSettingsValue("CentralBroom/speeds.HeavySweep").toInt());
}

QString CentralBroom::toString(BroomStates s)
{
    return QVariant::fromValue(s).toString();//std::string(QMetaEnum::fromType<BroomStates>().valueToKey(s));
    //return QtEnumToString(s);
    // switch (s) {
    // case BroomOff:
    //     return "BroomOff";
    //     break;
    // case BroomDownOut:
    //     return "BroomDownOut";
    //     break;
    // case BroomDownIn:
    //     return "BroomDownIn";
    //     break;
    // case BroomDowned:
    //     return "BroomDowned";
    //     break;
    // case BroomFlowOut:
    //     return "BroomFlowOut";
    //     break;
    // case BroomFlowIn:
    //     return "BroomFlowIn";
    //     break;
    // case BroomFlowed:
    //     return "BroomFlowed";
    //     break;
    // case BroomSlideOut:
    //     return "BroomSlideOut";
    //     break;
    // case BroomSlideIn:
    //     return "BroomSlideIn";
    //     break;
    // case BroomSlided:
    //     return "BroomSlided";
    //     break;
    // case BroomRotateOut:
    //     return "BroomRotateOut";
    //     break;
    // case BroomRotateIn:
    //     return "BroomRotateIn";
    //     break;
    // case BroomRotated:
    //     return "BroomRotated";
    //     break;
    // case BroomBounceOut:
    //     return "BroomBounceOut";
    //     break;
    // case BroomBounced:
    //     return "BroomBounced";
    //     break;
    // default:
    //     return "UnknownState";
    // }
}

void CentralBroom::setState(BroomStates state_)
{
    auto mainWindow = (MainWindow *)parent;
    curState = state_;

    if (curState == CentralBroom::BroomOff)
    {// поднялась щетка
        // остановим поднимаение
        goNone();
        //myCan->setState(StateFRMBroomL1, false);
    }
    if (curState == CentralBroom::BroomDownOut)
    {// началось опускание
        startActionTime = QDateTime::currentDateTime();
        goNone();
        goDown();
        //myCan->setState(StateFRMBroomL1, true);
        mainWindow->addLog("Щетка опускается", MainWindow::InfoStatus);
    }
    if (curState == CentralBroom::BroomDowned)
    {
        goNone();
    }
    if (curState == CentralBroom::BroomDownIn)
    {
        // Запоминаем что поднимание начилось
        startActionTime = QDateTime::currentDateTime();
        goNone();
        goUp();
        mainWindow->addLog("Щетка поднимается", MainWindow::InfoStatus);
    }
    if (curState == CentralBroom::BroomFlowOut)
    {// началось плавание
        startActionTime = QDateTime::currentDateTime();
        goNone();
        goFlow();
        mainWindow->addLog("Щетка плавающая", MainWindow::InfoStatus);
    }
    if (curState == CentralBroom::BroomFlowed)
    {// закончилось плавание
        if (!mainWindow->workMode.centralBroomFlow)
        {
            goNoFlow();
            mainWindow->addLog("Щетка не плавающая", MainWindow::InfoStatus);
        }
    }
    if (curState == CentralBroom::BroomFlowIn)
    {// заканчиваем плавание
        goNoFlow();
        mainWindow->addLog("Щетка не плавающая", MainWindow::InfoStatus);
    }
    if (curState == CentralBroom::BroomSlideOut)
    {
        startActionTime = QDateTime::currentDateTime();
        goNone();
        if (!needSlided)
            goRight();
        else
            goLeft();
        mainWindow->addLog("Щетка поворачивается", MainWindow::InfoStatus);
    }
    if (curState == CentralBroom::BroomSlideIn)
    {
        startActionTime = QDateTime::currentDateTime();
        goNone();
        goRight();
        mainWindow->addLog("Щетка поворачивается", MainWindow::InfoStatus);
    }
    if (curState == CentralBroom::BroomSlided)
    {
        goNone();
    }
    if (curState == CentralBroom::BroomRotateOut)
    {
        startActionTime = QDateTime::currentDateTime();
        mainWindow->addLog("Щетка раскручивается", MainWindow::InfoStatus);
    }
    if (curState == CentralBroom::BroomRotateIn)
    {
        startActionTime = QDateTime::currentDateTime();
        // тормозим щетки
        goNoRotate();
        mainWindow->addLog("Щетка останавливается", MainWindow::InfoStatus);
    }
    if (curState == CentralBroom::BroomBounceOut)
    {// отскок — поворот в противоположную сторону
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(BroomBounceOut, 0) > 0) {
            goNone();
            if (!needSlided)
                goLeft();
            else
                goRight();
            mainWindow->addLog("Щетка отскок", MainWindow::InfoStatus);
        }
    }
    if (curState == CentralBroom::BroomBounced)
    {// отскок завершён
        goNone();
    }
}

void CentralBroom::goLeft()
{
    myCan->setState(StateValveF9, true);
    myCan->setState(StateValveA1, true);
}

void CentralBroom::goRight()
{
    myCan->setState(StateValveF3, true);
    myCan->setState(StateValveA1, true);
}

void CentralBroom::goNone()
{
    myCan->setState(StateValveF9, false);
    myCan->setState(StateValveF3, false);
    myCan->setState(StateValveF4, false);
    myCan->setState(StateValveF10, false);
    goPressNone();
}

void CentralBroom::goPressNone()
{
    myCan->setState(StateValveF2, false);
    myCan->setState(StateValveF8, false);
}

void CentralBroom::goPressUp()
{
    myCan->setState(StateValveF2, true);
    myCan->setState(StateValveA1, true);
}

void CentralBroom::goPressDown()
{
    myCan->setState(StateValveF8, true);
    myCan->setState(StateValveA1, true);
}

void CentralBroom::goUp()
{
    myCan->setState(StateValveF10, true);
    myCan->setState(StateValveA1, true);
}

void CentralBroom::goDown()
{
    myCan->setState(StateValveF4, true);
    myCan->setState(StateValveA1, true);
}

void CentralBroom::goFlow()
{
    myCan->setState(StateValveC1, true);
    myCan->setState(StateValveC2, true);
}

void CentralBroom::goNoFlow()
{
    myCan->setState(StateValveC1, false);
    myCan->setState(StateValveC2, false);
}



void CentralBroom::goRotate(int speed_)
{
    myCan->setState(StateValveD1, speed_ / 2);
    myCan->setState(StateValveA1, true);
}

void CentralBroom::goNoRotate()
{
    myCan->setState(StateValveD1, 0);
}

void CentralBroom::increaseSpeed()
{
    int spd = myCan->getState(StateValveD1).toUInt() * 2;
    if (spd + 10 < 100)
        spd += 10;
    else
        spd = 100;
    goRotate(spd);
}

void CentralBroom::decreaseSpeed()
{
    int spd = myCan->getState(StateValveD1).toUInt() * 2;
    if (spd - 10 > 0)
        spd -= 10;
    else
        spd = 0;
    goRotate(spd);
}

CentralBroom::BroomStates CentralBroom::getState()
{
    return curState;
}

void CentralBroom::setNeedState(BroomStates state_)
{
    needState = state_;
//    qDebug() << "Central broom needState " << toString(needState);
}

CentralBroom::BroomStates CentralBroom::getAbleState()
{
    return ableState;
}

CentralBroom::BroomStates CentralBroom::getNeedState()
{
    return needState;
}

void CentralBroom::checkNeedState()
{// утанавливает максимальную границу до которой может дойти щетка (при текущих параметрах)
    // проверяет соседние модули и собирает информацию о их состояниях (нажатые кнопки, обороты, статусы и пр.)
    checkFriendVars();
    if (needState != BroomOff)
    {
        if (!startClean)
        {// пуск отжат или никакой режим смета не выбран или если щетки не выдвинуты
            ableState = BroomOff;// можно только продолжать пытаться включиться (используется такой странный статус потому что надо показать постоянно желание включиться даже если не нажали пуск например)
        }
        else
        {
            ableState = BroomFlowed;// максимум можно все
        }
    }
    else
        ableState = BroomOff;
}

int CentralBroom::getTimeout(){//получает таймаут в секундах (сколько надо простаивать в той или иной операции)
    return timeouts.value(curState, 0);
}

bool CentralBroom::testStateTimer()
{// мощная функция проверки таймаута одновременно с концевиками и прочими условиями (для каждого состояния)

    auto mainWindow = (MainWindow *)parent;
    qint64 msecs_to = startActionTime.msecsTo(QDateTime::currentDateTime());
    //qint64 tmp_msecs = msecs_to;
    auto timeout = getTimeout() * 1000;

    // if (msecs_to > timeout)
    //     tmp_msecs = timeout;

    // bool timeTest = false;
    // if (msecs_to > timeout)
    // {// тест по времени прошел а мы ничего не достигли. Нужны тревоги
    //     timeTest = true;
    //     //return true;
    // }

    bool timeTest = msecs_to > timeout;
    bool dkpAndPositionTest = false;

    // проверяем концевики
    if (curState == CentralBroom::BroomDownOut)    {
        if (timeTest){
            mainWindow ->addLog("Щетка: достигнут тайм-аут", MainWindow::InfoStatus);
            dkpAndPositionTest = true;
        }
    }

    if(timeTest &&
        (curState == CentralBroom::BroomRotateOut||
         curState == CentralBroom::BroomRotateIn||
         curState == CentralBroom::BroomFlowOut||
         curState == CentralBroom::BroomFlowIn ||
         curState == CentralBroom::BroomBounceOut)){
            dkpAndPositionTest = true;
    }
//    if (state == CentralBroom::BroomDownOut && timeTest)
//        dkpAndPositionTest = true;

    // рейка идет вверх, ждем концевик
    if (curState == CentralBroom::BroomDownIn){

        const bool sensorReached = myCan->getState(StateDKPBroomUp).toBool();
        if (timeTest && !sensorReached)
        {
            if (!broomAlarmed)
            {
                mainWindow->addLog("Щетка: достигнут тайм-аут", MainWindow::InfoStatus);
                goNone();
            }
            broomAlarmed = true;
        }
        else if (sensorReached)
        {
            mainWindow->addLog("Щетка: достигнут датчик", MainWindow::InfoStatus);
        }
        if (timeTest || sensorReached)
            dkpAndPositionTest = true;

    }
    if (curState == CentralBroom::BroomSlideOut)
    {
        const bool sensorReached = needSlided ? myCan->getState(StateDKPBroomLeft).toBool() : myCan->getState(StateDKPBroomRight).toBool();
        if (timeTest && !sensorReached)
        {
            if (!broomAlarmed)
            {
                mainWindow->addLog("Щетка: достигнут тайм-аут", MainWindow::InfoStatus);
                goNone();
            }
            broomAlarmed = true;
        }
        else if (sensorReached)
        {
            mainWindow->addLog("Щетка: достигнут датчик", MainWindow::InfoStatus);
        }
        if (timeTest || sensorReached)
            dkpAndPositionTest = true;

    }
    if (curState == CentralBroom::BroomSlideIn)
    {
        const bool sensorReached = myCan->getState(StateDKPBroomRight).toBool();
        if (timeTest && !sensorReached)
        {
            if (!broomAlarmed)
            {
                mainWindow->addLog("Щетка: достигнут тайм-аут", MainWindow::InfoStatus);
                goNone();
            }
            broomAlarmed = true;
        }
        else if (sensorReached)
        {
            mainWindow->addLog("Щетка: достигнут датчик", MainWindow::InfoStatus);
        }
        if (timeTest || sensorReached)
            dkpAndPositionTest = true;

    }

    if (dkpAndPositionTest)
    {
        broomAlarmed = false;
        return true;// достигнут концевик или нужное положение (мы молодцы)
    }

    return false;
}

void CentralBroom::checkFriendVars(){
    startClean = ((MainWindow*)parent)->startClean;
}

void CentralBroom::progressLoop(){

    auto mainWindow = (MainWindow *)parent;
    // рисуем положение щетки (в зависимости от прижима)
    //broomWidget->setGeometry(broomWidget->geometry().x(), 418 + myCan->getState(StateBroomPressLevelD7).toUInt(), broomWidget->geometry().width(), broomWidget->geometry().height());

    if (curState >= CentralBroom::BroomRotateOut)
    {
        //обороты движка
        mainWindow->canForEngine->setEngineCommand(rpmForSweepType.value(mainWindow->workMode.sweepType) * 8);
        // скорость щеток
        goRotate(speedForSweepType.value(mainWindow->workMode.sweepType));
    }

    // проверяет до какого состояния может добираться щетка
    checkNeedState();
    if (curState < needState && curState < ableState)
    {// нужно прогрессировать вверх (выдвигать, мыть и гусей не забыть)
        BroomStates s = curState;
        stateUp(curState);
        if (s != curState)// && (state == needState || state == ableState))
        {
            qDebug() << "Central broom state " << toString(curState);
        }
    }
    else if (curState > needState || curState > ableState)
    {// прогрессируем вниз
        BroomStates s = curState;
        stateDown(curState);
        if (s != curState)// && (state == needState || state == ableState))
        {
            qDebug() << "Central broom state " << toString(curState);
        }
    }
}

CentralBroom::BroomStates CentralBroom::stateUp(BroomStates state)
{// пытаемся прогрессировать статусом вверх (если что меняем направление статуса, если вдруг был понижающий прогресс)
    switch (state) {
    case BroomOff:
        // начинаем опускание
        setState(BroomSlideOut);
        break;
    case BroomDownOut:
        // заканчиваем опускание по таймеру
        if (testStateTimer())
            setState(BroomDowned);
        break;
    case BroomDownIn:
        // меняем направление на опускание (до этого поднимались)
        setState(BroomDownOut);
        break;
    case BroomDowned:
        // начинаем вращение или поворот
        setState(BroomFlowOut);
        break;
    case BroomFlowIn:
        setState(BroomFlowOut);
        break;
    case BroomFlowOut:
        if (testStateTimer())
            setState(BroomFlowed);
        break;
    case BroomFlowed:
        // начинаем вращение или поворот
        setState(BroomRotateOut);
        break;
    case BroomRotated:
        // начинаем вращение или поворот
        setState(BroomDownOut);
        break;
    case BroomRotateOut:
        // заканчиваем раскрутку
        if (testStateTimer())
            setState(BroomRotated);
        break;
    case BroomRotateIn:
        // меняем направление раскрутки ( до этого тормозились)
        setState(BroomRotateOut);
        break;
    case BroomSlideOut:
        // заканчиваем поворот щетки
        if (testStateTimer())
            setState(BroomSlided);
        break;
    case BroomSlideIn:
        setState(BroomSlideOut);
        break;
    case BroomSlided:
        qDebug()<< "Finish broom sliding";
        setState(BroomBounceOut);
        break;
    case BroomBounceOut:
        if (testStateTimer())
            setState(BroomBounced);
        break;
    case BroomBounced:
        setState(BroomRotateOut);
        break;
    default:
        break;
    }
    return curState;
}

CentralBroom::BroomStates CentralBroom::stateDown(BroomStates state)
{// пытаемся прогрессировать статусом вниз (если что меняем направление статуса, если вдруг был повышающий прогресс)
    switch (state) {
    case BroomDownOut:
        // меняем направление на поднимание (до этого опускались)
        setState(BroomDownIn);
        break;
    case BroomDownIn:
        // заканчиваем подъем по таймеру и переходим в стостояние готовности к включению
        if (testStateTimer())
            setState(BroomRotated);
        break;
    case BroomDowned:
        // начинаем поднимаение по таймеру
        setState(BroomDownIn);
        break;
    case BroomFlowIn:
        setState(BroomDowned);
        break;
    case BroomFlowOut:
        setState(BroomFlowIn);
        break;
    case BroomFlowed:
        setState(BroomFlowIn);
        break;
    case BroomSlideOut:
        setState(BroomSlideIn);
        break;
    case BroomSlideIn:
        if (testStateTimer())
            setState(BroomOff);
        break;
    case BroomSlided:
        setState(BroomSlideIn);
        break;
    case BroomRotateOut:
        setState(BroomRotateIn);
        break;
    case BroomRotateIn:
        if (testStateTimer())
            setState(BroomBounced);
        break;
    case BroomRotated:
        setState(BroomRotateIn);
        break;
    case BroomBounceOut:
        // прерываем отскок, считаем завершённым
        setState(BroomBounced);
        break;
    case BroomBounced:
        setState(BroomSlided);
        break;
    default:
        break;
    }
    return curState;
}
