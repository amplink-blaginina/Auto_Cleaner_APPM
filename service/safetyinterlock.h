#ifndef SAFETYINTERLOCK_H
#define SAFETYINTERLOCK_H

#include <QObject>


// SafetyInterlock.h
class SafetyInterlock : public QObject {
    Q_OBJECT
public:
    enum BlockReason {
        None,
        RollRequired,       // требуется прокрутка
        ColdEngine,         // холодный двигатель
        WaterInFuel,        // вода в топливе
        AirFilterClogged,   // засор ВФ
        OilFilterClogged,   // засор МФ
        StarterLimit,       // лимит попыток стартера
        RollLimit,          // лимит попыток прокрутки
        IgnitionOff         // зажигание выключено
    };

    bool canStart() const;      // можно ли включить стартер?
    bool canRoll()  const;      // можно ли прокрутку?
    BlockReason reason() const;
    QString reasonText() const;

public slots:
    void setEngineTemp(int temp, bool valid);   // от J1939
    void setHeatRelay(bool active);             // от GPIO
    void setWaterSensor(bool bad);              // от CAN
    void setAirFilter(bool bad);
    void setOilFilter(bool bad);
    void setRollCompleted(bool ok);             // прокрутка выполнена
    void setIgnition(bool on);                  // зажигание

signals:
    void stateChanged(bool blocked, BlockReason reason, QString text);
};

#endif // SAFETYINTERLOCK_H
