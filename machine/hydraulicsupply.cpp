#include "hydraulicsupply.h"

HydraulicSupply::HydraulicSupply(IoBus *io)
    : _io(io)
{
}

void HydraulicSupply::request(const void *owner, bool need){
    if (need)
        _owners.insert(owner);
    else
        _owners.remove(owner);
    apply();
}

void HydraulicSupply::forceOff(){
    _owners.clear();
    apply();
}

bool HydraulicSupply::isOn() const{
    return !_owners.isEmpty();
}

void HydraulicSupply::apply(){
    _io->set(StateValveA1, isOn());
}
