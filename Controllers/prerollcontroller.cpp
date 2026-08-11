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
    _state->serviceIgnitionAutoRestoreBlocked = serviceEngineVisible || prerollSequenceActive || rollRunActive;
    //qDebug()<<"engineVisible: " << serviceEngineVisible<<"   prerollSequence: " << prerollSequenceActive <<"    rollRun: " << rollRunActive;
    if (starterPrerollButton != NULL)
        starterPrerollButton->setEnabled(prerollStarterUnlocked && !rollNeedReboot && !rollBlocked());

    // Обновляем состояние кнопки ПРОКРУТКА (зафиксирована когда идёт подготовка или активна)
    if (prerollButton != NULL)
        prerollButton->setChecked(prerollSequenceActive || prerollStarterUnlocked);

    if(_starter->starterPressed){
        qDebug()<<"### starterPressed";
        prerollSequenceActive = false;
        prerollStarterUnlocked = false;
        stopRollOutput();
        _can->setStarterAvailable(false);
    }

    updateRollStatusText(prerollStatusLabel);    // Обновляем статусную строку

    const bool prerollPressed = serviceEngineVisible && prerollButton != NULL && prerollButton->isDown();
    const bool prerollPressedEdge = prerollPressed && !prerollButtonPrev;

    const bool prerollStarterPressed = serviceEngineVisible && starterPrerollButton != NULL && starterPrerollButton->isDown();
    const bool prerollStarterPressedEdge = prerollStarterPressed && !prerollStarterButtonPrev;

    const bool rollInputPressed = _can->getRollIn();//can0->getState(StateRollIn).toBool();
    const bool rollInputPressedEdge = rollInputPressed && !rollInputPrev;

    if (prerollPressedEdge){
       // qDebug()<<"### prerollPressed";
        if (prerollSequenceActive || prerollStarterUnlocked){
            // ОТМЕНА: выходим из режима прокрутки, восстанавливаем зажигание
            prerollSequenceActive = false;
            prerollStarterUnlocked = false;
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
            prerollSequenceActive = true;
            prerollSequenceStep = 1;
            prerollStepStartedAt = QDateTime::currentDateTime();
            prerollStarterUnlocked = false;
            stopRollOutput();
            _can->setStarterAvailable(false);
            _can->setIgnition(false);
            _starter->resetIgnitionTimer();
        }
    }

    if (prerollSequenceActive){
        //qDebug()<<"### prerollPrepeared";
        const int elapsed = qAbs(prerollStepStartedAt.secsTo(QDateTime::currentDateTime()));
        if (prerollSequenceStep == 1 && elapsed >= 2){
            _can->setStarterAvailable(true);
            //can0->setState(StateStarterAllow, true);
            prerollSequenceStep = 2;
            prerollStepStartedAt = QDateTime::currentDateTime();
        }
        else if (prerollSequenceStep == 2 && elapsed >= 1){
            prerollStarterUnlocked = true;
            prerollSequenceActive = false;
           _screenLog->printLog("Прокрутка подготовлена");
        }
    }

    if (rollPauseActive && !inRollPause()){
        rollPauseActive = false;
        rollPauseWarned = false;
    }

    const bool rollStartRequest = prerollStarterPressedEdge || rollInputPressedEdge;
    if (rollStartRequest && !rollRunActive){
        if (rollBlocked()){
           _screenLog->printWarning("Прокрутка заблокирована");
        }
        else if (inRollPause()){
            _screenLog->printWarning("Пауза между пусками " + QString::number(rollPauseSecondsLeft()) + " секунды осталось");
            rollPauseWarned = true;
        }
        else if (!prerollStarterUnlocked && !rollInputPressedEdge){
           _screenLog->printWarning("Сначала выполните подготовку прокрутки");
        }
        else{
            rollRunActive = true;
            rollRunStartedAt = QDateTime::currentDateTime();
            rollAttemptsUsed++;
            _starter->stopStarterOutput();
            _can->setRollStarter(true);
           _screenLog->printWarning("Стартер прокрутка включен");
        }
    }

    if (rollRunActive){
        const bool oilRele = !_can->getOilRele();//can0->getState(StateOilRele).toBool();
        const int elapsedRoll = qAbs(rollRunStartedAt.secsTo(QDateTime::currentDateTime()));
        if (oilRele){
            stopRollOutput();
            rollRunActive = false;
            rollPauseActive = false;
            rollNeedReboot = false;
            rollAttemptsUsed = 0;
            prerollStarterUnlocked = false;
            _can->setStarterAvailable(false);
            //can0->setState(StateStarterAllow, false);
            rollCompleted = true;
            needRollProcedure = false;
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
            rollRunActive = false;
            rollPauseActive = true;
            rollPauseStartedAt = QDateTime::currentDateTime();
            _view->addLogError("Долгая работа стартера");
            if (_globals->rollAttemptsLimitReached(rollAttemptsUsed)){//rollAttemptsUsed >= rollMaxAttempts
                rollNeedReboot = true;
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
    if (rollNeedReboot)
        statusText = "Лимит попыток исчерпан. Требуется перезагрузка пульта.";
    else if (rollRunActive){
        const int elapsed = qAbs(rollRunStartedAt.secsTo(QDateTime::currentDateTime()));
        statusText = QString("Прокрутка активна... %1 сек. | Попытка %2/%3")
                         .arg(elapsed).arg(rollAttemptsUsed).arg(_globals ->getRollAttempts());
    }
    else if (inRollPause())
        statusText = QString("Пауза между попытками: %1 сек.").arg(rollPauseSecondsLeft());
    else if (prerollSequenceActive)
        statusText = "Подготовка прокрутки (выключение зажигания)...";
    else if (prerollStarterUnlocked)
        statusText = "Готово — нажмите СТАРТЕР ПРОКРУТКА для прокрутки";
    else if (rollBlocked()){
        QStringList reasons;
        if (rollLockedByTemperature)    reasons << "холодный двигатель (ждите теплореле)";
        if (rollLockedByEmergency)      reasons << "аварийный режим";
        statusText = "Прокрутка заблокирована: " + reasons.join(", ");
        _view->setStyle(statusLabel,"color: red;");
    }
    else if (needRollProcedure)
        statusText = "Требуется прокрутка. Нажмите ПРОКРУТКА для подготовки.";
    else if (rollCompleted)
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
    return rollLockedByTemperature
           || rollLockedByEmergency
           || rollNeedReboot;
}

void PrerollController::stopRollOutput(){
    _can->setRollStarter(false);
    //can0->setState(StateStarterRoll, false);
}

bool PrerollController::inRollPause() const{
    if (!rollPauseActive)
        return false;
    return _globals->checkRollPause(rollPauseStartedAt);
}

int PrerollController::rollPauseSecondsLeft() const{
    if (!rollPauseActive)
        return 0;
    return _globals-> rollPauseSecondsLeft(rollPauseStartedAt);
}

void PrerollController::resetValues(){
    rollCompleted = false;
    logNeedRollShown = false;
}

void PrerollController::checkIfAwaitForRoll(int daysFromLastStart){
    needRollProcedure = !rollCompleted && !_state->disableRollRequirement &&
                        _globals->isNeedRoolByDate(daysFromLastStart);
    _starter->starterLockedByRoll = needRollProcedure;
}

void PrerollController::lockByTemperature(bool state){
    _starter->starterLockedByTemperature = state;
    rollLockedByTemperature = state;
}

void PrerollController::checkEmergencies(){
    _state->waterAlarm = _can->getState(StateWaterSensor) && _state->waterSensorEmergencyMode;
    _state->airAlarm = _can->getState(StateAirFilterBad) && _state->airFilterEmergencyMode;
    _state->oilAlarm = _can->getState(StateOilFilterBad);
    bool isAlarm = _state->waterAlarm || _state->airAlarm || _state->oilAlarm;

    _starter->starterLockedByEmergency = !_state->ignoreAllEmergency && isAlarm;
    rollLockedByEmergency = !_state->ignoreAllEmergency && isAlarm;

    if (needRollProcedure && !logNeedRollShown){
        _view->screenLog->printWarning("Требуется прокрутка вспомогательного ДВС");
        logNeedRollShown = true;
    }
    if (!needRollProcedure){
        logNeedRollShown = false;
    }

    if (_starter->starterLockedByTemperature && !_state->logNeedWarmShown){
        _view->screenLog->printWarning("Требуется прогрев вспомогательного ДВС");
        _state->logNeedWarmShown = true;
    }
    if (!_starter->starterLockedByTemperature){
        _state->logNeedWarmShown = false;
    }
}
