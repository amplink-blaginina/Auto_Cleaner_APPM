#include "currentstate.h"
#include "qdebug.h"
#include "settingsreader.h"
#include <qdatetime.h>
#include <Controllers/gpiocontroller.h>

CurrentState::CurrentState(SettingsReader *reader, GPIOController *gpio, CanController *can0) {
    _reader = reader;
    _gpio = gpio;
    _can = can0;
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

    resetVehicleValues();
}

void CurrentState::resetVehicleValues(){
    engineCoolantTemp = -40;
    vehicleSpeed = 0;
    vehicleVoltage = 0;
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

void CurrentState::setCoolantTmp(int value){
    engineCoolantTemp = value;
}

void CurrentState::setVehicleSpeed(int value){
    vehicleSpeed = value;
}

void CurrentState::setVehicleVoltage(float value){
    vehicleVoltage = value;
}

void CurrentState::readValues(){
    if(_reader->isSettingsContainsValue("Engine/lastStartDate")){
        qDebug()<<"!!! has settings ";
    }
    else{
        qDebug()<<"!!! no settings ";
    }

   // if(!_reader->isSettingsContainsValue("Engine/lastStartDate")){
        disableRollRequirement = _reader->readSettingsValue("Engine/disableRollRequirement").toBool();
        //qDebug()<<"!!! readStartDate1: "<<lastEngineStartDate;
        lastEngineStartDate = _reader->readSettingsValue("Engine/lastStartDate").toDate();
        qDebug()<<"!!! readStartDate: "<<lastEngineStartDate;
   // }

    // if (!lastEngineStartDate.isValid())
    //     lastEngineStartDate = QDate::currentDate();
}

void CurrentState::updateStartDate(){
    qDebug()<<"!!! updateStartDate: "<<lastEngineStartDate;
    lastEngineStartDate = QDate::currentDate();
    _reader->updateStartDate(lastEngineStartDate);
}

int CurrentState::getDaysFromStart(){
    // if (!lastEngineStartDate.isValid()){
    //     return 100;
    // }
    //qDebug()<<"!!! LastStartDate  "<<lastEngineStartDate;
    //qDebug()<<"!!! LastStartDate  "<<lastEngineStartDate.daysTo(QDate::currentDate());
    return lastEngineStartDate.daysTo(QDate::currentDate());
}

bool CurrentState::isSettingsMode(){
    return menuMode == SettingsMode;
}

bool CurrentState::isDiagMode(){
    return menuMode == DiagMode;
}

bool CurrentState::isDiagOrSettingsMode(){
    return isDiagMode() || isSettingsMode();
}

bool CurrentState::isIgnitionEnabled(){
    return !_can->isDisabled();
}

bool CurrentState:: isAlarm(){
    waterAlarm = _can->getState(StateWaterSensor) && waterSensorEmergencyMode;
    airAlarm = _can->getState(StateAirFilterBad) && airFilterEmergencyMode;
    oilAlarm = _can->getState(StateOilFilterBad);
    return waterAlarm || airAlarm || oilAlarm;
}
