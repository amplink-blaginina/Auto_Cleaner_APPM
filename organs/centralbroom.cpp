#include "centralbroom.h"
#include <settingsreader.h>
#include <machine/sweeptype.h>
#include <QDebug>
#include <QMetaEnum>
#include <QTimer>
#include <QThread>

CentralBroom::CentralBroom(const MachineIo &machine, MachineContext *context, ViewController *logger_, QObject *parent) : QObject(parent){
    io = machine.io;
    hydraulics = machine.hydraulics;
    engineRpm = machine.engineRpm;
    logger = logger_;
    _context = context;
    setState(BroomOff);
    setNeedState(BroomOff);
    needGoLeft = false;
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
    auto reader = _context->settingsReader();

    rpmForSweepType.insert(LeafSweep, reader->readSettingsValue("Engine/rpm.LeafSweep").toInt());
    rpmForSweepType.insert(LightSweep, reader->readSettingsValue("Engine/rpm.LightSweep").toInt());
    rpmForSweepType.insert(MediumSweep, reader->readSettingsValue("Engine/rpm.MediumSweep").toInt());
    rpmForSweepType.insert(HeavySweep, reader->readSettingsValue("Engine/rpm.HeavySweep").toInt());

    timeouts.insert(BroomSlideOut, reader->readSettingsValue("CentralBroom/timeouts.BroomSlideOut").toFloat());
    timeouts.insert(BroomSlideIn, reader->readSettingsValue("CentralBroom/timeouts.BroomSlideIn").toFloat());
    timeouts.insert(BroomBounceOut, reader->readSettingsValue("CentralBroom/timeouts.BroomBounceOut").toFloat());
    timeouts.insert(BroomDownOut, reader->readSettingsValue("CentralBroom/timeouts.BroomDownOut").toFloat());
    timeouts.insert(BroomDownIn, reader->readSettingsValue("CentralBroom/timeouts.BroomDownIn").toFloat());
    timeouts.insert(BroomFlowOut, reader->readSettingsValue("CentralBroom/timeouts.BroomFlowOut").toFloat());
    timeouts.insert(BroomRotateOut, reader->readSettingsValue("CentralBroom/timeouts.BroomRotateOut").toFloat());
    timeouts.insert(BroomRotateIn, reader->readSettingsValue("CentralBroom/timeouts.BroomRotateIn").toFloat());

    speedForSweepType.insert(LeafSweep, reader->readSettingsValue("CentralBroom/speeds.LeafSweep").toInt());
    speedForSweepType.insert(LightSweep, reader->readSettingsValue("CentralBroom/speeds.LightSweep").toInt());
    speedForSweepType.insert(MediumSweep, reader->readSettingsValue("CentralBroom/speeds.MediumSweep").toInt());
    speedForSweepType.insert(HeavySweep, reader->readSettingsValue("CentralBroom/speeds.HeavySweep").toInt());

    lowerTimeSec = reader->readSettingsValue("CentralBroom/lowerTimeSec").toFloat();
    raiseTimeSec = reader->readSettingsValue("CentralBroom/raiseTimeSec").toFloat();
    spinHeight = qBound(0, reader->readSettingsValue("CentralBroom/spinHeightPercent").toInt(), 100) / 100.0;
}

QString CentralBroom::toString(BroomStates s){
    const char *key = QMetaEnum::fromType<BroomStates>().valueToKey(s);
    return key ? QString::fromLatin1(key) : QStringLiteral("UnknownState");
}

void CentralBroom::setState(BroomStates state_){
    if(state == state_){
        return;
    }
    state = state_;

    // автомат знает положение точно: опускание закончено - внизу, подъём закончен (или ещё не опускали) - вверху
    if (state == CentralBroom::BroomDowned)
        heightEstimate = 1;
    if (state == CentralBroom::BroomRotated)
        heightEstimate = 0;

    if (state == CentralBroom::BroomOff||//щётка в крайне верхнем положении // остановим поднимаение
        state == CentralBroom::BroomDowned||//щётка в крайне нижнем положении
        state == CentralBroom::BroomSlided||//щётка в крайне боковом положении
        state == CentralBroom::BroomBounced){// отскок завершён

        goNone();
        //io->set(StateFRMBroomL1, false);
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
        auto isFlowing = _flowSelected;
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
    io->set(StateValveF9, state);
    hydraulics->request(this, state);
}
void CentralBroom::goRight(bool state){
    printMovement(organsEnums::Right, state, isPressed);
    io->set(StateValveF3, state);
    hydraulics->request(this, state);
}

void CentralBroom::goNone(){
    io->set(StateValveF9, false);
    io->set(StateValveF3, false);
    io->set(StateValveF4, false);
    io->set(StateValveF10, false);

    setDirection(organsEnums::None);
    hydraulics->request(this, false);
}

void CentralBroom::goUp(bool state, bool isPressed){
    if(isPressed){
        hydraulics->request(this, state);
        io->set(StateValveF2, state);
        printMovement(organsEnums::Up, state, isPressed);
    }
    else{
        goUpImmediate(state);
    }
}


void CentralBroom::goDown(bool state, bool isPressed){
    if(isPressed){
        io->set(StateValveF8, state);
        hydraulics->request(this, state);
        printMovement(organsEnums::Down, state, isPressed);
    }
    else{
        goDownImmediate(state);
    }
}

void CentralBroom::goUpImmediate(bool state){
    if(state){
        emit flowCancelRequested();
    }
    printMovement(organsEnums::Up, state, false);
    hydraulics->request(this, state);
    io->set(StateValveF10, state);
}

void CentralBroom::goDownImmediate(bool state){
    if(state){
        emit flowCancelRequested();
    }
    printMovement(organsEnums::Down, state, false);
    hydraulics->request(this, state);
    io->set(StateValveF4, state);
}

void CentralBroom::printMovement(organsEnums::Direction dir, bool state, bool isPressed){
    logger->printMovementLog(isPressed && (dir == organsEnums::Up||dir == organsEnums::Down)?
                                 organsEnums::Broom:
                                 organsEnums::BroomBlock,
                             dir,
                             state?"": " завершено");
}

void CentralBroom::goPressUp(bool state){
    io->set(StateValveF2, state);
    hydraulics->request(this, state);
}

void CentralBroom::goPressDown(bool state){
    io->set(StateValveF8, state);
    hydraulics->request(this, state);
}

void CentralBroom::stopPress(){
    io->set(StateValveF2, false);
    io->set(StateValveF8, false);
    hydraulics->request(this, false);
}

void CentralBroom::setDirection(organsEnums::Direction dir){
    setDirection(dir, isPressed);
}

void CentralBroom::setDirection(organsEnums::Direction dir, bool pressed){
    if(dir == direction && pressed == isPressed)
        return;
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
    }
}

void CentralBroom::setPressActive(bool state){
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
    logger->addLog(state?"Щетка: прижим активирован":"Щетка: прижим деактивирован");
}

void CentralBroom::setFlowActive(bool state){
    if(isFlowing == state)
        return;
    isFlowing = state;
    goFlow(state);
}

void CentralBroom::goFlow(bool state){
    logger->addLog(state?"Щетка: плавание активировано":"Щетка: плавание деактивировано");
    io->set(StateValveC1, state);
    io->set(StateValveC2, state);
}

void CentralBroom::updateHeightEstimate(){
    const double dt = heightClock.isValid() ? heightClock.restart() / 1000.0 : 0;
    if (!heightClock.isValid())
        heightClock.start();

    if (io->get(StateDKPBroomUp).toBool()){// верхний концевик - точно наверху
        heightEstimate = 0;
        return;
    }
    if (!hydraulics->isOn())
        return;
    if (direction == organsEnums::Down)
        heightEstimate = lowerTimeSec > 0 ? qMin(1.0, heightEstimate + dt / lowerTimeSec) : 1.0;
    else if (direction == organsEnums::Up)
        heightEstimate = raiseTimeSec > 0 ? qMax(0.0, heightEstimate - dt / raiseTimeSec) : 0.0;
}

bool CentralBroom::shouldSpin() const{
    switch (state) {
    case BroomRotateOut:// раскрутка перед опусканием
    case BroomRotated:
    case BroomDownOut:// опускаем уже раскрученной
        return true;
    case BroomDownIn:// подъём: останавливаем сразу, не дожидаясь верхнего положения
    case BroomRotateIn:
        return false;
    default:
        // работа внизу и ручное управление: по высоте - у земли и в плавании крутится, выше порога стоит
        return isFlowing || heightEstimate >= spinHeight;
    }
}

void CentralBroom::updateRotation(){
    const bool spin = shouldSpin();
    if (spin){
        engineRpm->request(this, rpmForSweepType.value(_context->sweepType()) * 8);//обороты движка
        goRotate(speedForSweepType.value(_context->sweepType()));// скорость щеток
    }
    else{
        engineRpm->release(this);// щётка не крутится - обороты ей не нужны
        if (spinning)
            goNoRotate();
    }
    if (spin != spinning && state != BroomRotateOut && state != BroomRotateIn){// о раскрутке и торможении автомат пишет сам
        const QString height = QString::number(qRound(heightEstimate * 100));
        logger->addLog(spin ? "Щетка: раскручиваем (высота " + height + "% хода)"
                            : "Щетка: останавливаем (высота " + height + "% хода)");
    }
    spinning = spin;
}

void CentralBroom::goRotate(int speed_){
    io->set(StateValveD1, speed_ / 2);
    //hydraulics->request(this, true);
}

void CentralBroom::goNoRotate(){
    io->set(StateValveD1, 0);
}

// void CentralBroom::increaseSpeed(){
//     int spd = io->get(StateValveD1).toUInt() * 2;
//     if (spd + 10 < 100)
//         spd += 10;
//     else
//         spd = 100;
//     goRotate(spd);
// }

// void CentralBroom::decreaseSpeed(){
//     int spd = io->get(StateValveD1).toUInt() * 2;
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
        const bool sensorReached = io->get(StateDKPBroomUp).toBool();
        if(checkMovementAndStopOnTimeout(timeoutReached, sensorReached, organsEnums::Up)){
            movementFinished = true;
        }
    }

    if (state == CentralBroom::BroomSlideOut){
        const bool sensorReached = needGoLeft ? io->get(StateDKPBroomLeft).toBool() : io->get(StateDKPBroomRight).toBool();
        if(checkMovementAndStopOnTimeout(timeoutReached, sensorReached, needGoLeft? organsEnums::Left: organsEnums::Right)){
            movementFinished = true;
        }
    }
    if (state == CentralBroom::BroomSlideIn){
        const bool sensorReached = io->get(StateDKPBroomRight).toBool();
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
    startClean = _context->isCleaning();
}

void CentralBroom::progressLoop(){
    // рисуем положение щетки (в зависимости от прижима)
    //broomWidget->setGeometry(broomWidget->geometry().x(), 418 + io->get(StateBroomPressLevelD7).toUInt(), broomWidget->geometry().width(), broomWidget->geometry().height());

    updateHeightEstimate();
    updateRotation();

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
    case BroomDownIn:return BroomRotateOut;// подъём прервали: щётка уже остановлена - сначала раскручиваем, потом опускаем
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
