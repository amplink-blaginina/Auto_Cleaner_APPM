#ifndef ENGINE_H
#define ENGINE_H

#include <QObject>
#include <QWidget>
#include <QProgressBar>
#include <QLabel>

#include "can/mycanj1939.h"

#define ENGINE_ONLINE_EDGE 5

class Engine : public QObject
{
    Q_OBJECT
public:
    explicit Engine(MyCanJ1939* canj1939, QObject *parent_);
    QObject * parent;
    quint16 rpm;
    qint16 engineCoolantTemp;
    int engineCounterToShow;
    bool waitOnStart;
    quint8 damage;
    quint16 DM01SPNValue;
    quint8 DM01FMIValue;
    quint8 online;

    QTimer mainProgressTimer;
    bool coolantTempEverReceived;

    quint16 getRpm();

    // поиск своих параметров из CAN с надстройкой J1939
    bool parseCanJ1939(quint32 pgn, quint8 sa, QByteArray data);
public slots:
    // слот для получания данных из CAN
    void incomeData(quint32 pgn, quint8 sa, QByteArray data);
    void mainProgress();
signals:

};

#endif // ENGINE_H
