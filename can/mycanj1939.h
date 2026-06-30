#ifndef MYCANJ1939_H
#define MYCANJ1939_H

#include <QObject>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>
#include <linux/can.h>
#include <sys/ioctl.h>
#include <linux/can/j1939.h>

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QByteArray>

#include "logger.h"

class MyCanJ1939 : public QObject
{
    Q_OBJECT
public:
    struct Tsc1SendCmd
    {
        quint32 pgn;
        quint8 sa;
        quint8 dat_j1939[8];
        Tsc1SendCmd()
        {
            sa = 0x80;
            pgn = 0;
            memset(dat_j1939, 0, 8);
        }
    };


    explicit MyCanJ1939(QString camName_, Logger * logger_, bool ableToRestart_, QObject *parent = nullptr);
    void CAN_reset();
    void CAN_clear_frames();
    void canSend(quint32 pgn_, quint8 sa_, quint8* dat_j1939_, int data_j1939_length);

    Logger * logger;

    bool ableToRestart;
    QString canName;
    int sock;
    struct timeval tv;
    struct sockaddr_can jaddr;

    int nbytes_j1939;
    quint8 dat_j1939[256];
    struct can_frame frame;
    j1939_filter filt;
    socklen_t lenght_j;
    struct ifreq ifr;
    bool canFailStatus;

    // тред для отделения в отдельный поток
    QThread *mThread;
    // таймер для приема и отправки
    QTimer *mTimerRecv;
    QTimer *mTimerSend;

    // считаем пропущенные приемы
    int incomeFailCounter;
    // считаем количество ошибок при отправке
    int sendFailCounter;

    // Блок для работы с командой tsc1 (все общение через мутекс)
    QMutex tsc1Mutex;
    struct Tsc1SendCmd tsc1;
    quint8 getTsc1Byte(int byteNumber);
    void setTsc1Byte(quint8 tsc1Byte, int byteNumber);
    void setEngineCommand(quint16 rpm_need);

public slots:
    void CAN_init();
    void canTimerTimeoutRecv();
    void canTimerTimeoutSend();
signals:
    void canDataReadyJ1939(quint32, quint8, QByteArray);
    void canError();
};

#endif // MYCANJ1939_H
