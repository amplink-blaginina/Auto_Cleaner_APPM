#ifndef SIMIOBUS_H
#define SIMIOBUS_H

#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QTimer>

#include "io/iobus.h"

// Ось движения органа в модели машины: положение 0..1 между двумя крайними точками.
// Двигается, пока открыт силовой клапан A1 и включён клапан одного из направлений.
struct SimAxis
{
    QString name;
    QList<DeviceStates> toMin;      // клапаны движения к положению 0
    QList<DeviceStates> toMax;      // клапаны движения к положению 1
    double travelSec;               // время хода от края до края
    double pos;                     // текущее положение
    QList<DeviceStates> minSensors; // концевики в положении 0
    QList<DeviceStates> maxSensors; // концевики в положении 1
    QString minName;
    QString maxName;
};

// Симуляция сигналов машины для отладки без техники (ключ запуска --sim).
// Выходы запоминаются, входы задаются на панели симуляции,
// концевики положения выставляет модель машины (её можно выключить - как будто датчиков нет).
class SimIoBus : public QObject, public IoBus
{
    Q_OBJECT
public:
    explicit SimIoBus(QObject *parent = nullptr);

    QVariant get(DeviceStates signal) override;
    void set(DeviceStates signal, const QVariant &value) override;
    bool isOnline() override;

    // значение для панели (без побочных эффектов)
    QVariant value(DeviceStates signal) const;
    // вход, заданный на панели
    void setInput(DeviceStates signal, const QVariant &value);

    const QList<SimAxis> &axes() const;
    bool isModelSensor(DeviceStates signal) const;
    void setModelEnabled(bool enabled);
    bool isModelEnabled() const;

private slots:
    void step();

private:
    void addAxis(const SimAxis &axis);
    bool anyOn(const QList<DeviceStates> &outputs) const;
    void updateSensors(const SimAxis &axis);

    QHash<int, QVariant> _values;
    QList<SimAxis> _axes;
    QTimer _timer;
    QElapsedTimer _clock;
    bool _modelEnabled = true;
};

#endif // SIMIOBUS_H
