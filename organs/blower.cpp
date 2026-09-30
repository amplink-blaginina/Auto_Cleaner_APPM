#include "blower.h"
#include <settingsreader.h>
#include <machine/sweeptype.h>
#include <Controllers/viewcontroller.h>
#include <QDebug>
#include <QMetaEnum>

Blower::Blower(const MachineIo &machine, MachineContext *context, ViewController *logger_, QObject *parent)
    : Organ("Обдув", machine, context, logger_, parent)
{
    readSettings();
    auto timeout = [this](BlowerStates s){ return [this, s]{ return timeouts.value(s, 0); }; };

    OrganSequence::Step down;// вниз концевика нет - по времени, вверх до обоих концевиков
    down.out = {{"опускание", "Обдув: опускаем раструб", [this]{ goDown(); }, nullptr, timeout(BlowerDownOut)}};
    down.in = {{"подъём", "Обдув: поднимаем раструб", [this]{ goUp(); },
                [this]{ return io->get(StateDKPBlowerUp1).toBool() && io->get(StateDKPBlowerUp2).toBool(); },
                timeout(BlowerDownIn)}};
    sequence.addStep(down);

    OrganSequence::Step slide;// поворот до упора в выбранную сторону; назад клапанов нет - только выдержка
    slide.out = {{"поворот", "Обдув: выставляем направление обдува", [this]{ goSlide(_right); }, nullptr,
                  timeout(BlowerSlideOut)}};
    slide.in = {{"поворот", "", nullptr, nullptr, timeout(BlowerSlideIn)}};
    sequence.addStep(slide);

    // скорость вентилятора плавно меняет changeRotationSpeed, шаг только выдерживает время
    OrganSequence::Step rotate;
    rotate.out = {{"раскрутка", "Обдув: раскручиваем вентилятор", nullptr, nullptr, timeout(BlowerRotateOut)}};
    rotate.in = {{"торможение", "Обдув: выключаем вентилятор", [this]{ setTargetRotationSpeed(0); }, nullptr,
                  timeout(BlowerRotateIn)}};
    sequence.addStep(rotate);

    sequence.setHalt([this]{ goOff(); });
    sequence.setOnChange([this](int s){ qDebug() << "Blower state " << toString(BlowerStates(s)); });
    goHome();
}

void Blower::readSettings()
{
    timeouts.clear();
    rpmForSweepType.clear();
    speedForSweepType.clear();

    auto reader = _context->settingsReader();
    rpmForSweepType.insert(LeafSweep, reader->readSettingsValue("Engine/rpm.LeafSweep").toInt());
    rpmForSweepType.insert(LightSweep, reader->readSettingsValue("Engine/rpm.LightSweep").toInt());
    rpmForSweepType.insert(MediumSweep, reader->readSettingsValue("Engine/rpm.MediumSweep").toInt());
    rpmForSweepType.insert(HeavySweep, reader->readSettingsValue("Engine/rpm.HeavySweep").toInt());

    // назначаем таймауты на длительные операции
    timeouts.insert(BlowerSlideOut, reader->readSettingsValue("Blower/timeouts.BlowerSlideOut").toInt());
    timeouts.insert(BlowerSlideIn, reader->readSettingsValue("Blower/timeouts.BlowerSlideIn").toInt());
    timeouts.insert(BlowerDownOut, reader->readSettingsValue("Blower/timeouts.BlowerDownOut").toInt());
    timeouts.insert(BlowerDownIn, reader->readSettingsValue("Blower/timeouts.BlowerDownIn").toInt());
    timeouts.insert(BlowerRotateOut, reader->readSettingsValue("Blower/timeouts.BlowerRotateOut").toInt());
    timeouts.insert(BlowerRotateIn, reader->readSettingsValue("Blower/timeouts.BlowerRotateIn").toInt());

    speedForSweepType.insert(LeafSweep, reader->readSettingsValue("Blower/speeds.LeafSweep").toInt());
    speedForSweepType.insert(LightSweep, reader->readSettingsValue("Blower/speeds.LightSweep").toInt());
    speedForSweepType.insert(MediumSweep, reader->readSettingsValue("Blower/speeds.MediumSweep").toInt());
    speedForSweepType.insert(HeavySweep, reader->readSettingsValue("Blower/speeds.HeavySweep").toInt());

    qDebug() << timeouts;
}

QString Blower::toString(BlowerStates s){
    const char *key = QMetaEnum::fromType<BlowerStates>().valueToKey(s);
    return key ? QString::fromLatin1(key) : QStringLiteral("UnknownState");
}

void Blower::setDirection(bool isRight){
    isTargetRight = isRight;
}

void Blower::setLifted(bool lifted){
    _lifted = lifted;
    emit selectionChanged();
}

void Blower::goOff(){
    io->set(StateValveE7, false);
    io->set(StateValveE3, false);
    io->set(StateValveE1, false);
    io->set(StateValveE5, false);
    hydraulics->request(this, false);
}

void Blower::goRotate(quint8 speed){
    //qDebug()<<"# RotationSpeed: "<<speed;
    io->set(StateValveD3, speed);
}

void Blower::goSlide(bool turn_right){
    if (turn_right){
        hydraulics->request(this, true);
        io->set(StateValveE7, true);
    }
    else{
        hydraulics->request(this, true);
        io->set(StateValveE3, true);
    }
}

void Blower::goUp(){
    hydraulics->request(this, true);
    io->set(StateValveE1, true);
}

void Blower::goDown()
{
    hydraulics->request(this, true);
    io->set(StateValveE5, true);
}

void Blower::goNone(){
    hydraulics->request(this, false);
    io->set(StateValveE5, false);
    io->set(StateValveE1, false);
    io->set(StateValveE3, false);
    io->set(StateValveE7, false);
}

void Blower::setTargetRotationSpeed(float speed){
    targetRotationSpeed = speed;
}

void Blower::changeRotationSpeed(){
    if(currentRotationSpeed < targetRotationSpeed){
        currentRotationSpeed += speedRotationStep;
        if (currentRotationSpeed>targetRotationSpeed){
            currentRotationSpeed = targetRotationSpeed;
        }
        goRotate(currentRotationSpeed);
        qDebug()<<"# blower speed up: "<<currentRotationSpeed;
        return;
    }

    if(currentRotationSpeed > targetRotationSpeed){
        currentRotationSpeed-=speedRotationStep;
        if(currentRotationSpeed<0){
            currentRotationSpeed = 0;
        }
        goRotate(currentRotationSpeed);
        qDebug()<<"# blower speed down: "<<currentRotationSpeed;
        return;
    }
}

void Blower::beforeStep(){
    if (getState() >= Blower::BlowerRotateOut){
        auto type = _context->sweepType();
        engineRpm->request(this, rpmForSweepType.value(type) * 8);//обороты движка
        setTargetRotationSpeed(speedForSweepType.value(type));// скорость вентилятора
    }
    else if (getState() <= Blower::BlowerDowned){
        engineRpm->release(this);// вентилятор остановлен - обороты обдуву не нужны
    }

    // смена стороны во время уборки: останавливаем, поворачиваем раструб обратно до нижнего положения,
    // там меняем сторону и снова идём к работе
    if (_context->isCleaning() && isTargetRight != _right){
        if (!sequence.isPassingThrough()){
            sequence.passThrough(BlowerDowned, [this]{
                logger->addLog("Обдув: меняем сторону");
                setSide(isTargetRight);
            });
        }
    }
    else if (sequence.isPassingThrough()){
        sequence.cancelPassThrough();
    }
}

void Blower::afterStep(){
    changeRotationSpeed();
}

bool Blower::isRotating(){
    return currentRotationSpeed > 0;
}

void Blower::holdSide(bool right){
    if (!_context->isCleaning())
        return;// уборка не запущена - кнопка только выбирает сторону, гидравлику не трогаем
    isTargetRight = right;
    if (!isSideSelected()){
        // сторона не выбрана - выбираем удерживаемую, иначе обдув останется выключенным
        _lifted = false;
        setSide(right);
    }
    else if (_lifted)// удержание стороны снова разворачивает поднятый обдув
        setLifted(false);
    setNeedState(BlowerRotated);
    // вручную раструб не поворачиваем: включённый обдув не может стоять в промежуточном положении,
    // поворот до крайнего положения делает автомат
}

void Blower::holdUp(){
    if (!_context->isCleaning())
        return;// уборка не запущена - гидравлику не трогаем
    // поднимаем обдув, выбранная сторона остаётся подсвеченной
    if (!_lifted)
        setLifted(true);
    setNeedState(BlowerOff);
}

void Blower::holdDown(){
    if (!_context->isCleaning())
        return;// уборка не запущена - гидравлику не трогаем
    // запуск: подсвечиваем сторону обдува, по умолчанию правую
    if (sequence.need() != BlowerRotated){
        _lifted = false;
        const bool right = !_left;
        isTargetRight = right;
        setSide(right);
    }
    setNeedState(BlowerRotated);
}
