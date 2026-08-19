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
private:
    MyCan *_can0;
};

#endif // CANCONTROLLER_H
