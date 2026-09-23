#ifndef SCREENLOG_H
#define SCREENLOG_H

#include "qobjectdefs.h"
#include <qstring.h>
#include <QObject>
#include <organsenums.h>

class ScreenLog: public QObject {
    Q_OBJECT
    public:
        ScreenLog();
        void printLog(const QString& msg);
        void printWarning(const QString& msg);
        void printError(const QString& msg);
        void printTest(const QString& msg);
        QString getOrganText(organsEnums::Organ organ);
        QString getMovementText(organsEnums::Direction direction);
        void printMovementLog(organsEnums::Organ organ, organsEnums::Direction, QString additionMsg = "");
        void printBusyLog(organsEnums::Organ organ);
        void printDirectonSelectedLog(organsEnums::Organ, organsEnums::Direction);
    signals:
        void logSignal(const QString& msg);
        void warningSignal(const QString& msg);
        void errorSignal(const QString& msg);
        void testSignal(const QString& msg);
    private:
        QString getSideText(organsEnums::Direction direction);
};

#endif // SCREENLOG_H
