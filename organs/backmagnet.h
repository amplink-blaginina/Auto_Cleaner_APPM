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
#include "organs/organcontroller.h"
#include "organs/organsenums.h"

class MainWindow;
class BackMagnet : public OrganController
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
    Q_ENUM(BackMagnetStates)

    bool isInstalled() const override;

    bool isSelected() const override;
    void setSelected(bool selected) override;

    bool isTransitioning() const override;
    bool isInHomeState() const override;
    bool isInWorkingState() const override;

    void requestHomeState() override;
    void updateTargetFromWorkMode(
        const OrganWorkMode &mode
        ) override;
    void stopAllOutputs() override;

    bool supportsDirection(
        organsEnums::Direction direction
        ) const override;

    void setManualDirection(
        organsEnums::Direction direction
        ) override;
    explicit BackMagnet(MyCan *myCan_, MyCanJ1939 *myCanJ1939_, QSettings *settings_, ViewController *logger, MainWindow* mainWindow, QObject *parent_);
    QObject * parent;
    ViewController *logger;
    MyCan *myCan;
    MyCanJ1939 * myCanJ1939;
    QTimer progressTimer;
    QSettings *settings;
    bool choosed = false;
    bool magnetAlarmed = false;
    bool startClean = false;
    BackMagnetStates state = BackMagnetOff; // стутус который мы предполагаем сейчас (лигические выводы)
    BackMagnetStates needState = BackMagnetOff; // статус который мы желаем достичь
    BackMagnetStates ableState = BackMagnetOff; // статус который мы можем достичь

    void readSettings();
    void checkNeedState();
    bool testStateTimer();
    int getTimeout();
    void checkFriendVars();
    BackMagnetStates stateUp();
    BackMagnetStates stateDown();

    QDateTime startActionTime;

    // таймауты на каждую длительную операцию
    QMap<BackMagnetStates, float> timeouts;


    // установка и получение состояния модуля
    BackMagnetStates getState();
    void setState(BackMagnetStates state_);
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

private:
    MainWindow *_mainWindow;
    bool hasPositionSensor(
        organsEnums::Direction direction
        ) const;
};

#endif // BACKMAGNET_H
