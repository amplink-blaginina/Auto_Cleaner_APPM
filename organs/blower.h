#ifndef BLOWER_H
#define BLOWER_H

#include <QObject>
#include <QWidget>
#include <QDateTime>
#include <QMap>
#include <QSettings>
#include <screenlog.h>

#include <can/mycan.h>
#include <can/mycanj1939.h>
#include <machine/machinecontext.h>
#include <machine/machineio.h>

#include <Controllers/viewcontroller.h>
class Blower : public QObject
{
    Q_OBJECT
public:
    // состояния модуля (последовательный) 0 и 255 крайние состояния которые заставляют прогрессировать модуль по этапам в какую либо сторону
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
    ViewController *logger;
    IoBus *io;
    HydraulicSupply *hydraulics;
    EngineRpmDemand *engineRpm;
    QTimer progressTimer;
    bool choosed;
    bool blowerAlarmed;

    void readSettings();
    void checkNeedState();
    bool testStateTimer();
    int getTimeout();
    void checkFriendVars();

    BlowerStates stateUp();
    BlowerStates stateDown();

    QDateTime startActionTime;

    // таймауты на каждую длительную операцию
    QMap<BlowerStates, float> timeouts;
    // скорость вращения щетки под каждый тип смета
    QMap<int, int> speedForSweepType;
    // обороты двигателя под каждый тип смета
    QMap<int, int> rpmForSweepType;

    bool startClean;
    bool rightBlow;

    // установка и получение состояния модуля
    void setState(BlowerStates state_);
    BlowerStates state; // стутус который мы предполагаем сейчас (лигические выводы)
    BlowerStates needState; // статус который мы желаем достичь
    BlowerStates ableState; // статус который мы можем достичь
    BlowerStates getState();
    QString toString(BlowerStates s);

    void goOff();
    void goSlide(bool turn_right);
    void goUp();
    void goDown();
    void goRotate(quint8 speed);

    // установка и получение требуемого состояния модуля (к чему модуль движется так скажем)
    void setNeedState(BlowerStates state_);
    BlowerStates getNeedState();
    void setAbleState(BlowerStates state_);
    BlowerStates getAbleState();

    void goNone();
    bool isRotating();
    QTime stoppingStartedAt;
    QTime startingStartedAt;
    QTime rotationStartedAt;
    float stopDelay = 3;
    void setStartMomentForStopping();
    void setStartMomentForStarting();
    void setStartMomentForRotation();
    bool isHeldLongEnough(const QTime &since) const;
    void updateWhenUpPressed();
    void updateWhenDownPressed();
    void updateWhenRotationPressed(bool isRight);
    void setDirection(bool);
    bool targetRight() const { return isTargetRight; }// сторона, на которую идёт обдув

    // выбор оператора: сторона обдува и «поднят вручную» (выбранная сторона при этом сохраняется)
    bool isLeftSelected() const { return _left; }
    bool isRightSelected() const { return _right; }
    bool isLifted() const { return _lifted; }
    bool isActive() const { return (_left || _right) && !_lifted; }// обдув должен работать
    void toggleSide(bool right);// кнопка стороны до начала уборки: выбрать сторону или снять выбор
    void setSide(bool right);
    void setLifted(bool lifted);
public slots:
    // слот для получания данных из CAN
    void progressLoop();
signals:
    void selectionChanged();// выбор оператора изменился - перерисовать экран, пересчитать цели органов
private:
    bool _left = false;
    bool _right = false;
    bool _lifted = false;
    void updateTransitioning();
    bool isTargetRight = false;
    float targetRotationSpeed = 0;
    float currentRotationSpeed =0;
    float speedRotationStep = 1;
    void changeRotationSpeed();
    void setTargetRotationSpeed(float speed);

    BlowerStates rotate();
    MachineContext *_context;
};

#endif // BLOWER_H
