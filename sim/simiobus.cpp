#include "simiobus.h"

#include <QtMath>

namespace {
const double travelSec = 2.5;// время хода органа от края до края в модели
}

SimIoBus::SimIoBus(QObject *parent)
    : QObject(parent)
{
    // пульт на месте (иначе CanController::isDisabled())
    _values.insert(Board0IN1, true);

    // модель машины АППМ: какие клапаны двигают органы и где стоят концевики.
    // 0 - верхнее (исходное) положение или левый край, 1 - нижнее положение или правый край
    addAxis({"Магнит", {StateValveE2}, {StateValveE6}, travelSec, 0,
             {StateDKPBackMagnetUp}, {}, "верх", "низ"});
    addAxis({"Отвал: подъём", {StateValveF1}, {StateValveF7}, travelSec, 0,
             {StateDKPDumpUp}, {}, "верх", "низ"});
    addAxis({"Отвал: поворот", {StateValveF12}, {StateValveF6}, travelSec, 1,
             {StateDKPDumpLeft}, {StateDKPDumpRight}, "лево", "право"});
    addAxis({"Щётка: подъём", {StateValveF2, StateValveF10}, {StateValveF8, StateValveF4}, travelSec, 0,
             {StateDKPBroomUp}, {}, "верх", "низ"});
    addAxis({"Щётка: поворот", {StateValveF9}, {StateValveF3}, travelSec, 1,
             {StateDKPBroomLeft}, {StateDKPBroomRight}, "лево", "право"});
    addAxis({"Обдув: подъём", {StateValveE1}, {StateValveE5}, travelSec, 0,
             {StateDKPBlowerUp1, StateDKPBlowerUp2}, {}, "верх", "низ"});
    addAxis({"Обдув: поворот", {StateValveE3}, {StateValveE7}, travelSec, 1,
             {}, {}, "лево", "право"});

    _clock.start();
    connect(&_timer, &QTimer::timeout, this, &SimIoBus::step);
    _timer.start(50);
}

QVariant SimIoBus::get(DeviceStates signal){
    return _values.value(signal, 0);
}

void SimIoBus::set(DeviceStates signal, const QVariant &value){
    _values.insert(signal, value);
}

bool SimIoBus::isOnline(){
    return true;
}

QVariant SimIoBus::value(DeviceStates signal) const{
    return _values.value(signal, 0);
}

void SimIoBus::setInput(DeviceStates signal, const QVariant &value){
    _values.insert(signal, value);
}

const QList<SimAxis> &SimIoBus::axes() const{
    return _axes;
}

bool SimIoBus::isModelSensor(DeviceStates signal) const{
    for (const SimAxis &axis : _axes)
        if (axis.minSensors.contains(signal) || axis.maxSensors.contains(signal))
            return true;
    return false;
}

void SimIoBus::setModelEnabled(bool enabled){
    _modelEnabled = enabled;
}

bool SimIoBus::isModelEnabled() const{
    return _modelEnabled;
}

void SimIoBus::addAxis(const SimAxis &axis){
    _axes.append(axis);
    updateSensors(axis);
}

bool SimIoBus::anyOn(const QList<DeviceStates> &outputs) const{
    for (DeviceStates output : outputs)
        if (_values.value(output, 0).toBool())
            return true;
    return false;
}

void SimIoBus::updateSensors(const SimAxis &axis){
    const double eps = 0.01;
    for (DeviceStates sensor : axis.minSensors)
        _values.insert(sensor, axis.pos <= eps);
    for (DeviceStates sensor : axis.maxSensors)
        _values.insert(sensor, axis.pos >= 1 - eps);
}

void SimIoBus::step(){
    const double dt = _clock.restart() / 1000.0;
    const bool pressure = _values.value(StateValveA1, 0).toBool();
    for (SimAxis &axis : _axes){
        const bool toMin = anyOn(axis.toMin);
        const bool toMax = anyOn(axis.toMax);
        if (pressure && toMin != toMax){
            const double delta = dt / axis.travelSec;
            axis.pos = qBound(0.0, axis.pos + (toMax ? delta : -delta), 1.0);
        }
        if (_modelEnabled)
            updateSensors(axis);
    }
}
