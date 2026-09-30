#include "simiobus.h"

#include <QtMath>

namespace {
const double travelSec = 2.5;// время хода органа от края до края в модели
const double spinUpSec = 1.5;// разгон вращения от 0 до максимума в модели
const double fallSec = 3;// в плавании орган опускается под собственным весом
}

double SimRotor::command() const{
    return qBound(0.0, values->value(output, 0).toDouble() * percentPerUnit, 100.0);
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
             {StateDKPDumpUp}, {}, "верх", "низ", false, {StateValveC3, StateValveC4}, fallSec});
    addAxis({"Отвал: поворот", {StateValveF12}, {StateValveF6}, travelSec, 1,
             {StateDKPDumpLeft}, {StateDKPDumpRight}, "лево", "право", true});
    // щётка: портал поднимается и опускается целиком (F10/F4), сама щётка ходит внутри портала - прижим/отжим (F8/F2)
    addAxis({"Щётка: портал", {StateValveF10}, {StateValveF4}, travelSec, 0,
             {StateDKPBroomUp}, {}, "верх", "низ", false, {StateValveC1, StateValveC2}, fallSec});
    addAxis({"Щётка: в портале", {StateValveF2}, {StateValveF8}, travelSec, 0,
             {}, {}, "отжата", "прижата"});
    addAxis({"Щётка: поворот", {StateValveF9}, {StateValveF3}, travelSec, 1,
             {StateDKPBroomLeft}, {StateDKPBroomRight}, "лево", "право", true});
    addAxis({"Обдув: подъём", {StateValveE1}, {StateValveE5}, travelSec, 0,
             {StateDKPBlowerUp1, StateDKPBlowerUp2}, {}, "верх", "низ"});
    addAxis({"Обдув: поворот", {StateValveE3}, {StateValveE7}, travelSec, 1,
             {}, {}, "лево", "право", true});

    // вращение: щётке программа задаёт половину скорости в процентах (CentralBroom::goRotate), обдуву - проценты
    _rotors.append({"Щётка: вращение", StateValveD1, 2, spinUpSec, 0, &_values});
    _rotors.append({"Обдув: вентилятор", StateValveD3, 1, spinUpSec, 0, &_values});

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

const QList<SimRotor> &SimIoBus::rotors() const{
    return _rotors;
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
        else if (axis.fallSec > 0 && anyOn(axis.fall))// плавание: опускается под собственным весом
            axis.pos = qMin(1.0, axis.pos + dt / axis.fallSec);
        if (_modelEnabled)
            updateSensors(axis);
    }
    for (SimRotor &rotor : _rotors){
        const double target = rotor.command();
        const double delta = dt * 100 / rotor.spinUpSec;
        rotor.speed = rotor.speed < target ? qMin(target, rotor.speed + delta) : qMax(target, rotor.speed - delta);
    }
}
