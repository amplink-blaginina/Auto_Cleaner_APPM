#include "cancontroller.h"
#include "mycan.h"
#include "qvariant.h"

CanController::CanController(MyCan *can0, IoBus *io){
    _can0 = can0;
    _io = io;}

void CanController::setStarterAvailable(bool state){
    _io->set(StateStarterAllow, state);}

bool CanController::getIgnition(){
    return _io->get(StateIgnitionOut).toBool();}

void CanController::invertIgnition(){
    invertState(StateIgnitionOut);}

void CanController::setIgnition(bool state){
    _io->set(StateIgnitionOut, state);}

void CanController::setRollStarter(bool state){
    _io->set(StateStarterRoll, state);}

bool CanController::getRollIn(){
    return _io->get(StateRollIn).toBool();}

bool CanController::getHeatState(){
    return _io->get(StateHeatRele).toBool();}

bool CanController::getOilRele(){
    return _io->get(StateOilRele).toBool();}

bool CanController::getAlarm(){
    return _io->get(StateAlarmIn).toBool();}

bool CanController::isDisabled(){
    return !_io->get(Board0IN1).toBool() || _io->get(StatePVIPowerIn).toBool();}

bool CanController::isBoard0IN(){
    return _io->get(Board0IN1).toBool();}

bool CanController::getSensorPower(){
    return _io->get(StateSensorsPower).toBool();}

bool CanController::getHydraulicFan(){
    return _io->get(StateHydraulicFan).toBool();}

uint CanController::getOilTmp(){
    return _io->get(StateHydraulicOilTemperature).toUInt();}

QVariant CanController::getOriginalState(quint8 board_, quint8 channel_){
    return _can0->getOriginalState(board_, channel_);}

void CanController::setOriginalState(quint8 board_, quint8 channel_, int value){
    _can0->setOriginalState(board_, channel_, value);
}
bool CanController::getState(DeviceStates key){
    return _io->get(key).toBool();}

int CanController::getInt(DeviceStates key){
    return _io->get(key).toInt();}

void CanController::setState(DeviceStates key, bool value){
    return _io->set(key, value);}

void CanController::invertState(DeviceStates key){
    setState(key, !getState(key));
}

bool CanController::isConfigured(){
    return _can0->isConfigured();
}

template<typename T>
T CanController:: getParam(const DeviceStates key, const T &defaultValue) const
{
    QVariant v = _io->get(key);
    if (!v.isValid())
        return defaultValue;
    return v.value<T>();   // QVariant сам приведёт к bool, int, double, QString...
}


void CanController::fillSystemConfig(){

}

