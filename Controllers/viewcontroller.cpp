#include "viewcontroller.h"
#include "qscrollbar.h"

#include <logger.h>
#include <qdatetime.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qscroller.h>
#include <sys/socket.h>

ViewController::ViewController(QWidget *parent, Logger *logger) {
    screenLog = new ScreenLog();
    _parent = parent;
    _logger = logger;
    //_ui = ui;

    // верхний лог со скролом
    messageList = createMessageList();
    //WARNING
    QScroller::grabGesture(messageList->viewport(), QScroller::LeftMouseButtonGesture);
    connect(messageList, SIGNAL(entered(QModelIndex)), this, SLOT(messageListPressed()));
    connect(messageList, SIGNAL(viewportEntered()), this, SLOT(messageListPressed()));
    //ui->logLayout->addWidget(messageList);

    connect(screenLog, &ScreenLog::logSignal,
            this, [this](const QString& msg){ addLog(msg, InfoStatus); });
    connect(screenLog, &ScreenLog::warningSignal,
            this, [this](const QString& msg){ addLog(msg, WarningStatus); });
    connect(screenLog, &ScreenLog::errorSignal,
            this, [this](const QString& msg){ addLog(msg, FatalStatus); });
    connect(screenLog, &ScreenLog::testSignal,
            this, [this](const QString& msg){ addLog(msg, TestStatus); });

}

MessageList* ViewController::getMessageList(){
    return messageList;
}


void ViewController::setStyle(QWidget *widget, QString path){
    if (widget->styleSheet() != path)
        widget->setStyleSheet(path);
}

void ViewController::setText(QLabel *lbl, QString text){
    if (lbl->text() != text)
        lbl->setText(text);
}

void ViewController::addLog(QString text, LogStatus logStatus){
    qDebug() << "add Log" << text;
    QString log_text = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
    QColor color;
    if (logStatus == InfoStatus){
        color.setRgb(255, 255, 255);
        log_text += " [ INFO ]";
    }
    if (logStatus == WarningStatus){
        color.setRgb(255, 223, 0);
        log_text += " [ WARN ]";
    }
    if (logStatus == FatalStatus){
        color.setRgb(255, 0, 0);
        log_text += " [ FAIL ]";
    }
    if(logStatus == TestStatus){
        color.setRgb(0, 223, 223);
        log_text += " [ TEST ]";
    }

    messageList->addMessage(text,
                            color,
                            QPixmap(),
                            QDateTime::currentDateTime());

    _logger->addLogText(log_text + " " + text + '\n' + '\r');
}


MessageList* ViewController::createMessageList(){
    MessageList* messageList = new MessageList(_parent);
    messageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    messageList->verticalScrollBar()->setStyleSheet("QScrollBar {width:0px;}");
    messageList->setStyleSheet("QListView {background: transparent;}");
    messageList->setFixedWidth(519);
    messageList->setFixedHeight(115);
    return messageList;
}


void ViewController::messageListPressed(){// если касались списка лога, то не трогаем его еще 5 секунд, после этого он сам скролится вниз
    qDebug() << "pressed";
    messageList->manualControl = 50;
}

void ViewController::addLog(QString msg){
    addLog(msg, InfoStatus);
}

void ViewController::addLogWarning(QString msg){
    addLog(msg, WarningStatus);
}

void ViewController::addLogError(QString msg){
    addLog(msg, FatalStatus);
}
void ViewController::printMovementLog(organsEnums::Organ organ, organsEnums::Direction dir, QString additionalMsg) {
    screenLog->printMovementLog(organ, dir, additionalMsg);
}


