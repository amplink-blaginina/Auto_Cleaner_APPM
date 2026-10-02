// gpio_worker — опрос входов пульта и управление выходами (libgpiod 1.6.3)
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28; RPi 4B, Raspbian 12 armhf, Qt 5.15.8
// изменения: добавлен блок для экрана «GPIO ПУ» — состояние линий по номеру, подмена входов,
// тестовый режим выходов; поведение для остального кода не изменилось
#pragma once
#include <QObject>
//#include <QThread>
#include <QMutex>
#include <QMap>
#include <QList>
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

    // ---------- для экрана «GPIO ПУ» (вызывать только из главного треда) ----------
    // состояние одной линии, занятой воркером
    struct PinState
    {
        int pin;            // номер BCM
        bool isOutput;      // выход или вход
        bool physical;      // уровень на ноге
        bool logical;       // значение, как его видит логика (с учетом инверсии и подмены)
        bool overridden;    // вход подменен с экрана
    };
    // все линии воркера (и входы и выходы)
    QList<PinState> pinStates();

    // подмена входа: логика видит logicalValue вместо реального уровня
    bool setInputOverride(int pin, bool logicalValue);
    void clearInputOverride(int pin);
    void clearAllInputOverrides();

    // тестовый режим: пока включен, setOutput() от логики в ноги не пишет,
    // выходами управляет только экран через setOutputTest()
    void setTestMode(bool on);
    bool isTestMode();
    bool setOutputTest(int pin, bool logicalValue);

signals:
    // опциональный сигнал, если хочешь реагировать на изменение входов
    void inputChanged(GPIOInput input, bool value);

private:
    void configureHardware();
    bool readPhysicalInput(int pin);
    void writePhysicalOutput(int pin, bool value);

    void readGroup();

    // входы/выходы с инверсией (кнопки пульта подтянуты к питанию, стартер инверсный)
    bool isInvertedInput(GPIOInput input);
    bool isInvertedOutput(GPIOOutput output);
    // запись выхода в логических единицах
    void applyOutput(GPIOOutput output, bool logicalValue);

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

    // экран «GPIO ПУ»
    QMap<GPIOInput, bool> overrideInput;    // подмененные входы (логическое значение)
    QMap<GPIOInput, bool> emittedInput;     // что последний раз отдали в inputChanged
    QMap<GPIOOutput, bool> logicOutput;     // что последний раз просила логика
    bool testMode = false;

    // интервал между переключениями/опросами (ms)
    int pollIntervalMs = 10;

public slots:
    void initCycle();
    void readCycle();
};
