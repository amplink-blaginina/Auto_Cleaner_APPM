#include "engine.h"
#include "mainwindow.h"

#include <QDebug>
#include <QTimer>
#include <QThread>

Engine::Engine(MyCanJ1939* canj1939, QObject *parent_) : QObject(parent_)
{
    parent = parent_;
    rpm = 0;
    engineCoolantTemp = 0;
    coolantTempEverReceived = false;
    waitOnStart = false;
    damage = 0;
    DM01SPNValue = 0;
    DM01FMIValue = 0;
    online = 0;

    connect(canj1939, SIGNAL(canDataReadyJ1939(quint32, quint8, QByteArray)), this, SLOT(incomeData(quint32, quint8, QByteArray)), Qt::QueuedConnection);
    connect(&mainProgressTimer, SIGNAL(timeout()), this, SLOT(mainProgress()));
    mainProgressTimer.start(500);
}

quint16 Engine::getRpm()
{
    return rpm;
}

void Engine::incomeData(quint32 pgn, quint8 sa, QByteArray data)
{
    parseCanJ1939(pgn, sa, data);
}

void Engine::mainProgress()
{
    if (online <= ENGINE_ONLINE_EDGE * 10)
        online++;
}

bool Engine::parseCanJ1939(quint32 pgn, quint8 sa, QByteArray data)
{
    //qDebug() << "pgn " << QString::number(pgn,16) << " size " << data.size();
    // разбираем команду dm01
    // разбираем команду SHUTDN
    if (pgn == 0xF004) //eec1
    {
        online = 0;
        quint16 rpm_ = data[3] + (data[4] << 8);
        rpm = rpm_ / 8;
    }
    if (pgn == 0xFEEE) //et1
    {
        engineCoolantTemp = data[0] - 40;
        coolantTempEverReceived = true;
    }
    if (pgn == 0xFEE4)
    {
        if (data[3]&0x01)
            waitOnStart = true;
        else
            waitOnStart = false;
    }
    if (pgn == 0xFECA)
    {
        //if (data.size() < 50)
        //    return false;
        //qDebug() << data.toHex();
        if (data[0] & 0x10)
            damage = 2;// heavy
        else if (data[0] & 0x04)
            damage = 1;// light
        else
            damage = 0;

        DM01SPNValue = (data[2] << 8) | data[3];
        DM01FMIValue = data[4];
    }
    return true;
}
