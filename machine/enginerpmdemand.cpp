#include "enginerpmdemand.h"

#include <algorithm>

#include "can/mycanengine.h"

EngineRpmDemand::EngineRpmDemand(MyCanEngine *engine)
    : _engine(engine)
{
}

void EngineRpmDemand::request(const void *owner, quint16 rpm){
    _requests.insert(owner, rpm);
    const auto values = _requests.values();
    _engine->setEngineCommand(*std::max_element(values.begin(), values.end()));
}

void EngineRpmDemand::release(const void *owner){
    // после снятия последней заявки обороты простоя выставит главный цикл (applyIdle)
    _requests.remove(owner);
    if (!_requests.isEmpty()){
        const auto values = _requests.values();
        _engine->setEngineCommand(*std::max_element(values.begin(), values.end()));
    }
}

void EngineRpmDemand::applyIdle(quint16 idleRpm){
    if (_requests.isEmpty())
        _engine->setEngineCommand(idleRpm);
}
