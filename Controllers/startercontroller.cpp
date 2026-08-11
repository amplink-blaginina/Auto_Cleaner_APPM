#include "startercontroller.h"

#include "cancontroller.h"
#include "gpiocontroller.h"

StarterController::StarterController(GlobalSettings *globals,
                                     CurrentState *state,
                                     Engine *engine,
                                     ScreenLog *screenLog,
                                     CanController *can0,
                                     GPIOController *gpio) {
    _globals = globals;
    _state = state;
    _engine = engine;
    _screenLog = screenLog;
    _can = can0;
    _gpio = gpio;
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
    //starter = false;
    starterStartedAlarmed = false;
    starterBroomAlarmed = false;
    starterBunkerAlarmed = false;
    ignitionOffTimer = 0;
    engineStartedOk = false;
}

void StarterController::resetValues(){
    starterNeedReboot = false;
    starterAttemptsUsed = 0;
    starterPauseActive = false;
}
bool StarterController::inStarterPause() const{
    if (!starterPauseActive)
        return false;
    return _globals->checkStarterPause(starterPauseStartedAt);
    // const int passed = qAbs(starterPauseStartedAt.secsTo(QDateTime::currentDateTime()));
    // //qDebug()<<"timer: "<<passed<<"   targetTime: "<<starterPauseSec;
    // return passed < starterPauseSec;
}
int StarterController::starterPauseSecondsLeft() const{
    if (!starterPauseActive)
        return 0;
    return _globals->starterPauseSecondsLeft(starterPauseStartedAt);
    // const int passed = qAbs(starterPauseStartedAt.secsTo(QDateTime::currentDateTime()));
    // return qMax(0, starterPauseSec - passed);
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
        starterPauseActive = false;
        starterPauseWarned = false;
    }
}

bool StarterController::isStarterClicked(){
    //const bool starterPressedEdge =
    return starterPressed && !starterPressedPrev;
}

void StarterController:: setStarterPressed(bool state){
    if(starterPressed == state)
        return;
    starterPressed = state;
    _gpio->setStarter(state);
    //qDebug()<<(state?"Зажигание нажато":"Зажигание не нажато");
    //_gpio->setOutput(GPIOOutput::OUT_STARTER, state);
}

bool StarterController:: isStarterPressed(){
    return starterPressed;
}

QString StarterController::getFatalStatusMessage(){
    if(!starterBlocked()){
        return "Стартер не заблокирован";
    }
    if(starterLockedByRoll){
        return "Стартер заблокирован: прокрутка";
    }
    if(starterLockedByTemperature){
        return "Стартер заблокирован: требуется прогрев двигателя";
    }
    if(starterLockedByEmergency){
        if(_state->waterAlarm)
            return "Стартер заблокирован: по датчику воды";
        if(_state->airAlarm)
            return "Стартер заблокирован: по датчику воздуха";
        if(_state->oilAlarm)
            return "Стартер заблокирован: по датчику масла";
    }
    if(starterNeedReboot){
        return "Стартер заблокирован: требуется перезагрузка";
    }
    if(_engine->waitOnStart){
        return "Стартер заблокирован: ожидание на старте";
    }
    return "Стартер не заблокирован";
}

void StarterController::showStarter(){
    // const bool starterPressed = gpioMatirx->keyPressed == GPIOInput::IN_STARTER;
    // qDebug()<<"starter pressed: "<< starterPressed<<"    starterButtonPressed: "<<starterPressed;

    const auto rpm = _engine->getRpm();
    // // Если идёт прокрутка — стартер не управляется отсюда
    // if (rollRunActive || prerollSequenceActive){
    //     starterPressedPrev = starterPressed;
    //     return;
    // }

    if (rpm < 500 && engineStartedOk)
       _screenLog->printError("Двигатель заглох");
    checkPauseState();
    // // БЛОКИРОВКА: пока идёт прокрутка — кнопка стартера не управляет зажиганием
    const bool starterPressedEdge = isStarterClicked();
    //starterPressed && !starterPressedPrev;

    // if (rollRunActive || prerollSequenceActive)
    // {
    //     starterPressedPrev = starterPressed;
    //     return;
    // }

    const bool engineRunning = rpm > 700;

    if (engineRunning){
        stopStarterOutput();
        starterStarted = false;
    }

    if (starterPressedEdge && engineRunning){//двигатель запущен и нажали кнопку стартера
        _can->setIgnition(false);// Повторное нажатие при работающем двигателе — глушим ДВС
        resetIgnitionTimer();
        engineStartedOk = false;
        starterPauseActive = true;
        starterPauseStartedAt = QDateTime::currentDateTime();
       _screenLog->printWarning("Повторное нажатие старт/стоп: выключаем зажигание");
    }
    else if (starterPressed){// либо двигатель не запущен, либо нажали на кнопку стартера не только что, но всё ещё держим
        if(!starterPauseActive){//если не находимся в паузе после глушения ДВС
            //qDebug()<<" Зажигание 1";
            _can->setIgnition(true);//осуществляем запуск
        }

        if (!engineRunning){//если двигатель не запущен
            if (starterNeedReboot){
                if (starterPressedEdge)
                   _screenLog->printError("Достигнут лимит попыток запуска, требуется перезагрузка пульта");
                stopStarterOutput();
                starterStarted = false;
            }
            else if (starterBlocked()){
                if (starterPressedEdge){
                    if (starterLockedByRoll)
                       _screenLog->printWarning("Требуется прокрутка вспомогательного ДВС");
                    else if (starterLockedByTemperature || _engine->waitOnStart)
                       _screenLog->printWarning("Требуется прогрев вспомогательного ДВС");
                    else
                       _screenLog->printError(getFatalStatusMessage());
                }
                stopStarterOutput();
                starterStarted = false;
            }
            else if (inStarterPause()){
                if (starterPressedEdge || !starterPauseWarned){
                   _screenLog->printWarning("Пауза между пусками " + QString::number(starterPauseSecondsLeft()) + " секунды осталось");
                    starterPauseWarned = true;
                }
                stopStarterOutput();
                starterStarted = false;
            }
            else{
                if (!starterStarted){
                    starterStarted = true;
                    starterStartedTime = QDateTime::currentDateTime();
                    starterAttemptsUsed++;
                   _screenLog->printWarning("Стартер включен");
                    setStarterPressed(true);
                    //gpio->setOutput(GPIOOutput::OUT_STARTER, true);
                    _gpio->setStarterLight(true);
                }

                const int starterRunTime = qAbs(starterStartedTime.secsTo(QDateTime::currentDateTime()));
                if (engineRunning){
                    stopStarterOutput();
                    starterStarted = false;
                    starterPauseActive = false;
                    starterNeedReboot = false;
                    starterAttemptsUsed = 0;
                   _screenLog->printWarning("Двигатель набрал обороты");
                }
                else if (_globals-> starterWorkingLimitReached(starterStartedTime)){//starterRunTime >= starterMaxWorkSec
                    stopStarterOutput();
                    starterStarted = false;
                    starterPauseActive = true;
                    starterPauseStartedAt = QDateTime::currentDateTime();
                   _screenLog->printError("Долгая работа стартера");
                    if (_globals->starterAttemptsLimitReached(starterAttemptsUsed)){
                        starterNeedReboot = true;
                       _screenLog->printError("Достигнут лимит попыток запуска, требуется перезагрузка пульта");
                    }
                }
            }
        }
    }
    else{
        if (starterStarted){
            stopStarterOutput();
            starterStarted = false;
            if (!engineRunning){
                starterPauseActive = true;
                starterPauseStartedAt = QDateTime::currentDateTime();
                if (_globals->starterAttemptsLimitReached(starterAttemptsUsed)){//starterAttemptsUsed >= starterMaxAttempts){
                    starterNeedReboot = true;
                   _screenLog->printError("Достигнут лимит попыток запуска, требуется перезагрузка пульта");
                }
            }
        }
    }

    if (engineRunning){
        engineStartedOk = true;
        starterNeedReboot = false;
        starterAttemptsUsed = 0;
    }
    else{
        engineStartedOk = false;
    }

    starterPressedPrev = starterPressed;
}

void StarterController::stopStarterOutput(){
    setStarterPressed(false);
    //gpio->setOutput(GPIOOutput::OUT_STARTER, false);
    _gpio->setStarterLight(false);
}
void StarterController::resetIgnitionTimer(){
    ignitionOffTimer = 0;
}
void StarterController::increaseIgnitionTimer(){
    ignitionOffTimer++;
    if (ignitionOffTimer >= _globals->restartIgnitionDelay * 10){
        startIgnition();
    }
}

void StarterController::startIgnition(){// запуск зажигания
    //qDebug() << "start ignition";
    //gp->Set_GPIO_State(OUT_IGNITION, 1);
    //qDebug()<<" Зажигание 2";
    _can->setIgnition(true);
    //can0->setState(StateIgnitionOut, true);
}
