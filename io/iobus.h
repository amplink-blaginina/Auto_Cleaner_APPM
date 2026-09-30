#ifndef IOBUS_H
#define IOBUS_H

#include <QVariant>

#include <can/mycan.h>

// Сигналы машины (выходы, дискретные и аналоговые входы) по логическому имени.
// Логика (органы, контроллеры) работает только через этот интерфейс и не знает,
// откуда берётся сигнал: платы БУЦ по CAN или симуляция.
class IoBus
{
public:
    virtual ~IoBus() = default;

    virtual QVariant get(DeviceStates signal) = 0;
    virtual void set(DeviceStates signal, const QVariant &value) = 0;
    // есть связь с блоками управления (данные входов актуальны)
    virtual bool isOnline() = 0;
};

#endif // IOBUS_H
