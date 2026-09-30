#include "blower.h"
#include "mainwindow.h"
#include <QDebug>
#include <QTimer>
#include <QThread>

Blower::Blower(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ViewController *logger_, MainWindow* mainWindow, QObject *parent_) : QObject(parent_)
{
    myCan = myCan_;
    myCanJ1939 = myCanJ1939_;
    parent = parent_;
    logger = logger_;
    _mainWindow = mainWindow;
    setState(BlowerOff);
    setNeedState(BlowerOff);
    settings = settings_;
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

    auto reader = _mainWindow->getReader();
    rpmForSweepType.insert(MainWindow::LeafSweep, reader->readSettingsValue("Engine/rpm.LeafSweep").toInt());
    rpmForSweepType.insert(MainWindow::LightSweep, reader->readSettingsValue("Engine/rpm.LightSweep").toInt());
    rpmForSweepType.insert(MainWindow::MediumSweep, reader->readSettingsValue("Engine/rpm.MediumSweep").toInt());
    rpmForSweepType.insert(MainWindow::HeavySweep, reader->readSettingsValue("Engine/rpm.HeavySweep").toInt());

    // назначаем таймауты на длительные операции
    timeouts.insert(BlowerSlideOut, reader->readSettingsValue("Blower/timeouts.BlowerSlideOut").toInt());
    timeouts.insert(BlowerSlideIn, reader->readSettingsValue("Blower/timeouts.BlowerSlideIn").toInt());
    timeouts.insert(BlowerDownOut, reader->readSettingsValue("Blower/timeouts.BlowerDownOut").toInt());
    timeouts.insert(BlowerDownIn, reader->readSettingsValue("Blower/timeouts.BlowerDownIn").toInt());
    timeouts.insert(BlowerRotateOut, reader->readSettingsValue("Blower/timeouts.BlowerRotateOut").toInt());
    timeouts.insert(BlowerRotateIn, reader->readSettingsValue("Blower/timeouts.BlowerRotateIn").toInt());

    speedForSweepType.insert(MainWindow::LeafSweep, reader->readSettingsValue("Blower/speeds.LeafSweep").toInt());
    speedForSweepType.insert(MainWindow::LightSweep, reader->readSettingsValue("Blower/speeds.LightSweep").toInt());
    speedForSweepType.insert(MainWindow::MediumSweep, reader->readSettingsValue("Blower/speeds.MediumSweep").toInt());
    speedForSweepType.insert(MainWindow::HeavySweep, reader->readSettingsValue("Blower/speeds.HeavySweep").toInt());

    qDebug() << timeouts;
}

QString Blower::toString(BlowerStates s){
    const char *key = QMetaEnum::fromType<BlowerStates>().valueToKey(s);
    return key ? QString::fromLatin1(key) : QStringLiteral("UnknownState");
}

void Blower::setDirection(bool isRight){
    isTargetRight = isRight;
}

void Blower::setState(BlowerStates state_){
    state = state_;

    if (state == Blower::BlowerOff){// выключили
        goOff();
        //myCan->setState(StateFRMBackL2, false);
    }
    if (state == Blower::BlowerDownOut){// началось опускание
        startActionTime = QDateTime::currentDateTime();
        goOff();
        goDown();
        //myCan->setState(StateFRMBackL2, true);
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
    myCan->setState(StateValveE7, false);
    myCan->setState(StateValveE3, false);
    myCan->setState(StateValveE1, false);
    myCan->setState(StateValveE5, false);
    myCan->setState(StateValveA1, false);
}

void Blower::goRotate(quint8 speed){
    //qDebug()<<"# RotationSpeed: "<<speed;
    myCan->setState(StateValveD3, speed);
}

void Blower::goSlide(bool turn_right){
    if (turn_right){
        myCan->setState(StateValveA1, true);
        myCan->setState(StateValveE7, true);
    }
    else{
        myCan->setState(StateValveA1, true);
        myCan->setState(StateValveE3, true);
    }
}

void Blower::goUp(){
    myCan->setState(StateValveA1, true);
    myCan->setState(StateValveE1, true);
}

void Blower::goDown()
{
    myCan->setState(StateValveA1, true);
    myCan->setState(StateValveE5, true);
}

void Blower::goNone(){
    myCan->setState(StateValveA1, false);
    myCan->setState(StateValveE5, false);
    myCan->setState(StateValveE1, false);
    myCan->setState(StateValveE3, false);
    myCan->setState(StateValveE7, false);
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
        const bool sensorReached = myCan->getState(StateDKPBlowerUp1).toBool() && myCan->getState(StateDKPBlowerUp2).toBool();
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
    startClean = _mainWindow->startClean;
    rightBlow = _mainWindow->workMode.blowRight;
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
        auto type = _mainWindow->workMode.sweepType;
        _mainWindow->canForEngine->setEngineCommand(rpmForSweepType.value(type) * 8);//обороты движка
        setTargetRotationSpeed(speedForSweepType.value(type));// скорость щеток
        //goRotate();
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

    if(_mainWindow->startClean && isTargetRight != rightBlow){
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
void Blower::updateWhenRotationPressed(bool isRight){
    if (!_mainWindow->startClean)
        return;// уборка не запущена - кнопка только выбирает сторону, гидравлику не трогаем

    const int elapsed = qAbs(rotationStartedAt.secsTo(QDateTime::currentDateTime().time()));
    if(elapsed > stopDelay){
        //setState (BlowerOff);
        //qDebug()<<"# Set target direction 2: "<<(isTargetRight?"right":"left");
        isTargetRight = isRight;
        if (_mainWindow->startClean && _mainWindow->workMode.blowLifted)// удержание стороны снова разворачивает поднятый обдув
            _mainWindow->setBlowerLifted(false);
        //_mainWindow->changeBlowDirection(isRight);
        setNeedState(BlowerRotated);
    }
    else{
        //qDebug()<<"# wait: "<<elapsed;
    }

    if(!isRotating()){
        goSlide(isRight);
        // if(isRight){
        //     goRight();
        // }
        // else{
        //     goLeft();
        // }
        //qDebug()<<"# blower slide";
        //goUp();
    }
    else{
        //qDebug()<<"# side: "<<isTargetRight<<"/"<<rightBlow;
    }
}

void Blower::updateWhenUpPressed(){
    const int elapsed = qAbs(stoppingStartedAt.secsTo(QDateTime::currentDateTime().time()));
    if(elapsed > stopDelay){
        // поднимаем обдув, выбранная сторона остаётся подсвеченной
        if (_mainWindow->startClean && !_mainWindow->workMode.blowLifted){
            _mainWindow->setBlowerLifted(true);
        }
        setNeedState(BlowerOff);
    }
    else{
        qDebug()<<"# wait: "<<elapsed;
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
    const int elapsed = qAbs(startingStartedAt.secsTo(QDateTime::currentDateTime().time()));
    if(elapsed > stopDelay){
        // запуск удержанием: подсвечиваем сторону обдува, по умолчанию правую
        if (_mainWindow->startClean && needState != BlowerRotated){
            _mainWindow->workMode.blowLifted = false;
            const bool right = !_mainWindow->workMode.blowLeft;
            isTargetRight = right;
            rightBlow = right;
            _mainWindow->changeBlowDirection(right);
        }
        setNeedState(BlowerRotated);//setState(BlowerOff);
    }
    else{
        qDebug()<<"# wait: "<<elapsed;
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
        logger->addLog("Обдув: заканчиваем опускание по таймеру");
        // заканчиваем опускание по таймеру
        if (testStateTimer())
            setState(BlowerDowned);
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
            _mainWindow->changeBlowDirection(isTargetRight);
        }
        setState(BlowerSlideOut);
        break;
    case BlowerSlideOut:
        logger->addLog("Обдув: заканчиваем поворот щётки");
        // заканчиваем поворот щетки
        if (testStateTimer())
            setState(BlowerSlided);
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
