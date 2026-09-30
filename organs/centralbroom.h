#ifndef CENTRALBROOM_H
#define CENTRALBROOM_H

#include "organsenums.h"

#include <QObject>
#include <QWidget>
#include <QDateTime>
#include <QElapsedTimer>
#include <QMap>
#include <QSettings>
#include <screenlog.h>
//#include <mainwindow.h>

#include <can/mycan.h>
#include <can/mycanj1939.h>
#include <machine/machinecontext.h>
#include <machine/machineio.h>

#include <Controllers/viewcontroller.h>
class CentralBroom : public QObject
{
    Q_OBJECT
public:
    // состояния модуля (последовательный) 0 и 255 крайние состояния которые заставляют прогрессировать модуль по этапам в какую либо сторону
    enum BroomStates
    {
        BroomOff        = 0,
        BroomSlideIn    = 1,
        BroomSlideOut   = 2,
        BroomSlided     = 3,
        BroomBounceOut  = 4,
        BroomBounced    = 5,
        BroomRotateIn   = 6,
        BroomRotateOut  = 7,
        BroomRotated    = 8,
        BroomDownIn     = 9,
        BroomDownOut    = 10,
        BroomDowned     = 11,
        BroomFlowIn     = 12,
        BroomFlowOut    = 13,
        BroomFlowed     = 14,
        BroomPressOut   = 15,
        BroomPressed    = 16,

    };
    Q_ENUM(BroomStates);

    explicit CentralBroom(const MachineIo &machine, MachineContext *context, ViewController *logger, QObject *parent);
    ViewController *logger;
    IoBus *io;
    HydraulicSupply *hydraulics;
    EngineRpmDemand *engineRpm;
    QTimer progressTimer;
    bool choosed;
    bool broomAlarmed;

    void readSettings();
    void checkNeedState();
    bool testStateTimer();
    int getTimeout();
    void checkFriendVars();
    BroomStates stateUp();
    BroomStates stateDown();

    QDateTime startActionTime;

    // таймауты на каждую длительную операцию
    QMap<BroomStates, float> timeouts;
    // скорость вращения щетки под каждый тип смета
    QMap<int, int> speedForSweepType;
    // обороты двигателя под каждый тип смета
    QMap<int, int> rpmForSweepType;

    bool startClean;
    bool needGoLeft; // тут главный признак-будет ли эта щетка желать развернуться или нет (это поворот ВЛЕВО)
    bool isPressed = false;
    bool isFlowing = false;
    organsEnums::Direction direction = organsEnums::None;


    void setState(BroomStates state_);// установка и получение состояния модуля
    BroomStates state = BroomPressed; // стутус который мы предполагаем сейчас (лигические выводы); до первого setState(BroomOff) - любое, кроме Off
    BroomStates needState; // статус который мы желаем достичь
    void setNeedState(BroomStates state_);// установка и получение требуемого состояния модуля (к чему модуль движется так скажем)
    BroomStates getNeedState();

    BroomStates ableState; // статус который мы можем достичь
    void setAbleState(BroomStates state_);
    BroomStates getAbleState();

    BroomStates getState();
    QString toString(BroomStates s);

    void setDirection(organsEnums::Direction dir);
    void setDirection(organsEnums::Direction dir, bool isPressed);
    void goPressUp(bool state);
    void goPressDown(bool state);
    void stopPress();
    void setPressActive(bool state);
    void setFlowActive(bool state);
    // выбор оператора: плавание (включается при работе органа)
    bool isFlowSelected() const { return _flowSelected; }
    void selectFlow(bool selected) { _flowSelected = selected; }

    void goUpImmediate(bool state);
    void goDownImmediate(bool);

public slots:
    // слот для получания данных из CAN
    void progressLoop();
signals:
    void flowCancelRequested();// орган двигают вверх/вниз - плавание надо снять

private:
    void goLeft();
    void goLeft(bool state);
    void goRight();
    void goRight(bool state);
    void goNone();
    void goUp();
    void goUp(bool state, bool isPressed);
    void goDown();
    void goDown(bool state, bool isPressed);
    void goFlow(bool state);
    //void goNoFlow();
    void goRotate(int speed_);
    void goNoRotate();

    void printMovement(organsEnums::Direction dir, bool state, bool isPressed);

    // вращение: опускается на поверхность только раскрученной, при подъёме останавливается
    bool shouldSpin() const;
    void updateRotation();
    // высота 0 - верх, 1 - низ. Датчика высоты нет: оцениваем по времени работы клапанов подъёма/опускания,
    // верхний концевик сбрасывает оценку. Времена хода берём не больше реальных - оценка ошибается в безопасную сторону
    void updateHeightEstimate();
    double heightEstimate = 0;
    float lowerTimeSec = 5;
    float raiseTimeSec = 5;
    float spinHeight = 0.8;// ниже - щётка должна крутиться, выше - стоять
    bool spinning = false;
    QElapsedTimer heightClock;

    // void increaseSpeed();
    // void decreaseSpeed();
    void goSlide(bool toLeft);
    bool isTimeoutReached();
    bool wereBusyAndTimeoutReached(bool timeoutReached, BroomStates state);
    bool checkMovementAndStopOnTimeout(bool timeoutReached, bool isSensorReached, organsEnums::Direction dir);

    void printMovement(int, bool, bool);
    CentralBroom::BroomStates getNextState(CentralBroom::BroomStates current);
    CentralBroom::BroomStates getPreviousState(CentralBroom::BroomStates current);
    MachineContext *_context;
    bool _flowSelected = false;
};

#endif // CENTRALBROOM_H
