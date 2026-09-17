#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#pragma once
#include "organs/organsenums.h"
#include <QString>

class SettingsReader;

enum class MachineModel
{
    Unknown,
    APPM200,
    APPM318D4,
    Airport
};

enum class Equipment
{
    CentralBroom,
    FrontDump,
    Blower,
    BackMagnet,

    BroomFloat,
    BroomPress,
    DumpFloat
};

enum class Sensor
{
    WaterInFuel,
    AirFilter,
    OilFilter,
    HeatRelay,

    PressureFilter1,
    PressureFilter2,
    PressureFilter3,
    DrainFilter,

    HydraulicTankLevel,

    HydraulicOilTemperature,
    HydraulicDistributorPressure,
    HydraulicBroomPressure,
    HydraulicFanPressure,
    HydraulicBroomPressPressure
};

class Configuration
{
public:
    explicit Configuration(SettingsReader* settingsReader);

    void load();

    MachineModel model() const;
    QString modelName() const;

    bool hasEquipment(Equipment equipment) const;

    bool hasCentralBroom() const;
    bool hasFrontDump() const;
    bool hasBlower() const;
    bool hasBackMagnet() const;
    bool hasSensor(Sensor sensor) const;
    bool hasPositionSensor(
        organsEnums::Organ organ,
        organsEnums::Direction direction
        ) const;
    bool hasBlowerUpSensor1() const;
    bool hasBlowerUpSensor2() const;

private:
    bool readBool(const QString& key) const;

    SettingsReader* m_settingsReader = nullptr;

    MachineModel m_model = MachineModel::Unknown;
    struct PositionSensors
    {
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
    };

    PositionSensors m_broomPositionSensors;
    PositionSensors m_dumpPositionSensors;
    PositionSensors m_blowerPositionSensors;
    bool m_blowerUpSensor1Installed = true;
    bool m_blowerUpSensor2Installed = true;
    PositionSensors m_backMagnetPositionSensors;

    bool m_centralBroomInstalled = false;
    bool m_frontDumpInstalled = false;
    bool m_blowerInstalled = false;
    bool m_backMagnetInstalled = false;

    bool m_broomFloatInstalled = false;
    bool m_broomPressInstalled = false;
    bool m_dumpFloatInstalled = false;

    bool m_waterInFuelInstalled = true;
    bool m_airFilterInstalled = true;
    bool m_oilFilterInstalled = true;
    bool m_heatRelayInstalled = true;
    bool m_pressureFilter1Installed = true;
    bool m_pressureFilter2Installed = true;
    bool m_pressureFilter3Installed = true;

    bool m_drainFilterInstalled = true;
    bool m_hydraulicTankLevelInstalled = true;
    bool m_hydraulicOilTemperatureInstalled = true;

    bool m_hydraulicDistributorPressureInstalled = true;
    bool m_hydraulicBroomPressureInstalled = true;
    bool m_hydraulicFanPressureInstalled = true;
    bool m_hydraulicBroomPressPressureInstalled = true;
};


#endif // CONFIGURATION_H
