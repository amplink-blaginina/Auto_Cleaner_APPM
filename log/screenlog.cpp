#include "screenlog.h"
#include "qdebug.h"

#include <organsenums.h>

ScreenLog::ScreenLog() {}
void ScreenLog::printLog(const QString& msg) {
    qDebug()<<"print log: "<<msg;
    emit logSignal(msg);
}
void ScreenLog::printWarning(const QString& msg) {
    qDebug()<<"print warning: "<<msg;
    emit warningSignal(msg);
}
void ScreenLog::printError(const QString& msg) {
    qDebug()<<"print error: "<<msg;
    emit errorSignal(msg);
}

void ScreenLog::printMovementLog(organsEnums::Organ organ, organsEnums::Direction, QString additionalMsg) {
    printLog(getOrganText(organ)+": "+getMovementText(organsEnums::Down) + additionalMsg);
}

QString ScreenLog::getMovementText(organsEnums::Direction direction){
    switch (direction) {
    case organsEnums::Up:
        return "Движение вверх";
    case organsEnums::Down:
        return "Движение вниз";
    case organsEnums::Left:
        return "Движение влево" ;
    case organsEnums::Right:
        return "Движение вправо" ;
    default:
        return "Error";
    }
}

QString ScreenLog::getOrganText(organsEnums::Organ organ){
    switch (organ) {
    case organsEnums::BlowerOrgan:
        return "Обдув";
    case organsEnums::BroomOrgan:
        return "Щетка";
    case organsEnums::DumpOrgan:
        return "Отвал" ;
    default:
        return "Error";
    }
}

// QString ScreenLog::getFatalStatusMessage(){
//     if(!starterBlocked()){
//         return "Стартер не заблокирован";
//     }
//     if(starterLockedByRoll){
//         return "Стартер заблокирован: прокрутка";
//     }
//     if(starterLockedByTemperature){
//         return "Стартер заблокирован: требуется прогрев двигателя";
//     }
//     if(starterLockedByEmergency){
//         if(waterAlarm)
//             return "Стартер заблокирован: по датчику воды";
//         if(airAlarm)
//             return "Стартер заблокирован: по датчику воздуха";
//         if(oilAlarm)
//             return "Стартер заблокирован: по датчику масла";
//     }
//     if(starterNeedReboot){
//         return "Стартер заблокирован: требуется перезагрузка";
//     }
//     if(engine->waitOnStart){
//         return "Стартер заблокирован: ожидание на старте";
//     }
// }
