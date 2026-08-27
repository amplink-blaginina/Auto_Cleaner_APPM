#ifndef CANCONTROLLER_H
#define CANCONTROLLER_H

#include <mycan.h>


class CanController
{
public:
    CanController(MyCan *can0);
    bool getState(DeviceStates state);
    void setStarterAvailable(bool state);
    void setIgnition(bool state);
    bool getRollIn();
    void setRollStarter(bool state);
    bool getOilRele();
    bool getAlarm();
    int getHydraOilTmp();
    bool isDisabled();
    bool isBoard0IN();
    void fillSystemConfig();
    bool getHeatState();
    void invertIgnition();
    bool getIgnition();
    bool getSensorPower();
    bool getHydraulicFan();
    void setState(DeviceStates state, bool value);
    void invertState(DeviceStates state);
    uint getOilTmp();
    template<typename T>
    T getParam(const DeviceStates key, const T &defaultValue = T{}) const;
    int getInt(DeviceStates key);
    bool isConfigured();
    QVariant getOriginalState(quint8 board_, quint8 channel_);
    void setOriginalState(quint8 board_, quint8 channel_, int value);
private:
    MyCan *_can0;
};

#endif // CANCONTROLLER_H
