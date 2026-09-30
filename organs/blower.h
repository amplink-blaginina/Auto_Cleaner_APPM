#ifndef BLOWER_H
#define BLOWER_H

#include "organ.h"

#include <QMap>

class Blower : public Organ
{
    Q_OBJECT
public:
    // коды состояний движка (OrganSequence): шаги опускание, поворот, раскрутка
    enum BlowerStates
    {
        BlowerOff         = 0,
        BlowerDownIn      = 1,
        BlowerDownOut     = 2,
        BlowerDowned      = 3,
        BlowerSlideIn     = 4,
        BlowerSlideOut    = 5,
        BlowerSlided      = 6,
        BlowerRotateIn    = 7,
        BlowerRotateOut   = 8,
        BlowerRotated     = 9
    };
    Q_ENUM(BlowerStates)

    explicit Blower(const MachineIo &machine, MachineContext *context, ViewController *logger, QObject *parent);

    void readSettings() override;

    BlowerStates getState() const { return BlowerStates(sequence.state()); }
    void setNeedState(BlowerStates state_) { sequence.setNeed(state_); }
    BlowerStates targetState() const { return BlowerStates(sequence.targetState()); }// куда обдув идёт сейчас
    QString toString(BlowerStates s);

    void goOff();
    void goSlide(bool turn_right);
    void goUp();
    void goDown();
    void goRotate(quint8 speed);
    void goNone();

    bool isRotating();
    // команды оператора во время уборки (кнопку удержали)
    void holdUp();             // остановить и поднять, выбранная сторона сохраняется
    void holdDown();           // опустить и запустить
    void holdSide(bool right); // запустить в эту сторону или сменить сторону
    void setDirection(bool);
    bool targetRight() const { return isTargetRight; }// сторона, на которую идёт обдув

    // выбор оператора: «поднят вручную» (выбранная сторона при этом сохраняется)
    bool isLifted() const { return _lifted; }
    bool isActive() const { return isSideSelected() && !_lifted; }// обдув должен работать
    void setLifted(bool lifted);
    void clearSelection() override;

protected:
    void beforeStep() override;
    void afterStep() override;

private:
    bool _lifted = false;
    bool isTargetRight = false;// сторона, на которую надо перейти; отличается от _right, пока идёт смена стороны
    float targetRotationSpeed = 0;
    float currentRotationSpeed =0;
    float speedRotationStep = 1;
    void changeRotationSpeed();
    void setTargetRotationSpeed(float speed);

    // таймауты на каждую длительную операцию
    QMap<BlowerStates, float> timeouts;
    // скорость вращения под каждый тип смета
    QMap<int, int> speedForSweepType;
    // обороты двигателя под каждый тип смета
    QMap<int, int> rpmForSweepType;
};

#endif // BLOWER_H
