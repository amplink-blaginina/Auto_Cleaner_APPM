// serviceGPIOPUform — экран «GPIO TESTING ПУ»: GPIO пульта, температура, листинг CAN, клавиатура
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28; RPi 4B, Raspbian 12 armhf, Qt 5.15.8,
//         libgpiod 1.6.3, SocketCAN
// изменения: переписан под стиль проекта - разметка в serviceGPIOPUform.ui, слоты on_..._clicked,
//         таймер mainProgress, стили через ViewController; поведение то же, что в версии 1
#ifndef SERVICEGPIOPUFORM_H
#define SERVICEGPIOPUFORM_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QElapsedTimer>
#include <QMap>
#include <QSharedPointer>
#include <gpiod.hpp>

#include <can/mycan.h>
#include "gpio_types.hpp"
#include "cantracebuffer.h"
#include "cantracelistener.h"
#include "cantraceview.h"

class MainWindow;
class GPIOWorker;
class GPIOMatrix;

namespace Ui {
class ServiceGPIOPUForm;
}

// запускается из настроек как «GPIO БУЦ»: new ServiceGPIOPUForm(_can, _mainWindow, _parent), show(), raise();
// удаляется сама при закрытии.
// пока экран открыт: логика машины в выходы не пишет (тестовый режим GPIOWorker),
// входы и кнопки клавиатуры можно подменять; при выходе все возвращается логике
class ServiceGPIOPUForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceGPIOPUForm(MyCan* can, MainWindow* mainWindow, QWidget *parent = nullptr);
    ~ServiceGPIOPUForm();

    // кто владеет линией GPIO
    enum PinKind
    {
        PinWorker = 0,      // GPIOWorker - входы пульта и выходы, ими можно управлять
        PinMatrix,          // GPIOMatrix - клавиатура, только показываем
        PinFree,            // никем не занята - запрашиваем сами как вход, только показываем
        PinUnavailable      // занята кем-то еще
    };

    QTimer mainProgressTimer;       // 100 мс: GPIO, клавиатура, CAN
    QTimer temperatureTimer;        // 1 с: температура
    QTimer statusTimer;             // гасит строку статуса

    MainWindow *_mainWindow;

private slots:
    void mainProgress();
    void temperatureProgress();

    void pinPressed();
    void pinReleased();
    void keyPressed();
    void keyReleased();

    void can1PauseRequested();
    void can2PauseRequested();
    void saveFinished(bool ok, QString message);

    void on_pushButton_exit_clicked();
    void on_pushButton_keyboard_clicked();
    void on_pushButton_saveCan_clicked();
    void on_pushButton_can1Pause_clicked();
    void on_pushButton_can2Pause_clicked();

private:
    void addPin(QPushButton *button, int pin);
    void addKey(QPushButton *button, int row, int col);
    void initCan(int channel, QString canName, QLabel *indicator, QLabel *info, QPushButton *pause, QFrame *list);

    void checkPins();
    void checkKeys();
    void checkCan();
    void setPaused(int channel, bool paused);
    void setKeyboardMode(bool on);
    void showStatus(QString text, bool error = false);
    QString keyName(GPIOInput key);

    Ui::ServiceGPIOPUForm *ui;
    MyCan *_can;
    QWidget *_parent;
    GPIOWorker *_gpio;
    GPIOMatrix *_gpioMatrix;

    // GPIO
    QList<QPushButton*> pins;           // у кнопки свойства "pin" и "kind"
    QElapsedTimer pinPressTimer;        // долгое нажатие на вход - снять подмену
    gpiod::chip freeChip;               // для свободных линий
    QMap<int, gpiod::line> freeLines;

    // клавиатура
    QList<QPushButton*> keys;           // у кнопки свойство "key"
    bool keyboardMode;

    // CAN: 0 - CAN 1 (Global/canDeivce, линия БУЦ), 1 - CAN 2 (Global/j1939Deivce)
    QSharedPointer<CanTraceBuffer> canBuffer[2];
    CanTraceListener *canListener[2];
    CanTraceView *canView[2];
    QLabel *canIndicator[2];
    QLabel *canInfo[2];
    QPushButton *canPause[2];
    int canLastState[2];
    qint64 openTimeUs;                  // время открытия экрана - от него секунды в листинге
    bool saving;
};

#endif // SERVICEGPIOPUFORM_H
