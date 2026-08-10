#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include "settingsreader.h"

#include <qstring.h>
#include <qvariant.h>


class GlobalSettings
{
public:
    GlobalSettings(SettingsReader *reader);
    QVariant readSettingsValue(QString name);
    void setDefaults();
    void readValues();

    int starterMaxWorkSec;
    int starterPauseSec;
    int starterMaxAttempts;
    int rollMaxWorkSec;
    int rollPauseSec;
    int rollMaxAttempts;
    int requireRollAfterDays;
    int lowTempRequireWarm;
    int waterSensorRedHours;
    int airFilterRedHours;
    int restartIgnitionDelay;
    int rpmNone; // скорость двигателя для холостых

    int disableCleanSpeed;
    int enableCleanSpeed;

    int ventEdge;//не используется
    quint8 enigneAddr;

    int getRollAttempts() const;
    int getRpm() const;

    bool starterWorkingLimitReached(QDateTime startMoment) const;
    bool starterAttemptsLimitReached(int used) const;
    bool checkStarterPause(QDateTime startMoment) const;
    bool checkRollPause(QDateTime startMoment) const;
    int starterPauseSecondsLeft(QDateTime startMoment) const;
    int rollPauseSecondsLeft(QDateTime startMoment) const;
    bool rollAttemptsLimitReached(int used) const;
    bool rollWorkingLimitReached(QDateTime startMoment) const;
    bool isEngineCold(int value) const;
    bool isNeedRoolByDate(int value) const;
private:
    SettingsReader *_reader;
};

#endif // GLOBALSETTINGS_H
