#include "prerollcontroller.h"
#include <currentstate.h>
#include <mainwindow.h>
#include <qpushbutton.h>

PrerollController::PrerollController(GlobalSettings *globals, Engine *engine,
                                     ScreenLog *screenLog, CanController *can,
                                     StarterController *starter, CurrentState *state, ViewController *view){
    _globals = globals;
    _engine = engine;
    _screenLog = screenLog;
    _can = can;
    _starter = starter;
    _state = state;
    _view = view;
}

void PrerollController::processPrerollInService(QPushButton* prerollButton,
                                                QPushButton* starterPrerollButton,
                                                QLabel* prerollStatusLabel,
                                                bool isEngineFormVisible){//superDiagMode
    const bool serviceEngineVisible = _state->isDiagMode() && isEngineFormVisible;
    _state->serviceIgnitionAutoRestoreBlocked = serviceEngineVisible || _state->prerollSequenceActive || _state->rollRunActive;
    //qDebug()<<"engineVisible: " << serviceEngineVisible<<"   prerollSequence: " << prerollSequenceActive <<"    rollRun: " << rollRunActive;
    if (starterPrerollButton != NULL)
        starterPrerollButton->setEnabled(_state->prerollStarterUnlocked && !_state->rollNeedReboot && !rollBlocked());

    // Обновляем состояние кнопки ПРОКРУТКА (зафиксирована когда идёт подготовка или активна)
    if (prerollButton != NULL)
        prerollButton->setChecked(_state->prerollSequenceActive || _state->prerollStarterUnlocked);

    if(_starter->starterPressed){
        qDebug()<<"### starterPressed";
        _state->prerollSequenceActive = false;
        _state->prerollStarterUnlocked = false;
        stopRollOutput();
        _can->setStarterAvailable(false);
    }

    updateRollStatusText(prerollStatusLabel);    // Обновляем статусную строку

    const bool prerollPressed = serviceEngineVisible && prerollButton != NULL && prerollButton->isDown();
    const bool prerollPressedEdge = prerollPressed && !_state->prerollButtonPrev;

    const bool prerollStarterPressed = serviceEngineVisible && starterPrerollButton != NULL && starterPrerollButton->isDown();
    const bool prerollStarterPressedEdge = prerollStarterPressed && !_state->prerollStarterButtonPrev;

    const bool rollInputPressed = _can->getRollIn();//can0->getState(StateRollIn).toBool();
    const bool rollInputPressedEdge = rollInputPressed && !_state->rollInputPrev;

    if (prerollPressedEdge){
       // qDebug()<<"### prerollPressed";
        if (_state->prerollSequenceActive || _state->prerollStarterUnlocked){
            // ОТМЕНА: выходим из режима прокрутки, восстанавливаем зажигание
            _state->prerollSequenceActive = false;
            _state->prerollStarterUnlocked = false;
            stopRollOutput();
            _can->setStarterAvailable(false);
            //can0->setState(StateStarterAllow, false);
            // restoreIgnitionAfterRoll();
            _screenLog->printLog("Режим прокрутки отменён оператором");
        }
        else if (rollBlocked()){
           _screenLog->printWarning("Прокрутка заблокирована");
        }
        else{
           _screenLog->printWarning("Запуск алгоритма прокрутки");
            _state->prerollSequenceActive = true;
            _state->prerollSequenceStep = 1;
            prerollStepStartedAt = QDateTime::currentDateTime();
            _state->prerollStarterUnlocked = false;
            stopRollOutput();
            _can->setStarterAvailable(false);
            _can->setIgnition(false);
            _starter->resetIgnitionTimer();
        }
    }

    if (_state->prerollSequenceActive){
        //qDebug()<<"### prerollPrepeared";
        const int elapsed = qAbs(prerollStepStartedAt.secsTo(QDateTime::currentDateTime()));
        if (_state->prerollSequenceStep == 1 && elapsed >= 2){
            _can->setStarterAvailable(true);
            //can0->setState(StateStarterAllow, true);
            _state->prerollSequenceStep = 2;
            prerollStepStartedAt = QDateTime::currentDateTime();
        }
        else if (_state->prerollSequenceStep == 2 && elapsed >= 1){
            _state->prerollStarterUnlocked = true;
            _state->prerollSequenceActive = false;
           _screenLog->printLog("Прокрутка подготовлена");
        }
    }

    if (_state->rollPauseActive && !inRollPause()){
        _state->rollPauseActive = false;
        _state->rollPauseWarned = false;
    }

    const bool rollStartRequest = prerollStarterPressedEdge || rollInputPressedEdge;
    if (rollStartRequest && !_state->rollRunActive){
        if (rollBlocked()){
           _screenLog->printWarning("Прокрутка заблокирована");
        }
        else if (inRollPause()){
            _screenLog->printWarning("Пауза между пусками " + QString::number(rollPauseSecondsLeft()) + " секунды осталось");
            _state->rollPauseWarned = true;
        }
        else if (!_state->prerollStarterUnlocked && !rollInputPressedEdge){
           _screenLog->printWarning("Сначала выполните подготовку прокрутки");
        }
        else{
            _state->rollRunActive = true;
            rollRunStartedAt = QDateTime::currentDateTime();
            _state->rollAttemptsUsed++;
            _starter->stopStarterOutput();
            _can->setRollStarter(true);
           _screenLog->printWarning("Стартер прокрутка включен");
        }
    }

    if (_state->rollRunActive){
        const bool oilRele = !_can->getOilRele();//can0->getState(StateOilRele).toBool();
        const int elapsedRoll = qAbs(rollRunStartedAt.secsTo(QDateTime::currentDateTime()));
        if (oilRele){
            stopRollOutput();
            _state->rollRunActive = false;
            _state->rollPauseActive = false;
            _state->rollNeedReboot = false;
            _state->rollAttemptsUsed = 0;
            _state->prerollStarterUnlocked = false;
            _can->setStarterAvailable(false);
            //can0->setState(StateStarterAllow, false);
            _state->rollCompleted = true;
            _state->needRollProcedure = false;
            _starter->starterLockedByRoll = false;
            _state->updateStartDate();
            // _state->lastEngineStartDate = QDate::currentDate();
            // _state->setValue("Engine/lastStartDate", lastEngineStartDate);
            // _state->sync();
            // ВОССТАНАВЛИВАЕМ ЗАЖИГАНИЕ после успешной прокрутки
            _can->setIgnition(true);
           // can0->setState(StateIgnitionOut, true);
            _starter->resetIgnitionTimer();


            _view->addLog("Прокрутка завершена по реле масла");
        }
        else if (_globals->rollWorkingLimitReached(rollRunStartedAt))//elapsedRoll >= rollMaxWorkSec)
        {
            stopRollOutput();
            _state->rollRunActive = false;
            _state->rollPauseActive = true;
            rollPauseStartedAt = QDateTime::currentDateTime();
            _view->addLogError("Долгая работа стартера");
            if (_globals->rollAttemptsLimitReached(_state->rollAttemptsUsed)){//rollAttemptsUsed >= rollMaxAttempts
                _state->rollNeedReboot = true;
                _view->addLogError("Достигнут лимит попыток прокрутки, требуется перезагрузка пульта");
            }
        }
    }
}

void PrerollController::updateRollStatusText(QLabel* statusLabel){
    //QLabel* statusLabel = serviceOtherEngineLeftForm->findChild<QLabel*>("label_prerollStatus");

    if (statusLabel == NULL){
        return;}

    QString statusText;
    if (_state->rollNeedReboot)
        statusText = "Лимит попыток исчерпан. Требуется перезагрузка пульта.";
    else if (_state->rollRunActive){
        const int elapsed = qAbs(rollRunStartedAt.secsTo(QDateTime::currentDateTime()));
        statusText = QString("Прокрутка активна... %1 сек. | Попытка %2/%3")
                         .arg(elapsed).arg(_state->rollAttemptsUsed).arg(_globals ->getRollAttempts());
    }
    else if (inRollPause())
        statusText = QString("Пауза между попытками: %1 сек.").arg(rollPauseSecondsLeft());
    else if (_state->prerollSequenceActive)
        statusText = "Подготовка прокрутки (выключение зажигания)...";
    else if (_state->prerollStarterUnlocked)
        statusText = "Готово — нажмите СТАРТЕР ПРОКРУТКА для прокрутки";
    else if (rollBlocked()){
        QStringList reasons;
        if (_state->rollLockedByTemperature)    reasons << "холодный двигатель (ждите теплореле)";
        if (_state->rollLockedByEmergency)      reasons << "аварийный режим";
        statusText = "Прокрутка заблокирована: " + reasons.join(", ");
        _view->setStyle(statusLabel,"color: red;");
    }
    else if (_state->needRollProcedure)
        statusText = "Требуется прокрутка. Нажмите ПРОКРУТКА для подготовки.";
    else if (_state->rollCompleted)
        statusText = "Прокрутка успешно завершена";
    else
        statusText = "";

    if (rollBlocked() && !statusText.isEmpty()){
        _view->setStyle(statusLabel,"color: red;");
    }
    else{
        _view->setStyle(statusLabel,"color: yellow;");
    }

    if (statusLabel->text() != statusText)
        statusLabel->setText(statusText);

}

bool PrerollController::rollBlocked() const{
    return _state->rollLockedByTemperature
           || _state->rollLockedByEmergency
           || _state->rollNeedReboot;
}

void PrerollController::stopRollOutput(){
    _can->setRollStarter(false);
    //can0->setState(StateStarterRoll, false);
}

bool PrerollController::inRollPause() const{
    if (!_state->rollPauseActive)
        return false;
    return _globals->checkRollPause(rollPauseStartedAt);
}

int PrerollController::rollPauseSecondsLeft() const{
    if (!_state->rollPauseActive)
        return 0;
    return _globals-> rollPauseSecondsLeft(rollPauseStartedAt);
}

void PrerollController::resetValues(){
    _state->rollCompleted = false;
    _state->logNeedRollShown = false;
}

void PrerollController::checkIfAwaitForRoll(int daysFromLastStart){
    _state->needRollProcedure = !_state->rollCompleted && !_state->disableRollRequirement &&
                        _globals->isNeedRoolByDate(daysFromLastStart);
    _starter->starterLockedByRoll = _state->needRollProcedure;
}

void PrerollController::lockByTemperature(bool state){
    _starter->starterLockedByTemperature = state;
    _state->rollLockedByTemperature = state;
}

void PrerollController::checkEmergencies(){
    _state->waterAlarm = _can->getState(StateWaterSensor) && _state->waterSensorEmergencyMode;
    _state->airAlarm = _can->getState(StateAirFilterBad) && _state->airFilterEmergencyMode;
    _state->oilAlarm = _can->getState(StateOilFilterBad);
    bool isAlarm = _state->waterAlarm || _state->airAlarm || _state->oilAlarm;

    _starter->starterLockedByEmergency = !_state->ignoreAllEmergency && isAlarm;
    _state->rollLockedByEmergency = !_state->ignoreAllEmergency && isAlarm;

    if (_state->needRollProcedure && !_state->logNeedRollShown){
        _view->screenLog->printWarning("Требуется прокрутка вспомогательного ДВС");
        _state->logNeedRollShown = true;
    }
    if (!_state->needRollProcedure){
        _state->logNeedRollShown = false;
    }

    if (_starter->starterLockedByTemperature && !_state->logNeedWarmShown){
        _view->screenLog->printWarning("Требуется прогрев вспомогательного ДВС");
        _state->logNeedWarmShown = true;
    }
    if (!_starter->starterLockedByTemperature){
        _state->logNeedWarmShown = false;
    }
}
