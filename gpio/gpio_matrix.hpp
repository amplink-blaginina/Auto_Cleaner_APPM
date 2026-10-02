// gpio_matrix — опрос матричной клавиатуры 5x4 (libgpiod 1.6.3)
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28; база - версия с RPi (с tryGetLine)
// изменения: добавлен блок для экрана «GPIO ПУ»
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

class GPIOMatrix : public QObject
{
    Q_OBJECT

public:
    explicit GPIOMatrix(QObject *parent = nullptr);
    ~GPIOMatrix();

    GPIOInput keyPressed;

    // ---------- для экрана «GPIO ПУ» (вызывать только из главного треда) ----------
    // раскладка клавиатуры
    static int rows();
    static int cols();
    static GPIOInput keyAt(int row, int col);

    // состояние линии клавиатуры
    struct LineState
    {
        int pin;        // номер BCM
        bool isOutput;  // столбцы на время сканирования строк становятся выходами
        bool value;     // уровень на ноге
    };
    QList<LineState> lineStates();

    // нажатие кнопки с экрана: логика видит key в keyPressed; IN_NONE - отпустить
    void setKeyOverride(GPIOInput key);

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

    // кнопка, нажатая на экране «GPIO ПУ»
    GPIOInput keyOverride = GPIOInput::IN_NONE;

    // интервал между переключениями/опросами (ms)
    int pollIntervalMs = 10;
    bool tryGetLine(int key, gpiod::line &result);

public slots:
    void initCycle();
    void readCycle();
};
