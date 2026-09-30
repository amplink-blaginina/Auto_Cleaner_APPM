#ifndef FRONTRAIL_H
#define FRONTRAIL_H

#include "screenlog.h"
#include <QObject>
#include <QWidget>
#include <QDateTime>
#include <QMap>
#include <QSettings>

#include <can/mycan.h>
#include <can/mycanj1939.h>
#include <machine/machineio.h>

#include <Controllers/viewcontroller.h>
class MainWindow;
class FrontRail : public QObject
{
    Q_OBJECT
public:
    // состояния модуля (последовательный) 0 и 255 крайние состояния которые заставляют прогрессировать модуль по этапам в какую либо сторону
    enum FrontRailStates
    {
        FrontRailOff        = 0,
        FrontRailSlideIn    = 1,
        FrontRailSlideOut   = 2,
        FrontRailSlided     = 3,
        FrontRailBounceOut  = 4,
        FrontRailBounced    = 5,
        FrontRailDownIn     = 6,
        FrontRailDownOut    = 7,
        FrontRailDowned     = 8,
        FrontRailFlowIn     = 9,
        FrontRailFlowOut    = 10,
        FrontRailFlowed     = 11
    };
    Q_ENUM(FrontRailStates)
    explicit FrontRail(const MachineIo &machine, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ViewController *logger, MainWindow* mainWindow, QObject *parent_);
    QObject * parent;
    ViewController *logger;
    IoBus *io;
    HydraulicSupply *hydraulics;
    EngineRpmDemand *engineRpm;
    MyCanJ1939 * myCanJ1939;
    QTimer progressTimer;
    QSettings *settings;
    bool choosed;
    bool railAlarmed;

    void readSettings();
    void checkNeedState();
    bool testStateTimer();
    float getTimeout();
    void checkFriendVars();
    FrontRailStates stateUp();
    FrontRailStates stateDown();

    QDateTime startActionTime;
    bool isFlowing;
    organsEnums::Direction direction;

    // таймауты на каждую длительную операцию
    QMap<FrontRailStates, float> timeouts;

    bool startClean;
    bool needGoLeft; // тут главный признак-будет ли эта щетка желать развернуться или нет (true значит надо повернуться вправо,
                     // считаем что 0 это самое правое значение и поворот влево увеличивает его до 100)

    // установка и получение состояния модуля
    void setState(FrontRailStates state_);
    FrontRailStates state; // стутус который мы предполагаем сейчас (лигические выводы)
    FrontRailStates needState; // статус который мы желаем достичь
    FrontRailStates ableState; // статус который мы можем достичь
    FrontRailStates getState();
    QString toString(FrontRailStates s);
    void goLeft(bool state);
    void goRight(bool state);
    void goNone(bool state);
    void goUp(bool state);
    void goDown(bool state);
    void goFlow(bool state);


    // установка и получение требуемого состояния модуля (к чему модуль движется так скажем)
    void setNeedState(FrontRailStates state_);
    FrontRailStates getNeedState();
    void setAbleState(FrontRailStates state_);
    FrontRailStates getAbleState();

    void setDirection(organsEnums::Direction dir);
    //void setDirection(organsEnums::Direction dir, bool isPressed);
    void setFlowActive(bool state);

public slots:
    // слот для получания данных из CAN
    void progressLoop();
private:
    void goLeft();
    void goRight();
    void goNone();
    void goUp();
    void goDown();
    void printMovement(organsEnums::Direction dir, bool state);
    MainWindow *_mainWindow;
signals:

};

#endif // FRONTRAIL_H
