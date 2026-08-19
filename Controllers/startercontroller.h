#ifndef STARTERCONTROLLER_H
#define STARTERCONTROLLER_H

#include "cancontroller.h"
#include "gpiocontroller.h"

#include <currentstate.h>
#include <engine.h>
#include <globalsettings.h>
#include <qdatetime.h>
#include <screenlog.h>


class StarterController
{
public:
    StarterController(GlobalSettings *globals,
                      CurrentState *state,
                      Engine *engine,
                      ScreenLog *screenLog,
                      CanController *can0,
                      GPIOController *gpio);

    QDateTime starterStartedTime;
    QDateTime starterPauseStartedAt;

    int starterAttemptsUsed;
    bool starterLockedByRoll;
    bool starterLockedByTemperature;
    bool starterLockedByEmergency;
    bool starterNeedReboot;
    bool starterPressedPrev;
    bool starterPauseWarned;
    bool starterStarted;
    bool starterStartedAlarmed;
    bool starterBroomAlarmed;
    bool starterBunkerAlarmed;
    bool starterPauseActive;
    bool starterPressed = false;
    bool engineStartedOk = false;
    quint32 ignitionOffTimer;



    int starterPauseSecondsLeft() const;
    bool inStarterPause() const;
    bool starterBlocked() const;
    void setStarterPressed(bool state);
    void showStarter();
    void setDefaults();
    void resetValues();
    void checkPauseState();
    void stopStarterOutput();
    void resetIgnitionTimer();
    void increaseIgnitionTimer();
    void setIgnition(bool state);
    void TurnOffTheEngine();
    void restoreIgnition();
    void forceStopIgnition();
    bool isStarterReleased();
    bool isStarterPressed();
    bool isStarterClicked();
    QString getFatalStatusMessage();


    //void startIgnition();

private:
    GlobalSettings *_globals;
    Engine *_engine;
    ScreenLog *_screenLog;
    CanController *_can;
    GPIOController *_gpio;
    CurrentState *_state;
    bool engineWasRunning;
    bool intentionalShutdown;
    bool checkAttemptsLimitReached();
    void handleStarterTimeout();
    bool checkAbleToStart();
};

#endif // STARTERCONTROLLER_H
