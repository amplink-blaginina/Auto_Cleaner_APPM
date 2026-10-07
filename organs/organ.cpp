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

void Organ::clearSelection(){
    if (!_left && !_right)
        return;
    _left = false;
    _right = false;
    emit selectionChanged();
}

void Organ::followActualSide(bool onLeft){
    // выбор переходит, только когда при ручном повороте разложенного органа во время уборки оценка положения
    // пересекла середину. Пока орган дома или его движет автомат (опускание, уборка домой через середину),
    // выбор не трогаем. Расхождение оценки с выбором без пересечения (датчик перепутан или не работает,
    // оценка «застряла» у одного упора) выбор тоже не меняет
    const bool crossed = _lastOnLeft >= 0 && onLeft != (_lastOnLeft == 1);
    _lastOnLeft = onLeft ? 1 : 0;
    if (isHome())
        _manualSideMove = false;
    if (!crossed || !_manualSideMove || !_context->isCleaning() || isHome() || isTransitioning()
        || !isSideSelected() || onLeft == _left)
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
    // работать можно только во время уборки и если орган есть в текущем режиме
    // (до beforeStep: в нём орган уже должен знать, началась ли уборка)
    sequence.setAble(_context->isCleaning() && _available ? sequence.topState() : 0);
    beforeStep();
    sequence.tick();
    afterStep();
}
