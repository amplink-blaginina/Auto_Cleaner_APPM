#include "cancontroller.h"
#include "mycan.h"
#include "qvariant.h"

CanController::CanController(MyCan *can0){
    _can0 = can0;
}

void CanController::setStarterAvailable(bool state){
    _can0->setState(StateStarterAllow, state);
}

void CanController::setIgnition(bool state){
    _can0->setState(StateIgnitionOut, state);
}

bool CanController::getIgnition(){
    _can0->getState(StateIgnitionOut).toBool();
}

void CanController::setRollStarter(bool state){
    _can0->setState(StateStarterRoll, state);
}

bool CanController::getRollIn(){
    return _can0->getState(StateRollIn).toBool();
}

bool CanController::getHeatState(){
    return _can0->getState(StateHeatRele).toBool();
}

bool CanController::getOilRele(){
    return _can0->getState(StateOilRele).toBool();
}
int CanController::getHydraOilTmp(){
    return _can0->getState(StateHydraulicOilTemperature).toUInt();
}
bool CanController::getAlarm(){
    return _can0->getState(StateAlarmIn).toBool();
}
bool CanController::isDisabled(){
    return !_can0->getState(Board0IN1).toBool() || _can0->getState(StatePVIPowerIn).toBool();}

bool CanController::isBoard0IN(){
    return _can0->getState(Board0IN1).toBool();
}
bool CanController::getState(DeviceStates state){
    return _can0->getState(state).toBool();
}

void CanController::fillSystemConfig(){

}

void CanController::invertIgnition(){
    _can0->setState(StateIgnitionOut, !_can0->getState(StateIgnitionOut).toBool());
}
