#include "organ.h"

#include <Controllers/viewcontroller.h>
#include <settingsreader.h>

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
    // только ручное управление разложенным органом во время уборки: пока орган дома или его движет автомат
    // (опускание, уборка домой через середину), выбор не трогаем
    if (!_context->isCleaning() || isHome() || isTransitioning() || !isSideSelected() || onLeft == _left)
        return;
    setSide(!onLeft);
    logger->addLog(_name + (onLeft ? ": сторона сменилась на левую" : ": сторона сменилась на правую"));
}

float Organ::travelTimeSetting(const QString &key, float fallback){
    bool ok = false;
    const float value = _context->settingsReader()->readSettingsValue(key).toFloat(&ok);
    if (ok && value > 0)
        return value;
    logger->addLogWarning("Настройка " + key + " задана неверно, используется " + QString::number(fallback) + " с");
    return fallback;
}

void Organ::progressLoop(){
    // работать можно только во время уборки (до beforeStep: в нём орган уже должен знать, началась ли уборка)
    sequence.setAble(_context->isCleaning() ? sequence.topState() : 0);
    beforeStep();
    sequence.tick();
    afterStep();
}
