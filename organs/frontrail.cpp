#include "frontrail.h"

#include "mainwindow.h"

#include <QDebug>
#include <QTimer>
#include <QThread>

FrontRail::FrontRail(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ScreenLog *logger_, QObject *parent_) : QObject(parent_){
    myCan = myCan_;
    myCanJ1939 = myCanJ1939_;
    parent = parent_;
    logger= logger_;
    setState(FrontRailOff);
    setNeedState(FrontRailOff);
    needSlided = false;
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
    auto mainWin = (MainWindow*)parent;
    // назначаем таймауты на длительные операции
    timeouts.insert(FrontRailSlideOut, mainWin ->readSettingsValue("Dump/timeouts.DumpSlideOut").toFloat());
    timeouts.insert(FrontRailSlideIn, mainWin ->readSettingsValue("Dump/timeouts.DumpSlideIn").toFloat());
    timeouts.insert(FrontRailBounceOut, mainWin->readSettingsValue("Dump/timeouts.DumpBounceOut").toFloat());
    timeouts.insert(FrontRailDownOut, mainWin->readSettingsValue("Dump/timeouts.DumpDownOut").toFloat());
    timeouts.insert(FrontRailDownIn, mainWin->readSettingsValue("Dump/timeouts.DumpDownIn").toFloat());
    timeouts.insert(FrontRailFlowOut, mainWin->readSettingsValue("Dump/timeouts.DumpFlowOut").toFloat());
}

QString FrontRail::toString(FrontRailStates s){
    switch (s) {
    case FrontRailOff:
        return "FrontRailOff";
        break;
    case FrontRailDownOut:
        return "FrontRailDownOut";
        break;
    case FrontRailDownIn:
        return "FrontRailDownIn";
        break;
    case FrontRailDowned:
        return "FrontRailDowned";
        break;
    case FrontRailFlowOut:
        return "FrontRailFlowOut";
        break;
    case FrontRailFlowIn:
        return "FrontRailFlowIn";
        break;
    case FrontRailFlowed:
        return "FrontRailFlowed";
        break;
    case FrontRailSlideOut:
        return "FrontRailSlideOut";
        break;
    case FrontRailSlideIn:
        return "FrontRailSlideIn";
        break;
    case FrontRailSlided:
        return "FrontRailSlided";
        break;
    case FrontRailBounceOut:
        return "FrontRailBounceOut";
        break;
    case FrontRailBounced:
        return "FrontRailBounced";
        break;
    default:
        return "UnknownState";
    }
}

void FrontRail::setState(FrontRailStates state_){
    qDebug()<<" статус отвала: "<<state_;
    state = state_;
    if (state == FrontRail::FrontRailOff){// перешла в домашнее щетка
        // отменить опускание
        goNone();
    }
    if (state == FrontRail::FrontRailDownOut){// началось опускание ( из верхнего в нижние, мимо домашнего)
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailDownOut, 0) > 0) {
            goNone();
            goDown();
            logger->printLog("Отвал опускается");
            //mainWindow->addLog("Отвал опускается", MainWindow::InfoStatus);
        }
    }
    if (state == FrontRail::FrontRailDowned){// опустилась на нужный уровень
        goNone();
    }
    if (state == FrontRail::FrontRailDownIn){// поднимаем из нижнего в самое верхнее
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailDownIn, 0) > 0) {
            goNone();
            goUp();
            logger->printLog("Отвал поднимается");
            //mainWindow->addLog(, MainWindow::InfoStatus);
        }
    }
    if (state == FrontRail::FrontRailFlowOut){// началось плавание
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailFlowOut, 0) > 0){
            goNone();
            goFlow();
        }
        logger->printLog("Плавающий режим отвала");
        //mainWindow->addLog("Плавающий режим отвала", MainWindow::InfoStatus);
    }
    if (state == FrontRail::FrontRailFlowed){// закончилось плавание
        if (((MainWindow*)parent)->workMode.frontDumpFlow){
            goFlow();
            logger->printLog("Плавающий режим отвала");
        }
        else{
            goNoFlow();
            logger->printLog("Не плавающий режим отвала");
        }
    }
    if (state == FrontRail::FrontRailFlowIn){// заканчиваем плавание
        goNoFlow();
        logger->printLog("Отвал не плавающий");
    }

    if (state == FrontRail::FrontRailSlideOut){// начинается поворот на нужный угол
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailSlideOut, 0) > 0) {
            goNone();
            if (!needSlided)
                goRight();
            else
                goLeft();
            logger->printLog("Отвал поворачивает");
        }
    }
    if (state == FrontRail::FrontRailSlideIn)
    {
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailSlideIn, 0) > 0) {
            goNone();
            goRight();
            logger->printLog("Отвал поворачивает");
        }
    }
    if (state == FrontRail::FrontRailSlided)
    {// повернулась куда надо
        goNone();
    }
    if (state == FrontRail::FrontRailBounceOut)
    {// отскок — поворот в противоположную сторону
        startActionTime = QDateTime::currentDateTime();
        if (timeouts.value(FrontRailBounceOut, 0) > 0) {
            goNone();
            if (!needSlided)
                goLeft();
            else
                goRight();
            logger->printLog("Отвал отскок");
        }
    }
    if (state == FrontRail::FrontRailBounced)
    {// отскок завершён
        goNone();
    }
}

void FrontRail::goRight(){
    myCan->setState(StateValveF6, true);
    myCan->setState(StateValveA1, true);
}

void FrontRail::goLeft(){
    myCan->setState(StateValveF12, true);
    myCan->setState(StateValveA1, true);
}

void FrontRail::goNone(){
    myCan->setState(StateValveF6, false);
    myCan->setState(StateValveF12, false);
    myCan->setState(StateValveF1, false);
    myCan->setState(StateValveF7, false);
    myCan->setState(StateValveA1, false);
}

void FrontRail::goDown(){
    myCan->setState(StateValveF7, true);
    myCan->setState(StateValveA1, true);
}

void FrontRail::goUp(){
    myCan->setState(StateValveF1, true);
    myCan->setState(StateValveA1, true);
}

void FrontRail::goFlow(){
    qDebug()<<"!!! dump go Flow";
    myCan->setState(StateValveC3, true);
    myCan->setState(StateValveC4, true);
}

void FrontRail::goNoFlow(){
    qDebug()<<"!!! dump stop Flow";
    myCan->setState(StateValveC3, false);
    myCan->setState(StateValveC4, false);
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
        const bool sensorReached = myCan->getState(StateDKPDumpUp).toBool();
        if (timeTest && !sensorReached)
        {
            if (!railAlarmed){
                logger->printLog("Отвал: достигнут тайм-аут");
                goNone();
            }
            railAlarmed = true;
        }
        else if (sensorReached){
            logger->printLog("Отвал: достигнут датчик");
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
        const bool sensorReached = myCan->getState((needSlided ? StateDKPDumpLeft : StateDKPDumpRight)).toBool();
        if (timeTest && !sensorReached){
            if (!railAlarmed){
                logger->printLog("Отвал: достигнут тайм-аут");
                goNone();
            }
            railAlarmed = true;
        }
        else if (sensorReached){

            logger->printLog("Отвал: достигнут датчик");
        }
        if (timeTest || sensorReached){
            dkpAndPositionTest = true;// не ждем таймера и разрешаем завершить процесс
        }
    }
    if (state == FrontRail::FrontRailSlideIn){
        const bool sensorReached = myCan->getState(StateDKPDumpRight).toBool();
        if (timeTest && !sensorReached){
            if (!railAlarmed){
                logger->printLog("Отвал: достигнут тайм-аут");
                goNone();
            }
            railAlarmed = true;
        }
        else if (sensorReached){
            logger->printLog("Отвал: достигнут датчик");
        }
        if (timeTest || sensorReached)
            dkpAndPositionTest = true;// не ждем таймера и разрешаем завершить процесс
    }

    if (dkpAndPositionTest){
        railAlarmed = false;
        return true;// достигнут концевик или нужное положение (мы молодцы)
    }


//    bool dkpAndPositionTest = false;
//    int tmp_rotate = myCan->getState(StateFrontRailRotateD19).toInt();
//    int tmp_need_rotate = levelRotateSlided;
//    if (!needSlided)
//        tmp_need_rotate = levelRotateNotSlided;
//    // проверяем концевики
//    if (state == FrontRail::FrontRailUpOut)
//    {// рейка в выше домашнего состояния
//        if (timeTest && myCan->getState(StateFrontRailLevelD16).toInt() > levelUp)
//        {
//            if (!railAlarmed)
//                ((MainWindow*)parent)->addLog("Все плохо. Передняя рейка не вышла выше домашнего состояния", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (myCan->getState(StateFrontRailLevelD16).toInt() <= levelUp)
//            dkpAndPositionTest = true;
//    }

//    if (state == FrontRail::FrontRailUpIn)
//    {// рейка в домашнее состояние
//        if (timeTest && myCan->getState(StateFrontRailLevelD16).toInt() < levelHome)
//        {
//            if (!railAlarmed)
//                ((MainWindow*)parent)->addLog("Все плохо. Передняя рейка не вернулась из верхнего состояния", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (myCan->getState(StateFrontRailLevelD16).toInt() >= levelHome)
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailDownOut)
//    {// рейка опускается
//        if (timeTest && abs(myCan->getState(StateFrontRailLevelD16).toInt() < needLevelDown) > 2)
//        {
//            if (!railAlarmed)
//                ((MainWindow*)parent)->addLog("Все плохо. Передняя рейка не пришла в рабочее состояние", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (myCan->getState(StateFrontRailLevelD16).toInt() >= needLevelDown)
//            dkpAndPositionTest = true;
//    }

//    if (state == FrontRail::FrontRailDownIn)
//    {// рейка поднимается
//        if (timeTest && myCan->getState(StateFrontRailLevelD16).toInt() > levelUp)
//        {
//            if (!railAlarmed)
//                ((MainWindow*)parent)->addLog("Все плохо. Передняя рейка не поднимается в верхнее состояние", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (myCan->getState(StateFrontRailLevelD16).toInt() <= levelUp)
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailSlideOut)
//    {// рейка поворачивается
//        if (timeTest && abs(tmp_need_rotate - tmp_rotate) > 1)
//        {
//            if (!railAlarmed)
//                ((MainWindow*)parent)->addLog("Все плохо. " + QString(((MainWindow*)parent)->workMode == MainWindow::SummerMode?"Передняя рейка не повернулась":"Отвал не повернулся"), MainWindow::FatalStatus);
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
//                ((MainWindow*)parent)->addLog("Все плохо. Передняя рейка не вернулась в домашнее состояние", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (abs(levelRotateHome - tmp_rotate) <= 1)
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailExpandOut)
//    {// рейка разворачивается
//        if (timeTest && (myCan->getState(StateDKPFrontLeftRailD17).toBool() || myCan->getState(StateDKPFrontRightRailD18).toBool()))
//        {
//            if (!railAlarmed && myCan->getState(StateDKPFrontLeftRailD17).toBool())
//                ((MainWindow*)parent)->addLog("Все плохо. Передняя левая рейка не развернулась", MainWindow::FatalStatus);
//            if (!railAlarmed && myCan->getState(StateDKPFrontRightRailD18).toBool())
//                ((MainWindow*)parent)->addLog("Все плохо. Передняя правая рейка не развернулась", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (!myCan->getState(StateDKPFrontLeftRailD17).toBool() && !myCan->getState(StateDKPFrontRightRailD18).toBool())
//            dkpAndPositionTest = true;
//    }
//    if (state == FrontRail::FrontRailExpandIn)
//    {// рейка заворачивается
//        if (timeTest && (!myCan->getState(StateDKPFrontLeftRailD17).toBool() || !myCan->getState(StateDKPFrontRightRailD18).toBool()))
//        {
//            if (!railAlarmed && myCan->getState(StateDKPFrontLeftRailD17).toBool())
//                ((MainWindow*)parent)->addLog("Все плохо. Передняя левая рейка не свернулась", MainWindow::FatalStatus);
//            if (!railAlarmed && myCan->getState(StateDKPFrontRightRailD18).toBool())
//                ((MainWindow*)parent)->addLog("Все плохо. Передняя правая рейка не свернулась", MainWindow::FatalStatus);
//            railAlarmed = true;
//        }
//        if (myCan->getState(StateDKPFrontLeftRailD17).toBool() && myCan->getState(StateDKPFrontRightRailD18).toBool())
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
    startClean = ((MainWindow*)parent)->startClean;
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
