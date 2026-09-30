#include "organ.h"

#include <Controllers/viewcontroller.h>

Organ::Organ(const QString &name, const MachineIo &machine, MachineContext *context, ViewController *logger_, QObject *parent)
    : QObject(parent),
      io(machine.io),
      hydraulics(machine.hydraulics),
      engineRpm(machine.engineRpm),
      logger(logger_),
      _context(context),
      sequence(name, logger_),
      _name(name)
{
    connect(&progressTimer, &QTimer::timeout, this, &Organ::progressLoop);
    progressTimer.start(100);
}

void Organ::toggleSide(bool right){
    const bool wasSelected = right ? _right : _left;
    _left = !right && !wasSelected;
    _right = right && !wasSelected;
    emit selectionChanged();
}

void Organ::setSide(bool right){
    _left = !right;
    _right = right;
    emit selectionChanged();
}

void Organ::followActualSide(bool onLeft){
    // только ручное управление во время уборки: пока орган движет автомат (в том числе уборка домой
    // через середину), выбор не трогаем
    if (!_context->isCleaning() || isTransitioning() || !isSideSelected() || onLeft == _left)
        return;
    setSide(!onLeft);
    logger->addLog(_name + (onLeft ? ": сторона сменилась на левую" : ": сторона сменилась на правую"));
}

void Organ::progressLoop(){
    beforeStep();
    // работать можно только во время уборки
    sequence.setAble(_context->isCleaning() ? sequence.topState() : 0);
    sequence.tick();
    afterStep();
}
