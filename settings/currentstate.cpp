#include "currentstate.h"
#include "settingsreader.h"
#include <qdatetime.h>
#include <Controllers/gpiocontroller.h>

CurrentState::CurrentState(SettingsReader *reader, GPIOController *gpio) {
    _reader = reader;
    _gpio = gpio;
}

void CurrentState::setDefaults(){
    disableRollRequirement = false;
    rollLockedByTemperature = false;
    rollLockedByEmergency = false;
    needRollProcedure = false;
    rollCompleted = false;
    waterAlarm = false;
    airAlarm = false;
    oilAlarm = false;
    
    superDiagMode = false;
    
    prerollButtonPrev = false;
    prerollStarterButtonPrev = false;
    rollInputPrev = false;
    prerollSequenceActive = false;
    prerollSequenceStep = 0;
    prerollStarterUnlocked = false;
    rollRunActive = false;
    rollPauseActive = false;
    rollAttemptsUsed = 0;
    rollNeedReboot = false;
    rollPauseWarned = false;
    serviceIgnitionAutoRestoreBlocked = false;
    
    logNeedRollShown = false;
    logNeedWarmShown = false;
    waterSensorActivePrev = false;
    airFilterActivePrev = false;
    oilFilterActivePrev = false;
    heatRelayActivePrev = false;
    
    waterSensorEmergencyMode = true;
    airFilterEmergencyMode = true;
    
    disableTemperatureBlock = false;
    ignoreAllEmergency = false;
    
    engineRunStatePrev = false;
    
    
    waterSensorTimeStarted = false;
    airFilterTimeStarted = false;
    waterSensorStartedAt = 0;
    airFilterStartedAt = 0;
}

void CurrentState::setMode(quint8 mode){
    menuMode = mode;
}
void CurrentState::setSweepMode(){
    menuMode = SweepMode;
    superDiagMode = false;
}

void CurrentState::setDiagMode(bool state){
    if(state){
        superDiagMode = true;
        menuMode = DiagMode;}
    else{
        superDiagMode = false;
    }
}

void CurrentState::setSettingsMode(){
    setMode(CurrentState::SettingsMode);
}

void CurrentState::readValues(){
    disableRollRequirement = _reader->readSettingsValue("Engine/disableRollRequirement").toBool();
    lastEngineStartDate = _reader->readSettingsValue("Engine/lastStartDate").toDate();
    if (!lastEngineStartDate.isValid())
        lastEngineStartDate = QDate::currentDate();
}

void CurrentState::updateStartDate(){
    lastEngineStartDate = QDate::currentDate();
    _reader->updateStartDate(lastEngineStartDate);
}

int CurrentState::getDaysFromStart(){
    return lastEngineStartDate.daysTo(QDate::currentDate());
}

bool CurrentState::isSettingsMode(){
    return menuMode == SettingsMode;
}
bool CurrentState::isDiagMode(){
    return menuMode == DiagMode;
}
bool CurrentState::isDiagOrSettingsMode(){
    return menuMode == DiagMode || menuMode == SettingsMode;
}
