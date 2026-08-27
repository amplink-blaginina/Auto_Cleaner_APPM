#include "centralbroom.h"

#include "mainwindow.h"

#include <QDebug>
#include <QTimer>
#include <QThread>

CentralBroom::CentralBroom(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ViewController *logger_, QObject *parent_) : QObject(parent_){
    myCan = myCan_;
    myCanJ1939 = myCanJ1939_;
    logger = logger_;
    parent = parent_;
    setState(BroomOff);
    setNeedState(BroomOff);
    needGoLeft = false;
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
    auto reader = mainWindow->getReader();

    rpmForSweepType.insert(MainWindow::LeafSweep, reader->readSettingsValue("Engine/rpm.LeafSweep").toInt());
    rpmForSweepType.insert(MainWindow::LightSweep, reader->readSettingsValue("Engine/rpm.LightSweep").toInt());
    rpmForSweepType.insert(MainWindow::MediumSweep, reader->readSettingsValue("Engine/rpm.MediumSweep").toInt());
    rpmForSweepType.insert(MainWindow::HeavySweep, reader->readSettingsValue("Engine/rpm.HeavySweep").toInt());

    timeouts.insert(BroomSlideOut, reader->readSettingsValue("CentralBroom/timeouts.BroomSlideOut").toFloat());
    timeouts.insert(BroomSlideIn, reader->readSettingsValue("CentralBroom/timeouts.BroomSlideIn").toFloat());
    timeouts.insert(BroomBounceOut, reader->readSettingsValue("CentralBroom/timeouts.BroomBounceOut").toFloat());
    timeouts.insert(BroomDownOut, reader->readSettingsValue("CentralBroom/timeouts.BroomDownOut").toFloat());
    timeouts.insert(BroomDownIn, reader->readSettingsValue("CentralBroom/timeouts.BroomDownIn").toFloat());
    timeouts.insert(BroomFlowOut, reader->readSettingsValue("CentralBroom/timeouts.BroomFlowOut").toFloat());
    timeouts.insert(BroomRotateOut, reader->readSettingsValue("CentralBroom/timeouts.BroomRotateOut").toFloat());
    timeouts.insert(BroomRotateIn, reader->readSettingsValue("CentralBroom/timeouts.BroomRotateIn").toFloat());

    speedForSweepType.insert(MainWindow::LeafSweep, reader->readSettingsValue("CentralBroom/speeds.LeafSweep").toInt());
    speedForSweepType.insert(MainWindow::LightSweep, reader->readSettingsValue("CentralBroom/speeds.LightSweep").toInt());
    speedForSweepType.insert(MainWindow::MediumSweep, reader->readSettingsValue("CentralBroom/speeds.MediumSweep").toInt());
    speedForSweepType.insert(MainWindow::HeavySweep, reader->readSettingsValue("CentralBroom/speeds.HeavySweep").toInt());
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
    qDebug()<<"BroomState "<<state_;
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
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(BroomBounceOut, 0) > 0) {
            goNone();
            goSlide(!needGoLeft);
            logger->addLog("Щетка: отскок");
        }
    }

    //---------------------------------------------------------------------------
    if (state == CentralBroom::BroomFlowOut){// началось плавание
        setFlowActive(true);
        goNone();
    }

    if (state == CentralBroom::BroomFlowed){// закончилось плавание
        auto isFlowing = ((MainWindow*)parent)->workMode.centralBroomFlow;
        setFlowActive(isFlowing);
    }
    if (state == CentralBroom::BroomFlowIn){// заканчиваем плавание
        setFlowActive(false);
    }
    //----------------------------------------------------------------------------
    startActionTime = QDateTime::currentDateTime();
    if (state == CentralBroom::BroomRotateOut){
        logger->addLog("Щетка раскручивается");
    }
    if (state == CentralBroom::BroomRotateIn){// тормозим щетки
        goNoRotate();
        logger->addLog("Щетка останавливается");
    }
    //----------------------------------------------------------------------------
    goNone();
    if (state == CentralBroom::BroomDownOut){// началось опускание
        setDirection(organsEnums::Down);
    }

    if (state == CentralBroom::BroomDownIn){// Запоминаем что поднимание начилось
        setDirection(organsEnums::Up);
    }

    if (state == CentralBroom::BroomSlideOut){
        goSlide(needGoLeft);
    }

    if (state == CentralBroom::BroomSlideIn){
        setDirection(organsEnums::Right);
    }

}

void CentralBroom::goSlide(bool toLeft){
    setDirection(toLeft?organsEnums::Left:organsEnums::Right);
}

void CentralBroom::goLeft(){setDirection(organsEnums::Left);}
void CentralBroom::goRight(){setDirection(organsEnums::Right);}
void CentralBroom::goUp(){
    setDirection(organsEnums::Up);}
void CentralBroom::goDown(){
    setDirection(organsEnums::Down);}

void CentralBroom::goLeft(bool state){
    printMovement(organsEnums::Left, state, isPressed);
    myCan->setState(StateValveF9, state);
    myCan->setState(StateValveA1, state);
}
void CentralBroom::goRight(bool state){
    printMovement(organsEnums::Right, state, isPressed);
    myCan->setState(StateValveF3, state);
    myCan->setState(StateValveA1, state);
}

void CentralBroom::goNone(){
    myCan->setState(StateValveF9, false);
    myCan->setState(StateValveF3, false);
    myCan->setState(StateValveF4, false);
    myCan->setState(StateValveF10, false);

    setDirection(organsEnums::None);
    myCan->setState(StateValveA1, false);
}

void CentralBroom::goUp(bool state, bool isPressed){
    if(isPressed){
        myCan->setState(StateValveA1, state);
        myCan->setState(StateValveF2, state);
    }
    else{
        goUpImmediate(state);
    }
    printMovement(organsEnums::Up, state, isPressed);
}


void CentralBroom::goDown(bool state, bool isPressed){
    if(isPressed){
        myCan->setState(StateValveF8, state);
        myCan->setState(StateValveA1, state);
    }
    else{
        goDownImmediate(state);
    }
    printMovement(organsEnums::Down, state, isPressed);
}

void CentralBroom::goUpImmediate(bool state){
    if(state){
        ((MainWindow*)parent)->tryToDisableBroomFlow();
    }
    myCan->setState(StateValveA1, state);
    myCan->setState(StateValveF10, state);
}

void CentralBroom::goDownImmediate(bool state){
    if(state){
        ((MainWindow*)parent)->tryToDisableBroomFlow();
    }
    myCan->setState(StateValveA1, state);
    myCan->setState(StateValveF4, state);
}

void CentralBroom::printMovement(organsEnums::Direction dir, bool state, bool isPressed){
    logger->printMovementLog(isPressed && (dir == organsEnums::Up||dir == organsEnums::Down)?
                                 organsEnums::Broom:
                                 organsEnums::BroomBlock,
                             dir,
                             state?"": " завершено");
}

void CentralBroom::goPressUp(bool state){
    myCan->setState(StateValveF2, state);
    myCan->setState(StateValveA1, state);
}

void CentralBroom::goPressDown(bool state){
    myCan->setState(StateValveF8, state);
    myCan->setState(StateValveA1, state);
}

void CentralBroom::stopPress(){
    myCan->setState(StateValveF2, false);
    myCan->setState(StateValveF8, false);
    myCan->setState(StateValveA1, false);
}

void CentralBroom::setDirection(organsEnums::Direction dir){
    setDirection(dir, isPressed);
}

void CentralBroom::setDirection(organsEnums::Direction dir, bool pressed){
    qDebug()<<"!!! direction: "<< dir;
    if(dir == direction && pressed == isPressed)
        return;
    qDebug()<<"!!! "<< dir;
    switch (direction) {
        case organsEnums::Up:
            goUp(false, isPressed);
            break;
        case organsEnums::Down:
            goDown(false, isPressed);
            break;
        case organsEnums::Left:
            goLeft(false);
            break;
        case organsEnums::Right:
            goRight(false);
            break;
        default:
            break;
    }
    //printMovement(dir, false, isPressed);
    direction = dir;
    setPressActive(pressed);

    switch (dir) {
        case organsEnums::Up:
            goUp(true, pressed);
            break;
        case organsEnums::Down:
            goDown(true, pressed);
            break;
        case organsEnums::Left:
            goLeft(true);
            break;
        case organsEnums::Right:
            goRight(true);
            break;
        default:
            goNone();
            break;
    //printMovement(dir, true, pressed);
    }
}

void CentralBroom::setPressActive(bool state){
    //qDebug()<<"### setPressed: "<<state;
    if(isPressed == state)
        return;
    if(direction!= organsEnums::None){
        switch (direction) {
        case organsEnums::Up:
            goUp(false, isPressed);
            goUp(true, state);
            break;
        case organsEnums::Down:
            goDown(false, isPressed);
            goDown(false, state);
            break;
        default:
            break;}
    }
    isPressed = state;
    //((MainWindow*)parent)->setBroomPressView(state);
    logger->addLogWarning(state?"Щетка: прижим активирован":"Щетка: прижим деактивирован");
}


// void CentralBroom::setPressActive(bool state){
//     if(isPressed == state)
//         return;
//     if(direction!= organsEnums::None)
//         logger->addLogWarning(state?"Щетка: прижим активирован":"Щетка: прижим деактивирован");

//     if(isPressed)
//         stopPress();
//     else
//         goNone();

//     isPressed = state;
// }

void CentralBroom::setFlowActive(bool state){
    qDebug()<<"###flow!!!  "<<state;
    if(isFlowing == state)
        return;
    isFlowing = state;
    // QString msg = (state? "Switch broom flow to true": "Switch broom flow to false");
    // logger->printTest(msg);
    // //если двигались вверх или вниз без поджима и включили плавающий режим - прекращаем движение
    // if(state && !isPressed
    //     && (direction == organsEnums::Up ||direction == organsEnums::Down)){
    //     setDirection(organsEnums::None);
    // }
    goFlow(state);
   // ((MainWindow*)parent)->setBroomFlowView(state);
}

void CentralBroom::goFlow(bool state){
    logger->addLogWarning(state?"Щетка: плавание активировано":"Щетка: плавание деактивировано");
    myCan->setState(StateValveC1, state);
    myCan->setState(StateValveC2, state);
}

void CentralBroom::goRotate(int speed_){
    myCan->setState(StateValveD1, speed_ / 2);
    //myCan->setState(StateValveA1, true);
}

void CentralBroom::goNoRotate(){
    myCan->setState(StateValveD1, 0);
}

// void CentralBroom::increaseSpeed(){
//     int spd = myCan->getState(StateValveD1).toUInt() * 2;
//     if (spd + 10 < 100)
//         spd += 10;
//     else
//         spd = 100;
//     goRotate(spd);
// }

// void CentralBroom::decreaseSpeed(){
//     int spd = myCan->getState(StateValveD1).toUInt() * 2;
//     if (spd - 10 > 0)
//         spd -= 10;
//     else
//         spd = 0;
//     goRotate(spd);
// }

CentralBroom::BroomStates CentralBroom::getState(){
    return state;
}

void CentralBroom::setNeedState(BroomStates state_){
    needState = state_;
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
        logger->printMovementLog(organsEnums::BroomBlock, dir, " остановлено, достигнут датчик");
        return true;
    }
    else{
        if(timeoutReached){
            if (!broomAlarmed){
                logger->printMovementLog(organsEnums::BroomBlock, dir," достигнут тайм-аут");
                goNone();
            }
            broomAlarmed = true;
            return true;
        }
    }
    return false;
}

bool CentralBroom::testStateTimer(){// мощная функция проверки таймаута одновременно с концевиками и прочими условиями (для каждого состояния)
    bool timeoutReached = isTimeoutReached();
    bool movementFinished = false;

    //    if (state == CentralBroom::BroomDownOut && timeTest)
    //        dkpAndPositionTest = true;
    if (timeoutReached) {
        if (state == CentralBroom::BroomRotateOut ||
            state == CentralBroom::BroomRotateIn ||
            state == CentralBroom::BroomFlowOut ||
            state == CentralBroom::BroomFlowIn ||
            state == CentralBroom::BroomBounceOut) {
            movementFinished = true;
        }
    }

    if (state == CentralBroom::BroomDownOut){// проверяем концевики
        if (timeoutReached){
            logger->printMovementLog( organsEnums::BroomBlock, organsEnums::Down, " достигнут тайм-аут");
            movementFinished = true;
        }
    }

    if (state == CentralBroom::BroomDownIn){// рейка идет вверх, ждем концевик
        const bool sensorReached = myCan->getState(StateDKPBroomUp).toBool();
        if(checkMovementAndStopOnTimeout(timeoutReached, sensorReached, organsEnums::Up)){
            movementFinished = true;
        }
    }

    if (state == CentralBroom::BroomSlideOut){
        const bool sensorReached = needGoLeft ? myCan->getState(StateDKPBroomLeft).toBool() : myCan->getState(StateDKPBroomRight).toBool();
        if(checkMovementAndStopOnTimeout(timeoutReached, sensorReached, needGoLeft? organsEnums::Left: organsEnums::Right)){
            movementFinished = true;
        }
    }
    if (state == CentralBroom::BroomSlideIn){
        const bool sensorReached = myCan->getState(StateDKPBroomRight).toBool();
        if(checkMovementAndStopOnTimeout(timeoutReached, sensorReached, organsEnums::Right)){
            movementFinished = true;
        }
    }

    if (movementFinished){
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

CentralBroom::BroomStates CentralBroom::getNextState(CentralBroom::BroomStates current){
    switch (current) {
    case BroomOff: return BroomSlideOut;// начинаем опускание
    case BroomDownOut: return BroomDowned;// заканчиваем опускание по таймеру
    case BroomDownIn:return BroomDownOut;// меняем направление на опускание (до этого поднимались)
    case BroomDowned:return BroomFlowOut;// начинаем вращение или поворот
    case BroomFlowIn:return BroomFlowOut;
    case BroomFlowOut:return BroomFlowed;
    case BroomFlowed:return BroomRotateOut; // начинаем вращение или поворот
    case BroomRotated:return BroomDownOut;// начинаем вращение или поворот
    case BroomRotateOut:return BroomRotated; // заканчиваем раскрутку
    case BroomRotateIn:return BroomRotateOut;// меняем направление раскрутки ( до этого тормозились)
    case BroomSlideOut:return BroomSlided;// заканчиваем поворот щетки
    case BroomSlideIn:return BroomSlideOut;
    case BroomSlided: return BroomBounceOut;
    case BroomBounceOut:return BroomBounced;
    case BroomBounced:return BroomRotateOut;
    default: return current;}
}

CentralBroom::BroomStates CentralBroom::getPreviousState(CentralBroom::BroomStates current){
    switch (current) {
    case BroomDownOut: return BroomDownIn;// меняем направление на поднимание (до этого опускались)
    case BroomDownIn:return BroomRotated;// заканчиваем подъем по таймеру и переходим в стостояние готовности к включению
    case BroomDowned:return BroomDownIn;// начинаем поднимаение по таймеру
    case BroomFlowIn:return BroomDowned;
    case BroomFlowOut:return BroomFlowIn;
    case BroomFlowed:return BroomFlowIn; // начинаем вращение или поворот
    case BroomSlideOut:return BroomSlideIn;// заканчиваем поворот щетки
    case BroomSlideIn:return BroomOff;
    case BroomSlided: return BroomSlideIn;
    case BroomRotateOut:return BroomRotateIn; // заканчиваем раскрутку
    case BroomRotateIn:return BroomBounced;// меняем направление раскрутки ( до этого тормозились)
    case BroomRotated:return BroomRotateIn;// начинаем вращение или поворот
    case BroomBounceOut:return BroomBounced;
    case BroomBounced:return BroomSlided;
    default: return current;}
}

CentralBroom::BroomStates CentralBroom::stateUp(){// пытаемся прогрессировать статусом вверх (если что меняем направление статуса, если вдруг был понижающий прогресс)
    if(state == BroomDownOut||
        state == BroomFlowOut||
        state == BroomRotateOut||
        state == BroomSlideOut||
        state == BroomBounceOut){
        if(!testStateTimer()){
            return state;
        }
    }
    auto newState = getNextState(state);
    if(newState != state){
        setState(newState);
        qDebug()<<"setState: "<<newState;
    }
    return state;
}

CentralBroom::BroomStates CentralBroom::stateDown()
{// пытаемся прогрессировать статусом вниз (если что меняем направление статуса, если вдруг был повышающий прогресс)

    if(state == BroomDownIn||
        state == BroomSlideIn||
        state == BroomRotateIn){
        if (!testStateTimer())
            return state;
    }
    auto newState = getPreviousState(state);
    if(state!=newState){
        setState(newState);
    }
    return state;
}
