#ifndef STARTERCONTROLLER_H
#define STARTERCONTROLLER_H

#include "cancontroller.h"
#include "gpiocontroller.h"

#include <boolstatewatcher.h>
#include <currentstate.h>
#include <engine.h>
#include <globalsettings.h>
#include <qdatetime.h>
#include <screenlog.h>
#include <other/engine/serviceotherengineleftform.h>

class MainWindow;
class StarterController
{
public:
    StarterController(GlobalSettings *globals,
                      CurrentState *state,
                      Engine *engine,
                      ScreenLog *screenLog,
                      CanController *can0,
                      GPIOController *gpio,
                      MainWindow* mainWindow);

    QDateTime starterStartedTime;
    QDateTime starterPauseStartedAt;

    int starterAttemptsUsed = 0;
    bool starterLockedByRoll = false;
    bool starterLockedByTemperature = false;
    bool starterLockedByEmergency = false;
    bool starterNeedReboot = false;
    bool starterPressedPrev = false;
    bool starterPauseWarned = false;
    bool starterStarted = false;
    bool starterStartedAlarmed = false;
    bool starterBroomAlarmed = false;
    bool starterBunkerAlarmed = false;
    bool starterPauseActive = false;
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
    //bool isStarterClicked();
    QString getFatalStatusMessage();

    void updateButtons(bool state);
    void setEngineForm(ServiceOtherEngineLeftForm *otherEngineForm);
    void updateStarterStatusText(QString text, bool isError = false);
private:
    GlobalSettings *_globals;
    Engine *_engine;
    ScreenLog *_screenLog;
    CanController *_can;
    GPIOController *_gpio;
    CurrentState *_state;
    bool engineWasRunning;
    bool intentionalShutdown;
    BoolStateWatcher m_starter;

    bool checkAttemptsLimitReached();
    void handleStarterTimeout();
    bool checkAbleToStart();
    void startStarterOutput();
    void engineShutdown();
    void configureButtons();
    void onStarterPress();
    void onStarterHold();
    void onStarterRelease();
    void updateStarterState();
    bool getEngineRunning();
    MainWindow *_mainWindow;
    QLabel *_statusLbl;
    bool trySetStarterState(bool state);
};

#endif // STARTERCONTROLLER_H
