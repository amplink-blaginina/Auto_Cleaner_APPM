#ifndef BACKMAGNET_H
#define BACKMAGNET_H

#include <QObject>
#include <QWidget>
#include <QDateTime>
#include <QMap>
#include <QSettings>
#include <screenlog.h>

#include <can/mycan.h>
#include <can/mycanj1939.h>

#include <Controllers/viewcontroller.h>

class BackMagnet : public QObject
{
    Q_OBJECT
public:
    // состояния модуля (последовательный) 0 и 255 крайние состояния которые заставляют прогрессировать модуль по этапам в какую либо сторону
    enum BackMagnetStates
    {
        BackMagnetOff        = 0,
        BackMagnetDownIn     = 1,
        BackMagnetDownOut    = 2,
        BackMagnetDowned     = 3
    };

    explicit BackMagnet(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ViewController *logger, QObject *parent_);
    QObject * parent;
    ViewController *logger;
    MyCan *myCan;
    MyCanJ1939 * myCanJ1939;
    QTimer progressTimer;
    QSettings *settings;
    bool choosed;
    bool magnetAlarmed;

    void readSettings();
    void checkNeedState();
    bool testStateTimer();
    int getTimeout();
    void checkFriendVars();
    BackMagnetStates stateUp();
    BackMagnetStates stateDown();

    QDateTime startActionTime;

    // таймауты на каждую длительную операцию
    QMap<BackMagnetStates, int> timeouts;

    bool startClean;

    // установка и получение состояния модуля
    void setState(BackMagnetStates state_);
    BackMagnetStates state; // стутус который мы предполагаем сейчас (лигические выводы)
    BackMagnetStates needState; // статус который мы желаем достичь
    BackMagnetStates ableState; // статус который мы можем достичь
    BackMagnetStates getState();
    QString toString(BackMagnetStates s);

    void goOff();
    void goUp();
    void goDown();

    // установка и получение требуемого состояния модуля (к чему модуль движется так скажем)
    void setNeedState(BackMagnetStates state_);
    BackMagnetStates getNeedState();
    void setAbleState(BackMagnetStates state_);
    BackMagnetStates getAbleState();

public slots:
    // слот для получания данных из CAN
    void progressLoop();
signals:

};

#endif // BACKMAGNET_H
