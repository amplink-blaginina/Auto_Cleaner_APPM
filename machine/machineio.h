#ifndef MACHINEIO_H
#define MACHINEIO_H

#include "io/iobus.h"
#include "machine/enginerpmdemand.h"
#include "machine/hydraulicsupply.h"

// Всё, через что орган управляет машиной: сигналы и общие ресурсы
struct MachineIo
{
    IoBus *io;
    HydraulicSupply *hydraulics;
    EngineRpmDemand *engineRpm;
};

#endif // MACHINEIO_H
