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

#include <Controllers/viewcontroller.h>
class MainWindow;
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

    explicit Blower(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ViewController *logger, MainWindow* mainWindow, QObject *parent_);
    QObject * parent;
    ViewController *logger;
    MyCan *myCan;
    MyCanJ1939 * myCanJ1939;
    QTimer progressTimer;
    QSettings *settings;
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
    void updateWhenUpPressed();
    void updateWhenDownPressed();
    void updateWhenRotationPressed(bool isRight);
public slots:
    // слот для получания данных из CAN
    void progressLoop();
signals:
private:
    void updateTransitioning();
    bool isTargetRight = false;
    float targetRotationSpeed = 0;
    float currentRotationSpeed =0;
    float speedRotationStep = 1;
    void changeRotationSpeed();
    void setTargetRotationSpeed(float speed);

    BlowerStates rotate();
    MainWindow *_mainWindow;
};

#endif // BLOWER_H
