#include "startercontroller.h"
#include <mainwindow.h>
#include "cancontroller.h"
#include "gpiocontroller.h"
#include "qdebug.h"

StarterController::StarterController(GlobalSettings *globals,
                                     CurrentState *state,
                                     Engine *engine,
                                     ScreenLog *screenLog,
                                     CanController *can0,
                                     GPIOController *gpio,
                                     MainWindow* mainWindow) {
    _globals = globals;
    _state = state;
    _engine = engine;
    _screenLog = screenLog;
    _can = can0;
    _gpio = gpio;
    _mainWindow = mainWindow;
    configureButtons();
}

void StarterController::setDefaults(){
    starterLockedByRoll = false;
    starterLockedByTemperature = false;
    starterLockedByEmergency = false;
    starterPauseActive = false;
    starterAttemptsUsed = 0;
    starterNeedReboot = false;
    starterPressedPrev = false;
    starterPauseWarned = false;
    starterStarted = false;
    starterStartedTime = QDateTime::currentDateTime();
    starterStartedAlarmed = false;
    starterBroomAlarmed = false;
    starterBunkerAlarmed = false;
    ignitionOffTimer = 0;
    engineWasRunning = false;
    intentionalShutdown = false;
}

void StarterController::resetValues(){
    starterNeedReboot = false;
    starterAttemptsUsed = 0;
    starterPauseActive = false;
}

void StarterController::configureButtons(){
    m_starter = BoolStateWatcher{
       {
           .onActivated = [this] {
                //qDebug()<<"*StarterPressed";
                onStarterPress();
           },
           .onDeactivated = [this] {
                //qDebug()<<"*StarterReleased";
                onStarterRelease();
           },
           .whileActive = [this] {
                onStarterHold();
         },
           .whileInactive = [this] {
                updateStarterState();
         }
       }};

}

void StarterController::setEngineForm(ServiceOtherEngineLeftForm *otherEngineForm){
    _statusLbl = otherEngineForm->findChild<QLabel*>("label_prerollStatus");
}

void StarterController::updateStarterStatusText(QString text, bool isError){
         _mainWindow->getView()->setStyle(_statusLbl, isError? "color: red;": "color: yellow;");

         if (_statusLbl->text() != text)
             _statusLbl->setText(text);
         if(isError){
             _screenLog->printError(text);}
         else{
             _screenLog->printWarning(text);
         }
}

// void StarterController::updateStarterStatusText(){
//     QString statusText;
//     if (_state->rollNeedReboot)
//         statusText = "Лимит попыток исчерпан. Требуется перезагрузка пульта.";
//     else if (_state->rollRunActive){
//         const int elapsed = qAbs(rollRunStartedAt.secsTo(QDateTime::currentDateTime()));
//         statusText = QString("Прокрутка активна... %1 сек. | Попытка %2/%3")
//                          .arg(elapsed).arg(_state->rollAttemptsUsed).arg(_globals ->getRollAttempts());
//     }
//     else if (inRollPause())
//         statusText = QString("Пауза между попытками: %1 сек.").arg(rollPauseSecondsLeft());
//     else if (_state->prerollSequenceActive)
//         statusText = "Подготовка прокрутки (выключение зажигания)...";
//     else if (_state->prerollStarterUnlocked)
//         statusText = "Готово — нажмите СТАРТЕР ПРОКРУТКА для прокрутки";
//     else if (rollBlocked()){
//         QStringList reasons;
//         if (_state->rollLockedByTemperature) reasons << "холодный двигатель (ждите теплореле)";
//         if (_state->rollLockedByEmergency) reasons << "аварийный режим";
//         statusText = "Прокрутка заблокирована: " + reasons.join(", ");
//         _view->setStyle(_statusLbl,"color: red;");
//     }
//     else if (_state->needRollProcedure)
//         statusText = "Требуется прокрутка. Нажмите ПРОКРУТКА для подготовки.";
//     else if (_state->rollCompleted)
//         statusText = "Прокрутка успешно завершена";
//     else
//         statusText = "";

//     _view->setStyle(_statusLbl, (rollBlocked() && !statusText.isEmpty())? "color: red;": "color: yellow;");

//     if (_statusLbl->text() != statusText)
//         _statusLbl->setText(statusText);
// }

void StarterController::updateButtons(bool screenPressed, bool physicalPressed){
    if (!buttonsSinceStart.isValid())
        buttonsSinceStart.start();// окно проверки отсчитываем от первого опроса кнопок

    // кнопка пульта нажата сразу после запуска: человек так быстро не нажмёт, значит замыкание или залипание.
    // Сигнал с неё больше не принимаем, иначе стартер включится сам
    if (physicalPressed && !physicalStarterFaulty && buttonsSinceStart.elapsed() < STARTER_STUCK_WINDOW_MS){
        physicalStarterFaulty = true;
        qWarning() << "STARTER: кнопка стартера на пульте нажата при запуске программы - неисправна, сигнал с неё игнорируется";
    }
    if (physicalStarterFaulty)
        physicalPressed = false;

    m_starter.update(screenPressed || physicalPressed);
}

void StarterController::warnIfPhysicalStarterFaulty(){
    if (!physicalStarterFaulty)
        return;
    const QString text = "Кнопка стартера на пульте неисправна (нажата при включении). Сигнал с неё игнорируется";
    _screenLog->printWarning(text);
    if (_statusLbl){
        _mainWindow->getView()->setStyle(_statusLbl, "color: yellow;");
        _statusLbl->setText(text);
    }
}

bool StarterController::inStarterPause() const{
    if (!starterPauseActive)
        return false;
    return _globals->checkStarterPause(starterPauseStartedAt);
}

int StarterController::starterPauseSecondsLeft() const{
    if (!starterPauseActive)
        return 0;
    return _globals->starterPauseSecondsLeft(starterPauseStartedAt);
}

bool StarterController::starterBlocked() const{
    return starterLockedByRoll
           || starterLockedByTemperature
           || starterLockedByEmergency
           || starterNeedReboot
           || _engine->waitOnStart;
}

void StarterController::checkPauseState(){
    if (!inStarterPause()){
        //qDebug()<<"* 3";
        starterPauseActive = false;
        starterPauseWarned = false;
    }
}

// bool StarterController::isStarterClicked(){
//     return starterPressed && !starterPressedPrev;
// }

void StarterController::setStarterPressed(bool state){
    qDebug()<<"starter: "<<state;
    m_starter.update(state);
    // if (starterPressed == state)
    //     return;
    // qDebug()<<"!!! set pressed "<< state;
    // starterPressed = state;
    // _gpio->setStarter(state);
}

bool StarterController::isStarterPressed(){
    return starterPressed;
}

QString StarterController::getFatalStatusMessage(){
    if (!starterBlocked()){
        return "Стартер не заблокирован";
    }
    if (starterLockedByRoll){
        return "Стартер заблокирован: прокрутка";
    }
    if (starterLockedByTemperature){
        return "Стартер заблокирован: требуется прогрев двигателя";
    }
    if (starterLockedByEmergency){
        if (_state->waterAlarm)
            return "Стартер заблокирован: по датчику воды";
        if (_state->airAlarm)
            return "Стартер заблокирован: по датчику воздуха";
        if (_state->oilAlarm)
            return "Стартер заблокирован: по датчику масла";
    }
    if (starterNeedReboot){
        return "Стартер заблокирован: требуется перезагрузка";
    }
    if (_engine->waitOnStart){
        return "Стартер заблокирован: ожидание на старте";
    }
    return "Стартер не заблокирован";
}

// Если лимит попыток запуска исчерпан — блокируем стартер и требуем перезагрузку пульта.
// Возвращает true, если лимит был достигнут именно сейчас.
bool StarterController::checkAttemptsLimitReached(){
    if (_globals->starterAttemptsLimitReached(starterAttemptsUsed)){
        starterNeedReboot = true;
        _screenLog->printError("Достигнут лимит попыток запуска, требуется перезагрузка пульта");
        return true;
    }
    return false;
}

// Стартер отработал слишком долго без запуска двигателя: останавливаем его,
// уходим в паузу между попытками и при необходимости требуем перезагрузку.
void StarterController::handleStarterTimeout(){
    //qDebug()<<"стартер работал слишком долго!";
    stopStarterOutput();
    starterStarted = false;
    starterPauseActive = true;
    //qDebug()<<"* savePauseMoment 1";
    //starterPauseStartedAt = QDateTime::currentDateTime();
    _screenLog->printError("Долгая работа стартера");
    checkAttemptsLimitReached();
}

void StarterController::engineShutdown(){
    setIgnition(false); // повторное нажатие при работающем двигателе — глушим ДВС
    resetIgnitionTimer();
    intentionalShutdown = true; // следующее падение оборотов — не авария, а штатное глушение
    //qDebug()<<"* savePauseMoment 2";
    starterPauseActive = true;
    starterPauseStartedAt = QDateTime::currentDateTime();
    updateStarterStatusText("Повторное нажатие старт/стоп: выключаем зажигание");
}

void StarterController::onStarterPress()  // бывший starterPressedEdge
{
    _mainWindow->preroll->resetPreroll();
    starterPressed = true;
    starterStartedTime = QDateTime::currentDateTime();
    if (getEngineRunning()){ // повторное нажатие при работающем двигателе — глушим ДВС
        engineShutdown();
        setIgnition(false);
        resetIgnitionTimer();
        intentionalShutdown = true;
        starterPauseActive = true;
        //qDebug()<<"* savePauseMoment 3";
        starterPauseStartedAt = QDateTime::currentDateTime();
        updateStarterStatusText("Повторное нажатие старт/стоп: выключаем зажигание");
        return;
    }

    // двигатель не запущен — только разовые сообщения, сама работа идёт в onHold
    if (starterNeedReboot)
        updateStarterStatusText("Достигнут лимит попыток запуска, требуется перезагрузка пульта", true);
    else if (starterBlocked()){
        if (starterLockedByRoll)
            updateStarterStatusText("Требуется прокрутка вспомогательного ДВС");
        else if (starterLockedByTemperature || _engine->waitOnStart)
            updateStarterStatusText("Требуется прогрев вспомогательного ДВС");
        else
            _screenLog->printError(getFatalStatusMessage());
    }
    else if (inStarterPause()){
        //qDebug()<<"* 2";
        updateStarterStatusText("Пауза между пусками " + QString::number(starterPauseSecondsLeft()) + " секунды осталось");
        starterPauseWarned = true;
    }
}

void StarterController::onStarterHold()  // каждый цикл, пока кнопка нажата
{
    if (getEngineRunning()){
        starterAttemptsUsed = 0;
        return;
    }
    // if (_globals->starterWorkingLimitReached(starterStartedTime)){
    //     handleStarterTimeout();
    //     return;
    // }

    if (starterNeedReboot){
        qDebug()<<"* starter need reboot";
        stopStarterOutput();
        starterStarted = false;
    }
    else if (starterBlocked()){
        qDebug()<<"* starter blocked";
        stopStarterOutput();
        starterStarted = false;
    }
    else if (inStarterPause()){
        qDebug()<<"* in starter pause";
        // if (_globals->starterWorkingLimitReached(starterStartedTime)){
        //     qDebug()<<"* starter working limit";
        //     handleStarterTimeout();
        //     return;
        // }
        //qDebug()<<"* 1";
        if (!starterPauseWarned){ // случай «кнопку держали, когда началась пауза»

            updateStarterStatusText("Пауза между пусками. Осталось секунд: " + QString::number(starterPauseSecondsLeft()) );
            starterPauseWarned = true;
        }
        stopStarterOutput();
        starterStarted = false;
    }
    else{
        if (_globals->starterWorkingLimitReached(starterStartedTime)){
            qDebug()<<"* working limit reached";
            handleStarterTimeout();
            return;
        }

       // if (!starterPauseActive){
            qDebug()<<"* ignition";
            setIgnition(true); // держим зажигание включённым, пока жмём
            //starterStartedTime = QDateTime::currentDateTime();
        // }
        // else{
        //     qDebug()<<"* pause active";
        // }

        if(trySetStarterState(true)){
            qDebug()<<"* !starter started";
        }
        else{
            qDebug()<<"* starter started";
        }
        //qDebug()<<"** 1";
        // if (!starterStarted){ // старт вращения — по флагу, а не по фронту: сработает и после конца паузы без нового нажатия
        //
        //     starterStarted = true;
        //     updateStarterStatusText("Стартер включен");
        //     //setStarterPressed(true);
        //     //starterPressed = true;
        //    // starterAttemptsUsed++;
        //     _gpio->setStarter(true);
        //     _gpio->setStarterLight(true);
        // }
        // else{
        //
        // }
        // if (_globals->starterWorkingLimitReached(starterStartedTime))
        //     handleStarterTimeout();
    }
}

bool StarterController::trySetStarterState(bool state){
    if(starterStarted == state){
        return false;
    }
    if(state){
        updateStarterStatusText("Стартер включен");
        //_can->setIgnition(true);
    }
    starterStarted = state;
    _gpio->setStarter(state);
    _gpio->setStarterLight(state);
    return true;
}

void StarterController::onStarterRelease()  // бывшая ветка else
{
    starterPressed = false;
    if (!trySetStarterState(false))
        return;

    //stopStarterOutput();
    //setStarterPressed(false);

    // starterStarted = false;
    // _gpio->setStarter(false);
    // _gpio->setStarterLight(false);
    starterAttemptsUsed++;
    if (!getEngineRunning()){ // не успел завестись — включаем паузу между пусками
        starterPauseActive = true;
        //qDebug()<<"* savePauseMoment 4";
        starterPauseStartedAt = QDateTime::currentDateTime();
        starterPauseWarned = false;
        checkAttemptsLimitReached();
    }
}

void StarterController::updateStarterState()  // каждый цикл, независимо от кнопки (бывший хвост функции)
{
    //qDebug("*");
    if (getEngineRunning()){
        if (starterStarted){ // двигатель набрал обороты, пока крутили стартер
            stopStarterOutput();
            starterStarted = false;
            starterPauseActive = false;
            updateStarterStatusText("Двигатель набрал обороты");
        }
        starterNeedReboot = false;
        starterAttemptsUsed = 0;
    }
}
bool StarterController::getEngineRunning(){
    return _engine->getRpm() > 700;
}

 void StarterController::showStarter(){

 }


void StarterController::stopStarterOutput(){
    setStarterPressed(false);
    _gpio->setStarterLight(false);
}

void StarterController::startStarterOutput(){
    updateStarterStatusText("Стартер включен");
    //_screenLog->printWarning("Стартер включен");
    setStarterPressed(true);
    _gpio->setStarterLight(true);
}

void StarterController::resetIgnitionTimer(){
    ignitionOffTimer = 0;
}

void StarterController::increaseIgnitionTimer(){
    ignitionOffTimer++;
    if (ignitionOffTimer >= _globals->restartIgnitionDelay * 10){
        setIgnition(true);
    }
}

void StarterController::setIgnition(bool state){ // запуск зажигания
    _can->setIgnition(state);
}

// Принудительно глушим зажигание извне (например, из PrerollController) и помечаем
// остановку как намеренную, чтобы showStarter() не выдал ложное "Двигатель заглох".
void StarterController::forceStopIgnition(){
    setIgnition(false);
    resetIgnitionTimer();
    intentionalShutdown = true;
}

// Восстанавливаем зажигание после отмены/завершения прокрутки.
void StarterController::restoreIgnition(){
    setIgnition(true);
    resetIgnitionTimer();
}


//     const auto rpm = _engine->getRpm();

//     // Двигатель считался работающим на прошлом такте, а сейчас обороты упали ниже 500 —
//     // это неожиданная остановка, но не намеренное глушение через старт/стоп.
//     if (rpm < 500 && engineWasRunning && !intentionalShutdown)
//         _screenLog->printError("Двигатель заглох");

//     checkPauseState();
//     const bool starterPressedEdge = isStarterClicked();
//     const bool engineRunning = rpm > 700;
//     qDebug()<<"#showStarter  starterPressed: "<<starterPressed <<"  pressedPrev: "<<starterPressedPrev;

//     if (engineRunning){
//         stopStarterOutput();
//         starterStarted = false;
//         intentionalShutdown = false; // прошлое намеренное выключение больше не актуально
//     }

//     if (starterPressedEdge){ //нажали кнопку стартера
//         //qDebug()<<"***";
//         if(engineRunning)//двигатель работает
//         {
//             engineShutdown();
//         }
//         else{
//             if (!starterPauseActive){ // если не находимся в паузе после глушения ДВС
//                 _can->setIgnition(true); // осуществляем запуск
//                 return;
//             }
//             if (starterNeedReboot){
//                 _screenLog->printError("Достигнут лимит попыток запуска, требуется перезагрузка пульта");
//             }
//             if (starterBlocked()){
//                 if (starterLockedByRoll)
//                     _screenLog->printWarning("Требуется прокрутка вспомогательного ДВС");
//                 else if (starterLockedByTemperature || _engine->waitOnStart)
//                     _screenLog->printWarning("Требуется прогрев вспомогательного ДВС");
//                 else
//                     _screenLog->printError(getFatalStatusMessage());
//             }
//             if (inStarterPause()){
//                 if (!starterPauseWarned){
//                     _screenLog->printWarning("Пауза между пусками " + QString::number(starterPauseSecondsLeft()) + " секунды осталось");
//                     starterPauseWarned = true;
//                 }
//             }
//             stopStarterOutput();
//             starterStarted = false;
//         }
//     }
//     else if (starterPressed){//стартер удерживается
//         //qDebug()<<"*";
//         if (_globals->starterWorkingLimitReached(starterStartedTime)){
//             handleStarterTimeout();
//             return;
//         }
//         if (!engineRunning){ // если двигатель не запущен
//             if (!starterStarted){
//                 starterStarted = true;
//                 starterStartedTime = QDateTime::currentDateTime();
//                 starterAttemptsUsed++;
//                 startStarterOutput();
//             }
//         }
//         else{
//             stopStarterOutput();
//             starterStarted = false;
//             starterPauseActive = false;
//             starterNeedReboot = false;
//             starterAttemptsUsed = 0;
//             _screenLog->printWarning("Двигатель набрал обороты");
//         }
//     }
//     else{//стартер отпущен
//         //qDebug()<<"**";
//         if (starterStarted){
//             stopStarterOutput();
//             starterStarted = false;
//             if (!engineRunning){
//                 starterPauseActive = true;
//                 starterPauseStartedAt = QDateTime::currentDateTime();
//                 checkAttemptsLimitReached();
//             }
//         }
//     }


//     // if (starterPressedEdge && engineRunning){ // двигатель запущен и нажали кнопку стартера
//     //     engineShutdown();
//     //     _can->setIgnition(false); // повторное нажатие при работающем двигателе — глушим ДВС
//     //     resetIgnitionTimer();
//     //     intentionalShutdown = true; // следующее падение оборотов — не авария, а штатное глушение
//     //     starterPauseActive = true;
//     //     starterPauseStartedAt = QDateTime::currentDateTime();
//     //     _screenLog->printWarning("Повторное нажатие старт/стоп: выключаем зажигание");
//     // }
//     // else if (starterPressed){ // либо двигатель не запущен, либо кнопку всё ещё держим
//     //     if (!starterPauseActive){ // если не находимся в паузе после глушения ДВС
//     //         _can->setIgnition(true); // осуществляем запуск
//     //     }

//     //     if (!engineRunning){ // если двигатель не запущен
//     //         if (starterNeedReboot){
//     //             if (starterPressedEdge)
//     //                 _screenLog->printError("Достигнут лимит попыток запуска, требуется перезагрузка пульта");
//     //             stopStarterOutput();
//     //             starterStarted = false;
//     //         }
//     //         else if (starterBlocked()){
//     //             if (starterPressedEdge){
//     //                 if (starterLockedByRoll)
//     //                     _screenLog->printWarning("Требуется прокрутка вспомогательного ДВС");
//     //                 else if (starterLockedByTemperature || _engine->waitOnStart)
//     //                     _screenLog->printWarning("Требуется прогрев вспомогательного ДВС");
//     //                 else
//     //                     _screenLog->printError(getFatalStatusMessage());
//     //             }
//     //             stopStarterOutput();
//     //             starterStarted = false;
//     //         }
//     //         else if (inStarterPause()){
//     //             if (starterPressedEdge || !starterPauseWarned){
//     //                 _screenLog->printWarning("Пауза между пусками " + QString::number(starterPauseSecondsLeft()) + " секунды осталось");
//     //                 starterPauseWarned = true;
//     //             }
//     //             stopStarterOutput();
//     //             starterStarted = false;
//     //         }
//     //         else{
//     //             if (!starterStarted){
//     //                 starterStarted = true;
//     //                 starterStartedTime = QDateTime::currentDateTime();
//     //                 starterAttemptsUsed++;
//     //                 _screenLog->printWarning("Стартер включен");
//     //                 setStarterPressed(true);
//     //                 _gpio->setStarterLight(true);
//     //             }

//     //             if (engineRunning){
//     //                 stopStarterOutput();//выключаем зажигание потому что двигатель набрал обороты
//     //                 starterStarted = false;
//     //                 starterPauseActive = false;
//     //                 starterNeedReboot = false;
//     //                 starterAttemptsUsed = 0;
//     //                 _screenLog->printWarning("Двигатель набрал обороты");
//     //             }
//     //             else if (_globals->starterWorkingLimitReached(starterStartedTime)){
//     //                 handleStarterTimeout();
//     //             }
//     //         }
//     //     }
//     // }
//     // else{
//     //     if (starterStarted){
//     //         stopStarterOutput();
//     //         starterStarted = false;
//     //         if (!engineRunning){
//     //             starterPauseActive = true;
//     //             starterPauseStartedAt = QDateTime::currentDateTime();
//     //             checkAttemptsLimitReached();
//     //         }
//     //     }
//     // }

//     // if (engineRunning){
//     //     starterNeedReboot = false;
//     //     starterAttemptsUsed = 0;
//     // }

//     // engineWasRunning = engineRunning;
//     // starterPressedPrev = starterPressed;
// }
