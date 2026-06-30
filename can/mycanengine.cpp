#include "mycanengine.h"

#include <QDebug>

MyCanEngine::MyCanEngine(QString canName_, Logger * logger_, bool ableToRestart_, QObject *parent) : QObject(parent)
{
    logger = logger_;
    canName = canName_;
    sendFailCounter = 0;
    ableToRestart = ableToRestart_;
    engineAddr = 7;

    engine.can_id = 0x18000007|CAN_EFF_FLAG; // был адрес 07 0x18000007
    engine.can_dlc = 8;
    memset(engine.data, 0, 8);

    rpmNeed = 0;
    rpmCur = 0;

    mThread = new QThread();
    moveToThread(mThread);
    connect(mThread, SIGNAL(started()), this, SLOT(CAN_init()));//, Qt::DirectConnection
    connect(this, SIGNAL(destroyed(QObject*)), mThread, SLOT(quit()));
    connect(mThread, SIGNAL(finished()), mThread, SLOT(deleteLater()));
    mThread->start();

    setEngineAddr(engineAddr);
}

void MyCanEngine::setEngineAddr(quint8 addr)
{
    engineMutex.lock();
    engineAddr = addr;
    if (engineAddr == 3)
        engine.can_id = 0x0C000003|CAN_EFF_FLAG;
    else if (engineAddr == 7)
        engine.can_id = 0x18000007|CAN_EFF_FLAG;
    else
        engine.can_id = 0x18000000|addr|CAN_EFF_FLAG;
    engineMutex.unlock();
}

void MyCanEngine::CAN_init()
{
    qDebug() << "init " << canName;

    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    strcpy(ifr.ifr_name, canName.toStdString().c_str());
    ioctl(sock, SIOCGIFINDEX, &ifr);
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN; // обязательно на разбери!!!!
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    tv.tv_sec = 0;
    tv.tv_usec = 1;
    setsockopt(sock, SOL_SOCKET,SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET,SO_SNDTIMEO, (char*)&tv, sizeof(tv));

    mTimerSend10 = new QTimer();
    connect(mTimerSend10,SIGNAL(timeout()),this,SLOT(canTimerTimeoutSend10()));
    mTimerSend10->setInterval(10);
    mTimerSend10->start();
}
void MyCanEngine::CAN_reset()
{
    mTimerSend10->stop();
//    qDebug() << "reset " << canName;

    emit canError();

    sendFailCounter = 0;

    if (ableToRestart)
    {
        system(("ifconfig " + canName + " down").toStdString().c_str());
        system(("ifconfig " + canName + " up").toStdString().c_str());
    }
    mTimerSend10->start();
}

void MyCanEngine::canTimerTimeoutSend10()
{
    if (rpmCur != 0)
    {
        // изменяем по 1 rpm в 10 мс
        if (rpmCur > 0)
            rpmCur -= 8;
        else
            rpmCur += 8;
        setRealEngineCommand(rpmNeed - rpmCur);
    }
    engineMutex.lock();
    canSend(engine);
    engineMutex.unlock();
}

void MyCanEngine::setEngineCommand(quint16 rpm_need)
{
    quint16 tmp_rpm = engine.data[2] << 8;
    tmp_rpm = tmp_rpm | engine.data[1];
    if (rpm_need > 0 && tmp_rpm == 0)
        setRealEngineCommand(rpm_need); // мгновенный старт
    else
    {
        rpmNeed = rpm_need;
        rpmCur = (((int)rpm_need - (int)tmp_rpm) / 16) * 16;
    }
}

void MyCanEngine::setRealEngineCommand(quint16 rpm_need)
{
    quint16 tmp_rpm = engine.data[2] << 8;
    tmp_rpm = tmp_rpm | engine.data[1];
    if ( tmp_rpm != rpm_need)
        qDebug() << "Set rpm " << rpm_need / 8;
    engineMutex.lock();
    engine.data[0]=0x01;
    engine.data[1]=rpm_need;
    engine.data[2]=rpm_need>>8;
    engine.data[3]=0xFF;
    engine.data[4]=0xFF;
    engine.data[5]=0xFF;
    engine.data[6]=0xFF;
    engine.data[7]=0xFF;
    engineMutex.unlock();
}

void MyCanEngine::canSend(struct can_frame frame_)
{
    //qDebug() << canName << " send ";
    int nbytes_send = write(sock, &frame_, sizeof(frame_));
    // черный ящик
    // todo закоментарил
    //logger->addLogInfo(Logger::canData, frame_.can_id, QByteArray((const char *)frame_.data, 8));
    if (nbytes_send<0)
        sendFailCounter++;
    if (sendFailCounter > 500)
        CAN_reset();
}

