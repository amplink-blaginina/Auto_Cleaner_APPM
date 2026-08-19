#ifndef VIEWCONTROLLER_H
#define VIEWCONTROLLER_H

#include <MessageList.h>
#include <qlabel.h>
#include <qwidget.h>
#include <screenlog.h>
#include <QDebug>
#include <logger.h>
#include <qpushbutton.h>


class ViewController:QObject{
        Q_OBJECT
public:

    enum BtnShapeType{
        Full,
        Vertical,
        Horizontal
    };

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
    void updateFRM(QPushButton *btn, bool state, QString key){
        //QString frmPath = "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_";
        setStyle(btn, frmPath + key + (state? "_on.png);": "_off.png);"));
    }
public slots:
    void messageListPressed();
private:
    QWidget *_parent;
    QString frmPath = "border-style:none;outline: none;background-image: url(:/Images/Images/main/buttons/light_button_frm_";


};

#endif // VIEWCONTROLLER_H
