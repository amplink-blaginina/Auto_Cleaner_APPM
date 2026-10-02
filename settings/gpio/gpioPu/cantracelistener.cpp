// cantracelistener — прием одной линии CAN для листинга на экране «GPIO ПУ»
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28
#include "cantracelistener.h"

#include <QDebug>
#include <QDateTime>
#include <QVector>

#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/netlink.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_link.h>

#ifndef SO_RXQ_OVFL
#define SO_RXQ_OVFL 40
#endif

// сколько кадров вычитываем за один тик таймера приема
#define RECV_BATCH 2048

CanTraceListener::CanTraceListener(QString canName_, QSharedPointer<CanTraceBuffer> buffer_, QObject *parent) : QObject(parent)
{
    canName = canName_;
    buffer = buffer_;
    sock = -1;
    nlSock = -1;
    nlSeq = 1;
    state = BusNoInterface;
    bitrate = 0;
    drops = 0;
    mTimerRecv = nullptr;
    mTimerState = nullptr;

    // как в MyCan: объект живет в своем треде, удаляется через deleteLater()
    mThread = new QThread();
    moveToThread(mThread);
    connect(mThread, &QThread::started, this, &CanTraceListener::CAN_init);
    connect(this, &QObject::destroyed, mThread, &QThread::quit);
    connect(mThread, &QThread::finished, mThread, &QObject::deleteLater);
    mThread->start();
}

CanTraceListener::~CanTraceListener()
{// вызывается в треде приема (deleteLater)
    closeSocket();
    if (nlSock >= 0)
        close(nlSock);
    qDebug() << "GPIO PU: прием" << canName << "остановлен";
}

int CanTraceListener::getState()
{
    return state.loadAcquire();
}

quint32 CanTraceListener::getBitrate()
{
    return quint32(bitrate.loadAcquire());
}

quint32 CanTraceListener::getDrops()
{
    return quint32(drops.loadAcquire());
}

void CanTraceListener::CAN_init()
{
    qDebug() << "GPIO PU: прием" << canName << "запущен";

    // кадры копятся в сокете с метками времени ядра, поэтому 10 мс опроса хватает
    mTimerRecv = new QTimer(this);
    connect(mTimerRecv, &QTimer::timeout, this, &CanTraceListener::canTimerTimeoutRecv);
    mTimerRecv->start(10);

    mTimerState = new QTimer(this);
    connect(mTimerState, &QTimer::timeout, this, &CanTraceListener::canTimerTimeoutState);
    mTimerState->start(1000);

    canTimerTimeoutState();
}

bool CanTraceListener::openSocket()
{
    unsigned int index = if_nametoindex(canName.toStdString().c_str());
    if (index == 0)
        return false;// интерфейса нет

    sock = socket(PF_CAN, SOCK_RAW | SOCK_CLOEXEC, CAN_RAW);
    if (sock < 0)
    {
        qWarning() << "GPIO PU:" << canName << "socket:" << strerror(errno);
        return false;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = int(index);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        qWarning() << "GPIO PU:" << canName << "bind:" << strerror(errno);
        closeSocket();
        return false;
    }

    int on = 1;
    setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on));   // время приема от ядра
    setsockopt(sock, SOL_SOCKET, SO_RXQ_OVFL, &on, sizeof(on));    // счетчик потерь
    int rcvBuf = 1 << 20;// ограничено net.core.rmem_max
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvBuf, sizeof(rcvBuf));

    qDebug() << "GPIO PU: сокет" << canName << "открыт";
    return true;
}

void CanTraceListener::closeSocket()
{
    if (sock >= 0)
    {
        close(sock);
        sock = -1;
    }
}

void CanTraceListener::canTimerTimeoutRecv()
{
    if (sock < 0)
        return;// ждем, пока canTimerTimeoutState() откроет сокет

    QVector<CanTraceFrame> batch;
    batch.reserve(256);

    for (int i = 0; i < RECV_BATCH; i++)
    {
        struct can_frame frame;
        char control[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(quint32))];
        struct iovec iov;
        iov.iov_base = &frame;
        iov.iov_len = sizeof(frame);
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = control;
        msg.msg_controllen = sizeof(control);

        ssize_t n = recvmsg(sock, &msg, MSG_DONTWAIT);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                break;// все вычитали
            if (errno == ENETDOWN)
                break;// интерфейс опустили (MyCan делает ifconfig down/up) - сокет живой
            qWarning() << "GPIO PU:" << canName << "recvmsg:" << strerror(errno) << "- переоткрываем";
            closeSocket();// ENODEV и прочее
            break;
        }
        if (n < ssize_t(sizeof(struct can_frame)) || (frame.can_id & CAN_ERR_FLAG))
            continue;// кадры ошибок не пишем

        CanTraceFrame f;
        memset(&f, 0, sizeof(f));

        // время и потери из служебных данных сокета
        for (struct cmsghdr *c = CMSG_FIRSTHDR(&msg); c; c = CMSG_NXTHDR(&msg, c))
        {
            if (c->cmsg_level != SOL_SOCKET)
                continue;
            if (c->cmsg_type == SCM_TIMESTAMP)
            {
                struct timeval tv;
                memcpy(&tv, CMSG_DATA(c), sizeof(tv));
                f.timeUs = qint64(tv.tv_sec) * 1000000 + tv.tv_usec;
            }
            else if (c->cmsg_type == SO_RXQ_OVFL)
            {
                quint32 d;
                memcpy(&d, CMSG_DATA(c), sizeof(d));
                drops = int(d);
            }
        }
        if (f.timeUs == 0)
            f.timeUs = QDateTime::currentMSecsSinceEpoch() * 1000;

        if (frame.can_id & CAN_EFF_FLAG)
        {
            f.id = frame.can_id & CAN_EFF_MASK;
            f.flags |= CAN_TRACE_EXT;
        }
        else
            f.id = frame.can_id & CAN_SFF_MASK;
        if (frame.can_id & CAN_RTR_FLAG)
            f.flags |= CAN_TRACE_RTR;
        if (msg.msg_flags & MSG_DONTROUTE)
            f.flags |= CAN_TRACE_TX;// кадр ушел с этого RPi
        f.dlc = frame.can_dlc > 8 ? 8 : frame.can_dlc;
        memcpy(f.data, frame.data, 8);

        batch.append(f);
    }

    if (!batch.isEmpty())
        buffer->append(batch.constData(), batch.size());
}

void CanTraceListener::canTimerTimeoutState()
{// состояние контроллера и битрейт через rtnetlink (как ip -d link show)
    unsigned int index = if_nametoindex(canName.toStdString().c_str());
    if (index == 0)
    {
        state = BusNoInterface;
        bitrate = 0;
        closeSocket();
        return;
    }

    if (sock < 0)
        openSocket();

    if (nlSock < 0)
    {
        nlSock = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
        if (nlSock < 0)
            return;
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000;
        setsockopt(nlSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    // запрос RTM_GETLINK по индексу интерфейса
    struct
    {
        struct nlmsghdr nh;
        struct ifinfomsg ifi;
    } req;
    memset(&req, 0, sizeof(req));
    req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.nh.nlmsg_type = RTM_GETLINK;
    req.nh.nlmsg_flags = NLM_F_REQUEST;
    req.nh.nlmsg_seq = ++nlSeq;
    req.ifi.ifi_family = AF_UNSPEC;
    req.ifi.ifi_index = int(index);
    if (send(nlSock, &req, req.nh.nlmsg_len, 0) < 0)
        return;

    alignas(struct nlmsghdr) char answer[16384];
    int len = int(recv(nlSock, answer, sizeof(answer), 0));
    if (len <= 0)
        return;

    for (struct nlmsghdr *nh = (struct nlmsghdr *)answer; NLMSG_OK(nh, (unsigned int)len); nh = NLMSG_NEXT(nh, len))
    {
        if (nh->nlmsg_seq != nlSeq)
            continue;
        if (nh->nlmsg_type == NLMSG_ERROR)
        {
            state = BusNoInterface;
            return;
        }
        if (nh->nlmsg_type != RTM_NEWLINK)
            continue;

        struct ifinfomsg *ifi = (struct ifinfomsg *)NLMSG_DATA(nh);
        bool up = ifi->ifi_flags & IFF_UP;
        int canState = -1;
        quint32 canBitrate = 0;

        // IFLA_LINKINFO -> IFLA_INFO_DATA -> IFLA_CAN_STATE / IFLA_CAN_BITTIMING
        int alen = IFLA_PAYLOAD(nh);
        for (struct rtattr *a = IFLA_RTA(ifi); RTA_OK(a, alen); a = RTA_NEXT(a, alen))
        {
            if ((a->rta_type & NLA_TYPE_MASK) != IFLA_LINKINFO)
                continue;
            int blen = int(RTA_PAYLOAD(a));
            for (struct rtattr *b = (struct rtattr *)RTA_DATA(a); RTA_OK(b, blen); b = RTA_NEXT(b, blen))
            {
                if ((b->rta_type & NLA_TYPE_MASK) != IFLA_INFO_DATA)
                    continue;
                int clen = int(RTA_PAYLOAD(b));
                for (struct rtattr *c = (struct rtattr *)RTA_DATA(b); RTA_OK(c, clen); c = RTA_NEXT(c, clen))
                {
                    int type = c->rta_type & NLA_TYPE_MASK;
                    if (type == IFLA_CAN_STATE && RTA_PAYLOAD(c) >= sizeof(quint32))
                        canState = int(*(quint32 *)RTA_DATA(c));
                    else if (type == IFLA_CAN_BITTIMING && RTA_PAYLOAD(c) >= sizeof(struct can_bittiming))
                        canBitrate = ((struct can_bittiming *)RTA_DATA(c))->bitrate;
                }
            }
        }

        if (!up)
            state = BusDown;
        else
        {
            switch (canState)
            {
            case CAN_STATE_ERROR_ACTIVE:  state = BusActive;  break;
            case CAN_STATE_ERROR_WARNING: state = BusWarning; break;
            case CAN_STATE_ERROR_PASSIVE: state = BusPassive; break;
            case CAN_STATE_BUS_OFF:       state = BusOff;     break;
            case -1:                      state = BusActive;  break;// не CAN-драйвер (vcan)
            default:                      state = BusDown;    break;// STOPPED/SLEEPING
            }
        }
        bitrate = int(canBitrate);
        return;
    }
}
