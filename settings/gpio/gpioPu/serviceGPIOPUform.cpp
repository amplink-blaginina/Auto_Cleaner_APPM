// serviceGPIOPUform — экран «GPIO TESTING ПУ»: GPIO пульта, температура, листинг CAN, клавиатура
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28
#include "serviceGPIOPUform.h"
#include "ui_serviceGPIOPUform.h"

#include "mainwindow.h"
#include "trcwriter.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QSettings>
#include <QFontMetrics>
#include <QDebug>

// картинки экрана
#define GPIO_PU_IMG ":/Images/Images/settings/gpioPu/"
// удержание входа дольше - снять подмену
#define GPIO_PU_LONG_PRESS_MS 800

ServiceGPIOPUForm::ServiceGPIOPUForm(MyCan* can, MainWindow* mainWindow, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServiceGPIOPUForm)
{
    ui->setupUi(this);
    _can = can;
    _mainWindow = mainWindow;
    _parent = parent;
    _gpio = _mainWindow->gpio;
    _gpioMatrix = _mainWindow->gpioMatirx;

    keyboardMode = false;
    saving = false;

    // экран одноразовый - при закрытии удаляется сам
    setAttribute(Qt::WA_DeleteOnClose);
    qDebug() << "GPIO PU: экран открыт";

    // пока экран открыт, выходами управляет только он
    if (_gpio)
        _gpio->setTestMode(true);

    // ---- GPIO: кнопки из .ui, служебных линий (0,1,2,3,23,25) в разметке нет ----
    try
    {
        freeChip.open("/dev/gpiochip0");
    }
    catch (const std::exception &e)
    {
        qWarning() << "GPIO PU: gpiochip0:" << e.what();
    }
    addPin(ui->pushButton_pin4, 4);
    addPin(ui->pushButton_pin17, 17);
    addPin(ui->pushButton_pin27, 27);
    addPin(ui->pushButton_pin22, 22);
    addPin(ui->pushButton_pin5, 5);
    addPin(ui->pushButton_pin6, 6);
    addPin(ui->pushButton_pin13, 13);
    addPin(ui->pushButton_pin19, 19);
    addPin(ui->pushButton_pin26, 26);
    addPin(ui->pushButton_pin14, 14);
    addPin(ui->pushButton_pin15, 15);
    addPin(ui->pushButton_pin18, 18);
    addPin(ui->pushButton_pin24, 24);
    addPin(ui->pushButton_pin12, 12);
    addPin(ui->pushButton_pin16, 16);
    addPin(ui->pushButton_pin20, 20);
    addPin(ui->pushButton_pin21, 21);

    // ---- клавиатура 5x4 (раскладка как в GPIOMatrix) ----
    addKey(ui->pushButton_key_0_0, 0, 0);
    addKey(ui->pushButton_key_0_1, 0, 1);
    addKey(ui->pushButton_key_0_2, 0, 2);
    addKey(ui->pushButton_key_0_3, 0, 3);
    addKey(ui->pushButton_key_1_0, 1, 0);
    addKey(ui->pushButton_key_1_1, 1, 1);
    addKey(ui->pushButton_key_1_2, 1, 2);
    addKey(ui->pushButton_key_1_3, 1, 3);
    addKey(ui->pushButton_key_2_0, 2, 0);
    addKey(ui->pushButton_key_2_1, 2, 1);
    addKey(ui->pushButton_key_2_2, 2, 2);
    addKey(ui->pushButton_key_2_3, 2, 3);
    addKey(ui->pushButton_key_3_0, 3, 0);
    addKey(ui->pushButton_key_3_1, 3, 1);
    addKey(ui->pushButton_key_3_2, 3, 2);
    addKey(ui->pushButton_key_3_3, 3, 3);
    addKey(ui->pushButton_key_4_0, 4, 0);
    addKey(ui->pushButton_key_4_1, 4, 1);
    addKey(ui->pushButton_key_4_2, 4, 2);
    addKey(ui->pushButton_key_4_3, 4, 3);

    // ---- CAN: интерфейсы берем из тех же настроек, что и MainWindow ----
    QSettings settings(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini", QSettings::IniFormat);
    QString can1Name = settings.value("Global/canDeivce", "can1").toString();       // линия БУЦ (MyCan)
    QString can2Name = settings.value("Global/j1939Deivce", "can0").toString();     // двигатель (J1939)
    openTimeUs = QDateTime::currentMSecsSinceEpoch() * 1000;
    initCan(0, can1Name, ui->label_can1Indicator, ui->label_can1Info, ui->pushButton_can1Pause, ui->frame_can1List);
    initCan(1, can2Name, ui->label_can2Indicator, ui->label_can2Info, ui->pushButton_can2Pause, ui->frame_can2List);
    connect(canView[0], &CanTraceView::pauseRequested, this, &ServiceGPIOPUForm::can1PauseRequested);
    connect(canView[1], &CanTraceView::pauseRequested, this, &ServiceGPIOPUForm::can2PauseRequested);

    setKeyboardMode(false);

    connect(&mainProgressTimer, &QTimer::timeout, this, &ServiceGPIOPUForm::mainProgress);
    connect(&temperatureTimer, &QTimer::timeout, this, &ServiceGPIOPUForm::temperatureProgress);
    statusTimer.setSingleShot(true);
    connect(&statusTimer, &QTimer::timeout, ui->label_status, &QLabel::clear);

    temperatureProgress();
    mainProgress();

    mainProgressTimer.start(100);
    temperatureTimer.start(1000);
}

ServiceGPIOPUForm::~ServiceGPIOPUForm()
{
    mainProgressTimer.stop();
    temperatureTimer.stop();

    // все возвращаем логике машины
    if (_gpioMatrix)
        _gpioMatrix->setKeyOverride(GPIOInput::IN_NONE);
    if (_gpio)
    {
        _gpio->clearAllInputOverrides();
        _gpio->setTestMode(false);
    }

    // свободные линии отпускаем
    for (auto it = freeLines.begin(); it != freeLines.end(); ++it)
    {
        try
        {
            it.value().release();
        }
        catch (...)
        {
        }
    }
    freeLines.clear();

    // прием CAN: объект живет в своем треде - удаляем через deleteLater(),
    // главный тред не ждет (даже если ядро где-то зависло)
    for (int i = 0; i < 2; i++)
        canListener[i]->deleteLater();

    qDebug() << "GPIO PU: экран закрыт";
    delete ui;
}

// ---------------------------------------------------------------- GPIO

void ServiceGPIOPUForm::addPin(QPushButton *button, int pin)
{// определяем, кто владеет линией, и подключаем нажатия
    PinKind kind = PinUnavailable;

    bool isWorker = false;
    if (_gpio)
        for (const GPIOWorker::PinState &st : _gpio->pinStates())
            if (st.pin == pin)
                isWorker = true;

    bool isMatrix = false;
    if (_gpioMatrix)
        for (const GPIOMatrix::LineState &st : _gpioMatrix->lineStates())
            if (st.pin == pin)
                isMatrix = true;

    if (isWorker)
        kind = PinWorker;
    else if (isMatrix)
        kind = PinMatrix;
    else
    {// никем не занята - берем как вход, только для индикации
        try
        {
            gpiod::line line = freeChip.get_line(pin);
            line.request({"gpio-pu", gpiod::line_request::DIRECTION_INPUT, 0});
            freeLines[pin] = line;
            kind = PinFree;
        }
        catch (const std::exception &e)
        {
            qWarning() << "GPIO PU: линия" << pin << "недоступна:" << e.what();
        }
    }

    button->setProperty("pin", pin);
    button->setProperty("kind", kind);
    connect(button, &QPushButton::pressed, this, &ServiceGPIOPUForm::pinPressed);
    connect(button, &QPushButton::released, this, &ServiceGPIOPUForm::pinReleased);
    pins.append(button);
}

void ServiceGPIOPUForm::pinPressed()
{
    pinPressTimer.start();
}

void ServiceGPIOPUForm::pinReleased()
{// вход: тап - подменить (повторный - инвертировать подмену), долгое нажатие - снять подмену
 // выход: тап - переключить
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button || !_gpio || button->property("kind").toInt() != PinWorker)
        return;// клавиатура и свободные линии - только индикация

    int pin = button->property("pin").toInt();
    bool longPress = pinPressTimer.isValid() && pinPressTimer.elapsed() >= GPIO_PU_LONG_PRESS_MS;

    for (const GPIOWorker::PinState &st : _gpio->pinStates())
    {
        if (st.pin != pin)
            continue;
        if (st.isOutput)
            _gpio->setOutputTest(pin, !st.logical);
        else if (longPress)
            _gpio->clearInputOverride(pin);
        else
            _gpio->setInputOverride(pin, !st.logical);
        break;
    }
    checkPins();
}

void ServiceGPIOPUForm::checkPins()
{// раскраска: вход - серый/зеленый, выход - черный/фиолетовый, подмененный вход - желтый номер
    QMap<int, GPIOWorker::PinState> worker;
    QMap<int, GPIOMatrix::LineState> matrix;
    if (_gpio)
        for (const GPIOWorker::PinState &st : _gpio->pinStates())
            worker[st.pin] = st;
    if (_gpioMatrix)
        for (const GPIOMatrix::LineState &st : _gpioMatrix->lineStates())
            matrix[st.pin] = st;

    for (int i = 0; i < pins.size(); i++)
    {
        int pin = pins[i]->property("pin").toInt();
        int kind = pins[i]->property("kind").toInt();
        QString image = "indication_gray";
        QString color = "255,255,255";

        if (kind == PinWorker && worker.contains(pin))
        {
            GPIOWorker::PinState st = worker.value(pin);
            if (st.isOutput)
                image = st.logical ? "indication_violet" : "indication_black";
            else
                image = st.logical ? "indication_green" : "indication_gray";
            if (st.overridden)
                color = "255,213,74";
        }
        else if (kind == PinMatrix && matrix.contains(pin))
        {// столбцы клавиатуры на время сканирования строк становятся выходами - это не показываем
            image = matrix.value(pin).value ? "indication_green" : "indication_gray";
        }
        else if (kind == PinFree)
        {
            bool value = false;
            try
            {
                value = freeLines[pin].get_value();
            }
            catch (...)
            {
            }
            image = value ? "indication_green" : "indication_gray";
        }
        else
            color = "122,128,135";// недоступна

        _mainWindow->getView()->setStyle(pins[i], "border-style:none;outline: none;border-image:url(" GPIO_PU_IMG + image
                                         + ".png);color: rgb(" + color + ");font: 15px \"Montserrat\";");
    }
}

// ---------------------------------------------------------------- клавиатура

void ServiceGPIOPUForm::addKey(QPushButton *button, int row, int col)
{
    button->setProperty("key", static_cast<int>(GPIOMatrix::keyAt(row, col)));
    connect(button, &QPushButton::pressed, this, &ServiceGPIOPUForm::keyPressed);
    connect(button, &QPushButton::released, this, &ServiceGPIOPUForm::keyReleased);
    keys.append(button);
}

void ServiceGPIOPUForm::keyPressed()
{// удержание кнопки на экране = нажатие этой кнопки для логики
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (button && _gpioMatrix)
        _gpioMatrix->setKeyOverride(static_cast<GPIOInput>(button->property("key").toInt()));
}

void ServiceGPIOPUForm::keyReleased()
{
    if (_gpioMatrix)
        _gpioMatrix->setKeyOverride(GPIOInput::IN_NONE);
}

void ServiceGPIOPUForm::checkKeys()
{
    GPIOInput pressed = _gpioMatrix ? _gpioMatrix->keyPressed : GPIOInput::IN_NONE;

    // панель «Кнопка»: код и имя нажатой кнопки
    if (pressed == GPIOInput::IN_NONE)
        _mainWindow->getView()->setText(ui->label_keyValue, "—");
    else
        _mainWindow->getView()->setText(ui->label_keyValue, QString::number(static_cast<int>(pressed)) + "  " + keyName(pressed));

    if (!keyboardMode)
        return;

    // нажатая кнопка - зеленая
    for (int i = 0; i < keys.size(); i++)
    {
        bool on = keys[i]->property("key").toInt() == static_cast<int>(pressed);
        _mainWindow->getView()->setStyle(keys[i], on
            ? "border:none;border-radius:4px;background:rgb(71,169,73);color:rgb(255,255,255);font: bold 9px \"Montserrat\";"
            : "border:none;border-radius:4px;background:rgb(22,28,34);color:rgb(230,232,234);font: bold 9px \"Montserrat\";");
    }
}

QString ServiceGPIOPUForm::keyName(GPIOInput key)
{// имя кнопки в одну строку - берем подпись с экрана
    for (int i = 0; i < keys.size(); i++)
        if (keys[i]->property("key").toInt() == static_cast<int>(key))
            return keys[i]->text().replace('\n', ' ');
    return "";
}

void ServiceGPIOPUForm::setKeyboardMode(bool on)
{// режим клавиатуры: вместо GPIO слева сетка кнопок
    keyboardMode = on;
    ui->frame_pins->setVisible(!on);
    ui->frame_keyboard->setVisible(on);
    _mainWindow->getView()->setStyle(ui->pushButton_keyboard, QString("border-style:none;outline: none;background-image:url(" GPIO_PU_IMG)
                                     + (on ? "button_keyboard_on.png);" : "button_keyboard_off.png);"));
    if (!on && _gpioMatrix)
        _gpioMatrix->setKeyOverride(GPIOInput::IN_NONE);
    checkKeys();
}

void ServiceGPIOPUForm::on_pushButton_keyboard_clicked()
{
    setKeyboardMode(!keyboardMode);
}

// ---------------------------------------------------------------- CAN

void ServiceGPIOPUForm::initCan(int channel, QString canName, QLabel *indicator, QLabel *info, QPushButton *pause, QFrame *list)
{
    canBuffer[channel] = QSharedPointer<CanTraceBuffer>(new CanTraceBuffer());
    canListener[channel] = new CanTraceListener(canName, canBuffer[channel]);
    canIndicator[channel] = indicator;
    canInfo[channel] = info;
    canPause[channel] = pause;
    canLastState[channel] = -1;

    // листинг рисуем поверх рамки из .ui
    canView[channel] = new CanTraceView(list);
    canView[channel]->setGeometry(0, 0, list->width(), list->height());
    canView[channel]->setBuffer(canBuffer[channel], openTimeUs);
}

void ServiceGPIOPUForm::checkCan()
{
    for (int i = 0; i < 2; i++)
    {
        int state = canListener[i]->getState();
        if (state != canLastState[i])
        {// индикатор: зеленый - ERROR-ACTIVE, желтый - WARNING/PASSIVE, красный - остальное
            canLastState[i] = state;
            QString image = "indication_can_red";
            if (state == CanTraceListener::BusActive)
                image = "indication_can_green";
            else if (state == CanTraceListener::BusWarning || state == CanTraceListener::BusPassive)
                image = "indication_can_yellow";
            _mainWindow->getView()->setStyle(canIndicator[i], "background-image:url(" GPIO_PU_IMG + image + ".png);");

            canView[i]->setEmptyText(state == CanTraceListener::BusNoInterface
                                     ? "нет интерфейса " + canListener[i]->canName
                                     : QString("нет кадров"));
        }

        // подпись: интерфейс, состояние или битрейт, потери
        QString text = canListener[i]->canName;
        switch (state)
        {
        case CanTraceListener::BusNoInterface: text += " · нет"; break;
        case CanTraceListener::BusDown:        text += " · down"; break;
        case CanTraceListener::BusWarning:     text += " · warning"; break;
        case CanTraceListener::BusPassive:     text += " · passive"; break;
        case CanTraceListener::BusOff:         text += " · bus-off"; break;
        default:
            if (canListener[i]->getBitrate())
                text += QString(" · %1k").arg(canListener[i]->getBitrate() / 1000);
            break;
        }
        if (canListener[i]->getDrops())
            text += QString(" · потери %1").arg(canListener[i]->getDrops());
        _mainWindow->getView()->setText(canInfo[i], text);

        canView[i]->refresh();
    }
}

void ServiceGPIOPUForm::setPaused(int channel, bool paused)
{// пауза только останавливает экран, прием в буфер идет дальше
    canView[channel]->setPaused(paused);
    _mainWindow->getView()->setStyle(canPause[channel], QString("border-style:none;outline: none;background-image:url(" GPIO_PU_IMG)
                                     + (paused ? "button_pause_on.png);" : "button_pause_off.png);"));
}

void ServiceGPIOPUForm::on_pushButton_can1Pause_clicked()
{
    setPaused(0, !canView[0]->isPaused());
}

void ServiceGPIOPUForm::on_pushButton_can2Pause_clicked()
{
    setPaused(1, !canView[1]->isPaused());
}

void ServiceGPIOPUForm::can1PauseRequested()
{// начали листать без паузы
    setPaused(0, true);
}

void ServiceGPIOPUForm::can2PauseRequested()
{
    setPaused(1, true);
}

void ServiceGPIOPUForm::on_pushButton_saveCan_clicked()
{// «↑CAN»: оба буфера в <каталог программы>/can_logs/CANn_ГГММДД_ччммсс.trc
    if (saving)
        return;
    saving = true;
    _mainWindow->getView()->setStyle(ui->pushButton_saveCan, "border-style:none;outline: none;background-image:url(" GPIO_PU_IMG "button_can_on.png);");
    showStatus("Сохранение CAN...");

    QString dir = QCoreApplication::applicationDirPath() + "/can_logs/";
    QString stamp = QDateTime::currentDateTime().toString("yyMMdd_hhmmss");
    QList<TrcWriter::Job> jobs;
    for (int i = 0; i < 2; i++)
    {
        TrcWriter::Job job;
        job.buffer = canBuffer[i];// буфер общий - запись доживет, даже если экран закроют
        job.fileName = dir + QString("CAN%1_").arg(i + 1) + stamp + ".trc";
        job.canName = canListener[i]->canName;
        job.bitrate = canListener[i]->getBitrate();
        jobs.append(job);
    }

    // пишет в своем треде, результат придет в saveFinished()
    TrcWriter *writer = new TrcWriter(jobs);
    connect(writer, &TrcWriter::finished, this, &ServiceGPIOPUForm::saveFinished);
}

void ServiceGPIOPUForm::saveFinished(bool ok, QString message)
{
    saving = false;
    _mainWindow->getView()->setStyle(ui->pushButton_saveCan, "border-style:none;outline: none;background-image:url(" GPIO_PU_IMG "button_can_off.png);");
    showStatus((ok ? "Сохранено: " : "") + message, !ok);
}

// ---------------------------------------------------------------- общее

void ServiceGPIOPUForm::mainProgress()
{
    if (!isVisible())
        return;

    if (!keyboardMode)
        checkPins();
    checkKeys();
    checkCan();
}

void ServiceGPIOPUForm::temperatureProgress()
{// температура SoC
    QFile file("/sys/class/thermal/thermal_zone0/temp");
    if (file.open(QIODevice::ReadOnly))
    {
        bool ok = false;
        int milli = file.readAll().trimmed().toInt(&ok);
        if (ok)
        {
            _mainWindow->getView()->setText(ui->label_temperature, QString("%1 C°").arg(qRound(milli / 1000.0)));
            return;
        }
    }
    _mainWindow->getView()->setText(ui->label_temperature, "-- C°");
}

void ServiceGPIOPUForm::showStatus(QString text, bool error)
{// строка статуса справа от температуры, гаснет через 15 с
    _mainWindow->getView()->setStyle(ui->label_status, error
        ? "color: rgb(240,74,74); font: 13px \"Montserrat\"; background: transparent;"
        : "color: rgb(150,156,162); font: 13px \"Montserrat\"; background: transparent;");
    ui->label_status->setText(QFontMetrics(ui->label_status->font()).elidedText(text, Qt::ElideMiddle, ui->label_status->width()));
    statusTimer.start(15000);
}

void ServiceGPIOPUForm::on_pushButton_exit_clicked()
{
    close();// деструктор вернет GPIO логике и остановит прием CAN
}
