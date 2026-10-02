#include "globalsettings.h"
#include "qdebug.h"

#include <qdatetime.h>

GlobalSettings::GlobalSettings(SettingsReader *reader){
    _reader = reader;
    // адрес двигателя нужен сразу: MainWindow передает его в MyCanEngine::setEngineAddr() до readValues().
    // Раньше поле было не инициализировано, и TSC1 уходил со случайным SA (например 0x18000069 вместо 0x0C000003)
    enigneAddr = _reader->readSettingsValue("Engine/addr").toInt();
}

void GlobalSettings::setDefaults(){
    starterMaxWorkSec = 15;
    starterPauseSec = 60;
    starterMaxAttempts = 3;
    rollMaxWorkSec = 15;
    rollPauseSec = 60;
    rollMaxAttempts = 3;
    requireRollAfterDays = 5;
    lowTempRequireWarm = -10;
    waterSensorRedHours = 2;
    airFilterRedHours = 20;
}

void GlobalSettings::readValues(){
    rpmNone = readSettingsValue("Engine/rpm.None").toInt();// холостой ход
    enigneAddr = readSettingsValue("Engine/addr").toInt();// адрес двигателя
    enableCleanSpeed = _reader->readSettingsValue("Global/enableCleanSpeed").toInt();// пороги скорости
    disableCleanSpeed = _reader->readSettingsValue("Global/disableCleanSpeed").toInt();
    ventEdge = _reader->readSettingsValue("Engine/rpm.VentEdge").toInt();// охлаждение двигателя
    requireRollAfterDays = readSettingsValue("Engine/startRollRequiredDays").toInt();
    lowTempRequireWarm = readSettingsValue("Engine/startLowTemperatureEdge").toInt();
    starterMaxWorkSec = readSettingsValue("Engine/starterMaxWorkSec").toInt();
    starterPauseSec = readSettingsValue("Engine/starterPauseSec").toInt();
    starterMaxAttempts = readSettingsValue("Engine/starterMaxAttempts").toInt();
    rollMaxWorkSec = readSettingsValue("Engine/rollMaxWorkSec").toInt();
    rollPauseSec = readSettingsValue("Engine/rollPauseSec").toInt();
    rollMaxAttempts = readSettingsValue("Engine/rollMaxAttempts").toInt();
    waterSensorRedHours = readSettingsValue("Engine/waterSensorRedHours").toInt();
    airFilterRedHours = readSettingsValue("Engine/airFilterRedHours").toInt();
    restartIgnitionDelay = readSettingsValue("Global/restartIgnitionDelay").toInt();
}

QVariant GlobalSettings::readSettingsValue(QString key){
    return _reader->readSettingsValue(key);}

bool GlobalSettings::isNeedRoolByDate(int value) const{
    return value > requireRollAfterDays;}

bool GlobalSettings::isEngineCold(int value) const{
    return value < lowTempRequireWarm;}

bool GlobalSettings::rollWorkingLimitReached(QDateTime startMoment) const{
    const int rollRunTime = qAbs(startMoment.secsTo(QDateTime::currentDateTime()));
    return rollRunTime >= rollMaxWorkSec;
}

bool GlobalSettings::rollAttemptsLimitReached(int used) const{
    return used >= starterMaxAttempts;}

bool GlobalSettings::starterWorkingLimitReached(QDateTime startMoment) const{
    const int starterRunTime = qAbs(startMoment.secsTo(QDateTime::currentDateTime()));
    return starterRunTime >= starterMaxWorkSec;
}

bool GlobalSettings::starterAttemptsLimitReached(int used) const{
    return used >= starterMaxAttempts;}

bool GlobalSettings::checkStarterPause(QDateTime startMoment) const{
    const int passed = qAbs(startMoment.secsTo(QDateTime::currentDateTime()));
    return passed < starterPauseSec;
}

bool GlobalSettings::checkRollPause(QDateTime startMoment) const{
    const int passed = qAbs(startMoment.secsTo(QDateTime::currentDateTime()));
    return passed < rollPauseSec;
}

int GlobalSettings::starterPauseSecondsLeft(QDateTime startMoment) const{
    const int passed = qAbs(startMoment.secsTo(QDateTime::currentDateTime()));
    return qMax(0, starterPauseSec - passed);
}

int GlobalSettings::rollPauseSecondsLeft(QDateTime startMoment) const{
    const int passed = qAbs(startMoment.secsTo(QDateTime::currentDateTime()));
    return qMax(0, rollPauseSec - passed);
}

int GlobalSettings::getRollAttempts() const{
    return rollMaxAttempts;}

int GlobalSettings::getRpm() const{
    return rpmNone * 8;}

