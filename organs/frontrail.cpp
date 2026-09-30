#include "frontrail.h"

#include <settingsreader.h>
#include <Controllers/viewcontroller.h>

#include <QDebug>
#include <QMetaEnum>

FrontRail::FrontRail(const MachineIo &machine, MachineContext *context, ViewController *logger_, QObject *parent)
    : Organ("Отвал", machine, context, logger_, parent)
{
    readSettings();
    auto timeout = [this](FrontRailStates s){ return [this, s]{ return timeouts.value(s, 0); }; };

    OrganSequence::Step slide;// поворот в выбранную сторону
    slide.out = {{"поворот", "", [this]{ setDirection(_left ? organsEnums::Left : organsEnums::Right); },
                  [this]{ return io->get(_left ? StateDKPDumpLeft : StateDKPDumpRight).toBool(); },
                  timeout(FrontRailSlideOut)}};
    slide.in = {{"возврат поворота", "", [this]{ setDirection(organsEnums::Right); },
                 [this]{ return io->get(StateDKPDumpRight).toBool(); },
                 timeout(FrontRailSlideIn)}};
    sequence.addStep(slide);

    OrganSequence::Step bounce;// отскок от упора - поворот в противоположную сторону; назад ничего не делаем
    bounce.out = {{"отскок", "Отвал: отскок", [this]{ setDirection(_left ? organsEnums::Right : organsEnums::Left); },
                   nullptr, timeout(FrontRailBounceOut),
                   [this]{ return timeouts.value(FrontRailBounceOut, 0) <= 0; }}};
    sequence.addStep(bounce);

    OrganSequence::Step down;// вниз концевика нет - по времени, вверх до концевика
    down.out = {{"опускание", "", [this]{ setDirection(organsEnums::Down); }, nullptr, timeout(FrontRailDownOut)}};
    down.in = {{"подъём", "", [this]{ setDirection(organsEnums::Up); },
                [this]{ return io->get(StateDKPDumpUp).toBool(); },
                timeout(FrontRailDownIn)}};
    sequence.addStep(down);

    OrganSequence::Step flow;// плавание на время опускания на поверхность, потом - по выбору оператора
    flow.out = {{"плавание", "", [this]{ setFlowActive(true); }, nullptr, timeout(FrontRailFlowOut)}};
    flow.in = {{"плавание", "", [this]{ setFlowActive(false); }}};
    flow.done = [this]{ setFlowActive(_flowSelected); };
    sequence.addStep(flow);

    sequence.setHalt([this]{ setDirection(organsEnums::None); });
    sequence.setOnChange([this](int s){ qDebug() << "Front rail state " << toString(FrontRailStates(s)); });
    goHome();
}

void FrontRail::readSettings(){
    timeouts.clear();

    auto reader = _context->settingsReader();
    // назначаем таймауты на длительные операции
    timeouts.insert(FrontRailSlideOut, reader->readSettingsValue("Dump/timeouts.DumpSlideOut").toFloat());
    timeouts.insert(FrontRailSlideIn, reader->readSettingsValue("Dump/timeouts.DumpSlideIn").toFloat());
    timeouts.insert(FrontRailBounceOut, reader->readSettingsValue("Dump/timeouts.DumpBounceOut").toFloat());
    timeouts.insert(FrontRailDownOut, reader->readSettingsValue("Dump/timeouts.DumpDownOut").toFloat());
    timeouts.insert(FrontRailDownIn, reader->readSettingsValue("Dump/timeouts.DumpDownIn").toFloat());
    timeouts.insert(FrontRailFlowOut, reader->readSettingsValue("Dump/timeouts.DumpFlowOut").toFloat());
    side.setTravelSec(travelTimeSetting("Dump/slideTimeSec", 5));
}

QString FrontRail::toString(FrontRailStates s){
    const char *key = QMetaEnum::fromType<FrontRailStates>().valueToKey(s);
    return key ? QString::fromLatin1(key) : QStringLiteral("UnknownState");
}

void FrontRail::setDirection(organsEnums::Direction dir){
    if(dir == direction)
        return;

    switch (direction) {
    case organsEnums::Up:
        goUp(false);
        break;
    case organsEnums::Down:
        goDown(false);
        break;
    case organsEnums::Left:
        goLeft(false);
        break;
    case organsEnums::Right:
        goRight(false);
        break;
    default:
        break;
    }
    direction = dir;

    switch (dir) {
    case organsEnums::Up:
        goUp(true);
        break;
    case organsEnums::Down:
        goDown(true);
        break;
    case organsEnums::Left:
        goLeft(true);
        break;
    case organsEnums::Right:
        goRight(true);
        break;
    default:
        break;
    }
}

void FrontRail::goRight(bool state){
    io->set(StateValveF6, state);
    hydraulics->request(this, state);
    printMovement(organsEnums::Right, state);
}

void FrontRail::goLeft(bool state){
    io->set(StateValveF12, state);
    hydraulics->request(this, state);
    printMovement(organsEnums::Left, state);
}

void FrontRail::goDown(bool state){
    if(state){
        emit flowCancelRequested();
    }
    io->set(StateValveF7, state);
    hydraulics->request(this, state);
    printMovement(organsEnums::Down, state);
}

void FrontRail::goUp(bool state){
    if(state){
        emit flowCancelRequested();
    }
    io->set(StateValveF1, state);
    hydraulics->request(this, state);
}

void FrontRail::goFlow(bool state){
    logger->addLog(state?"Отвал: плавание активировано":"Отвал: плавание деактивировано");
    io->set(StateValveC3, state);
    io->set(StateValveC4, state);
}

void FrontRail::selectFlow(bool selected){
    if (_flowSelected == selected)
        return;
    _flowSelected = selected;
    emit selectionChanged();
}

void FrontRail::setFlowActive(bool state){
    if(isFlowing == state)
        return;
    isFlowing = state;
    goFlow(state);
}

void FrontRail::beforeStep(){
    const bool pressure = hydraulics->isOn();
    side.update(pressure && direction == organsEnums::Left, pressure && direction == organsEnums::Right,
                io->get(StateDKPDumpLeft).toBool(), io->get(StateDKPDumpRight).toBool());
    followActualSide(side.isLeft());
}

void FrontRail::printMovement(organsEnums::Direction dir, bool state){
    logger->printMovementLog(organsEnums::Dump,
                             dir,
                             state?"": " завершено");
}
