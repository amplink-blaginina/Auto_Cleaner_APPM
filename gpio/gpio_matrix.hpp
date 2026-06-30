#pragma once
#include <QObject>
//#include <QThread>
#include <QMutex>
#include <QMap>
#include <QString>
#include <QTimer>
#include <gpiod.hpp>
#include "gpio_types.hpp"

class GPIOMatrix : public QObject
{
    Q_OBJECT

public:
    explicit GPIOMatrix(QObject *parent = nullptr);
    ~GPIOMatrix();

    GPIOInput keyPressed;

private:
    void configureHardware();
    void reconfigureCols(bool conf);
    bool readPhysicalInput(int pin);
    void writePhysicalOutput(int pin, bool value);

    void readRows();
    void readCols();

private:
    gpiod::chip chip;

    QTimer *timerCycle;

    int colPressed;
    int antiNoiseCounter;
    quint8 step;

    // линии
    QMap<int, gpiod::line> lines;

    // интервал между переключениями/опросами (ms)
    int pollIntervalMs = 10;
    bool tryGetLine(int key, gpiod::line &result);

public slots:
    void initCycle();
    void readCycle();
};
