#ifndef STARTERCONTROLLER_H
#define STARTERCONTROLLER_H

#include "cancontroller.h"
#include "gpiocontroller.h"

#include <BoolStateWatcher.h>
#include <currentstate.h>
#include <engine.h>
#include <globalsettings.h>
#include <qdatetime.h>
#include <QElapsedTimer>
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

    // screenPressed - кнопка «Стартер» на экране ДВС, physicalPressed - кнопка стартера на пульте
    void updateButtons(bool screenPressed, bool physicalPressed);
    // кнопка пульта оказалась нажатой сразу после запуска программы - считаем её неисправной (замыкание)
    // и до перезапуска программы её сигнал не принимаем; экранная кнопка работает
    bool isPhysicalStarterFaulty() const { return physicalStarterFaulty; }
    void warnIfPhysicalStarterFaulty();// предупреждение при входе в окно ДВС (прокрутка)
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

    // проверка кнопки стартера пульта на залипание при запуске
    static constexpr qint64 STARTER_STUCK_WINDOW_MS = 3000;// нажата в первые 3 с опроса кнопок - неисправна
    QElapsedTimer buttonsSinceStart;
    bool physicalStarterFaulty = false;

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
    QLabel *_statusLbl = nullptr;
    bool trySetStarterState(bool state);
};

#endif // STARTERCONTROLLER_H
