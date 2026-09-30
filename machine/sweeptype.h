#ifndef SWEEPTYPE_H
#define SWEEPTYPE_H

// Тип смёта: по нему органы берут из настроек обороты двигателя и скорость вращения
enum SweepType
{
    NoneSweep   = 0,
    LeafSweep   = 1,
    LightSweep  = 2,
    MediumSweep = 3,
    HeavySweep  = 4
};

#endif // SWEEPTYPE_H
