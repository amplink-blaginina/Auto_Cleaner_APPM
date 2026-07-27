#include "gpio_matrix.hpp"
#include <QThread>
#include <QObject>
#include <chrono>
#include <thread>
#include <QMutexLocker>
#include <QDebug>

const char ROWS = 5;
const char COLS = 4;
GPIOInput keys[ROWS][COLS] = {
    {GPIOInput::IN_STARTCLEAN,  GPIOInput::IN_STARTER,      GPIOInput::IN_DUMP_DOWN,    GPIOInput::IN_BROOM_DOWN},
    {GPIOInput::IN_RESERVE1,    GPIOInput::IN_PAUSE_HOME,   GPIOInput::IN_DUMP_UP,      GPIOInput::IN_BLOW_DOWN},
    {GPIOInput::IN_FRM,         GPIOInput::IN_RESERVE2,     GPIOInput::IN_BROOM_UP,     GPIOInput::IN_BLOW_UP},
    {GPIOInput::IN_MODE_LEFT,   GPIOInput::IN_DUMP_LEFT,    GPIOInput::IN_BROOM_LEFT,   GPIOInput::IN_BLOW_LEFT},
    {GPIOInput::IN_MODE_RIGHT,  GPIOInput::IN_DUMP_RIGHT,   GPIOInput::IN_BROOM_RIGHT,  GPIOInput::IN_BLOW_RIGHT}
};
char rowPins[ROWS] = {4, 17, 27, 22, 5};
char colPins[COLS] = {14, 15, 18, 24};

GPIOMatrix::GPIOMatrix(QObject *parent)
    : QObject(parent), chip("/dev/gpiochip0")
{
    initCycle();
}

GPIOMatrix::~GPIOMatrix()
{
}

void GPIOMatrix::configureHardware()
{
    // ------- Запрос всех выходных линий -------
    for (auto it = 0; it < COLS; it++)
    {// столбцы изначально входа подтянуты к нулю и в ненажатом состоянии 1
        if (!lines.contains(colPins[it]))
        {
            try{
                gpiod::line line = chip.get_line(colPins[it]);
                line.request({"gpio-worker", gpiod::line_request::DIRECTION_INPUT, gpiod::line_request::FLAG_BIAS_PULL_DOWN});
                lines[colPins[it]] = line;}
            catch (const std::system_error& e) {
                qWarning() << "GPIO unavailable in configureHardware:" << e.what();
                continue;
            } catch (const std::exception& e) {
                qWarning() << "GPIO init exception:" << e.what();
                continue;
            }
        }
    }
    // ------- Запрос входов группы 0 -------
    for (auto it = 0; it < ROWS; it++)
    {// строки всегда входа подтянуты к нулю и в ненажатом состоянии 1

        if (!lines.contains(rowPins[it]))
        {
            gpiod::line line = chip.get_line(rowPins[it]);
            line.request({"gpio-worker", gpiod::line_request::DIRECTION_INPUT, gpiod::line_request::FLAG_BIAS_PULL_DOWN});
            lines[rowPins[it]] = line;
        }
    }
}

bool GPIOMatrix::tryGetLine(int key, gpiod::line &result) {
    if (!lines.contains(key)) {
        return false;
    }
    result = lines[key];
    return true;
}

void GPIOMatrix::reconfigureCols(bool conf)
{
    for (auto it = 0; it < COLS; it++)
    {
        gpiod::line line;
        if(!tryGetLine(colPins[it], line)){
            continue;
        }
        //auto &line = lines[colPins[it]];
        //lines[colPins[it]].release();
        if (conf)
        {
            line.set_direction_output(0);
            //lines[colPins[it]].request({"gpio-worker", gpiod::line_request::DIRECTION_OUTPUT}, 0);
            //writePhysicalOutput(colPins[it], false);
        }
        else
        {
            line.set_direction_input();
            line.set_flags(gpiod::line_request::FLAG_BIAS_PULL_DOWN);
            //lines[colPins[it]].request({"gpio-worker", gpiod::line_request::DIRECTION_INPUT, gpiod::line_request::FLAG_BIAS_PULL_DOWN});
        }
    }
}

bool GPIOMatrix::readPhysicalInput(int pin)
{
    gpiod::line line;
    if(tryGetLine(pin, line)){
        return line.get_value();
    }
    return false;
}

void GPIOMatrix::writePhysicalOutput(int pin, bool value)
{
    gpiod::line line;
    if(tryGetLine(pin, line)){
        //line.request({"gpio-worker", gpiod::line_request::DIRECTION_OUTPUT, 0}, value ? 1 : 0);
        return line.set_value(value);
    }
}

void GPIOMatrix::readCols()
{
    for (auto it = 0; it < COLS; it++)
    {
        bool val = readPhysicalInput(colPins[it]);
        if (val)
        {// какая то кнопка нажата переходим в новый режим
            antiNoiseCounter++;
            //qDebug() << "col" << it;
            if (antiNoiseCounter > 2)
            {
                antiNoiseCounter = 0;
                colPressed = it;
                step = 1;// режим сканирования строк
                reconfigureCols(true); // делаем выходы из них
            }
            return;
        }
    }
    antiNoiseCounter = 0;
    keyPressed = GPIOInput::IN_NONE; // никакая не нажата
}

void GPIOMatrix::readRows()
{
    for (auto it = 0; it < ROWS; it++)
    {
        bool val = readPhysicalInput(rowPins[it]);
        if (!val)
        {// какая то кнопка нажата понимаем кнопку
            //qDebug() << "row" << it;
            //qDebug() << "key" << (int)keys[it][colPressed];
            keyPressed = keys[it][colPressed];
            step = 0;// режим сканирования столбцов
            reconfigureCols(false); // делаем входы из них
            return;
        }
    }
    step = 0;
    keyPressed = GPIOInput::IN_NONE; // никакая не нажата
}

void GPIOMatrix::initCycle()
{
    colPressed = 0;
    keyPressed = GPIOInput::IN_NONE;
    step = 0;
    antiNoiseCounter = 0;
    configureHardware();
    timerCycle = new QTimer();
    connect(timerCycle,SIGNAL(timeout()),this,SLOT(readCycle()));
    timerCycle->setInterval(pollIntervalMs);
    timerCycle->start();
}

void GPIOMatrix::readCycle()
{
    timerCycle->stop();
    if (step == 0)
        readCols();
    else if (step == 1)
        readRows();
    timerCycle->start();
}
