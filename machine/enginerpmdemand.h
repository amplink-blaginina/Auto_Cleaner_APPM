#ifndef ENGINERPMDEMAND_H
#define ENGINERPMDEMAND_H

#include <QMap>

class MyCanEngine;

// Обороты двигателя: органы заявляют нужные обороты, на двигатель уходит наибольшая заявка.
// Без заявок - обороты простоя (их выставляет главный цикл).
// (раньше обдув, щётка и главный цикл писали обороты напрямую и перебивали друг друга)
// Значения - как для MyCanEngine::setEngineCommand (об/мин * 8)
class EngineRpmDemand
{
public:
    explicit EngineRpmDemand(MyCanEngine *engine);

    void request(const void *owner, quint16 rpm);
    void release(const void *owner);
    // обороты простоя: уходят на двигатель, только если заявок нет
    void applyIdle(quint16 idleRpm);

private:
    MyCanEngine *_engine;
    QMap<const void *, quint16> _requests;
};

#endif // ENGINERPMDEMAND_H
