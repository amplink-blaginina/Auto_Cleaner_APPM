#include "frontrail.h"

#include "mainwindow.h"

#include <QDebug>
#include <QTimer>
#include <QThread>

FrontRail::FrontRail(const MachineIo &machine, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ViewController *logger_, MainWindow* mainWindow, QObject *parent_) : QObject(parent_){
    io = machine.io;
    hydraulics = machine.hydraulics;
    engineRpm = machine.engineRpm;
    myCanJ1939 = myCanJ1939_;
    parent = parent_;
    _mainWindow = mainWindow;
    logger= logger_;
    setState(FrontRailOff);
    setNeedState(FrontRailOff);
    needGoLeft = false;
    settings = settings_;
    startClean = false;
    choosed = false;
    railAlarmed = false;

    readSettings();

    connect(&progressTimer, SIGNAL(timeout()), this, SLOT(progressLoop()));
    progressTimer.start(100);
}

void FrontRail::readSettings(){
    timeouts.clear();

    auto reader = _mainWindow->getReader();
    qDebug()<<"Central readSettings Dump";
    // назначаем таймауты на длительные операции
    timeouts.insert(FrontRailSlideOut, reader->readSettingsValue("Dump/timeouts.DumpSlideOut").toFloat());
    timeouts.insert(FrontRailSlideIn, reader->readSettingsValue("Dump/timeouts.DumpSlideIn").toFloat());
    timeouts.insert(FrontRailBounceOut, reader->readSettingsValue("Dump/timeouts.DumpBounceOut").toFloat());
    timeouts.insert(FrontRailDownOut, reader->readSettingsValue("Dump/timeouts.DumpDownOut").toFloat());
    timeouts.insert(FrontRailDownIn, reader->readSettingsValue("Dump/timeouts.DumpDownIn").toFloat());
    timeouts.insert(FrontRailFlowOut, reader->readSettingsValue("Dump/timeouts.DumpFlowOut").toFloat());
}

QString FrontRail::toString(FrontRailStates s){
    const char *key = QMetaEnum::fromType<FrontRailStates>().valueToKey(s);
    return key ? QString::fromLatin1(key) : QStringLiteral("UnknownState");
}


// void FrontRail::setDirection(organsEnums::Direction dir){
//     setDirection(dir, isPressed);
// }
void FrontRail::setDirection(organsEnums::Direction dir){
    if(dir == direction)
        return;

    switch (direction) {
    case organsEnums::Up:
        goUp(false);
        break;
    case organsEnums::Down:
        goDown(false);
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
    //setPressActive(pressed);

    switch (dir) {
    case organsEnums::Up:
        goUp(true);
        break;
    case organsEnums::Down:
        goDown(true);
        break;
    case organsEnums::Left:
        goLeft(true);
        break;
    case organsEnums::Right:
        goRight(true);
        break;
    default:
        break;
        //printMovement(dir, true, pressed);
    }
}

void FrontRail::setState(FrontRailStates state_){
    qDebug()<<" статус отвала: "<<state_;
    state = state_;
    if (state == FrontRail::FrontRailOff){// перешла в домашнее щетка
        // отменить опускание
        setDirection(organsEnums::None);
        //goNone();
    }
    if (state == FrontRail::FrontRailDownOut){// началось опускание ( из верхнего в нижние, мимо домашнего)
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailDownOut, 0) > 0) {
            setDirection(organsEnums::Down);
            //logger->addLog("Отвал опускается");
            //mainWindow->addLog("Отвал опускается", MainWindow::InfoStatus);
        }
    }
    if (state == FrontRail::FrontRailDowned){// опустилась на нужный уровень
        setDirection(organsEnums::None);
        //goNone();
    }
    if (state == FrontRail::FrontRailDownIn){// поднимаем из нижнего в самое верхнее
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailDownIn, 0) > 0) {
            setDirection(organsEnums::Up);
            // goNone();
            // goUp();
            //logger->printLog("Отвал поднимается");
            //mainWindow->addLog(, MainWindow::InfoStatus);
        }
    }
    if (state == FrontRail::FrontRailFlowOut){// началось плавание
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailFlowOut, 0) > 0){
            setFlowActive(true);
        }
        //logger->printLog("Плавающий режим отвала №1");
        //mainWindow->addLog("Плавающий режим отвала", MainWindow::InfoStatus);
    }
    if (state == FrontRail::FrontRailFlowed){// закончилось плавание
        qDebug()<<"Отвал: Плавание закончено";
        setFlowActive(_mainWindow->workMode.frontDumpFlow);
    }
    if (state == FrontRail::FrontRailFlowIn){// заканчиваем плавание
        setFlowActive(false);
    }

    if (state == FrontRail::FrontRailSlideOut){// начинается поворот на нужный угол
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailSlideOut, 0) > 0) {
            setDirection(needGoLeft? organsEnums::Left: organsEnums::Right);
            //logger->printLog("Отвал поворачивает");
        }
    }
    if (state == FrontRail::FrontRailSlideIn){
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailSlideIn, 0) > 0) {
            setDirection(organsEnums::Right);
            // goNone();
            // goRight();
            //logger->printLog("Отвал поворачивает");
        }
    }
    if (state == FrontRail::FrontRailSlided){// повернулась куда надо
        setDirection(organsEnums::None);
        //goNone();
    }
    if (state == FrontRail::FrontRailBounceOut){// отскок — поворот в противоположную сторону
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailBounceOut, 0) > 0) {
             setDirection(!needGoLeft? organsEnums::Left: organsEnums::Right);
            // goNone();
            // if (!needSlided)
            //     goLeft();
            // else
            //     goRight();
            logger->addLog("Отвал: отскок");
        }
    }
    if (state == FrontRail::FrontRailBounced){// отскок завершён
        setDirection(organsEnums::None);
        //goNone();
    }
}

void FrontRail::goRight(){goRight(true);}

void FrontRail::goRight(bool state){
    io->set(StateValveF6, state);
    hydraulics->request(this, state);
    printMovement(organsEnums::Right, state);
}
void FrontRail::goLeft(){goLeft(true);}
void FrontRail::goLeft(bool state){
    io->set(StateValveF12, state);
    hydraulics->request(this, state);
    printMovement(organsEnums::Left, state);
}

void FrontRail::goNone(){
    io->set(StateValveF6, false);
    io->set(StateValveF12, false);
    io->set(StateValveF1, false);
    io->set(StateValveF7, false);
    hydraulics->request(this, false);
}
void FrontRail::goDown(){goDown(true);}
void FrontRail::goDown(bool state){
    // if(state){
    //     setFlowActive(false);
    // }
    // if(state && !_mainWindow->isDumpTransitioning()){
    //     qDebug()<<"###goDOWN!!!";
    //     _mainWindow->setDumpFlowView(false);
    //     _mainWindow->workMode.frontDumpFlow = false;
    // }
    if(state){
        _mainWindow->tryToDisableDumpFlow();
    }
    io->set(StateValveF7, state);
    hydraulics->request(this, state);
    printMovement(organsEnums::Down, state);
}

void FrontRail::goUp(){
    goUp(true);
}

void FrontRail::goUp(bool state){
    // if(state){
    //     setFlowActive(false);
    //     _mainWindow->setDumpFlowView(state);
    // }

    // if(state && !_mainWindow->isDumpTransitioning()){
    //     qDebug()<<"###goUP!!!";
    //     _mainWindow->setDumpFlowView(false);
    //     _mainWindow->workMode.frontDumpFlow = false;
    // }
    if(state){
        _mainWindow->tryToDisableDumpFlow();
    }
    io->set(StateValveF1, state);
    hydraulics->request(this, state);
    //printMovement(organsEnums::Up, state);
}

void FrontRail::goFlow(bool state){
    logger->addLog(state?"Отвал: плавание активировано":"Отвал: плавание деактивировано");
    io->set(StateValveC3, state);
    io->set(StateValveC4, state);
}

void FrontRail::setFlowActive(bool state){
    if(isFlowing == state)
        return;
    isFlowing = state;
    goFlow(state);
   // _mainWindow->setDumpFlowView(state);
}


FrontRail::FrontRailStates FrontRail::getState(){
    return state;
}

void FrontRail::setNeedState(FrontRailStates state_){
    needState = state_;
//    qDebug() << "Central broom needState " << toString(needState);
}

FrontRail::FrontRailStates FrontRail::getNeedState(){
    return needState;
}

FrontRail::FrontRailStates FrontRail::getAbleState(){
    return ableState;
}

void FrontRail::checkNeedState()
{// утанавливает максимальную границу до которой может дойти щетка (при текущих параметрах)
    // проверяет соседние модули и собирает информацию о их состояниях (нажатые кнопки, обороты, статусы и пр.)
    checkFriendVars();
    if (needState != FrontRailOff){
        if (!startClean)
        {// пуск отжат или никакой режим смета не выбран или если щетки не выдвинуты
            ableState = FrontRailOff;// можно только продолжать пытаться включиться (используется такой странный статус потому что надо показать постоянно желание включиться даже если не нажали пуск например)
        }
        else
        {
            ableState = FrontRailFlowed;
        }
    }
    else
        ableState = FrontRailOff;
}

float FrontRail::getTimeout(){//получает таймаут в секундах (сколько надо простаивать в той или иной операции)
    return timeouts.value(state, 0);
}

void FrontRail::printMovement(organsEnums::Direction dir, bool state){
    logger->printMovementLog(organsEnums::Dump,
                             dir,
                             state?"": " завершено");
}


bool FrontRail::testStateTimer(){// мощная функция проверки таймаута одновременно с концевиками и прочими условиями (для каждого состояния)
    qint64 msecs_to = startActionTime.msecsTo(QDateTime::currentDateTime());
//    qint64 tmp_msecs = msecs_to;
//    if (msecs_to > getTimeout() * 1000)
//        tmp_msecs = getTimeout() * 1000;
    bool timeTest = false;
    if (msecs_to > getTimeout() * 1000){// тест по времени прошел а мы ничего не достигли. Нужны тревоги
        timeTest = true;
        //return true;
    }

    // проверяем концевики
    bool dkpAndPositionTest = false;
    // рейка идет вверх, ждем концевик ПЕРЕДНЯЯ
    if (state == FrontRail::FrontRailDownIn)
    {
        const bool sensorReached = io->get(StateDKPDumpUp).toBool();
        if (timeTest && !sensorReached)
        {
            if (!railAlarmed){
                logger->addLog("Отвал: достигнут тайм-аут");
                goNone();
            }
            railAlarmed = true;
        }
        else if (sensorReached){
            logger->addLog("Отвал: достигнут датчик");
        }
        if (timeTest || sensorReached)
            dkpAndPositionTest = true;

    }
    // вниз концевика нет. если таймер прошел то считаем что все ок
    if (state == FrontRail::FrontRailDownOut && timeTest)
        dkpAndPositionTest = true;
    if (state == FrontRail::FrontRailFlowOut && timeTest)
        dkpAndPositionTest = true;
    if (state == FrontRail::FrontRailFlowIn && timeTest)
        dkpAndPositionTest = true;
    if (state == FrontRail::FrontRailBounceOut && timeTest)
        dkpAndPositionTest = true;
    // щетка идет вбок, ждем концевик
    if (state == FrontRail::FrontRailSlideOut)
    {
        const bool sensorReached = io->get((needGoLeft ? StateDKPDumpLeft : StateDKPDumpRight)).toBool();
        if (timeTest && !sensorReached){
            if (!railAlarmed){
                logger->addLog("Отвал: достигнут тайм-аут");
                goNone();
            }
            railAlarmed = true;
        }
        else if (sensorReached){

            logger->addLog("Отвал: достигнут датчик");
        }
        if (timeTest || sensorReached){
            dkpAndPositionTest = true;// не ждем таймера и разрешаем завершить процесс
        }
    }
    if (state == FrontRail::FrontRailSlideIn){
        const bool sensorReached = io->get(StateDKPDumpRight).toBool();
        if (timeTest && !sensorReached){
            if (!railAlarmed){
                logger->addLog("Отвал: достигнут тайм-аут");
                goNone();
            }
            railAlarmed = true;
        }
        else if (sensorReached){
            logger->addLog("Отвал: достигнут датчик");
        }
        if (timeTest || sensorReached)
            dkpAndPositionTest = true;// не ждем таймера и разрешаем завершить процесс
    }

    if (dkpAndPositionTest){
        railAlarmed = false;
        return true;// достигнут концевик или нужное положение (мы молодцы)
    }


//    bool dkpAndPositionTest = false;
//    int tmp_rotate = io->get(StateFrontRailRotateD19).toInt();
//    int tmp_need_rotate = levelRotateSlided;
//    if (!needSlided)
//        tmp_need_rotate = levelRotateNotSlided;
//    // проверяем концевики
//    if (state == FrontRail::FrontRailUpOut)
//    {// рейка в выше домашнего состояния
//        if (timeTest && io->get(StateFrontRailLevelD16).toInt() > levelUp)
//        {
//            if (!railAlarmed)
//                _mainWindow->addLog("Все плохо. Передняя рейка не вышла выше домашнего состояния", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (io->get(StateFrontRailLevelD16).toInt() <= levelUp)
//            dkpAndPositionTest = true;
//    }

//    if (state == FrontRail::FrontRailUpIn)
//    {// рейка в домашнее состояние
//        if (timeTest && io->get(StateFrontRailLevelD16).toInt() < levelHome)
//        {
//            if (!railAlarmed)
//                _mainWindow->addLog("Все плохо. Передняя рейка не вернулась из верхнего состояния", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (io->get(StateFrontRailLevelD16).toInt() >= levelHome)
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailDownOut)
//    {// рейка опускается
//        if (timeTest && abs(io->get(StateFrontRailLevelD16).toInt() < needLevelDown) > 2)
//        {
//            if (!railAlarmed)
//                _mainWindow->addLog("Все плохо. Передняя рейка не пришла в рабочее состояние", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (io->get(StateFrontRailLevelD16).toInt() >= needLevelDown)
//            dkpAndPositionTest = true;
//    }

//    if (state == FrontRail::FrontRailDownIn)
//    {// рейка поднимается
//        if (timeTest && io->get(StateFrontRailLevelD16).toInt() > levelUp)
//        {
//            if (!railAlarmed)
//                _mainWindow->addLog("Все плохо. Передняя рейка не поднимается в верхнее состояние", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (io->get(StateFrontRailLevelD16).toInt() <= levelUp)
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailSlideOut)
//    {// рейка поворачивается
//        if (timeTest && abs(tmp_need_rotate - tmp_rotate) > 1)
//        {
//            if (!railAlarmed)
//                _mainWindow->addLog("Все плохо. " + QString(_mainWindow->workMode == MainWindow::SummerMode?"Передняя рейка не повернулась":"Отвал не повернулся"), MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (abs(tmp_need_rotate - tmp_rotate) <= 1)
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailSlideIn)
//    {// рейка заворачивается
//        if (timeTest && abs(levelRotateHome - tmp_rotate) > 1)
//        {
//            if (!railAlarmed)
//                _mainWindow->addLog("Все плохо. Передняя рейка не вернулась в домашнее состояние", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (abs(levelRotateHome - tmp_rotate) <= 1)
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailExpandOut)
//    {// рейка разворачивается
//        if (timeTest && (io->get(StateDKPFrontLeftRailD17).toBool() || io->get(StateDKPFrontRightRailD18).toBool()))
//        {
//            if (!railAlarmed && io->get(StateDKPFrontLeftRailD17).toBool())
//                _mainWindow->addLog("Все плохо. Передняя левая рейка не развернулась", MainWindow::FatalStatus);
//            if (!railAlarmed && io->get(StateDKPFrontRightRailD18).toBool())
//                _mainWindow->addLog("Все плохо. Передняя правая рейка не развернулась", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (!io->get(StateDKPFrontLeftRailD17).toBool() && !io->get(StateDKPFrontRightRailD18).toBool())
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailExpandIn)
//    {// рейка заворачивается
//        if (timeTest && (!io->get(StateDKPFrontLeftRailD17).toBool() || !io->get(StateDKPFrontRightRailD18).toBool()))
//        {
//            if (!railAlarmed && io->get(StateDKPFrontLeftRailD17).toBool())
//                _mainWindow->addLog("Все плохо. Передняя левая рейка не свернулась", MainWindow::FatalStatus);
//            if (!railAlarmed && io->get(StateDKPFrontRightRailD18).toBool())
//                _mainWindow->addLog("Все плохо. Передняя правая рейка не свернулась", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (io->get(StateDKPFrontLeftRailD17).toBool() && io->get(StateDKPFrontRightRailD18).toBool())
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailWaterOut)
//    {// рейка поливает
//        if (timeTest)
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailWaterIn)
//    {// рейка не поливает
//        if (timeTest)
//            dkpAndPositionTest = true;
//    }

//    if (dkpAndPositionTest)
//    {
//        railAlarmed = false;
//        return true;// достигнут концевик или нужное положение (мы молодцы)
//    }


    return false;
}

void FrontRail::checkFriendVars(){
    startClean = _mainWindow->startClean;
}

void FrontRail::progressLoop(){
    // проверяет до какого состояния может добираться щетка
    checkNeedState();
    if (state < needState && state < ableState){// нужно прогрессировать вверх (выдвигать, мыть и гусей не забыть)
        FrontRailStates s = state;
        stateUp();
        if (s != state)// && (state == needState || state == ableState))
        {
            qDebug() << "Front rail state " << toString(state);
        }
    }
    else if (state > needState || state > ableState){// прогрессируем вниз
        FrontRailStates s = state;
        stateDown();
        if (s != state)// && (state == needState || state == ableState))
        {
            qDebug() << "Front rail state " << toString(state);
        }
    }
}

FrontRail::FrontRailStates FrontRail::stateUp(){// пытаемся прогрессировать статусом вверх (если что меняем направление статуса, если вдруг был понижающий прогресс)
    switch (state) {
    case FrontRailOff:
        // начинаем поднимание с крючков
        setState(FrontRailSlideOut);
        break;
    case FrontRailSlideOut:
        // заканчиваем поворот щетки
        if (testStateTimer())
            setState(FrontRailSlided);
        break;
    case FrontRailSlideIn:
        setState(FrontRailSlideOut);
        break;
    case FrontRailSlided:
        setState(FrontRailBounceOut);
        break;
    case FrontRailBounceOut:
        if (testStateTimer())
            setState(FrontRailBounced);
        break;
    case FrontRailBounced:
        setState(FrontRailDownOut);
        break;
    case FrontRailDownOut:
        // заканчиваем опускание по таймеру
        if (testStateTimer())
            setState(FrontRailDowned);
        break;
    case FrontRailDownIn:
        // меняем направление
        setState(FrontRailDownOut);
        break;
    case FrontRailDowned:
        setState(FrontRailFlowOut);
        break;
    case FrontRailFlowIn:
        setState(FrontRailFlowOut);
        break;
    case FrontRailFlowOut:
        if (testStateTimer())
            setState(FrontRailFlowed);
        break;
    default:
        break;
    }
    return state;
}

FrontRail::FrontRailStates FrontRail::stateDown(){// пытаемся прогрессировать статусом вниз (если что меняем направление статуса, если вдруг был повышающий прогресс)
    switch (state) {
    case FrontRailDownOut:
        // меняем направление на поднимание (до этого опускались)
        setState(FrontRailDownIn);
        break;
    case FrontRailDownIn:
        // заканчиваем подъем по таймеру и переходим в верхнее состояние
        if (testStateTimer())
            setState(FrontRailBounced);
        break;
    case FrontRailDowned:
        // начинаем поднимаение по таймеру
        setState(FrontRailDownIn);
        break;
    case FrontRailFlowIn:
        setState(FrontRailDowned);
        break;
    case FrontRailFlowOut:
        setState(FrontRailFlowIn);
        break;
    case FrontRailFlowed:
        setState(FrontRailFlowIn);
        break;
    case FrontRailBounceOut:
        // прерываем отскок, считаем завершённым
        setState(FrontRailBounced);
        break;
    case FrontRailBounced:
        setState(FrontRailSlided);
        break;
    case FrontRailSlideOut:
        setState(FrontRailSlideIn);
        break;
    case FrontRailSlideIn:
        if (testStateTimer())
            setState(FrontRailOff);
        break;
    case FrontRailSlided:
        setState(FrontRailSlideIn);
        break;
    default:
        break;
    }
    return state;
}
