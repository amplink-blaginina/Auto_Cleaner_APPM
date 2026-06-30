#ifndef MYCANENGINE_H
#define MYCANENGINE_H

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>
#include <linux/can.h>
#include <sys/ioctl.h>

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>

#include "logger.h"

class MyCanEngine : public QObject
{
    Q_OBJECT
public:
    explicit MyCanEngine(QString camName_, Logger * logger_, bool ableToRestart_, QObject *parent = nullptr);

    void CAN_reset();
    void canSend(struct can_frame frame_);

    Logger * logger;

    bool ableToRestart;
    QString canName;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct timeval tv;
    int sock;
    int nbytes;

    // тред для отделения в отдельный поток
    QThread *mThread;
    // таймер для приема и отправки
    QTimer *mTimerSend10;

    int rpmNeed;
    int rpmCur;

    quint8 engineAddr;
    void setEngineAddr(quint8 addr);

    // считаем количество ошибок при отправке
    int sendFailCounter;

    QMutex engineMutex;
    struct can_frame engine;
    void setEngineCommand(quint16 rpm_need);
    void setRealEngineCommand(quint16 rpm_need);

public slots:
    void CAN_init();
    void canTimerTimeoutSend10();
signals:
    void canError();
};

#endif // MYCANENGINE_H
