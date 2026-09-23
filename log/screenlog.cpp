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
void ScreenLog::printTest(const QString& msg) {
    qDebug()<<"print test: "<<msg;
    emit testSignal(msg);
}

void ScreenLog::printMovementLog(organsEnums::Organ organ, organsEnums::Direction dir, QString additionalMsg) {
    printLog(getOrganText(organ)+": "+getMovementText(dir) + additionalMsg);
}

void ScreenLog::printBusyLog(organsEnums::Organ organ){
    printLog(getOrganText(organ)+" в движении, ожидайте");
}
void ScreenLog::printDirectonSelectedLog(organsEnums::Organ organ, organsEnums::Direction side){
    printLog(getOrganText(organ)+": выбрана "+getSideText(side)+" сторона");
}

QString ScreenLog::getSideText(organsEnums::Direction direction){
    switch (direction) {
    case organsEnums::Left:
        return "левая" ;
    case organsEnums::Right:
        return "правая" ;
    default:
        return "Error";
    }
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
    case organsEnums::Blower:
        return "Обдув";
    case organsEnums::Broom:
        return "Щетка";
    case organsEnums::BroomBlock:
        return "Портал щетки";
    case organsEnums::Dump:
        return "Отвал" ;
    default:
        return "Error";
    }
}
