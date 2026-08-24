#include "blower.h"

#include "mainwindow.h"

#include <QDebug>
#include <QTimer>
#include <QThread>

Blower::Blower(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ViewController *logger_, QObject *parent_) : QObject(parent_)
{
    myCan = myCan_;
    myCanJ1939 = myCanJ1939_;
    parent = parent_;
    logger = logger_;
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
    auto mainWindow = ((MainWindow*)parent);
    auto reader = mainWindow->getReader();
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

QString Blower::toString(BlowerStates s)
{
    switch (s) {
    case BlowerOff:
        return "BlowerOff";
        break;
    case BlowerDownOut:
        return "BlowerDownOut";
        break;
    case BlowerDownIn:
        return "BlowerDownIn";
        break;
    case BlowerDowned:
        return "BlowerDowned";
        break;
    case BlowerSlideOut:
        return "BlowerSlideOut";
        break;
    case BlowerSlideIn:
        return "BlowerSlideIn";
        break;
    case BlowerSlided:
        return "BlowerSlided";
        break;
    case BlowerRotateOut:
        return "BlowerRotateOut";
        break;
    case BlowerRotateIn:
        return "BlowerRotateIn";
        break;
    case BlowerRotated:
        return "BlowerRotated";
        break;
    default:
        return "UnknownState";
    }
}

void Blower::setState(BlowerStates state_)
{
    state = state_;

    if (state == Blower::BlowerOff)
    {// выключили
        goOff();
        //myCan->setState(StateFRMBackL2, false);
    }
    if (state == Blower::BlowerDownOut)
    {// началось опускание
        startActionTime = QDateTime::currentDateTime();
        goOff();
        goDown();
        //myCan->setState(StateFRMBackL2, true);
    }
    if (state == Blower::BlowerDowned)
    {
        goOff();
    }
    if (state == Blower::BlowerDownIn)
    {
        // Запоминаем что поднимание начилось
        startActionTime = QDateTime::currentDateTime();
        goOff();
        goUp();
    }
    if (state == Blower::BlowerSlideOut)
    {// поворот
        startActionTime = QDateTime::currentDateTime();

        goSlide(rightBlow);
    }
    if (state == Blower::BlowerSlideIn)
    {// поворот
        startActionTime = QDateTime::currentDateTime();
    }
    if (state == Blower::BlowerSlided)
    {// поворот
        goOff();
    }

    if (state == Blower::BlowerRotateOut)
    {// раскручивание
        startActionTime = QDateTime::currentDateTime();
    }
    if (state == Blower::BlowerRotateIn)
    {// остановка
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
    qDebug()<<"RotationSpeed: "<<speed;
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
//    qDebug() << "Central broom needState " << toString(needState);
}

Blower::BlowerStates Blower::getAbleState(){
    return ableState;
}

Blower::BlowerStates Blower::getNeedState(){
    return needState;
}

void Blower::checkNeedState(){// утанавливает максимальную границу до которой может дойти щетка (при текущих параметрах)
    if (needState != BlowerOff)
    {
        if (!startClean){// пуск отжат или никакой режим смета не выбран или если щетки не выдвинуты
            ableState = BlowerOff;// можно только продолжать пытаться включиться (используется такой странный статус потому что надо показать постоянно желание включиться даже если не нажали пуск например)
        }
        else{
            ableState = BlowerRotated;
        }
    }
    else
        ableState = BlowerOff;
}

int Blower::getTimeout(){//получает таймаут в секундах (сколько надо простаивать в той или иной операции)
    return timeouts.value(state, 0);
}

bool Blower::testStateTimer(){// мощная функция проверки таймаута одновременно с концевиками и прочими условиями (для каждого состояния)
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
    startClean = ((MainWindow*)parent)->startClean;
    rightBlow = ((MainWindow*)parent)->workMode.blowRight;
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
    }
    else if(currentRotationSpeed>targetRotationSpeed){
        currentRotationSpeed-=speedRotationStep;
        if(currentRotationSpeed<0){
            currentRotationSpeed = 0;
        }
    }
    goRotate(currentRotationSpeed);
}

void Blower::progressLoop(){
    // проверяет соседние модули и собирает информацию о их состояниях (нажатые кнопки, обороты, статусы и пр.)
    checkFriendVars();
    // проверяет до какого состояния может добираться щетка
    checkNeedState();

    if (state >= Blower::BlowerRotateOut){
        //обороты движка
        ((MainWindow*)parent)->canForEngine->setEngineCommand(rpmForSweepType.value(((MainWindow*)parent)->workMode.sweepType) * 8);
        // скорость щеток
        setTargetRotationSpeed(speedForSweepType.value(((MainWindow*)parent)->workMode.sweepType));
        //goRotate();
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

    changeRotationSpeed();
}

bool Blower::isRotating(){
    return currentRotationSpeed > 0;
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
