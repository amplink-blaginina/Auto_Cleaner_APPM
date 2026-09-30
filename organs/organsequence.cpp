#include "organsequence.h"

#include <Controllers/viewcontroller.h>

OrganSequence::OrganSequence(const QString &organName, ViewController *logger)
    : _name(organName), _logger(logger)
{
}

void OrganSequence::addStep(const Step &step){
    _steps.append(step);
}

int OrganSequence::positionOf(int code) const{
    // положение, к которому ведёт состояние: движение назад - к началу шага, вперёд и «выполнен» - к концу
    if (code <= 0)
        return 0;
    const int step = (code - 1) / 3;
    const int position = (code - 1) % 3 == 0 ? step : step + 1;
    return qMin(position, int(_steps.size()));
}

int OrganSequence::targetPosition() const{
    if (_viaPosition >= 0)
        return _viaPosition;
    return qMin(positionOf(_need), positionOf(_able));
}

const QVector<OrganSequence::Phase> &OrganSequence::currentMotion() const{
    const Step &step = _steps[(_state - 1) / 3];
    return (_state - 1) % 3 == 1 ? step.out : step.in;
}

float OrganSequence::timeoutOf(const Phase &phase) const{
    return phase.timeoutSec ? phase.timeoutSec() : 0;
}

void OrganSequence::passThrough(int stableCode, std::function<void()> then){
    _viaPosition = qBound(0, stableCode / 3, int(_steps.size()));
    _viaThen = std::move(then);
}

void OrganSequence::cancelPassThrough(){
    _viaPosition = -1;
    _viaThen = nullptr;
}

void OrganSequence::enter(int code){
    if (isStable(code))
        enterStable(qBound(0, code / 3, int(_steps.size())));
    else
        beginMotion((code - 1) / 3, (code - 1) % 3 == 1);
}

void OrganSequence::assume(int stableCode){
    if (!isStable(stableCode)){
        enter(stableCode);
        return;
    }
    _state = qBound(0, stableCode, topState());
    _phase = -1;
    notify();
}

void OrganSequence::tick(){
    const int target = targetPosition();
    if (isStable(_state)){
        const int position = _state / 3;
        if (position == _viaPosition){
            std::function<void()> then = std::move(_viaThen);
            cancelPassThrough();
            if (then)
                then();
            return;
        }
        if (target > position)
            beginMotion(position, true);
        else if (target < position)
            beginMotion(position - 1, false);
        return;
    }

    const int step = (_state - 1) / 3;
    const bool forward = (_state - 1) % 3 == 1;
    if (forward ? target <= step : target > step){// передумали - разворачиваемся с того места, где стоим
        beginMotion(step, !forward);
        return;
    }
    if (phaseFinished())
        advancePhase();
}

void OrganSequence::beginMotion(int step, bool forward){
    if (forward && step == 0)
        _alarmed = false;// новый выход из дома
    _state = 3 * step + (forward ? 2 : 1);
    _phase = -1;
    notify();
    advancePhase();
}

void OrganSequence::advancePhase(){
    const QVector<Phase> &phases = currentMotion();
    while (++_phase < phases.size()){
        const Phase &phase = phases[_phase];
        if (phase.skip && phase.skip())
            continue;
        if (_halt)
            _halt();
        if (!phase.log.isEmpty())
            _logger->addLog(phase.log);
        if (phase.start)
            phase.start();
        _clock.start();
        if (timeoutOf(phase) > 0)
            return;// ждём датчик или время
        // мгновенная фаза - сразу к следующей
    }
    const int step = (_state - 1) / 3;
    enterStable((_state - 1) % 3 == 1 ? step + 1 : step);
}

bool OrganSequence::phaseFinished(){
    const Phase &phase = currentMotion()[_phase];
    if (phase.sensor && phase.sensor()){
        _logger->addLog(_name + ": " + phase.name + " - достигнут датчик");
        return true;
    }
    const float timeout = timeoutOf(phase);
    if (_clock.elapsed() <= timeout * 1000)
        return false;
    if (phase.sensor){// датчик не сработал: останавливаем орган, идём дальше
        _logger->addLogWarning(_name + ": " + phase.name + " - датчик не сработал за "
                               + QString::number(timeout) + " с");
        if (_halt)
            _halt();
        _alarmed = true;
    }
    return true;
}

void OrganSequence::enterStable(int position){
    _state = 3 * position;
    _phase = -1;
    if (_halt)
        _halt();
    const std::function<void()> &done = position == 0 ? _home : _steps[position - 1].done;
    if (done)
        done();
    notify();
}

void OrganSequence::notify(){
    if (_onChange)
        _onChange(_state);
}
