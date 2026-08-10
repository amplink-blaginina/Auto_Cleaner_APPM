#ifndef CURRENTSTATE_H
#define CURRENTSTATE_H

#include "qglobal.h"
#include "settingsreader.h"

#include <qdatetime.h>

#include <Controllers/gpiocontroller.h>
class CurrentState
{
public:
    enum MenuMode
    {
        SweepMode  = 1,
        BunkerMode = 2,
        DiagMode   = 3,
        SettingsMode   = 4
    };

    CurrentState(SettingsReader *reader, GPIOController *gpio);
    void setMode(quint8 mode);

    // bool waterAlarm;
    // bool airAlarm;
    // bool oilAlarm;
    bool disableRollRequirement;
    bool rollLockedByTemperature;
    bool rollLockedByEmergency;
    bool needRollProcedure;
    bool rollCompleted;
    bool waterAlarm;
    bool airAlarm;
    bool oilAlarm;

    bool prerollButtonPrev;
    bool prerollStarterButtonPrev;
    bool rollInputPrev;
    bool prerollSequenceActive;
    int prerollSequenceStep;
    bool prerollStarterUnlocked;
    bool rollRunActive;
    bool rollPauseActive;
    int rollAttemptsUsed;
    bool rollNeedReboot;
    bool rollPauseWarned;
    bool serviceIgnitionAutoRestoreBlocked;

    bool logNeedRollShown;
    bool logNeedWarmShown;
    bool waterSensorActivePrev;
    bool airFilterActivePrev;
    bool oilFilterActivePrev;
    bool heatRelayActivePrev;

    bool waterSensorEmergencyMode;
    bool airFilterEmergencyMode;

    bool disableTemperatureBlock;
    bool ignoreAllEmergency;

    bool engineRunStatePrev;


    bool waterSensorTimeStarted;
    bool airFilterTimeStarted;
    int waterSensorStartedAt;
    int airFilterStartedAt;

    qint16 engineCoolantTemp;
    qint16 vehicleSpeed; // скорость движения машины
    float vehicleVoltage;
    qint16 vehicleTemperature;


    void setDefaults();
    void setSuperDiagMode(bool state);

    void readValues();
    bool isSettingsMode();
    bool isDiagMode();
    bool isDiagOrSettingsMode();

    //bool superDiagMode;
    SettingsReader *_reader;
    void updateStartDate();
    int getDaysFromStart();

    void setSweepMode();
    void setDiagMode(bool state = true);
    void setSettingsMode();

    void resetVehicleValues();
    void setCoolantTmp(int value);//QByteRef
    void setVehicleSpeed(int value);
    void setVehicleVoltage(float value);
private:
    QDate lastEngineStartDate;
    quint8 menuMode;
    bool superDiagMode;
    GPIOController *_gpio;

};

#endif // CURRENTSTATE_H
