#include "mycanj1939.h"

#include <QDebug>

MyCanJ1939::MyCanJ1939(QString canName_, Logger * logger_, bool ableToRestart_, QObject *parent) : QObject(parent)
{
    canFailStatus = true;
    logger = logger_;
    canName = canName_;
    ableToRestart = ableToRestart_;
    incomeFailCounter = 0;
    sendFailCounter = 0;
    mThread = new QThread();
    moveToThread(mThread);
    connect(mThread, SIGNAL(started()), this, SLOT(CAN_init()));//, Qt::DirectConnection
    connect(this, SIGNAL(destroyed(QObject*)), mThread, SLOT(quit()));
    connect(mThread, SIGNAL(finished()), mThread, SLOT(deleteLater()));
    mThread->start();
}

void MyCanJ1939::CAN_init()
{
//    must init like this
//    /sbin/ip link set can1 type can bitrate 250000 restart-ms 1
//    ifconfig can1 up
//    /sbin/ip link set can1 j1939 on
//    ip addr add dev can1 j1939 0x80

    qDebug() << "j1939 init " << canName;

//    sock = socket(PF_CAN, SOCK_DGRAM, CAN_J1939);
//    memset(&jaddr, 0, sizeof(jaddr));
//    jaddr.can_family = AF_CAN;
////    jaddr.can_addr.j1939.addr = 0x80; // 0x07
//////    jaddr.can_addr.j1939.addr=J1939_IDLE_ADDR;
////    jaddr.can_addr.j1939.pgn = 0x00FECA;
//////    jaddr.can_addr.j1939.pgn=J1939_NO_PGN;
//    jaddr.can_addr.j1939.addr = J1939_NO_ADDR;      // принимать все адреса
//    jaddr.can_addr.j1939.pgn  = J1939_NO_PGN;       // принимать все PGN
//    jaddr.can_addr.j1939.name = J1939_NO_NAME;
//    jaddr.can_ifindex = if_nametoindex(canName.toStdString().c_str()); // ifindex is a substitute.
//    //memset(&jaddr, 0, sizeof(jaddr));
//    if (bind(sock, (struct sockaddr *)&jaddr, sizeof(jaddr)) < 0) {
//         perror("bind");
//     }
//    tv.tv_sec = 0;
//    tv.tv_usec = 1;
//    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
//    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));


//    filt.name = 0;
//    filt.name_mask = 0;
//    filt.addr = 0x00;
//    filt.addr_mask = 0x00;
//    filt.pgn = 0xFECA;
//    filt.pgn_mask = 0xFEDF;
//    setsockopt(sock, SOL_CAN_J1939, SO_J1939_FILTER, &filt, sizeof(filt));

    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    strcpy(ifr.ifr_name, canName.toStdString().c_str());
    ioctl(sock, SIOCGIFINDEX, &ifr);
    memset(&jaddr, 0, sizeof(jaddr));
    jaddr.can_family = AF_CAN; // обязательно на разбери!!!!
    jaddr.can_ifindex = ifr.ifr_ifindex;
    bind(sock, (struct sockaddr *)&jaddr, sizeof(jaddr));
    tv.tv_sec = 0;
    tv.tv_usec = 1;
    setsockopt(sock, SOL_SOCKET,SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET,SO_SNDTIMEO, (char*)&tv, sizeof(tv));

    mTimerRecv = new QTimer();
    connect(mTimerRecv,SIGNAL(timeout()),this,SLOT(canTimerTimeoutRecv()));
    mTimerRecv->setInterval(100);
    mTimerRecv->start();

    mTimerSend = new QTimer();
    connect(mTimerSend,SIGNAL(timeout()),this,SLOT(canTimerTimeoutSend()));
//    mTimerSend->setInterval(10);
//    mTimerSend->start();
}

void MyCanJ1939::CAN_reset()
{
    canFailStatus = true;
    mTimerRecv->stop();
//    mTimerSend->stop();
//    qDebug() << "j1939 reset " << canName;

    emit canError();

    incomeFailCounter = 0;
    sendFailCounter = 0;

    if (ableToRestart)
    {
        system(("ifconfig " + canName + " down").toStdString().c_str());
        system(("ifconfig " + canName + " up").toStdString().c_str());
    }
    mTimerRecv->start();
//    mTimerSend->start();
}

void MyCanJ1939::CAN_clear_frames()
{
//    for (int i = 0; i < 256; i++) {
//        dat_j1939[i] = 0;
//    }
    frame.can_id=0x00;
    frame.can_dlc=0;
    frame.data[0]=0x00;
    frame.data[1]=0x00;
    frame.data[2]=0x00;
    frame.data[3]=0x00;
    frame.data[4]=0x00;
    frame.data[5]=0x00;
    frame.data[6]=0x00;
    frame.data[7]=0x00;
}

void MyCanJ1939::canTimerTimeoutRecv()
{
    do
    {
        //socklen_t jaddrlen = sizeof(jaddr);
        CAN_clear_frames();
        //nbytes_j1939 = recvfrom(sock, dat_j1939, sizeof(dat_j1939), 0,
        //        (struct sockaddr *)&jaddr, &jaddrlen);
        nbytes_j1939 = read(sock,&frame,sizeof(struct can_frame));
        //qDebug() << "CAN read error:" << strerror(errno);
        if (nbytes_j1939!=-1)
        {
            //qDebug() << canName << "j1939 income bytes " << nbytes_j1939;
            //emit canDataReadyJ1939(jaddr.can_addr.j1939.pgn, jaddr.can_addr.j1939.addr, QByteArray((char*)dat_j1939, nbytes_j1939));
            emit canDataReadyJ1939((frame.can_id >> 8) & 0x0000FFFF, frame.can_id & 0x000000FF, QByteArray((char*)frame.data, frame.can_dlc));
            incomeFailCounter = 0;
            if (((frame.can_id >> 8) & 0x0000FFFF) == 0xF004)
                canFailStatus = false;// только если пакеты прям от двигла
        }
        else
            incomeFailCounter++;
    } while (nbytes_j1939!=-1);
    if (incomeFailCounter > 50)
        CAN_reset();
}

void MyCanJ1939::canTimerTimeoutSend()
{
    tsc1Mutex.lock();
    //canSend(tsc1.pgn, tsc1.sa, tsc1.dat_j1939, 8);
    tsc1Mutex.unlock();
}

void MyCanJ1939::setEngineCommand(quint16 rpm_need)
{
    quint16 tmp_rpm = getTsc1Byte(2) << 8;
    tmp_rpm = tmp_rpm | getTsc1Byte(1);
    if ( tmp_rpm != rpm_need)
        qDebug() << "Set rpm " << rpm_need / 8;
    setTsc1Byte(1, 0);
    setTsc1Byte(rpm_need, 1);
    setTsc1Byte(rpm_need >> 8, 2);
    setTsc1Byte(255, 3);
    setTsc1Byte(255, 4);
    setTsc1Byte(255, 5);
    setTsc1Byte(255, 6);
    setTsc1Byte(255, 7);
}

void MyCanJ1939::setTsc1Byte(quint8 tsc1Byte, int byteNumber)
{
    tsc1Mutex.lock();
    tsc1.dat_j1939[byteNumber] = tsc1Byte;
    tsc1Mutex.unlock();
}

quint8 MyCanJ1939::getTsc1Byte(int byteNumber)
{
    tsc1Mutex.lock();
    quint8 ret = tsc1.dat_j1939[byteNumber];
    tsc1Mutex.unlock();
    return ret;
}

void MyCanJ1939::canSend(quint32 pgn_, quint8 sa_, quint8* dat_j1939_, int data_j1939_length)
{
//    qDebug() << canName << " j1939 send ";
    int prio = 6;
    setsockopt(sock, SOL_CAN_J1939, SO_J1939_SEND_PRIO, &prio, sizeof(prio));
    jaddr.can_addr.j1939.addr = 0x00; // sa_ 0x00 - значит всем
    jaddr.can_addr.j1939.pgn = pgn_;
    socklen_t jaddrlen = sizeof(jaddr);
    int nbytes_send = sendto(sock, dat_j1939_, data_j1939_length, 0, (struct sockaddr *)&jaddr, jaddrlen);
    // для черного ящика
    logger->addLogInfo(Logger::canJ1939Data, pgn_, QByteArray((char*)dat_j1939_, data_j1939_length));
    if (nbytes_send<0)
        sendFailCounter++;
    if (sendFailCounter > 500)
        CAN_reset();
}
