#include "caniobus.h"

CanIoBus::CanIoBus(MyCan *can)
    : _can(can)
{
}

QVariant CanIoBus::get(DeviceStates signal){
    return _can->getState(signal);
}

void CanIoBus::set(DeviceStates signal, const QVariant &value){
    _can->setState(signal, value);
}

bool CanIoBus::isOnline(){
    return _can->isActive();
}
