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
//#include <ui_serviceotherengineleftform.h>

#include <other/engine/serviceotherengineleftform.h>


class PrerollController
{
public:
    PrerollController(GlobalSettings *globals, Engine *engine,
                      ScreenLog *screenLog, CanController *can,
                      StarterController *starter, CurrentState *state,
                      ViewController *view);

    QDateTime rollRunStartedAt;
    QDateTime rollPauseStartedAt;
    QDateTime prerollStepStartedAt;

    void updateRollStatusText();
    void stopRollOutput();
    bool rollBlocked() const;
    bool inRollPause() const;

    int rollPauseSecondsLeft() const;
    void processPrerollInService(bool isEngineFormVisible);
    void onStarterPressed();

    void resetValues();
    CurrentState *_state;
    ViewController *_view;

    void checkIfAwaitForRoll(int daysFromLastStart);
    void lockByTemperature(bool state);
    void checkEmergencies();
    void update();
    void setEngineForm(ServiceOtherEngineLeftForm *otherEngineForm);
private:
    GlobalSettings *_globals;
    Engine *_engine;
    ScreenLog *_screenLog;
    CanController *_can;
    StarterController *_starter;

    ServiceOtherEngineLeftForm *_otherEngineForm;
    QPushButton *_prerollBtn;
    QPushButton *_starterPrerollBtn;
    QLabel *_statusLbl;
    bool foundButtons();
    bool _isInited = false;
};

#endif // PREROLLCONTROLLER_H
