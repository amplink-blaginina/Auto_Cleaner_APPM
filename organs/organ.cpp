#include "organ.h"

Organ::Organ(const QString &name, const MachineIo &machine, MachineContext *context, ViewController *logger_, QObject *parent)
    : QObject(parent),
      io(machine.io),
      hydraulics(machine.hydraulics),
      engineRpm(machine.engineRpm),
      logger(logger_),
      _context(context),
      sequence(name, logger_)
{
    connect(&progressTimer, &QTimer::timeout, this, &Organ::progressLoop);
    progressTimer.start(100);
}

void Organ::progressLoop(){
    beforeStep();
    // работать можно только во время уборки
    sequence.setAble(_context->isCleaning() ? sequence.topState() : 0);
    sequence.tick();
    afterStep();
}
