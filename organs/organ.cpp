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

void Organ::clearSelection(){
    if (!_left && !_right)
        return;
    _left = false;
    _right = false;
    emit selectionChanged();
}

void Organ::followActualSide(bool onLeft){
    // только ручное управление разложенным органом во время уборки: пока орган дома или его движет автомат
    // (опускание, уборка домой через середину), выбор не трогаем
    if (!_context->isCleaning() || isHome() || isTransitioning() || !isSideSelected() || onLeft == _left)
        return;
    setSide(!onLeft);
    logger->addLog(_name + (onLeft ? ": сторона сменилась на левую" : ": сторона сменилась на правую"));
}

void Organ::progressLoop(){
    // работать можно только во время уборки и если орган есть в текущем режиме
    // (до beforeStep: в нём орган уже должен знать, началась ли уборка)
    sequence.setAble(_context->isCleaning() && _available ? sequence.topState() : 0);
    beforeStep();
    sequence.tick();
    afterStep();
}
