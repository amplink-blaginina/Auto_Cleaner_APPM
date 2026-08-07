#ifndef VIEWCONTROLLER_H
#define VIEWCONTROLLER_H

#include <MessageList.h>
#include <qlabel.h>
#include <qwidget.h>
#include <screenlog.h>
#include <QDebug>
#include <logger.h>


class ViewController:QObject{
        Q_OBJECT
public:

    enum LogStatus
    {
        InfoStatus      = 0,
        WarningStatus   = 1,
        FatalStatus     = 2,
        TestStatus      = 3
    };

    ViewController(QWidget *parent, Logger *logger);

    ScreenLog *screenLog;
    MessageList* messageList;

    void setStyle(QWidget *btn, QString path);
    void setText(QLabel *lbl, QString text);
    void addLog(QString msg, LogStatus logStatus);

    void addLog(QString msg);
    void addLogWarning(QString msg);
    void addLogError(QString msg);

    MessageList *createMessageList();

    Logger *_logger;
    MessageList *getMessageList();
    void printMovementLog(organsEnums::Organ organ, organsEnums::Direction dir, QString additionalMsg);
public slots:
    void messageListPressed();
private:
    QWidget *_parent;

};

#endif // VIEWCONTROLLER_H
