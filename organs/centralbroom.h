#ifndef CENTRALBROOM_H
#define CENTRALBROOM_H

#include "organsenums.h"

#include <QObject>
#include <QWidget>
#include <QDateTime>
#include <QMap>
#include <QSettings>
#include <screenlog.h>
//#include <mainwindow.h>

#include <can/mycan.h>
#include <can/mycanj1939.h>

#include <Controllers/viewcontroller.h>
class MainWindow;
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

    explicit CentralBroom(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ViewController *logger, MainWindow* mainWindow, QObject *parent_);
    QObject * parent;
    ViewController *logger;
    MyCan *myCan;
    MyCanJ1939 * myCanJ1939;
    QTimer progressTimer;
    QSettings *settings;
    bool choosed = false;
    bool broomAlarmed = false;
    bool startClean = false;
    bool needGoLeft = false; // тут главный признак-будет ли эта щетка желать развернуться или нет (это поворот ВЛЕВО)
    bool isPressed = false;
    bool isFlowing = false;
    organsEnums::Direction direction = organsEnums::None;
    BroomStates state = BroomOff; // стутус который мы предполагаем сейчас (лигические выводы)
    BroomStates needState = BroomOff; // статус который мы желаем достичь
    BroomStates ableState = BroomOff; // статус который мы можем достичь

    void readSettings();
    void checkNeedState();
    bool testStateTimer();
    int getTimeout();
    void checkFriendVars();
    BroomStates stateUp();
    BroomStates stateDown();
    BroomStates getNeedState();

    QDateTime startActionTime;

    // таймауты на каждую длительную операцию
    QMap<BroomStates, float> timeouts;
    // скорость вращения щетки под каждый тип смета
    QMap<int, int> speedForSweepType;
    // обороты двигателя под каждый тип смета
    QMap<int, int> rpmForSweepType;



    void setState(BroomStates state_);// установка и получение состояния модуля
    void setNeedState(BroomStates state_);// установка и получение требуемого состояния модуля (к чему модуль движется так скажем)

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

    void goUpImmediate(bool state);
    void goDownImmediate(bool);

public slots:
    // слот для получания данных из CAN
    void progressLoop();
signals:

private:
    bool hasPositionSensor(organsEnums::Direction direction) const;

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

    // void increaseSpeed();
    // void decreaseSpeed();
    void goSlide(bool toLeft);
    bool isTimeoutReached();
    bool wereBusyAndTimeoutReached(bool timeoutReached, BroomStates state);
    bool checkMovementAndStopOnTimeout(bool timeoutReached, bool isSensorReached, organsEnums::Direction dir);

    void printMovement(int, bool, bool);
    CentralBroom::BroomStates getNextState(CentralBroom::BroomStates current);
    CentralBroom::BroomStates getPreviousState(CentralBroom::BroomStates current);
    MainWindow *_mainWindow;
};

#endif // CENTRALBROOM_H
