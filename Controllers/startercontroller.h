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

    void showStarter();
    bool starterStarted;
    QDateTime starterStartedTime;

    bool starterStartedAlarmed;
    bool starterBroomAlarmed;
    bool starterBunkerAlarmed;

    bool engineStartedOk = false;

    quint32 ignitionOffTimer;

    bool starterPressed = false;
    void setStarterPressed(bool state);

    bool starterLockedByRoll;
    bool starterLockedByTemperature;
    bool starterLockedByEmergency;

    bool starterPauseActive;
    QDateTime starterPauseStartedAt;
    int starterAttemptsUsed;
    bool starterNeedReboot;
    bool starterPressedPrev;
    bool starterPauseWarned;

    bool isStarterPressed();
    bool inStarterPause() const;
    bool isStarterClicked();
    int starterPauseSecondsLeft() const;
    void setDefaults();
    void resetValues();
    void checkPauseState();
    void stopStarterOutput();
    bool starterBlocked() const;

    QString getFatalStatusMessage();

    void resetIgnitionTimer();
    void increaseIgnitionTimer();
    void startIgnition();
private:
    GlobalSettings *_globals;
    Engine *_engine;
    ScreenLog *_screenLog;
    CanController *_can;
    GPIOController *_gpio;
    CurrentState *_state;
};

#endif // STARTERCONTROLLER_H
