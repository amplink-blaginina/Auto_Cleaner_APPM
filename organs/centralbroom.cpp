#include "centralbroom.h"

#include "mainwindow.h"

#include <QDebug>
#include <QTimer>
#include <QThread>

CentralBroom::CentralBroom(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ScreenLog *logger_, QObject *parent_) : QObject(parent_){
    myCan = myCan_;
    myCanJ1939 = myCanJ1939_;
    logger = logger_;
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

void CentralBroom::readSettings(){
    timeouts.clear();
    rpmForSweepType.clear();
    speedForSweepType.clear();
    auto mainWindow = (MainWindow*)parent;
    rpmForSweepType.insert(MainWindow::LeafSweep, mainWindow ->readSettingsValue("Engine/rpm.LeafSweep").toInt());
    rpmForSweepType.insert(MainWindow::LightSweep, mainWindow ->readSettingsValue("Engine/rpm.LightSweep").toInt());
    rpmForSweepType.insert(MainWindow::MediumSweep, mainWindow ->readSettingsValue("Engine/rpm.MediumSweep").toInt());
    rpmForSweepType.insert(MainWindow::HeavySweep, mainWindow ->readSettingsValue("Engine/rpm.HeavySweep").toInt());

    timeouts.insert(BroomSlideOut, mainWindow ->readSettingsValue("CentralBroom/timeouts.BroomSlideOut").toInt());
    timeouts.insert(BroomSlideIn, mainWindow ->readSettingsValue("CentralBroom/timeouts.BroomSlideIn").toInt());
    timeouts.insert(BroomBounceOut, mainWindow ->readSettingsValue("CentralBroom/timeouts.BroomBounceOut").toFloat());
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

QString CentralBroom::toString(BroomStates s){
    switch (s) {
    case BroomOff:
        return "BroomOff";
        break;
    case BroomDownOut:
        return "BroomDownOut";
        break;
    case BroomDownIn:
        return "BroomDownIn";
        break;
    case BroomDowned:
        return "BroomDowned";
        break;
    case BroomFlowOut:
        return "BroomFlowOut";
        break;
    case BroomFlowIn:
        return "BroomFlowIn";
        break;
    case BroomFlowed:
        return "BroomFlowed";
        break;
    case BroomSlideOut:
        return "BroomSlideOut";
        break;
    case BroomSlideIn:
        return "BroomSlideIn";
        break;
    case BroomSlided:
        return "BroomSlided";
        break;
    case BroomRotateOut:
        return "BroomRotateOut";
        break;
    case BroomRotateIn:
        return "BroomRotateIn";
        break;
    case BroomRotated:
        return "BroomRotated";
        break;
    case BroomBounceOut:
        return "BroomBounceOut";
        break;
    case BroomBounced:
        return "BroomBounced";
        break;
    default:
        return "UnknownState";
    }
}

void CentralBroom::setState(BroomStates state_){
    if(state == state_){
        return;
    }

    switch (state) {
    case BroomDownOut:
        goDown(false);
        break;
    case BroomDownIn:
        goUp(false);
        break;
    case BroomSlideOut:
        goLeft(false);
        goRight(false);
        break;
    default:
        break;
    }

    state = state_;
    auto mainWindow = (MainWindow*)parent;

    if (state == CentralBroom::BroomOff||//щётка в крайне верхнем положении // остановим поднимаение
        state == CentralBroom::BroomDowned||//щётка в крайне нижнем положении
        state == CentralBroom::BroomSlided||//щётка в крайне боковом положении
        state == CentralBroom::BroomBounced){// отскок завершён

        goNone();
        //myCan->setState(StateFRMBroomL1, false);
        return;
    }

    if (state == CentralBroom::BroomBounceOut){// отскок — поворот в противоположную сторону
        if (timeouts.value(BroomBounceOut, 0) > 0) {
            startActionTime = QDateTime::currentDateTime();
            goNone();
            goSlide(!needSlided);
            logger->printLog("Щетка отскок");
        }
    }

    //---------------------------------------------------------------------------
    if (state == CentralBroom::BroomFlowed){// закончилось плавание
        if (!mainWindow->workMode.centralBroomFlow){
            qDebug()<< "Broom float is over #1";
            goNoFlow();
            logger->printLog("Щетка не плавающая");
            //mainWindow->addLog("Щетка не плавающая", MainWindow::InfoStatus);
        }
    }
    if (state == CentralBroom::BroomFlowIn){// заканчиваем плавание
        qDebug()<< "Broom float is over #2";
        goNoFlow();
        logger->printLog("Щетка не плавающая");
    }
    //----------------------------------------------------------------------------
    startActionTime = QDateTime::currentDateTime();
    if (state == CentralBroom::BroomRotateOut){
        logger->printLog("Щетка раскручивается");
    }
    if (state == CentralBroom::BroomRotateIn){// тормозим щетки
        goNoRotate();
        logger->printLog("Щетка останавливается");
    }
    //----------------------------------------------------------------------------
    goNone();
    if (state == CentralBroom::BroomDownOut){// началось опускание
        goDown();
        //myCan->setState(StateFRMBroomL1, true);
        logger->printLog("Щетка опускается");
    }

    if (state == CentralBroom::BroomDownIn){// Запоминаем что поднимание начилось
        goUp();
        logger->printLog("Щетка поднимается");
    }

    if (state == CentralBroom::BroomSlideOut){
        goSlide(needSlided);
        logger->printLog("Щетка поворачивается");
    }

    if (state == CentralBroom::BroomSlideIn){
        goRight();
        logger->printLog("Щетка поворачивается");
    }

    if (state == CentralBroom::BroomFlowOut){// началось плавание
        goFlow();
        logger->printLog("Щетка плавающая");
    }
}

void CentralBroom::goSlide(bool toLeft){
    if(toLeft){
        goLeft();
    }
    else{
        goRight();
    }
}
void CentralBroom::goLeft(){ goLeft(true);}
void CentralBroom::goLeft(bool state){
    myCan->setState(StateValveF9, state);
    myCan->setState(StateValveA1, state);
}
void CentralBroom::goRight(){goRight(true);}
void CentralBroom::goRight(bool state){
    myCan->setState(StateValveF3, state);
    myCan->setState(StateValveA1, state);
}

void CentralBroom::goNone(){
    myCan->setState(StateValveF9, false);
    myCan->setState(StateValveF3, false);
    myCan->setState(StateValveF4, false);
    myCan->setState(StateValveF10, false);
    myCan->setState(StateValveA1, false);
    goPressNone();
}

void CentralBroom::goPressUp()
{
    myCan->setState(StateValveF2, true);
    myCan->setState(StateValveA1, true);
}
void CentralBroom::goUp(){goUp(true);}
void CentralBroom::goUp(bool state)
{
    myCan->setState(StateValveF10, state);
    myCan->setState(StateValveA1, state);
}

void CentralBroom::goDown(){goDown(true);}
void CentralBroom::goDown(bool state)
{
    myCan->setState(StateValveF4, state);
    myCan->setState(StateValveA1, state);
}

void CentralBroom::goPressDown(){
    myCan->setState(StateValveF8, true);
    myCan->setState(StateValveA1, true);
}

void CentralBroom::goPressNone(){
    myCan->setState(StateValveF2, false);
    myCan->setState(StateValveF8, false);
    myCan->setState(StateValveA1, false);
}

void CentralBroom::goFlow(){
    qDebug()<<"!!! broom go Flow: "<<myCan->getState(StateValveA1);
    myCan->setState(StateValveC1, true);
    myCan->setState(StateValveC2, true);
}

void CentralBroom::goNoFlow(){
    qDebug()<<"!!! broom stop Flow"<<myCan->getState(StateValveA1);
    myCan->setState(StateValveC1, false);
    myCan->setState(StateValveC2, false);
}

void CentralBroom::goRotate(int speed_){
    myCan->setState(StateValveD1, speed_ / 2);
    myCan->setState(StateValveA1, true);
}

void CentralBroom::goNoRotate(){
    myCan->setState(StateValveD1, 0);
}

void CentralBroom::increaseSpeed(){
    int spd = myCan->getState(StateValveD1).toUInt() * 2;
    if (spd + 10 < 100)
        spd += 10;
    else
        spd = 100;
    goRotate(spd);
}

void CentralBroom::decreaseSpeed(){
    int spd = myCan->getState(StateValveD1).toUInt() * 2;
    if (spd - 10 > 0)
        spd -= 10;
    else
        spd = 0;
    goRotate(spd);
}

CentralBroom::BroomStates CentralBroom::getState(){
    return state;
}

void CentralBroom::setNeedState(BroomStates state_){
    needState = state_;
//    qDebug() << "Central broom needState " << toString(needState);
}

CentralBroom::BroomStates CentralBroom::getAbleState(){
    return ableState;
}

CentralBroom::BroomStates CentralBroom::getNeedState(){
    return needState;
}

void CentralBroom::checkNeedState()
{// утанавливает максимальную границу до которой может дойти щетка (при текущих параметрах)
    // проверяет соседние модули и собирает информацию о их состояниях (нажатые кнопки, обороты, статусы и пр.)
    checkFriendVars();
    if (needState != BroomOff){
        if (!startClean){// пуск отжат или никакой режим смета не выбран или если щетки не выдвинуты
            ableState = BroomOff;// можно только продолжать пытаться включиться (используется такой странный статус потому что надо показать постоянно желание включиться даже если не нажали пуск например)
        }
        else{
            ableState = BroomFlowed;// максимум можно все
        }
    }
    else
        ableState = BroomOff;
}

int CentralBroom::getTimeout(){//получает таймаут в секундах (сколько надо простаивать в той или иной операции)
    return timeouts.value(state, 0);
}
bool CentralBroom::isTimeoutReached(){
    qint64 msecs_to = startActionTime.msecsTo(QDateTime::currentDateTime());
    //qint64 tmp_msecs = msecs_to;
    // if (msecs_to > getTimeout() * 1000)
    //     tmp_msecs = getTimeout() * 1000;
    return msecs_to > getTimeout() * 1000;
    // bool timeTest = false;
    // if (msecs_to > getTimeout() * 1000){// тест по времени прошел а мы ничего не достигли. Нужны тревоги
    //     timeTest = true;
    //     //return true;
    // }
}
bool CentralBroom::wereBusyAndTimeoutReached(bool timeoutReached, BroomStates state){
    if(timeoutReached){
        if(state == CentralBroom::BroomRotateOut||
            state == CentralBroom::BroomRotateIn||
            state == CentralBroom::BroomFlowOut||
            state == CentralBroom::BroomFlowIn||
            state == CentralBroom::BroomBounceOut
            ){
            return true;
        }
    }
    return false;
}

bool CentralBroom::checkMovementAndStopOnTimeout(bool timeoutReached, bool isSensorReached, organsEnums::Direction dir){
    if(isSensorReached){
        logger->printMovementLog(organsEnums::BroomOrgan, dir, " остановлено, достигнут датчик");
        return true;
    }
    else{
        if(timeoutReached){
            if (!broomAlarmed){
                logger->printMovementLog(organsEnums::BroomOrgan, dir," достигнут тайм-аут");
                goNone();
            }
            broomAlarmed = true;
            return true;
        }
    }
    return false;
}

bool CentralBroom::testStateTimer(){// мощная функция проверки таймаута одновременно с концевиками и прочими условиями (для каждого состояния)
    //MainWindow *mainWindow = (MainWindow *)parent;
    bool timeoutReached = isTimeoutReached();
    bool dkpAndPositionTest = false;

    //    if (state == CentralBroom::BroomDownOut && timeTest)
    //        dkpAndPositionTest = true;
    if (timeoutReached) {
        if (state == CentralBroom::BroomRotateOut ||
            state == CentralBroom::BroomRotateIn ||
            state == CentralBroom::BroomFlowOut ||
            state == CentralBroom::BroomFlowIn ||
            state == CentralBroom::BroomBounceOut) {
            dkpAndPositionTest = true;
        }
    }

    // проверяем концевики
    if (state == CentralBroom::BroomDownOut){
        if (timeoutReached){
            logger->printMovementLog( organsEnums::BroomOrgan, organsEnums::Down, " достигнут тайм-аут");
            dkpAndPositionTest = true;
        }
    }
    // рейка идет вверх, ждем концевик
    if (state == CentralBroom::BroomDownIn){
        const bool sensorReached = myCan->getState(StateDKPBroomUp).toBool();
        if(checkMovementAndStopOnTimeout(timeoutReached, sensorReached, organsEnums::Up)){
            dkpAndPositionTest = true;
        }
    }

    if (state == CentralBroom::BroomSlideOut){
        const bool sensorReached = needSlided ? myCan->getState(StateDKPBroomLeft).toBool() : myCan->getState(StateDKPBroomRight).toBool();
        if(checkMovementAndStopOnTimeout(timeoutReached, sensorReached, needSlided? organsEnums::Left: organsEnums::Right)){
            dkpAndPositionTest = true;
        }
    }
    if (state == CentralBroom::BroomSlideIn){
        const bool sensorReached = myCan->getState(StateDKPBroomRight).toBool();
        if(checkMovementAndStopOnTimeout(timeoutReached, sensorReached, organsEnums::Right)){
            dkpAndPositionTest = true;
        }
    }

    if (dkpAndPositionTest){
        broomAlarmed = false;
        return true;// достигнут концевик или нужное положение (мы молодцы)
    }

    return false;
}

void CentralBroom::checkFriendVars(){
    startClean = ((MainWindow*)parent)->startClean;
}

void CentralBroom::progressLoop(){
    // рисуем положение щетки (в зависимости от прижима)
    //broomWidget->setGeometry(broomWidget->geometry().x(), 418 + myCan->getState(StateBroomPressLevelD7).toUInt(), broomWidget->geometry().width(), broomWidget->geometry().height());

    if (state >= CentralBroom::BroomRotateOut){
        //обороты движка
        ((MainWindow*)parent)->canForEngine->setEngineCommand(rpmForSweepType.value(((MainWindow*)parent)->workMode.sweepType) * 8);
        // скорость щеток
        goRotate(speedForSweepType.value(((MainWindow*)parent)->workMode.sweepType));
    }

    // проверяет до какого состояния может добираться щетка
    checkNeedState();
    if (state < needState && state < ableState){// нужно прогрессировать вверх (выдвигать, мыть и гусей не забыть)
        BroomStates s = state;
        stateUp();
        if (s != state){// && (state == needState || state == ableState))
            qDebug() << "Central broom state progress: " << toString(state);
        }
    }
    else if (state > needState || state > ableState){// прогрессируем вниз
        BroomStates s = state;
        stateDown();
        if (s != state)// && (state == needState || state == ableState))
        {
            qDebug() << "Central broom state regress: " << toString(state);
        }
    }
}

CentralBroom::BroomStates CentralBroom::stateUp()
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
    return state;
}

CentralBroom::BroomStates CentralBroom::stateDown()
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
    return state;
}
