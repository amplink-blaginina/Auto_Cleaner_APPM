#ifndef PREROLLCONTROLLER_H
#define PREROLLCONTROLLER_H

#include "cancontroller.h"
#include "startercontroller.h"
#include "viewcontroller.h"

#include <currentstate.h>
#include <engine.h>
#include <globalsettings.h>
#include <qdatetime.h>
#include <QElapsedTimer>
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
                      ViewController *view,
                      MainWindow* mainWindow);

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
    void setPrerollPressed(bool state);
    void resetPreroll();
    // предупреждение при входе в окно ДВС: кнопка прокрутки на пульте неисправна или ещё нажата
    void warnIfRollInputBlocked();
private:
    // кнопка прокрутки на пульте: работает только в окне ДВС и только если её отпускали, пока окно открыто
    // (замкнутая до входа в меню не срабатывает); замкнутая сразу после появления связи с блоком -
    // неисправна (замыкание, залипание), до перезапуска программы её сигнал не принимаем
    bool readRollInput(bool serviceEngineVisible);
    static constexpr qint64 ROLL_STUCK_WINDOW_MS = 3000;
    QElapsedTimer rollInputSinceOnline;
    bool rollInputFaulty = false;
    bool rollInputArmed = false;
    GlobalSettings *_globals;
    Engine *_engine;
    ScreenLog *_screenLog;
    CanController *_can;
    StarterController *_starter;

    ServiceOtherEngineLeftForm *_otherEngineForm;
    QPushButton *_prerollBtn;
    QPushButton *_starterPrerollBtn;
    QLabel *_statusLbl;
    bool _isInited = false;
    BoolStateWatcher m_preroll;

    bool foundButtons();
    void configureButtons();
    MainWindow *_mainWindow;
};

#endif // PREROLLCONTROLLER_H
