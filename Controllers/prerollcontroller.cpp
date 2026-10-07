#include "prerollcontroller.h"
#include <currentstate.h>
#include <mainwindow.h>
#include <qpushbutton.h>

PrerollController::PrerollController(GlobalSettings *globals, Engine *engine,
                                     ScreenLog *screenLog, CanController *can,
                                     StarterController *starter, CurrentState *state,
                                     ViewController *view, MainWindow* mainWindow){
    _globals = globals;
    _engine = engine;
    _screenLog = screenLog;
    _can = can;
    _starter = starter;
    _state = state;
    _view = view;
    _mainWindow = mainWindow;
    configureButtons();
}

void PrerollController::configureButtons(){
    m_preroll = BoolStateWatcher{
         {
             .onActivated = [this] {
                 qDebug()<<"*PrerollPressed";

             },
             .onDeactivated = [this] {
                 qDebug()<<"*PrerollReleased";

             },
             .whileActive = [this] {

             },
             .whileInactive = [this] {

             }
        }};
}

void PrerollController::setEngineForm(ServiceOtherEngineLeftForm *otherEngineForm){
    _otherEngineForm = otherEngineForm;
    _prerollBtn = _otherEngineForm->findChild<QPushButton*>("pushButton_preroll");
    _starterPrerollBtn = _otherEngineForm->findChild<QPushButton*>("pushButton_starterPreroll");
    _statusLbl = _otherEngineForm->findChild<QLabel*>("label_prerollStatus");

    _isInited = _prerollBtn != NULL;
}

void PrerollController::setPrerollPressed(bool state){
    m_preroll.update(state);
}

bool PrerollController::foundButtons(){
    if(!_isInited){
        return false;
    }
    return _prerollBtn != NULL;
}

void PrerollController::update(){
    processPrerollInService(_otherEngineForm->isVisible());
}

void PrerollController::resetPreroll(){
    _state->prerollSequenceActive = false;
    _state->prerollStarterUnlocked = false;
    stopRollOutput();
    _can->setStarterAvailable(false);
}

void PrerollController::processPrerollInService(bool isEngineFormVisible){//superDiagMode
    if(!_isInited){
        return;
    }
    //qDebug()<<"check Preroll";
    const bool serviceEngineVisible = _state->isDiagMode() && isEngineFormVisible;
    _state->serviceIgnitionAutoRestoreBlocked = serviceEngineVisible || _state->prerollSequenceActive || _state->rollRunActive;

    _starterPrerollBtn->setEnabled(_state->prerollStarterUnlocked && !_state->rollNeedReboot && !rollBlocked());

    // Обновляем состояние кнопки ПРОКРУТКА (зафиксирована когда идёт подготовка или активна)
    _prerollBtn->setChecked(_state->prerollSequenceActive || _state->prerollStarterUnlocked);
    //qDebug()<<"* isStarterStarted: "<<_starter->starterPressed;
    if(_starter->starterPressed){
        resetPreroll();
    }

    //updateRollStatusText();

    const bool prerollPressed = serviceEngineVisible && _prerollBtn->isDown();
    setPrerollPressed(prerollPressed);
    const bool prerollPressedEdge = prerollPressed && !_state->prerollButtonPrev;

    const bool prerollStarterPressed = serviceEngineVisible && _starterPrerollBtn->isDown();
    const bool prerollStarterPressedEdge = prerollStarterPressed && !_state->prerollStarterButtonPrev;

    const bool rollInputPressed = readRollInput(serviceEngineVisible);
    const bool rollInputPressedEdge = rollInputPressed && !_state->rollInputPrev;

    if (prerollPressedEdge){
        if (_state->prerollSequenceActive || _state->prerollStarterUnlocked){
            // ОТМЕНА: выходим из режима прокрутки, восстанавливаем зажигание
            _state->prerollSequenceActive = false;
            _state->prerollStarterUnlocked = false;
            stopRollOutput();
            _can->setStarterAvailable(false);
            _starter->restoreIgnition();
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
            _starter->forceStopIgnition(); // намеренно глушим ДВС для подготовки к прокрутке
        }
    }

    if (_state->prerollSequenceActive){
        const int elapsed = qAbs(prerollStepStartedAt.secsTo(QDateTime::currentDateTime()));
        if (_state->prerollSequenceStep == 1 && elapsed >= 2){
            _can->setStarterAvailable(true);
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
        const bool oilRele = !_can->getOilRele();
        const int elapsedRoll = qAbs(rollRunStartedAt.secsTo(QDateTime::currentDateTime()));
        if (oilRele){
            stopRollOutput();
            _state->rollRunActive = false;
            _state->rollPauseActive = false;
            _state->rollNeedReboot = false;
            _state->rollAttemptsUsed = 0;
            _state->prerollStarterUnlocked = false;
            _can->setStarterAvailable(false);
            _state->rollCompleted = true;
            _state->needRollProcedure = false;
            _starter->starterLockedByRoll = false;
            qDebug()<<"!!! oilRele: updateStartDate";
            _state->updateStartDate();
            // ВОССТАНАВЛИВАЕМ ЗАЖИГАНИЕ после успешной прокрутки
            _starter->restoreIgnition();

            _view->addLog("Прокрутка завершена по реле масла");
        }
        else if (_globals->rollWorkingLimitReached(rollRunStartedAt))
        {
            stopRollOutput();
            _state->rollRunActive = false;
            _state->rollPauseActive = true;
            rollPauseStartedAt = QDateTime::currentDateTime();
            _view->addLogError("Долгая работа стартера");
            if (_globals->rollAttemptsLimitReached(_state->rollAttemptsUsed)){
                _state->rollNeedReboot = true;
                _view->addLogError("Достигнут лимит попыток прокрутки, требуется перезагрузка пульта");
            }
        }
    }

    updateRollStatusText();// Обновляем статусную строку
    // Фиксируем состояние кнопок/входа для корректного определения фронта на следующем такте
    _state->prerollButtonPrev = prerollPressed;
    _state->prerollStarterButtonPrev = prerollStarterPressed;
    _state->rollInputPrev = rollInputPressed;
}

bool PrerollController::readRollInput(bool serviceEngineVisible){
    const bool raw = _can->getRollIn();
    // окно проверки на залипание - от появления связи с блоком: раньше вход ещё не читается
    if (!rollInputSinceOnline.isValid() && _can->isBoard0IN())
        rollInputSinceOnline.start();
    if (raw && !rollInputFaulty && rollInputSinceOnline.isValid()
        && rollInputSinceOnline.elapsed() < ROLL_STUCK_WINDOW_MS){
        rollInputFaulty = true;
        qWarning() << "PREROLL: кнопка прокрутки на пульте нажата при запуске программы - неисправна, сигнал с неё игнорируется";
    }
    if (rollInputFaulty)
        return false;

    if (!serviceEngineVisible)
        rollInputArmed = false;// вне окна ДВС кнопка не работает
    else if (!raw)
        rollInputArmed = true;// отпущена в окне - следующее нажатие засчитаем
    return serviceEngineVisible && rollInputArmed && raw;
}

void PrerollController::warnIfRollInputBlocked(){
    QString text;
    if (rollInputFaulty)
        text = "Кнопка прокрутки на пульте неисправна (нажата при включении). Сигнал с неё игнорируется";
    else if (_can->getRollIn())
        text = "Кнопка прокрутки на пульте нажата. Отпустите её - прокрутка запускается новым нажатием";
    else
        return;
    _screenLog->printWarning(text);
    if (_statusLbl){
        _view->setStyle(_statusLbl, "color: yellow;");
        _statusLbl->setText(text);
    }
}

void PrerollController::updateRollStatusText(){
    QString statusText;
    if(_starter->isStarterPressed()){
        return;
    }
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
        if (_state->rollLockedByTemperature) reasons << "холодный двигатель (ждите теплореле)";
        if (_state->rollLockedByEmergency) reasons << "аварийный режим";
        statusText = "Прокрутка заблокирована: " + reasons.join(", ");
        _view->setStyle(_statusLbl,"color: red;");
    }
    else if (_state->needRollProcedure){
        statusText = "Требуется прокрутка. Нажмите ПРОКРУТКА для подготовки.";
    }
    else if (_state->rollCompleted)
        statusText = "Прокрутка успешно завершена";
     else
         return;

    _view->setStyle(_statusLbl, (rollBlocked() && !statusText.isEmpty())? "color: red;": "color: yellow;");

    if (_statusLbl->text() != statusText)
        _statusLbl->setText(statusText);
}

bool PrerollController::rollBlocked() const{
    return _state->rollLockedByTemperature
           || _state->rollLockedByEmergency
           || _state->rollNeedReboot;
}

void PrerollController::stopRollOutput(){
    _can->setRollStarter(false);
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
    //qDebug()<<"!!! isAwaitForRoll:  rollCompleted:"<<_state->rollCompleted<<"  disableRollRequirement: "<<_state->disableRollRequirement<<"  needRollByDate: "<<_globals->isNeedRoolByDate(daysFromLastStart);
    _state->needRollProcedure = !_state->rollCompleted && !_state->disableRollRequirement &&
                                _globals->isNeedRoolByDate(daysFromLastStart);
    _starter->starterLockedByRoll = _state->needRollProcedure;
}

void PrerollController::lockByTemperature(bool state){
    _starter->starterLockedByTemperature = state;
    _state->rollLockedByTemperature = state;
}

void PrerollController::checkEmergencies(){
    bool isAlarm = _state->isAlarm();

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
