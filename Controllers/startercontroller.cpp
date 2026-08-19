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
        starterPauseActive = false;
        starterPauseWarned = false;
    }
}

bool StarterController::isStarterClicked(){
    return starterPressed && !starterPressedPrev;
}

void StarterController::setStarterPressed(bool state){
    if (starterPressed == state)
        return;
    starterPressed = state;
    _gpio->setStarter(state);
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
    stopStarterOutput();
    starterStarted = false;
    starterPauseActive = true;
    starterPauseStartedAt = QDateTime::currentDateTime();
    _screenLog->printError("Долгая работа стартера");
    checkAttemptsLimitReached();
}

void StarterController::showStarter(){
    const auto rpm = _engine->getRpm();

    // Двигатель считался работающим на прошлом такте, а сейчас обороты упали ниже 500 —
    // это неожиданная остановка, но не намеренное глушение через старт/стоп.
    if (rpm < 500 && engineWasRunning && !intentionalShutdown)
        _screenLog->printError("Двигатель заглох");

    checkPauseState();
    const bool starterPressedEdge = isStarterClicked();
    const bool engineRunning = rpm > 700;

    if (engineRunning){
        stopStarterOutput();
        starterStarted = false;
        intentionalShutdown = false; // прошлое намеренное выключение больше не актуально
    }

    if (starterPressedEdge && engineRunning){ // двигатель запущен и нажали кнопку стартера
        _can->setIgnition(false); // повторное нажатие при работающем двигателе — глушим ДВС
        resetIgnitionTimer();
        intentionalShutdown = true; // следующее падение оборотов — не авария, а штатное глушение
        starterPauseActive = true;
        starterPauseStartedAt = QDateTime::currentDateTime();
        _screenLog->printWarning("Повторное нажатие старт/стоп: выключаем зажигание");
    }
    else if (starterPressed){ // либо двигатель не запущен, либо кнопку всё ещё держим
        if (!starterPauseActive){ // если не находимся в паузе после глушения ДВС
            _can->setIgnition(true); // осуществляем запуск
        }

        if (!engineRunning){ // если двигатель не запущен
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
                    _gpio->setStarterLight(true);
                }

                if (engineRunning){
                    stopStarterOutput();
                    starterStarted = false;
                    starterPauseActive = false;
                    starterNeedReboot = false;
                    starterAttemptsUsed = 0;
                    _screenLog->printWarning("Двигатель набрал обороты");
                }
                else if (_globals->starterWorkingLimitReached(starterStartedTime)){
                    handleStarterTimeout();
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
                checkAttemptsLimitReached();
            }
        }
    }

    if (engineRunning){
        starterNeedReboot = false;
        starterAttemptsUsed = 0;
    }
    engineWasRunning = engineRunning;

    starterPressedPrev = starterPressed;
}

void StarterController::stopStarterOutput(){
    setStarterPressed(false);
    _gpio->setStarterLight(false);
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
    _can->setIgnition(false);
    resetIgnitionTimer();
    intentionalShutdown = true;
}

// Восстанавливаем зажигание после отмены/завершения прокрутки.
void StarterController::restoreIgnition(){
    _can->setIgnition(true);
    resetIgnitionTimer();
}
