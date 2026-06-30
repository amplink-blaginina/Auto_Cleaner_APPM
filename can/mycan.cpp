#include "mycan.h"

#include <QDebug>

#define FAKE_OUT 0

MyCan::MyCan(QString canName_, Logger * logger_, bool ableToRestart_, QObject *parent) : QObject(parent)
{
    logger = logger_;
    canName = canName_;
    incomeFailCounter = 0;
    sendFailCounter = 0;
    incomeGOFailCounter = 0;
    incomePOFailCounter = 0;
    ableToRestart = ableToRestart_;
    failConfigureCounter = 0;
    haveSystemConfig = false;

    waterPumpInversion = false;

    // основная команда управления (управляет логикой и самой БУ_ЦП)
    BUCP.can_id = (quint32)0x0000A100|CAN_EFF_FLAG;
    BUCP.can_dlc = 8;
    memset(BUCP.data, 0, 8);
    BUCP2.can_id = (quint32)0x0000A200|CAN_EFF_FLAG;
    BUCP2.can_dlc = 8;
    memset(BUCP2.data, 0, 8);
    BUCP3.can_id = (quint32)0x0000A300|CAN_EFF_FLAG;
    BUCP3.can_dlc = 8;
    memset(BUCP3.data, 0, 8);

    // защита от самоубийства
    //setState(StatePVIPowerOut, true);
    //setState(StateBoardsPowerOut, true);
    //setState(StateBroomDistributor, true);

    mThread = new QThread();
    moveToThread(mThread);
    connect(mThread, SIGNAL(started()), this, SLOT(CAN_init()));//, Qt::DirectConnection
    connect(this, SIGNAL(destroyed(QObject*)), mThread, SLOT(quit()));
    connect(mThread, SIGNAL(finished()), mThread, SLOT(deleteLater()));
    mThread->start();

    firstInit = true;
    firstInitCounter = 0;

    last0000B100.can_id = 0;
    last0000B200.can_id = 0;
    last0000B300.can_id = 0;
    last0000B400.can_id = 0;
    last0000B500.can_id = 0;

    for (int i = 0; i < 8; i++)
    {
        b1Smooth.append(QList<quint8>());
        b1Raw.append(QList<quint8>());
        b2Smooth.append(QList<quint8>());
        b2Raw.append(QList<quint8>());
        b3Smooth.append(QList<quint8>());
        b3Raw.append(QList<quint8>());
        b4Smooth.append(QList<quint8>());
        b4Raw.append(QList<quint8>());
        b5Smooth.append(QList<quint8>());
        b5Raw.append(QList<quint8>());
    }
}

void MyCan::fillSystemConfigure(SystemConfigure* srcC, QMap<int, SystemElement*>* srcE)
{
    QMutexLocker l(&configureMutex);

    systemConfigure.clear();

    srcC->copy(&systemConfigure);
    foreach (int key, srcE->keys())
    {
        systemElements.insert(key, new SystemElement(srcE->value(key)));
    }

    configureStarted = false;
    BUCPConfigured = false;
    BUCPConfigureStep = 0;

    // заранее разобъем все каналы по пакетам и битам и байтам чтобы было быстрее искать
    // проходимся по битным параметрам
    quint8 frame_idx = 2;// команда B1 занята под общие нужды - начиаем с B2
    quint8 bit_idx = 0;
    quint8 byte_idx = 0;
    for (int i = 1; i < 9; i++)
    {
        for (int k = 0; k < 12; k++)
        {
            if (isBitIn(i, k)
                    || isBitOut(i, k))
            {
                systemConfigure.channelsBitId[i][k] = bit_idx;
                bit_idx++;
                systemConfigure.channelsByteId[i][k] = byte_idx;
                systemConfigure.channelsFrameId[i][k] = frame_idx;
            }
            if (bit_idx == 8)
            {// перелезли через край
                bit_idx = 0;
                byte_idx++;
            }
            if (byte_idx == 8)
            {// перелезли через край
                byte_idx = 0;
                frame_idx++;
            }
        }
    }
    // проходимся по байтным параметрам
    if (bit_idx != 0)
    {// неровно вышли
        bit_idx = 0;
        byte_idx++;
        if (byte_idx == 8)
        {// перелезли через край
            byte_idx = 0;
            frame_idx++;
        }
    }
    for (int i = 1; i < 9; i++)
    {
        for (int k = 0; k < 12; k++)
        {
            if (isByteIn(i, k)
                    || isByteOut(i, k))
            {
                systemConfigure.channelsBitId[i][k] = bit_idx;
                systemConfigure.channelsByteId[i][k] = byte_idx;
                byte_idx++;
                systemConfigure.channelsFrameId[i][k] = frame_idx;
            }
            if (byte_idx == 8)
            {// перелезли через край
                byte_idx = 0;
                frame_idx++;
            }
        }
    }
    // проходимся по двубайтным параметрам
    if (byte_idx >= 7)
    {// неровно вышли
        byte_idx = 0;
        frame_idx++;
    }
    for (int i = 1; i < 9; i++)
    {
        for (int k = 0; k < 12; k++)
        {
            if (is2ByteIn(i, k))
            {
                systemConfigure.channelsBitId[i][k] = bit_idx;
                systemConfigure.channelsByteId[i][k] = byte_idx;
                byte_idx++;
                byte_idx++;
                systemConfigure.channelsFrameId[i][k] = frame_idx;
            }
            if (byte_idx >= 7)
            {// неровно вышли
                byte_idx = 0;
                frame_idx++;
            }
        }
    }
    // и все должно уложиться в 5 команд B1 B2 B3 B4 B5 (хоть обосрись)

    // теперь надо собрать динамические команды A1 A2 (туда входят только ауты и шимы)
    frame_idx = 2;// команда A1 занята под общие нужды - начиаем с A2
    bit_idx = 0;
    byte_idx = 0;
    for (int i = 1; i < 9; i++)
    {
        for (int k = 0; k < 12; k++)
        {
            if (isBitOut(i, k))
            {
                systemConfigure.channelsOutBitId[i][k] = bit_idx;
                bit_idx++;
                systemConfigure.channelsOutByteId[i][k] = byte_idx;
                systemConfigure.channelsOutFrameId[i][k] = frame_idx;
            }
            if (bit_idx == 8)
            {// перелезли через край
                bit_idx = 0;
                byte_idx++;
            }
            if (byte_idx == 8)
            {// перелезли через край
                byte_idx = 0;
                frame_idx++;
            }
        }
    }
    // проходимся по байтным параметрам
    if (bit_idx != 0)
    {// неровно вышли
        bit_idx = 0;
        byte_idx++;
        if (byte_idx == 8)
        {// перелезли через край
            byte_idx = 0;
            frame_idx++;
        }
    }
    for (int i = 1; i < 9; i++)
    {
        for (int k = 0; k < 12; k++)
        {
            if (isByteOut(i, k))
            {
                systemConfigure.channelsOutBitId[i][k] = bit_idx;
                systemConfigure.channelsOutByteId[i][k] = byte_idx;
                byte_idx++;
                systemConfigure.channelsOutFrameId[i][k] = frame_idx;
            }
            if (byte_idx == 8)
            {// перелезли через край
                byte_idx = 0;
                frame_idx++;
            }
        }
    }

    haveSystemConfig = true;
}

bool MyCan::isHaveSystemConfig()
{
    QMutexLocker l(&configureMutex);
    return haveSystemConfig;
}

bool MyCan::isConfigured()
{
    bool ret;
    configureMutex.lock();
    ret = BUCPConfigured;
    configureMutex.unlock();
    return ret;
}

bool MyCan::isBitIn(quint8 i, quint8 k)
{
    if (systemConfigure.channelsType[i][k] == IN_MODE_NORMAL && (systemConfigure.boardsType[i] == BOARD_IN || systemConfigure.boardsType[i] == BOARD_IN_AN || systemConfigure.boardsType[i] == BOARD_IN_KM))
        return true;
    return false;
}

bool MyCan::isByteIn(quint8 i, quint8 k)
{
    if ((systemConfigure.channelsType[i][k] == IN_MODE_PWM || systemConfigure.channelsType[i][k] == IN_MODE_EXTI_8) && (systemConfigure.boardsType[i] == BOARD_IN || systemConfigure.boardsType[i] == BOARD_IN_AN || systemConfigure.boardsType[i] == BOARD_IN_KM))
        return true;
    if (systemConfigure.channelsType[i][k] == IN_MODE_ANALOG_8 && (systemConfigure.boardsType[i] == BOARD_IN_AN || systemConfigure.boardsType[i] == BOARD_IN_KM))
        return true;
    return false;
}

bool MyCan::is2ByteIn(quint8 i, quint8 k)
{
    if (systemConfigure.channelsType[i][k] == IN_MODE_EXTI_16 && (systemConfigure.boardsType[i] == BOARD_IN || systemConfigure.boardsType[i] == BOARD_IN_AN || systemConfigure.boardsType[i] == BOARD_IN_KM))
        return true;
    if (systemConfigure.channelsType[i][k] == IN_MODE_ANALOG_16 && (systemConfigure.boardsType[i] == BOARD_IN_AN || systemConfigure.boardsType[i] == BOARD_IN_KM))
        return true;
    return false;
}

bool MyCan::isBitOut(quint8 i, quint8 k)
{
    if (systemConfigure.channelsType[i][k] == OUT_MODE_NORMAL && (systemConfigure.boardsType[i] == BOARD_OUT || systemConfigure.boardsType[i] == BOARD_OUT_AN))
        return true;
    return false;
}

bool MyCan::isByteOut(quint8 i, quint8 k)
{
    if ((systemConfigure.channelsType[i][k] == OUT_MODE_PWM || systemConfigure.channelsType[i][k] == OUT_MODE_PFM || systemConfigure.channelsType[i][k] == OUT_MODE_FC) && (systemConfigure.boardsType[i] == BOARD_OUT || systemConfigure.boardsType[i] == BOARD_OUT_AN))
        return true;
    return false;
}

void MyCan::CAN_init()
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

    mTimerRecv = new QTimer();
    mTimerSend = new QTimer();

    connect(mTimerRecv,SIGNAL(timeout()),this,SLOT(canTimerTimeoutRecv()));
    mTimerRecv->setInterval(100);
    mTimerRecv->start();

    connect(mTimerSend,SIGNAL(timeout()),this,SLOT(canTimerTimeoutSend()));
    mTimerSend->setInterval(100);
    mTimerSend->start();
}
void MyCan::CAN_reset()
{
    mTimerRecv->stop();
    mTimerSend->stop();
//    qDebug() << "reset " << canName;

    emit canError();
    firstInit = true;
    firstInitCounter = 0;

    incomeFailCounter = 0;
    sendFailCounter = 0;
//    incomeGOFailCounter = 0;
//    incomePOFailCounter = 0;

    if (ableToRestart)
    {
        system(("ifconfig " + canName + " down").toStdString().c_str());
        system(("ifconfig " + canName + " up").toStdString().c_str());
    }
    mTimerRecv->start();
    mTimerSend->start();
}
void MyCan::CAN_clear_frames()
{
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

void MyCan::makeMedian(quint8* val, QList<QList<quint8>> &bRaw, QList<QList<quint8>> &bSmooth)
{
    for (int k = 0; k < 8; k++ )
    {
        bRaw[k].append(val[k]);
        if (bRaw[k].count() > MEDIAN_FILTER_SIZE)
            bRaw[k].removeFirst();
        else
            continue;
        // сортируем
        bSmooth[k].clear();
        for (int i = 0; i < MEDIAN_FILTER_SIZE; i++)
            bSmooth[k].append(bRaw[k][i]);
        for (int i = 0; i < MEDIAN_FILTER_SIZE; i++)
        {
            for (int j = 0; j < MEDIAN_FILTER_SIZE - i - 1; j++)
            {
                if (bSmooth[k][j] > bSmooth[k][j + 1])
                {
                    quint8 tmp = bSmooth[k][j];
                    bSmooth[k][j] = bSmooth[k][j + 1];
                    bSmooth[k][j + 1] = tmp;
                }
            }
        }
    }
}

void MyCan::canTimerTimeoutRecv()
{
    bool PO_ok = false;
    bool GO_ok = false;
    do
    {
        CAN_clear_frames();
        nbytes = read(sock,&frame,sizeof(struct can_frame));
        //qDebug() << "CAN read error:" << strerror(errno);
        if (nbytes!=-1)
        {
            //qDebug() << canName << " income bytes " << nbytes;
            frame.can_id&=0x3FFFFFFF;
            emit canDataReady(frame);
            incomeFailCounter = 0;
            if (frame.can_id == 0x0000B500 || frame.can_id == 0x0000B100 || frame.can_id == 0x0000B200 || frame.can_id == 0x0000B300 || frame.can_id == 0x0000B400)
            {
                PO_ok = true;
                GO_ok = true;
            }

            // êîìàíäà êîíôèãóðàöèè îò ÏÂÈ
            #define CAN_IN_CONFIGURE_EXTERNAL_ID  0x0000C100
            // êîìàíäà êîíôèãóðàöèè ê ÏÂÈ
            #define CAN_OUT_CONFIGURE_EXTERNAL_ID 0x0000C200
            // тут решаем сами - может начать конфигурироваться без пожелания плат (потому что мы знаем что не конфигурировались)
            configureMutex.lock();
            if (!BUCPConfigured && !configureStarted && haveSystemConfig)
            {
                BUCPConfigured = false;
                BUCPConfigureStep = 0;
                // высылаем первую командешку и составим список всех команд что мы пошлем
                configureStarted = true;
                frameConfig.can_id = (quint32)CAN_IN_CONFIGURE_EXTERNAL_ID|CAN_EFF_FLAG;
                frameConfig.can_dlc = 8;
                memset(frameConfig.data, 0, 8);
                frameConfig.data[0] = BUCPConfigureStep;
                frameConfig.data[1] = 98; // количество команд
                frameConfig.data[2] = systemConfigure.boardsType[1];
                frameConfig.data[3] = systemConfigure.boardsType[2];
                frameConfig.data[4] = systemConfigure.boardsType[3];
                frameConfig.data[5] = systemConfigure.boardsType[4];
                frameConfig.data[6] = systemConfigure.boardsType[5];
                frameConfig.data[7] = systemConfigure.boardsType[6];
                canSend(frameConfig);
                BUCPConfigureStep++;

                qDebug() << "start config BY ME";
            }

            if (frame.can_id == 0x0000B100)
            {// пришла команда от БУЦ с параметрами плат и желанием конфигурироваться (эта команда приходит всегда) там же 4 входа и выхода
                if (!configureStarted && frame.data[0] & 0x01 && haveSystemConfig)
                {// конфигурировать не начинали и ее хотят
                    BUCPConfigured = false;
                    BUCPConfigureStep = 0;
                    // высылаем первую командешку и составим список всех команд что мы пошлем
                    configureStarted = true;
                    frameConfig.can_id = (quint32)CAN_IN_CONFIGURE_EXTERNAL_ID|CAN_EFF_FLAG;
                    frameConfig.can_dlc = 8;
                    memset(frameConfig.data, 0, 8);
                    frameConfig.data[0] = BUCPConfigureStep;
                    frameConfig.data[1] = 98; // количество команд
                    frameConfig.data[2] = systemConfigure.boardsType[1];
                    frameConfig.data[3] = systemConfigure.boardsType[2];
                    frameConfig.data[4] = systemConfigure.boardsType[3];
                    frameConfig.data[5] = systemConfigure.boardsType[4];
                    frameConfig.data[6] = systemConfigure.boardsType[5];
                    frameConfig.data[7] = systemConfigure.boardsType[6];
                    canSend(frameConfig);
                    BUCPConfigureStep++;

                    qDebug() << "start config";
                }
            }

            if (frame.can_id == CAN_OUT_CONFIGURE_EXTERNAL_ID && haveSystemConfig)
            {// пришла команда ответ по конфигурации
                // сверимся что в ответ пришла такая же команда
                bool test = true;
                for (int i = 0; i < 8; i++)
                {
                    if (frameConfig.data[i] != frame.data[i])
                    {
                        test = false;
                        break;
                    }
                }
                if (test)
                {// ответ нас устроил
                    if (BUCPConfigureStep == 98)
                    {// вс отправлено
                        configureStarted = false;
                        BUCPConfigured = true;
                        qDebug() << "CONFIGURED";
                    }
                    else
                    {
                        frameConfig.can_id = (quint32)CAN_IN_CONFIGURE_EXTERNAL_ID|CAN_EFF_FLAG;
                        frameConfig.can_dlc = 8;
                        memset(frameConfig.data, 0, 8);
                        frameConfig.data[0] = BUCPConfigureStep;
                        if (BUCPConfigureStep == 1)
                        {// остатки плат
                            frameConfig.data[1] = systemConfigure.boardsType[7];
                            frameConfig.data[2] = systemConfigure.boardsType[8];
                        }
                        else
                        {// каналы
                            // переведем типы каналов чтобы не запутаться у себя (типы каналов )OUT ведутся с нуля)
                            if (systemConfigure.boardsType[(BUCPConfigureStep - 2) / 12 + 1] == BOARD_OUT || systemConfigure.boardsType[(BUCPConfigureStep - 2) / 12 + 1] == BOARD_OUT_AN)
                                frameConfig.data[1] = systemConfigure.channelsType[(BUCPConfigureStep - 2) / 12 + 1][((BUCPConfigureStep - 2) + 12) % 12] - 6;
                            else
                                frameConfig.data[1] = systemConfigure.channelsType[(BUCPConfigureStep - 2) / 12 + 1][((BUCPConfigureStep - 2) + 12) % 12];
                            frameConfig.data[2] = systemConfigure.channelsMedianSize[(BUCPConfigureStep - 2) / 12 + 1][((BUCPConfigureStep - 2) + 12) % 12] << 4;
                            frameConfig.data[2] |= systemConfigure.channelsPWMSize[(BUCPConfigureStep - 2) / 12 + 1][((BUCPConfigureStep - 2) + 12) % 12];
                            frameConfig.data[3] = systemConfigure.channelsLowPFM[(BUCPConfigureStep - 2) / 12 + 1][((BUCPConfigureStep - 2) + 12) % 12];
                            frameConfig.data[4] = systemConfigure.channelsHighPFM[(BUCPConfigureStep - 2) / 12 + 1][((BUCPConfigureStep - 2) + 12) % 12];
                        }
                        canSend(frameConfig);
                        BUCPConfigureStep++;
                    }
                }
            }
            configureMutex.unlock();

            // для черного ящика и для первой инициализации
            stateMutex.lock();
            if (frame.can_id == 0x0000B100)
            {
                last0000B100 = frame;
                makeMedian(frame.data, b1Raw, b1Smooth);
//                        qDebug() << "in1" << QByteArray((const char *)frame.data, 8).toHex();
            }
            if (frame.can_id == 0x0000B200)
            {
                last0000B200 = frame;
                makeMedian(frame.data, b2Raw, b2Smooth);
//                        qDebug() << "in2" << QByteArray((const char *)frame.data, 8).toHex();
            }
            if (frame.can_id == 0x0000B300)
            {
                last0000B300 = frame;
                makeMedian(frame.data, b3Raw, b3Smooth);
//                        qDebug() << "in3" << QByteArray((const char *)frame.data, 8).toHex();
            }
            if (frame.can_id == 0x0000B400)
            {
                last0000B400 = frame;
                makeMedian(frame.data, b4Raw, b4Smooth);
//                        qDebug() << "in4" << QByteArray((const char *)frame.data, 8).toHex();
            }
            if (frame.can_id == 0x0000B500)
            {
                last0000B500 = frame;
                makeMedian(frame.data, b5Raw, b5Smooth);
//                        qDebug() << "in4" << QByteArray((const char *)frame.data, 8).toHex();
            }
            //logger->addLogInfo(Logger::canData, frame.can_id, QByteArray((const char *)frame.data, 8));
            stateMutex.unlock();

            // для черного ящика и для первой инициализации
            if (frame.can_id == 0x0000B500 || frame.can_id == 0x0000B100 || frame.can_id == 0x0000B200 || frame.can_id == 0x0000B300 || frame.can_id == 0x0000B400)
            {
                if (firstInitCounter > 10)
                {
                }
                if (firstInit && firstInitCounter > 10)
                {// проверим есть ли конфликтные состояния
                    firstInit = false;
                }
                firstInitCounter++;
                logger->addLogInfo(Logger::canData, frame.can_id, QByteArray((const char *)frame.data, 8));
            }
        }
        else
            incomeFailCounter++;
    } while (nbytes!=-1);
    if (!BUCPConfigured)
    {
        failConfigureCounter++;
        if (failConfigureCounter > 20)
        {// конфижим заново
            configureStarted = false;
            failConfigureCounter = 0;
        }
    }
    if (!PO_ok)
        incomePOFailCounter++;
    else
        incomePOFailCounter = 0;
    if (!GO_ok)
        incomeGOFailCounter++;
    else
        incomeGOFailCounter = 0;

    if (incomeFailCounter > 10)
        CAN_reset();
    if (incomePOFailCounter > 10)
    {
        emit canPOError();
        firstInit = true;
        firstInitCounter = 0;
        incomePOFailCounter = 0;

        BUCPConfigured = false;
        stateMutex.lock();
        last0000B100.can_id = 0;
        last0000B200.can_id = 0;
        last0000B300.can_id = 0;
        last0000B400.can_id = 0;
        last0000B500.can_id = 0;
        for (int i = 0; i < 8; i++)
        {
            b1Smooth[i].clear();
            b1Raw[i].clear();
            b2Smooth[i].clear();
            b2Raw[i].clear();
            b3Smooth[i].clear();
            b3Raw[i].clear();
            b4Smooth[i].clear();
            b4Raw[i].clear();
            b5Smooth[i].clear();
            b5Raw[i].clear();
        }
        stateMutex.unlock();
    }
    if (incomeGOFailCounter > 10)
    {
        emit canGOError();
        firstInit = true;
        firstInitCounter = 0;
        incomeGOFailCounter = 0;
    }
}

void MyCan::canTimerTimeoutSend()
{
    if (firstInit)
        return;
    BUCPMutex.lock();
    canSend(BUCP);
    BUCPMutex.unlock();

    // изменение параметров для плавного хода
    for (int i = 1; i < 9; i++)
    {
        for (int k = 0; k < 12; k++)
        {
            if (systemConfigure.channelsValueChangeSpeed[i][k] != 0)
            {// плавный параметр
                qint16 val = getOriginalState(i, k).toInt();
                if (systemConfigure.channelsValueNeed[i][k] != val)
                {// идет смена
                    float shift = (float)systemConfigure.channelsValueChangeSpeed[i][k] / 10; // подсчет идет 10 раз в сек
                    if (systemConfigure.channelsValueNeed[i][k] > val )
                    {// увеличиваем
                        if (val + shift > systemConfigure.channelsValueNeed[i][k])
                        {
                            setOriginalState(i, k, (quint8)systemConfigure.channelsValueNeed[i][k], true);
                            systemConfigure.channelsValueCalculated[i][k] = systemConfigure.channelsValueNeed[i][k];
                        }
                        else
                        {
                            systemConfigure.channelsValueCalculated[i][k] += shift;
                            setOriginalState(i, k, (quint8)systemConfigure.channelsValueCalculated[i][k], true);
                        }
                    }
                    else if (systemConfigure.channelsValueNeed[i][k] < val )
                    {// уменьшаем
                        if (val - shift < systemConfigure.channelsValueNeed[i][k])
                        {
                            setOriginalState(i, k, (quint8)systemConfigure.channelsValueNeed[i][k], true);
                            systemConfigure.channelsValueCalculated[i][k] = systemConfigure.channelsValueNeed[i][k];
                        }
                        else
                        {
                            systemConfigure.channelsValueCalculated[i][k] -= shift;
                            setOriginalState(i, k, (quint8)systemConfigure.channelsValueCalculated[i][k], true);
                        }
                    }
                }
            }
        }
    }

    BUCP2Mutex.lock();
    canSend(BUCP2);
    BUCP2Mutex.unlock();

    BUCP3Mutex.lock();
    canSend(BUCP3);
    BUCP3Mutex.unlock();
}

void MyCan::setBUCPByte(quint8 BUCPByte, int byteNumber)
{
    BUCPMutex.lock();
    BUCP.data[byteNumber] = BUCPByte;
    BUCPMutex.unlock();
}

quint8 MyCan::getBUCPByte(int byteNumber)
{
    BUCPMutex.lock();
    quint8 ret = BUCP.data[byteNumber];
    BUCPMutex.unlock();
    return ret;
}

void MyCan::setBUCP2Byte(quint8 BUCPByte, int byteNumber)
{
    BUCP2Mutex.lock();
    BUCP2.data[byteNumber] = BUCPByte;
    BUCP2Mutex.unlock();
}

quint8 MyCan::getBUCP2Byte(int byteNumber)
{
    BUCP2Mutex.lock();
    quint8 ret = BUCP2.data[byteNumber];
    BUCP2Mutex.unlock();
    return ret;
}

void MyCan::setBUCP3Byte(quint8 BUCPByte, int byteNumber)
{
    BUCP3Mutex.lock();
    BUCP3.data[byteNumber] = BUCPByte;
    BUCP3Mutex.unlock();
}

quint8 MyCan::getBUCP3Byte(int byteNumber)
{
    BUCP3Mutex.lock();
    quint8 ret = BUCP3.data[byteNumber];
    BUCP3Mutex.unlock();
    return ret;
}

void MyCan::canSend(struct can_frame frame_)
{
    //qDebug() << canName << " send ";
    int nbytes_send = write(sock, &frame_, sizeof(frame_));
    // черный ящик
    logger->addLogInfo(Logger::canData, frame_.can_id, QByteArray((const char *)frame_.data, 8));
    if (nbytes_send<0)
        sendFailCounter++;
    if (sendFailCounter > 50)
        CAN_reset();
}

bool MyCan::isActive()
{// проверяет получена ли все параметры от плат для начала работ
    QMutexLocker l(&stateMutex);
    if (last0000B100.can_id != 0 && last0000B200.can_id != 0 &&
            b1Smooth[0].count() == MEDIAN_FILTER_SIZE && b2Smooth[0].count() == MEDIAN_FILTER_SIZE)
        return true;
    return false;
}

QVariant MyCan::getDataFromFrame(struct can_frame* frame, quint8 board_, quint8 channel_)
{
    if (systemConfigure.channelsType[board_][channel_] == IN_MODE_NORMAL
            || systemConfigure.channelsType[board_][channel_] == OUT_MODE_NORMAL)
    {
        if (FAKE_OUT && isBitOut(board_, channel_))
            return ((frame->data[systemConfigure.channelsOutByteId[board_][channel_]] & (1 << systemConfigure.channelsOutBitId[board_][channel_]))?true:false);
        else
            return ((frame->data[systemConfigure.channelsByteId[board_][channel_]] & (1 << systemConfigure.channelsBitId[board_][channel_]))?true:false);
    }

    if (systemConfigure.channelsType[board_][channel_] == IN_MODE_PWM
            || systemConfigure.channelsType[board_][channel_] == IN_MODE_EXTI_8
            || systemConfigure.channelsType[board_][channel_] == IN_MODE_ANALOG_8
            || systemConfigure.channelsType[board_][channel_] == OUT_MODE_PWM
            || systemConfigure.channelsType[board_][channel_] == OUT_MODE_FC
            || systemConfigure.channelsType[board_][channel_] == OUT_MODE_PFM)
    {
        if (FAKE_OUT && isByteOut(board_, channel_))
            return frame->data[systemConfigure.channelsOutByteId[board_][channel_]];
        else
            return frame->data[systemConfigure.channelsByteId[board_][channel_]];
    }
    if (systemConfigure.channelsType[board_][channel_] == IN_MODE_EXTI_16
            || systemConfigure.channelsType[board_][channel_] == IN_MODE_ANALOG_16)
        return ((quint16)frame->data[systemConfigure.channelsByteId[board_][channel_] + 1] << 8) + frame->data[systemConfigure.channelsByteId[board_][channel_]];
    return false;
}

QVariant MyCan::getOriginalState(quint8 board_, quint8 channel_)
{
    QMutexLocker l(&stateMutex);
    if (FAKE_OUT && (isBitOut(board_, channel_) || isByteOut(board_, channel_)))
    {
        if (systemConfigure.channelsOutFrameId[board_][channel_] == 2 && BUCP2.can_id != 0)
        {
            return getDataFromFrame(&BUCP2, board_, channel_);
        }
        if (systemConfigure.channelsOutFrameId[board_][channel_] == 3 && BUCP3.can_id != 0)
        {
            return getDataFromFrame(&BUCP3, board_, channel_);
        }
    }
    else
    {
        if (systemConfigure.channelsFrameId[board_][channel_] == 2 && last0000B200.can_id != 0 && b2Smooth[0].count() == MEDIAN_FILTER_SIZE)
        {
            return getDataFromFrame(&last0000B200, board_, channel_);
        }
        if (systemConfigure.channelsFrameId[board_][channel_] == 3 && last0000B300.can_id != 0 && b3Smooth[0].count() == MEDIAN_FILTER_SIZE)
        {
            return getDataFromFrame(&last0000B300, board_, channel_);
        }
        if (systemConfigure.channelsFrameId[board_][channel_] == 4 && last0000B400.can_id != 0 && b4Smooth[0].count() == MEDIAN_FILTER_SIZE)
        {
            return getDataFromFrame(&last0000B400, board_, channel_);
        }
        if (systemConfigure.channelsFrameId[board_][channel_] == 5 && last0000B500.can_id != 0 && b5Smooth[0].count() == MEDIAN_FILTER_SIZE)
        {
            return getDataFromFrame(&last0000B500, board_, channel_);
        }
    }
    return false;
}

void MyCan::toggleState(DeviceStates dev)
{
    setState(dev, !getState(dev).toBool());
}

QVariant MyCan::getState(DeviceStates dev, bool is_raw)
{
    QMutexLocker l(&stateMutex);
    if (dev == Board2Type)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[2] & 0x0F;
    if (dev == Board1Type)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return (last0000B100.data[2] >> 4) & 0x0F;
    if (dev == Board4Type)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[3] & 0x0F;
    if (dev == Board3Type)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return (last0000B100.data[3] >> 4) & 0x0F;
    if (dev == Board6Type)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[4] & 0x0F;
    if (dev == Board5Type)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return (last0000B100.data[4] >> 4) & 0x0F;
    if (dev == Board8Type)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[5] & 0x0F;
    if (dev == Board7Type)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return (last0000B100.data[5] >> 4) & 0x0F;
    if (dev == Board1Configured)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[1] & 0x01;
    if (dev == Board2Configured)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[1] & 0x02;
    if (dev == Board3Configured)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[1] & 0x04;
    if (dev == Board4Configured)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[1] & 0x08;
    if (dev == Board5Configured)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[1] & 0x10;
    if (dev == Board6Configured)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[1] & 0x20;
    if (dev == Board7Configured)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[1] & 0x40;
    if (dev == Board8Configured)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[1] & 0x80;
    if (dev == Board0IN1)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[6] & 0x01;
    if (dev == Board0IN2 || dev == StateAlarmIn)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[6] & 0x02;
    if (dev == Board0IN3 || dev == StatePVIPowerIn)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[6] & 0x04;
    if (dev == Board0IN4)// || dev == StateStarterIn)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[6] & 0x08;
    if (dev == Board0OUT1)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[6] & 0x10;
    if (dev == Board0OUT2 || dev == StateIgnitionOut)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[6] & 0x20;
    if (dev == Board0OUT3)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[6] & 0x40;
    if (dev == Board0OUT4)
        if (last0000B100.can_id != 0 && b1Smooth[0].count() == MEDIAN_FILTER_SIZE)
            return last0000B100.data[6] & 0x80;

    l.unlock();
    if (systemElements.contains(dev) && systemElements.value(dev)->board > 0 && BUCPConfigured)
    {
//        if (dev == StateOUTWaterPump && waterPumpInversion)
//            return !getOriginalState(systemElements[dev]->board, systemElements[dev]->channel).toBool();
//        else
            return getOriginalState(systemElements[dev]->board, systemElements[dev]->channel);
    }
    else
    {
        //if (!systemElements.contains(dev))
        //    qDebug() << "нет такого входа" << dev;
        return false;
    }
}

void MyCan::setOriginalState(quint8 board_, quint8 channel_, QVariant state, bool ignore_change_speed)
{
    QMutexLocker l(&stateMutex);
    if (!ignore_change_speed && systemConfigure.channelsValueChangeSpeed[board_][channel_] != 0)
    {// плавное изменение
        systemConfigure.channelsValueNeed[board_][channel_] = state.toInt();
        return;
    }
    if (systemConfigure.channelsOutFrameId[board_][channel_] == 2)
    {//A2
        if (systemConfigure.channelsType[board_][channel_] == OUT_MODE_NORMAL)
        {// битный
            setBUCP2Byte((state.toBool()?
                              getBUCP2Byte(systemConfigure.channelsOutByteId[board_][channel_]) | (1 << systemConfigure.channelsOutBitId[board_][channel_]):
                              getBUCP2Byte(systemConfigure.channelsOutByteId[board_][channel_]) & ~(1 << systemConfigure.channelsOutBitId[board_][channel_])), systemConfigure.channelsOutByteId[board_][channel_]);
        }
        else
        {// байтный
            setBUCP2Byte(state.toInt(), systemConfigure.channelsOutByteId[board_][channel_]);
        }
    }
    if (systemConfigure.channelsOutFrameId[board_][channel_] == 3)
    {//A3
        if (systemConfigure.channelsType[board_][channel_] == OUT_MODE_NORMAL)
        {// битный
            setBUCP3Byte((state.toBool()?
                              getBUCP3Byte(systemConfigure.channelsOutByteId[board_][channel_]) | (1 << systemConfigure.channelsOutBitId[board_][channel_]):
                              getBUCP3Byte(systemConfigure.channelsOutByteId[board_][channel_]) & ~(1 << systemConfigure.channelsOutBitId[board_][channel_])), systemConfigure.channelsOutByteId[board_][channel_]);
        }
        else
        {// байтный
            setBUCP3Byte(state.toInt(), systemConfigure.channelsOutByteId[board_][channel_]);
        }
    }
}

void MyCan::setWaterPumpInversion(bool inversion)
{
    BUCPMutex.lock();
    waterPumpInversion = inversion;
    BUCPMutex.unlock();
}

void MyCan::setState(DeviceStates dev, QVariant state, bool ignore_change_speed)
{
//    if (dev == StateOUTWaterPump)
//        state = (waterPumpInversion?(100 - state.toInt()):state.toInt());

    if (dev == StateIgnitionOut)
        setBUCPByte(state.toBool()?getBUCPByte(0) | 0x01:getBUCPByte(0) & ~(0x01), 0);
    else if (systemElements.contains(dev) && systemElements.value(dev)->board > 0 && BUCPConfigured)
        setOriginalState(systemElements.value(dev)->board, systemElements.value(dev)->channel, state, ignore_change_speed);
    //else if (!systemElements.contains(dev))
    //    qDebug() << "нет такого выхода" << dev;
}
