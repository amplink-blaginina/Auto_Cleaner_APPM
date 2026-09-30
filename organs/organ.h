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

    // выбор оператора: сторона работы органа
    bool isLeftSelected() const { return _left; }
    bool isRightSelected() const { return _right; }
    bool isSideSelected() const { return _left || _right; }
    void toggleSide(bool right);// кнопка стороны до начала уборки: выбрать сторону или снять выбор
    void setSide(bool right);

signals:
    void selectionChanged();// выбор оператора изменился - перерисовать экран, пересчитать цели органов

protected:
    virtual void beforeStep() {}// такт органа до шага последовательности
    virtual void afterStep() {}// и после
    // оператор вручную перевёл орган на другую сторону - выбор стороны переходит за ним
    void followActualSide(bool onLeft);

    IoBus *io;
    HydraulicSupply *hydraulics;
    EngineRpmDemand *engineRpm;
    ViewController *logger;
    MachineContext *_context;
    OrganSequence sequence;

    bool _left = false;
    bool _right = false;

private:
    void progressLoop();
    QString _name;
    QTimer progressTimer;
};

#endif // ORGAN_H
