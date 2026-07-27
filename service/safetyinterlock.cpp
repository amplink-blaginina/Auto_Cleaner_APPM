#include "safetyinterlock.h"

//SafetyInterlock::SafetyInterlock(QObject *parent_) : QObject(parent_) {

//}
bool SafetyInterlock::canStart() const{

}   // можно ли включить стартер?
bool SafetyInterlock::canRoll()  const{

}  // можно ли прокрутку?
SafetyInterlock::BlockReason SafetyInterlock::reason() const{

}
QString SafetyInterlock::reasonText() const{

}

void SafetyInterlock::setEngineTemp(int temp, bool valid){

} // от J1939
void SafetyInterlock::setHeatRelay(bool active){

}           // от GPIO
void SafetyInterlock::setWaterSensor(bool bad){

}             // от CAN
void SafetyInterlock::setAirFilter(bool bad){

}
void SafetyInterlock::setOilFilter(bool bad){

}
void SafetyInterlock::setRollCompleted(bool ok){

}            // прокрутка выполнена
void SafetyInterlock::setIgnition(bool on){

}                // зажигание
