#ifndef HYDRAULICSUPPLY_H
#define HYDRAULICSUPPLY_H

#include <QSet>

#include "io/iobus.h"

// Силовой клапан A1 - общий для всех гидравлических органов.
// Каждый орган заявляет, нужна ли ему гидравлика; клапан открыт, пока нужна хоть одному.
// (раньше каждый орган переключал A1 сам и мог закрыть его посреди движения другого органа)
class HydraulicSupply
{
public:
    explicit HydraulicSupply(IoBus *io);

    void request(const void *owner, bool need);
    // выключить для всех (простой: уборка не идёт, органы в исходном положении)
    void forceOff();
    bool isOn() const;

private:
    void apply();

    IoBus *_io;
    QSet<const void *> _owners;
};

#endif // HYDRAULICSUPPLY_H
