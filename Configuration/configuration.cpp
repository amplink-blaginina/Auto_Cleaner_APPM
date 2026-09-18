#include "configuration.h"

#include <settingsreader.h>

Configuration::Configuration(SettingsReader* settingsReader)
    : m_settingsReader(settingsReader)
{
}

bool Configuration::readBool(const QString& key) const
{
    if (m_settingsReader == nullptr) {
        return false;
    }

    return m_settingsReader->readSettingsValue(key).toBool();
}

void Configuration::load()
{
    if (m_settingsReader == nullptr) {
        return;
    }

    const QString modelValue =
        m_settingsReader->readSettingsValue("Machine/model").toString();

    if (modelValue == "APPM200") {
        m_model = MachineModel::APPM200;
    } else if (modelValue == "APPM318D4") {
        m_model = MachineModel::APPM318D4;
    } else if (modelValue == "Airport") {
        m_model = MachineModel::Airport;
    } else {
        m_model = MachineModel::Unknown;
    }

    m_centralBroomInstalled =
        readBool("Equipment/centralBroomInstalled");

    m_frontDumpInstalled =
        readBool("Equipment/frontDumpInstalled");

    m_blowerInstalled =
        readBool("Equipment/blowerInstalled");

    m_backMagnetInstalled =
        readBool("Equipment/backMagnetInstalled");

    m_broomFloatInstalled =
        readBool("Equipment/broomFloatInstalled");

    m_broomPressInstalled =
        readBool("Equipment/broomPressInstalled");

    m_dumpFloatInstalled =
        readBool("Equipment/dumpFloatInstalled");

    m_waterInFuelInstalled =
        readBool("Sensors/waterInFuelInstalled");

    m_airFilterInstalled =
        readBool("Sensors/airFilterInstalled");

    m_oilFilterInstalled =
        readBool("Sensors/oilFilterInstalled");

    m_heatRelayInstalled =
        readBool("Sensors/heatRelayInstalled");

    m_pressureFilter1Installed =
        readBool("Sensors/pressureFilter1Installed");

    m_pressureFilter2Installed =
        readBool("Sensors/pressureFilter2Installed");

    m_pressureFilter3Installed =
        readBool("Sensors/pressureFilter3Installed");

    m_drainFilterInstalled =
        readBool("Sensors/drainFilterInstalled");

    m_hydraulicTankLevelInstalled =
        readBool("Sensors/hydraulicTankLevelInstalled");
    m_hydraulicOilTemperatureInstalled =
        readBool("Sensors/hydraulicOilTemperatureInstalled");

    m_hydraulicDistributorPressureInstalled =
        readBool("Sensors/hydraulicDistributorPressureInstalled");

    m_hydraulicBroomPressureInstalled =
        readBool("Sensors/hydraulicBroomPressureInstalled");

    m_hydraulicFanPressureInstalled =
        readBool("Sensors/hydraulicFanPressureInstalled");

    m_hydraulicBroomPressPressureInstalled =
        readBool("Sensors/hydraulicBroomPressPressureInstalled");

    m_broomPositionSensors.up =
        readBool("PositionSensors/Broom/up");

    m_broomPositionSensors.down =
        readBool("PositionSensors/Broom/down");

    m_broomPositionSensors.left =
        readBool("PositionSensors/Broom/left");

    m_broomPositionSensors.right =
        readBool("PositionSensors/Broom/right");


    m_dumpPositionSensors.up =
        readBool("PositionSensors/Dump/up");

    m_dumpPositionSensors.down =
        readBool("PositionSensors/Dump/down");

    m_dumpPositionSensors.left =
        readBool("PositionSensors/Dump/left");

    m_dumpPositionSensors.right =
        readBool("PositionSensors/Dump/right");


    m_blowerPositionSensors.up =
        readBool("PositionSensors/Blower/up");

    m_blowerPositionSensors.down =
        readBool("PositionSensors/Blower/down");

    m_blowerPositionSensors.left =
        readBool("PositionSensors/Blower/left");

    m_blowerPositionSensors.right =
        readBool("PositionSensors/Blower/right");

    m_backMagnetPositionSensors.up =
        readBool("PositionSensors/BackMagnet/up");

    m_backMagnetPositionSensors.down =
        readBool("PositionSensors/BackMagnet/down");

    m_backMagnetPositionSensors.left =
        readBool("PositionSensors/BackMagnet/left");

    m_backMagnetPositionSensors.right =
        readBool("PositionSensors/BackMagnet/right");

    m_blowerUpSensor1Installed =
        readBool("PositionSensors/Blower/up1");

    m_blowerUpSensor2Installed =
        readBool("PositionSensors/Blower/up2");

    m_dumpPositionSensors.up =
        readBool("PositionSensors/Dump/up");

    m_dumpPositionSensors.down =
        readBool("PositionSensors/Dump/down");

    m_dumpPositionSensors.left =
        readBool("PositionSensors/Dump/left");

    m_dumpPositionSensors.right =
        readBool("PositionSensors/Dump/right");
    //-------------------------------------------

    m_broomFloatLicensed =
        readBool("License/broomFloat");

    m_broomPressLicensed =
        readBool("License/broomPress");

    m_dumpFloatLicensed =
        readBool("License/dumpFloat");
}

MachineModel Configuration::model() const
{
    return m_model;
}

QString Configuration::modelName() const
{
    switch (m_model) {
    case MachineModel::APPM200:
        return "АППМ-200";

    case MachineModel::APPM318D4:
        return "АППМ-318D4";

    case MachineModel::Airport:
        return "Аэродромная машина";

    case MachineModel::Unknown:
    default:
        return "Не задана";
    }
}

bool Configuration::hasEquipment(Equipment equipment) const
{
    switch (equipment) {
    case Equipment::CentralBroom:
        return m_centralBroomInstalled;

    case Equipment::FrontDump:
        return m_frontDumpInstalled;

    case Equipment::Blower:
        return m_blowerInstalled;

    case Equipment::BackMagnet:
        return m_backMagnetInstalled;

    case Equipment::BroomFloat:
        return m_broomFloatInstalled;

    case Equipment::BroomPress:
        return m_broomPressInstalled;

    case Equipment::DumpFloat:
        return m_dumpFloatInstalled;
    }

    return false;
}

bool Configuration::isFeatureLicensed(Feature feature) const
{
    switch (feature) {
    case Feature::BroomFloat:
        return m_broomFloatLicensed;

    case Feature::BroomPress:
        return m_broomPressLicensed;

    case Feature::DumpFloat:
        return m_dumpFloatLicensed;
    }

    return false;
}

bool Configuration::isBroomFloatAvailable() const
{
    return hasCentralBroom()
    && hasEquipment(Equipment::BroomFloat)
        && isFeatureLicensed(Feature::BroomFloat);
}

bool Configuration::isBroomPressAvailable() const
{
    return hasCentralBroom()
    && hasEquipment(Equipment::BroomPress)
        && isFeatureLicensed(Feature::BroomPress);
}

bool Configuration::isDumpFloatAvailable() const
{
    return hasFrontDump()
    && hasEquipment(Equipment::DumpFloat)
        && isFeatureLicensed(Feature::DumpFloat);
}

bool Configuration::hasCentralBroom() const
{
    return hasEquipment(Equipment::CentralBroom);
}

bool Configuration::hasFrontDump() const
{
    return hasEquipment(Equipment::FrontDump);
}

bool Configuration::hasBlower() const
{
    return hasEquipment(Equipment::Blower);
}

bool Configuration::hasBackMagnet() const
{
    return hasEquipment(Equipment::BackMagnet);
}

bool Configuration::hasSensor(Sensor sensor) const
{
    switch (sensor) {
    case Sensor::WaterInFuel:
        return m_waterInFuelInstalled;

    case Sensor::AirFilter:
        return m_airFilterInstalled;

    case Sensor::OilFilter:
        return m_oilFilterInstalled;

    case Sensor::HeatRelay:
        return m_heatRelayInstalled;

    case Sensor::PressureFilter1:
        return m_pressureFilter1Installed;

    case Sensor::PressureFilter2:
        return m_pressureFilter2Installed;

    case Sensor::PressureFilter3:
        return m_pressureFilter3Installed;

    case Sensor::DrainFilter:
        return m_drainFilterInstalled;

    case Sensor::HydraulicTankLevel:
        return m_hydraulicTankLevelInstalled;

    case Sensor::HydraulicOilTemperature:
        return m_hydraulicOilTemperatureInstalled;

    case Sensor::HydraulicDistributorPressure:
        return m_hydraulicDistributorPressureInstalled;

    case Sensor::HydraulicBroomPressure:
        return m_hydraulicBroomPressureInstalled;

    case Sensor::HydraulicFanPressure:
        return m_hydraulicFanPressureInstalled;

    case Sensor::HydraulicBroomPressPressure:
        return m_hydraulicBroomPressPressureInstalled;
    }
    return false;
}

bool Configuration::hasPositionSensor(organsEnums::Organ organ, organsEnums::Direction direction) const
{
    const PositionSensors* sensors = nullptr;

    switch (organ) {
    case organsEnums::Broom:
        sensors = &m_broomPositionSensors;
        break;

    case organsEnums::Dump:
        sensors = &m_dumpPositionSensors;
        break;

    case organsEnums::Blower:
        sensors = &m_blowerPositionSensors;
        break;
    case organsEnums::BackMagnet:
        sensors = &m_backMagnetPositionSensors;
        break;
    case organsEnums::BroomBlock:
    default:
        return false;
    }

    switch (direction) {
    case organsEnums::Up:
        return sensors->up;

    case organsEnums::Down:
        return sensors->down;

    case organsEnums::Left:
        return sensors->left;

    case organsEnums::Right:
        return sensors->right;

    case organsEnums::None:
    default:
        return false;
    }
}

bool Configuration::hasBlowerUpSensor1() const
{
    return m_blowerUpSensor1Installed;
}

bool Configuration::hasBlowerUpSensor2() const
{
    return m_blowerUpSensor2Installed;
}
