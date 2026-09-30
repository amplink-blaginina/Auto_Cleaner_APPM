#include "centralbroom.h"
#include <settingsreader.h>
#include <machine/sweeptype.h>
#include <Controllers/viewcontroller.h>
#include <QDebug>
#include <QMetaEnum>

CentralBroom::CentralBroom(const MachineIo &machine, MachineContext *context, ViewController *logger_, QObject *parent)
    : Organ("Щетка", machine, context, logger_, parent)
{
    readSettings();
    auto timeout = [this](BroomStates s){ return [this, s]{ return timeouts.value(s, 0); }; };

    OrganSequence::Step slide;// поворот в выбранную сторону
    slide.out = {{"поворот", "", [this]{ goSlide(_left); },
                  [this]{ return io->get(_left ? StateDKPBroomLeft : StateDKPBroomRight).toBool(); },
                  timeout(BroomSlideOut)}};
    slide.in = {{"возврат поворота", "", [this]{ setDirection(organsEnums::Right); },
                 [this]{ return io->get(StateDKPBroomRight).toBool(); },
                 timeout(BroomSlideIn)}};
    sequence.addStep(slide);

    OrganSequence::Step bounce;// отскок от упора - поворот в противоположную сторону; назад ничего не делаем
    bounce.out = {{"отскок", "Щетка: отскок", [this]{ goSlide(!_left); }, nullptr, timeout(BroomBounceOut),
                   [this]{ return timeouts.value(BroomBounceOut, 0) <= 0; }}};
    sequence.addStep(bounce);

    // вращение включает updateRotation по состоянию, шаг только выдерживает время раскрутки/торможения
    OrganSequence::Step rotate;
    rotate.out = {{"раскрутка", "Щетка раскручивается", nullptr, nullptr, timeout(BroomRotateOut)}};
    rotate.in = {{"торможение", "Щетка останавливается", [this]{ goNoRotate(); }, nullptr, timeout(BroomRotateIn)}};
    rotate.done = [this]{// раскрутка перед опусканием или подъём закончен - портал вверху
        if (!isPressed)// с прижимом автомат двигает щётку внутри портала, а не портал
            heightEstimate = 0;
    };
    sequence.addStep(rotate);

    OrganSequence::Step down;
    down.out = {// подъём прервали - щётка уже остановлена: сначала раскручиваем, потом опускаем
                {"раскрутка", "Щетка раскручивается", nullptr, nullptr, timeout(BroomRotateOut),
                 [this]{ return spinning; }},
                {"опускание", "", [this]{ setDirection(organsEnums::Down); }, nullptr, timeout(BroomDownOut)}};
    down.in = {{"подъём", "", [this]{ setDirection(organsEnums::Up); },
                [this]{ return io->get(StateDKPBroomUp).toBool(); },
                timeout(BroomDownIn)}};
    down.done = [this]{// опускание закончено - портал внизу
        if (!isPressed)// с прижимом автомат двигает щётку внутри портала, а не портал
            heightEstimate = 1;
    };
    sequence.addStep(down);

    OrganSequence::Step flow;// плавание на время опускания на поверхность, потом - по выбору оператора
    flow.out = {{"плавание", "", [this]{ setFlowActive(true); }, nullptr, timeout(BroomFlowOut)}};
    flow.in = {{"плавание", "", [this]{ setFlowActive(false); }}};
    flow.done = [this]{ setFlowActive(_flowSelected); };
    sequence.addStep(flow);

    sequence.setHalt([this]{ goNone(); });
    sequence.setOnChange([this](int s){ qDebug() << "Central broom state " << toString(BroomStates(s)); });
    goHome();
}

void CentralBroom::readSettings(){
    timeouts.clear();
    rpmForSweepType.clear();
    speedForSweepType.clear();
    auto reader = _context->settingsReader();

    rpmForSweepType.insert(LeafSweep, reader->readSettingsValue("Engine/rpm.LeafSweep").toInt());
    rpmForSweepType.insert(LightSweep, reader->readSettingsValue("Engine/rpm.LightSweep").toInt());
    rpmForSweepType.insert(MediumSweep, reader->readSettingsValue("Engine/rpm.MediumSweep").toInt());
    rpmForSweepType.insert(HeavySweep, reader->readSettingsValue("Engine/rpm.HeavySweep").toInt());

    timeouts.insert(BroomSlideOut, reader->readSettingsValue("CentralBroom/timeouts.BroomSlideOut").toFloat());
    timeouts.insert(BroomSlideIn, reader->readSettingsValue("CentralBroom/timeouts.BroomSlideIn").toFloat());
    timeouts.insert(BroomBounceOut, reader->readSettingsValue("CentralBroom/timeouts.BroomBounceOut").toFloat());
    timeouts.insert(BroomDownOut, reader->readSettingsValue("CentralBroom/timeouts.BroomDownOut").toFloat());
    timeouts.insert(BroomDownIn, reader->readSettingsValue("CentralBroom/timeouts.BroomDownIn").toFloat());
    timeouts.insert(BroomFlowOut, reader->readSettingsValue("CentralBroom/timeouts.BroomFlowOut").toFloat());
    timeouts.insert(BroomRotateOut, reader->readSettingsValue("CentralBroom/timeouts.BroomRotateOut").toFloat());
    timeouts.insert(BroomRotateIn, reader->readSettingsValue("CentralBroom/timeouts.BroomRotateIn").toFloat());

    speedForSweepType.insert(LeafSweep, reader->readSettingsValue("CentralBroom/speeds.LeafSweep").toInt());
    speedForSweepType.insert(LightSweep, reader->readSettingsValue("CentralBroom/speeds.LightSweep").toInt());
    speedForSweepType.insert(MediumSweep, reader->readSettingsValue("CentralBroom/speeds.MediumSweep").toInt());
    speedForSweepType.insert(HeavySweep, reader->readSettingsValue("CentralBroom/speeds.HeavySweep").toInt());

    lowerTimeSec = reader->readSettingsValue("CentralBroom/lowerTimeSec").toFloat();
    raiseTimeSec = reader->readSettingsValue("CentralBroom/raiseTimeSec").toFloat();
    flowDropSec = reader->readSettingsValue("CentralBroom/flowDropTimeSec").toFloat();
    spinHeight = qBound(0, reader->readSettingsValue("CentralBroom/spinHeightPercent").toInt(), 100) / 100.0;
    side.setTravelSec(reader->readSettingsValue("CentralBroom/slideTimeSec").toFloat());
}

QString CentralBroom::toString(BroomStates s){
    const char *key = QMetaEnum::fromType<BroomStates>().valueToKey(s);
    return key ? QString::fromLatin1(key) : QStringLiteral("UnknownState");
}

void CentralBroom::goSlide(bool toLeft){
    setDirection(toLeft?organsEnums::Left:organsEnums::Right);
}

void CentralBroom::goLeft(bool state){
    printMovement(organsEnums::Left, state, isPressed);
    io->set(StateValveF9, state);
    hydraulics->request(this, state);
}
void CentralBroom::goRight(bool state){
    printMovement(organsEnums::Right, state, isPressed);
    io->set(StateValveF3, state);
    hydraulics->request(this, state);
}

void CentralBroom::goNone(){
    io->set(StateValveF9, false);
    io->set(StateValveF3, false);
    io->set(StateValveF4, false);
    io->set(StateValveF10, false);

    setDirection(organsEnums::None);
    hydraulics->request(this, false);
}

void CentralBroom::goUp(bool state, bool isPressed){
    if(isPressed){
        hydraulics->request(this, state);
        io->set(StateValveF2, state);
        printMovement(organsEnums::Up, state, isPressed);
    }
    else{
        goUpImmediate(state);
    }
}


void CentralBroom::goDown(bool state, bool isPressed){
    if(isPressed){
        io->set(StateValveF8, state);
        hydraulics->request(this, state);
        printMovement(organsEnums::Down, state, isPressed);
    }
    else{
        goDownImmediate(state);
    }
}

void CentralBroom::goUpImmediate(bool state){
    if(state){
        emit flowCancelRequested();
    }
    printMovement(organsEnums::Up, state, false);
    hydraulics->request(this, state);
    io->set(StateValveF10, state);
}

void CentralBroom::goDownImmediate(bool state){
    if(state){
        emit flowCancelRequested();
    }
    printMovement(organsEnums::Down, state, false);
    hydraulics->request(this, state);
    io->set(StateValveF4, state);
}

void CentralBroom::printMovement(organsEnums::Direction dir, bool state, bool isPressed){
    logger->printMovementLog(isPressed && (dir == organsEnums::Up||dir == organsEnums::Down)?
                                 organsEnums::Broom:
                                 organsEnums::BroomBlock,
                             dir,
                             state?"": " завершено");
}

void CentralBroom::goPressUp(bool state){
    io->set(StateValveF2, state);
    hydraulics->request(this, state);
}

void CentralBroom::goPressDown(bool state){
    io->set(StateValveF8, state);
    hydraulics->request(this, state);
}

void CentralBroom::stopPress(){
    io->set(StateValveF2, false);
    io->set(StateValveF8, false);
    hydraulics->request(this, false);
}

void CentralBroom::setDirection(organsEnums::Direction dir){
    setDirection(dir, isPressed);
}

void CentralBroom::setDirection(organsEnums::Direction dir, bool pressed){
    if(dir == direction && pressed == isPressed)
        return;
    switch (direction) {
        case organsEnums::Up:
            goUp(false, isPressed);
            break;
        case organsEnums::Down:
            goDown(false, isPressed);
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
    setPressActive(pressed);

    switch (dir) {
        case organsEnums::Up:
            goUp(true, pressed);
            break;
        case organsEnums::Down:
            goDown(true, pressed);
            break;
        case organsEnums::Left:
            goLeft(true);
            break;
        case organsEnums::Right:
            goRight(true);
            break;
        default:
            goNone();
            break;
    }
}

void CentralBroom::setPressActive(bool state){
    if(isPressed == state)
        return;
    if(direction!= organsEnums::None){
        switch (direction) {
        case organsEnums::Up:
            goUp(false, isPressed);
            goUp(true, state);
            break;
        case organsEnums::Down:
            goDown(false, isPressed);
            goDown(false, state);
            break;
        default:
            break;}
    }
    isPressed = state;
    logger->addLog(state?"Щетка: прижим активирован":"Щетка: прижим деактивирован");
}

void CentralBroom::selectFlow(bool selected){
    if (_flowSelected == selected)
        return;
    _flowSelected = selected;
    emit selectionChanged();
}

void CentralBroom::selectPress(bool selected){
    if (_pressSelected == selected)
        return;
    _pressSelected = selected;
    setPressActive(selected);
    if (!selected)
        stopPress();// прижим выключили - сбрасываем поджим сразу, чтобы не оставались активные клапаны
    emit selectionChanged();
}

void CentralBroom::setFlowActive(bool state){
    if(isFlowing == state)
        return;
    isFlowing = state;
    goFlow(state);
}

void CentralBroom::goFlow(bool state){
    logger->addLog(state?"Щетка: плавание активировано":"Щетка: плавание деактивировано");
    io->set(StateValveC1, state);
    io->set(StateValveC2, state);
}

void CentralBroom::updateHeightEstimate(){
    const double dt = heightClock.isValid() ? heightClock.restart() / 1000.0 : 0;
    if (!heightClock.isValid())
        heightClock.start();

    if (io->get(StateDKPBroomUp).toBool()){// верхний концевик - точно наверху
        heightEstimate = 0;
        return;
    }
    // считаем только портал (F4 вниз, F10 вверх): прижим/отжим щётки внутри портала (F8/F2) на правило вращения
    // не влияет - он выбирает износ щётки
    const bool portalDown = io->get(StateValveF4).toBool();
    const bool portalUp = io->get(StateValveF10).toBool();
    if (isFlowing && !portalDown && !portalUp){
        // плавание: портал опускается под собственным весом и через flowDropTimeSec щётка лежит на поверхности
        heightEstimate = flowDropSec > 0 ? qMin(1.0, heightEstimate + dt / flowDropSec) : 1.0;
        return;
    }
    if (!hydraulics->isOn() || portalDown == portalUp)
        return;
    if (portalDown)
        heightEstimate = lowerTimeSec > 0 ? qMin(1.0, heightEstimate + dt / lowerTimeSec) : 1.0;
    else
        heightEstimate = raiseTimeSec > 0 ? qMax(0.0, heightEstimate - dt / raiseTimeSec) : 0.0;
}

bool CentralBroom::shouldSpin() const{
    switch (getState()) {
    case BroomRotateOut:// раскрутка перед опусканием
    case BroomRotated:
    case BroomDownOut:// опускаем уже раскрученной
        return true;
    case BroomDownIn:// подъём: останавливаем сразу, не дожидаясь верхнего положения
    case BroomRotateIn:
        return false;
    default:
        // работа внизу, ручное управление и плавание: по высоте портала - у земли крутится, выше порога стоит
        // (в плавании портал опускается под собственным весом - щётка раскрутится, когда подойдёт к земле)
        return heightEstimate >= spinHeight;
    }
}

void CentralBroom::updateRotation(){
    const bool spin = shouldSpin();
    if (spin){
        engineRpm->request(this, rpmForSweepType.value(_context->sweepType()) * 8);//обороты движка
        goRotate(speedForSweepType.value(_context->sweepType()));// скорость щеток
    }
    else{
        engineRpm->release(this);// щётка не крутится - обороты ей не нужны
        if (spinning)
            goNoRotate();
    }
    const BroomStates state = getState();
    if (spin != spinning && state != BroomRotateOut && state != BroomRotateIn && state != BroomDownOut){// о раскрутке и торможении автомат пишет сам
        const QString height = QString::number(qRound(heightEstimate * 100));
        logger->addLog(spin ? "Щетка: раскручиваем (портал на " + height + "% хода)"
                            : "Щетка: останавливаем (портал на " + height + "% хода)");
    }
    spinning = spin;
}

void CentralBroom::goRotate(int speed_){
    io->set(StateValveD1, speed_ / 2);
    //hydraulics->request(this, true);
}

void CentralBroom::goNoRotate(){
    io->set(StateValveD1, 0);
}

// void CentralBroom::increaseSpeed(){
//     int spd = io->get(StateValveD1).toUInt() * 2;
//     if (spd + 10 < 100)
//         spd += 10;
//     else
//         spd = 100;
//     goRotate(spd);
// }

// void CentralBroom::decreaseSpeed(){
//     int spd = io->get(StateValveD1).toUInt() * 2;
//     if (spd - 10 > 0)
//         spd -= 10;
//     else
//         spd = 0;
//     goRotate(spd);
// }

void CentralBroom::beforeStep(){
    const bool pressure = hydraulics->isOn();
    side.update(pressure && direction == organsEnums::Left, pressure && direction == organsEnums::Right,
                io->get(StateDKPBroomLeft).toBool(), io->get(StateDKPBroomRight).toBool());
    followActualSide(side.isLeft());
    updateHeightEstimate();
    updateRotation();
}
