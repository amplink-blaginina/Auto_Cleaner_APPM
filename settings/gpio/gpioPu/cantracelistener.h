// cantracelistener — прием одной линии CAN для листинга на экране «GPIO ПУ»
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28; SocketCAN CAN_RAW, rtnetlink, Qt 5.15.8
// изменения: переписан по образцу MyCan (QObject в своем QThread, таймеры приема и состояния)
#ifndef CANTRACELISTENER_H
#define CANTRACELISTENER_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QAtomicInt>
#include <QSharedPointer>

#include "cantracebuffer.h"

// только чтение: свой сокет CAN_RAW, на работу MyCan никак не влияет;
// кадры, отправленные этим же RPi, помечаются как Tx;
// интерфейс может пропадать/появляться - переподключаемся сами;
// раз в секунду спрашиваем у ядра состояние контроллера и битрейт
class CanTraceListener : public QObject
{
    Q_OBJECT
public:
    enum BusState
    {
        BusNoInterface = 0, // интерфейса нет в системе
        BusDown,            // интерфейс опущен
        BusActive,          // ERROR-ACTIVE
        BusWarning,         // ERROR-WARNING
        BusPassive,         // ERROR-PASSIVE
        BusOff              // BUS-OFF
    };

    explicit CanTraceListener(QString canName_, QSharedPointer<CanTraceBuffer> buffer_, QObject *parent = nullptr);
    ~CanTraceListener();

    QString canName;

    // читаются из главного треда
    int getState();
    quint32 getBitrate();
    quint32 getDrops();  // потери в ядре (переполнение очереди сокета)

    // тред для отделения в отдельный поток
    QThread *mThread;
    // таймеры приема и опроса состояния
    QTimer *mTimerRecv;
    QTimer *mTimerState;

public slots:
    void CAN_init();
    void canTimerTimeoutRecv();
    void canTimerTimeoutState();

private:
    bool openSocket();
    void closeSocket();

    QSharedPointer<CanTraceBuffer> buffer;
    int sock;           // CAN_RAW
    int nlSock;         // rtnetlink для состояния
    quint32 nlSeq;

    QAtomicInt state;
    QAtomicInt bitrate;
    QAtomicInt drops;
};

#endif // CANTRACELISTENER_H
