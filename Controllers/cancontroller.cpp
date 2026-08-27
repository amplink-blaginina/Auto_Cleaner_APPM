#include "cancontroller.h"
#include "mycan.h"
#include "qvariant.h"

CanController::CanController(MyCan *can0){
    _can0 = can0;}

void CanController::setStarterAvailable(bool state){
    _can0->setState(StateStarterAllow, state);}

bool CanController::getIgnition(){
    return _can0->getState(StateIgnitionOut).toBool();}

void CanController::invertIgnition(){
    invertState(StateIgnitionOut);}

void CanController::setIgnition(bool state){
    _can0->setState(StateIgnitionOut, state);}

void CanController::setRollStarter(bool state){
    _can0->setState(StateStarterRoll, state);}

bool CanController::getRollIn(){
    return _can0->getState(StateRollIn).toBool();}

bool CanController::getHeatState(){
    return _can0->getState(StateHeatRele).toBool();}

bool CanController::getOilRele(){
    return _can0->getState(StateOilRele).toBool();}

int CanController::getHydraOilTmp(){
    return _can0->getState(StateHydraulicOilTemperature).toUInt();}

bool CanController::getAlarm(){
    return _can0->getState(StateAlarmIn).toBool();}

bool CanController::isDisabled(){
    return !_can0->getState(Board0IN1).toBool() || _can0->getState(StatePVIPowerIn).toBool();}

bool CanController::isBoard0IN(){
    return _can0->getState(Board0IN1).toBool();}

bool CanController::getSensorPower(){
    return _can0->getState(StateSensorsPower).toBool();}

bool CanController::getHydraulicFan(){
    return _can0->getState(StateHydraulicFan).toBool();}

uint CanController::getOilTmp(){
    return _can0->getState(StateHydraulicOilTemperature).toUInt();
}
QVariant CanController::getOriginalState(quint8 board_, quint8 channel_){
    return _can0->getOriginalState(board_, channel_);
}
void CanController::setOriginalState(quint8 board_, quint8 channel_, int value){
    _can0->setOriginalState(board_, channel_, value);
}
bool CanController::getState(DeviceStates key){
    return _can0->getState(key).toBool();}

int CanController::getInt(DeviceStates key){
    return _can0->getState(key).toInt();}

void CanController::setState(DeviceStates key, bool value){
    return _can0->setState(key, value);}

void CanController::invertState(DeviceStates key){
    setState(key, !getState(key));
}

bool CanController::isConfigured(){
    return _can0->isConfigured();
}

template<typename T>
T CanController:: getParam(const DeviceStates key, const T &defaultValue) const
{
    QVariant v = _can0->getState(key);
    if (!v.isValid())
        return defaultValue;
    return v.value<T>();   // QVariant сам приведёт к bool, int, double, QString...
}


void CanController::fillSystemConfig(){

}

