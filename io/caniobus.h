#ifndef CANIOBUS_H
#define CANIOBUS_H

#include "io/iobus.h"

// Сигналы через платы БУЦ по CAN (сопоставление сигнал -> плата/канал ведёт MyCan)
class CanIoBus : public IoBus
{
public:
    explicit CanIoBus(MyCan *can);

    QVariant get(DeviceStates signal) override;
    void set(DeviceStates signal, const QVariant &value) override;
    bool isOnline() override;

private:
    MyCan *_can;
};

#endif // CANIOBUS_H
