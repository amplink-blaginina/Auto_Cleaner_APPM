#include "blower.h"

#include "mainwindow.h"
#include "Configuration/configuration.h"
#include <QDebug>
#include <QTimer>
#include <QThread>

Blower::Blower(
    MyCan *myCan_,
    MyCanJ1939 *myCanJ1939_,
    QSettings *settings_,
    ViewController *view_,
    MainWindow* mainWindow,
    QObject *parent_
    )
    : OrganController(
          organsEnums::Blower,
          parent_
          )
{
    myCan = myCan_;
    myCanJ1939 = myCanJ1939_;
    parent = parent_;
    view = view_;
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

void Blower::setMovementDirection(
    organsEnums::Direction newDirection
    )
{
    if (activeDirection == newDirection) {
        return;
    }

    if (activeDirection != organsEnums::None) {
        publishMovementChanged(
            activeDirection,
            false
            );
    }

    activeDirection = newDirection;

    if (activeDirection != organsEnums::None) {
        publishMovementChanged(
            activeDirection,
            true
            );
    }
}

void Blower::setState(BlowerStates state_){
    state = state_;
    publishStateChanged(
        static_cast<int>(state)
        );

    if (state == Blower::BlowerOff){
        goOff();
    }
    if (state == Blower::BlowerDownOut){
        startActionTime = QDateTime::currentDateTime();
        goOff();
        goDown();
    }
    if (state == Blower::BlowerDowned){
        goOff();
    }
    if (state == Blower::BlowerDownIn){
        startActionTime = QDateTime::currentDateTime();
        goOff();
        goUp();
    }
    if (state == Blower::BlowerSlideOut){
        startActionTime = QDateTime::currentDateTime();

        goSlide(rightBlow);
    }
    if (state == Blower::BlowerSlideIn){
        startActionTime = QDateTime::currentDateTime();
    }
    if (state == Blower::BlowerSlided){
        goOff();
    }
    if (state == Blower::BlowerRotateOut){
        startActionTime = QDateTime::currentDateTime();
    }
    if (state == Blower::BlowerRotateIn){
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

    setMovementDirection(
        organsEnums::None
        );
}

void Blower::goRotate(quint8 speed){
    qDebug()<<"# RotationSpeed: "<<speed;
    myCan->setState(StateValveD3, speed);

    const bool active = speed > 0;

    if (rotationActive != active) {
        rotationActive = active;

        publishModeChanged(
            QStringLiteral("rotation"),
            rotationActive
            );
    }
}

void Blower::goSlide( bool turnRight )
{
    selectedSide = turnRight
                       ? organsEnums::Right
                       : organsEnums::Left;

    myCan->setState(StateValveA1, true);

    if (turnRight) {
        myCan->setState(StateValveE7, true);

        setMovementDirection(
            organsEnums::Right
            );
    } else {
        myCan->setState(StateValveE3, true);

        setMovementDirection(
            organsEnums::Left
            );
    }
}

void Blower::goUp(){
    myCan->setState(StateValveA1, true);
    myCan->setState(StateValveE1, true);

    setMovementDirection(
        organsEnums::Up
        );
}

void Blower::goDown(){
    myCan->setState(StateValveA1, true);
    myCan->setState(StateValveE5, true);

    setMovementDirection(
        organsEnums::Down
        );
}

void Blower::goNone(){
    myCan->setState(StateValveA1, false);
    myCan->setState(StateValveE5, false);
    myCan->setState(StateValveE1, false);
    myCan->setState(StateValveE3, false);
    myCan->setState(StateValveE7, false);

    setMovementDirection(
        organsEnums::None
        );
}

Blower::BlowerStates Blower::getState(){
    return state;
}

void Blower::setNeedState(BlowerStates state_){
    if (needState == state_) {
        return;
    }

    needState = state_;

    switch (needState) {
    case BlowerOff:
        view->addLog("Обдув: останавливаем работу, поднимаем раструб");
        break;

    case BlowerRotated:
        view->addLog("Обдув: запускаем рабочую последовательность");
        break;

    default:
        view->addLog(
            "Обдув: новое целевое состояние " + toString(needState)
            );
        break;
    }
}

Blower::BlowerStates Blower::getAbleState(){
    return ableState;
}

Blower::BlowerStates Blower::getNeedState(){
    return needState;
}

void Blower::checkNeedState(){
    ableState = (!startClean || needState == BlowerOff) ? BlowerOff : BlowerRotated;
}

int Blower::getTimeout(){
    return timeouts.value(state, 0);
}

bool Blower::hasAnyUpPositionSensor() const
{
    const Configuration* configuration =
        _mainWindow->getMachineConfiguration();

    return configuration->hasBlowerUpSensor1()
           || configuration->hasBlowerUpSensor2();
}

bool Blower::areUpPositionSensorsReached() const
{
    const Configuration* configuration =
        _mainWindow->getMachineConfiguration();

    const bool firstSensorOk =
        !configuration->hasBlowerUpSensor1()
        || myCan->getState(StateDKPBlowerUp1).toBool();

    const bool secondSensorOk =
        !configuration->hasBlowerUpSensor2()
        || myCan->getState(StateDKPBlowerUp2).toBool();

    return firstSensorOk && secondSensorOk;
}

bool Blower::testStateTimer(){
    qint64 msecs_to = startActionTime.msecsTo(QDateTime::currentDateTime());
    bool timeTest = false;
    if (msecs_to > getTimeout() * 1000){
        timeTest = true;
    }

    bool dkpAndPositionTest = false;
    if (state == Blower::BlowerDownIn) {
        const Configuration* configuration =
            _mainWindow->getMachineConfiguration();

        const bool up1Installed =
            configuration->hasBlowerUpSensor1();

        const bool up2Installed =
            configuration->hasBlowerUpSensor2();

        const bool hasAnyUpSensor =
            up1Installed || up2Installed;

        const bool upPositionReached =
            hasAnyUpSensor
            && (!up1Installed
                || myCan->getState(StateDKPBlowerUp1).toBool())
            && (!up2Installed
                || myCan->getState(StateDKPBlowerUp2).toBool());

        if (upPositionReached) {
            view->addLog("Продувка: достигнут верхний датчик");
            dkpAndPositionTest = true;
        }
        else if (timeTest) {
            if (!blowerAlarmed) {
                view->addLog(
                    hasAnyUpSensor
                        ? "Продувка: завершено по тайм-ауту, ДКП не сработал"
                        : "Продувка: завершено по тайм-ауту (ДКП отсутствуют)"
                    );

                goOff();
            }

            blowerAlarmed = true;
            dkpAndPositionTest = true;
        }
    }
    if ((state == Blower::BlowerDownOut
         || state == Blower::BlowerSlideOut
         || state == Blower::BlowerRotateOut
         || state == Blower::BlowerSlideIn
         || state == Blower::BlowerRotateIn) && timeTest)
        dkpAndPositionTest = true;

    if (dkpAndPositionTest){
        blowerAlarmed = false;
        return true;
    }

    return false;
}

void Blower::setTargetRotationSpeed(float speed){
    targetRotationSpeed = speed;
}

void Blower::changeRotationSpeed(){
    if(currentRotationSpeed < targetRotationSpeed){
        currentRotationSpeed += speedRotationStep;
        if (currentRotationSpeed > targetRotationSpeed){
            currentRotationSpeed = targetRotationSpeed;
        }
        goRotate(currentRotationSpeed);
        qDebug()<<"# blower speed up: "<<currentRotationSpeed;
        return;
    }

    if(currentRotationSpeed > targetRotationSpeed){
        currentRotationSpeed -= speedRotationStep;
        if(currentRotationSpeed < 0){
            currentRotationSpeed = 0;
        }
        goRotate(currentRotationSpeed);
        qDebug()<<"# blower speed down: "<<currentRotationSpeed;
        return;
    }
}

void Blower::progressLoop(){
    checkNeedState();

    if (state >= Blower::BlowerRotateOut) {
        _mainWindow->canForEngine->setEngineCommand(
            rpmForSweepType.value(sweepType * 8)
            );

        setTargetRotationSpeed(
            speedForSweepType.value(sweepType)
            );
    }

    updateTransitioning();

    changeRotationSpeed();
}

void Blower::updateTransitioning(){
    if (startClean && isTargetRight != rightBlow) {
        rotate();
        return;
    }

    if (state < needState && state < ableState){
        BlowerStates s = state;
        stateUp();
        if (s != state)
        {
            qDebug() << "Blower state " << toString(state);
        }
    }
    else if (state > needState || state > ableState){
        BlowerStates s = state;
        stateDown();
        if (s != state)
        {
            qDebug() << "Blower state " << toString(state);
        }
    }
}

bool Blower::isRotating(){
    return currentRotationSpeed > 0;
}

void Blower::setStartMomentForStopping(){
    qDebug()<<"# wait for stop!";
    stoppingStartedAt = QDateTime::currentDateTime().time();
}

void Blower::setStartMomentForStarting(){
    qDebug()<<"# wait for start!";
    startingStartedAt = QDateTime::currentDateTime().time();
}

void Blower::setStartMomentForRotation(){
    rotationStartedAt = QDateTime::currentDateTime().time();
}

void Blower::updateWhenRotationPressed(bool isRight){
    const int elapsed = qAbs(rotationStartedAt.secsTo(QDateTime::currentDateTime().time()));
    if(elapsed > stopDelay){
        qDebug()<<"# Set target direction 2: "<<(isTargetRight?"right":"left");
        isTargetRight = isRight;
        setNeedState(BlowerRotated);
    }
    else{
        qDebug()<<"# wait: "<<elapsed;
    }

    if(!isRotating()){
        goSlide(isRight);
        qDebug()<<"# blower slide";
    }
    else{
        qDebug()<<"# side: "<<isTargetRight<<"/"<<rightBlow;
    }
}

void Blower::updateWhenUpPressed(){
    const int elapsed = qAbs(stoppingStartedAt.secsTo(QDateTime::currentDateTime().time()));

    if(elapsed > stopDelay){
        setNeedState(BlowerOff);
    }
    else{
        qDebug()<<"# wait: "<<elapsed;
    }

    if(!isRotating()){
        goUp();
    }
}

void Blower::updateWhenDownPressed(){
    const int elapsed = qAbs(startingStartedAt.secsTo(QDateTime::currentDateTime().time()));
    if(elapsed > stopDelay){
        setNeedState(BlowerRotated);
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
    qDebug()<<"# rotate: "<<isTargetRight<<"/"<<rightBlow;

    switch (state) {
    case BlowerOff:
        view->addLog("Обдув: опускаем раструб");
        setState(BlowerDownOut);
        break;
    case BlowerDownOut:
        view->addLog("Обдув: заканчиваем опускание по таймеру");
        if (testStateTimer())
            setState(BlowerDowned);
        break;
    case BlowerDownIn:
        view->addLog("Обдув: меняем направление на опускание");
        setState(BlowerDownOut);
        break;
    case BlowerDowned:
        view->addLog("Обдув: выставлем направление обдува");

        if(isTargetRight != rightBlow){
            qDebug()<<"# Set target direction 1: "<<(isTargetRight?"right":"left");
            rightBlow = isTargetRight;
            _mainWindow->changeBlowDirection(isTargetRight);
        }
        setState(BlowerSlideOut);
        break;
    case BlowerSlideOut:
        view->addLog("Обдув: заканчиваем поворот щётки");
        if (testStateTimer())
            setState(BlowerSlided);
        break;
    case BlowerSlideIn:
        if(isTargetRight != rightBlow){
            if (testStateTimer())
                setState(BlowerDowned);
        }
        else{
            setState(BlowerSlideOut);
        }
        break;
    case BlowerSlided:
        if(isTargetRight != rightBlow){
            setState(BlowerSlideIn);
        }
        else{
            view->addLog("Обдув: раскручиваем вентилятор");
            setState(BlowerRotateOut);
        }
        break;
    case BlowerRotateOut:
        if(isTargetRight != rightBlow){
            setState(BlowerRotateIn);
        }
        else{
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
            setState(BlowerRotateOut);
        }
        break;
    case BlowerRotated:
        if(isTargetRight != rightBlow){
            view->addLog("Обдув: выключаем вентилятор");
            setState(BlowerRotateIn);
        }
        break;
    default:
        break;
    }
    return state;
}

Blower::BlowerStates Blower::stateUp(){
    switch (state) {
    case BlowerOff:
        view->addLog("Опускаем раструб");
        setState(BlowerDownOut);
        break;
    case BlowerDownOut:
        if (testStateTimer())
            setState(BlowerDowned);
        break;
    case BlowerDownIn:
        setState(BlowerDownOut);
        break;
    case BlowerDowned:
        view->addLog("Выставлем направление обдува");
        setState(BlowerSlideOut);
        break;
    case BlowerSlideOut:
        if (testStateTimer())
            setState(BlowerSlided);
        break;
    case BlowerSlideIn:
        setState(BlowerSlideOut);
        break;
    case BlowerSlided:
        view->addLog("Раскручиваем вентилятор");
        setState(BlowerRotateOut);
        break;
    case BlowerRotateOut:
        if (testStateTimer())
            setState(BlowerRotated);
        break;
    case BlowerRotateIn:
        setState(BlowerRotateOut);
        break;
    default:
        break;
    }
    return state;
}

Blower::BlowerStates Blower::stateDown()
{
    switch (state) {
    case BlowerDownOut:
        setState(BlowerDownIn);
        break;
    case BlowerDownIn:
        if (testStateTimer())
            setState(BlowerOff);
        break;
    case BlowerDowned:
        view->addLog("Поднимаем раструб");
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
        view->addLog("Выключаем вентилятор");
        setState(BlowerRotateIn);
        break;
    default:
        break;
    }
    return state;
}

bool Blower::isInstalled() const{
    return _mainWindow->getMachineConfiguration()
    ->hasBlower();
}

bool Blower::isSelected() const{
    return choosed;
}

void Blower::setSelected(bool selected){
    choosed = selected;
}

bool Blower::isTransitioning() const
{
    BlowerStates targetState = needState;

    if (needState != BlowerOff
        && ableState < needState) {
        targetState = ableState;
    }

    return state != targetState;
}

bool Blower::isInHomeState() const{
    return state == BlowerOff || blowerAlarmed;
}

bool Blower::isInWorkingState() const{
    return state == BlowerRotated;
}

void Blower::requestHomeState(){
    setNeedState(BlowerOff);
}

void Blower::forceSafeState(){
    setNeedState(BlowerOff);
    setState(BlowerOff);
}

void Blower::updateTargetFromWorkMode( const OrganWorkMode &mode){
    startClean = mode.startClean;
    rightBlow = mode.blowerRight;
    sweepType = mode.sweepType;

    const bool active =
        mode.blowerLeft
        || mode.blowerRight;

    choosed = active;

    setNeedState(
        active
            ? BlowerRotated
            : BlowerOff
        );
}

void Blower::stopAllOutputs(){
    setTargetRotationSpeed(0);
    currentRotationSpeed = 0;
    goRotate(0);
    goNone();
}

bool Blower::supportsDirection( organsEnums::Direction direction ) const{
    return direction == organsEnums::Up
           || direction == organsEnums::Down
           || direction == organsEnums::Left
           || direction == organsEnums::Right;
}

// void Blower::setManualDirection( organsEnums::Direction direction ){
//     switch (direction) {
//     case organsEnums::Up:
//         goUp();
//         break;

//     case organsEnums::Down:
//         goDown();
//         break;

//     case organsEnums::Left:
//         goSlide(false);
//         break;

//     case organsEnums::Right:
//         goSlide(true);
//         break;

//     case organsEnums::None:
//     default:
//         goNone();
//         break;
//     }
// }

void Blower::applyTargetDirection()
{
    setManualDirection(targetDirection);
}

void Blower::updateManualDirection( organsEnums::Direction direction){
    switch (direction) {
    case organsEnums::Up:
        updateWhenUpPressed();
        break;

    case organsEnums::Down:
        updateWhenDownPressed();
        break;

    case organsEnums::Left:
        updateWhenRotationPressed(false);
        break;

    case organsEnums::Right:
        updateWhenRotationPressed(true);
        break;

    case organsEnums::None:
    default:
        break;
    }
}

QList<OrganButtonDef> Blower::buttonDefinitions() const
{
    using DE = organsEnums;

    return {
            {DE::Up,    GPIOInput::IN_BLOW_UP,    "pushButton_blowerUp",    "label_blowerUpDown", blowerVertPath + "up_on.png);",   blowerVertPath + "off.png);"},
            {DE::Down,  GPIOInput::IN_BLOW_DOWN,  "pushButton_blowerDown",  "label_blowerUpDown", blowerVertPath + "down_on.png);", blowerVertPath + "off.png);"},
            {DE::Left,  GPIOInput::IN_BLOW_LEFT,  "pushButton_blowerLeft",  "label_blower",       blowerHorPath + "left_on.png);",  {}, true},
            {DE::Right, GPIOInput::IN_BLOW_RIGHT, "pushButton_blowerRight", "label_blower",       blowerHorPath + "right_on.png);", {}, true},
            };
}

void Blower::holdTick(organsEnums::Direction direction)
{
    switch (direction) {
    case organsEnums::Up:    updateWhenUpPressed();           break;
    case organsEnums::Down:  updateWhenDownPressed();         break;
    case organsEnums::Left:  updateWhenRotationPressed(false); break;
    case organsEnums::Right: updateWhenRotationPressed(true);  break;
    default: break;
    }
}

void Blower::setManualDirection(organsEnums::Direction direction)
{
    switch (direction) {
    case organsEnums::Up:
        if (isRotating()) {
            view->addLog("Удерживайте кнопку вверх для остановки обдува и подъёма");
            setStartMomentForStopping();
        } else {
            goUp();
        }
        break;

    case organsEnums::Down:
        if (!isRotating()) {
            setStartMomentForStarting();
        }
        break;

    case organsEnums::Left:
        if (isRotating()) {
            view->addLog("Удерживайте кнопку влево для смены направления обдува");
            setStartMomentForRotation();
        } else {
            goSlide(false);
        }
        break;

    case organsEnums::Right:
        if (isRotating()) {
            view->addLog("Удерживайте кнопку вправо для смены направления обдува");
            setStartMomentForRotation();
        } else {
            goSlide(true);
        }
        break;

    case organsEnums::None:
    default:
        goNone();
        break;
    }
}
