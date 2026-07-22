#ifndef CENTRALBROOM_H
#define CENTRALBROOM_H

#include "organsenums.h"

#include <QObject>
#include <QWidget>
#include <QDateTime>
#include <QMap>
#include <QSettings>
#include <screenlog.h>

#include <can/mycan.h>
#include <can/mycanj1939.h>

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
        BroomFlowed     = 14

    };

    Q_ENUM(BroomStates);
    explicit CentralBroom(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ScreenLog *logger, QObject *parent_);
    QObject * parent;
    ScreenLog *logger;
    MyCan *myCan;
    MyCanJ1939 * myCanJ1939;
    QTimer progressTimer;
    QSettings *settings;
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
    QMap<BroomStates, int> timeouts;
    // скорость вращения щетки под каждый тип смета
    QMap<int, int> speedForSweepType;
    // обороты двигателя под каждый тип смета
    QMap<int, int> rpmForSweepType;

    bool startClean;
    bool needSlided; // тут главный признак-будет ли эта щетка желать развернуться или нет (это поворот ВЛЕВО)

    // установка и получение состояния модуля
    void setState(BroomStates state_);
    BroomStates state; // стутус который мы предполагаем сейчас (лигические выводы)
    BroomStates needState; // статус который мы желаем достичь
    BroomStates ableState; // статус который мы можем достичь
    BroomStates getState();
    QString toString(BroomStates s);

    void goLeft();
    void goLeft(bool state);
    void goRight();
    void goRight(bool state);
    void goNone();
    void goPressNone();
    void goUp();
    void goUp(bool state);
    void goDown();
    void goDown(bool state);
    void goFlow();
    void goNoFlow();
    void goRotate(int speed_);
    void goNoRotate();
    void increaseSpeed();
    void decreaseSpeed();
    void goPressUp();
    void goPressDown();

    // установка и получение требуемого состояния модуля (к чему модуль движется так скажем)
    void setNeedState(BroomStates state_);
    BroomStates getNeedState();
    void setAbleState(BroomStates state_);
    BroomStates getAbleState();

public slots:
    // слот для получания данных из CAN
    void progressLoop();
signals:

private:
    void goSlide(bool toLeft);
    bool isTimeoutReached();
    bool wereBusyAndTimeoutReached(bool timeoutReached, BroomStates state);
    bool checkMovementAndStopOnTimeout(bool timeoutReached, bool isSensorReached, organsEnums::Direction dir);
};

#endif // CENTRALBROOM_H
