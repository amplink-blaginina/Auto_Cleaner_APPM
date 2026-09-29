#include "settingsreader.h"
#include "qdebug.h"

#include <qdatetime.h>

SettingsReader::SettingsReader(QSettings *settings) {
    _settings = settings;
}

QVariant SettingsReader::readSettingsValue(QString name){// читает значение из настроек (если значения нет, то берет дефолтное)
    // @Invalid() в ini (записано раньше при отсутствии дефолта) считаем отсутствующим значением
    if (_settings->contains(name) && _settings->value(name).isValid()){

        return _settings->value(name);}
    else{

        _settings->setValue(name, _defaultValues.value(name));
    }
    return _defaultValues.value(name);
}

bool SettingsReader::isSettingsContainsValue(QString name){
    return _settings->contains(name);
}

void SettingsReader::setDefaults(){
    _defaultValues.insert("Global/canDeivce", "can1");
    _defaultValues.insert("Global/j1939Deivce", "can0");
    _defaultValues.insert("Global/password", "1234");
    _defaultValues.insert("Global/secretPassword", "51234");
    _defaultValues.insert("Global/brightness.level", 1);
    _defaultValues.insert("Global/restartIgnitionDelay", 60);
    _defaultValues.insert("Global/enableCleanSpeed", 50);// км/ч
    _defaultValues.insert("Global/disableCleanSpeed", 50);// км/ч
    _defaultValues.insert("Global/engineTempWarn", 90);// C
    _defaultValues.insert("Global/engineTempCrit", 100);// C
    _defaultValues.insert("Global/engineTempWarnTime", 5);// минут на охлаждение

    _defaultValues.insert("Global/hydroTempWarn", 50);// C
    _defaultValues.insert("Global/hydroTempCrit", 80);// C

    _defaultValues.insert("Dump/timeouts.DumpDownOut", 10);
    _defaultValues.insert("Dump/timeouts.DumpDownIn", 10);
    _defaultValues.insert("Dump/timeouts.DumpFlowOut", 2);
    _defaultValues.insert("Dump/timeouts.DumpSlideOut", 10);
    _defaultValues.insert("Dump/timeouts.DumpSlideIn", 10);
    _defaultValues.insert("Dump/timeouts.DumpBounceOut", "0.1");

    _defaultValues.insert("CentralBroom/timeouts.BroomDownOut", 5);
    _defaultValues.insert("CentralBroom/timeouts.BroomDownIn", 10);
    _defaultValues.insert("CentralBroom/timeouts.BroomFlowOut", 2);
    _defaultValues.insert("CentralBroom/timeouts.BroomSlideOut", 10);
    _defaultValues.insert("CentralBroom/timeouts.BroomSlideIn", 10);
    _defaultValues.insert("CentralBroom/timeouts.BroomRotateOut", 1);
    _defaultValues.insert("CentralBroom/timeouts.BroomRotateIn", 1);
    _defaultValues.insert("CentralBroom/timeouts.BroomBounceOut", "0.1");
    _defaultValues.insert("CentralBroom/speeds.LeafSweep", 70);
    _defaultValues.insert("CentralBroom/speeds.LightSweep", 80);
    _defaultValues.insert("CentralBroom/speeds.MediumSweep", 90);
    _defaultValues.insert("CentralBroom/speeds.HeavySweep", 100);

    _defaultValues.insert("BackMagnet/timeouts.BackMagnetDownOut", 10);
    _defaultValues.insert("BackMagnet/timeouts.BackMagnetDownIn", 15);

    _defaultValues.insert("Blower/timeouts.BlowerDownIn", 5);
    _defaultValues.insert("Blower/timeouts.BlowerDownOut", 5);
    _defaultValues.insert("Blower/timeouts.BlowerSlideIn", 5);
    _defaultValues.insert("Blower/timeouts.BlowerSlideOut", 5);
    _defaultValues.insert("Blower/timeouts.BlowerRotateIn", 1);
    _defaultValues.insert("Blower/timeouts.BlowerRotateOut", 1);
    _defaultValues.insert("Blower/speeds.LeafSweep", 70);
    _defaultValues.insert("Blower/speeds.LightSweep", 80);
    _defaultValues.insert("Blower/speeds.MediumSweep", 90);
    _defaultValues.insert("Blower/speeds.HeavySweep", 100);
    _defaultValues.insert("Blower/fanAccelRate", 10);

    _defaultValues.insert("Engine/rpm.None", 800);
    _defaultValues.insert("Engine/rpm.LeafSweep", 1100);
    _defaultValues.insert("Engine/rpm.LightSweep", 1400);
    _defaultValues.insert("Engine/rpm.MediumSweep", 1700);
    _defaultValues.insert("Engine/rpm.HeavySweep", 2000);
    _defaultValues.insert("Engine/addr", 7);
    _defaultValues.insert("Engine/rpm.dieselDefault", 1100); // дизельная скорость для регулирования
    _defaultValues.insert("Engine/temperature.electricWork", 20); // рабочая температура эл. двигателя
    _defaultValues.insert("Engine/temperature.dieselWork", 20); // рабочая температура диз. двигателя
    _defaultValues.insert("Engine/temperature.electricCrit", 100); // крит температура эл. двигателя
    _defaultValues.insert("Engine/temperature.dieselCrit", 100); // крит температура диз. двигателя
    _defaultValues.insert("Engine/hydroTempK", "1");
    _defaultValues.insert("Engine/hydroTempB", "-40");
    _defaultValues.insert("Engine/startRollRequiredDays", 5);
    _defaultValues.insert("Engine/startLowTemperatureEdge", -10);
    _defaultValues.insert("Engine/starterMaxWorkSec", 15);
    _defaultValues.insert("Engine/starterPauseSec", 60);
    _defaultValues.insert("Engine/starterMaxAttempts", 3);
    _defaultValues.insert("Engine/rollMaxWorkSec", 15);
    _defaultValues.insert("Engine/rollPauseSec", 60);
    _defaultValues.insert("Engine/rollMaxAttempts", 3);
    _defaultValues.insert("Engine/waterSensorRedHours", 2);
    _defaultValues.insert("Engine/airFilterRedHours", 20);
    _defaultValues.insert("Engine/waterSensorEmergencyMode", true);
    _defaultValues.insert("Engine/airFilterEmergencyMode", true);
    _defaultValues.insert("Engine/disableRollRequirement", false);
    _defaultValues.insert("Engine/disableTemperatureBlock", false);
    _defaultValues.insert("Engine/ignoreAllEmergency", false);
    _defaultValues.insert("Engine/lastStartDate", QDate::currentDate().addDays(-6));
    qDebug()<<"!!! defaultValues: "<<_defaultValues.value("Engine/lastStartDate");
    _defaultValues.insert("Global/hydraulicPressure1K", "1");
    _defaultValues.insert("Global/hydraulicPressure1B", "0");
    _defaultValues.insert("Global/hydraulicPressure2K", "1");
    _defaultValues.insert("Global/hydraulicPressure2B", "0");
    _defaultValues.insert("Global/hydraulicPressure3K", "1");
    _defaultValues.insert("Global/hydraulicPressure3B", "0");
    _defaultValues.insert("Global/hydraulicPressure4K", "1");
    _defaultValues.insert("Global/hydraulicPressure4B", "0");
    // инит главных счетчиков
    _defaultValues.insert("TOCur/Engine", 0);
    _defaultValues.insert("TOCur/EngineToday", 0); // срез на начало дня
    _defaultValues.insert("TOCur/DateToday", QDateTime::currentDateTime().date().addDays(-1)); // какая сегодня дата
    _defaultValues.insert("TOCur/System", 0);

    // ТО
    _defaultValues.insert("TO/EngineOil", 24 * 3600);
    _defaultValues.insert("TOCur/EngineOil", 0);

    //    defaultValues.insert("TO/PneumaticCheck", 24 * 3600);
    //    defaultValues.insert("TOCur/PneumaticCheck", 0);
    //    defaultValues.insert("TO/Hydraulic", 70 * 3600);
    //    defaultValues.insert("TOCur/Hydraulic", 0);
    //    defaultValues.insert("TO/Sharnirs", 24 * 3600);
    //    defaultValues.insert("TOCur/Sharnirs", 0);
    //    defaultValues.insert("TO/FanGear", 24 * 3600);
    //    defaultValues.insert("TOCur/FanGear", 0);
    _defaultValues.insert("TO/HydraulicOilCheck", 24 * 3600);
    _defaultValues.insert("TOCur/HydraulicOilCheck", 0);
    //    defaultValues.insert("TO/WorkCheck", 24 * 3600);
    //    defaultValues.insert("TOCur/WorkCheck", 0);
    //    defaultValues.insert("TO/WaterCheck", 24 * 3600);
    //    defaultValues.insert("TOCur/WaterCheck", 0);
    //    defaultValues.insert("TO/PneumaticJointCheck", 24 * 3600);
    //    defaultValues.insert("TOCur/PneumaticJointCheck", 0);
    //    defaultValues.insert("TO/FanWashing", 24 * 3600);
    //    defaultValues.insert("TOCur/FanWashing", 0);
    _defaultValues.insert("TO/CarLubrication", 100 * 3600);
    _defaultValues.insert("TOCur/CarLubrication", 0);
    _defaultValues.insert("TO/CarTightening", 100 * 3600);
    _defaultValues.insert("TOCur/CarTightening", 0);
    //    defaultValues.insert("TO/SomeCheck1", 100 * 3600);
    //    defaultValues.insert("TOCur/SomeCheck1", 0);
    //    defaultValues.insert("TO/BroomTightening", 100 * 3600);
    //    defaultValues.insert("TOCur/BroomTightening", 0);
    //    defaultValues.insert("TO/FanLubricant", 100 * 3600);
    //    defaultValues.insert("TOCur/FanLubricant", 0);
    //    defaultValues.insert("TO/HydraulicJointCheck", 100 * 3600);
    //    defaultValues.insert("TOCur/HydraulicJointCheck", 0);
    //    defaultValues.insert("TO/BackCoverSeal", 100 * 3600);
    //    defaultValues.insert("TOCur/BackCoverSeal", 0);
    //    defaultValues.insert("TO/OilFilterInHydroTank", 100 * 3600);
    //    defaultValues.insert("TOCur/OilFilterInHydroTank", 0);
    //    defaultValues.insert("TO/HydraulicOilChange", 100 * 3600);
    //    defaultValues.insert("TOCur/HydraulicOilChange", 0);
    //    defaultValues.insert("TO/ElectricCheck", 100 * 3600);
    //    defaultValues.insert("TOCur/ElectricCheck", 0);
    //    defaultValues.insert("TO/HydroTankWash", 100 * 3600);
    //    defaultValues.insert("TOCur/HydroTankWash", 0);
    _defaultValues.insert("TO/EngineTO", 100 * 3600);
    _defaultValues.insert("TOCur/EngineTO", 0);
    //    defaultValues.insert("TO/SleavesCheck", 500 * 3600);
    //    defaultValues.insert("TOCur/SleavesCheck", 0);
    _defaultValues.insert("TO/PressureFilterChange", 500 * 3600);
    _defaultValues.insert("TOCur/PressureFilterChange", 0);
    //    defaultValues.insert("TO/WaterCheck2", 500 * 3600);
    //    defaultValues.insert("TOCur/WaterCheck2", 0);

}


void SettingsReader::updateStartDate(QDate value){
    qDebug()<<"!!! updateStartDate: "<<value;
    _settings->setValue("Engine/lastStartDate", value);
    _settings->sync();
}
