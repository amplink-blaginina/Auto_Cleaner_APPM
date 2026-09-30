#include "blower.h"
#include <settingsreader.h>
#include <machine/sweeptype.h>
#include <QDebug>
#include <QMetaEnum>
#include <QTimer>
#include <QThread>

Blower::Blower(const MachineIo &machine, MachineContext *context, ViewController *logger_, QObject *parent) : QObject(parent)
{
    io = machine.io;
    hydraulics = machine.hydraulics;
    engineRpm = machine.engineRpm;
    logger = logger_;
    _context = context;
    setState(BlowerOff);
    setNeedState(BlowerOff);
    startClean = false;
    choosed = false;
    blowerAlarmed = false;

    readSettings();

    connect(&progressTimer, SIGNAL(timeout()), this, SLOT(progressLoop()));
    progressTimer.start(100);
}

void Blower::readSettings()
{
    timeouts.clear();
    rpmForSweepType.clear();
    speedForSweepType.clear();

    auto reader = _context->settingsReader();
    rpmForSweepType.insert(LeafSweep, reader->readSettingsValue("Engine/rpm.LeafSweep").toInt());
    rpmForSweepType.insert(LightSweep, reader->readSettingsValue("Engine/rpm.LightSweep").toInt());
    rpmForSweepType.insert(MediumSweep, reader->readSettingsValue("Engine/rpm.MediumSweep").toInt());
    rpmForSweepType.insert(HeavySweep, reader->readSettingsValue("Engine/rpm.HeavySweep").toInt());

    // назначаем таймауты на длительные операции
    timeouts.insert(BlowerSlideOut, reader->readSettingsValue("Blower/timeouts.BlowerSlideOut").toInt());
    timeouts.insert(BlowerSlideIn, reader->readSettingsValue("Blower/timeouts.BlowerSlideIn").toInt());
    timeouts.insert(BlowerDownOut, reader->readSettingsValue("Blower/timeouts.BlowerDownOut").toInt());
    timeouts.insert(BlowerDownIn, reader->readSettingsValue("Blower/timeouts.BlowerDownIn").toInt());
    timeouts.insert(BlowerRotateOut, reader->readSettingsValue("Blower/timeouts.BlowerRotateOut").toInt());
    timeouts.insert(BlowerRotateIn, reader->readSettingsValue("Blower/timeouts.BlowerRotateIn").toInt());

    speedForSweepType.insert(LeafSweep, reader->readSettingsValue("Blower/speeds.LeafSweep").toInt());
    speedForSweepType.insert(LightSweep, reader->readSettingsValue("Blower/speeds.LightSweep").toInt());
    speedForSweepType.insert(MediumSweep, reader->readSettingsValue("Blower/speeds.MediumSweep").toInt());
    speedForSweepType.insert(HeavySweep, reader->readSettingsValue("Blower/speeds.HeavySweep").toInt());

    qDebug() << timeouts;
}

QString Blower::toString(BlowerStates s){
    const char *key = QMetaEnum::fromType<BlowerStates>().valueToKey(s);
    return key ? QString::fromLatin1(key) : QStringLiteral("UnknownState");
}

void Blower::setDirection(bool isRight){
    isTargetRight = isRight;
}

void Blower::toggleSide(bool right){
    const bool wasSelected = right ? _right : _left;
    _left = !right && !wasSelected;
    _right = right && !wasSelected;
    emit selectionChanged();
}

void Blower::setSide(bool right){
    _left = !right;
    _right = right;
    emit selectionChanged();
}

void Blower::setLifted(bool lifted){
    _lifted = lifted;
    emit selectionChanged();
}

void Blower::setState(BlowerStates state_){
    state = state_;

    if (state == Blower::BlowerOff){// выключили
        goOff();
        //io->set(StateFRMBackL2, false);
    }
    if (state == Blower::BlowerDownOut){// началось опускание
        startActionTime = QDateTime::currentDateTime();
        goOff();
        goDown();
        //io->set(StateFRMBackL2, true);
    }
    if (state == Blower::BlowerDowned){
        goOff();
    }
    if (state == Blower::BlowerDownIn){// Запоминаем что поднимание начилось
        startActionTime = QDateTime::currentDateTime();
        goOff();
        goUp();
    }
    if (state == Blower::BlowerSlideOut){// поворот
        startActionTime = QDateTime::currentDateTime();

        goSlide(rightBlow);
    }
    if (state == Blower::BlowerSlideIn){// поворот
        startActionTime = QDateTime::currentDateTime();
    }
    if (state == Blower::BlowerSlided){// поворот
        goOff();
    }

    if (state == Blower::BlowerRotateOut){// раскручивание
        startActionTime = QDateTime::currentDateTime();
    }
    if (state == Blower::BlowerRotateIn){// остановка
        startActionTime = QDateTime::currentDateTime();
        setTargetRotationSpeed(0);
    }
}

void Blower::goOff(){
    io->set(StateValveE7, false);
    io->set(StateValveE3, false);
    io->set(StateValveE1, false);
    io->set(StateValveE5, false);
    hydraulics->request(this, false);
}

void Blower::goRotate(quint8 speed){
    //qDebug()<<"# RotationSpeed: "<<speed;
    io->set(StateValveD3, speed);
}

void Blower::goSlide(bool turn_right){
    if (turn_right){
        hydraulics->request(this, true);
        io->set(StateValveE7, true);
    }
    else{
        hydraulics->request(this, true);
        io->set(StateValveE3, true);
    }
}

void Blower::goUp(){
    hydraulics->request(this, true);
    io->set(StateValveE1, true);
}

void Blower::goDown()
{
    hydraulics->request(this, true);
    io->set(StateValveE5, true);
}

void Blower::goNone(){
    hydraulics->request(this, false);
    io->set(StateValveE5, false);
    io->set(StateValveE1, false);
    io->set(StateValveE3, false);
    io->set(StateValveE7, false);
}

Blower::BlowerStates Blower::getState(){
    return state;
}

void Blower::setNeedState(BlowerStates state_){
    needState = state_;
}

Blower::BlowerStates Blower::getAbleState(){
    return ableState;
}

Blower::BlowerStates Blower::getNeedState(){
    return needState;
}

void Blower::checkNeedState(){// утанавливает максимальную границу до которой может дойти обдув (при текущих параметрах)
    ableState = (!startClean || needState == BlowerOff)?BlowerOff:BlowerRotated;
}

int Blower::getTimeout(){//получает таймаут в секундах (сколько надо простаивать в той или иной операции)
    return timeouts.value(state, 0);
}

bool Blower::testStateTimer(){// мощная функция проверки таймаута одновременно с концевиками и прочими условиями (для каждого состояния)
    qint64 msecs_to = startActionTime.msecsTo(QDateTime::currentDateTime());
    // qint64 tmp_msecs = msecs_to;
    // if (msecs_to > getTimeout() * 1000)
    //     tmp_msecs = getTimeout() * 1000;
    bool timeTest = false;
    if (msecs_to > getTimeout() * 1000){// тест по времени прошел а мы ничего не достигли. Нужны тревоги
        timeTest = true;
        //return true;
    }

    // проверяем концевики
    bool dkpAndPositionTest = false;
    // магнимт идет вверх, ждем концевик
    if (state == Blower::BlowerDownIn){
        const bool sensorReached = io->get(StateDKPBlowerUp1).toBool() && io->get(StateDKPBlowerUp2).toBool();
        if (timeTest && !sensorReached){
            if (!blowerAlarmed){
                logger->addLog("Продувка: достигнут тайм-аут");
                goOff();
            }
            blowerAlarmed = true;
        }
        else if (sensorReached){
            logger->addLog("Продувка: достигнут датчик");
        }
        if (timeTest || sensorReached)
            dkpAndPositionTest = true;// не ждем таймера и разрешаем завершить процесс

    }
    // вниз концевика нет. если таймер прошел то считаем что все ок
    if ((state == Blower::BlowerDownOut
         || state == Blower::BlowerSlideOut
         || state == Blower::BlowerRotateOut
         || state == Blower::BlowerSlideIn
         || state == Blower::BlowerRotateIn) && timeTest)
        dkpAndPositionTest = true;

    if (dkpAndPositionTest){
        blowerAlarmed = false;
        return true;// достигнут концевик или нужное положение (мы молодцы)
    }

    return false;
}

void Blower::checkFriendVars(){
    startClean = _context->isCleaning();
    rightBlow = _right;
    //qDebug()<<"# Set target direction 3: "<<(isTargetRight?"right":"left");
    //isTargetRight = rightBlow;
}

void Blower::setTargetRotationSpeed(float speed){
    targetRotationSpeed = speed;
}

void Blower::changeRotationSpeed(){
    if(currentRotationSpeed < targetRotationSpeed){
        currentRotationSpeed += speedRotationStep;
        if (currentRotationSpeed>targetRotationSpeed){
            currentRotationSpeed = targetRotationSpeed;
        }
        goRotate(currentRotationSpeed);
        qDebug()<<"# blower speed up: "<<currentRotationSpeed;
        return;
    }

    if(currentRotationSpeed > targetRotationSpeed){
        currentRotationSpeed-=speedRotationStep;
        if(currentRotationSpeed<0){
            currentRotationSpeed = 0;
        }
        goRotate(currentRotationSpeed);
        qDebug()<<"# blower speed down: "<<currentRotationSpeed;
        return;
    }
}

void Blower::progressLoop(){
    checkFriendVars();// проверяет соседние модули и собирает информацию о их состояниях (нажатые кнопки, обороты, статусы и пр.)
    checkNeedState();// проверяет до какого состояния может добираться щетка

    if (state >= Blower::BlowerRotateOut){
        auto type = _context->sweepType();
        engineRpm->request(this, rpmForSweepType.value(type) * 8);//обороты движка
        setTargetRotationSpeed(speedForSweepType.value(type));// скорость щеток
        //goRotate();
    }
    else if (state <= Blower::BlowerDowned){
        engineRpm->release(this);// вентилятор остановлен - обороты обдуву не нужны
    }
    updateTransitioning();
    // if(isTargetRight != rightBlow){
    //     rotate();
    // }

    // if (state < needState && state < ableState){// нужно прогрессировать вверх (выдвигать, мыть и гусей не забыть)
    //     BlowerStates s = state;
    //     stateUp();
    //     if (s != state)// && (state == needState || state == ableState))
    //     {
    //         qDebug() << "Blower state " << toString(state);
    //     }
    // }
    // else if (state > needState || state > ableState){// прогрессируем вниз
    //     BlowerStates s = state;
    //     stateDown();
    //     if (s != state)// && (state == needState || state == ableState))
    //     {
    //         qDebug() << "Blower state " << toString(state);
    //     }
    // }
    changeRotationSpeed();
}

void Blower::updateTransitioning(){
    //qDebug()<<"# "<<isTargetRight<<"/"<<rightBlow;

    if(_context->isCleaning() && isTargetRight != rightBlow){
        //qDebug()<<"## ";
        rotate();
        return;
    }

    if (state < needState && state < ableState){// нужно прогрессировать вверх (выдвигать, мыть и гусей не забыть)
        BlowerStates s = state;
        stateUp();
        if (s != state)// && (state == needState || state == ableState))
        {
            qDebug() << "Blower state " << toString(state);
        }
    }
    else if (state > needState || state > ableState){// прогрессируем вниз
        BlowerStates s = state;
        stateDown();
        if (s != state)// && (state == needState || state == ableState))
        {
            qDebug() << "Blower state " << toString(state);
        }
    }
}

bool Blower::isRotating(){
    return currentRotationSpeed > 0;
}

void Blower::setStartMomentForStopping(){
    //qDebug()<<"# wait for stop!";
    stoppingStartedAt = QDateTime::currentDateTime().time();
}

void Blower::setStartMomentForStarting(){
    //qDebug()<<"# wait for start!";
    startingStartedAt = QDateTime::currentDateTime().time();
}

void Blower::setStartMomentForRotation(){
    rotationStartedAt = QDateTime::currentDateTime().time();

}

bool Blower::isHeldLongEnough(const QTime &since) const{
    // кнопку держат дольше stopDelay секунд (считаем в мс, иначе целые секунды дают лишнюю секунду)
    if (!since.isValid())
        return false;
    return qAbs(since.msecsTo(QDateTime::currentDateTime().time())) > stopDelay * 1000;
}
void Blower::updateWhenRotationPressed(bool isRight){
    if (!_context->isCleaning())
        return;// уборка не запущена - кнопка только выбирает сторону, гидравлику не трогаем

    if(isHeldLongEnough(rotationStartedAt)){
        //setState (BlowerOff);
        //qDebug()<<"# Set target direction 2: "<<(isTargetRight?"right":"left");
        isTargetRight = isRight;
        if (!_left && !_right){
            // сторона не выбрана - выбираем удерживаемую, иначе обдув останется выключенным
            _lifted = false;
            rightBlow = isRight;
            setSide(isRight);
        }
        else if (_lifted)// удержание стороны снова разворачивает поднятый обдув
            setLifted(false);
        //setSide(isRight);
        setNeedState(BlowerRotated);
    }
    // вручную раструб не поворачиваем: включённый обдув не может стоять в промежуточном положении,
    // поворот до крайнего положения делает автомат после удержания
}

void Blower::updateWhenUpPressed(){
    if(isHeldLongEnough(stoppingStartedAt)){
        // поднимаем обдув, выбранная сторона остаётся подсвеченной
        if (_context->isCleaning() && !_lifted){
            setLifted(true);
        }
        setNeedState(BlowerOff);
    }

    if(!isRotating()){
        qDebug()<<"# blower move up";
        goUp();
    }
    else{
        qDebug()<<"# speed: "<<currentRotationSpeed<<"/"<<targetRotationSpeed;
    }
}

void Blower::updateWhenDownPressed(){
    if(isHeldLongEnough(startingStartedAt)){
        // запуск удержанием: подсвечиваем сторону обдува, по умолчанию правую
        if (_context->isCleaning() && needState != BlowerRotated){
            _lifted = false;
            const bool right = !_left;
            isTargetRight = right;
            rightBlow = right;
            setSide(right);
        }
        setNeedState(BlowerRotated);//setState(BlowerOff);
    }

    if(!isRotating()){
        qDebug()<<"# blower move down";
        goDown();
    }
    else{
        qDebug()<<"# speed: "<<currentRotationSpeed<<"/"<<targetRotationSpeed;
    }
}

Blower::BlowerStates Blower::rotate(){
    //qDebug()<<"# rotate: "<<isTargetRight<<"/"<<rightBlow;
    //QString message = (isTargetRight + "#" + rightBlow);
    //logger->addLog(message);
    //logger->addLog(" # Rotation state: "+ state);

    switch (state) {
    case BlowerOff:
        // начинаем опускание
        logger->addLog("Обдув: опускаем раструб");
        setState(BlowerDownOut);
        break;
    case BlowerDownOut:
        // заканчиваем опускание по таймеру
        if (testStateTimer()){
            logger->addLog("Обдув: заканчиваем опускание по таймеру");
            setState(BlowerDowned);
        }
        break;
    case BlowerDownIn:
        logger->addLog("Обдув: меняем направление на опускание");
        // меняем направление на опускание (до этого поднимались)
        setState(BlowerDownOut);
        break;
    case BlowerDowned:
        logger->addLog("Обдув: выставлем направление обдува");

        if(isTargetRight != rightBlow){
            //qDebug()<<"# Set target direction 1: "<<(isTargetRight?"right":"left");
            rightBlow = isTargetRight;
            setSide(isTargetRight);
        }
        setState(BlowerSlideOut);
        break;
    case BlowerSlideOut:
        // заканчиваем поворот щетки
        if (testStateTimer()){
            logger->addLog("Обдув: заканчиваем поворот щётки");
            setState(BlowerSlided);
        }
        break;
    case BlowerSlideIn:
        if(isTargetRight != rightBlow){
            if (testStateTimer())
                setState(BlowerDowned);
        }
        else{
            setState(BlowerSlideOut);}
        break;
    case BlowerSlided:
        if(isTargetRight != rightBlow){
            setState(BlowerSlideIn);
        }
        else{
        logger->addLog("Обдув: раскручиваем вентилятор");
            setState(BlowerRotateOut);}
        break;
    case BlowerRotateOut:
        if(isTargetRight != rightBlow){
            setState(BlowerRotateIn);
        }
        else{
        // заканчиваем раскрутку а так же регулируем обороты дизеляки
        if (testStateTimer())
            setState(BlowerRotated);
        }
        break;
    case BlowerRotateIn:
        if(isTargetRight != rightBlow){
            if (testStateTimer())
                setState(BlowerSlided);
        }
        else{
        // меняем направление раскрутки ( до этого тормозились)
            setState(BlowerRotateOut);}
        break;
    case BlowerRotated:
        if(isTargetRight != rightBlow){
            logger->addLog("Обдув: выключаем вентилятор");
            setState(BlowerRotateIn);
        }

        break;
    default:
        break;
    }
    return state;
}

Blower::BlowerStates Blower::stateUp(){// пытаемся прогрессировать статусом вверх (если что меняем направление статуса, если вдруг был понижающий прогресс)
    switch (state) {
    case BlowerOff:
        // начинаем опускание
        logger->addLog("Опускаем раструб");
        setState(BlowerDownOut);
        break;
    case BlowerDownOut:
        // заканчиваем опускание по таймеру
        if (testStateTimer())
            setState(BlowerDowned);
        break;
    case BlowerDownIn:
        // меняем направление на опускание (до этого поднимались)
        setState(BlowerDownOut);
        break;
    case BlowerDowned:
        logger->addLog("Выставлем направление обдува");
        setState(BlowerSlideOut);
        break;
    case BlowerSlideOut:
        // заканчиваем поворот щетки
        if (testStateTimer())
            setState(BlowerSlided);
        break;
    case BlowerSlideIn:
        setState(BlowerSlideOut);
        break;
    case BlowerSlided:
        logger->addLog("Раскручиваем вентилятор");
        setState(BlowerRotateOut);
        break;
    case BlowerRotateOut:
        // заканчиваем раскрутку а так же регулируем обороты дизеляки
        if (testStateTimer())
            setState(BlowerRotated);
        break;
    case BlowerRotateIn:
        // меняем направление раскрутки ( до этого тормозились)
        setState(BlowerRotateOut);
        break;
    default:
        break;
    }
    return state;
}

Blower::BlowerStates Blower::stateDown()
{// пытаемся прогрессировать статусом вниз (если что меняем направление статуса, если вдруг был повышающий прогресс)
    switch (state) {
    case BlowerDownOut:
        // меняем направление на поднимание (до этого опускались)
        setState(BlowerDownIn);
        break;
    case BlowerDownIn:
        // заканчиваем подъем по таймеру и переходим в стостояние готовности к включению
        if (testStateTimer())
            setState(BlowerOff);
        break;
    case BlowerDowned:
        // начинаем поднимаение по таймеру
        logger->addLog("Поднимаем раструб");
        setState(BlowerDownIn);
        break;
    case BlowerSlideOut:
        setState(BlowerSlideIn);
        break;
    case BlowerSlideIn:
        if (testStateTimer())
            setState(BlowerDowned);
        break;
    case BlowerSlided:
        setState(BlowerSlideIn);
        break;
    case BlowerRotateOut:
        setState(BlowerRotateIn);
        break;
    case BlowerRotateIn:
        if (testStateTimer())
            setState(BlowerSlided);
        break;
    case BlowerRotated:
        logger->addLog("Выключаем вентилятор");
        setState(BlowerRotateIn);
        break;
    default:
        break;
    }
    return state;
}
