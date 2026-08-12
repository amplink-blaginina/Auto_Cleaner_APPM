#ifndef PREROLLCONTROLLER_H
#define PREROLLCONTROLLER_H

#include "cancontroller.h"
#include "startercontroller.h"
#include "viewcontroller.h"

#include <currentstate.h>
#include <engine.h>
#include <globalsettings.h>
#include <qdatetime.h>
#include <screenlog.h>
#include <ui_mainwindow.h>


class PrerollController
{
public:
    PrerollController(GlobalSettings *globals, Engine *engine,
                      ScreenLog *screenLog, CanController *can,
                      StarterController *starter, CurrentState *state,
                      ViewController *view);

    // bool rollLockedByTemperature;
    // bool rollLockedByEmergency;
    // bool needRollProcedure;
    // bool rollCompleted;

    QDateTime rollRunStartedAt;
    QDateTime rollPauseStartedAt;
    QDateTime prerollStepStartedAt;

    // bool rollInputPrev;
    // bool rollRunActive;
    // bool rollPauseActive;
    //int rollAttemptsUsed;
    //bool rollNeedReboot;
   // bool rollPauseWarned;
    // bool logNeedRollShown;

    // bool prerollStarterButtonPrev;
    // bool prerollStarterUnlocked;
    // bool prerollButtonPrev;
    // bool prerollSequenceActive;
    // int prerollSequenceStep;

    void updateRollStatusText(QLabel *status);
    void stopRollOutput();
    bool rollBlocked() const;
    bool inRollPause() const;

    int rollPauseSecondsLeft() const;
    void processPrerollInService(QPushButton *preroll,
                                 QPushButton *prerollStarter,
                                 QLabel *prerollStatusLabel,
                                 bool isEngineFormVisible);
    void onStarterPressed();

    void resetValues();
    CurrentState *_state;
    ViewController *_view;

    void checkIfAwaitForRoll(int daysFromLastStart);
    void lockByTemperature(bool state);
    void checkEmergencies();
private:
    GlobalSettings *_globals;
    Engine *_engine;
    ScreenLog *_screenLog;
    CanController *_can;
    StarterController *_starter;
};

#endif // PREROLLCONTROLLER_H
