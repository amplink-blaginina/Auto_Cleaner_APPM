// gpio_worker — опрос входов пульта и управление выходами (libgpiod 1.6.3)
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28
// изменения: см. gpio_worker.hpp
#include "gpio_worker.hpp"
#include <QThread>
#include <QObject>
#include <chrono>
#include <thread>
#include <QMutexLocker>
#include <QDebug>

GPIOWorker::GPIOWorker(QObject *parent)
    : QObject(parent)
{
    // без GPIO (симуляция, отладка на ПК) работаем без физических кнопок, а не падаем
    try{
        chip.open("/dev/gpiochip0");
    }
    catch (const std::exception& e){
        qWarning() << "GPIO недоступен, физические кнопки не работают:" << e.what();
    }
    // Настройка физики
    group0 = {
        // кнопка на пульте выкл зажигания
        {GPIOInput::IN_IGNITION_OFF,     12},
        // кнопка на пульте открыть бункер
        {GPIOInput::IN_OPEN_BUNKER,     26},
        // кнопка на пульте закрыть бункер
        {GPIOInput::IN_CLOSE_BUNKER,     20},
        // кнопка на пульте поднять бункер
        {GPIOInput::IN_LIFT_BUNKER,     19},
        // кнопка на пульте опутсть бункер
        {GPIOInput::IN_LOWER_BUNKER,     16},
        // кнопка на пульте ФРМ
        {GPIOInput::IN_FRM_PULT,     13},
//        // признак пульта на своем месте
//        {GPIOInput::IN_PULT_DOWN,     20},
//        // правые качельки вверх и вниз. поджатие щеток (без)
////        {GPIOInput::IN_PRESS_UP,     20},
//        {GPIOInput::IN_PRESS_DOWN,     20},
//        // кнопка включения габаритов на панели (фикс)
//        {GPIOInput::IN_GABARIT,     20},
//        // кнопка алиас переключения режима
//        {GPIOInput::IN_MODE,     20},
    };

    outputPins = {
        {GPIOOutput::OUT_STARTER,           6},
//        {GPIOOutput::OUT_STARTER_LIGHT,           26},
//        {GPIOOutput::OUT_VENTILATION,           26},
//        {GPIOOutput::OUT_MAYAK_LIGHT,           26},
//        {GPIOOutput::OUT_PVI_TEMP,           26},
    };

    initCycle();
}

GPIOWorker::~GPIOWorker()
{
}

void GPIOWorker::configureHardware()
{
    if (!chip)
        return;
    // ------- Запрос всех выходных линий -------
    for (auto it = outputPins.begin(); it != outputPins.end(); ++it) {
        if (!lines.contains(it.value()))
        {
            gpiod::line line = chip.get_line(it.value());
            line.request({"gpio-worker", gpiod::line_request::DIRECTION_OUTPUT, 0});
            lines[it.value()] = line;
            //line.set_value(0);
        }
        valuesOutput[it.key()] = false;
    }
    // ------- Запрос входов группы 0 -------
    for (auto it = group0.begin(); it != group0.end(); ++it) {
        if (!lines.contains(it.value()))
        {
            gpiod::line line = chip.get_line(it.value());
            line.request({"gpio-worker", gpiod::line_request::DIRECTION_INPUT, gpiod::line_request::FLAG_BIAS_PULL_UP});
            lines[it.value()] = line;
        }
        valuesInput[it.key()] = false;
    }
}

bool GPIOWorker::readPhysicalInput(int pin)
{
    if (!lines.contains(pin))
        return false;
    auto &line = lines[pin];
    return line.get_value();
}

void GPIOWorker::writePhysicalOutput(int pin, bool value)
{
    if (!lines.contains(pin))
        return;
    auto &line = lines[pin];
    line.set_value(value);
}

void GPIOWorker::readGroup()
{
    auto &src = group0;

    for (auto it = src.begin(); it != src.end(); ++it) {
        GPIOInput key = it.key();
        int pin = it.value();
        bool newVal = readPhysicalInput(pin);
        valuesInput[key] = newVal;

        // если вход подменен с экрана «GPIO ПУ» - отдаем подмену (в физических единицах)
        bool effective = newVal;
        if (overrideInput.contains(key))
            effective = isInvertedInput(key) ? !overrideInput.value(key) : overrideInput.value(key);

        if (emittedInput.value(key, false) != effective)
        {
            emittedInput[key] = effective;
            emit inputChanged(key, effective);
        }
    }
}

void GPIOWorker::initCycle()
{
    configureHardware();
    timerCycle = new QTimer();
    connect(timerCycle,SIGNAL(timeout()),this,SLOT(readCycle()));
    timerCycle->setInterval(pollIntervalMs);
    timerCycle->start();
}

void GPIOWorker::readCycle()
{
    timerCycle->stop();
    //readGroup(0);//постоянная группа
    readGroup();
    timerCycle->start();
}

bool GPIOWorker::isInvertedInput(GPIOInput input)
{// кнопки пульта подтянуты к питанию - нажата = 0
    return input == GPIOInput::IN_FRM_PULT
            || input == GPIOInput::IN_CLOSE_BUNKER
            || input == GPIOInput::IN_LIFT_BUNKER
            || input == GPIOInput::IN_LOWER_BUNKER
            || input == GPIOInput::IN_OPEN_BUNKER
            || input == GPIOInput::IN_IGNITION_OFF;
}

bool GPIOWorker::isInvertedOutput(GPIOOutput output)
{// инвертироавнные выходы
    return output == GPIOOutput::OUT_STARTER;
}

bool GPIOWorker::getInput(GPIOInput input)
{
    // подмена с экрана «GPIO ПУ» важнее реального входа
    if (overrideInput.contains(input))
        return overrideInput.value(input);

    if (isInvertedInput(input))
        return !valuesInput.value(input, false);
    else
        return valuesInput.value(input, false);
}

bool GPIOWorker::getOutput(GPIOOutput output)
{
    return valuesOutput.value(output, false);
}

void GPIOWorker::applyOutput(GPIOOutput output, bool logicalValue)
{
    // выходы без линии (закомментированные в outputPins) пропускаем,
    // иначе outputPins[output] добавит в карту пин 0
    if (!outputPins.contains(output))
        return;

    bool value = isInvertedOutput(output) ? !logicalValue : logicalValue;
    writePhysicalOutput(outputPins.value(output), value);
    valuesOutput[output] = value;
}

void GPIOWorker::setOutput(GPIOOutput output, bool value)
{
    // запоминаем желание логики, чтобы вернуть его после выхода из «GPIO ПУ»
    logicOutput[output] = value;

    if (testMode)
        return;// выходами сейчас управляет экран «GPIO ПУ»

    applyOutput(output, value);
}

// ---------------- экран «GPIO ПУ» ----------------

QList<GPIOWorker::PinState> GPIOWorker::pinStates()
{
    QList<PinState> res;

    // выходы
    for (auto it = outputPins.begin(); it != outputPins.end(); ++it)
    {
        if (!lines.contains(it.value()))
            continue;
        PinState st;
        st.pin = it.value();
        st.isOutput = true;
        st.physical = readPhysicalInput(st.pin);// для выхода get_value() отдает выставленный уровень
        st.logical = isInvertedOutput(it.key()) ? !st.physical : st.physical;
        st.overridden = false;
        res.append(st);
    }
    // входы
    for (auto it = group0.begin(); it != group0.end(); ++it)
    {
        if (!lines.contains(it.value()))
            continue;
        PinState st;
        st.pin = it.value();
        st.isOutput = false;
        st.physical = valuesInput.value(it.key(), false);
        st.logical = getInput(it.key());
        st.overridden = overrideInput.contains(it.key());
        res.append(st);
    }
    return res;
}

bool GPIOWorker::setInputOverride(int pin, bool logicalValue)
{
    for (auto it = group0.begin(); it != group0.end(); ++it)
    {
        if (it.value() == pin)
        {
            overrideInput[it.key()] = logicalValue;
            qDebug() << "GPIO PU: подмена входа" << pin << "=" << logicalValue;
            return true;
        }
    }
    return false;
}

void GPIOWorker::clearInputOverride(int pin)
{
    for (auto it = group0.begin(); it != group0.end(); ++it)
    {
        if (it.value() == pin && overrideInput.remove(it.key()))
            qDebug() << "GPIO PU: подмена входа" << pin << "снята";
    }
}

void GPIOWorker::clearAllInputOverrides()
{
    if (!overrideInput.isEmpty())
        qDebug() << "GPIO PU: все подмены входов сняты";
    overrideInput.clear();
}

void GPIOWorker::setTestMode(bool on)
{
    if (testMode == on)
        return;
    testMode = on;
    qDebug() << "GPIO PU: тестовый режим выходов" << on;

    if (!on)
    {// отдаем выходы логике: последнее, что она просила,
     // а если ничего не просила - исходный физический 0
        for (auto it = outputPins.begin(); it != outputPins.end(); ++it)
        {
            if (logicOutput.contains(it.key()))
                applyOutput(it.key(), logicOutput.value(it.key()));
            else
            {
                writePhysicalOutput(it.value(), false);
                valuesOutput[it.key()] = false;
            }
        }
    }
}

bool GPIOWorker::isTestMode()
{
    return testMode;
}

bool GPIOWorker::setOutputTest(int pin, bool logicalValue)
{
    if (!testMode)
        return false;

    for (auto it = outputPins.begin(); it != outputPins.end(); ++it)
    {
        if (it.value() == pin && lines.contains(pin))
        {
            applyOutput(it.key(), logicalValue);
            qDebug() << "GPIO PU: выход" << pin << "=" << logicalValue;
            return true;
        }
    }
    return false;
}
