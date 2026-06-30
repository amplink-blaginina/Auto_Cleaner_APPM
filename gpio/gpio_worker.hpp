#pragma once
#include <QObject>
//#include <QThread>
#include <QMutex>
#include <QMap>
#include <QString>
#include <QTimer>
#include <gpiod.hpp>
#include "gpio_types.hpp"

class GPIOWorker : public QObject
{
    Q_OBJECT

public:
    explicit GPIOWorker(QObject *parent = nullptr);
    ~GPIOWorker();

    // тред для отделения в отдельный поток
    //QThread *mThread;

    // getter (без блокировок для пользователя)
    bool getInput(GPIOInput input);
    bool getOutput(GPIOOutput output);

    // сеттеры для управления выходами из главного треда (потокобезопасно)
    void setOutput(GPIOOutput output, bool value);

signals:
    // опциональный сигнал, если хочешь реагировать на изменение входов
    void inputChanged(GPIOInput input, bool value);

private:
    void configureHardware();
    bool readPhysicalInput(int pin);
    void writePhysicalOutput(int pin, bool value);

    void readGroup();

private:
    gpiod::chip chip;

    QTimer *timerCycle;

    // рабочие выходы: имя -> pin
    QMap<GPIOOutput, int> outputPins;

    QMap<GPIOInput, int> group0;

    // линии
    QMap<int, gpiod::line> lines;

    // текущее состояние всех входов
    QMap<GPIOInput, bool> valuesInput;
    QMap<GPIOOutput, bool> valuesOutput;

    // интервал между переключениями/опросами (ms)
    int pollIntervalMs = 10;

public slots:
    void initCycle();
    void readCycle();
};
