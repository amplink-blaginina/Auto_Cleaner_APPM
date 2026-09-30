#ifndef ORGAN_H
#define ORGAN_H

#include "organsequence.h"

#include <QObject>
#include <QTimer>

#include <machine/machinecontext.h>
#include <machine/machineio.h>

class ViewController;

// Общая часть органа: доступ к машине, таймер и движок последовательности.
// Конкретный орган описывает свои шаги в sequence и при необходимости добавляет действия на каждом такте.
class Organ : public QObject
{
    Q_OBJECT
public:
    Organ(const QString &name, const MachineIo &machine, MachineContext *context, ViewController *logger, QObject *parent);

    bool choosed = false;// орган выбран для уборки

    virtual void readSettings() = 0;

    bool isTransitioning() const { return sequence.isTransitioning(); }// орган ещё не дошёл до цели
    bool isAlarmed() const { return sequence.isAlarmed(); }
    bool isHome() const { return sequence.state() == 0; }
    void goHome() { sequence.enter(0); }// считать орган убранным и всё выключить
    void assumeDeployed() { sequence.assume(sequence.topState()); }// считать разложенным - орган уберётся заново

protected:
    virtual void beforeStep() {}// такт органа до шага последовательности
    virtual void afterStep() {}// и после

    IoBus *io;
    HydraulicSupply *hydraulics;
    EngineRpmDemand *engineRpm;
    ViewController *logger;
    MachineContext *_context;
    OrganSequence sequence;

private:
    void progressLoop();
    QTimer progressTimer;
};

#endif // ORGAN_H
